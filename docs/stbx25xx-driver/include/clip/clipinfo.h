#ifndef PALLAS_CLIPINFO_H
#define PALLAS_CLIPINFO_H

typedef struct tagBufInfo
{
  unsigned long ulStartAdrOff;     //start address offset
  unsigned long ulLen;             //buffer length
}BUFINFO;

typedef struct tagClipInfo
{ 
  unsigned long ulBufAdrOff;      //buffer segment start address offset,  user never change it
  unsigned long ulBufLen;         //buffer segment length, user never change it
  unsigned long uClipAdrOff;      //clip start address offset; must > ulBufAdrOff
  unsigned long uClipLen;         //clip length; uClipAdrOff + uClipLen < ulBufAdrOff + ulBufLen
  unsigned int  uFlag;            //1: end of stream flag
  void    *pOtherInfo;            //pointer to other information
}CLIPINFO;

#endif
