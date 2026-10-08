//! INTEG-015: execute the actual linked UART0 parser/C key owner/assembly
//! callback bridge. Only the application callback and hardware ports are models.
use ez80::{Cpu, Machine, Reg16};
use std::{collections::HashMap, env, fs, path::Path};
const SP: u32 = 0xaff00;
const EXIT: u32 = 0xa0000;
const CALLBACK: u32 = 0xa0100;
const NOTICE: [u8;10] = [b'P',b'3',b'D',b'R',1,1,0x34,0x12,0xcd,0xab];
struct Image { data: Vec<u8>, syms: HashMap<String,u32> }
impl Image {
    fn load(path: &str) -> Self {
        let p = Path::new(path);
        let syms = fs::read_to_string(p.join("nm.txt")).unwrap().lines()
            .filter_map(|l| { let v: Vec<_> = l.split_whitespace().collect();
                if v.len()!=3 {return None;} Some((v[2].to_string(),u32::from_str_radix(v[0],16).unwrap())) }).collect();
        Self { data: fs::read(p.join("MOS.bin")).unwrap(), syms }
    }
    fn a(&self,n:&str)->u32 { *self.syms.get(n).expect(n) }
}
struct Board { mem: Vec<u8>, calls: Vec<Vec<u8>>, edit: bool }
impl Machine for Board {
    fn peek(&self,a:u32)->u8 { self.mem[a as usize] }
    fn poke(&mut self,a:u32,v:u8) { self.mem[a as usize]=v; }
    fn use_cycles(&self,_:i32) {}
    fn port_in(&mut self,a:u16)->u8 { panic!("unexpected port read {a:x}") }
    fn port_out(&mut self,a:u16,_:u8) { panic!("unexpected port write {a:x}") }
}
impl Board {
    fn new(im:&Image, source:u8, backend:u8, registered:bool)->Self {
        let mut b=Self {mem:vec![0;0x1000000], calls:vec![],edit:false};
        b.mem[..im.data.len()].copy_from_slice(&im.data);
        b.poke(im.a("_emos_key_source"),source);
        b.poke(im.a("_emosVduBackend"),backend);
        b._poke24(im.a("_user_kbvector"), if registered {CALLBACK} else {0});
        b.poke(CALLBACK,0xc9); // application RET
        b
    }
    fn state(&self,im:&Image)->Vec<u8> {
        let mut v=vec![];
        for (n,count) in [("_keyascii",1),("_keymods",1),("_keydown",1),
            ("_keycode",1),("_keycount",1),("_keymap",16),("_held",32),
            ("_modifiers",1),("_zero_down",1)] {
            let at=im.a(n) as usize; v.extend_from_slice(&self.mem[at..at+count]);
        } v
    }
    fn byte(&mut self,im:&Image,value:u8) {
        let mut c=Cpu::new_ez80(); c.state.reg.adl=true; c.state.reg.madl=true;
        c.state.reg.pc=im.a("vdp_protocol"); c.state.reg.set24(Reg16::SP,SP);
        c.state.reg.set24(Reg16::HL,im.a("_vdp_protocol_data"));
        c.state.reg.set24(Reg16::BC,value as u32);
        self._poke24(SP,EXIT);
        let mut steps=0;
        while c.state.reg.pc != EXIT {
            if c.state.reg.pc==CALLBACK {
                let at=c.state.reg.get24(Reg16::DE) as usize;
                assert_eq!(at,im.a("_vdp_protocol_data") as usize,"DE payload ABI");
                self.calls.push(self.mem[at..at+10].to_vec());
                if self.edit { self.mem[at..at+4].copy_from_slice(&[b'z',0,2,1]); }
            }
            c.execute_instruction(self); steps+=1;
            assert!(steps<12000,"stuck at {:x}",c.state.reg.pc);
            assert!(!c.state.reg.iff1,"parser enabled interrupts");
        }
        assert_eq!(c.state.reg.get24(Reg16::SP),SP+3,"callback return/stack");
    }
    fn packet(&mut self,im:&Image,command:u8,data:&[u8]) {
        assert!(data.len()<=255); self.byte(im,command); self.byte(im,data.len() as u8);
        for &v in data {self.byte(im,v);}
        assert_eq!(self.peek(im.a("_vdp_protocol_state")),0,"packet recovery");
    }
}
fn main() {
    let args:Vec<_>=env::args().collect(); assert_eq!(args.len(),3,"old-image new-image");
    let old=Image::load(&args[1]); let im=Image::load(&args[2]); let mut checks=0;
    for source in 0..=2 {
        let mut b=Board::new(&old,source,0,true); b.packet(&old,0x81,&NOTICE);
        assert!(b.calls.is_empty(),"old-image rejection control"); checks+=1;
    }
    for source in 0..=2 { for backend in 0..=1 { for registered in [false,true] {
        let mut b=Board::new(&im,source,backend,registered);
        // Preserve an actually held key and nonzero sysvars through completion.
        b.poke(im.a("_keycount"),77); b.poke(im.a("_held")+2,0x55);
        b.poke(im.a("_keymap")+1,0x33);
        let before=b.state(&im); b.edit=true; b.packet(&im,0x81,&NOTICE);
        assert_eq!(b.calls.len(),usize::from(backend==0 && registered));
        if !b.calls.is_empty() {assert_eq!(b.calls[0],NOTICE);}
        assert_eq!(b.state(&im),before,"completion altered keys"); checks+=1;
    }}}
    for source in 0..=2 { for length in 0..=255 {
        let mut b=Board::new(&im,source,0,true);
        // Prime the receive buffer with a complete record, leaving stale tail.
        b.packet(&im,0x81,&NOTICE); b.calls.clear();
        let mut payload=vec![0x81;length];
        for (at,&v) in NOTICE.iter().enumerate().take(length) {payload[at]=v;}
        b.packet(&im,0x81,&payload);
        assert_eq!(b.calls.len(),usize::from(length==10),"source{source} length{length}");
        b.packet(&im,0x80,&[0x5a]); assert_eq!(b.peek(im.a("_gp")),0x5a);
        checks+=1;
    }}
    for at in 0..4 {
        let mut b=Board::new(&im,2,0,true); let mut bad=NOTICE; bad[at]^=1;
        b.packet(&im,0x81,&bad); assert!(b.calls.is_empty()); checks+=1;
    }
    for source in 0..=2 { for backend in 0..=1 {
        let mut b=Board::new(&im,source,backend,true);
        b.packet(&im,0x81,&[b'a',0,1,1]);
        let admitted=source==0;
        assert_eq!(b.calls.len(),usize::from(admitted));
        assert_eq!(b.peek(im.a("_keycount")),u8::from(admitted));
        let held=b.state(&im); b.packet(&im,0x81,&NOTICE);
        assert_eq!(b.state(&im),held);
        b.packet(&im,0x81,&[b'a',0,1,0]);
        assert_eq!(b.peek(im.a("_keycount")),2*u8::from(admitted));
        assert_eq!(b.peek(im.a("_held")),0); assert_eq!(b.peek(im.a("_keymap")),0);
        checks+=1;
    }}
    for bad in [[b'a',0,249,1],[b'a',0,1,2]] {
        let mut b=Board::new(&im,0,0,true); let before=b.state(&im);
        b.packet(&im,0x81,&bad); assert!(b.calls.is_empty()); assert_eq!(b.state(&im),before); checks+=1;
    }
    let mut b=Board::new(&im,0,0,true); b.edit=true;
    b.packet(&im,0x81,&[b'a',0,1,1]);
    assert_eq!(b.peek(im.a("_keyascii")),b'z'); assert_eq!(b.peek(im.a("_keycode")),2);
    assert_eq!(b.peek(im.a("_held")),4); checks+=1;
    println!("PASS {checks} linked CPU cases: old rejection, completion ABI, malformed/oversized recovery, source isolation, genuine keys, callback edits, unchanged key state");
}
