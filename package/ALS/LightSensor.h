//  LightSensor.h - Xlight light sensor lib

#ifndef LightSensor_h
#define LightSensor_h

#include "application.h"

class LightSensor {
	private:
		uint8_t _pin;
    uint8_t _type;
    boolean firstreading;
		unsigned long _lastreadtime;
    uint16_t _lastValue;
    uint16_t _lowVal;
    uint16_t _upVal;
    uint16_t _minLevel;
    uint16_t _maxLevel;
    uint16_t read();

	public:
    LightSensor(uint8_t pin, uint8_t type = 0);
		void begin(uint16_t lval=0, uint16_t uval=4000, uint16_t minlevel=0, uint16_t maxlevel=100);
    uint16_t getLevel();
};

#endif /* LightSensor_h */
