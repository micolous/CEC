#ifndef CEC_DEVICE_H__
#define CEC_DEVICE_H__

#include "CEC.h"

class CEC_Device : public CEC_LogicalDevice
{
public:
  CEC_Device(int physicalAddress, int cec_gpio_addr, int cec_gpio_pin);
  
  void Initialize(CEC_DEVICE_TYPE type);
  virtual void Run();
  
protected:
  ~CEC_Device();
  virtual bool LineState();
  virtual void SetLineState(bool);
  virtual void SignalIRQ();
  virtual bool IsISRTriggered();
  virtual bool IsISRTriggered2() { return _isrTriggered; }

  virtual void OnReady();
  virtual void OnReceive(int source, int dest, unsigned char* buffer, int count);
  
private:
  bool _isrTriggered;
  bool _lastLineState2;
  int  _cec_gpio_addr, _cec_gpio_pin;
};

#endif // CEC_DEVICE_H__

