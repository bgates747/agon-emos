/* Keep sequential reports after old campaigns are archived off-card. */
static unsigned first_report(void){
 uint8_t f=mos_fopen("next.txt",FA_READ);if(!f)return 1;
 char text[8];unsigned n=mos_fread(f,text,sizeof(text));mos_fclose(f);
 unsigned value=0,i=0;
 while(i<n&&text[i]>='0'&&text[i]<='9'){
  value=value*10+(unsigned)(text[i++]-'0');if(value>10000)return 0;
 }
 if(!i||!value)return 0;
 while(i<n&&(text[i]=='\r'||text[i]=='\n'))i++;
 return i==n&&n<sizeof(text)?value:0;
}
static int save_next_report(unsigned next){
 char text[12];int n=snprintf(text,sizeof(text),"%u\r\n",next);
 uint8_t f=mos_fopen("next.txt",FA_WRITE|FA_CREATE_ALWAYS);if(!f)return 0;
 unsigned got=mos_fwrite(f,text,(unsigned)n);mos_fclose(f);return got==(unsigned)n;
}
