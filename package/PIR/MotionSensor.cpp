/**
* MotionSensor.cpp - Xlight motion sensor lib
*
* Created by Umar Bhutta <umar.bh@datatellit.com>
* Copyright (C) 2015-2016 DTIT
* Full contributor list:
*   Baoshi Sun <bs.sun@datatellit.com>
*   Umar Bhutta <umar.bh@datatellit.com>
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
* Version 1.0 - Created by Umar Bhutta <umar.bh@datatellit.com>
*
* DESCRIPTION
* 1. support PIR motion sensor (3 pin)
* 2. use moving avarage algorithm to calculate result
*
**/

#include "MotionSensor.h"

MotionSensor::MotionSensor(uint8_t pin)
{
	_pin = pin;
	clearBuf();
}

void MotionSensor::clearBuf()
{
	_index = 0;
	_sum = 0;
	memset(_buf, 0x00, sizeof(_buf));
}

void MotionSensor::begin()
{
	pinMode(_pin, INPUT);
	clearBuf();
}

bool MotionSensor::getMotion()
{
	read();
	// Pool for result
	return(_sum > MOTION_BUF_SIZE / 2);
}

bool MotionSensor::read()
{
	bool pirState = (digitalRead(_pin) == HIGH);
	// Replace the buffer tail element and get moving avarage
	_sum =  _sum - _buf[_index] + pirState;
	_buf[_index++] = pirState;
	_index %= MOTION_BUF_SIZE;

	return pirState;
}
