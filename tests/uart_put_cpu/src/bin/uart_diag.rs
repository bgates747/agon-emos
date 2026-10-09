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
  if name=="_emos_gateway" { let v=c.state.reg.get24(Reg16::HL);assert!(v<=255);v as u8 } else {c.state.reg.a()}
 }
}
fn main(){
 let args:Vec<_>=env::args().collect();assert_eq!(args.len(),2);let mut b=Board::new(&args[1]);
 b.active();b.set("_uart1_keyboard_owned",0);b.set("_uart1_rts_owned",0);b.set("_serialFlags",0);b.set("_emosPolicy",4);b.set("_emosModeState",0);b.set("_emosBusy",0);
 b.ports[0x9f]=0xff;b.ports[0xa1]=0;b.ports[0xd5]=0x60;
 let req=0xb0000u32;let input=0xb0100u32;let output=0xb0110u32;
 b.poke(req,66);b.poke(req+2,1);b.poke(req+4,3);b.poke(req+5,8);b.poke(req+6,2);
 b._poke24(req+10,input);b._poke24(req+13,2);b._poke24(req+16,output);b._poke24(req+19,2);
 for (i,v) in b"ext".iter().enumerate(){b.poke(req+25+i as u32,*v);}
 for (i,v) in b"uartdiag".iter().enumerate(){b.poke(req+41+i as u32,*v);}
 let mut cases=0;
 for policy in 0..4 {b.set("_emosPolicy",policy);assert_eq!(b.call("_emos_gateway",&[req],true),32);assert!(b.io.is_empty());cases+=1;}
 b.set("_emosPolicy",4);
 for mode in 1..4 {b.set("_emosModeState",mode);assert_eq!(b.call("_emos_gateway",&[req],true),35);assert!(b.io.is_empty());cases+=1;}
 b.set("_emosModeState",0);b.set("_emosBusy",1);assert_eq!(b.call("_emos_gateway",&[req],true),31);assert!(b.io.is_empty());b.set("_emosBusy",0);cases+=1;
 assert_eq!(b.call("_emos_gateway",&[req],false),31);assert!(b.io.is_empty());cases+=1;
 for p in [0,0xaffff,0xb7fff,0xb8000,0xffffff] {
  b._poke24(req+10,p);assert_eq!(b.call("_emos_gateway",&[req],true),19);assert!(b.io.is_empty());
  b._poke24(req+10,input);b._poke24(req+16,p);assert_eq!(b.call("_emos_gateway",&[req],true),19);assert!(b.io.is_empty());
  b._poke24(req+16,output);cases+=2;
 }
 b.set("_uart1_keyboard_owned",1);assert_eq!(b.call("_emos_gateway",&[req],true),0);assert_eq!(b.peek(output),3);assert!(b.io.is_empty());b.set("_uart1_keyboard_owned",0);cases+=1;
 b.set("_emosParallelWriterLock",1);assert_eq!(b.call("_emos_gateway",&[req],true),0);assert_eq!(b.peek(output),3);assert!(b.io.is_empty());b.set("_emosParallelWriterLock",0);cases+=1;
 assert_eq!(b.call("_emos_gateway",&[req],true),0);assert_eq!(b.peek(output),1);assert_eq!(b.get("_serialFlags")&0x30,0x30);assert_eq!(b.get("_uart1_rts_owned"),1);cases+=1;
 // Open while leased must leave all pins alone.
 assert_eq!(b.call("_emos_gateway",&[req],true),0);assert_eq!(b.peek(output),3);assert!(b.io.is_empty());cases+=1;
 b.poke(input,2);
 for v in 0..=255u8 {b.poke(input+1,v);assert_eq!(b.call("_emos_gateway",&[req],true),0);assert_eq!(b.peek(output),1);assert!(b.io.contains(&(true,0xd0,v)));cases+=1;}
 b.poke(input,1);b.poke(input+1,0);b.ports[0xd5]=0x61;b.ports[0xd0]=0xab;
 assert_eq!(b.call("_emos_gateway",&[req],true),0);assert_eq!(b.peek(output),1);assert_eq!(b.peek(output+1),0xab);cases+=1;
 b.ports[0xd5]=0x62;assert_eq!(b.call("_emos_gateway",&[req],true),0);assert_eq!(b.peek(output),2);cases+=1;
 b.ports[0xd5]=0x60;b.poke(input,3);
 for v in [1,0] {b.poke(input+1,v);assert_eq!(b.call("_emos_gateway",&[req],true),0);assert_eq!(b.peek(output),1);assert_eq!(b.ports[0x9e]&4,if v==1 {0}else{4});cases+=1;}
 // Actual lifecycle, including sdlink/admission cleanup, releases abandoned lease.
 b.call("_emos_application_leave",&[0],true);assert_eq!(b.get("_serialFlags")&0x30,0);assert_eq!(b.get("_uart1_rts_owned"),0);assert_eq!(b.get("_emosPolicy"),0);cases+=1;
 b.call("_emos_uartdiag_reset",&[],true);assert!(b.io.is_empty());cases+=1;
 println!("PASS {cases} linked diagnostic gateway cases; MOSlet/mode/IRQ/buffer guards, UART/parallel contention, every TX byte, RX/error/RTS and actual exit cleanup; IX/SP/IFF");
}
