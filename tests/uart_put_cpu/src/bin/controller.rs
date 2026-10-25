//! INPUT-001: execute the linked controller gateway, packet parser and complete
//! UART IRQ with bounded, synthetic arrivals. This is an instruction-boundary
//! regression, not a calibrated physical UART/FIFO timing model. Legacy mode
//! preserves the installed CRC-bearing failure as a controlled baseline.
use ez80::{Cpu,Machine,Reg16};
use std::{fs,env,path::Path,collections::{HashMap,VecDeque}};
const SP:u32=0xaff00;const EXIT:u32=0xa0000;const REQ:u32=0x40000;
struct Board {mem:Vec<u8>,sent:Vec<Vec<u8>>,incoming:VecDeque<u8>,fifo:VecDeque<u8>,overrun:bool,pcdr:u8,inject:bool,legacy:bool,max_masked:usize,period:usize}
impl Machine for Board {
 fn peek(&self,a:u32)->u8{self.mem[a as usize]}
 fn poke(&mut self,a:u32,v:u8){self.mem[a as usize]=v;}
 fn use_cycles(&self,_:i32){}
 fn port_in(&mut self,a:u16)->u8{match a {0x9e=>self.pcdr,0xd5=>if self.overrun{0x62}else if !self.fifo.is_empty(){0x61}else{0x60},0xd0=>self.fifo.pop_front().unwrap(),_=>panic!("unexpected port read {a:x}")}}
 fn port_out(&mut self,a:u16,v:u8){match a {0x9e=>self.pcdr=v,0xd1=>{},_=>panic!("unexpected port write {a:x}")}}
}
fn sym(s:&HashMap<String,u32>,n:&str)->u32{*s.get(n).expect(n)}
fn call(b:&mut Board,s:&HashMap<String,u32>,name:&str,args:&[u32],irq:bool)->u32{
 let mut c=Cpu::new_ez80();c.state.reg.adl=true;c.state.reg.madl=true;
 c.state.reg.pc=sym(s,name);c.state.reg.set24(Reg16::SP,SP);
 c.state.reg.set24(Reg16::IX,0xab1234);c.state.reg.set24(Reg16::IY,0xbc1234);
 c.state.reg.iff1=irq;c.state.reg.iff2=irq;b._poke24(SP,EXIT);
 for(i,&a)in args.iter().enumerate(){b._poke24(SP+3+(i as u32)*3,a);}
 let mut steps=0;let mut arrival=0;let mut started=false;let mut masked=0;b.max_masked=0;
 while c.state.reg.pc!=EXIT{
  if c.state.reg.pc==sym(s,"_emos_keyboard_send"){
   let sp=c.state.reg.get24(Reg16::SP);let at=b._peek24(sp+3)as usize;let n=b._peek24(sp+6)as usize;
   b.sent.push(b.mem[at..at+n].to_vec());
   c.state.reg.set16(Reg16::AF,0);c.state.reg.pc=b._peek24(sp);c.state.reg.set24(Reg16::SP,sp+3);
  }else{c.execute_instruction(b);}
  steps+=1;
  if c.state.reg.iff1 {masked=0;} else {masked+=1;b.max_masked=b.max_masked.max(masked);}
  if b.inject {
   if !c.state.reg.iff1 {started=true;}
   if started {arrival+=1;if arrival%b.period==0 && b.pcdr&4==0 {
    if let Some(byte)=b.incoming.pop_front(){if b.fifo.len()==16{b.overrun=true;}else{b.fifo.push_back(byte);}}
   }}
   if c.state.reg.iff1 && (!b.fifo.is_empty()||b.overrun) && b.peek(sym(s,"_emos_key_faulted"))==0 {
    let sp=c.state.reg.get24(Reg16::SP)-4;b.poke(sp,3);b._poke24(sp+1,c.state.reg.pc);
    c.state.reg.set24(Reg16::SP,sp);c.state.reg.pc=sym(s,"_emos_keyboard_irq_entry");c.state.reg.iff1=false;c.state.reg.iff2=false;
   }
  }
  assert!(steps<1000000,"stuck {} {:x}",name,c.state.reg.pc);
 }
 let result=c.state.reg.get24(Reg16::HL);
 if name!="_emos_keyboard_byte" {println!("{}: HL={} instructions={} max_masked={}",name,result,steps,b.max_masked);}
 assert_eq!(c.state.reg.get24(Reg16::SP),SP+3);assert_eq!(c.state.reg.get24(Reg16::IX),0xab1234);
 result
}
fn service(b:&mut Board,s:&HashMap<String,u32>,op:u8)->u32{
 b.mem[REQ as usize..REQ as usize+66].fill(0);b.poke(REQ,66);b.poke(REQ+2,1);
 b.poke(REQ+4,3);b.poke(REQ+5,10);b.poke(REQ+6,2);
 b._poke24(REQ+10,0x40100);b._poke24(REQ+13,1);b.poke(0x40100,op);
 if op==1{b._poke24(REQ+16,0x40200);b._poke24(REQ+19,if b.legacy{98}else{96});}
 b.mem[REQ as usize+25..REQ as usize+28].copy_from_slice(b"ext");
 b.mem[REQ as usize+41..REQ as usize+51].copy_from_slice(b"controller");
 call(b,s,"_emos_gateway",&[REQ],true)
}
fn main(){
 let args:Vec<_>=env::args().collect();assert!(args.len()==2||args.len()==3,"IMAGE_DIR [--legacy]");let path=Path::new(&args[1]);let legacy=args.get(2).map(|s|s=="--legacy").unwrap_or(false);
 let s:HashMap<String,u32>=fs::read_to_string(path.join("nm.txt")).unwrap().lines().filter_map(|l|{let v:Vec<_>=l.split_whitespace().collect();if v.len()!=3{return None}Some((v[2].to_string(),u32::from_str_radix(v[0],16).unwrap()))}).collect();
 let rom=fs::read(path.join("MOS.bin")).unwrap();let mut b=Board{mem:vec![0;0x1000000],sent:vec![],incoming:VecDeque::new(),fifo:VecDeque::new(),overrun:false,pcdr:0,inject:false,legacy,max_masked:0,period:256};b.mem[..rom.len()].copy_from_slice(&rom);
 b.poke(sym(&s,"_emosPolicy"),3);b.poke(sym(&s,"_emos_key_source"),2);b.poke(sym(&s,"_uart1_keyboard_owned"),1);b.poke(sym(&s,"_clock"),60);
 assert_eq!(service(&mut b,&s,0),0);assert_eq!(service(&mut b,&s,1),31);assert_eq!(service(&mut b,&s,2),0);
 assert_eq!(b.sent.len(),1);println!("request={:x?}",b.sent[0]);
 let mut p=vec![0u8;if legacy{98}else{96}];p[..4].copy_from_slice(if legacy{b"CIS1"}else{b"CIS2"});p[4..8].copy_from_slice(&b.sent[0][7..11]);p[8..10].copy_from_slice(&b.sent[0][15..17]);p[10]=3;p[12]=1;p[20]=2;p[28]=1;p[36]=100;p[44]=7;if legacy{p[96..].copy_from_slice(&[0x2a,0x94]);} // Frozen historical fixture; no checksum calculation.
 for byte in [0x8e,p.len()as u8].iter().chain(p.iter()){
  call(&mut b,&s,"_emos_keyboard_byte",&[*byte as u32],false);
 }
 assert_eq!(service(&mut b,&s,1),0);assert_eq!(b.peek(0x4020a),3);assert_eq!(b.peek(0x4022c),7);
 assert_eq!(service(&mut b,&s,2),0);
 p[8]=2;p[36]=200;b.incoming.extend([0x8e,p.len()as u8]);b.incoming.extend(p);b.inject=true;
 let read=service(&mut b,&s,1);b.inject=false;
 println!("overlap READ={} FIFO-overrun={} EMOS-fault={} remaining={}",read,b.overrun,b.peek(sym(&s,"_emos_key_faulted")),b.incoming.len());
 if legacy {
  assert!(b.overrun);assert_eq!(b.peek(sym(&s,"_emos_key_faulted")),1);assert_eq!(service(&mut b,&s,2),35);
  println!("PASS legacy reproduction: reply overlap causes UART fault, REQUEST35 and keyboard loss");
 }else{
  assert_eq!(read,31);assert!(!b.overrun);assert_eq!(b.peek(sym(&s,"_emos_key_faulted")),0);assert!(b.max_masked<512);
  // Complete the pending frame through the real parser after the busy READ.
  let remaining:Vec<_>=b.fifo.drain(..).chain(b.incoming.drain(..)).collect();
  for byte in remaining{call(&mut b,&s,"_emos_keyboard_byte",&[byte as u32],false);}
  assert_eq!(service(&mut b,&s,1),0);assert_eq!(b.peek(0x40224),200);assert!(b.max_masked<512);
  // A keyboard record can arrive during an ordinary cached READ. The full
  // UART IRQ must preserve the foreground state and publish Escape normally.
  b.incoming.extend([0x81,4,27,0,27,1]);b.period=64;b.inject=true;
  assert_eq!(service(&mut b,&s,1),0);b.inject=false;
  assert!(b.fifo.is_empty()&&b.incoming.is_empty(),"whole Escape record must traverse the real IRQ during READ");
  assert!(!b.overrun);assert_eq!(b.peek(sym(&s,"_emos_key_faulted")),0);
  assert_eq!(b.peek(sym(&s,"_keyascii")),27);assert_eq!(b.peek(sym(&s,"_keydown")),1);
  println!("PASS corrected linked controller: pending READ busy, coherent fresh copy, bounded masking and Escape delivery");
 }
}
