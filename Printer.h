
#include <Arduino_FreeRTOS.h>
//#include <avr/pgmspace.h>


#define RESET_EVENT (32)
#define WITHDRAW_ONE_EVENT (16)
#define NL_EVENT (8)
#define EXTEND_EVENT (4)
#define PRINT_EVENT (2)
#define WITHDRAW_EVENT (1)

#define CHARS_PER_LINE (64)
#define HBITS_PER_CHAR (6)
#define VBITS_PER_CHAR (8)
#define DOTS_PER_LINE (384)
#define BufByteNum (DOTS_PER_LINE/8)

typedef struct
{
  uint8_t dotbuf [VBITS_PER_CHAR][BufByteNum];
  uint8_t showmask [BufByteNum];
  uint8_t* boldmask;
  uint8_t* undlinemask;
  /*
    uint8_t l_uint8_t;
    uint8_t r_uint8_t;
    uint8_t l_bit;
    uint8_t r_bit;*/
} DotBuffer;


extern DotBuffer *fgBuf ,*bgBuf;
extern volatile bool printer_busy;
/*
  uint8_t dotbuf_fg[VBITS_PER_CHAR][Bufuint8_tNum];
  uint8_t dotbuf_bg[VBITS_PER_CHAR][Bufuint8_tNum];

  uint8_t boldmask_fg[Bufuint8_tNum+1];
  uint8_t boldmask_bg[Bufuint8_tNum+1];

  uint8_t undlinemask_fg[Bufuint8_tNum+1];
  uint8_t undlinemask_bg[Bufuint8_tNum+1];
*/
void taskPrinterCtrl(void *pvParameters);
void initDotBuf();
