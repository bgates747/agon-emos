//! Execute real serializer reservation + bounded TX; no physical timing claim.
use ez80::{Cpu,Machine,Reg16};
use std::{collections::HashMap,env,fs,path::Path};
const SP:u32=0xaff00;const EXIT:u32=0xa0000;
struct Board {mem:Vec<u8>,sym:HashMap<String,u32>,ports:[u8;256],io:Vec<(bool,u16,u8)>,iff:bool, ticking:bool, fail_after:usize, sent:usize}
impl Machine for Board {
 fn peek(&self,a:u32)->u8{self.mem[a as usize]}
 fn poke(&mut self,a:u32,v:u8){self.mem[a as usize]=v;}
 fn use_cycles(&self,_:i32){}
 fn port_in(&mut self,a:u16)->u8 {
  assert!(!self.iff,"IO with IRQs enabled");let v=self.ports[a as usize];
  self.io.push((false,a,v));if a==0xd5 {self.ports[a as usize]&=!0x9e;}v
 }
 fn port_out(&mut self,a:u16,v:u8){assert!(!self.iff);self.ports[a as usize]=v;self.io.push((true,a,v));if a==0xd0 {self.sent+=1;if self.fail_after==self.sent {self.ports[0xd5]|=2;}}}
}
impl Board {
 fn new(dir:&str)->Self {
  let p=Path::new(dir);let bin=fs::read(p.join("MOS.bin")).unwrap();
  let mut b=Self {mem:vec![0;0x100000],sym:fs::read_to_string(p.join("nm.txt")).unwrap().lines()
   .filter_map(|l|{let v:Vec<_>=l.split_whitespace().collect();if v.len()!=3 {None} else {Some((v[2].into(),u32::from_str_radix(v[0],16).unwrap()))}}).collect(),ports:[0;256],io:vec![],iff:false,ticking:false,fail_after:0,sent:0};
  b.mem[..bin.len()].copy_from_slice(&bin);b
 }
 fn set(&mut self,name:&str,v:u8){let a=self.sym[name];self.poke(a,v);}
 fn get(&self,name:&str)->u8{self.peek(self.sym[name])}
 fn active(&mut self){
  for n in ["_emos_key_faulted","_transitioning","_preparing","_fault_requested","_stop_requested","_parallel_reserved","_emos_key_rx","_emosParallelWriterLock","_emosParallelPinsOwned","_emosParallelConfigStatus"]{self.set(n,0);}
  self.set("_emosParallelInitialized",1);self.set("_uart1_keyboard_owned",1);self.set("_uart1_rts_owned",1);
  self.ticking=false;self.fail_after=0;self.sent=0;self.set("_clock",0);self.ports=[0;256];self.ports[0x9e]=0xa1;self.ports[0x9f]=0xfb;self.ports[0xa1]=3;
  self.ports[0xd1]=5;self.ports[0xd2]=7;self.ports[0xd3]=3;self.ports[0xd5]=0x60;self.io.clear();
 }
 fn call(&mut self,name:&str,args:&[u32],iff:bool)->u8 {
  let mut c=Cpu::new_ez80();c.state.reg.adl=true;c.state.reg.madl=true;c.state.reg.iff1=iff;c.state.reg.iff2=iff;
  c.state.reg.pc=self.sym[name];c.state.reg.set24(Reg16::SP,SP);c.state.reg.set24(Reg16::IX,0x123456);
  self._poke24(SP,EXIT);for(i,a)in args.iter().enumerate(){self._poke24(SP+3+3*i as u32,*a);}
  self.io.clear();let mut n=0;
  while c.state.reg.pc!=EXIT {self.iff=c.state.reg.iff1;c.execute_instruction(self);n+=1;if self.ticking && n%50==0 {let a=self.sym["_clock"];self.poke(a,self.peek(a).wrapping_add(2));}assert!(n<200000,"unbounded {name}");}
  assert_eq!(c.state.reg.get24(Reg16::IX),0x123456,"IX {name}");assert_eq!(c.state.reg.get24(Reg16::SP),SP+3);
  assert_eq!(c.state.reg.iff1,iff,"IFF {name}");assert_eq!(c.state.reg.iff2,iff);
  c.state.reg.a()
 }
}
fn main(){
 let args:Vec<_>=env::args().collect();assert_eq!(args.len(),2);let mut b=Board::new(&args[1]);let mut cases=0;
 let packet:[u8;19]=[23,0,247,69,88,2,2,1,2,3,4,1,0,0,16,1,0,42,99];
 b.mem[0x71000..0x71013].copy_from_slice(&packet);
 // Every private entry rejects an IRQ-context call and preserves IRQ state.
 for iff in [false,true] {
  b.active();let expected=if iff {1}else{3};assert_eq!(b.call("_emos_keyboard_parallel_reserve",&[],iff),expected);assert!(b.io.is_empty());cases+=1;
 }
 for busy in 0..3u8 {
  b.active();b.set("_parallel_reserved",busy);b.set("_transitioning",u8::from(busy!=0));
  for iff in [false,true] {
   if busy==1 && iff {continue;}
   assert_eq!(b.call("_emos_keyboard_parallel_send",&[0x71000,19],iff),1);assert!(b.io.is_empty());assert_eq!(b.get("_parallel_reserved"),busy);cases+=1;
  }
 }
 // Exact transmitted bytes, reservation held throughout the UART cycle.
 b.active();assert_eq!(b.call("_emos_keyboard_parallel_reserve",&[],true),1);
 assert_eq!(b.call("_emos_keyboard_parallel_reserve",&[],true),3);assert!(b.io.is_empty());cases+=1;
 assert_eq!(b.call("_emos_keyboard_parallel_send",&[0x71000,19],true),0);
 let bytes:Vec<_>=b.io.iter().filter(|(w,p,_)|*w&&*p==0xd0).map(|(_,_,v)|*v).collect();assert_eq!(bytes,packet);
 assert_eq!(b.get("_parallel_reserved"),1);assert_eq!(b.get("_transitioning"),1);assert_eq!(b.get("_emosParallelWriterLock"),1);cases+=1;
 for name in ["_emos_keyboard_send","_emos_keyboard_text","_emos_keyboard_layout","_emos_keyboard_transport_claim"] {
  assert_eq!(b.call(name,&[0x71000,1],true),1);assert!(b.io.is_empty(),"reserved writer {name}");cases+=1;
 }
 b.set("_emos_key_rx",1);assert_eq!(b.call("_emos_keyboard_parallel_park",&[],true),3);assert!(b.io.is_empty());b.set("_emos_key_rx",0);cases+=1;
 b.ports[0xd5]=0x20;assert_eq!(b.call("_emos_keyboard_parallel_park",&[],true),0);assert_eq!(b.get("_emosParallelWriterLock"),1);cases+=1;
 b.ports[0xd5]=0x60;assert_eq!(b.call("_emos_keyboard_parallel_park",&[],true),1);
 assert_eq!(b.call("_emos_keyboard_parallel_release",&[],true),3);assert!(b.io.is_empty());
 assert_eq!(b.call("_emos_keyboard_parallel_send",&[0x71000,19],true),1);assert!(b.io.is_empty());cases+=1;
 assert_eq!(b.call("_emos_keyboard_parallel_unpark",&[],true),1);assert_eq!(b.get("_transitioning"),1);assert_eq!(b.get("_emosParallelWriterLock"),1);
 assert_eq!(b.call("_emos_keyboard_send",&[0x71000,19],true),1);assert!(b.io.is_empty());
 assert_eq!(b.call("_emos_keyboard_parallel_send",&[0x71000,19],true),0);
 assert_eq!(b.call("_emos_keyboard_parallel_release",&[],true),1);assert_eq!(b.get("_transitioning"),0);assert_eq!(b.get("_parallel_reserved"),0);
 assert_eq!(b.call("_emos_keyboard_send",&[0x71000,19],true),0);cases+=1;
 // Pending errors/stop forbid private and ordinary replay even after release.
 for flag in ["_fault_requested","_stop_requested","_emos_key_faulted"] {
  b.active();assert_eq!(b.call("_emos_keyboard_parallel_reserve",&[],true),1);b.set(flag,1);
  assert_eq!(b.call("_emos_keyboard_parallel_send",&[0x71000,19],true),1);assert!(b.io.is_empty());
  assert_eq!(b.call("_emos_keyboard_parallel_release",&[],true),1);
  assert_eq!(b.call("_emos_keyboard_send",&[0x71000,19],true),1);assert!(b.io.is_empty());cases+=1;
 }
 // Fault after exactly one sent byte: never resend or publish full completion.
 b.active();assert_eq!(b.call("_emos_keyboard_parallel_reserve",&[],true),1);b.fail_after=1;
 assert_eq!(b.call("_emos_keyboard_parallel_send",&[0x71000,19],true),2);assert_eq!(b.sent,1);assert_eq!(b.get("_fault_requested"),1);assert_eq!(b.get("_parallel_reserved"),1);
 assert_eq!(b.call("_emos_keyboard_parallel_send",&[0x71000,19],true),1);assert!(b.io.is_empty());
 assert_eq!(b.call("_emos_keyboard_parallel_release",&[],true),1);assert_eq!(b.call("_emos_keyboard_send",&[0x71000,19],true),1);assert!(b.io.is_empty());cases+=1;
 // Bounded existing deadline with CTS held; synthetic ticks, no time claim.
 b.active();assert_eq!(b.call("_emos_keyboard_parallel_reserve",&[],true),1);b.ports[0x9e]|=8;b.ticking=true;
 assert_eq!(b.call("_emos_keyboard_parallel_send",&[0x71000,19],true),2);assert_eq!(b.sent,0);assert_eq!(b.get("_fault_requested"),1);assert_eq!(b.get("_emosParallelWriterLock"),1);cases+=1;
 println!("PASS {cases} linked eZ80 reservation cases; exact TX, late partial RX, nested/busy refusal, park/restore/status fencing, failed/partial sends, bounded CTS wait, IX/SP/IFF");
}
