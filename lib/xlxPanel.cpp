/**
 * xlxPanel.cpp - Xlight SmartController Panel Class
 *
 * Created by Baoshi Sun <bs.sun@datatellit.com>
 * Copyright (C) 2015-2016 DTIT
 * Full contributor list:
 *
 * Documentation:
 * Support Forum:
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 *******************************
 *
 * REVISION HISTORY
 * Version 1.0 - Created by Baoshi Sun <bs.sun@datatellit.com>
 *
 * FEATURE DESCRIPTION
 * 1. Click-rotation knob
 * 2. LED brightness indicator
 * 3. RGB status LED
 *
**/
#include "xliPinMap.h"
#include "xlxPanel.h"
#include "xlxLogger.h"

#define PANEL_NUM_LEDS_RING   12

// the one and only instance of Panel class
xlPanelClass thePanel;

uint16_t CCWTipLight595 [] = {0x0000,0x0001,0x0003,0x0007,0x000f,0x001f,0x003f,0x007f,0x00ff,0x01ff,0x03ff,0x07ff,0x0fff};
uint16_t CWTipLight595 [] = {0x0000,0x0800,0x0c00,0x0e00,0x0f00,0x0f80,0x0fc0,0x0fe0,0x0ff0,0x0ff8,0x0ffc,0x0ffe,0x0fff};

xlPanelClass::xlPanelClass()
{
  m_pEncoder = NULL;
  m_pHC595 = NULL;
  m_nDimmerValue = 0;
}

xlPanelClass::~xlPanelClass()
{
  if( m_pEncoder ) {
    delete m_pEncoder;
    m_pEncoder = NULL;
  }

  if( m_pHC595 ) {
    delete m_pHC595;
    m_pHC595 = NULL;
  }
}

void xlPanelClass::InitPanel()
{
  InitEncoder(PIN_KNOB_A_PHASE, PIN_KNOB_B_PHASE, PIN_KNOB_BUTTON);
  InitHC595(2, PIN_MOSI_LED, PIN_SCK_LED, PIN_LATCH_LED);
}

void xlPanelClass::InitEncoder(uint8_t A, uint8_t B, uint8_t buttonPin)
{
  if( !m_pEncoder ) {
    m_pEncoder = new ClickEncoder(A, B, buttonPin);
  }
}

void xlPanelClass::InitHC595(int numberOfShiftRegisters, int serialDataPin, int clockPin, int latchPin)
{
  if( !m_pHC595 ) {
    // create shift register object (number of shift registers, data pin, clock pin, latch pin)
    m_pHC595 = new ShiftRegister74HC595(numberOfShiftRegisters, serialDataPin, clockPin, latchPin);
  }
}

bool xlPanelClass::EncoderAvailable()
{
  if( !m_pEncoder ) return false;
  m_pEncoder->service();
  return true;
}

bool xlPanelClass::HC595Available()
{
  return(m_pHC595);
}

bool xlPanelClass::ProcessEncoder()
{
  if( !m_pEncoder ) return false;

	// Read dimmer value
	int16_t _dimValue = GetDimmerValue();
	_dimValue += m_pEncoder->getValue();
	SetDimmerValue(_dimValue);

	// Read button input
	ButtonType b = m_pEncoder->getButton();
  if (b != BUTTON_OPEN) {
    SERIAL("Button: ");
    #define VERBOSECASE(label) case label: SERIAL_LN(#label); break;
    switch (b) {
      VERBOSECASE(BUTTON_PRESSED);
      VERBOSECASE(BUTTON_HELD)
      VERBOSECASE(BUTTON_RELEASED)
      VERBOSECASE(BUTTON_CLICKED)
      case BUTTON_DOUBLE_CLICKED:
          SERIAL_LN("ClickEncoder::DoubleClicked");
          m_pEncoder->setAccelerationEnabled(!m_pEncoder->getAccelerationEnabled());
          SERIAL_LN("  Acceleration is %s", m_pEncoder->getAccelerationEnabled() ? "enabled" : "disabled");
        break;
    }
  }

}

int16_t xlPanelClass::GetDimmerValue()
{
	return m_nDimmerValue;
}

void xlPanelClass::SetDimmerValue(int16_t _value)
{
	// Restrict range
  _value = constrain(_value, 0, 100);

	if( m_nDimmerValue != _value ) {
		m_nDimmerValue = _value;
    SetHC595();
		LOGD(LOGTAG_EVENT, "Dimmer changed to %d", _value);
	}
}

uint8_t xlPanelClass::GetButtonStatus()
{
  if( !m_pEncoder ) return 0;
	return (uint8_t)m_pEncoder->getButton();
}

// Change LED ring
bool xlPanelClass::SetHC595()
{
  if( !m_pHC595 ) return false;

  // Change dimmer value to LED ring position
  uint8_t pos = map(m_nDimmerValue, 0, 100, 0, PANEL_NUM_LEDS_RING);
  //uint8_t stateOfPin5 = m_pHC595->get(5);
  //if( stateOfPin5 ) { m_pHC595->setAll((uint8_t *)(CWTipLight595 + pos));} else
  m_pHC595->setAll((uint8_t *)(CCWTipLight595 + pos));
  return true;
}

bool xlPanelClass::CheckLEDRing()
{
  if( !m_pHC595 ) return false;

  m_pHC595->setAllHigh(); // set all pins HIGH
  delay(500);

  m_pHC595->setAllLow();  // set all pins LOW
  delay(500);

  for (int i = 0; i < 8; i++) {
    m_pHC595->set(i, HIGH); // set single pin HIGH
    delay(250);
  }

  return true;
}
