"""Compile the real private CLI wrapper with scripted editor/service outcomes."""
from pathlib import Path
import subprocess,tempfile,unittest
ROOT=Path(__file__).resolve().parents[1]
class CliResumeTests(unittest.TestCase):
 def test_silent_jobs_errors_and_normal_exit(self):
  s=(ROOT/'src/mos.c').read_text();a=s.index('UINT24 mos_input(');b=s.index('// Parse a MOS command',a)
  prefix=r'''
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
typedef int UINT24; typedef int INT24; typedef unsigned char BYTE;
#define EMOS_CLI_SERVICE 254
static char output[256]; static int edits, jobs, failure;
static int capture(const char *fmt,...) { va_list ap;va_start(ap,fmt);int n=vsnprintf(output+strlen(output),sizeof(output)-strlen(output),fmt,ap);va_end(ap);return n; }
#define printf capture
static char *expandVariableToken(const char *n){(void)n;return "*";}
static void umm_free(void *p){(void)p;}
static int emos_cli_editline(char *b,int n){(void)b;(void)n;return edits++<2?EMOS_CLI_SERVICE:13;}
static int emos_admission_dispatch(void){return ++jobs==1?failure:0;}
static void mos_error(int e){assert(e==35);printf("\n\rBackend error\n\r");}
'''
  suffix=r'''
int main(void){char b[8];assert(mos_input(b,sizeof b)==13);assert(jobs==2);assert(!strcmp(output,"*\n\r"));
output[0]=0;edits=jobs=0;failure=35;assert(mos_input(b,sizeof b)==13);assert(jobs==2);assert(!strcmp(output,"*\n\rBackend error\n\r*\n\r"));return 0;}
'''
  with tempfile.TemporaryDirectory() as d:
   src=Path(d)/'cli.c';exe=Path(d)/'cli';src.write_text(prefix+s[a:b]+suffix)
   subprocess.run(['cc','-std=c17','-Wall','-Wextra','-Werror',str(src),'-o',str(exe)],check=True)
   subprocess.run([str(exe)],check=True)
