#include "Keyboard.h"

USB Usb;
HIDBoot<USB_HID_PROTOCOL_KEYBOARD>    HidKeyboard(&Usb);
KbdRptParser Prs;

SemaphoreHandle_t serial_lock = xSemaphoreCreateMutex();

void taskReadKeyboard(void *pvParameters)
{
  (void) pvParameters;
  TickType_t xLastWakeTime;

  
  HidKeyboard.SetReportParser(0, &Prs);

  xLastWakeTime = xTaskGetTickCount ();
  for (;;) {
    xTaskDelayUntil(&xLastWakeTime, 501);
    Usb.Task();
  }
}

void KbdRptParser::OnKeyDown(uint8_t mod, uint8_t key)
{
  MODIFIERKEYS mod_key;
  *((uint8_t*)&mod_key) = mod;
  uint8_t temp = OemToAscii(mod, key);

  if (mod_key.bmLeftCtrl || mod_key.bmRightCtrl)//按ctrl
  {
    if (temp >= 'a' && temp <= 'z') //字母键
    {
      //MutexSerialSend(temp - 64);//ctrl+shift（caps）+字母（转义和大小写无关）
      MutexSerialSend(char(temp - 96));
      return;
    }
    else if (temp >= 'A' && temp <= 'Z')
    {
      MutexSerialSend(char(temp - 64));
      return;
    }
    else
    {
      switch (temp)
      {
        case 8: //bs
          MutexSerialSend('\x7f');
          return;
        case '@':
          MutexSerialSend('\x00');
          return;
        case '[':
          MutexSerialSend('\x1b');
          return;
        case '\\':
          MutexSerialSend('\x1c');
          return;
        case ']':
          MutexSerialSend('\x1d');
          return;
        case '6':
          MutexSerialSend('\x1e');
          return;
        case '-':
          MutexSerialSend('\x1f');
          return;
        default:
          return;
      }
    }
  }

  switch (key)
  {
    case 40: //enter
      MutexSerialSend('\n');
      break;
    case 41: //esc
      MutexSerialSend('\x1b');
      break;
    case 42: //backspace
      MutexSerialSend('\x7f');
      break;
    case 43: //tab
      MutexSerialSend('\t');
      break;
    default:
      if (temp)
        MutexSerialSend(char(temp));
  }
};
