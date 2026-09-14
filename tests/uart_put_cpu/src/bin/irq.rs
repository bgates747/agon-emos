//! Compare actual complete IRQ boundaries with controlled FIFO and C callbacks.
use ez80::{Cpu, Machine, Reg16};
use std::{env, fs, path::Path};
const SP: u32 = 0xaf000;
const EXIT: u32 = 0xa0000;
const SWAP: u32 = EXIT + 16;
#[derive(Clone, Debug)]
struct Case {
    pending: u8,
    owned: u8,
    fault: u8,
    n: usize,
    status_at: Option<usize>,
    status: u8,
    fault_after: Option<usize>,
    pcdr: u8,
}
#[derive(Clone, Debug, PartialEq)]
enum Event {
    Read(u16, u8),
    Write(u16, u8),
    Byte(u8),
    Fault,
    Async,
}
struct Image {
    data: Vec<u8>,
    entry: u32,
    owned: u32,
    fault: u32,
    byte: u32,
    fail: u32,
    async_fn: Option<u32>,
    async_length: Option<u32>,
}
impl Image {
    fn load(path: &str) -> Self {
        let p = Path::new(path);
        let nm = fs::read_to_string(p.join("nm.txt")).unwrap();
        let address = |s: &str| {
            nm.lines()
                .find(|l| l.split_whitespace().last() == Some(s))
                .map(|l| u32::from_str_radix(l.split_whitespace().next().unwrap(), 16).unwrap())
        };
        Self {
            data: fs::read(p.join("MOS.bin")).unwrap(),
            entry: address("_emos_keyboard_irq_entry").unwrap(),
            owned: address("_uart1_keyboard_owned").unwrap(),
            fault: address("_emos_key_faulted").unwrap(),
            byte: address("_emos_keyboard_byte").unwrap(),
            fail: address("_emos_keyboard_fault").unwrap(),
            async_fn: address("_emos_keyboard_async_irq"),
            async_length: address("_emos_keyboard_async_length")
                .or_else(|| address("_async_length")),
        }
    }
}
struct Board {
    mem: Vec<u8>,
    x: Case,
    pcdr: u8,
    lsr: usize,
    read: usize,
    events: Vec<Event>,
}
impl Machine for Board {
    fn peek(&self, a: u32) -> u8 {
        self.mem[a as usize]
    }
    fn poke(&mut self, a: u32, v: u8) {
        self.mem[a as usize] = v;
    }
    fn use_cycles(&self, _: i32) {}
    fn port_in(&mut self, a: u16) -> u8 {
        let v = match a {
            0x9e => self.pcdr,
            0xd5 => {
                let n = self.lsr;
                self.lsr += 1;
                if self.x.status_at == Some(n) {
                    self.x.status
                } else if self.read < self.x.n {
                    0x61
                } else {
                    0x60
                }
            }
            0xd0 => {
                assert!(self.read < self.x.n, "read empty FIFO");
                let v = (self.read as u8).wrapping_mul(73);
                self.read += 1;
                v
            }
            _ => panic!("IN {a:x}"),
        };
        self.events.push(Event::Read(a, v));
        v
    }
    fn port_out(&mut self, a: u16, v: u8) {
        assert_eq!(a, 0x9e);
        self.pcdr = v;
        self.events.push(Event::Write(a, v));
    }
}
fn swap(c: &mut Cpu, b: &mut Board) {
    c.state.reg.pc = SWAP;
    c.execute_instruction(b);
    c.execute_instruction(b);
}
fn run(im: &Image, x: &Case) -> (Vec<Event>, u8, usize) {
    let mut b = Board {
        mem: vec![0; 0x100000],
        x: x.clone(),
        pcdr: x.pcdr,
        lsr: 0,
        read: 0,
        events: vec![],
    };
    b.mem[..im.data.len()].copy_from_slice(&im.data);
    b.mem[im.owned as usize] = x.owned;
    b.mem[im.fault as usize] = x.fault;
    if let Some(a) = im.async_length {
        b.mem[a as usize] = x.pending;
    }
    b.mem[SWAP as usize] = 0xd9;
    b.mem[SWAP as usize + 1] = 0x08;
    b.mem[SP as usize] = 3; // MADL interrupt frame: mode byte, then 24-bit PC
    b._poke24(SP + 1, EXIT);
    let mut c = Cpu::new_ez80();
    c.state.reg.adl = true;
    c.state.reg.madl = true;
    let shadow = [
        (Reg16::BC, 0x123456),
        (Reg16::DE, 0x234567),
        (Reg16::HL, 0x345678),
    ];
    for (r, v) in shadow {
        c.state.reg.set24(r, v);
    }
    c.state.reg.set16(Reg16::AF, 0x4567);
    swap(&mut c, &mut b);
    let primary = [
        (Reg16::BC, 0x654321),
        (Reg16::DE, 0x765432),
        (Reg16::HL, 0x876543),
        (Reg16::IX, 0xabc123),
        (Reg16::IY, 0xbcd234),
    ];
    for (r, v) in primary {
        c.state.reg.set24(r, v);
    }
    c.state.reg.set16(Reg16::AF, 0x9876);
    c.state.reg.set24(Reg16::SP, SP);
    c.state.reg.iff1 = true;
    c.state.reg.iff2 = true;
    c.state.reg.pc = im.entry;
    let mut delivered = 0;
    let mut steps = 0;
    while c.state.reg.pc != EXIT {
        let pc = c.state.reg.pc;
        // Execute the real idle callee; only pending work is an effect boundary.
        if pc == im.byte || pc == im.fail || (x.pending != 0 && im.async_fn == Some(pc)) {
            assert!(!c.state.reg.iff1);
            let sp = c.state.reg.get24(Reg16::SP);
            if pc == im.byte {
                b.events.push(Event::Byte(b.mem[(sp + 3) as usize]));
                delivered += 1;
                if x.fault_after == Some(delivered) {
                    b.mem[im.fault as usize] = 1;
                }
            } else if pc == im.fail {
                b.events.push(Event::Fault);
                b.mem[im.fault as usize] = 1;
            } else {
                b.events.push(Event::Async);
            }
            // Legal C caller-clobbers plus shadow helper registers: outer IRQ owns all.
            for r in [Reg16::BC, Reg16::DE, Reg16::HL, Reg16::IY] {
                c.state.reg.set24(r, 0xdeadde);
            }
            c.state.reg.set16(Reg16::AF, 0xabcd);
            swap(&mut c, &mut b);
            for r in [Reg16::BC, Reg16::DE, Reg16::HL] {
                c.state.reg.set24(r, 0xdead12);
            }
            c.state.reg.set16(Reg16::AF, 0x1234);
            swap(&mut c, &mut b);
            c.state.reg.pc = b._peek24(sp);
            c.state.reg.set24(Reg16::SP, sp + 3);
        } else {
            let n = b.events.len();
            let enabled = c.state.reg.iff1;
            c.execute_instruction(&mut b);
            if n != b.events.len() {
                assert!(!enabled && !c.state.reg.iff1);
            }
        }
        steps += 1;
        assert!(steps < 5000, "no IRQ return {:x} {x:?}", c.state.reg.pc);
    }
    assert!(delivered <= 16);
    assert_eq!(c.state.reg.get24(Reg16::SP), SP + 4);
    assert!(c.state.reg.iff1 && c.state.reg.iff2);
    for (r, v) in primary {
        assert_eq!(c.state.reg.get24(r), v, "primary {r:?}");
    }
    assert_eq!(c.state.reg.get16(Reg16::AF), 0x9876);
    swap(&mut c, &mut b);
    for (r, v) in shadow {
        assert_eq!(c.state.reg.get24(r), v, "shadow {r:?}");
    }
    assert_eq!(c.state.reg.get16(Reg16::AF), 0x4567);
    (b.events, b.mem[im.fault as usize], steps)
}
fn main() {
    let args: Vec<_> = env::args().collect();
    assert_eq!(args.len(), 3);
    let a = Image::load(&args[1]);
    let b = Image::load(&args[2]);
    assert_eq!(a.async_fn.is_some(), b.async_fn.is_some());
    let mut cases = vec![];
    let base = Case {
        pending: 0,
        owned: 1,
        fault: 0,
        n: 16,
        status_at: None,
        status: 0x61,
        fault_after: None,
        pcdr: 0xa3,
    };
    for owned in [0, 1, 255] {
        for fault in [0, 1, 255] {
            for n in 0..=17 {
                for pcdr in [0, 0xa3, 0xff] {
                    cases.push(Case {
                        owned,
                        fault,
                        n,
                        pcdr,
                        ..base.clone()
                    });
                }
            }
        }
    }
    for status in 0..=255 {
        for at in [0, 1, 15] {
            cases.push(Case {
                status,
                status_at: Some(at),
                ..base.clone()
            });
        }
    }
    for at in 1..=16 {
        cases.push(Case {
            fault_after: Some(at),
            ..base.clone()
        });
    }
    assert_eq!(a.async_length.is_some(), a.async_fn.is_some());
    assert_eq!(b.async_length.is_some(), b.async_fn.is_some());
    if a.async_fn.is_some() {
        for pending in [1, 144] {
            let more: Vec<_> = cases
                .iter()
                .take(1270)
                .cloned()
                .map(|mut x| {
                    x.pending = pending;
                    x
                })
                .collect();
            cases.extend(more);
        }
    }
    let n = cases.len();
    for x in cases {
        let (ea, fa, sa) = run(&a, &x);
        let (eb, fb, sb) = run(&b, &x);
        assert_eq!((ea, fa), (eb, fb), "{x:?}");
        if x.status_at.is_none()
            && x.fault_after.is_none()
            && x.owned == 1
            && x.fault == 0
            && x.n == 16
            && x.pcdr == 0xa3
        {
            println!(
                "Full 16-byte IRQ pending={} instructions: baseline={sa}, candidate={sb}",
                x.pending
            );
        }
    }
    println!("PASS {n} complete IRQ comparisons: ports, FIFO cap, callback faults/clobbers, all LSR values, all primary/shadow registers, stack and IFF");
}
