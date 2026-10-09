//! Maintained session owners in the complete wrapper-linked EMOS image.
use ez80::{Cpu, Machine, Reg16};
use std::{collections::HashMap, env, fs, path::Path};
const SP:u32=0xaff00;const EXIT:u32=0xa0000;const H:u32=0x70000;const S:u32=0x70100;
const A:u32=0x70200;const B:u32=0x70300;const OUT:u32=0x70400;
struct Board {mem:Vec<u8>,sym:HashMap<String,u32>}
impl Machine for Board {
 fn peek(&self,a:u32)->u8{self.mem[a as usize]}
 fn poke(&mut self,a:u32,v:u8){self.mem[a as usize]=v;}
 fn use_cycles(&self,_:i32){}
 fn port_in(&mut self,p:u16)->u8{panic!("session read port {p:x}")}
 fn port_out(&mut self,p:u16,_:u8){panic!("session wrote port {p:x}")}
}
impl Board {
 fn new(dir:&str)->Self{
  let p=Path::new(dir);let bin=fs::read(p.join("MOS.bin")).unwrap();
  let sym=fs::read_to_string(p.join("nm.txt")).unwrap().lines().filter_map(|l|{
   let v:Vec<_>=l.split_whitespace().collect();if v.len()!=3{None}else{Some((v[2].into(),u32::from_str_radix(v[0],16).unwrap()))}
  }).collect();let mut b=Self{mem:vec![0xa5;0x100000],sym};b.mem[..bin.len()].copy_from_slice(&bin);b
 }
 fn set(&mut self,name:&str,v:u8){self.poke(self.sym[name],v);}
 fn owner(&mut self,held:u8){
  self.set("_parallel_reserved",held);self.set("_transitioning",held);
  self.set("_uart1_keyboard_owned",1);self.set("_fault_requested",0);
  self.set("_stop_requested",0);self.set("_emos_key_faulted",0);
 }
 fn call(&mut self,name:&str,args:&[u32],iff:bool)->u8{
  let mut c=Cpu::new_ez80();c.state.reg.adl=true;c.state.reg.madl=true;
  c.state.reg.iff1=iff;c.state.reg.iff2=iff;c.state.reg.pc=self.sym[name];
  c.state.reg.set24(Reg16::SP,SP);c.state.reg.set24(Reg16::IX,0x123456);
  self._poke24(SP,EXIT);for(i,a)in args.iter().enumerate(){self._poke24(SP+3+3*i as u32,*a);}
  let mut n=0;while c.state.reg.pc!=EXIT{c.execute_instruction(self);n+=1;assert!(n<50000,"unbounded {name}");}
  assert_eq!(c.state.reg.get24(Reg16::SP),SP+3);assert_eq!(c.state.reg.get24(Reg16::IX),0x123456);
  assert_eq!(c.state.reg.iff1,iff);assert_eq!(c.state.reg.iff2,iff);c.state.reg.a()
 }
 fn put(&mut self,p:u32,bytes:&[u8]){self.mem[p as usize..p as usize+bytes.len()].copy_from_slice(bytes);}
 fn at(&self,p:u32,n:usize)->&[u8]{&self.mem[p as usize..p as usize+n]}
}
fn main(){
 let args:Vec<_>=env::args().collect();assert_eq!(args.len(),3);let mut b=Board::new(&args[1]);
 let vectors=fs::read_to_string(&args[2]).unwrap();let mut cases=0;
 for iff in [false,true]{for line in vectors.lines(){
  let v:Vec<u8>=line.split_whitespace().map(|x|x.parse().unwrap()).collect();assert_eq!(v.len(),98);
  b.owner(v[1]);b.put(H,&v[3..7]);b.put(S,&v[7..18]);b.put(A,&v[18..34]);b.put(B,&v[34..50]);b.put(OUT,&v[50..66]);
  let(name,a)=match v[0]{
   1=>("_emos_parallel_session_start",vec![H,S,A,OUT]),
   2=>("_emos_parallel_session_accept",vec![H,S,A,OUT]),
   3=>("_emos_parallel_session_admit",vec![H,v[2] as u32,S,A,B]),
   4=>("_emos_parallel_session_cancel",vec![H,S]),_=>panic!("op")};
  let r=b.call(name,&a,iff);let rejected=!iff && v[0]!=4;
  if v[0]!=4{assert_eq!(r,if rejected{0}else{v[66]},"return {line}");}
  assert_eq!(b.at(H,4),if rejected{&v[3..7]}else{&v[67..71]},"handover {line}");
  assert_eq!(b.at(S,11),if rejected{&v[7..18]}else{&v[71..82]},"session {line}");
  assert_eq!(b.at(OUT,16),if rejected{&v[50..66]}else{&v[82..98]},"out {line}");
  assert_eq!(b.at(A,16),&v[18..34]);assert_eq!(b.at(B,16),&v[34..50]);
  for(p,n)in[(H,4),(S,11),(A,16),(B,16),(OUT,16)]{assert_eq!(b.peek(p-1),0xa5);assert_eq!(b.peek(p+n),0xa5);}
  cases+=1;
 }}
 for name in ["_parallel_reserved","_transitioning","_uart1_keyboard_owned","_fault_requested","_stop_requested","_emos_key_faulted"]{
  for value in 0..=3u8{for iff in [false,true]{
   b.owner(1);b.set(name,value);
   let ready=iff && match name{
    "_parallel_reserved"|"_uart1_keyboard_owned"=>value==1,"_transitioning"=>value!=0,_=>value==0};
   assert_eq!(b.call("_emos_keyboard_parallel_reserved",&[],iff),u8::from(ready));cases+=1;
  }}
 }
 println!("PASS {cases} linked eZ80 session checks; host/target agreement, real reservation and IRQ guards, IX/SP/IFF, immutable inputs, memory guards and no IO");
}
