//! Complete-image session negotiation: actual UART TX, receive framer and ISR
//! dispatcher. Register/clock model only; no electrical or timing claim.
use ez80::{Cpu,Machine,Reg16};
use std::{collections::HashMap,env,fs,path::Path};
const SP:u32=0xaff00;const EXIT:u32=0xa0000;const H:u32=0x70000;const S:u32=0x70100;const T:u32=0x70200;
struct Board{mem:Vec<u8>,sym:HashMap<String,u32>,ports:[u8;256],tx:Vec<u8>,iff:bool,replies:usize,missing:bool,bad:Option<usize>,duplicate:bool}
impl Machine for Board{
 fn peek(&self,a:u32)->u8{self.mem[a as usize]}
 fn poke(&mut self,a:u32,v:u8){self.mem[a as usize]=v;}
 fn use_cycles(&self,_:i32){}
 fn port_in(&mut self,a:u16)->u8{assert!(!self.iff);assert!([0x9e,0xd5].contains(&a),"unexpected read {a:x}");self.ports[a as usize]}
 fn port_out(&mut self,a:u16,v:u8){assert!(!self.iff);assert_eq!(a,0xd0,"unexpected output");self.tx.push(v);}
}
fn seal(p:&mut [u8]){let mut crc=0xffffu16;for v in &p[..14]{crc^=(*v as u16)<<8;for _ in 0..8{crc=(crc<<1)^if crc&0x8000!=0{0x1021}else{0};}}p[14]=crc as u8;p[15]=(crc>>8) as u8;}
impl Board{
 fn new(dir:&str)->Self{
  let p=Path::new(dir);let bin=fs::read(p.join("MOS.bin")).unwrap();
  let mut b=Self{mem:vec![0;0x100000],sym:fs::read_to_string(p.join("nm.txt")).unwrap().lines().filter_map(|l|{let v:Vec<_>=l.split_whitespace().collect();if v.len()!=3{None}else{Some((v[2].into(),u32::from_str_radix(v[0],16).unwrap()))}}).collect(),ports:[0;256],tx:vec![],iff:false,replies:0,missing:false,bad:None,duplicate:false};
  b.mem[..bin.len()].copy_from_slice(&bin);b
 }
 fn set(&mut self,n:&str,v:u8){self.poke(self.sym[n],v);}
 fn get(&self,n:&str)->u8{self.peek(self.sym[n])}
 fn receive(&mut self,p:&[u8],sp:u32){
  for value in [0xff,16].into_iter().chain(p.iter().copied()){
   self.execute("_emos_keyboard_byte",&[value as u32],false,sp,false);
  }
 }
 fn execute(&mut self,name:&str,args:&[u32],iff:bool,sp:u32,inject:bool)->u8{
  let mut c=Cpu::new_ez80();c.state.reg.adl=true;c.state.reg.madl=true;c.state.reg.iff1=iff;c.state.reg.iff2=iff;c.state.reg.pc=self.sym[name];c.state.reg.set24(Reg16::SP,sp);c.state.reg.set24(Reg16::IX,0x123456);
  self._poke24(sp,EXIT);for(i,a)in args.iter().enumerate(){self._poke24(sp+3+3*i as u32,*a);}
  let mut n=0;
  while c.state.reg.pc!=EXIT{
   self.iff=c.state.reg.iff1;c.execute_instruction(self);n+=1;
   if inject && n%500==0{let a=self.sym["_clock"];self.poke(a,self.peek(a).wrapping_add(2));}
   if inject && !self.missing && self.tx.len()/19>self.replies && c.state.reg.iff1 && self.get("_parallel_reserved")==1{
    let base=self.replies*19;assert_eq!(&self.tx[base..base+3],&[23,0,247]);
    let mut p=self.tx[base+3..base+19].to_vec();assert_eq!(p[2],2);assert_eq!(p[12],3);
    let mut valid=p.clone();seal(&mut valid);assert_eq!(valid,p,"transmitted CRC");
    p[3]|=0x80;p[8..12].copy_from_slice(&[0x78,0x56,0x34,0x12]);seal(&mut p);
    if let Some(i)=self.bad{p[i]^=1;seal(&mut p);}
    self.replies+=1;let isr_sp=c.state.reg.get24(Reg16::SP)-256;self.receive(&p,isr_sp);
    if self.duplicate{p[8]^=0x20;seal(&mut p);self.receive(&p,isr_sp);}
   }
   assert!(n<2_000_000,"unbounded {name}");
  }
  assert_eq!(c.state.reg.get24(Reg16::SP),sp+3);assert_eq!(c.state.reg.get24(Reg16::IX),0x123456);assert_eq!(c.state.reg.iff1,iff);assert_eq!(c.state.reg.iff2,iff);c.state.reg.a()
 }
 fn run(&mut self,iff:bool)->u8{self.execute("_emos_parallel_session_negotiate",&[H,S,T],iff,SP,true)}
 fn active(&mut self){
  self.set("_uart1_keyboard_owned",1);self.set("_uart1_rts_owned",1);self.set("_parallel_reserved",1);self.set("_transitioning",1);self.set("_emosParallelWriterLock",1);
  self.ports[0x9e]=0xa1;self.ports[0xd5]=0x60;
  self.mem[H as usize..H as usize+4].copy_from_slice(&[4,1,0,1]);self.mem[T as usize..T as usize+4].copy_from_slice(&[1,2,3,4]);
  for(p,n)in[(H,4),(S,11),(T,4)]{self.poke(p-1,0xa5);self.poke(p+n,0xa5);}
 }
 fn guards(&self){for(p,n)in[(H,4),(S,11),(T,4)]{assert_eq!(self.peek(p-1),0xa5);assert_eq!(self.peek(p+n),0xa5);}assert_eq!(&self.mem[T as usize..T as usize+4],&[1,2,3,4]);assert_eq!(self.get("_parallel_reserved"),1);assert_eq!(self.get("_emosParallelWriterLock"),1);assert_eq!(self.get("_emosVduBackend"),0);}
}
fn main(){
 let args:Vec<_>=env::args().collect();assert_eq!(args.len(),2);let mut cases=0;
 for duplicate in [false,true]{let mut b=Board::new(&args[1]);b.active();b.duplicate=duplicate;assert_eq!(b.run(true),1);assert_eq!(b.tx.len(),38);assert_eq!(b.peek(S+10),3);assert_eq!(&b.mem[S as usize..S as usize+4],&[0x78,0x56,0x34,0x12]);b.guards();cases+=1;}
 for index in [2,3,4,12,13]{let mut b=Board::new(&args[1]);b.active();b.bad=Some(index);assert_eq!(b.run(true),0);assert_eq!(b.tx.len(),19);assert_eq!(b.peek(S+10),0);assert_eq!(b.peek(H),0);b.guards();cases+=1;}
 let mut b=Board::new(&args[1]);b.active();b.missing=true;assert_eq!(b.run(true),0);assert_eq!(b.tx.len(),19);assert_eq!(b.peek(S+10),0);b.guards();cases+=1;
 let mut b=Board::new(&args[1]);b.active();assert_eq!(b.run(false),0);assert!(b.tx.is_empty());b.guards();cases+=1;
 let mut b=Board::new(&args[1]);b.active();b.set("_emos_console_owned",1);assert_eq!(b.run(true),0);assert!(b.tx.is_empty());assert_eq!(b.peek(S+10),0);b.guards();cases+=1;
 println!("PASS {cases} complete-image eZ80 control cases: actual reserved UART TX, receive framer, ISR reply dispatcher, foreground commit, duplicate/invalid/missing replies, IRQ refusal, memory/IX/SP/IFF and reservation preservation");
}
