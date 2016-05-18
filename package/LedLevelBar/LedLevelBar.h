//  LedLevelBar.h - Xlight LED level bar (brightness indicator) lib

#ifndef LedLevelBar_h
#define LedLevelBar_h

#include "application.h"

#define LEDLEVELBAR_MAXPINS     3

// Types of LED level bar
typedef enum
{
  ledLBarProgress = 0,    // Progress bar (accumulated)
  ledLBarSpot             // Spot
} ledLBar_t;

// Address decoder class, e.g. LS138
class AddressDecoder {
  protected:
    uint8_t _pins;
    uint8_t _pin[LEDLEVELBAR_MAXPINS];
    uint8_t _addressSpace;

  public:
    AddressDecoder(uint8_t pins = 3, uint8_t address = 0);
    bool configPin(uint8_t id, uint8_t pin);
    uint8_t getTotalPins();
    uint8_t getTotalAddress();
    bool setAddress(uint8_t address);
};

class LedLevelBar : public AddressDecoder
{
	private:
    ledLBar_t _type;
    uint8_t _level;     // 0 based:from 0 to (TotalLevels -1)

	public:
    LedLevelBar(ledLBar_t type = ledLBarProgress, uint8_t pins = 3, uint8_t levels = 0);
    ledLBar_t getType();
    uint8_t getLevel();
    void setLevel(uint8_t lv);
    void refreshLevelBar();
};

#endif /* LedLevelBar_h */
