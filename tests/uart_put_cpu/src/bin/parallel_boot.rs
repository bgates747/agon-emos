//! Execute private boot pad leaves in the complete image; GPIO model, not bench timing.
use ez80::{Cpu,Machine,Reg16};
use std::{collections::HashMap,env,fs,path::Path};
const SP:u32=0xaff00;const EXIT:u32=0xa0000;
struct Board {mem:Vec<u8>,sym:HashMap<String,u32>,ports:[u8;256],io:Vec<(bool,u16,u8)>,iff:bool}
impl Machine for Board {
 fn peek(&self,a:u32)->u8{self.mem[a as usize]}
 fn poke(&mut self,a:u32,v:u8){self.mem[a as usize]=v;}
 fn use_cycles(&self,_:i32){}
 fn port_in(&mut self,a:u16)->u8 {
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
   .filter_map(|l|{let v:Vec<_>=l.split_whitespace().collect();if v.len()!=3 {None} else {Some((v[2].into(),u32::from_str_radix(v[0],16).unwrap()))}}).collect(),ports:[0;256],io:vec![],iff:false};
  b.mem[..bin.len()].copy_from_slice(&bin);b
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
  while c.state.reg.pc!=EXIT {self.iff=c.state.reg.iff1;c.execute_instruction(self);n+=1;assert!(n<3000,"unbounded {name}");}
  assert_eq!(c.state.reg.get24(Reg16::IX),0x123456,"IX {name}");assert_eq!(c.state.reg.get24(Reg16::SP),SP+3);
  assert_eq!(c.state.reg.iff1,iff,"IFF {name}");assert_eq!(c.state.reg.iff2,iff);
  c.state.reg.a()
 }
}
fn main(){
 let args:Vec<_>=env::args().collect();assert_eq!(args.len(),2);let mut b=Board::new(&args[1]);let mut cases=0;
 const STATE:u32=0x71000;
 for iff in [false,true] {
  for keep in 0..=255u8 {if keep&0xb0!=0 {continue;}
   b.ports=[0x55;256];b.ports[0xa2]=keep;b.ports[0xa3]=keep;b.ports[0xa4]=keep;b.ports[0xa5]=keep;
   b.poke(STATE-1,0xa5);b.poke(STATE+4,0xa5);
   b.call("_emos_parallel_boot_fence",&[STATE],iff);
   assert_eq!(&b.mem[STATE as usize..STATE as usize+4],&[0,1,0,1]);
   assert_eq!(b.ports[0x9f],255);assert_eq!(b.ports[0xa0],0);assert_eq!(b.ports[0xa1],0);
   assert_eq!(b.ports[0xd1],0);assert_eq!(b.ports[0xd4],16);
   assert_eq!(b.ports[0xa2],keep|0x90);assert_eq!(b.ports[0xa3],keep|0x10);
   assert_eq!(b.ports[0xa4],keep);assert_eq!(b.ports[0xa5],keep);
   let writes:Vec<_>=b.io.iter().filter(|(w,_,_)|*w).map(|(_,p,v)|(*p,*v)).collect();
   assert_eq!(writes,vec![(0xd1,0),(0x9f,255),(0xa0,0),(0xa1,0),(0xd4,16),
    (0xa3,keep|0xb0),(0xa4,keep),(0xa5,keep),(0xa2,keep|0x90),
    (0xa3,keep|0x10),(0xa4,keep),(0xa5,keep)]);
   assert_eq!(b.peek(STATE-1),0xa5);assert_eq!(b.peek(STATE+4),0xa5);cases+=1;
  }
  for phase in [0,1,13,2,3,4,5] {for ready in [false,true] {for up in [false,true] {
   b.poke(STATE,phase);b.poke(STATE+1,if phase==13||phase==2 {0}else{1});b.poke(STATE+2,0);b.poke(STATE+3,1);
   b.ports[0x9f]=255;b.ports[0xa0]=0;b.ports[0xa1]=0;b.ports[0xa2]=0x4b|if ready {0x10}else{0};
   let (expected,next,v)=match phase {
    0=>(0,1,1),1=>if ready {(0,13,0)}else{(0,1,1)},
    13=>if ready {(0,13,0)}else{(5,2,0)},
    2=>if up {(0,3,1)}else{(5,2,0)},
    3=>if ready {(6,4,1)}else{(0,3,1)},
    4=>if ready {(6,4,1)}else{(2,0,1)},_ =>(2,5,1)};
   assert_eq!(b.call("_emos_parallel_boot_poll",&[STATE,up as u32],iff),expected);
   assert_eq!(b.peek(STATE),next);assert_eq!(b.peek(STATE+1),v);
   if expected==2 {assert!(!b.io.iter().any(|(w,_,_)|*w));}
   else {let writes:Vec<_>=b.io.iter().filter(|(w,_,_)|*w).map(|(_,p,val)|(*p,*val)).collect();
    assert_eq!(writes,vec![(0xa2,0x5b|if v!=0 {0x80}else{0})]);}
   assert_eq!(b.peek(STATE-1),0xa5);assert_eq!(b.peek(STATE+4),0xa5);cases+=1;
  }}}
  for bad in [0x9f,0xa0,0xa1] {
   b.poke(STATE,0);b.poke(STATE+1,1);b.poke(STATE+2,0);b.poke(STATE+3,1);
   b.ports[0x9f]=255;b.ports[0xa0]=0;b.ports[0xa1]=0;b.ports[bad]^=0x80;
   assert_eq!(b.call("_emos_parallel_boot_poll",&[STATE,1],iff),2);
   assert_eq!(b.peek(STATE),0);assert!(!b.io.iter().any(|(w,_,_)|*w));cases+=1;
  }
 }
 println!("PASS {cases} complete-image eZ80 boot-adapter cases; exact port sequence, preserved onboard GPIO, release refusal, restore gate, memory, IX/SP/IFF");
}
