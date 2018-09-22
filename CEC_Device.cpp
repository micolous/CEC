#include "CEC_Device.h"
#include "Timer.h"
//#include <Arduino.h>
#include <libopencm3/efm32/cmu.h>
#include <libopencm3/efm32/gpio.h>

CEC_Device::CEC_Device(int physicalAddress, int cec_gpio_addr, int cec_gpio_pin)
: CEC_LogicalDevice(physicalAddress)
, _isrTriggered(false)
, _lastLineState2(true)
, _cec_gpio_addr(cec_gpio_addr)
, _cec_gpio_pin(cec_gpio_pin)
{
}

CEC_Device::~CEC_Device() {}

void CEC_Device::Initialize(CEC_DEVICE_TYPE type)
{
   cmu_periph_clock_enable(CMU_GPIO);
   gpio_mode_setup(_cec_gpio_addr, GPIO_MODE_INPUT_PULL_FILTER, _cec_gpio_pin);
   gpio_set(_cec_gpio_addr, _cec_gpio_pin);
/*   

  pinMode(_out_line, OUTPUT);
  pinMode( _in_line,  INPUT);

  digitalWrite(_out_line, LOW);
  delay(200);
*/
  CEC_LogicalDevice::Initialize(type);
}

void CEC_Device::OnReady()
{
  // This is called after the logical address has been
  // allocated
  usb_puts("Device ready\n");
}

void CEC_Device::OnReceive(int source, int dest, unsigned char* buffer, int count)
{
  // This is called when a frame is received.  To transmit
  // a frame call TransmitFrame.  To receive all frames, even
  // those not addressed to this device, set Promiscuous to true.
  usb_puts("Packet received ");
  //usb_putsl(hex(millis()), 2);
  usb_putsl(hex(source), 2);
  usb_puts(" -> ");
  usb_putsl(hex(dest), 2);
  usb_putsl(": ", 2);
  for (int i=0; i < count; i++) {
    usb_putsl(hex(buffer[i]), 2);
    usb_putsl(" ", 1);
  }
  usb_putsl("\n", 1);
  
/*
  DbgPrint("Packet received at %ld: %02d -> %02d: %02X", millis(), source, dest, ((source&0x0f)<<4)|(dest&0x0f));
  for (int i = 0; i < count; i++)
    DbgPrint(":%02X", buffer[i]);
  DbgPrint("\n");
*/
}

bool CEC_Device::LineState()
{
   uint16_t state = gpio_get(_cec_gpio_addr, _cec_gpio_pin);
   return state == 0;
//  int state = digitalRead(_in_line);
//  return state == LOW;
}

void CEC_Device::SetLineState(bool state)
{
   if (state) {
      gpio_mode_setup(_cec_gpio_addr, GPIO_MODE_INPUT_PULL_FILTER, _cec_gpio_pin);
   } else {
      gpio_mode_setup(_cec_gpio_addr, GPIO_MODE_WIRED_OR_PULL_DOWN, _cec_gpio_pin);
   }
   
  //digitalWrite(_out_line, state?LOW:HIGH);
  // give enough time for the line to settle before sampling
  // it
  delayMicroseconds(50);
  _lastLineState2 = LineState();
}

void CEC_Device::SignalIRQ()
{
  // This is called when the line has changed state
  _isrTriggered = true;
}

bool CEC_Device::IsISRTriggered()
{
  if (_isrTriggered)
  {
    _isrTriggered = false;
    return true;
  }
  return false;
}

void CEC_Device::Run()
{
  bool state = LineState();
  if (_lastLineState2 != state)
  {
    _lastLineState2 = state;
    SignalIRQ();
  }
  CEC_LogicalDevice::Run();
}
