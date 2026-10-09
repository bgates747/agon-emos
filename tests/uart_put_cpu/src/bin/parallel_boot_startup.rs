//! Execute private boot pad leaves in the complete image; GPIO model, not bench timing.
use ez80::{Cpu,Machine,Reg16};
use std::{collections::HashMap,env,fs,path::Path};
const SP:u32=0xaff00;const EXIT:u32=0xa0000;
struct Board {mem:Vec<u8>,sym:HashMap<String,u32>,ports:[u8;256],io:Vec<(bool,u16,u8)>,iff:bool,peer:bool,moving:bool}
impl Machine for Board {
 fn peek(&self,a:u32)->u8{self.mem[a as usize]}
 fn poke(&mut self,a:u32,v:u8){self.mem[a as usize]=v;}
 fn use_cycles(&self,_:i32){}
 fn port_in(&mut self,a:u16)->u8 {
  if a==0xa2 && self.peer {
   let phase=self.get("_parallel_boot_state");
   if phase==1||phase==3||phase==4 {self.ports[0xa2]|=0x10;}else{self.ports[0xa2]&=!0x10;}
  }
  let v=self.ports[a as usize];
  self.io.push((false,a,v));if a==0xd5 {self.ports[a as usize]&=!0x9e;}v
 }
 fn port_out(&mut self,a:u16,v:u8){
  if [0xa2,0xa3,0xa4,0xa5].contains(&a){assert!(!self.iff,"Port D atomic helper must mask IRQs");}
  self.ports[a as usize]=v;self.io.push((true,a,v));
 }
}
impl Board {
 fn new(dir:&str)->Self {
  let p=Path::new(dir);let bin=fs::read(p.join("MOS.bin")).unwrap();
  let mut b=Self {mem:vec![0;0x100000],sym:fs::read_to_string(p.join("nm.txt")).unwrap().lines()
   .filter_map(|l|{let v:Vec<_>=l.split_whitespace().collect();if v.len()!=3 {None} else {Some((v[2].into(),u32::from_str_radix(v[0],16).unwrap()))}}).collect(),ports:[0;256],io:vec![],iff:false,peer:false,moving:false};
  b.mem[..bin.len()].copy_from_slice(&bin);
  b._poke24(b.sym["__2nd_jump_table"]+0x35,b.sym["__default_mi_handler"]);b
 }
 #[allow(dead_code)]
 fn set(&mut self,name:&str,v:u8){let a=self.sym[name];self.poke(a,v);}
 #[allow(dead_code)]
 fn get(&self,name:&str)->u8{self.peek(self.sym[name])}
 #[allow(dead_code)]
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
  while c.state.reg.pc!=EXIT {self.iff=c.state.reg.iff1;c.execute_instruction(self);n+=1;
   if self.moving && c.state.reg.pc==self.sym["_emos_keyboard_clock"] {
    // Locate the linked LD A,(clock) address, rather than a hardcoded build address.
    let pc=self.sym["_emos_keyboard_clock"];
    let a=self.peek(pc+1) as u32|((self.peek(pc+2) as u32)<<8)|((self.peek(pc+3) as u32)<<16);
    let v=self.peek(a);self.poke(a,v.wrapping_add(1));
   }
   assert!(n<100000000,"unbounded {name}");}
  assert_eq!(c.state.reg.get24(Reg16::IX),0x123456,"IX {name}");assert_eq!(c.state.reg.get24(Reg16::SP),SP+3);
  assert_eq!(c.state.reg.iff1,iff,"IFF {name}");assert_eq!(c.state.reg.iff2,iff);
  c.state.reg.a()
 }
}
fn main(){
 let args:Vec<_>=env::args().collect();assert_eq!(args.len(),2);let mut cases=0;
 for peer in [false,true] {for ready in [false,true] {for moving in [false,true] {
  let mut b=Board::new(&args[1]);b.peer=peer;b.moving=moving;
  b.ports[0xa2]=if ready {0x10}else{0};
  b.call("_emos_parallel_boot_start",&[],true);
  assert_eq!(b.call("_emos_parallel_boot_uart_allowed",&[],true),0);
  b.call("_emos_parallel_boot_connect",&[],true);
  if peer {
   assert_eq!(b.get("_parallel_boot_grant"),2);
   assert_ne!(b.call("_emos_parallel_boot_uart_allowed",&[],true),0);
   b.peer=false;b.ports[0xa2]&=!0x10;
   assert_eq!(b.call("_emos_parallel_boot_uart_allowed",&[],true),0);
  }else{
   assert_eq!(b.get("_parallel_boot_grant"),0);
   assert_eq!(b.ports[0x9f],255);assert_eq!(b.ports[0xa0],0);assert_eq!(b.ports[0xa1],0);
  }
  assert_eq!(b.get("_uart1_keyboard_owned"),0);
  assert_eq!(b.get("_emos_key_source"),0);
  cases+=1;
 }}}
 // Before acknowledged release, even a direct open must not enable pins.
 let mut b=Board::new(&args[1]);b.call("_emos_parallel_boot_start",&[],true);
 const CFG:u32=0x71000;b._poke24(CFG,1152000);b.poke(CFG+6,1);
 assert_eq!(b.call("_open_UART1",&[CFG],true),255);
 assert!(b.io.iter().all(|(w,_,_)|!*w));cases+=1;
 b.set("_uart1_keyboard_owned",2);
 assert_eq!(b.call("_uart1_keyboard_unpark",&[],true),3);
 assert!(b.io.iter().all(|(w,_,_)|!*w));cases+=1;
 // An IRQ-disabled caller cannot claim the transport at the foreground hook.
 let mut b=Board::new(&args[1]);b.peer=true;b.moving=true;
 b.call("_emos_parallel_boot_start",&[],false);
 b.call("_emos_parallel_boot_connect",&[],false);
 assert_eq!(b.get("_parallel_boot_grant"),0);assert_eq!(b.ports[0x9f],255);cases+=1;
 // Busy existing ownership causes release, without a timed fallback.
 let mut b=Board::new(&args[1]);b.peer=true;b.moving=true;
 b.call("_emos_parallel_boot_start",&[],true);b.set("_transitioning",1);
 b.call("_emos_parallel_boot_connect",&[],true);
 assert_eq!(b.get("_parallel_boot_grant"),0);assert_eq!(b.ports[0x9f],255);cases+=1;
 println!("PASS {cases} linked startup cases: actual coordinator, deadline/frozen-clock fuse, early open, busy claim and mainboard input");
}
