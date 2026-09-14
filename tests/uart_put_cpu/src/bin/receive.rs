//! Byte-state equivalence at the linked parser boundary. Stock effects are
//! recorded at their existing call boundary, not replaced by guessed semantics.
use ez80::{Cpu, Machine, Reg16};
use std::{cell::Cell, env, fs, path::Path};
const SP: u32 = 0xaf000;
const EXIT: u32 = 0xa0000;
struct Image {
    data: Vec<u8>,
    entry: u32,
    clock: u32,
    fields: Vec<u32>,
    owned: u32,
    fault: u32,
    source: u32,
    hooks: Vec<(u32, &'static str)>,
}
impl Image {
    fn load(path: &str) -> Self {
        let p = Path::new(path);
        let nm = fs::read_to_string(p.join("nm.txt")).unwrap();
        let addr = |n: &str| {
            let l = nm
                .lines()
                .find(|l| l.split_whitespace().last() == Some(n))
                .expect(n);
            u32::from_str_radix(l.split_whitespace().next().unwrap(), 16).unwrap()
        };
        let data = fs::read(p.join("MOS.bin")).unwrap();
        let c = addr("_emos_keyboard_clock") as usize;
        assert_eq!(data[c], 0x3a);
        let clock = u32::from_le_bytes([data[c + 1], data[c + 2], data[c + 3], 0]);
        let fields = if nm.contains("_emos_key_rx") {
            let r = addr("_emos_key_rx");
            vec![r, r + 1, r + 2, r + 3, r + 4, r + 5, r + 6]
        } else {
            [
                "_state",
                "_command",
                "_length",
                "_remaining",
                "_used",
                "_partial_at",
                "_payload",
            ]
            .iter()
            .map(|n| addr(n))
            .collect()
        };
        Self {
            data,
            entry: addr("_emos_keyboard_byte"),
            clock,
            fields,
            owned: addr("_uart1_keyboard_owned"),
            fault: addr("_emos_key_faulted"),
            source: addr("_emos_key_source"),
            hooks: [
                "_emos_keyboard_fault",
                "_emos_keyboard_effect",
                "_emos_keyboard_settings",
                "_emos_sdlink_packet",
                "_emos_console_packet",
            ]
            .iter()
            .map(|&s| (addr(s), s))
            .collect(),
        }
    }
}
struct Board {
    mem: Vec<u8>,
    clock: u32,
    reads: Cell<usize>,
}
impl Machine for Board {
    fn peek(&self, a: u32) -> u8 {
        if a == self.clock {
            self.reads.set(self.reads.get() + 1);
            42
        } else {
            self.mem[a as usize]
        }
    }
    fn poke(&mut self, a: u32, v: u8) {
        self.mem[a as usize] = v;
    }
    fn use_cycles(&self, _: i32) {}
    fn port_in(&mut self, a: u16) -> u8 {
        panic!("unexpected port IN {a:x}")
    }
    fn port_out(&mut self, a: u16, _: u8) {
        panic!("unexpected port OUT {a:x}")
    }
}
#[derive(Debug, PartialEq)]
struct Result {
    state: Vec<u8>,
    payload: Vec<u8>,
    fault: u8,
    hooks: Vec<(&'static str, Vec<u8>)>,
    reads: usize,
}
fn board(im: &Image) -> Board {
    let mut b = Board {
        mem: vec![0; 0x100000],
        clock: im.clock,
        reads: Cell::new(0),
    };
    b.mem[..im.data.len()].copy_from_slice(&im.data);
    b
}
fn reset(b: &mut Board, im: &Image) {
    b.mem[im.owned as usize] = 1;
    b.mem[im.fault as usize] = 0;
    b.mem[im.source as usize] = 2;
    for a in &im.fields[..6] {
        b.mem[*a as usize] = 0;
    }
    b.mem[im.fields[6] as usize..im.fields[6] as usize + 240].fill(0);
}
fn byte(
    im: &Image,
    b: &mut Board,
    c: &mut Cpu,
    initial: &ez80::Registers,
    value: u8,
) -> (Result, usize) {
    c.state.reg = initial.clone();
    c.state.halted = false;
    c.state.nmi_pending = false;
    c.state.reset_pending = false;
    c.state.index = Reg16::HL;
    c.state.displacement = 0;
    c.state.clear_sz_prefix();
    c.state.cached_instruction = false;
    c.state.instructions_executed = 0;
    c.state.reg.adl = true;
    c.state.reg.pc = im.entry;
    c.state.reg.set24(Reg16::SP, SP);
    c.state.reg.set24(Reg16::IX, 0xabc123);
    c.state.reg.set24(Reg16::IY, 0xbcd234);
    b._poke24(SP, EXIT);
    b._poke24(SP + 3, 0xaabb00 | value as u32);
    b.reads.set(0);
    let mut hooks = vec![];
    let mut steps = 0;
    while c.state.reg.pc != EXIT {
        if let Some((_, name)) = im.hooks.iter().find(|(a, _)| *a == c.state.reg.pc) {
            let sp = c.state.reg.get24(Reg16::SP);
            let args = match *name {
                "_emos_keyboard_fault" => {
                    b.mem[im.fault as usize] = 1;
                    vec![]
                }
                "_emos_keyboard_effect" | "_emos_keyboard_settings" => {
                    let p = b._peek24(sp + 3) as usize;
                    let n = if *name == "_emos_keyboard_effect" {
                        4
                    } else {
                        5
                    };
                    let data = b.mem[p..p + n].to_vec();
                    if *name == "_emos_keyboard_effect" {
                        b.mem[p] = 0x33;
                    }
                    data
                }
                "_emos_sdlink_packet" => {
                    let p = b._peek24(sp + 3) as usize;
                    let n = b.mem[(sp + 6) as usize] as usize;
                    b.mem[p..p + n].to_vec()
                }
                "_emos_console_packet" => {
                    let cmd = b.mem[(sp + 3) as usize];
                    let p = b._peek24(sp + 6) as usize;
                    let n = b.mem[(sp + 9) as usize] as usize;
                    let mut v = vec![cmd];
                    v.extend_from_slice(&b.mem[p..p + n]);
                    v
                }
                _ => unreachable!(),
            };
            hooks.push((*name, args));
            c.state.reg.pc = b._peek24(sp);
            c.state.reg.set24(Reg16::SP, sp + 3);
        } else {
            c.execute_instruction(b);
        }
        steps += 1;
        assert!(steps < 4000, "no return {:x}", c.state.reg.pc);
        assert!(!c.state.reg.iff1, "parser enabled interrupts");
    }
    assert_eq!(c.state.reg.get24(Reg16::SP), SP + 3);
    assert_eq!(c.state.reg.get24(Reg16::IX), 0xabc123);
    (
        Result {
            state: im.fields[..6].iter().map(|a| b.mem[*a as usize]).collect(),
            payload: b.mem[im.fields[6] as usize..im.fields[6] as usize + 240].to_vec(),
            fault: b.mem[im.fault as usize],
            hooks,
            reads: b.reads.get(),
        },
        steps,
    )
}
fn main() {
    let args: Vec<_> = env::args().collect();
    assert_eq!(args.len(), 3);
    let a = Image::load(&args[1]);
    let b = Image::load(&args[2]);
    let mut ba = board(&a);
    let mut bb = board(&b);
    let mut ca = Cpu::new_ez80();
    let mut cb = Cpu::new_ez80();
    let initial = ca.state.reg.clone();
    let mut count = 0;
    let mut steps = [0usize; 2];
    let mut compare = |bytes: &[u8], owned: u8, fault: u8| {
        reset(&mut ba, &a);
        reset(&mut bb, &b);
        ba.mem[a.owned as usize] = owned;
        bb.mem[b.owned as usize] = owned;
        ba.mem[a.fault as usize] = fault;
        bb.mem[b.fault as usize] = fault;
        for &v in bytes {
            let (ra, sa) = byte(&a, &mut ba, &mut ca, &initial, v);
            let (rb, sb) = byte(&b, &mut bb, &mut cb, &initial, v);
            assert!(
                ra == rb,
                "input {v:02x}, packet prefix {:?}; baseline {ra:?}, candidate {rb:?}",
                &bytes[..bytes.len().min(8)]
            );
            steps[0] += sa;
            steps[1] += sb;
            count += 1;
        }
    };
    for command in 0..=255u8 {
        for len in 0..=255u8 {
            let mut bytes = vec![command, len];
            bytes.extend((0..len as usize).map(|i| (i as u8).wrapping_mul(73)));
            bytes.extend_from_slice(&[0x80, 1, 0x34]);
            compare(&bytes, 1, 0);
        }
    }
    for owned in [0, 1, 255] {
        for fault in [0, 1, 255] {
            compare(
                &[
                    0x81, 4, 0, 0, 1, 1, 0x88, 5, 1, 2, 3, 4, 5, 0x8d, 3, 0x80, 0x81, 0xff,
                ],
                owned,
                fault,
            );
        }
    }
    // Concatenated valid events, callback mutation, bad key value and body sizes.
    compare(
        &[
            0x81, 4, 65, 0, 1, 1, 0x81, 4, 0, 0, 1, 0, 0x81, 4, 0, 0, 249, 1, 0x80, 1, 0,
        ],
        1,
        0,
    );
    println!("PASS {count} linked parser byte comparisons, all 256 header/length values, bounded stores, call arguments, callback mutation, owner/fault, stack/IX and masked IRQs; interpreted instructions C={}, candidate={}",steps[0],steps[1]);
}
