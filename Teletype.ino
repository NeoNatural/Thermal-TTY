#define SERIAL_RX_BUFFER_SIZE 92 //重定义
#define SERIAL_TX_BUFFER_SIZE 32 //重定义

#include "HardwareConfig.h"
#include "Printer.h"
#include "CodeParse.h"
#include "Keyboard.h"
#include "FlowControl.h"

TaskHandle_t taskReadKeyboardHandler;
TaskHandle_t taskPrinterCtrlHandler;
TaskHandle_t taskCodeParseHandler;
TaskHandle_t taskFlowControlHandler;

void setup() {

  // put your setup code here, to run once:
  InitSerialPort();
  Usb.Init();
  //vTaskDelay(10000);
  xTaskCreate(taskReadKeyboard, // Task function
              "KeyRd", // Task name
              512, // Stack size
              NULL,
              configMAX_PRIORITIES-3, // Priority
              &taskReadKeyboardHandler ); // TaskHandle

  xTaskCreate(taskPrinterCtrl, // Task function
              "PrtCtrl", // Task name
              128, // Stack size
              NULL,
              configMAX_PRIORITIES-1, // Priority
              &taskPrinterCtrlHandler ); // TaskHandle
              
  xTaskCreate(taskParseVT100Code, // Task function
              "CodePar", // Task name
              256, // Stack size
              NULL,
              configMAX_PRIORITIES-4, // Priority
              &taskCodeParseHandler ); // TaskHandle
   //*           
  xTaskCreate(taskFlowControl, // Task function
              "FlowCtl", // Task name
              128, // Stack size
              NULL,
              configMAX_PRIORITIES-1, // Priority
              &taskFlowControlHandler ); // TaskHandle
              //*/
}

void loop() {
  // put your main code here, to run repeatedly:

}
