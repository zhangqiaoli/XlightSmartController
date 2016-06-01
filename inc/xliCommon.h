//  xlCommon.h - Xlight common definitions header

#ifndef xliCommon_h
#define xliCommon_h

#include "application.h"

#define BITTEST(var,pos)          (((var)>>(pos)) & 0x0001)
#define BITMASK(pos)              (0x0001 << (pos))
#define BITSET(var,pos)           ((var) | BITMASK(pos))
#define BITUNSET(var,pos)         ((var) & (~BITMASK(pos)))

//Here, common data types have been given alternative names through #define statements

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

// Sensor Read Speed (in ticks)
#define SEN_DHT_SPEED_LOW         30
#define SEN_DHT_SPEED_NORMAL      5
#define SEN_DHT_SPEED_HIGH        2

#define SEN_ALS_SPEED_LOW         10
#define SEN_ALS_SPEED_NORMAL      2
#define SEN_ALS_SPEED_HIGH        1

#define SEN_PIR_SPEED_LOW         4
#define SEN_PIR_SPEED_NORMAL      2
#define SEN_PIR_SPEED_HIGH        1

// Maximun number of device associated to one controller
#define MAX_DEVICE_PER_CONTROLLER 16

// Maximun JSON data length
#define COMMAND_JSON_SIZE         64
#define SENSORDATA_JSON_SIZE      196

// Sensor list: maximun 16 sensors
typedef enum
{
  sensorDHT = 0,
  sensorALS,
  sensorMIC,
  sensorVIBRATION,
  sensorPIR,
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
