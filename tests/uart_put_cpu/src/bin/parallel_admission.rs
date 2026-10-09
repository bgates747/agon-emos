//! Execute maintained, wrapper-linked EMOS admission gate. No peripheral IO.
use ez80::{Cpu, Machine, Reg16};
use std::{collections::HashMap, env, fs, path::Path};
const SP:u32=0xaff00;const EXIT:u32=0xa0000;const STATE:u32=0x70000;
const SESSION:u32=0x70100;const OFFER:u32=0x70200;const ACK:u32=0x70300;
struct Board {mem:Vec<u8>,symbols:HashMap<String,u32>}
impl Machine for Board {
 fn peek(&self,a:u32)->u8 {self.mem[a as usize]}
 fn poke(&mut self,a:u32,v:u8){self.mem[a as usize]=v;}
 fn use_cycles(&self,_:i32){}
 fn port_in(&mut self,a:u16)->u8{panic!("admission read port {a:x}")}
 fn port_out(&mut self,a:u16,_:u8){panic!("admission wrote port {a:x}")}
}
impl Board {
 fn new(dir:&str)->Self {
  let p=Path::new(dir);let bin=fs::read(p.join("MOS.bin")).unwrap();
  let symbols=fs::read_to_string(p.join("nm.txt")).unwrap().lines().filter_map(|l|{
   let v:Vec<_>=l.split_whitespace().collect();if v.len()!=3{None}else{Some((v[2].into(),u32::from_str_radix(v[0],16).unwrap()))}
  }).collect();
  let mut b=Self{mem:vec![0xa5;0x100000],symbols};b.mem[..bin.len()].copy_from_slice(&bin);b
 }
 fn call(&mut self,mode:u8,iff:bool)->u8 {
  let mut cpu=Cpu::new_ez80();cpu.state.reg.adl=true;cpu.state.reg.madl=true;
  cpu.state.reg.iff1=iff;cpu.state.reg.iff2=iff;
  cpu.state.reg.pc=self.symbols["_emos_parallel_handover_admit"];
  cpu.state.reg.set24(Reg16::SP,SP);cpu.state.reg.set24(Reg16::IX,0x123456);
  cpu.state.reg.set24(Reg16::IY,0x654321);self._poke24(SP,EXIT);
  for(i,a)in[STATE,mode as u32,SESSION,OFFER,ACK].iter().enumerate(){self._poke24(SP+3+3*i as u32,*a);}
  let mut n=0;
  while cpu.state.reg.pc!=EXIT{
   cpu.execute_instruction(self);n+=1;assert!(n<30000,"unbounded admission");
   assert_eq!(cpu.state.reg.iff1,iff);assert_eq!(cpu.state.reg.iff2,iff);
  }
  assert_eq!(cpu.state.reg.get24(Reg16::SP),SP+3);assert_eq!(cpu.state.reg.get24(Reg16::IX),0x123456);
  for (p,len) in [(STATE,4),(SESSION,6),(OFFER,16),(ACK,16)] {assert_eq!(self.peek(p-1),0xa5);assert_eq!(self.peek(p+len),0xa5);}
  (cpu.state.reg.get16(Reg16::AF)>>8) as u8
 }
}
fn main(){
 let args:Vec<_>=env::args().collect();assert_eq!(args.len(),3,"image-directory host-vectors");
 let mut b=Board::new(&args[1]);let data=fs::read_to_string(&args[2]).unwrap();let mut count=0;
 for iff in [false,true]{for line in data.lines(){
  let v:Vec<u8>=line.split_whitespace().map(|x|x.parse().unwrap()).collect();assert_eq!(v.len(),41);
  let state=[v[1],1,0,1];let mut session=v[2..8].to_vec();
  b.mem[STATE as usize..STATE as usize+4].copy_from_slice(&state);
  b.mem[SESSION as usize..SESSION as usize+6].copy_from_slice(&session);
  b.mem[OFFER as usize..OFFER as usize+16].copy_from_slice(&v[8..24]);
  b.mem[ACK as usize..ACK as usize+16].copy_from_slice(&v[24..40]);
  let actual=b.call(v[0],iff);assert_eq!(actual,v[40],"{line}");
  let expected=if actual!=0{session[4]=v[16];session[5]=v[17];[5,1,0,0]}else{state};
  assert_eq!(&b.mem[STATE as usize..STATE as usize+4],&expected,"{line}");
  assert_eq!(&b.mem[SESSION as usize..SESSION as usize+6],&session,"{line}");
  assert_eq!(&b.mem[OFFER as usize..OFFER as usize+16],&v[8..24]);
  assert_eq!(&b.mem[ACK as usize..ACK as usize+16],&v[24..40]);count+=1;
 }}
 println!("PASS {count} linked eZ80 admission cases; host/target agreement, C ABI, bounded return, IX/SP/IFF, no IO, immutable offers and memory guards");
}
