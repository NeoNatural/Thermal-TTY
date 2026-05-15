#include "HardwareConfig.h"

void InitSerialPort()
{
  serial_port.begin(115200);
  while(!serial_port);
}
