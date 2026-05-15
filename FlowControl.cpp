#include "FlowControl.h"
#include "HardwareConfig.h"
#include "Keyboard.h"

void taskFlowControl(void *pvParameters)
{
  (void) pvParameters;
  bool busy = false;
  for (;;) {
    vTaskDelay(6);
    //*
    if (!busy && serial_port.available() >= SERIAL_RX_BUFFER_SIZE - 30 )
    {
      //Serial.println(F("XOFF"));
      MutexSerialSend('\x13');//DC3 XOFF
      
      busy = true;
      //continue;
    }
    if (busy && serial_port.available() < 16)
    {
      //Serial.println(F("XON"));
      MutexSerialSend('\x11');//DC1 XON
      
      busy = false;
    }
    //*/
    /*
    if (serial_port.available() >= SERIAL_RX_BUFFER_SIZE - 16 )
    {
      MutexSerialSend('\x13');//DC3 XOFF
      busy = true;
    }
    if (serial_port.available() < 36)
    {
      MutexSerialSend('\x11');//DC1 XON
      busy = false;
    }    
    */
  }
}
