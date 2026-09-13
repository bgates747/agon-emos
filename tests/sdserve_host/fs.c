#define _POSIX_C_SOURCE 200809L
#include <agon/mos.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
static char disk[400];static unsigned fault,remaining,progress;
void fs_root(const char *path) { strcpy(disk,path);fault=remaining=progress=0; }
void fs_fault(unsigned operation,unsigned occurrence) { fault=operation;remaining=occurrence; }
unsigned fs_progress(void) { return progress; }
void service_progress(void) { ++progress; }
static int bad(unsigned op) { if(op!=fault || !remaining) return 0;return !--remaining; }
static void path(char *out,const char *in) { strcpy(out,disk);strcat(out,in); }
static uint8_t error(void) { return errno==ENOENT?FR_NO_FILE:errno==EEXIST?FR_EXIST:FR_DISK_ERR; }
uint8_t ffs_fopen(FIL *f,const char *name,uint8_t mode) {
    char p[550];path(p,name);f->error=0;
    if(bad(1)) return FR_DISK_ERR;
    if(mode&FA_CREATE_NEW) {
        int fd=open(p,O_RDWR|O_CREAT|O_EXCL,0600);if(fd<0)return error();
        f->file=fdopen(fd,"w+b");
    } else f->file=fopen(p,(mode&FA_WRITE)?"r+b":"rb");
    return f->file?0:error();
}
uint8_t ffs_fclose(FIL *f) { int failed=bad(4),r=fclose(f->file);f->file=NULL;return failed||r?FR_DISK_ERR:0; }
unsigned ffs_fread(FIL *f,char *p,unsigned n) {
    if(bad(2)) { f->error=FR_DISK_ERR;return 0; }
    unsigned got=fread(p,1,n,f->file);if(ferror(f->file))f->error=FR_DISK_ERR;return got;
}
unsigned ffs_fwrite(FIL *f,const char *p,unsigned n) {
    if(bad(3)) { return n?fwrite(p,1,n-1,f->file):0; } // disk-full short write, no hard error
    unsigned got=fwrite(p,1,n,f->file);if(ferror(f->file))f->error=FR_DISK_ERR;return got;
}
uint8_t ffs_ferror(FIL *f) { return f->error; }
uint8_t ffs_fsync(FIL *f) { return bad(5)||fflush(f->file)||fsync(fileno(f->file))?FR_DISK_ERR:0; }
uint8_t ffs_fsize(FIL *f,uint32_t *size) {
    struct stat s;if(fstat(fileno(f->file),&s))return error();*size=s.st_size;return 0;
}
uint8_t ffs_flseek_p(FIL *f,uint32_t *off) { return fseek(f->file,*off,SEEK_SET)?FR_DISK_ERR:0; }
uint8_t ffs_stat(FILINFO *info,const char *name) {
    char p[550];struct stat s;path(p,name);
    if(stat(p,&s))return error();info->fsize=s.st_size;info->fattrib=S_ISDIR(s.st_mode)?AM_DIR:0;return 0;
}
uint8_t ffs_dopen(DIR *d,const char *name) { path(d->path,name);d->dir=opendir(d->path);return d->dir?0:error(); }
uint8_t ffs_dclose(DIR *d) { return closedir(d->dir)?FR_DISK_ERR:0; }
uint8_t ffs_dread(DIR *d,FILINFO *info) {
    struct dirent *e;
    do { e=readdir(d->dir); } while(e && (!strcmp(e->d_name,".")||!strcmp(e->d_name,"..")));
    memset(info,0,sizeof(*info));if(!e)return 0;
    strcpy(info->fname,e->d_name);char p[800];struct stat s;
    strcpy(p,d->path);strcat(p,"/");strcat(p,e->d_name);
    if(stat(p,&s))return error();info->fsize=s.st_size;info->fattrib=S_ISDIR(s.st_mode)?AM_DIR:0;return 0;
}
uint8_t ffs_unlink(const char *name) { char p[550];path(p,name);return bad(7)?FR_DISK_ERR:unlink(p)?error():0; }
uint8_t ffs_rename(const char *source,const char *target) {
    char a[550],b[550];path(a,source);path(b,target);
    if(bad(6))return FR_DISK_ERR;
    if(!access(b,F_OK))return FR_EXIST;return rename(a,b)?error():0;
}
