//  xlCommon.h - Xlight common definitions header

#ifndef xliCommon_h
#define xliCommon_h

#include "application.h"

#define BITTEST(var,pos)          (((var)>>(pos)) & 0x0001)
#define BITMASK(pos)              (0x0001 << (pos))
#define BITSET(var,pos)           ((var) | BITMASK(pos))
#define BITUNSET(var,pos)         ((var) & (~BITMASK(pos)))

// Common Data Type
#define BOOL                      boolean
#define UC                        uint8_t
#define US                        uint16_t
#define UL                        uint32_t
#define BYTE                      int8_t
#define SHORT                     int16_t
#define LONG                      int32_t

// Device Status
#define STATUS_OFF                0x00        // Power Off
#define STATUS_INIT               0x01        // Initializing
#define STATUS_BMW                0x02        // Basic Mode of Working
#define STATUS_DIS                0x03        // WAN Disconnected
#define STATUS_NWS                0x04        // Normal Working Status
#define STATUS_SLP                0x05        // Sleep
#define STATUS_RST                0x06        // Reseting
#define STATUS_ERR                0x07        // Fatal Error

// Serial Port Speed
#define SERIALPORT_SPEED_LOW      9600
#define SERIALPORT_SPEED_MID      19200
#define SERIALPORT_SPEED_HIGH     115200
#define SERIALPORT_SPEED_DEFAULT  SERIALPORT_SPEED_LOW

// Maximun number of device associated to one controller
#define MAX_DEVICE_PER_CONTROLLER 16

// Sensor list: maximun 16 sensors
typedef enum
{
  sensorDHT = 0,
  sensorLIGHT,
  sensorMIC,
  sensorVIBRATION,
  sensorIRED,
  sensorSMOKE,
  sensorGAS,
  sensorDUST,
  sensorLEAK,
  sensorBEAT
} sensors_t;

// Device (lamp) type
typedef enum
{
  devtypUnknown = 0,
  devtypCRing3,     // Color ring
  devtypCRing2,
  devtypCRing1,
  devtypWRing3,     // White ring
  devtypWRing2,
  devtypWRing1,
  devtypDummy
} devicetype_t;

#endif /* xliCommon_h */
