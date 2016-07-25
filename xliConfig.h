//  xliConfig.h - Xlight SmartController configuration header

#ifndef xliConfig_h
#define xliConfig_h

/*** USER DEFINES:  ***/
#define FAILURE_HANDLING
//#define SERIAL_DEBUG

/**********************/

#define PRIPSTR "%s"
#define pgm_read_word(p) (*(p))
#ifdef SERIAL_DEBUG
  #define IF_SERIAL_DEBUG(x) ({x;})
#else
  #define IF_SERIAL_DEBUG(x)
#endif

// Xlight Application Identification
#define XLA_ORGANIZATION          "xlight.ca"               // Default value. Read from EEPROM
#define XLA_PRODUCT_NAME          "XController"             // Default value. Read from EEPROM
#define XLA_AUTHORIZATION         "use-token-auth"
#define XLA_TOKEN                 "your-access-token"       // Can update online

//------------------------------------------------------------------
// System level working constants
//------------------------------------------------------------------
// Running Time Environment Parameters
#define RTE_DELAY_PUBLISH         500
#define RTE_DELAY_SYSTIMER        50          // System Timer interval, can be very fast, e.g. 50 means 25ms
#define RTE_DELAY_SELFCHECK       1000        // Self-check interval

// Number of ticks on System Timer
#define RTE_TICK_FASTPROCESS			1						// Pace of execution of FastProcess

// Keep this number of rows in all tables after writing to Flash
#define POST_FLASH_TABLE_SIZE       4

// Maximun number of rows for any working memory table implimented using ChainClass
#define PRE_FLASH_MAX_TABLE_SIZE    8

// Change it only if Config_t structure is updated
#define VERSION_CONFIG_DATA         1

// Maximun number of device associated to one controller
#define MAX_DEVICE_PER_CONTROLLER   16

// Maximun JSON data length
#define COMMAND_JSON_SIZE           64
#define SENSORDATA_JSON_SIZE        196

#endif /* xliConfig_h */
