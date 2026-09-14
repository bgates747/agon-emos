//! Execute the complete linked public sender, including its private block.
use ez80::{Cpu,Machine,Reg16};
use std::{cell::Cell,env,fs,path::Path};
const SP:u32=0xaf000; const EXIT:u32=0xa0000; const DATA:u32=0x70000;
#[derive(Debug,PartialEq,Clone)] enum Io { R(u16,u8), W(u16,u8) }
struct Image { data:Vec<u8>, send:u32, block:u32, clock:u32, owned:u32, fault:u32, request:u32 }
impl Image { fn load(path:&str)->Self {
 let p=Path::new(path);let nm=fs::read_to_string(p.join("nm.txt")).unwrap();
 let addr=|n:&str| {let l=nm.lines().find(|l|l.split_whitespace().last()==Some(n)).expect(n);u32::from_str_radix(l.split_whitespace().next().unwrap(),16).unwrap()};
 let data=fs::read(p.join("MOS.bin")).unwrap();let clock_fn=addr("_emos_keyboard_clock") as usize;assert_eq!(data[clock_fn],0x3a);let clock=u32::from_le_bytes([data[clock_fn+1],data[clock_fn+2],data[clock_fn+3],0]);
 Self{data,send:addr("_emos_keyboard_send"),block:if nm.contains("_emos_keyboard_transmit_block"){addr("_emos_keyboard_transmit_block")}else{addr("_transmit_block")},clock,owned:addr("_uart1_keyboard_owned"),fault:addr("_emos_key_faulted"),request:addr("_fault_requested")}
}}
#[derive(Clone,Debug)] struct Case {len:usize,seed:u8,step:u8,blocked:usize,empty:usize,error:Option<usize>,inject:Option<(usize,bool)>,mutate:bool,iff:bool, initial:Option<(u16,u32)>}
struct Board {mem:Vec<u8>,clock:u32,reads:Cell<usize>,x:Case,io:Vec<Io>,out:Vec<u8>,polls:usize,pcdr:u8}
impl Machine for Board {
 fn peek(&self,a:u32)->u8 {if a==self.clock{let n=self.reads.get();self.reads.set(n+1);self.x.seed.wrapping_add((n as u8).wrapping_mul(self.x.step))}else{self.mem[a as usize]}}
 fn poke(&mut self,a:u32,v:u8){self.mem[a as usize]=v;}
 fn use_cycles(&self,_:i32){}
 fn port_in(&mut self,a:u16)->u8 {let v=match a {
  0xd5=>{self.polls+=1;if self.x.mutate&&self.polls==1{self.mem[DATA as usize]=0xee;}if self.x.error==Some(self.out.len()){0x22}else if self.polls<=self.x.empty{0}else{0x20}},
  0x9e=>self.pcdr | if self.polls<=self.x.blocked{8}else{0},_=>panic!("IN {a:x}")};self.io.push(Io::R(a,v));v}
 fn port_out(&mut self,a:u16,v:u8){match a{0xd0=>self.out.push(v),0xd1=>assert_eq!(v,0),0x9e=>self.pcdr=v,_=>panic!("OUT {a:x}")};self.io.push(Io::W(a,v));}
}
#[derive(Debug,PartialEq)] struct Result {value:u8,io:Vec<Io>,out:Vec<u8>,reads:usize,deadline:Vec<u8>,request:u8}
fn run(im:&Image,x:&Case)->(Result,usize){
 let mut b=Board{mem:vec![0;0x100000],clock:im.clock,reads:Cell::new(0),x:x.clone(),io:vec![],out:vec![],polls:0,pcdr:0xa3};b.mem[..im.data.len()].copy_from_slice(&im.data);
 for i in 0..x.len {b.mem[DATA as usize+i]=(i as u8).wrapping_mul(73).wrapping_add(0x42);}
 b.mem[im.owned as usize]=1;b._poke24(SP,EXIT);b._poke24(SP+3,DATA);b._poke24(SP+6,0x990000|x.len as u32);
 let mut c=Cpu::new_ez80();c.state.reg.adl=true;
 b.mem[EXIT as usize+16]=0xd9;b.mem[EXIT as usize+17]=0x08;
 let shadow=[(Reg16::BC,0x123456),(Reg16::DE,0x234567),(Reg16::HL,0x345678)];
 for(r,v)in shadow{c.state.reg.set24(r,v);}c.state.reg.set16(Reg16::AF,0x4567);c.state.reg.pc=EXIT+16;c.execute_instruction(&mut b);c.execute_instruction(&mut b);
 c.state.reg.set24(Reg16::IX,0xabcd12);c.state.reg.set24(Reg16::IY,0xbcde23);c.state.reg.set24(Reg16::SP,SP);c.state.reg.pc=im.send;c.state.reg.iff1=x.iff;c.state.reg.iff2=x.iff;
 let mut steps=0;let mut dp=None;let mut injected=false;let mut max_masked=0;let mut masked=0;
 while c.state.reg.pc!=EXIT {
  if c.state.reg.pc==im.block{let p=b._peek24(c.state.reg.get24(Reg16::SP)+9);dp=Some(p);if let Some((elapsed,budget))=x.initial{b.mem[p as usize+1]=elapsed as u8;b.mem[p as usize+2]=(elapsed>>8)as u8;b._poke24(p+3,budget);}}
  let n=b.io.len();let irq=c.state.reg.iff1;c.execute_instruction(&mut b);
  if n!=b.io.len(){assert!(!irq&&!c.state.reg.iff1,"unmasked I/O {x:?}");}
  if !c.state.reg.iff1{masked+=1;max_masked=max_masked.max(masked);}else{masked=0;}
  if let Some((at,fault))=x.inject {if !injected&&b.reads.get()>=at&&c.state.reg.iff1{b.mem[if fault{im.fault}else{im.owned} as usize]=if fault{1}else{0};injected=true;}}
  steps+=1;assert!(steps<60000000,"no return {:x}: {x:?}",c.state.reg.pc);
 }
 assert_eq!(c.state.reg.get24(Reg16::SP),SP+3);assert_eq!(c.state.reg.get24(Reg16::IX),0xabcd12);assert_eq!(c.state.reg.iff1,x.iff);assert_eq!(c.state.reg.iff2,x.iff);
 if x.iff{assert!(max_masked<100,"excess interrupt exclusion {max_masked}");}
 let value=c.state.reg.a();c.state.reg.pc=EXIT+16;c.execute_instruction(&mut b);c.execute_instruction(&mut b);
 for(r,v)in shadow{assert_eq!(c.state.reg.get24(r),v,"shadow {r:?}");}assert_eq!(c.state.reg.get16(Reg16::AF),0x4567);
 let deadline=dp.map(|p|b.mem[p as usize..p as usize+6].to_vec()).unwrap_or_default();
 (Result{value,io:b.io,out:b.out,reads:b.reads.get(),deadline,request:b.mem[im.request as usize]},steps)
}
fn main(){let args:Vec<_>=env::args().collect();assert_eq!(args.len(),3);let a=Image::load(&args[1]);let b=Image::load(&args[2]);let mut count=0;
 let base=Case{len:8,seed:0,step:0,blocked:0,empty:0,error:None,inject:None,mutate:false,iff:true,initial:None};let mut cases=vec![];
 for len in [0,1,2,8,257,4097,65535]{for seed in [0,254]{for step in [0,2]{cases.push(Case{len,seed,step,..base.clone()});}}}
 for blocked in [1,16,80,usize::MAX]{for empty in [0,1,24]{for step in [0,2,127,255]{cases.push(Case{blocked,empty,step,mutate:true,..base.clone()});}}}
 for error in [0,1,3,7]{cases.push(Case{error:Some(error),..base.clone()});}
 for at in [1,2,3,9]{for fault in [false,true]{cases.push(Case{inject:Some((at,fault)),..base.clone()});}}
 for elapsed in [0,511,599]{for budget in [1,2,65536,65537,262144]{for step in [0,2]{cases.push(Case{initial:Some((elapsed,budget)),step,..base.clone()});}}}
 cases.push(Case{iff:false,..base.clone()});
 for x in cases {let(ra,sa)=run(&a,&x);let(rb,sb)=run(&b,&x);assert!(ra==rb,"case {x:?}: values {}/{}, reads {}/{}, out lengths {}/{}, deadline {:?}/{:?}, request {}/{}, first I/O mismatch {:?}",ra.value,rb.value,ra.reads,rb.reads,ra.out.len(),rb.out.len(),ra.deadline,rb.deadline,ra.request,rb.request,ra.io.iter().zip(&rb.io).position(|(a,b)|a!=b));if x.len==65535&&x.step==0&&x.seed==0{println!("Ready 65535 instructions: C={sa}, candidate={sb}");}count+=1;}
 println!("PASS {count} full linked sender comparisons: port sequence, deadline, exact/partial payload, timeout, IRQ-off refusal, fault/owner loss, retention, IFF and ABI");
}
