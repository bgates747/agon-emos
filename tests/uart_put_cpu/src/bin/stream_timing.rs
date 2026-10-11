//! Execute the actual linked private UART observation. No hardware timing claim.
use ez80::{Cpu,Machine,Reg16};
use std::{collections::HashMap,env,fs,path::Path};
const SP:u32=0xaff00; const EXIT:u32=0xa0000;
struct Board{mem:Vec<u8>,ports:[u8;256],reads:usize,writes:usize}
impl Machine for Board {
 fn peek(&self,a:u32)->u8{self.mem[a as usize]}
 fn poke(&mut self,a:u32,v:u8){self.mem[a as usize]=v;}
 fn use_cycles(&self,_:i32){}
 fn port_in(&mut self,a:u16)->u8{
  assert!([0xd5,0x9e].contains(&a),"unexpected UART observer input {a:x}");
  if a==0xd5{self.reads+=1;}self.ports[a as usize]
 }
 fn port_out(&mut self,a:u16,v:u8){
  assert!([0xd1,0x9e].contains(&a),"unexpected UART observer output {a:x}");
  self.writes+=1;self.ports[a as usize]=v;
 }
}
fn main(){
 let args:Vec<_>=env::args().collect();assert_eq!(args.len(),2);let p=Path::new(&args[1]);
 let image=fs::read(p.join("MOS.bin")).unwrap();
 let symbols:HashMap<String,u32>=fs::read_to_string(p.join("nm.txt")).unwrap().lines().filter_map(|l|{
  let v:Vec<_>=l.split_whitespace().collect();if v.len()!=3{None}else{Some((v[2].into(),u32::from_str_radix(v[0],16).unwrap()))}
 }).collect();
 let mut b=Board{mem:vec![0;0x100000],ports:[0;256],reads:0,writes:0};
 b.mem[..image.len()].copy_from_slice(&image);let mut cases=0;
 for iff in [false,true]{for route in 0..3{for owned in 0..3{for fault in 0..2{for status in 0..256{
  b.poke(symbols["_emosVduBackend"],route);b.poke(symbols["_uart1_keyboard_owned"],owned);
  b.poke(symbols["_emos_key_faulted"],fault);b.ports[0xd5]=status as u8;b.ports[0xd1]=1;b.ports[0x9e]=0x51;
  b.reads=0;b.writes=0;
  let admitted=iff&&route==1&&owned==1&&fault==0;
  let error=status&0x9e!=0;
  let expected=if !admitted{3}else if error{2}else if status&0x40!=0{1}else{0};
  let mut c=Cpu::new_ez80();c.state.reg.adl=true;c.state.reg.madl=true;
  c.state.reg.iff1=iff;c.state.reg.iff2=iff;c.state.reg.pc=symbols["_emos_stream_timing_uart"];
  c.state.reg.set24(Reg16::SP,SP);b._poke24(SP,EXIT);
  c.state.reg.set24(Reg16::IX,0x123456);c.state.reg.set24(Reg16::IY,0x654321);
  let mut count=0;while c.state.reg.pc!=EXIT{c.execute_instruction(&mut b);count+=1;assert!(count<500);}
  assert_eq!(c.state.reg.get24(Reg16::HL),expected);
  assert_eq!(c.state.reg.iff1,iff);assert_eq!(c.state.reg.iff2,iff);
  assert_eq!(c.state.reg.get24(Reg16::SP),SP+3);
  assert_eq!(c.state.reg.get24(Reg16::IX),0x123456);assert_eq!(c.state.reg.get24(Reg16::IY),0x654321);
  assert_eq!(b.reads,if admitted{1}else{0});
  assert_eq!(b.writes,if admitted&&error{2}else{0});
  if admitted&&error{assert_eq!(b.ports[0xd1],0);assert_eq!(b.ports[0x9e],0x55);}
  cases+=1;
 }}}}}
 println!("PASS {cases} linked UART drain cases: TEMT/THRE, every LSR value, errors, ownership, route, IX/IY/SP/IFF; no hardware timing");
}
