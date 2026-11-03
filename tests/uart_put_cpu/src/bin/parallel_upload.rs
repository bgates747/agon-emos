//! Actual linked upload slice, fake only GPIO/CPU scheduling. Not GPIO timing.
use ez80::{Cpu,Machine,Reg16};
use std::{collections::HashMap,env,fs,path::Path,time::Instant};
const SP:u32=0xaff00;const EXIT:u32=0xa0000;const BUFFER:u32=0x71000;
struct Board {mem:Vec<u8>,symbols:HashMap<String,u32>,pd:u8,nonowned:u8,iff:bool,
 n:usize,falls:usize,rises:usize,reads:usize,lose:Option<usize>}
fn value(i:usize)->u8{(i*73+(i>>8)*19+17)as u8}
impl Machine for Board {
 fn peek(&self,a:u32)->u8{self.mem[a as usize]}
 fn poke(&mut self,a:u32,v:u8){self.mem[a as usize]=v;}
 fn use_cycles(&self,_:i32){}
 fn port_in(&mut self,p:u16)->u8{
  match p {
   0xa2 => (self.pd&!0x10)|if self.lose.is_some_and(|n|self.falls>=n){0x10}else{0},
   0x9e => {assert_eq!(self.falls,self.reads+1);assert_eq!(self.rises,self.reads+1);
    let b=value(self.reads);self.reads+=1;b},
   _ => panic!("unexpected port IN {p:x}"),
  }
 }
 fn port_out(&mut self,p:u16,v:u8){
  assert_eq!(p,0xa2);assert!(!self.iff);assert_eq!(v&0x4f,self.nonowned);
  assert_eq!(v&0x90,0x10,"VALID must stay low; READY output latch stays released");
  if self.pd&0x20!=0&&v&0x20==0 {self.falls+=1;assert!(self.falls<=self.n);}
  if self.pd&0x20==0&&v&0x20!=0 {self.rises+=1;}
  self.pd=v;
 }
}
impl Board {
 fn new(dir:&str,n:usize)->Self{
  let p=Path::new(dir);let bytes=fs::read(p.join("MOS.bin")).unwrap();
  let mut mem=vec![0xa5;0x100000];mem[..bytes.len()].copy_from_slice(&bytes);
  let symbols=fs::read_to_string(p.join("nm.txt")).unwrap().lines().filter_map(|line|{
   let f:Vec<_>=line.split_whitespace().collect();
   if f.len()==3 {Some((f[2].into(),u32::from_str_radix(f[0],16).unwrap()))}else{None}
  }).collect();
  Self{mem,symbols,pd:0x30,nonowned:0,iff:false,n,falls:0,rises:0,reads:0,lose:None}
 }
 fn run(&mut self,iff:bool)->u8{
  let mut cpu=Cpu::new_ez80();cpu.state.reg.adl=true;cpu.state.reg.madl=true;
  cpu.state.reg.iff1=iff;cpu.state.reg.iff2=iff;
  cpu.state.reg.pc=self.symbols["_emos_parallel_upload_bytes"];
  for(reg,v)in[(Reg16::SP,SP),(Reg16::IX,0x123456),(Reg16::IY,0x654321),
               (Reg16::HL,0x456789),(Reg16::DE,0x345678),(Reg16::BC,0x234567)]{cpu.state.reg.set24(reg,v);}
  self._poke24(SP,EXIT);self._poke24(SP+3,BUFFER);self._poke24(SP+6,self.n as u32);
  let mut steps=0;let mut masked=0;
  while cpu.state.reg.pc!=EXIT {
   self.iff=cpu.state.reg.iff1;
   if iff&&self.iff {self.nonowned=(steps as u8)&0x4f;self.pd=(self.pd&0xb0)|self.nonowned;}
   cpu.execute_instruction(self);steps+=1;assert!(steps<20000);
   if iff&&!cpu.state.reg.iff1{masked+=1;assert!(masked<=10);}else{masked=0;}
   if !iff{assert!(!cpu.state.reg.iff1&&!cpu.state.reg.iff2);}
  }
  for(reg,v)in[(Reg16::SP,SP+3),(Reg16::IX,0x123456),(Reg16::IY,0x654321),
               (Reg16::HL,0x456789),(Reg16::DE,0x345678),(Reg16::BC,0x234567)]{assert_eq!(cpu.state.reg.get24(reg),v);}
  assert_eq!(cpu.state.reg.iff1,iff);assert_eq!(cpu.state.reg.iff2,iff);
  assert_eq!(self.pd&0xb0,0x30);cpu.state.reg.a()
 }
}
fn main(){let a:Vec<_>=env::args().collect();assert_eq!(a.len(),2);let began=Instant::now();let mut cases=0;
 for iff in [false,true] {
  for n in 1..=512 {
   let mut b=Board::new(&a[1],n);assert_eq!(b.run(iff),0);assert_eq!(b.falls,n);
   assert_eq!(b.rises,n);assert_eq!(b.reads,n);
   assert_eq!(b.peek(BUFFER-1),0xa5);assert_eq!(b.peek(BUFFER+n as u32),0xa5);
   for i in 0..n{assert_eq!(b.peek(BUFFER+i as u32),value(i));}cases+=1;
  }
  for lost in [0,1,255,256,511] {
   let mut b=Board::new(&a[1],512);b.lose=Some(lost);assert_eq!(b.run(iff),0xe5);
   assert_eq!(b.falls,lost);assert_eq!(b.rises,lost);assert_eq!(b.reads,lost);
   assert_eq!(b.peek(BUFFER+lost as u32),0xa5);cases+=1;
  }
 }
 println!("PASS {cases} linked upload slice cases: all1..512, both IFF states, exact bytes/edges, no tail, READY abort, register preservation; elapsed_host_ms={}; not bench timing",began.elapsed().as_millis());
}
