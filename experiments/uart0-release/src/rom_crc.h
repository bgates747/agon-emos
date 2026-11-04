/* A is returned by a tiny assembly read, avoiding a C null-pointer load. */
extern uint8_t rom_byte0(void);
static uint32_t installed_crc(unsigned n){
 static uint32_t table[256];
 for(unsigned i=0;i<256;i++){
  uint32_t c=i;for(unsigned j=0;j<8;j++)c=(c>>1)^((c&1)?0xedb88320UL:0);table[i]=c;
 }
 uint32_t c=0xffffffffUL;c=table[(c^rom_byte0())&255]^(c>>8);
 const volatile uint8_t*p=(const volatile uint8_t*)1;
 for(unsigned i=1;i<n;i++)c=table[(c^*p++)&255]^(c>>8);
 return c^0xffffffffUL;
}
