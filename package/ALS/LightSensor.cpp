/**
 * LightSensor.cpp - Xlight light sensor lib
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
 * DESCRIPTION
 * 1. support 3-Pin light sensor (photoresistor)
 *
 * ToDo:
 * 1.
**/

#include "LightSensor.h"

LightSensor::LightSensor(uint8_t pin, uint8_t type)
{
	_pin = pin;
	_type = type;
  firstreading = true;
}

// lval & uval: map voltages [0, 3.3] into integer values [0, 4095].
/// Therefore, need to check the light sensor output voltage scale.
void LightSensor::begin(uint16_t lval, uint16_t uval, uint16_t minlevel, uint16_t maxlevel)
{
	pinMode(_pin, INPUT);
	_lastreadtime = 0;
  _lastValue = 0;
  _lowVal = lval;
  _upVal = uval;
  _minLevel = minlevel;
  _maxLevel = maxlevel;
}

uint16_t LightSensor::getLevel()
{
  uint16_t val = read();
  uint16_t level = map(val, _lowVal, _upVal, _minLevel, _maxLevel);
  return level;
}

uint16_t LightSensor::read()
{
  unsigned long currenttime;

  currenttime = millis();
	if( currenttime < _lastreadtime )
  {
		_lastreadtime = 0;
	}
	if( firstreading || ((currenttime - _lastreadtime) >= 1000) )
  {
    _lastValue = analogRead(_pin);
    firstreading = false;
    _lastreadtime = millis();
  }

  return _lastValue;
}
