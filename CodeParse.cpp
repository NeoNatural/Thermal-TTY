#include "HardwareConfig.h"
#include "Printer.h"
#include "Font8x6.h"
#include <timers.h>
#include "vt100_ctl.h"
#define BEEP_PIN 36

uint8_t tbuf[CHARS_PER_LINE] = {0};
volatile uint8_t tbuf_cursor = 0;
volatile uint8_t print_cursor_l = 0;

void idleTimerCallBack(TimerHandle_t xTimer);
extern TaskHandle_t taskPrinterCtrlHandler;
TimerHandle_t idleTimer = xTimerCreate
                          ( "idleT",
                            7001,
                            pdFALSE,
                            0,
                            idleTimerCallBack );


void insert_tbuf(uint8_t t)
{
  tbuf[tbuf_cursor++] = t;
  //print_cursor_r++;
}

inline void clearbgBuf()
{
  for (int i = 0; i < VBITS_PER_CHAR; i++) memset(bgBuf->dotbuf[i], 0, BufByteNum);
  memset(bgBuf->showmask, 0, BufByteNum);
  if (bgBuf->boldmask)
  {
    free(bgBuf->boldmask);
    bgBuf->boldmask = NULL;
  }
  if (bgBuf->undlinemask)
  {
    free(bgBuf->undlinemask);
    bgBuf->undlinemask = NULL;
  }
}

inline void setbgBufshowmask()
{
  if (! tbuf_cursor > print_cursor_l) 
  {
    memset(bgBuf->showmask, 0, BufByteNum);
    return;
  }
  uint16_t pos = HBITS_PER_CHAR * print_cursor_l;
  uint8_t l_byte = pos / 8;
  uint8_t l_bit = pos % 8;

  pos = HBITS_PER_CHAR * tbuf_cursor - 1;
  uint8_t r_byte = pos / 8;
  uint8_t r_bit = pos % 8;

  memset(bgBuf->showmask, 0, l_byte);
  if (BufByteNum - r_byte - 1 > 0) memset(bgBuf->showmask + r_byte + 1, 0, BufByteNum - r_byte - 1);
  memset(bgBuf->showmask + l_byte, 0xff, r_byte - l_byte + 1);
  bgBuf->showmask[l_byte] &= 0xff >> l_bit;
  bgBuf->showmask[r_byte] &= 0xff  << (7 - r_bit);
}

void char2bgBuf(int cursor_l, int cursor_r)
{
  for (int i = 0; i < VBITS_PER_CHAR; i++)
  {
    //Lines
    uint16_t pos = HBITS_PER_CHAR * print_cursor_l;
    for (int j = cursor_l; j < cursor_r; j++)
      //int j = tbuf_cursor - 1;
    {
      //Bytes
      for (int k = 0; k < HBITS_PER_CHAR; k++)
      {
        uint8_t byte_ind = pos / 8;
        uint8_t bit_ind = pos % 8;
        uint8_t cc = tbuf[j];
        //uint8_t cc=0;
        if (cc > 0x20) cc = cc - 0x20;
        //else cc = 0x7F - 0x20; //DEL ,which is black square in font
        else cc = 0; // space

        unsigned char ti = pgm_read_byte(&(chars_8x6[cc][k])) ;//chars_8x6[cc][k];

        ti <<= i;
        //*
        if (ti & 0x80)
          bgBuf->dotbuf[i][byte_ind] |= 0x80 >> bit_ind;
        else
          bgBuf->dotbuf[i][byte_ind] &= ~(0x80 >> bit_ind);
        pos += 1;
        //*/
      }
      //pos += HBITS_PER_CHAR;
    }
  }
}

volatile bool time_out_event = false;

void taskParseVT100Code(void *pvParameters)
{
  (void) pvParameters;
  initDotBuf();
  char char_from_serial; //接收的字符、
  TickType_t xLastWakeTime;
  bool idle = true, need_print , renew, need_ext, need_cr, nl_protect = false, cursor_hold = false;
  //uint8_t need_nl = 0;
  bool need_nl;
  xLastWakeTime = xTaskGetTickCount ();
  for (;;) {
    //################################Content Control##########################
    renew = false;
    need_nl = false ;
    need_print = false;
    need_ext = false;
    need_cr = false;
    if (serial_port.available()) {
      //Serial.println(F("Start serial Content Read"));
      char_from_serial = serial_port.read();
      Serial.print(char_from_serial);
      if (char_from_serial == '\e') //received VT100 control code
      {
        //Serial.println(F("Received Char is: ESC"));
        //vTaskDelay(23);
        char ctlcode = -1;
        serial_port.readBytes(&ctlcode, 1); //readBytes has a timeout, which can be used to wait for the next byte
        if (ctlcode == '[')
        {
          //Serial.println(F("Ctrl Code"));
          parse_ctl_code();
          //Serial.println(F("Ctrl Code Return"));
        }
        else if (ctlcode == '(' || ctlcode == ')') //set character set commands, not implemented
        {
          serial_port.readBytes(&ctlcode, 1);
        }
        else if (ctlcode == 'D') //index: move cursor down and scroll screen up if cursor at the bottom
        {
          //moveCursorandScroll(1, tft);
        }
        else if (ctlcode == 'M') //revindex: move cursor up and scroll screen down if cursor at the top
        {
          //moveCursorandScroll(-1, tft);
        }
      }
      else if (char_from_serial == 0) // I don't know why the linux machine will send a lot of 0's
      { // when backspace is pressed. Under 115200 baud rate, arduino
        ;                     // will receive more than 40 0's and under 9600 baud rate,
        //Serial.println(F("Received Char is: NULL"));
        //vTaskDelay(23);
      }                             // arduino will receive ~3 0's.
      else if (char_from_serial == 8) //BS, move cursor backwards
      {
        //Serial.println(F("Received Char is: BS"));
        //vTaskDelay(23);
        

        if (tbuf_cursor > 0)
        {
          tbuf_cursor--;
          Serial.print(F("t_cusor-- ="));
          Serial.println(tbuf_cursor);
        }
        if (tbuf_cursor < print_cursor_l) {
          Serial.println(F("tbuf_cursor < print_cursor_l"));
          renew = true;
          print_cursor_l = 0;
          need_nl = true;
          if(tbuf_cursor)
          {
            cursor_hold = true;
          }
          else
          {
            nl_protect = true;
          }
        }
      }
      else if (char_from_serial == 7) //bell
      {
        tone(BEEP_PIN, 784, 100); //G5 note
      }
      else if (char_from_serial == '\x0a') //LF
      {
        //Serial.println(F("Received Char is: LF"));
        //vTaskDelay(23);
        renew = true;
        if (tbuf_cursor) {
          need_nl = true;
          need_cr = true;
        }
        else {
          if (!nl_protect) {
            need_nl = true;
            need_cr = true;
          }
          else {
            nl_protect = false;
          }
        }
      }
      else if (char_from_serial == '\x0d') //CR
      {
        //Serial.println(F("Received Char is: CR"));
        //vTaskDelay(23);
        //tbuf_cursor = print_cursor_l =0;
        //continue;
      }
      else if (char_from_serial == '\x09') //H TAB
      {
        //Serial.println(F("Received Char is: H TAB"));
        //vTaskDelay(23);
        renew = true;
        uint8_t target_pos = tbuf_cursor - tbuf_cursor % 8 + 8 ;

        while (tbuf_cursor < target_pos - 1)
        {
          insert_tbuf('\x20');//space
        }
        insert_tbuf('\x09');//this is a mark for curser jumping while delete;
        //now tbuf_cursor should equals to target_pos
        //we do not need to worry about the curser arriving to end, the State Control will solve it
      }
      else if (char_from_serial == '\x0B') //V TAB
      {
        //Serial.println(F("Received Char is: V TAB"));
        //vTaskDelay(23);
        ;
      }
      else if (char_from_serial <= 31)
      {
        ; // unimplemented control characters
        //Serial.println(F("Received Char is: <=31"));
        //vTaskDelay(23);
      }
      else if (char_from_serial < 0) // UTF-8 characters (unsupported, just ignore)
      {
        //Serial.println(F("Received Char is: UTF-8"));
        //vTaskDelay(23);
        char_from_serial <<= 1; //the first byte of UTF-8 character indicates how many bytes this character has
        while (char_from_serial & 0b10000000) //the number of bits that are 1 in the beginning is the number of bytes.
        {
          while (!serial_port.available());
          serial_port.read();
          char_from_serial <<= 1;
        }
        renew = true;
        insert_tbuf('\x7F');//write 0x7F as black squre
      }
      else// normal characters
      {
        //Serial.print(F("Received Char is: "));
        //Serial.println(char_from_serial);
        //vTaskDelay(23);
        renew = true;
        insert_tbuf(char_from_serial);
      }
      //Serial.println(F("End serial Content Read"));
    }
    //################################State Control##########################
    if (idle)
    {
      if (renew)
      {
        //Serial.println(F("idle and renew"));
        time_out_event = false;
        idle = false;
        uint8_t sig;
        if (need_nl) {
          need_nl = false;
          sig = WITHDRAW_ONE_EVENT;
          cursor_hold = false;
        }
        else {
          sig = WITHDRAW_EVENT;
        }
        sig |= RESET_EVENT;

        while (xTaskNotify( taskPrinterCtrlHandler, sig, eSetValueWithoutOverwrite ) != pdPASS) {
          vTaskDelay(11);
          Serial.println("Stuck in idle withdraw");
        }
        //Serial.println(F("wait for the end of withdraw"));
        //ulTaskNotifyTake(pdTRUE, portMAX_DELAY); //wait for the end of withdraw
        //Serial.println(F("Reach end of withdraw"));
        //xTimerStart( idleTimer, portMAX_DELAY ); //reset was moved to printer.cpp
        if (nl_protect && tbuf_cursor) {
          nl_protect = false;
        }
        if (tbuf_cursor >= CHARS_PER_LINE) {
          //need_nl++;
          need_print = true;
          need_cr = true;
          nl_protect = true;
        }
        sig = 0;
        if (need_print)
        {
          /*
            Serial.print(F("PrintLine! "));
            Serial.print(F("tbuf_cursor= "));
            Serial.print(tbuf_cursor);
            Serial.print(F(" cursor_l= "));
            Serial.println(print_cursor_l);
          */
          sig |= PRINT_EVENT;
          char2bgBuf(print_cursor_l, tbuf_cursor);
          setbgBufshowmask();
          if (!cursor_hold) print_cursor_l = tbuf_cursor;
          else cursor_hold = false;
        }
        if (need_cr)
        {
          print_cursor_l = tbuf_cursor = 0;
        }
        if (sig) {

          while (xTaskNotify( taskPrinterCtrlHandler, sig, eSetValueWithoutOverwrite ) != pdPASS)
          {
            vTaskDelay(11);
            Serial.println("Stuck in idle print");
          }
          if (need_print)
          {
            //Serial.print(F("Wait for exchange buffer"));
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY); //wait for buffer exchange
            //Serial.println(F("Buffer has been exchanged"));
          }
        }
      }  // if renew
      else
      {
        //Serial.println(F("idle and no renew"));
        vTaskDelay(7);
      }
    } //if idle
    else
    {
      if (renew)
      {
        //Serial.println(F("not idle and renew"));
        xTimerReset( idleTimer, 0 );//
        time_out_event = false;
        if (nl_protect && tbuf_cursor) {
          nl_protect = false;
        }
        if (tbuf_cursor >= CHARS_PER_LINE) {
          need_print = true;
          need_cr = true;
          nl_protect = true;
          //need_nl++;
        }
        if (need_nl)
        {
          need_print = true;
          need_nl = false;
        }

      }
      else  //if renew
      {
        if (time_out_event)
        {
          //Serial.println(F("Time out event"));
          time_out_event = false ;
          idle = true;
          need_print = true;
          need_ext = true;
        }
        else
        {
          //Serial.println(F("not idle but no renew"));
          vTaskDelay(7);
        }
      }  // if renew else

      uint8_t sig = 0;
      if (need_print)
      {
        /*
          Serial.print(F("PrintLine! "));
          Serial.print(F("tbuf_cursor= "));
          Serial.print(tbuf_cursor);
          Serial.print(F(" cursor_l= "));
          Serial.println(print_cursor_l);
        */
        //need_nl--;
        sig |= PRINT_EVENT;
        char2bgBuf(print_cursor_l, tbuf_cursor);
        setbgBufshowmask();
        if (!cursor_hold) print_cursor_l = tbuf_cursor;
        else cursor_hold = false;
      }
      if (need_cr)
      {
        print_cursor_l = tbuf_cursor = 0;
      }
      /*
        if (idle && need_nl>0 )
        {
        //sig |= NL_EVENT;
        if (need_nl>0)
        {
          Serial.println(F("Error more than one NL left"));
        }
        //need_nl= 0;
        }
        //*/
      if (need_ext)
      {
        sig |= EXTEND_EVENT;
      }
      if (sig) {

        while (xTaskNotify( taskPrinterCtrlHandler, sig, eSetValueWithoutOverwrite ) != pdPASS)
        {
          vTaskDelay(11);
          Serial.println("Stuck in !idle");
        }
        if (need_print)
        {
          //Serial.print(F("Wait for exchange buffer"));
          ulTaskNotifyTake(pdTRUE, portMAX_DELAY); //wait for buffer exchange
          //Serial.println(F("Buffer has been exchanged"));
        }
      }
    }  //if idle else
  }//for
}

void idleTimerCallBack(TimerHandle_t xTimer)
{
  time_out_event = true;
}
