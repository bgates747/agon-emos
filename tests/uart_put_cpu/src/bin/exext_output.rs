//! Actual linked byte/count batching: data, refusal, C stack/IX/IFF and instruction scope.
//! No physical throughput claim: this stops before the4096-byte flush boundary.
use ez80::{Cpu,Machine,Reg16};
use std::{collections::HashMap,env,fs,path::Path};
const SP:u32=0xaff00;const EXIT:u32=0xa0000;const DATA:u32=0xa0200;
struct Board {mem:Vec<u8>}
impl Machine for Board {
 fn peek(&self,a:u32)->u8 {self.mem[a as usize]}
 fn poke(&mut self,a:u32,v:u8){self.mem[a as usize]=v;}
 fn use_cycles(&self,_:i32){}
 fn port_in(&mut self,a:u16)->u8{panic!("unexpected physical input {a:x}")}
 fn port_out(&mut self,a:u16,_:u8){panic!("unexpected physical output {a:x}")}
}
fn main(){
 let args:Vec<_>=env::args().collect();assert_eq!(args.len(),2);let p=Path::new(&args[1]);
 let data=fs::read(p.join("MOS.bin")).unwrap();
 let s:HashMap<String,u32>=fs::read_to_string(p.join("nm.txt")).unwrap().lines().filter_map(|l|{let v:Vec<_>=l.split_whitespace().collect();if v.len()!=3{None}else{Some((v[2].into(),u32::from_str_radix(v[0],16).unwrap()))}}).collect();
 for counted in [false,true] {
 let mut b=Board{mem:vec![0;0x1000000]};b.mem[..data.len()].copy_from_slice(&data);
 b.poke(s["_emos_exext_owned"],1);b.poke(s["_emosVduBackend"],2);
 for i in 0..1000 {b.poke(DATA+i,(i&255)as u8);}
 let mut c=Cpu::new_ez80();c.state.reg.adl=true;c.state.reg.madl=true;
 let mut steps=0;let calls=if counted {1}else{1000};
 for i in 0..calls {
  c.state.reg.set24(Reg16::SP,SP);b._poke24(SP,EXIT);
  c.state.reg.set24(Reg16::IX,0x123456);c.state.reg.set24(Reg16::IY,0x654321);
  c.state.reg.iff1=true;c.state.reg.iff2=true;
  if counted {b._poke24(SP+3,DATA);b._poke24(SP+6,1000);c.state.reg.pc=s["_emos_parallel_route_write_stream"];}else {b._poke24(SP+3,(i&255)as u32);c.state.reg.pc=s["_emos_parallel_route_write_byte"];}
  while c.state.reg.pc!=EXIT {c.execute_instruction(&mut b);steps+=1;assert!(steps<2000000);}
  assert_eq!(c.state.reg.get16(Reg16::AF)>>8,0);assert_eq!(c.state.reg.get24(Reg16::SP),SP+3);
  assert_eq!(c.state.reg.get24(Reg16::IX),0x123456);assert!(c.state.reg.iff1);
 }
 assert_eq!((u16::from(b.peek(s["_queued"]+1))<<8)|u16::from(b.peek(s["_queued"])),1000);
 for i in 0..1000 {assert_eq!(b.peek(s["_outgoing"]+i), (i&255)as u8);}
 assert_eq!(b.peek(s["_outgoing"]+1000),0);assert_eq!(b.peek(s["_emos_exext_busy"]),0);
 println!("PASS linked output counted={counted} bytes=1000 instructions={steps} (batch only, no physical transfer)");
 // Independent refusal cases must leave the queued stream unchanged.
 for kind in 0..4 {
  b.poke(s["_emos_exext_owned"],if kind==0 {0}else{1});b.poke(s["_emos_exext_busy"],if kind==1 {1}else{0});b.poke(s["_emosVduBackend"],if kind==2 {1}else{2});
  c.state.reg.iff1=kind!=3;c.state.reg.iff2=kind!=3;c.state.reg.set24(Reg16::SP,SP);b._poke24(SP,EXIT);b._poke24(SP+3,0xee);c.state.reg.pc=s["_emos_parallel_route_write_byte"];
  let mut guard=0;while c.state.reg.pc!=EXIT{c.execute_instruction(&mut b);guard+=1;assert!(guard<10000);}
  assert_ne!(c.state.reg.get16(Reg16::AF)>>8,0);assert_eq!((u16::from(b.peek(s["_queued"]+1))<<8)|u16::from(b.peek(s["_queued"])),1000);
  assert_eq!(b.peek(s["_outgoing"]+1000),0);assert_eq!(c.state.reg.iff1,kind!=3);
 }
 }
}
