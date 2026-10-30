//! Execute the linked controller gateway/parser and complete UART IRQ with
//! bounded synthetic arrivals. This checks instruction boundaries, not physical
//! UART timing. --legacy preserves the frozen CRC-bearing failure fixture;
//! it never calculates a checksum or treats that ABI as a current interface.
use ez80::{Cpu, Machine, Reg16};
use std::{collections::{HashMap, VecDeque}, env, fs, path::Path};
const SP: u32 = 0xaff00;
const EXIT: u32 = 0xa0000;
const REQ: u32 = 0x40000;
const INPUT: u32 = 0x40100;
const OUTPUT: u32 = 0x40200;
const OK: u32 = 0;
const INVALID: u32 = 19;
const BUSY: u32 = 31;
const UNSAFE: u32 = 32;
const UNAVAILABLE: u32 = 35;
const OPEN: u8 = 4;
const RECEIVE: u8 = 5;
const SEND: u8 = 6;
const CLOSE: u8 = 7;

struct Board {
    mem: Vec<u8>, sent: Vec<Vec<u8>>, incoming: VecDeque<u8>, fifo: VecDeque<u8>,
    overrun: bool, pcdr: u8, inject: bool, max_masked: usize, period: usize,
    iff: bool, watch_copy: bool, copy_writes: usize, send_result: u8,
    copy_expiry: Option<u32>, irq_entries: usize,
}
impl Machine for Board {
    fn peek(&self, a: u32) -> u8 { self.mem[a as usize] }
    fn poke(&mut self, a: u32, v: u8) {
        if self.watch_copy && (OUTPUT..OUTPUT + 96).contains(&a) {
            assert!(self.iff, "raw controller copy masked interrupts at {a:x}");
            self.copy_writes += 1;
            if self.copy_writes == 32 {
                if let Some(clock) = self.copy_expiry.take() {
                    self.mem[clock as usize] = self.mem[clock as usize].wrapping_add(30);
                }
            }
        }
        self.mem[a as usize] = v;
    }
    fn use_cycles(&self, _: i32) {}
    fn port_in(&mut self, a: u16) -> u8 {
        match a {
            0x9e => self.pcdr,
            0xd5 => if self.overrun { 0x62 } else if !self.fifo.is_empty() { 0x61 } else { 0x60 },
            0xd0 => self.fifo.pop_front().expect("UART read needs a byte"),
            _ => panic!("unexpected port read {a:x}"),
        }
    }
    fn port_out(&mut self, a: u16, v: u8) {
        match a { 0x9e => self.pcdr = v, 0xd1 => {}, _ => panic!("unexpected port write {a:x}") }
    }
}
fn sym(s: &HashMap<String, u32>, n: &str) -> u32 { *s.get(n).expect(n) }
fn call(b: &mut Board, s: &HashMap<String, u32>, name: &str, args: &[u32], irq: bool) -> u32 {
    let mut c = Cpu::new_ez80();
    c.state.reg.adl = true; c.state.reg.madl = true;
    c.state.reg.pc = sym(s, name); c.state.reg.set24(Reg16::SP, SP);
    c.state.reg.set24(Reg16::IX, 0xab1234); c.state.reg.set24(Reg16::IY, 0xbc1234);
    c.state.reg.iff1 = irq; c.state.reg.iff2 = irq;
    b._poke24(SP, EXIT);
    for (i, &a) in args.iter().enumerate() { b._poke24(SP + 3 + i as u32 * 3, a); }
    let (mut steps, mut arrival, mut started, mut masked) = (0, 0, false, 0);
    b.max_masked = 0; b.copy_writes = 0; b.irq_entries = 0;
    while c.state.reg.pc != EXIT {
        b.iff = c.state.reg.iff1;
        if c.state.reg.pc == sym(s, "_emos_keyboard_send") {
            // Fixed serializer boundary only: gateway, packet parser, copy,
            // locks and complete keyboard IRQ execute their linked bytes.
            let sp = c.state.reg.get24(Reg16::SP);
            let at = b._peek24(sp + 3) as usize;
            let n = b._peek24(sp + 6) as usize;
            b.sent.push(b.mem[at..at + n].to_vec());
            c.state.reg.set16(Reg16::AF, (b.send_result as u16) << 8);
            c.state.reg.pc = b._peek24(sp); c.state.reg.set24(Reg16::SP, sp + 3);
        } else { c.execute_instruction(b); }
        steps += 1;
        if c.state.reg.iff1 { masked = 0; } else { masked += 1; b.max_masked = b.max_masked.max(masked); }
        if b.inject {
            if !c.state.reg.iff1 { started = true; }
            if started {
                arrival += 1;
                if arrival % b.period == 0 && b.pcdr & 4 == 0 {
                    if let Some(byte) = b.incoming.pop_front() {
                        if b.fifo.len() == 16 { b.overrun = true; } else { b.fifo.push_back(byte); }
                    }
                }
            }
            if c.state.reg.iff1 && (!b.fifo.is_empty() || b.overrun) && b.peek(sym(s, "_emos_key_faulted")) == 0 {
                let sp = c.state.reg.get24(Reg16::SP) - 4;
                b.poke(sp, 3); b._poke24(sp + 1, c.state.reg.pc);
                c.state.reg.set24(Reg16::SP, sp); c.state.reg.pc = sym(s, "_emos_keyboard_irq_entry");
                c.state.reg.iff1 = false; c.state.reg.iff2 = false; b.irq_entries += 1;
            }
        }
        assert!(steps < 1_000_000, "stuck {name} {:x}", c.state.reg.pc);
    }
    assert_eq!(c.state.reg.get24(Reg16::SP), SP + 3, "SP {name}");
    assert_eq!(c.state.reg.get24(Reg16::IX), 0xab1234, "IX {name}");
    assert_eq!(c.state.reg.iff1, irq, "IFF1 {name}");
    assert_eq!(c.state.reg.iff2, irq, "IFF2 {name}");
    let result = c.state.reg.get24(Reg16::HL);
    if name != "_emos_keyboard_byte" {
        println!("{name}: HL={result} instructions={steps} max_masked={} raw_stores={} IRQs={}", b.max_masked, b.copy_writes, b.irq_entries);
    }
    result
}
fn prepare(b: &mut Board, input: &[u8], output: u32, capacity: u32) {
    b.mem[REQ as usize..REQ as usize + 66].fill(0);
    b.poke(REQ, 66); b.poke(REQ + 2, 1); b.poke(REQ + 4, 3); b.poke(REQ + 5, 10); b.poke(REQ + 6, 2);
    b._poke24(REQ + 10, INPUT); b._poke24(REQ + 13, input.len() as u32);
    b._poke24(REQ + 16, output); b._poke24(REQ + 19, capacity);
    b.mem[INPUT as usize..INPUT as usize + input.len()].copy_from_slice(input);
    b.mem[REQ as usize + 25..REQ as usize + 28].copy_from_slice(b"ext");
    b.mem[REQ as usize + 41..REQ as usize + 51].copy_from_slice(b"controller");
}
fn gateway(b: &mut Board, s: &HashMap<String, u32>, irq: bool) -> u32 { call(b, s, "_emos_gateway", &[REQ], irq) }
fn service(b: &mut Board, s: &HashMap<String, u32>, op: u8) -> u32 {
    let capacity = match op { OPEN => 4, RECEIVE => 98, _ => 0 };
    prepare(b, &[op], if capacity == 0 { 0 } else { OUTPUT }, capacity);
    gateway(b, s, true)
}
fn open(b: &mut Board, s: &HashMap<String, u32>) -> u32 {
    assert_eq!(service(b, s, OPEN), OK);
    assert_eq!(b._peek24(REQ + 22), 4);
    let token = u32::from_le_bytes(b.mem[OUTPUT as usize..OUTPUT as usize + 4].try_into().unwrap());
    assert!(token > 0 && token < 0x1000000, "boot-local nonzero token {token:x}"); token
}
fn request(token: u32, query: u16, acknowledgement: u32, operation: u8) -> [u8; 20] {
    let mut p = [0; 20];
    p[..8].copy_from_slice(&[SEND, 23, 0, 0xf7, b'C', b'I', 2, operation]);
    p[8..12].copy_from_slice(&token.to_le_bytes());
    p[12..16].copy_from_slice(&acknowledgement.to_le_bytes());
    p[16..18].copy_from_slice(&query.to_le_bytes()); p
}
fn send(b: &mut Board, s: &HashMap<String, u32>, token: u32, query: u16, ack: u32, op: u8) -> u32 {
    prepare(b, &request(token, query, ack, op), 0, 0); gateway(b, s, true)
}
fn reply(token: u32, query: u16, seed: u8) -> Vec<u8> {
    let mut p: Vec<u8> = (0..96).map(|i| (i as u8).wrapping_mul(37).wrapping_add(seed)).collect();
    p[..4].copy_from_slice(b"CIS2"); p[4..8].copy_from_slice(&token.to_le_bytes()); p[8..10].copy_from_slice(&query.to_le_bytes());
    // Deliberately invalid controller semantics: the resident endpoint must
    // preserve these bytes for application validation, without interpreting
    // flags, age, reserved words, source identity or transition records.
    p[10] = 0xfe; p[11] = 0xff; p
}
fn deliver(b: &mut Board, s: &HashMap<String, u32>, p: &[u8]) {
    for byte in [0x8e, p.len() as u8].iter().chain(p.iter()) { call(b, s, "_emos_keyboard_byte", &[*byte as u32], false); }
}
fn receive(b: &mut Board, s: &HashMap<String, u32>, p: &[u8], rtt: u8, age: u8) {
    b.watch_copy = true;
    b.mem[OUTPUT as usize - 1..OUTPUT as usize + 99].fill(0xcc);
    assert_eq!(service(b, s, RECEIVE), OK);
    b.watch_copy = false;
    assert_eq!(b._peek24(REQ + 22), 98);
    assert_eq!(&b.mem[OUTPUT as usize..OUTPUT as usize + 96], p);
    assert_eq!(&b.mem[OUTPUT as usize + 96..OUTPUT as usize + 98], &[rtt, age]);
    assert_eq!(b.peek(OUTPUT - 1), 0xcc); assert_eq!(b.peek(OUTPUT + 98), 0xcc);
    assert_eq!(b.copy_writes, 96, "all raw bytes copied with interrupts enabled");
    assert!(b.max_masked < 512, "controller critical section too long");
    assert_eq!(service(b, s, RECEIVE), BUSY, "reply consumption must be one-shot");
    assert_eq!(b._peek24(REQ + 22), 0);
}
fn transport_checks(b: &mut Board, s: &HashMap<String, u32>) {
    // Old input operation values cannot accidentally invoke the new service.
    for op in 0..4 { assert_eq!(service(b, s, op), INVALID); }
    assert!(b.sent.is_empty());
    prepare(b, &[OPEN], OUTPUT, 4);
    b.poke(sym(s, "_emosPolicy"), 0); assert_eq!(gateway(b, s, true), UNSAFE); b.poke(sym(s, "_emosPolicy"), 3);
    b.poke(sym(s, "_emosBusy"), 1); assert_eq!(gateway(b, s, true), BUSY); b.poke(sym(s, "_emosBusy"), 0);
    assert_eq!(gateway(b, s, false), BUSY);
    for address in [0, 0x3ffff, 0xb0000, 0xffffff] {
        prepare(b, &[OPEN], OUTPUT, 4); b._poke24(REQ + 10, address);
        assert_eq!(gateway(b, s, true), INVALID, "input {address:x}");
    }
    for address in [0, 0x3ffff, 0xafffd, 0xffffff] {
        prepare(b, &[OPEN], address, 4); assert_eq!(gateway(b, s, true), INVALID, "output {address:x}");
    }
    prepare(b, &[OPEN], OUTPUT, 4);
    let header = b.mem[REQ as usize..REQ as usize + 66].to_vec();
    for address in [0x3ffff, 0xaffff, 0xb0000] {
        b.mem[address as usize..address as usize + 66].copy_from_slice(&header);
        assert_eq!(call(b, s, "_emos_gateway", &[address], true), UNSAFE, "request {address:x}");
    }
    for (input, output, capacity) in [(vec![OPEN, 0], OUTPUT, 4), (vec![OPEN], OUTPUT, 3), (vec![RECEIVE], OUTPUT, 96), (vec![SEND; 19], 0, 0), (vec![CLOSE], OUTPUT, 1)] {
        prepare(b, &input, output, capacity); assert_eq!(gateway(b, s, true), INVALID);
    }
    assert!(b.sent.is_empty());
    let token = open(b, s);
    assert_eq!(service(b, s, OPEN), BUSY);
    assert_eq!(service(b, s, RECEIVE), BUSY);
    for (offset, byte) in [(1, 22), (2, 1), (3, 0xf6), (4, b'X'), (5, b'X'), (6, 1), (7, 0), (7, 3), (8, token as u8 ^ 1), (16, 0)] {
        let mut p = request(token, 1, 0x12345678, 1); p[offset] = byte;
        prepare(b, &p, 0, 0); assert_eq!(gateway(b, s, true), INVALID, "envelope byte {offset}");
    }
    assert!(b.sent.is_empty(), "invalid calls must not reach the serializer");
    assert_eq!(send(b, s, token, 1, 0x12345678, 1), OK);
    assert_eq!(b.sent.last().unwrap(), &request(token, 1, 0x12345678, 1)[1..]);
    assert_eq!(send(b, s, token, 2, 0, 1), BUSY);
    assert_eq!(service(b, s, RECEIVE), BUSY);
    let p = reply(token, 1, 7);
    deliver(b, s, &p[..95]); assert_eq!(service(b, s, RECEIVE), BUSY);
    deliver(b, s, &reply(token + 1, 1, 7)); assert_eq!(service(b, s, RECEIVE), BUSY);
    deliver(b, s, &reply(token, 2, 7)); assert_eq!(service(b, s, RECEIVE), BUSY);
    b.poke(sym(s, "_clock"), 65); deliver(b, s, &p);
    deliver(b, s, &reply(token, 1, 99)); // READY storage cannot be overwritten.
    assert_eq!(send(b, s, token, 2, 0, 1), BUSY, "unread reply blocks a new request");
    b.poke(sym(s, "_clock"), 68); receive(b, s, &p, 5, 3);
    assert_eq!(send(b, s, token, 2, 0, 1), OK);
    let p = reply(token, 2, 11);
    b.incoming.extend([0x8e, 96]); b.incoming.extend(p.clone()); b.inject = true;
    assert_eq!(service(b, s, RECEIVE), BUSY, "pending RECEIVE must not expose the previous copy");
    b.inject = false;
    assert!(!b.overrun); assert_eq!(b.peek(sym(s, "_emos_key_faulted")), 0);
    let remaining: Vec<_> = b.fifo.drain(..).chain(b.incoming.drain(..)).collect();
    for byte in remaining { call(b, s, "_emos_keyboard_byte", &[byte as u32], false); }
    receive(b, s, &p, 0, 0);
    // RECEIVE is no longer a cached READ: publish another reply before asking
    // the complete IRQ to deliver Escape during the next unlocked raw copy.
    assert_eq!(send(b, s, token, 3, 0, 1), OK);
    let p = reply(token, 3, 19); deliver(b, s, &p);
    b.incoming.extend([0x81, 4, 27, 0, 27, 1]); b.period = 8; b.inject = true; b.watch_copy = true;
    assert_eq!(service(b, s, RECEIVE), OK); b.inject = false; b.watch_copy = false;
    assert!(b.irq_entries > 0, "Escape must traverse the actual linked IRQ");
    assert!(b.fifo.is_empty() && b.incoming.is_empty(), "whole Escape record must arrive during RECEIVE");
    assert_eq!(&b.mem[OUTPUT as usize..OUTPUT as usize + 96], p.as_slice()); assert_eq!(b.copy_writes, 96);
    assert!(!b.overrun); assert_eq!(b.peek(sym(s, "_emos_key_faulted")), 0);
    assert_eq!(b.peek(sym(s, "_keyascii")), 27); assert_eq!(b.peek(sym(s, "_keydown")), 1);
    assert_eq!(service(b, s, RECEIVE), BUSY);
    let sent = b.sent.len(); assert_eq!(service(b, s, CLOSE), OK); assert_eq!(b.sent.len(), sent, "CLOSE is local revocation");
    assert_eq!(service(b, s, RECEIVE), UNAVAILABLE);
    println!("PASS linked raw controller: admission/bounds, old-client refusal, exact envelope/correlation, one-shot raw copy and Escape through complete IRQ; SP/IX/IFF preserved");
}
fn lifetime_checks(b: &mut Board, s: &HashMap<String, u32>) {
    let clock = sym(s, "_clock");
    let mut token = open(b, s);
    b.poke(clock, 250); assert_eq!(send(b, s, token, 1, 0, 1), OK);
    b.poke(clock, 4); let p = reply(token, 1, 29); deliver(b, s, &p);
    b.poke(clock, 8); receive(b, s, &p, 10, 4);
    // Expiry after publication is rechecked after the unlocked copy; an expired
    // buffer must not be committed as a successful application delivery.
    assert_eq!(send(b, s, token, 2, 0, 1), OK); deliver(b, s, &reply(token, 2, 31));
    b.copy_expiry = Some(clock); b.watch_copy = true;
    assert_eq!(service(b, s, RECEIVE), UNAVAILABLE); b.watch_copy = false;
    assert_eq!(b._peek24(REQ + 22), 0); assert_eq!(service(b, s, RECEIVE), UNAVAILABLE);
    token = open(b, s);
    b.poke(clock, 100); assert_eq!(send(b, s, token, 1, 0, 1), OK);
    b.poke(clock, 130); call(b, s, "_emos_controller_tick", &[], false);
    b.poke(clock, 100); deliver(b, s, &reply(token, 1, 37));
    assert_eq!(service(b, s, RECEIVE), UNAVAILABLE, "tick expiry cannot revive across low-byte wrap");
    let old_token = token; token = open(b, s); assert_ne!(token, old_token);
    assert_eq!(send(b, s, token, 1, 0, 1), OK);
    deliver(b, s, &reply(old_token, 1, 39)); assert_eq!(service(b, s, RECEIVE), BUSY);
    let p = reply(token, 1, 41); deliver(b, s, &p); receive(b, s, &p, 0, 0);
    assert_eq!(send(b, s, token, 2, 0, 1), OK);
    b.poke(clock, 130); deliver(b, s, &reply(token, 2, 43));
    assert_eq!(service(b, s, RECEIVE), UNAVAILABLE, "delayed replies cannot establish a new lifetime");
    token = open(b, s); b.poke(clock, 140);
    assert_eq!(send(b, s, token, 1, 0, 1), OK); deliver(b, s, &reply(token, 1, 47));
    b.poke(clock, 170); call(b, s, "_emos_controller_tick", &[], false); b.poke(clock, 140);
    assert_eq!(service(b, s, RECEIVE), UNAVAILABLE, "unread reply expires permanently");
    token = open(b, s); b.send_result = 1;
    assert_eq!(send(b, s, token, 1, 0, 1), 15); b.send_result = 0;
    assert_eq!(service(b, s, RECEIVE), UNAVAILABLE, "failed serializer revokes admission");
    token = open(b, s); assert_eq!(send(b, s, token, 1, 0, 1), OK);
    assert_eq!(send(b, s, token, 2, 0, 2), OK, "admitted wire close can cancel a pending request");
    assert_eq!(service(b, s, CLOSE), OK);
    open(b, s); b.poke(sym(s, "_emos_key_faulted"), 1);
    let sent = b.sent.len(); assert_eq!(service(b, s, CLOSE), OK); assert_eq!(b.sent.len(), sent);
    b.poke(sym(s, "_emos_key_faulted"), 0); assert_eq!(service(b, s, RECEIVE), UNAVAILABLE);
    token = open(b, s); assert_eq!(send(b, s, token, 1, 0, 1), OK);
    call(b, s, "_emos_controller_reset", &[], false); deliver(b, s, &reply(token, 1, 53));
    assert_eq!(service(b, s, RECEIVE), UNAVAILABLE, "lifecycle reset discards abandoned replies");
    println!("PASS linked controller lifetime: raw timing across wrap, copy/request/cache expiry, no revival, token renewal, serializer failure, wire/local close and lifecycle reset");
}
fn legacy_service(b: &mut Board, s: &HashMap<String, u32>, op: u8) -> u32 {
    prepare(b, &[op], if op == 1 { OUTPUT } else { 0 }, if op == 1 { 98 } else { 0 }); gateway(b, s, true)
}
fn legacy_checks(b: &mut Board, s: &HashMap<String, u32>) {
    assert_eq!(legacy_service(b, s, 0), 0); assert_eq!(legacy_service(b, s, 1), 31); assert_eq!(legacy_service(b, s, 2), 0);
    assert_eq!(b.sent.len(), 1); println!("legacy request={:x?}", b.sent[0]);
    let mut p = vec![0u8; 98]; p[..4].copy_from_slice(b"CIS1"); p[4..8].copy_from_slice(&b.sent[0][7..11]);
    p[8..10].copy_from_slice(&b.sent[0][15..17]); p[10] = 3; p[12] = 1; p[20] = 2; p[28] = 1; p[36] = 100; p[44] = 7;
    p[96..].copy_from_slice(&[0x2a, 0x94]); // Frozen historical bytes, never a checksum calculation.
    deliver(b, s, &p); assert_eq!(legacy_service(b, s, 1), 0); assert_eq!(b.peek(OUTPUT + 10), 3); assert_eq!(b.peek(OUTPUT + 44), 7);
    assert_eq!(legacy_service(b, s, 2), 0); p[8] = 2; p[36] = 200;
    b.incoming.extend([0x8e, 98]); b.incoming.extend(p); b.inject = true;
    let read = legacy_service(b, s, 1); b.inject = false;
    println!("legacy overlap READ={read} FIFO-overrun={} EMOS-fault={} remaining={}", b.overrun, b.peek(sym(s, "_emos_key_faulted")), b.incoming.len());
    assert!(b.overrun); assert_eq!(b.peek(sym(s, "_emos_key_faulted")), 1); assert_eq!(legacy_service(b, s, 2), 35);
    println!("PASS legacy reproduction: reply overlap causes UART fault, REQUEST35 and keyboard loss");
}
fn main() {
    let args: Vec<_> = env::args().collect();
    assert!(args.len() == 2 || (args.len() == 3 && args[2] == "--legacy"), "IMAGE_DIR [--legacy]");
    let path = Path::new(&args[1]);
    let s: HashMap<String, u32> = fs::read_to_string(path.join("nm.txt")).unwrap().lines().filter_map(|l| {
        let v: Vec<_> = l.split_whitespace().collect();
        if v.len() != 3 { None } else { Some((v[2].to_string(), u32::from_str_radix(v[0], 16).unwrap())) }
    }).collect();
    let rom = fs::read(path.join("MOS.bin")).unwrap();
    let mut b = Board { mem: vec![0; 0x1000000], sent: vec![], incoming: VecDeque::new(), fifo: VecDeque::new(), overrun: false,
        pcdr: 0, inject: false, max_masked: 0, period: 256, iff: false, watch_copy: false, copy_writes: 0,
        send_result: 0, copy_expiry: None, irq_entries: 0 };
    b.mem[..rom.len()].copy_from_slice(&rom);
    b.poke(sym(&s, "_emosPolicy"), 3); b.poke(sym(&s, "_emos_key_source"), 2);
    b.poke(sym(&s, "_uart1_keyboard_owned"), 1); b.poke(sym(&s, "_clock"), 60);
    if args.len() == 3 { legacy_checks(&mut b, &s); } else { transport_checks(&mut b, &s); lifetime_checks(&mut b, &s); }
}
