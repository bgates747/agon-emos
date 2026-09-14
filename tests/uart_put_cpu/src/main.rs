//! Execute the linked C baseline and assembly candidate, not reimplementations.
//! This is ABI/operation equivalence evidence, never physical UART timing.
use ez80::{Cpu, Machine, Reg16, Reg8};
use std::{cell::Cell, env, fs, path::Path};

const SP: u32 = 0xaf000;
const EXIT: u32 = 0xa0000;
const SWAP: u32 = EXIT + 16;
#[derive(Debug, PartialEq, Clone)]
enum Io { Read(u16, u8), Write(u16, u8) }
struct Board {
    mem: Vec<u8>, lsr: u8, pcdr: u8, events: Vec<Io>, cycles: Cell<i64>,
}
impl Machine for Board {
    fn peek(&self, a: u32) -> u8 { self.use_cycles(1); self.mem[a as usize] }
    fn poke(&mut self, a: u32, v: u8) { self.use_cycles(1); self.mem[a as usize] = v; }
    fn use_cycles(&self, n: i32) { self.cycles.set(self.cycles.get() + n as i64); }
    fn port_in(&mut self, a: u16) -> u8 {
        let v = match a { 0xd5 => self.lsr, 0x9e => self.pcdr, _ => panic!("unexpected IN {a:x}") };
        self.events.push(Io::Read(a,v)); v
    }
    fn port_out(&mut self, a: u16, v: u8) {
        assert!([0xd0,0xd1,0x9e].contains(&a), "unexpected OUT {a:x}");
        if a == 0x9e { self.pcdr = v; }
        self.events.push(Io::Write(a,v));
    }
}
struct Image { data: Vec<u8>, put: u32, owned: u32, fault: u32 }
impl Image {
    fn load(dir: &str) -> Self {
        let dir = Path::new(dir);
        let nm = fs::read_to_string(dir.join("nm.txt")).unwrap();
        let address = |name| {
            let line = nm.lines().find(|l| l.split_whitespace().last() == Some(name)).expect(name);
            u32::from_str_radix(line.split_whitespace().next().unwrap(),16).unwrap()
        };
        Self { data: fs::read(dir.join("MOS.bin")).unwrap(), put: address("_uart1_keyboard_put"),
            owned: address("_uart1_keyboard_owned"), fault: address("_emos_key_faulted") }
    }
}
#[derive(Clone, Copy, Debug)]
struct Input { owned: u8, fault: u8, lsr: u8, pcdr: u8, value: u8, iff: bool }
fn swap(cpu: &mut Cpu, b: &mut Board) {
    cpu.state.reg.pc = SWAP;
    cpu.execute_instruction(b); // EXX
    cpu.execute_instruction(b); // EX AF,AF'
}
fn run(img: &Image, b: &mut Board, x: Input) -> (u8, Vec<Io>, usize, i64) {
    let mut cpu = Cpu::new_ez80();
    cpu.state.reg.adl = true;
    b.mem[SWAP as usize] = 0xd9;
    b.mem[SWAP as usize + 1] = 0x08;
    let shadow = [(Reg16::BC,0x123456),(Reg16::DE,0x234567),(Reg16::HL,0x345678)];
    for (r,v) in shadow { cpu.state.reg.set24(r,v); }
    cpu.state.reg.set16(Reg16::AF,0x4567);
    swap(&mut cpu,b);
    cpu.state.reg.set24(Reg16::IX,0xabcdef);
    cpu.state.reg.set24(Reg16::IY,0xbcdef0);
    cpu.state.reg.set24(Reg16::SP,SP);
    cpu.state.reg.set8(Reg8::I,0x83);
    cpu.state.reg.iff1 = x.iff;
    cpu.state.reg.iff2 = x.iff;
    cpu.state.reg.pc = img.put;
    b.mem[img.owned as usize] = x.owned;
    b.mem[img.fault as usize] = x.fault;
    b._poke24(SP,EXIT);
    b._poke24(SP+3,0x567800 | x.value as u32);
    b.lsr = x.lsr; b.pcdr = x.pcdr; b.events.clear(); b.cycles.set(0);
    let mut steps = 0;
    while cpu.state.reg.pc != EXIT {
        let events = b.events.len();
        let enabled = cpu.state.reg.iff1;
        cpu.execute_instruction(b);
        if b.events.len() != events {
            assert!(!enabled && !cpu.state.reg.iff1, "I/O outside interrupt exclusion: {x:?}");
        }
        steps += 1;
        assert!(steps < 500, "did not return: {:06x}, {x:?}",cpu.state.reg.pc);
    }
    let cycles = b.cycles.get();
    assert_eq!(cpu.state.reg.iff1,x.iff,"IFF1 {x:?}");
    assert_eq!(cpu.state.reg.iff2,x.iff,"IFF2 {x:?}");
    assert_eq!(cpu.state.reg.get24(Reg16::SP),SP+3,"SP {x:?}");
    assert_eq!(cpu.state.reg.get24(Reg16::IX),0xabcdef,"IX {x:?}");
    assert_eq!(cpu.state.reg.get24(Reg16::IY),0xbcdef0,"IY {x:?}");
    assert_eq!(b._peek24(SP+3),0x567800 | x.value as u32);
    assert_eq!(b.mem[img.owned as usize],x.owned);
    assert_eq!(b.mem[img.fault as usize],x.fault);
    let result = cpu.state.reg.a();
    swap(&mut cpu,b);
    for (r,v) in shadow { assert_eq!(cpu.state.reg.get24(r),v,"alternate {r:?} {x:?}"); }
    assert_eq!(cpu.state.reg.get16(Reg16::AF),0x4567,"alternate AF {x:?}");
    (result,b.events.clone(),steps,cycles)
}
fn expected(x: Input) -> (u8,Vec<Io>) {
    if x.owned == 0 || x.fault != 0 { return (3,vec![]); }
    let mut io = vec![Io::Read(0xd5,x.lsr)];
    io.push(Io::Read(0x9e,x.pcdr));
    if x.lsr & 0x9e != 0 {
        io.push(Io::Write(0x9e,x.pcdr | 4));
        io.push(Io::Write(0xd1,0)); return (2,io);
    }
    if x.pcdr & 8 != 0 { return (4,io); }
    if x.lsr & 0x20 == 0 { return (0,io); }
    io.push(Io::Write(0xd0,x.value)); (1,io)
}
fn main() {
    let args: Vec<_> = env::args().collect();
    assert_eq!(args.len(),3,"usage: emos-uart-put-check BASELINE_DIR CANDIDATE_DIR");
    let images = [Image::load(&args[1]),Image::load(&args[2])];
    let mut boards: Vec<_> = images.iter().map(|im| {
        let mut mem = vec![0;0x100000]; mem[..im.data.len()].copy_from_slice(&im.data);
        Board { mem,lsr:0,pcdr:0,events:vec![],cycles:Cell::new(0) }
    }).collect();
    let mut count = 0;
    let mut min = [[usize::MAX;5];2]; let mut max = [[0;5];2];
    for owned in [0,1,255] { for fault in [0,1,255] { for lsr in 0..=255 {
        for pcdr in [0,4,8,12,0xa3,0xab,0xf7,0xff] {
            for value in [0,1,0x55,0x80,0xaa,0xff] { for iff in [false,true] {
                let x = Input{owned,fault,lsr,pcdr,value,iff};
                let want = expected(x);
                for (n,img) in images.iter().enumerate() {
                    let (result,events,steps,_) = run(img,&mut boards[n],x);
                    assert_eq!((result,events),want,"image {n}, {x:?}");
                    min[n][result as usize] = min[n][result as usize].min(steps);
                    max[n][result as usize] = max[n][result as usize].max(steps);
                }
                count += 1;
            }}
        }
    }}}
    println!("PASS {count} input cases per linked image; result/ports/IFF/stack/IX/IY/alternate registers");
    println!("result,baseline_min_instructions,baseline_max_instructions,candidate_min_instructions,candidate_max_instructions");
    for i in 0..5 { println!("{i},{},{},{},{}",min[0][i],max[0][i],min[1][i],max[1][i]); }
}
