//! Execute the real VBlank/save/ExExt/packet callback boundaries. Only physical
//! transport service is modeled; nested IRQ delivery is injected at that boundary.
use ez80::{Cpu,Machine,Reg16};
use std::{collections::HashMap,env,fs,path::Path};
const SP:u32=0xaff00;const EXIT:u32=0xa0000;const CALLBACK:u32=EXIT+0x100;const SWAP:u32=EXIT+0x200;
struct Board {mem:Vec<u8>}
impl Machine for Board {
 fn peek(&self,a:u32)->u8 {self.mem[a as usize]}
 fn poke(&mut self,a:u32,v:u8){self.mem[a as usize]=v;}
 fn use_cycles(&self,_:i32){}
 fn port_in(&mut self,a:u16)->u8{assert_eq!(a,0x9a);0x5a}
 fn port_out(&mut self,a:u16,v:u8){assert_eq!(a,0x9a);assert_eq!(v,0x5a);}
}
fn ret(c:&mut Cpu,b:&Board){let sp=c.state.reg.get24(Reg16::SP);c.state.reg.pc=b._peek24(sp);c.state.reg.set24(Reg16::SP,sp+3);}
fn swap(c:&mut Cpu,b:&mut Board){let pc=c.state.reg.pc;c.state.reg.pc=SWAP;c.execute_instruction(b);c.execute_instruction(b);c.state.reg.pc=pc;}
fn main(){
 let args:Vec<_>=env::args().collect();assert_eq!(args.len(),2);let p=Path::new(&args[1]);
 let data=fs::read(p.join("MOS.bin")).unwrap();
 let s:HashMap<String,u32>=fs::read_to_string(p.join("nm.txt")).unwrap().lines().filter_map(|l|{let v:Vec<_>=l.split_whitespace().collect();if v.len()!=3{None}else{Some((v[2].into(),u32::from_str_radix(v[0],16).unwrap()))}}).collect();
 for nested in [false,true] {
 let mut b=Board{mem:vec![0;0x1000000]};b.mem[..data.len()].copy_from_slice(&data);
 b.poke(SWAP,0xd9);b.poke(SWAP+1,0x08);b.poke(CALLBACK,0xc9);
 for (n,v) in [("_emos_exext_owned",1),("_emosVduBackend",2),("_emos_console_owned",1),("_uart1_keyboard_owned",1),("_emos_key_source",2)]{b.poke(s[n],v);}
 b._poke24(s["_user_kbvector"],CALLBACK);
 b.poke(SP,3);b._poke24(SP+1,EXIT);
 let mut c=Cpu::new_ez80();c.state.reg.adl=true;c.state.reg.madl=true;
 let primary=[(Reg16::BC,0x123456),(Reg16::DE,0x234567),(Reg16::HL,0x345678),(Reg16::IX,0x456789),(Reg16::IY,0x56789a)];
 let alternate=[(Reg16::BC,0x876543),(Reg16::DE,0x765432),(Reg16::HL,0x654321)];
 for &(r,v) in &alternate{c.state.reg.set24(r,v);}c.state.reg.set16(Reg16::AF,0x4321);swap(&mut c,&mut b);
 for &(r,v) in &primary{c.state.reg.set24(r,v);}c.state.reg.set16(Reg16::AF,0x1234);
 let mut services=0;let mut callbacks=0;let mut nesting=false;let mut resume=None;let mut steps=0;
 for tick in 0..2 {
 b.poke(SP,3);b._poke24(SP+1,EXIT);
 c.state.reg.set24(Reg16::SP,SP);c.state.reg.pc=s["_vblank_handler"];c.state.reg.iff1=true;c.state.reg.iff2=true;
 while c.state.reg.pc!=EXIT{
  let pc=c.state.reg.pc;
  if Some(pc)==resume && nesting {nesting=false;resume=None;}
  if pc==s["_emos_keyboard_tick"] {c.state.reg.pc=s["_emos_exext_tick"];continue;}
  if pc==s["_emos_exext_service"] {
   assert!(c.state.reg.iff1);assert_eq!(b.peek(s["_emos_exext_busy"]),1);services+=1;assert_eq!(services,1);
   if nested {
    let sp=c.state.reg.get24(Reg16::SP)-4;b.poke(sp,3);b._poke24(sp+1,pc+1);
    c.state.reg.set24(Reg16::SP,sp);resume=Some(pc+1);nesting=true;c.state.reg.pc=s["_vblank_handler"];continue;
   }
  }
  if pc==s["_emos_exext_service"] || (nested && pc==s["_emos_exext_service"]+1 && !nesting) {
   assert!(c.state.reg.iff1);
   let at=s["_incoming"] as usize;b.mem[at..at+12].copy_from_slice(&[0x81,4,b'a',0,1,1,0x81,4,0,0,1,0]);
   b.poke(s["_received"],12);b.poke(s["_received"]+1,0);
   for r in [Reg16::BC,Reg16::DE,Reg16::HL,Reg16::IY]{c.state.reg.set24(r,0xdeadde);}c.state.reg.set16(Reg16::AF,0xabcd);
   swap(&mut c,&mut b);for r in [Reg16::BC,Reg16::DE,Reg16::HL]{c.state.reg.set24(r,0xdead12);}c.state.reg.set16(Reg16::AF,0xabcd);swap(&mut c,&mut b);
   ret(&mut c,&b);continue;
  }
  if pc==CALLBACK {assert!(!c.state.reg.iff1);callbacks+=1;assert_eq!(callbacks,tick+1);let at=c.state.reg.get24(Reg16::DE) as usize;assert_eq!(&b.mem[at..at+4],if tick==0 {&[b'a',0,1,1]} else {&[0,0,1,0]});}
  c.execute_instruction(&mut b);steps+=1;assert!(steps<50000,"stuck {:x}",c.state.reg.pc);
 }
 assert_eq!(services,1);assert_eq!(callbacks,tick+1);assert_eq!(b.peek(s["_emos_exext_busy"]),0);assert_eq!(b.peek(s["_keydown"]),if tick==0 {1} else {0});
 assert_eq!(b.peek(s["_keyascii"]),if tick==0 {b'a'} else {0});
 assert_eq!(c.state.reg.get24(Reg16::SP),SP+4);assert!(c.state.reg.iff1);
 for &(r,v) in &primary{assert_eq!(c.state.reg.get24(r),v,"primary {:?}",r);}assert_eq!(c.state.reg.get16(Reg16::AF),0x1234);
 swap(&mut c,&mut b);for &(r,v) in &alternate{assert_eq!(c.state.reg.get24(r),v,"alternate {:?}",r);}assert_eq!(c.state.reg.get16(Reg16::AF),0x4321);
 swap(&mut c,&mut b);
 }
 println!("PASS linked VBlank ExExt nested={} bundled down/up separated, callbacks, primary/alternate/IX/IY/SP/IFF",nested);
 }
}
