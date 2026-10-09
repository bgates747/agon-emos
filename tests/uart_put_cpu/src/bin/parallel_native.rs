//! PORT-008: actual C guard and assembly bytes in a RAM-only linked image.
//! Fake only I/O registers. No physical PARLIO timing/throughput claim.
use ez80::{Cpu,Machine,Reg16};
use std::{collections::HashMap,env,fs,path::Path};
const BASE:usize=0x40000;const SP:u32=0xaff00;const EXIT:u32=0xa0000;
const H:u32=0x70000;const BUFFER:u32=0x71000;
struct Board{mem:Vec<u8>,sym:HashMap<String,u32>,ports:[u8;256],iff:bool,
 reverse:bool,n:usize,falls:usize,rises:usize,reads:usize,tx:Vec<u8>,writes:usize,
 began:bool,ended:bool,lose_after:Option<usize>,nonowned:u8}
fn pixel(i:usize)->u8{((i*73)^(i>>8)^17) as u8}
impl Machine for Board{
 fn peek(&self,a:u32)->u8{self.mem[a as usize]}
 fn poke(&mut self,a:u32,v:u8){self.mem[a as usize]=v;}
 fn use_cycles(&self,_:i32){}
 fn port_in(&mut self,a:u16)->u8{
  if a==0xa2 {return (self.ports[0xa2]&!0x10)|if self.lose_after.is_some_and(|n|self.falls>=n){0x10}else{0};}
  if a==0x9e{
   assert!(self.reverse&&self.began&&!self.ended&&self.ports[0x9f]==255);
   assert_eq!(self.falls,self.reads+1);assert_eq!(self.rises,self.reads+1);
   assert!(self.reads<self.n);let v=pixel(self.reads);self.reads+=1;return v;
  }
  assert!([0x9f,0xa0,0xa1,0xa3,0xa4,0xa5,0xd1,0xd4].contains(&a),"unexpected IN {a:x}");
  self.ports[a as usize]
 }
 fn port_out(&mut self,a:u16,v:u8){
  self.writes+=1;
  if a==0xa2{
   assert!(!self.iff,"Port D RMW must mask interrupts");
   assert_eq!(v&0x4f,self.nonowned,"lost unrelated onboard GPIO bits");
   assert_eq!(v&0x10,0x10,"READY output latch must remain released");
   let old=self.ports[0xa2];
   if old&0x80!=0&&v&0x80==0{self.began=true;assert!(v&0x20!=0);}
   if v&0x80==0&&old&0x20!=0&&v&0x20==0{
    self.falls+=1;assert!(self.falls<=self.n);
    if !self.reverse {assert_eq!(self.ports[0x9f],0);self.tx.push(self.ports[0x9e]);}
   }
   if v&0x80==0&&old&0x20==0&&v&0x20!=0{self.rises+=1;}
   if old&0x80==0&&v&0x80!=0{self.ended=true;assert_eq!(self.ports[0x9f],255);}
  }else if a==0x9e{
   assert!(!self.reverse&&self.ports[0x9f]==0);
   assert!(self.ports[0xa2]&0x20!=0,"data changed while clock low");
  }else{assert_eq!(a,0x9f);assert!(v==0||v==255);}
  self.ports[a as usize]=v;
 }
}
impl Board{
 fn new(dir:&str,n:usize,reverse:bool)->Self{
  let p=Path::new(dir);let bytes=fs::read(p.join("payload.bin")).unwrap();
  let mut b=Self{mem:vec![0xa5;0x100000],sym:fs::read_to_string(p.join("nm.txt")).unwrap().lines().filter_map(|l|{let v:Vec<_>=l.split_whitespace().collect();if v.len()!=3{None}else{Some((v[2].into(),u32::from_str_radix(v[0],16).unwrap()))}}).collect(),
   ports:[0;256],iff:false,reverse,n,falls:0,rises:0,reads:0,tx:vec![],writes:0,began:false,ended:false,lose_after:None,nonowned:0x4b};
  b.mem[BASE..BASE+bytes.len()].copy_from_slice(&bytes);
  b.mem[H as usize..H as usize+4].copy_from_slice(&[11,1,1,0]);
  for i in 0..n{if !reverse{b.poke(BUFFER+i as u32,pixel(i));}}
  b.ports[0x9f]=255;b.ports[0xa2]=0xa0|b.nonowned;
  b.ports[0xa3]=0x10;b.ports[0xd4]=0x10;b
 }
 fn run(&mut self,iff:bool,args:&[u32])->u8{self.execute("_emos_parallel_native_payload",iff,args)}
 fn execute(&mut self,name:&str,iff:bool,args:&[u32])->u8{
  let mut c=Cpu::new_ez80();c.state.reg.adl=true;c.state.reg.madl=true;
  c.state.reg.iff1=iff;c.state.reg.iff2=iff;c.state.reg.pc=self.sym[name];
  c.state.reg.set24(Reg16::SP,SP);c.state.reg.set24(Reg16::IX,0x123456);c.state.reg.set24(Reg16::IY,0x654321);
  self._poke24(SP,EXIT);for(i,a)in args.iter().enumerate(){self._poke24(SP+3+i as u32*3,*a);}
  let mut steps=0;let mut masked=0;
  while c.state.reg.pc!=EXIT{
   self.iff=c.state.reg.iff1;
   if iff&&self.iff{
    // Model arbitrary ISR-owned changes between port operations. Atomic RMW
    // must preserve the latest bits, not an epoch-start snapshot.
    self.nonowned=(steps as u8)&0x4f;self.ports[0xa2]=(self.ports[0xa2]&0xb0)|self.nonowned;
   }
   c.execute_instruction(self);steps+=1;
   if iff&&!c.state.reg.iff1{masked+=1;assert!(masked<=8,"whole-block interrupt mask");}else{masked=0;}
   if !iff{assert!(!c.state.reg.iff1&&!c.state.reg.iff2);}
   assert!(steps<2_000_000,"unbounded native payload");
  }
  assert_eq!(c.state.reg.get24(Reg16::SP),SP+3);assert_eq!(c.state.reg.get24(Reg16::IX),0x123456);
  if name=="_emos_parallel_native_bytes"{assert_eq!(c.state.reg.get24(Reg16::IY),0x654321);}assert_eq!(c.state.reg.iff1,iff);assert_eq!(c.state.reg.iff2,iff);
  c.state.reg.a()
 }
 fn args(&self)->[u32;6]{[H,1,u32::from(self.reverse),BUFFER,self.n as u32,0]}
}
fn main(){let a:Vec<_>=env::args().collect();assert_eq!(a.len(),2,"RAM-only linked-image directory");let mut cases=0;
 for iff in [false,true]{for reverse in [false,true]{
  for n in [1,2,3,7,255,256,257,1024,4095,4096]{
   let mut b=Board::new(&a[1],n,reverse);assert_eq!(b.run(iff,&b.args()),0);
   assert!(b.began&&b.ended);assert_eq!(b.falls,n);assert_eq!(b.rises,n);
   if reverse{assert_eq!(b.reads,n);for i in 0..n{assert_eq!(b.peek(BUFFER+i as u32),pixel(i));}}
   else{assert_eq!(b.tx,(0..n).map(pixel).collect::<Vec<_>>());}
   assert_eq!(b.peek(BUFFER-1),0xa5);assert_eq!(b.peek(BUFFER+n as u32),0xa5);
   assert_eq!(b.ports[0x9f],255);assert_eq!(b.ports[0xa2]&0xb0,0xb0);cases+=1;
  }
  let mut raw=Board::new(&a[1],8,reverse);
  assert_eq!(raw.execute("_emos_parallel_native_bytes",iff,&[BUFFER,8,u32::from(reverse)]),0);
  assert_eq!(raw.falls,8);cases+=1;
  let mut b=Board::new(&a[1],8,reverse);b.lose_after=Some(3);assert_eq!(b.run(iff,&b.args()),0xe5);
  assert_eq!(b.falls,3);assert_eq!(b.rises,3);assert!(b.ended);assert_eq!(b.peek(H+3),1);assert_eq!(b.ports[0x9f],255);cases+=1;
  for (index,value) in [(0,0),(3,0),(4,0),(4,4097),(2,2)]{
   let mut b=Board::new(&a[1],8,reverse);let mut args=b.args();args[index]=value;
   assert_eq!(b.run(iff,&args),0xe0);assert_eq!(b.writes,0);cases+=1;
  }
  for (address,value) in [(H,10),(H+1,0),(H+2,0),(H+3,1)]{
   let mut b=Board::new(&a[1],8,reverse);b.poke(address,value);
   assert_eq!(b.run(iff,&b.args()),0xe1);assert_eq!(b.writes,0);cases+=1;
  }
  for (port,value) in [(0x9f,0),(0xa0,1),(0xa1,1),(0xa2,0),(0xa3,0),(0xa4,0x80),(0xa5,0x20),(0xd1,1),(0xd4,0)]{
   let mut b=Board::new(&a[1],8,reverse);b.ports[port]=value;
   assert_eq!(b.run(iff,&b.args()),0xe1);assert_eq!(b.writes,0);cases+=1;
  }
  let mut b=Board::new(&a[1],8,reverse);let mut args=b.args();args[1]=0;
  assert_eq!(b.run(iff,&args),0xe1);assert_eq!(b.writes,0);cases+=1;
 }}
 println!("PASS {cases} linked native eZ80 cases: actual C guard/assembly, both directions, bytes/edges, bounds, READY loss, Port D ISR interleavings, IX/SP/IFF and raw-loop IY; RAM-only, no bench timing");
}
