//  xlxPanel.h - Xlight SmartController Panel Class
#ifndef xlxPanel_h
#define xlxPanel_h

#include "ClickEncoder.h"
#include "ShiftRegister74HC595.h"

class xlPanelClass
{
private:
  ClickEncoder *m_pEncoder;
  ShiftRegister74HC595 *m_pHC595;
  int16_t m_nDimmerValue;

protected:
  bool SetHC595();

public:
  xlPanelClass();
  virtual ~xlPanelClass();
  void InitPanel();
  void InitEncoder(uint8_t A, uint8_t B, uint8_t buttonPin);
  void InitHC595(int numberOfShiftRegisters, int serialDataPin, int clockPin, int latchPin);

  bool EncoderAvailable();
  bool HC595Available();
  bool ProcessEncoder();
  bool CheckLEDRing();

  int16_t GetDimmerValue();
  void SetDimmerValue(int16_t _value);
  uint8_t GetButtonStatus();
};

//------------------------------------------------------------------
// Function & Class Helper
//------------------------------------------------------------------
extern xlPanelClass thePanel;

#endif /* xlxPanel_h */
