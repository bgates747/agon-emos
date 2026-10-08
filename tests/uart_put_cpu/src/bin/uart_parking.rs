//! Execute actual linked UART parking/IRQ/TX code; register model, not bench timing.
use ez80::{Cpu,Machine,Reg16};
use std::{collections::HashMap,env,fs,path::Path};
const SP:u32=0xaff00;const EXIT:u32=0xa0000;
struct Board {mem:Vec<u8>,sym:HashMap<String,u32>,ports:[u8;256],io:Vec<(bool,u16,u8)>,iff:bool}
impl Machine for Board {
 fn peek(&self,a:u32)->u8{self.mem[a as usize]}
 fn poke(&mut self,a:u32,v:u8){self.mem[a as usize]=v;}
 fn use_cycles(&self,_:i32){}
 fn port_in(&mut self,a:u16)->u8 {
  assert!(!self.iff,"IO with IRQs enabled");let v=self.ports[a as usize];
  self.io.push((false,a,v));if a==0xd5 {self.ports[a as usize]&=!0x9e;}v
 }
 fn port_out(&mut self,a:u16,v:u8){assert!(!self.iff);self.ports[a as usize]=v;self.io.push((true,a,v));}
}
impl Board {
 fn new(dir:&str)->Self {
  let p=Path::new(dir);let bin=fs::read(p.join("MOS.bin")).unwrap();
  let mut b=Self {mem:vec![0;0x100000],sym:fs::read_to_string(p.join("nm.txt")).unwrap().lines()
   .filter_map(|l|{let v:Vec<_>=l.split_whitespace().collect();if v.len()!=3 {None} else {Some((v[2].into(),u32::from_str_radix(v[0],16).unwrap()))}}).collect(),ports:[0;256],io:vec![],iff:false};
  b.mem[..bin.len()].copy_from_slice(&bin);b
 }
 fn set(&mut self,name:&str,v:u8){let a=self.sym[name];self.poke(a,v);}
 fn get(&self,name:&str)->u8{self.peek(self.sym[name])}
 fn active(&mut self){
  for n in ["_emos_key_faulted","_transitioning","_preparing","_fault_requested","_stop_requested","_parallel_reserved","_emos_key_rx","_emosParallelWriterLock","_emosParallelPinsOwned","_emosParallelConfigStatus"]{self.set(n,0);}
  self.set("_emosParallelInitialized",1);self.set("_uart1_keyboard_owned",1);self.set("_uart1_rts_owned",1);
  self.ports=[0;256];self.ports[0x9e]=0xa1;self.ports[0x9f]=0xfb;self.ports[0xa1]=3;
  self.ports[0xd1]=5;self.ports[0xd2]=7;self.ports[0xd3]=3;self.ports[0xd5]=0x60;self.io.clear();
 }
 fn call(&mut self,name:&str,args:&[u32],iff:bool)->u8 {
  let mut c=Cpu::new_ez80();c.state.reg.adl=true;c.state.reg.madl=true;c.state.reg.iff1=iff;c.state.reg.iff2=iff;
  c.state.reg.pc=self.sym[name];c.state.reg.set24(Reg16::SP,SP);c.state.reg.set24(Reg16::IX,0x123456);
  self._poke24(SP,EXIT);for(i,a)in args.iter().enumerate(){self._poke24(SP+3+3*i as u32,*a);}
  self.io.clear();let mut n=0;
  while c.state.reg.pc!=EXIT {self.iff=c.state.reg.iff1;c.execute_instruction(self);n+=1;assert!(n<3000,"unbounded {name}");}
  assert_eq!(c.state.reg.get24(Reg16::IX),0x123456,"IX {name}");assert_eq!(c.state.reg.get24(Reg16::SP),SP+3);
  assert_eq!(c.state.reg.iff1,iff,"IFF {name}");assert_eq!(c.state.reg.iff2,iff);
  c.state.reg.a()
 }
}
fn main(){
 let args:Vec<_>=env::args().collect();assert_eq!(args.len(),2);let mut b=Board::new(&args[1]);let mut cases=0;
 for status in 0..=255u8 {
  b.active();b.ports[0xd5]=status;let r=b.call("_emos_keyboard_parallel_park",&[],true);
  assert_eq!(b.io.iter().filter(|(w,p,_)|!*w && *p==0xd5).count(),1);
  if status&0x9e!=0 {
   assert_eq!(r,2);assert_eq!(b.get("_fault_requested"),1);assert_eq!(b.get("_emosParallelWriterLock"),0);
   assert_eq!(b.get("_uart1_keyboard_owned"),1);assert_eq!(b.ports[0xd1],0);
  } else if status&0x40==0 || status&1!=0 {
   assert_eq!(r,0);assert_eq!(b.get("_emosParallelWriterLock"),0);assert_eq!(b.ports[0xa1],3);
  } else {
   assert_eq!(r,1);assert_eq!(b.get("_uart1_keyboard_owned"),2);assert_eq!(b.get("_emosParallelWriterLock"),1);
   assert_eq!(b.get("_transitioning"),1);assert_eq!(b.ports[0xd4],0x10);
   let writes:Vec<_>=b.io.iter().filter(|(w,_,_)|*w).map(|(_,p,v)|(*p,*v)).collect();
   assert_eq!(writes,vec![(0xd1,0),(0xd4,16),(0x9f,255),(0xa0,0),(0xa1,0)]);
   // Real late IRQ, completion tail, direct byte, block, parser and close:
   // none may touch a shared pad or UART register while logically reserved.
   for name in ["_uart1_keyboard_irq","_uart1_keyboard_irq_done","_uart1_keyboard_stop","_uart1_keyboard_close","_close_UART1","_emos_keyboard_tick","_emos_keyboard_byte"] {
    b.call(name,&[42],false);assert!(b.io.is_empty(),"parked IO {name}");cases+=1;
   }
   assert_eq!(b.call("_uart1_keyboard_put",&[42],true),3);assert!(b.io.is_empty());
   b._poke24(0x70003,100);b.poke(0x70000,0);
   assert_eq!(b.call("_emos_keyboard_transmit_block",&[0x71000,1,0x70000],true),0);assert!(b.io.is_empty());
   // Local parallel driver must relinquish every lane before UART rebind.
   b.ports[0x9f]=0;assert_eq!(b.call("_emos_keyboard_parallel_unpark",&[],true),3);
   assert_eq!(b.get("_emosParallelWriterLock"),1);assert!(!b.io.iter().any(|(w,_,_)|*w));
   b.ports[0x9f]=255;assert_eq!(b.call("_emos_keyboard_parallel_unpark",&[],true),1);
   assert_eq!(b.get("_uart1_keyboard_owned"),1);assert_eq!(b.get("_emosParallelWriterLock"),0);
   assert_eq!(b.get("_transitioning"),0);assert_eq!(b.ports[0xd4],0);assert_eq!(b.ports[0xd1],5);
   assert_eq!(b.ports[0x9f],0xfb);assert_eq!(b.ports[0xa1],3);assert_eq!(b.ports[0xd2],7);
   assert_eq!(b.io.iter().filter(|(w,p,_)|*w && *p==0xd2).count(),0);cases+=3;
  }cases+=1;
 }
 for name in ["_transitioning","_preparing","_fault_requested","_stop_requested","_emos_key_faulted","_emos_key_rx","_emosParallelWriterLock","_emosParallelPinsOwned"]{
  b.active();b.set(name,1);assert_eq!(b.call("_emos_keyboard_parallel_park",&[],true),3);assert!(b.io.is_empty());cases+=1;
 }
 b.active();assert_eq!(b.call("_emos_keyboard_parallel_park",&[],false),3);assert!(b.io.is_empty());cases+=1;
 for reg in [0x9f,0xa0,0xa1,0xd4] {
  b.active();b.ports[reg]^=0x80;assert_eq!(b.call("_emos_keyboard_parallel_park",&[],true),3);
  assert!(!b.io.iter().any(|(w,_,_)|*w));assert_eq!(b.get("_emosParallelWriterLock"),0);cases+=1;
 }
 println!("PASS {cases} linked eZ80 UART parking cases; LSR, packet/serializer fences, late IRQ/TX, IFF, IX/SP, exact release/restore writes");
}
