//! PORT-008 F02c1: execute linked eZ80 sequencer against host transition vectors.
//! Complements paired behavioral tests; no peripheral or wall-clock emulation.
use ez80::{Cpu, Machine, Reg16};
use std::{collections::HashMap, env, fs, path::Path};
const SP:u32=0xaff00; const EXIT:u32=0xa0000; const STATE:u32=0x70000;
struct Board { mem:Vec<u8>, symbols:HashMap<String,u32> }
impl Machine for Board {
 fn peek(&self,a:u32)->u8 {self.mem[a as usize]}
 fn poke(&mut self,a:u32,v:u8) {self.mem[a as usize]=v;}
 fn use_cycles(&self,_:i32) {}
 fn port_in(&mut self,a:u16)->u8 {panic!("unbound sequencer read port {a:x}")}
 fn port_out(&mut self,a:u16,_:u8) {panic!("unbound sequencer wrote port {a:x}")}
}
impl Board {
 fn new(path:&str)->Self {
  let p=Path::new(path);let bin=fs::read(p.join("MOS.bin")).unwrap();
  let mut b=Self{mem:vec![0xa5;0x100000],symbols:fs::read_to_string(p.join("nm.txt")).unwrap()
   .lines().filter_map(|l| {let v:Vec<_>=l.split_whitespace().collect();
    if v.len()!=3 {None} else {Some((v[2].into(),u32::from_str_radix(v[0],16).unwrap()))}}).collect()};
  b.mem[..bin.len()].copy_from_slice(&bin);b
 }
 fn call(&mut self,name:&str,args:&[u32],iff:bool)->u8 {
  let mut cpu=Cpu::new_ez80();cpu.state.reg.adl=true;cpu.state.reg.madl=true;
  cpu.state.reg.iff1=iff;cpu.state.reg.iff2=iff;
  cpu.state.reg.pc=*self.symbols.get(name).expect(name);
  cpu.state.reg.set24(Reg16::SP,SP);cpu.state.reg.set24(Reg16::IX,0x123456);
  cpu.state.reg.set24(Reg16::IY,0x654321);self._poke24(SP,EXIT);
  for(i,a)in args.iter().enumerate(){self._poke24(SP+3+3*i as u32,*a);}
  let mut n=0;
  while cpu.state.reg.pc!=EXIT {
   cpu.execute_instruction(self);n+=1;assert!(n<2000,"unbounded {name}");
   assert_eq!(cpu.state.reg.iff1,iff);assert_eq!(cpu.state.reg.iff2,iff);
  }
  assert_eq!(cpu.state.reg.get24(Reg16::SP),SP+3);
  assert_eq!(cpu.state.reg.get24(Reg16::IX),0x123456);
  // IY is caller-clobbered in the AgonDev C ABI; IX is callee-preserved.
  assert_eq!(self.peek(STATE-1),0xa5);assert_eq!(self.peek(STATE+4),0xa5);
  (cpu.state.reg.get16(Reg16::AF)>>8) as u8
 }
}
fn main(){
 let args:Vec<_>=env::args().collect();assert_eq!(args.len(),3,"image-directory host-vectors");
 let mut b=Board::new(&args[1]);let text=fs::read_to_string(&args[2]).unwrap();let mut cases=0;
 for iff in [false,true] {
  for line in text.lines(){
   let v:Vec<u8>=line.split_whitespace().map(|n|n.parse().unwrap()).collect();assert_eq!(v.len(),8);
   b.poke(STATE,v[0]);b.poke(STATE+1,v[1]&1);b.poke(STATE+2,(v[1]>>1)&1);b.poke(STATE+3,(v[1]>>2)&1);
   let result=b.call("_emos_parallel_handover_step",&[STATE,v[2] as u32],iff);
   assert_eq!(result,v[3],"action: {line}");
   for i in 0..4 {assert_eq!(b.peek(STATE+i),v[4+i as usize],"state: {line}");}cases+=1;
  }
  b.call("_emos_parallel_handover_init",&[STATE],iff);
  assert_eq!(&b.mem[STATE as usize..STATE as usize+4],&[0,1,0,1]);cases+=1;
  assert_eq!(b.call("_emos_parallel_handover_begin",&[STATE],iff),0);cases+=1;
  b.poke(STATE,4);assert_eq!(b.call("_emos_parallel_handover_begin",&[STATE],iff),1);
  assert_eq!(b.peek(STATE),5);assert_eq!(b.peek(STATE+3),0);cases+=1;
  b.poke(STATE+1,0);b.poke(STATE+2,1);
  b.call("_emos_parallel_handover_cancel",&[STATE],iff);
  assert_eq!(&b.mem[STATE as usize..STATE as usize+4],&[0,0,1,1]);cases+=1;
 }
 println!("PASS {cases} linked eZ80 handover cases; host/target agreement, C ABI, bounded steps, IX/SP/IFF and memory guards");
}
