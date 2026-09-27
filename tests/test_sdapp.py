"""Actual linked helper + two copies of checked engine; no P4 hardware claim."""
import ctypes as C
from pathlib import Path
import subprocess,tempfile,unittest,random
R=Path(__file__).resolve().parents[1]
class SdappTests(unittest.TestCase):
 @classmethod
 def setUpClass(cls):
  cls.tmp=tempfile.TemporaryDirectory();d=Path(cls.tmp.name)
  base=['cc','-std=c17','-Wall','-Wextra','-Werror','-Wno-misleading-indentation','-fsanitize=undefined','-fPIC','-I'+str(R/'tests/sdserve_host'),'-I'+str(R/'projects/sdserve/src'),'-I'+str(R/'lib/sdapp')]
  ren=['-Dservice_'+x+'=peer_'+x for x in ('init','init_mode','request','exiting','stop','progress')]
  subprocess.run(base+ren+['-c',str(R/'projects/sdserve/src/service.c'),'-o',str(d/'peer.o')],check=True)
  subprocess.run(base+['-Dservice_progress=adapter_progress','-c',str(R/'tests/sdserve_host/fs.c'),'-o',str(d/'fs.o')],check=True)
  subprocess.run(base+['-shared',str(R/'lib/sdapp/sdapp.c'),str(R/'projects/sdserve/src/service.c'),str(R/'tests/sdapp_host/peer.c'),str(d/'peer.o'),str(d/'fs.o'),'-o',str(d/'test.so')],check=True)
  cls.lib=C.CDLL(str(d/'test.so'));cls.cb=C.CFUNCTYPE(C.c_int,C.c_void_p)
  cls.lib.emos_file_transfer.argtypes=[C.c_uint,C.c_char_p,C.c_char_p,C.c_uint,cls.cb,C.c_void_p]
 @classmethod
 def tearDownClass(cls):cls.tmp.cleanup()
 def setUp(self):
  self.disk=tempfile.TemporaryDirectory();self.root=Path(self.disk.name)
  for n in ('agon','p4'):(self.root/n).mkdir()
  self.lib.fs_root(str(self.root).encode());self.lib.mock_reset(0)
 def tearDown(self):self.disk.cleanup()
 def run_transfer(self,d,src,dst,overwrite=0,cancel=None):
  cb=self.cb(cancel or (lambda _:0));r=self.lib.emos_file_transfer(d,src.encode(),dst.encode(),overwrite,cb,None)
  self.assertEqual(self.lib.mock_count(0),0);return r
 def test_both_directions_binary_boundaries(self):
  for d in (3,4):
   for size in (0,1,211,212,213,1027):
    with self.subTest(direction=d,size=size):
     self.lib.mock_reset(0);src=f'/{"p4" if d==3 else "agon"}/source';dst=f'/{"agon" if d==3 else "p4"}/out{size}'
     data=random.Random(size).randbytes(size);(self.root/src[1:]).write_bytes(data)
     self.assertEqual(self.run_transfer(d,src,dst),0);self.assertEqual((self.root/dst[1:]).read_bytes(),data)
 def test_faults_and_unsupported(self):
  (self.root/'p4/source').write_bytes(b'abc')
  for mode,expected in ((1,6),(2,6),(3,6),(4,1),(5,2),(7,2),(8,6),(9,6)):
   self.lib.mock_reset(mode);self.assertEqual(self.run_transfer(3,'/p4/source','/agon/out'),expected)
   self.assertFalse((self.root/'agon/out').exists())
 def test_overwrite_refused_then_checked(self):
  (self.root/'p4/source').write_bytes(b'new');(self.root/'agon/out').write_bytes(b'old')
  self.assertEqual(self.run_transfer(3,'/p4/source','/agon/out'),5)
  self.assertEqual((self.root/'agon/out').read_bytes(),b'old')
  self.lib.mock_reset(0);self.assertEqual(self.run_transfer(3,'/p4/source','/agon/out',1),0)
  self.assertEqual((self.root/'agon/out').read_bytes(),b'new');self.assertEqual((self.root/'agon/out.p17bak').read_bytes(),b'old')
 def test_cancel_and_reentry(self):
  self.assertEqual(self.run_transfer(3,'/p4/source','/agon/out',cancel=lambda _:1),4)
  self.assertEqual(self.lib.mock_count(1),0)
  (self.root/'agon/source').write_bytes(bytes(range(256))*3)
  self.assertEqual(self.run_transfer(4,'/agon/source','/p4/out',cancel=lambda _: self.lib.mock_count(3)>0),8)
  self.assertFalse((self.root/'p4/out').exists());self.assertTrue((self.root/'p4/out.p17part').exists())
 def test_lost_activation_ack_never_reports_success(self):
  self.lib.mock_reset(6);(self.root/'agon/source').write_bytes(b'confirmed bytes but lost receipt')
  self.assertEqual(self.run_transfer(4,'/agon/source','/p4/out'),8)
  self.assertEqual((self.root/'p4/out').read_bytes(),b'confirmed bytes but lost receipt')
 def test_descriptor_fragments_and_nested_call(self):
  a='a'*80;b='b'*80
  (self.root/'p4'/a).mkdir();(self.root/'agon'/b).mkdir()
  src='/p4/'+a+'/source';dst='/agon/'+b+'/out'
  (self.root/src[1:]).write_bytes(bytes(range(255)))
  nested=[]
  # Inner call must not release the outer lease; invoke directly here.
  def cancel(_):
   if not nested:
    cb=self.cb(lambda _:0);nested.append(self.lib.emos_file_transfer(3,b'/p4/no',b'/agon/no',0,cb,None))
   return 0
  self.assertEqual(self.run_transfer(3,src,dst,cancel=cancel),0)
  self.assertEqual(nested,[1]);self.assertEqual((self.root/dst[1:]).read_bytes(),bytes(range(255)))
 def test_bad_paths(self):
  for p in ('/', '/a/../b','/a//b','/a.','relative','/a\\b','/a?b'):
   self.assertEqual(self.run_transfer(3,p,'/agon/out'),3)
  self.assertEqual(self.lib.mock_count(1),0)
if __name__=='__main__':unittest.main()
