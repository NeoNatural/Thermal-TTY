#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#include "HardwareConfig.h"
#include <hidboot.h>
#include <usbhub.h>

extern USB Usb;

extern SemaphoreHandle_t serial_lock;

void taskReadKeyboard(void *pvParameters);

inline void MutexSerialSend(uint8_t t)
{
  if ( xSemaphoreTake( serial_lock, portMAX_DELAY ) == pdTRUE )
  {
    serial_port.write(t);
    xSemaphoreGive( serial_lock );
  }
}

inline void MutexSerialSend(__FlashStringHelper* head)
{
  if ( xSemaphoreTake( serial_lock, portMAX_DELAY ) == pdTRUE )
  {
    serial_port.print(head);
    xSemaphoreGive( serial_lock );
  }
}

class KbdRptParser : public KeyboardReportParser
{
    void OnKeyDown(uint8_t mod, uint8_t key);
};
