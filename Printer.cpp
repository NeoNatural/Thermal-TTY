#include<Arduino.h>
#include "Printer.h"
#include <timers.h>

#define DIR (5)
#define EN (34)
#define LAT_ (32)
#define STR0 (30)
#define STR1 (28)
#define CLK (26)
#define DAT (24)
#define STEP (22)

#define MAX_HEAT_DOT (192)
#define REPLACE_TIME (7000)  //试出来的。不能大不能小?
#define ROLL_TIME (10)

#define INVERSE_TIME (400)
#define EXTEND_BIAS (0)

#define STEP_PULSE_TIME (50)

extern TaskHandle_t taskCodeParseHandler;
extern TimerHandle_t idleTimer;

volatile uint16_t HEAT_TIME = 2500;
volatile bool printer_busy = false;
//uint8_t print_cursor_r = 255;

DotBuffer  buf1, buf2;

DotBuffer *fgBuf = &buf1, *bgBuf = &buf2;

void initDotBuf()
{
  fgBuf->boldmask = NULL;
  fgBuf->undlinemask = NULL;
  bgBuf->boldmask = NULL;
  bgBuf->undlinemask = NULL;
}

TickType_t xLastWakeTime_printer;

void init_roll()
{
  digitalWrite(DIR, HIGH);
  xLastWakeTime_printer = xTaskGetTickCount ();
  for (int l = 0; l < 40; l++)
  {
    xTaskDelayUntil(&xLastWakeTime_printer, ROLL_TIME);
    digitalWrite(STEP, LOW);
    delayMicroseconds(STEP_PULSE_TIME);
    digitalWrite(STEP, HIGH);
  }
  xTaskDelayUntil(&xLastWakeTime_printer, INVERSE_TIME);
  digitalWrite(DIR, LOW);
  for (int l = 0; l < 40; l++)
  {
    xTaskDelayUntil(&xLastWakeTime_printer, ROLL_TIME);
    digitalWrite(STEP, LOW);
    delayMicroseconds(STEP_PULSE_TIME);
    digitalWrite(STEP, HIGH);
  }
  //delay(INVERSE_TIME);
  xTaskDelayUntil(&xLastWakeTime_printer, INVERSE_TIME);
}

uint8_t extend_correct = 0;

void extend(uint16_t n)
{
  //digitalWrite(DIR, LOW);
  /*
    uint8_t cor_num = 0;
    if(extend_correct>=3){
    cor_num = EXTEND_BIAS;
    extend_correct = 0;
    }
    else extend_correct++;
  */
  xLastWakeTime_printer = xTaskGetTickCount ();
  for (long l = 0; l < 4 * (VBITS_PER_CHAR + 1)*n; l++)
  {
    xTaskDelayUntil(&xLastWakeTime_printer, ROLL_TIME);
    digitalWrite(STEP, LOW);
    delayMicroseconds(STEP_PULSE_TIME);
    digitalWrite(STEP, HIGH);
  }
  xTaskDelayUntil(&xLastWakeTime_printer, INVERSE_TIME);
}

uint8_t withdraw_correct = 0;
int cnt = 2;
void withdraw(uint16_t n)
{
  uint8_t cor_num = EXTEND_BIAS + 1;
  if (withdraw_correct >= 2) {
    cor_num = EXTEND_BIAS;
    withdraw_correct = 0;

  }
  else withdraw_correct++;
  //Serial.println(cor_num);
  digitalWrite(DIR, HIGH);
  xLastWakeTime_printer = xTaskGetTickCount ();
  for (long l = 0; l < 4 * (VBITS_PER_CHAR + 1)*n + 40 + cor_num; l++)
  {
    xTaskDelayUntil(&xLastWakeTime_printer, ROLL_TIME);
    digitalWrite(STEP, LOW);
    delayMicroseconds(STEP_PULSE_TIME);
    digitalWrite(STEP, HIGH);
  }
  xTaskDelayUntil(&xLastWakeTime_printer, INVERSE_TIME);
  digitalWrite(DIR, LOW);
  for (int l = 0; l < 40; l++)
  {
    xTaskDelayUntil(&xLastWakeTime_printer, ROLL_TIME);
    digitalWrite(STEP, LOW);
    delayMicroseconds(STEP_PULSE_TIME);
    digitalWrite(STEP, HIGH);
  }
  xTaskDelayUntil(&xLastWakeTime_printer, INVERSE_TIME);
}

uint8_t bits_number(uint8_t n)//count bits "1"
{
  uint8_t count = 0;
  while (n) {
    count += n & 1;
    n >>= 1;
  }
  return count;
}

void PrintLine_split(DotBuffer& buf, uint8_t row)
{
  uint8_t probe;
  uint8_t* Array = buf.dotbuf[row];
  /*
    uint8_t l_uint8_t = buf.l_uint8_t;
    uint8_t r_uint8_t = buf.r_uint8_t;
    uint8_t l_bit = buf.l_bit;
    uint8_t r_bit = buf.r_bit;*/
  for (uint8_t i = 0; i < BufByteNum; i++)
  {
    if (buf.showmask[i] && Array[i] ) {
      probe = 0x80;
      for (uint8_t j = 0; j < 8; j++)
      {
        if (buf.showmask[i] & Array[i] & probe)
          digitalWrite(DAT, HIGH);
        else
          digitalWrite(DAT, LOW);
        delayMicroseconds(1);
        digitalWrite(CLK, HIGH);
        delayMicroseconds(1);
        digitalWrite(CLK, LOW);
        probe >>= 1;
      }
    } else {
      digitalWrite(DAT, LOW);
      for (uint8_t j = 0; j < 8; j++)
      {
        delayMicroseconds(1);
        digitalWrite(CLK, HIGH);
        delayMicroseconds(1);
        digitalWrite(CLK, LOW);
      }
    }

  }

  //Lock One Bits Line data
  delayMicroseconds(10);
  digitalWrite(LAT_, LOW);
  delayMicroseconds(10);
  digitalWrite(LAT_, HIGH);
  delayMicroseconds(10);
}

inline void heat_once()
{
  digitalWrite(EN, HIGH);
  digitalWrite(STR0, HIGH);
  vTaskDelay(HEAT_TIME);
  digitalWrite(STR0, LOW);
  digitalWrite(STR1, HIGH);
  vTaskDelay(HEAT_TIME);
  digitalWrite(STR1, LOW);
  digitalWrite(EN, LOW);
  vTaskDelay(10);
}

void PrintLine(bool bold = false)
{
  for (int i = 0; i < VBITS_PER_CHAR; i++)
  {
    //uint8_t* _array = dotbuf_fg[i];
    if (bold)
    {
      //*
      HEAT_TIME = 120;
      PrintLine_split(*fgBuf, i);
      for (int l = 0; l < 4; l++)
      {
        heat_once();
        digitalWrite(STEP, LOW);
        delayMicroseconds(STEP_PULSE_TIME);
        digitalWrite(STEP, HIGH);
      }
      //*/
      /*
        for (uint8_t s =0;s< MAX;s++)
        {
        _array[s]=_array[s         ]^0xff;
        }
        if (i==VBITS_PER_CHAR - 1)
        memset(_array,0xff,MAX);
        HEAT_TIME = 3000;
        PrintLine_split(_array);
        digitalWrite(STEP, LOW);
        delayMicroseconds(200);
        digitalWrite(STEP, HIGH);
        delay(4);
        digitalWrite(STEP, LOW);
        delayMicroseconds(200);
        digitalWrite(STEP, HIGH);
        PrintLine_split(_array);
        digitalWrite(STEP, LOW);
        delayMicroseconds(200);
        digitalWrite(STEP, HIGH);
        delay(4);
        digitalWrite(STEP, LOW);
        delayMicroseconds(200);
        digitalWrite(STEP, HIGH);
      */
    }
    else {
      HEAT_TIME = 50;
      //
      PrintLine_split(*fgBuf, i);
      heat_once();
      digitalWrite(STEP, LOW);
      delayMicroseconds(STEP_PULSE_TIME);
      digitalWrite(STEP, HIGH);
      vTaskDelay(ROLL_TIME);
      digitalWrite(STEP, LOW);
      delayMicroseconds(STEP_PULSE_TIME);
      digitalWrite(STEP, HIGH);

      heat_once();
      digitalWrite(STEP, LOW);
      delayMicroseconds(STEP_PULSE_TIME);
      digitalWrite(STEP, HIGH);
      vTaskDelay(ROLL_TIME);
      digitalWrite(STEP, LOW);
      delayMicroseconds(STEP_PULSE_TIME);
      digitalWrite(STEP, HIGH);
    }
  }
  //*
  xLastWakeTime_printer = xTaskGetTickCount ();
  for (int l = 0; l < 4; l++)  //行间距
  {
    xTaskDelayUntil(&xLastWakeTime_printer, ROLL_TIME);
    digitalWrite(STEP, LOW);
    delayMicroseconds(STEP_PULSE_TIME);
    digitalWrite(STEP, HIGH);
  }
  //*/
}


void initPrinter() {
  // put your setup code here, to run once:

  pinMode(EN, OUTPUT);
  pinMode(STEP, OUTPUT);
  pinMode(STR0, OUTPUT);
  pinMode(STR1, OUTPUT);
  pinMode(DAT, OUTPUT);
  pinMode(CLK, OUTPUT);
  pinMode(LAT_, OUTPUT);
  pinMode(DIR, OUTPUT);

  digitalWrite(EN, LOW);
  digitalWrite(STEP, HIGH);
  digitalWrite(STR0, LOW);
  digitalWrite(STR1, LOW);
  digitalWrite(DAT, LOW);
  digitalWrite(CLK, LOW);
  digitalWrite(LAT_, HIGH);
  digitalWrite(DIR, LOW);

  init_roll();
  /*
    PrintStr("!---------@@@@ Orz X86 Printer Test @@@@---------!");
    PrintStr("!---------@@@@ Orz X86 Printer Test @@@@---------!",true);
    PrintStr("For X86FPGA Printer Testing...");
    PrintStr("For X86FPGA Printer Testing...",true);
    PrintStr("Version: 0.02  ,  2021.2 By Orz Studio");   PrintStr(" ");
    PrintStr("  @@@@@@           ");
    PrintStr(" @@@  @@@  @@  @@@ @@@@@@ ");
    PrintStr("@@@    @@  @@@@   @   @@");
    PrintStr("@@    @@@  @@       @@");
    PrintStr("@@@  @@@  @@      @@   @");
    PrintStr(" @@@@@@  @@      @@@@@@  ");
    //*/
  extend(1);
  //memset(tbuf, 0x20, CHARS_PER_LINE);
  //tbuf[0] = 0x40;
}

inline void exchangeDotBuffer()
{
  DotBuffer* temp = fgBuf;
  fgBuf = bgBuf;
  bgBuf = temp;
}

void taskPrinterCtrl(void *pvParameters)
{
  (void) pvParameters;
  initPrinter();

  uint32_t note;
  //xLastWakeTime_printer = xTaskGetTickCount ();
  //xTaskDelayUntil(&xLastWakeTime_printer, 6);
  for (;;) {
    //Serial.println(F("Start Wait Command"));
    xTaskNotifyWait(    0,      /* Don't clear any notification bits on entry. */
                        0xffffffff, /* Reset the notification value to 0 on exit. */
                        &note, /* Notified value pass out in
                                             ulNotifiedValue. */
                        portMAX_DELAY );
    printer_busy = true;
    //Serial.println(F("Get Command"));
    if (note & PRINT_EVENT)
    {
      //Serial.println(F("Ready to exchange buffer"));
      exchangeDotBuffer();
      xTaskNotifyGive(taskCodeParseHandler);
      //Serial.println(F("buffer change notify taken"));
    }
    if (note & WITHDRAW_EVENT)
    {
      //Serial.print(" |Withdraw Two| ");
      //Serial.println(F("Ready to withdraw"));
      withdraw(2);
      //xTaskNotifyGive(taskCodeParseHandler);
      //Serial.println(F("end withdraw notify taken"));
    }
    if (note & WITHDRAW_ONE_EVENT)
    {
      //Serial.print(" |Withdraw One| ");
      //Serial.println(F("Ready to withdraw"));
      withdraw(1);

      //xTaskNotifyGive(taskCodeParseHandler);
      //Serial.println(F("end withdraw notify taken"));
    }
    if (note & PRINT_EVENT)
    {
      PrintLine();
    }
    if (note & NL_EVENT)
    {
      extend(1);
      //print_cursor_l = tbuf_cursor = 0;
    }
    if (note & EXTEND_EVENT)
    {
      extend(1);
    }
    if (note & RESET_EVENT)
    {
      xTimerReset( idleTimer, 0 );
    }
    printer_busy = false;
  }
}
