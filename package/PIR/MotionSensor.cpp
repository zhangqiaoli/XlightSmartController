/**
* MotionSensor.cpp - Xlight motion sensor lib
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
* 1. support PIR motion sensor (3 pin)
*
* ToDo:
* 1.
**/

#include "MotionSensor.h"

MotionSensor::MotionSensor(uint8_t pin, uint8_t type)
{
	_pin = pin;
	_type = type;
	firstreading = true;
}

void MotionSensor::begin()
{
	_lastreadtime = 0;
	_lastValue = 0;
}

uint16_t MotionSensor::getMotion()
{
	bool isMotionDetected = read();
	return isMotionDetected;
}

bool MotionSensor::read()
{
	unsigned long currenttime;

	currenttime = millis();
	if (currenttime < _lastreadtime)
	{
		_lastreadtime = 0;
	}
	if (firstreading || ((currenttime - _lastreadtime) >= 1000))
	{
	_lastValue = analogRead(_pin);
		if (_lastValue == 1) {
			if (pirState == 0) {
				// we have just turned on
				motion = 1;
				pirState = 1;
			}
		}
		else {
			if (pirState == 1) {
				motion = 0;
				pirState = 0;
			}
		}

		firstreading = false;
		_lastreadtime = millis();
	}

	return motion;
}
