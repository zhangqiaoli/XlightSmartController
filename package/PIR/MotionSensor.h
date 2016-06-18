//  MotionSensor.h - Xlight motion sensor lib

#ifndef MotionSensor_h
#define MotionSensor_h

#include "application.h"

class MotionSensor {
private:
	uint8_t _pin;
	uint8_t _type;
	bool firstreading;
	unsigned long _lastreadtime;

	bool pirState = 0;
	uint16_t _lastValue;
	bool _motion = 0;

	bool read();

public:
	MotionSensor(uint8_t pin, uint8_t type = 0);
	void begin();
	bool getMotion();
};

#endif /* MotionSensor_h */
