"""Real foreground engine through fault-injected POSIX adapter, not FAT qualification."""
import ctypes as C
from pathlib import Path
import random
import struct as S
import subprocess
import tempfile
import unittest
import zlib

ROOT=Path(__file__).resolve().parents[1]
def wire(session,seq,op,payload=b''):
    h=S.pack('<2sBBIIBBH',b'SD',1,1,session,seq,op,0,len(payload))
    return h+S.pack('<I',zlib.crc32(h+payload))+payload
def path(p):
    p=p.encode('ascii');return bytes([len(p)])+p

class SdserveTests(unittest.TestCase):
    FAST=0
    def test_reviewed_codec_matches_owner_when_available(self):
        owner=ROOT.parent/'agon-extender/vdp/video/extender/storage/sd_wire.h'
        if not owner.exists():self.skipTest('Independent checkout; owner codec unavailable')
        self.assertEqual(owner.read_bytes(),(ROOT/'projects/sdserve/src/sd_wire.h').read_bytes())
    @classmethod
    def setUpClass(cls):
        cls.build=tempfile.TemporaryDirectory();lib=Path(cls.build.name)/'sd.so'
        subprocess.run(['cc','-std=c17','-Wall','-Wextra','-Werror',
                        '-Wno-misleading-indentation','-fsanitize=undefined',
                        '-fPIC','-shared','-I'+str(ROOT/'tests/sdserve_host'),
                        str(ROOT/'projects/sdserve/src/service.c'),
                        str(ROOT/'tests/sdserve_host/fs.c'),'-o',str(lib)],check=True)
        cls.lib=C.CDLL(str(lib));cls.lib.service_request.argtypes=[C.c_char_p,C.c_uint,C.c_void_p]
    @classmethod
    def tearDownClass(cls): cls.build.cleanup()
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory();self.disk=Path(self.temp.name)
        (self.disk/'test').mkdir();self.lib.fs_root(str(self.disk).encode())
        self.assertEqual(self.lib.service_init_mode(b'/test',101,self.FAST),1)
        self.seq=0;self.sid=19;self.rpc(1)
    def tearDown(self): self.lib.service_stop();self.temp.cleanup()
    def send(self,request):
        out=C.create_string_buffer(240);n=self.lib.service_request(request,len(request),out)
        if not n:return None
        b=out.raw[:n];self.assertEqual(S.unpack_from('<I',b,16)[0],zlib.crc32(b[:16]+b[20:]))
        self.assertEqual(b[4:13],request[4:13]);return b[13],b[20:]
    def rpc(self,op,p=b'',status=0):
        self.seq+=1;self.last=wire(self.sid,self.seq,op,p);result=self.send(self.last)
        self.assertIsNotNone(result);self.assertEqual(result[0],status);return result[1]
    def begin(self,data,name='/test/game.bin'):
        return self.rpc(5,S.pack('<III',73,len(data),zlib.crc32(data))+path(name))
    def stage(self,data):
        self.begin(data)
        for at in range(0,len(data),212):self.rpc(6,S.pack('<II',73,at)+data[at:at+212])
        self.rpc(7,S.pack('<I',73))
    def move_descriptor(self,source,destination):
        a=source.encode();b=destination.encode()
        return S.pack('<BBHHH',6,0,len(a),len(b),0)+a+b
    def move_frag(self,d,offset=0,count=184,status=0,crc=None):
        return self.rpc(14,S.pack('<HHI',len(d),offset,zlib.crc32(d) if crc is None else crc)+d[offset:offset+count],status)
    def test_root_metadata_uses_directory_api(self):
        self.lib.service_stop()
        self.assertEqual(self.lib.service_init_mode(b'/',101,self.FAST),1)
        self.seq=0;self.rpc(1)
        self.assertEqual(self.rpc(2,path('/')),S.pack('<IB',0,16))
    def test_directory_primitives_and_guards(self):
        self.rpc(12,path('/test/new'));self.assertTrue((self.disk/'test/new').is_dir())
        self.rpc(12,path('/test/new'),6)
        self.rpc(13,b'\x01'+path('/test/new'));self.assertTrue((self.disk/'test/new').exists())
        (self.disk/'test/new/file').write_bytes(b'keep')
        self.rpc(13,b'\x00'+path('/test/new'),6)
        for p in ('/test','/TEST','/test/../else','/test/a.p17part','/test/X.P17BAK','/test/sdserve.bin','/'):
            self.rpc(12,path(p),1);self.rpc(13,b'\x01'+path(p),1)
        self.rpc(13,b'\x00'+path('/test/new/file'));self.rpc(13,b'\x00'+path('/test/new'))
        self.assertFalse((self.disk/'test/new').exists())
    def test_move_full_paths_final_only_replay_and_collision(self):
        a='/test/'+('a'*114);b='/test/'+('b'*114);(self.disk/a[1:]).write_bytes(b'payload')
        d=self.move_descriptor(a,b);self.assertEqual(len(d),248)
        self.assertEqual(self.move_frag(d),S.pack('<HB',184,0));self.assertTrue((self.disk/a[1:]).exists())
        self.assertEqual(self.move_frag(d,184),S.pack('<HB',248,1))
        self.assertEqual(self.send(self.last),(0,S.pack('<HB',248,1)))
        self.assertFalse((self.disk/a[1:]).exists());self.assertEqual((self.disk/b[1:]).read_bytes(),b'payload')
        (self.disk/'test/other').write_bytes(b'old');self.move_frag(self.move_descriptor(b,'/test/other'),status=6)
        self.assertEqual((self.disk/'test/other').read_bytes(),b'old')
    def test_move_fragments_cancelled_by_interleave_and_bad_identity(self):
        a='/test/'+('a'*114);b='/test/'+('b'*114);(self.disk/a[1:]).write_bytes(b'x');d=self.move_descriptor(a,b)
        self.move_frag(d);self.rpc(2,path(a));self.move_frag(d,184,status=1)
        self.move_frag(d);self.move_frag(d,183,status=1)
        self.move_frag(d,crc=0);self.move_frag(d,184,crc=0,status=7)
        self.assertTrue((self.disk/a[1:]).exists());self.assertFalse((self.disk/b[1:]).exists())
    def test_move_relationships_reserved_and_fault(self):
        (self.disk/'test/dir').mkdir();(self.disk/'test/dir/a').write_bytes(b'x')
        for a,b in [('/test','/test/z'),('/test/dir','/test/dir/sub'),('/test/dir/a','/test/dir'),('/test/dir/a','/test/x.p17meta'),('/test/dir/a','/test/DIR/A')]:
            self.move_frag(self.move_descriptor(a,b),status=1)
        self.lib.fs_fault(6,1);self.move_frag(self.move_descriptor('/test/dir/a','/test/b'),status=6)
        self.assertEqual((self.disk/'test/dir/a').read_bytes(),b'x')
    def test_directory_mutations_busy_during_stage(self):
        self.begin(b'x');self.rpc(12,path('/test/new'),3);self.rpc(13,b'\x00'+path('/test/game.p17part'),3)
        self.move_frag(self.move_descriptor('/test/a','/test/b'),status=3)
        self.rpc(9,S.pack('<I',73))

    def test_empty_binary_boundaries_and_game_size(self):
        for size in (0,1,211,212,213,256,65536,131731):
            with self.subTest(size=size):
                data=random.Random(size).randbytes(size);self.stage(data)
                before=self.last;self.assertEqual(self.send(before),(0,S.pack('<II',size,zlib.crc32(data))))
                self.rpc(8,S.pack('<I',73))
                result=bytearray()
                for at in range(0,max(1,size),216):
                    p=self.rpc(4,S.pack('<IH',at,216)+path('/test/game.bin'))
                    self.assertEqual(S.unpack_from('<I',p)[0],size);result.extend(p[4:])
                self.assertEqual(bytes(result),data)
                self.assertEqual((self.disk/'test/game.bin').read_bytes(),data)
                self.rpc(10,b'\x03'+path('/test/game.bin'))
    def test_replay_conflict_corruption_sequence_and_session(self):
        self.begin(b'abcd');self.rpc(6,S.pack('<II',73,0)+b'abcd')
        self.assertEqual(self.send(self.last),(0,S.pack('<II',73,4)))
        self.assertEqual(self.send(wire(self.sid,self.seq,6,b'different'))[0],5)
        self.assertEqual(self.send(wire(self.sid,self.seq+2,7,S.pack('<I',73)))[0],5)
        self.assertEqual(self.send(wire(22,1,1))[0],3)
        damaged=bytearray(self.last);damaged[-1]^=128;self.assertIsNone(self.send(bytes(damaged)))
        self.rpc(7,S.pack('<I',73));self.rpc(9,S.pack('<I',73))
        self.assertEqual(self.send(wire(22,1,1))[0],0)
    def test_root_traversal_nul_and_bounds(self):
        for p in ('/else','/test/../else','/test//game','/test/a.','/test/a ', '/test/a:b', '/test/a\\b'):
            self.rpc(2,path(p),1)
        self.rpc(2,b'\x08/test/a\0',1)
        self.rpc(4,S.pack('<IH',0,217)+path('/test/game.bin'),1)
        self.rpc(5,S.pack('<III',1,0,0)+path('/test/game.p17bak'),1)
    def test_staged_path_reserves_room_for_readback(self):
        long_name='/test/'+('a'*107)
        self.assertEqual(len(long_name),113)
        self.rpc(5,S.pack('<III',1,0,0)+path(long_name),1)
        self.assertEqual(list((self.disk/'test').iterdir()),[])
        self.begin(b'',long_name[:-1]);self.rpc(7,S.pack('<I',73))
        self.assertEqual(self.rpc(4,S.pack('<IH',0,1)+path(long_name[:-1]+'.p17part')),b'\0'*4)
        self.rpc(9,S.pack('<I',73))
    def test_bad_transfer_is_an_accepted_error_not_a_stale_session(self):
        self.begin(b'a')
        for op in (6,7,8,9):
            payload=S.pack('<I',74)+(S.pack('<I',0)+b'a' if op==6 else b'')
            self.rpc(op,payload,1)
        self.rpc(6,S.pack('<II',73,0)+b'a');self.rpc(7,S.pack('<I',73))
        self.rpc(9,S.pack('<I',73))
    def test_disk_full_sync_and_close_failure_preserve_target(self):
        old=self.disk/'test/game.bin';old.write_bytes(b'old')
        for op in (3,8):  # short write and hard device error
            self.begin(b'data');self.lib.fs_fault(op,1)
            self.rpc(6,S.pack('<II',73,0)+b'data',6)
            self.assertEqual(old.read_bytes(),b'old')
            self.assertEqual(self.rpc(10,b'\0'+path('/test/game.bin')),b'\x07')
            self.rpc(10,b'\x02'+path('/test/game.bin'))
        for op in (4,5):
            self.begin(b'data');self.rpc(6,S.pack('<II',73,0)+b'data')
            self.lib.fs_fault(op,1);self.rpc(7,S.pack('<I',73),6)
            self.assertEqual(old.read_bytes(),b'old');self.rpc(10,b'\x02'+path('/test/game.bin'))
    def test_interrupted_rename_restore_and_cleanup(self):
        old=self.disk/'test/game.bin';old.write_bytes(b'old')
        self.stage(b'new');self.lib.fs_fault(6,2);self.rpc(8,S.pack('<I',73),6)
        self.assertFalse(old.exists());self.assertEqual((self.disk/'test/game.bin.p17bak').read_bytes(),b'old')
        self.assertEqual(self.rpc(10,b'\0'+path('/test/game.bin')),b'\x0e')
        self.rpc(10,b'\x01'+path('/test/game.bin'));self.rpc(10,b'\x02'+path('/test/game.bin'))
        self.assertEqual(old.read_bytes(),b'old')
    def test_journal_retirement_failure_and_corruption(self):
        (self.disk/'test/game.bin').write_bytes(b'old');self.stage(b'new')
        self.lib.fs_fault(7,1);self.rpc(8,S.pack('<I',73),6)
        self.assertEqual(self.rpc(10,b'\0'+path('/test/game.bin')),b'\x0d')
        self.rpc(10,b'\x02'+path('/test/game.bin'));self.rpc(10,b'\x03'+path('/test/game.bin'))
        self.stage(b'again');self.lib.service_stop()
        meta=self.disk/'test/game.bin.p17meta';meta.write_bytes(b'bad')
        self.assertEqual(self.rpc(10,b'\0'+path('/test/game.bin')),b'\x17')
        self.rpc(10,b'\x02'+path('/test/game.bin'),8)
    def test_read_error_and_listing(self):
        (self.disk/'test/a').write_bytes(b'data');self.lib.fs_fault(2,1)
        self.rpc(4,S.pack('<IH',0,4)+path('/test/a'),6)
        p=self.rpc(3,S.pack('<I',0)+path('/test'))
        self.assertEqual(p[4],0);self.assertEqual(p[11:],b'a')
        self.assertEqual(self.rpc(3,S.pack('<I',1)+path('/test'))[4],1)
    def test_unfinished_stage_survives_stop(self):
        (self.disk/'test/game.bin').write_bytes(b'old');self.begin(b'data')
        self.rpc(6,S.pack('<II',73,0)+b'da');self.lib.service_stop()
        self.lib.service_init(b'/test',102);self.sid=20;self.seq=0;self.rpc(1)
        self.assertEqual(self.rpc(10,b'\0'+path('/test/game.bin')),b'\x07')
        self.rpc(10,b'\x02'+path('/test/game.bin'))
    def test_first_upload_orphan_can_be_explicitly_abandoned(self):
        self.begin(b'data');self.rpc(6,S.pack('<II',73,0)+b'da')
        self.lib.service_stop();self.lib.service_init(b'/test',103)
        self.sid=21;self.seq=0;self.rpc(1)
        self.assertEqual(self.rpc(10,b'\0'+path('/test/game.bin')),b'\x06')
        self.rpc(10,b'\x02'+path('/test/game.bin'))
        self.assertEqual(list((self.disk/'test').iterdir()),[])

    def test_mode_and_verification_read_cost(self):
        self.assertEqual(S.unpack('<IHH',self.rpc(1)),(101,212,47|(16 if self.FAST else 0)))
        data=bytes(range(256))*16
        before=self.lib.fs_read_bytes()
        self.stage(data);self.rpc(8,S.pack('<I',73))
        self.assertEqual(self.lib.fs_read_bytes()-before,20+(0 if self.FAST else 2*len(data)))
        self.assertEqual((self.disk/'test/game.bin').read_bytes(),data)

    def test_declared_crc_is_not_verified_in_fast_mode(self):
        self.rpc(5,S.pack('<III',73,4,123)+path('/test/game.bin'))
        self.rpc(6,S.pack('<II',73,0)+b'data')
        self.rpc(7,S.pack('<I',73),0 if self.FAST else 7)
        if self.FAST:
            self.rpc(8,S.pack('<I',73))
            self.assertEqual((self.disk/'test/game.bin').read_bytes(),b'data')

    def test_post_finish_corruption_demonstrates_mode_tradeoff(self):
        self.stage(b'good')
        (self.disk/'test/game.bin.p17part').write_bytes(b'evil')
        self.rpc(8,S.pack('<I',73),0 if self.FAST else 7)
        self.assertEqual((self.disk/'test/game.bin').read_bytes(),b'evil')

    def test_self_overwrite_and_early_finish_still_rejected(self):
        self.rpc(5,S.pack('<III',73,0,0)+path('/test/SdSeRvE.BiN'),1)
        self.begin(b'abcd');self.rpc(6,S.pack('<II',73,0)+b'ab')
        self.rpc(7,S.pack('<I',73),7);self.rpc(9,S.pack('<I',73))

class FastSdserveTests(SdserveTests):
    FAST=1

if __name__=='__main__':unittest.main()
