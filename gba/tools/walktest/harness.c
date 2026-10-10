// mGBA harness for walktest.py: harness ROM SAVE, then a script on stdin, one command a line:
//   k KEYS FRAMES  hold keys (A, B, U, D, L, R = left, right, S = START, E = SELECT), then release
//   w FRAMES       wait
//   p FILE.ppm     save the screen
//   K KEYS         hold keys (or - for none) for the h lines after it
//   h FRAMES FILE  run, writing a hash of each frame's screen to FILE (one per line; timing.py)
#include <mgba/core/core.h>
#include <mgba/core/log.h>
#include <mgba-util/vfs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
static void nolog(struct mLogger* l, int c, enum mLogLevel lv, const char* f, va_list a){}
int main(int argc, char** argv){
  static struct mLogger lg = { .log = nolog }; mLogSetDefaultLogger(&lg);
  struct mCore* core = mCoreFind(argv[1]); core->init(core); mCoreInitConfig(core, NULL);
  unsigned w,h; core->desiredVideoDimensions(core,&w,&h);
  color_t* buf = malloc(w*h*sizeof(color_t)); core->setVideoBuffer(core, buf, w);
  mCoreLoadFile(core, argv[1]);
  struct VFile* sv = VFileOpen(argv[2], O_RDWR|O_CREAT); core->loadSave(core, sv);
  core->reset(core);
  char line[1024]; uint32_t held=0;
  while(fgets(line,sizeof line,stdin)){
    char cmd[8], a[512]; int n=0; a[0]=0;
    int c = sscanf(line,"%7s %511s %d",cmd,a,&n);
    if(c<1) continue;
    if(cmd[0]=='w'){ int f=atoi(a); core->setKeys(core,0); for(int i=0;i<f;i++) core->runFrame(core); }
    else if(cmd[0]=='k'){ uint32_t k=0; for(char*p=a;*p;p++){ switch(*p){case 'A':k|=1;break;case 'B':k|=2;break;case 'E':k|=4;break;case 'S':k|=8;break;case 'R':k|=16;break;case 'L':k|=32;break;case 'U':k|=64;break;case 'D':k|=128;break;} }
      core->setKeys(core,k); for(int i=0;i<(n?n:4);i++) core->runFrame(core); core->setKeys(core,0); for(int i=0;i<4;i++) core->runFrame(core); }
    else if(cmd[0]=='K'){ held=0; for(char*p=a;*p;p++){ switch(*p){case 'A':held|=1;break;case 'B':held|=2;break;case 'S':held|=8;break;case 'R':held|=16;break;case 'L':held|=32;break;case 'U':held|=64;break;case 'D':held|=128;break;} } }
    else if(cmd[0]=='h'){ int f=atoi(a); char out[512]; sscanf(line,"%*s %*s %511s",out); FILE* o=fopen(out,"a"); core->setKeys(core,held);
      for(int i=0;i<f;i++){ core->runFrame(core); uint32_t hsh=2166136261u; for(unsigned j=0;j<w*h;j++){ hsh=(hsh^buf[j])*16777619u; } fprintf(o,"%08x\n",hsh); }
      fclose(o); }
    else if(cmd[0]=='p'){ FILE* f=fopen(a,"wb"); fprintf(f,"P6 %u %u 255\n",w,h);
      for(unsigned i=0;i<w*h;i++){ uint32_t px=buf[i]; unsigned char rgb[3]={px&0xff,(px>>8)&0xff,(px>>16)&0xff}; fwrite(rgb,1,3,f);} fclose(f); }
  }
  core->deinit(core); return 0;
}
