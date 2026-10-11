"""Execute the maintained ordinary ExExt queue owner with modeled peers."""
from pathlib import Path
import subprocess, tempfile, unittest
ROOT=Path(__file__).resolve().parents[1]
class ExextStreamTests(unittest.TestCase):
 def test_owner(self):
  with tempfile.TemporaryDirectory() as tmp:
   exe=Path(tmp)/'stream'
   subprocess.run(['cc','-std=c17','-Wall','-Wextra','-Werror','-Wno-unused-function','-Wno-endif-labels',
    '-DEMOS_EXEXT_STREAM=1','-fsanitize=address,undefined','-fno-sanitize-recover=all',
    f'-I{ROOT/"tests/host"}',f'-I{ROOT/"src"}',
    str(ROOT/'tests/exext_stream_host.c'),str(ROOT/'src/emos_exext.c'),'-o',str(exe)],check=True)
   subprocess.run([str(exe)],check=True,timeout=30)
if __name__=='__main__':unittest.main()
