//  MotionSensor.h - Xlight motion sensor lib

#ifndef MotionSensor_h
#define MotionSensor_h

#include "application.h"

#define MOTION_BUF_SIZE			5
class MotionSensor {
private:
	uint8_t _pin;
	bool _buf[MOTION_BUF_SIZE];
	uint8_t _index;
	uint8_t _sum;

	bool read();
	void clearBuf();

public:
	MotionSensor(uint8_t pin);
	void begin();
	bool getMotion();
};

#endif /* MotionSensor_h */
