#include "SpkComp.h"

u32 crcTable[256] = {0};

void crcInit(){
  u32 i,j;
  u32 crc = 1;
  for(i = 128; i > 0; i >>= 1){
    if(crc & 1){
      crc >>= 1;
      crc ^= 0xEDB88320;
    }else{
      crc >>= 1;
    }
    for(j = 0; j < 256; j += 2*i){
      crcTable[i+j] = crc ^ crcTable[j];
    }
  }
}

u32 crcNext(u32 crc, u8 data){
  if(crcTable[1] == 0) crcInit();
  return (crc >> 8) ^ crcTable[(crc ^ data) & 0xFF];
}
