//! PORT-008 / INTEG-016: execute the actual linked receive core and its C ABI.
//! Only injected wire callbacks are modeled. No GPIO electrical/timing claims.
use ez80::{Cpu, Machine, Reg16};
use std::{collections::HashMap, env, fs, path::Path};
const SP:u32=0xaff00; const EXIT:u32=0xa0000;
const ENGINE:u32=0x70000; const OPS:u32=0x70100; const CONTEXT:u32=0x70200;
const OUTPUT:u32=0x71000;
const READY:u32=0xa0100; const WRITE:u32=0xa0110; const CONTROL:u32=0xa0120;
const TICKS:u32=0xa0130; const READ:u32=0xa0140;
struct Image { data:Vec<u8>, syms:HashMap<String,u32> }
impl Image {
 fn load(path:&str)->Self {let p=Path::new(path); Self {
  data:fs::read(p.join("MOS.bin")).unwrap(),
  syms:fs::read_to_string(p.join("nm.txt")).unwrap().lines().filter_map(|l|{
   let v:Vec<_>=l.split_whitespace().collect(); if v.len()!=3 {return None;}
   Some((v[2].into(),u32::from_str_radix(v[0],16).unwrap()))}).collect()}}
 fn a(&self,n:&str)->u32 {*self.syms.get(n).expect(n)}
}
struct Board { mem:Vec<u8>, len:usize, reads:usize, falls:usize, rises:usize,
 valid:bool, clock:bool, began:bool, ended:bool, tick:u32, step:u32,
 missing:bool, no_completion:bool, fault:bool }
impl Machine for Board {
 fn peek(&self,a:u32)->u8 {self.mem[a as usize]}
 fn poke(&mut self,a:u32,v:u8) {self.mem[a as usize]=v;}
 fn use_cycles(&self,_:i32){}
 fn port_in(&mut self,a:u16)->u8 {panic!("unexpected port IN {a:x}")}
 fn port_out(&mut self,a:u16,_:u8){panic!("unexpected port OUT {a:x}")}
}
impl Board {
 fn new(im:&Image,n:usize)->Self {
  let mut b=Self {mem:vec![0xa5;0x100000],len:n,reads:0,falls:0,rises:0,
   valid:true,clock:true,began:false,ended:false,tick:0,step:1,
   missing:false,no_completion:false,fault:false};
  b.mem[..im.data.len()].copy_from_slice(&im.data);
  for (i,a) in [READY,WRITE,CONTROL,TICKS].iter().enumerate() {b._poke24(OPS+3*i as u32,*a);}
  for a in [READY,WRITE,CONTROL,TICKS,READ] {b.poke(a,0xc9);} b
 }
 fn run(&mut self,im:&Image,name:&str,args:&[u32],iff:bool)->u8 {
  let mut c=Cpu::new_ez80(); c.state.reg.adl=true; c.state.reg.madl=true;
  c.state.reg.iff1=iff; c.state.reg.iff2=iff;
  c.state.reg.pc=im.a(name); c.state.reg.set24(Reg16::SP,SP);
  c.state.reg.set24(Reg16::IX,0x123456);
  self._poke24(SP,EXIT);
  for (i,a) in args.iter().enumerate(){self._poke24(SP+3+3*i as u32,*a);}
  let mut count=0;
  while c.state.reg.pc!=EXIT {
   let pc=c.state.reg.pc; let sp=c.state.reg.get24(Reg16::SP);
   let mut value=None;
   if [READY,WRITE,CONTROL,TICKS,READ].contains(&pc) {
    assert_eq!(self._peek24(sp+3),CONTEXT,"callback context ABI");
   }
   match pc {
    READY=>{value=Some(u8::from(self.missing || (self.ended && !self.no_completion)));}
    WRITE=>panic!("receive drove the data bus"),
    CONTROL=>{
     let vn=self.peek(sp+6)!=0; let ch=self.peek(sp+9)!=0;
     if !vn {self.began=true;}
     if vn && !self.valid {self.ended=true;}
     if !vn && self.clock && !ch {self.falls+=1;}
     if !vn && !self.clock && ch {self.rises+=1;}
     self.valid=vn; self.clock=ch;
    }
    TICKS=>{
     c.state.reg.set24(Reg16::HL,self.tick&0xffffff);
     c.state.reg.set24(Reg16::DE,self.tick>>24);
     self.tick=self.tick.wrapping_add(self.step);
    }
    READ=>{
     assert!(!self.valid && self.clock);
     assert_eq!(self.falls,self.reads+1); assert_eq!(self.rises,self.reads+1);
     assert!(self.reads<self.len);
     value=Some(((self.reads*73)^(self.reads>>8)^17) as u8); self.reads+=1;
     if self.fault && self.reads==3 {self.poke(ENGINE+26,0xe5);}
    }
    _=>{}
   }
   if let Some(v)=value {let flags=c.state.reg.get16(Reg16::AF)&255;
    c.state.reg.set16(Reg16::AF,((v as u16)<<8)|flags);}
   c.execute_instruction(self); count+=1;
   assert!(count<5_000_000,"stuck in {name} at {:x}",c.state.reg.pc);
   assert_eq!(c.state.reg.iff1,iff,"interrupt policy changed");
  }
  assert_eq!(c.state.reg.get24(Reg16::SP),SP+3);
  assert_eq!(c.state.reg.get24(Reg16::IX),0x123456);
  (c.state.reg.get16(Reg16::AF)>>8) as u8
 }
 fn init(&mut self,im:&Image,iff:bool) {
  assert_eq!(self.run(im,"_emos_parallel_engine_configure",&[ENGINE,OPS,CONTEXT,4096,100,100,15],iff),0);
  assert_eq!(self.run(im,"_emos_parallel_engine_open",&[ENGINE],iff),0);
 }
 fn read(&mut self,im:&Image,iff:bool,n:u32)->u8 {
  self.run(im,"_emos_parallel_engine_read",&[ENGINE,READ,OUTPUT,n],iff)
 }
}
fn main(){
 let a:Vec<_>=env::args().collect(); assert_eq!(a.len(),2,"linked-image-directory");
 let im=Image::load(&a[1]); let mut cases=0;
 for iff in [false,true] {
  for n in [1,2,3,7,255,256,257,1024,4095,4096] {
   let mut b=Board::new(&im,n); b.init(&im,iff);
   assert_eq!(b.read(&im,iff,n as u32),0);
   for i in 0..n {assert_eq!(b.peek(OUTPUT+i as u32),((i*73)^(i>>8)^17) as u8);}
   assert_eq!(b.peek(OUTPUT-1),0xa5); assert_eq!(b.peek(OUTPUT+n as u32),0xa5);
   assert_eq!(b.reads,n); assert!(b.ended && b.valid);
   assert_eq!(b._peek24(ENGINE+15),1); assert_eq!(b._peek24(ENGINE+19),n as u32);
   assert_eq!(b.peek(ENGINE+25),0); cases+=1;
  }
  for (missing,no_completion,step,fault,expected) in [
   (true,false,1,false,0xe3),(true,false,0,false,0xe3),
   (false,true,1,false,0xe4),(false,true,0,false,0xe4),
   (false,false,1,true,0xe5)] {
   let mut b=Board::new(&im,8); b.init(&im,iff); b.tick=0xfffffff0;
   b.missing=missing;b.no_completion=no_completion;b.step=step;b.fault=fault;
   assert_eq!(b.read(&im,iff,8),expected);
   assert_eq!(b.peek(ENGINE+25),0); assert_eq!(b._peek24(ENGINE+15),0);
   assert_eq!(b._peek24(ENGINE+19),0); assert!(b.valid);
   assert_eq!(b.read(&im,iff,8),expected,"fault must stick");cases+=1;
  }
  for n in [0,4097,65535] {let mut b=Board::new(&im,0);b.init(&im,iff);
   assert_eq!(b.read(&im,iff,n),0xe0);assert_eq!(b.reads,0);cases+=1;}
  let mut b=Board::new(&im,0);b.init(&im,iff);b.poke(ENGINE+25,1);
  assert_eq!(b.read(&im,iff,1),0xe2);assert_eq!(b.reads,0);cases+=1;
 }
 println!("PASS {cases} linked eZ80 reverse-core cases; actual C ABI, bytes, edges, bounds, deadlines, wrap, stuck ticks, faults, IX/SP/IFF");
}
