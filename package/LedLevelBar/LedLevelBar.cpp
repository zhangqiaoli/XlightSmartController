/**
 * LedLevelBar.cpp - Xlight LED level bar (brightness indicator) lib
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
 * DEPENDENCY
 * 1. IntervalTimer
 *
 * REVISION HISTORY
 * Version 1.0 - Created by Baoshi Sun <bs.sun@datatellit.com>
 *
 * DESCRIPTION
 * This lib can be used to control a group of LED beads as a level bar,
 * e.g. a brightness indicator. In general, the input is a value of level, and
 * the same number of LED beads will be turned on. For example, given
 * the level of 5, 5 LED beads will be on.
 * The number of pin is determined by the scale of level or the number of LEDs.
 * For instance, if we have 7 LEDs (or level 0 to level 7), 3 pins will be required.
 * In order to control a larger number of LEDs by using smaller number of pins,
 * address decoder, such as LS138, should be utilized.
 *
 * ToDo:
 * 1.
**/

#include "LedLevelBar.h"
#include "xliCommon.h"

// Address decoder class, e.g. LS138
AddressDecoder::AddressDecoder(uint8_t pins, uint8_t address)
{
  _pins = (pins > LEDLEVELBAR_MAXPINS ? LEDLEVELBAR_MAXPINS: pins);
  uint8_t maxAddress = 2 ^ _pins;
  if( address == 0 ) {
    _addressSpace = maxAddress;
  } else {
    _addressSpace = (address > maxAddress ? maxAddress : address);
  }
}

bool AddressDecoder::configPin(uint8_t id, uint8_t pin)
{
  if(id < _pins) {
    _pin[id] = pin;
    return true;
  }

  return false;
}

uint8_t AddressDecoder::getTotalPins()
{
  return _pins;
}

uint8_t AddressDecoder::getTotalAddress()
{
  return _addressSpace;
}

bool AddressDecoder::setAddress(uint8_t address)
{
  if( address < _addressSpace ) {
    for(uint8_t i = 0; i < _pins; i++ ) {
      if( BITTEST(address, i) ) {
        digitalWrite(_pin[i], HIGH);
      } else {
        digitalWrite(_pin[i], LOW);
      }
    }
    return true;
  }

  return false;
}

// LED level bar (brightness indicator) Class
LedLevelBar::LedLevelBar(ledLBar_t type, uint8_t pins, uint8_t levels)
 : AddressDecoder(pins, levels)
{
  _type = type;
  _level = 0;
}

ledLBar_t LedLevelBar::getType()
{
  return _type;
}

uint8_t LedLevelBar::getLevel()
{
  return _level;
}

void LedLevelBar::setLevel(uint8_t lv)
{
  if( _level != lv ) {
    if( setAddress(lv) )
      _level = lv;
  }
}

void LedLevelBar::refreshLevelBar()
{
  // Only refresh progress bar
  if( getType() != ledLBarProgress )
    return;

  static uint8_t pos = 0;
  if( pos++ <= getLevel() ) {
    setAddress(pos);
  }
  pos %= getTotalAddress();
}
