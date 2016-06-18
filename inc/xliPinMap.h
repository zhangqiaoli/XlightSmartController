//  xliPinMap.h - Xlight include file: MCU Pin Map
/// SBS updated 20150429 in accordance with EF H/W design

#ifndef xliPinMap_h
#define xliPinMap_h

// MCU Type: Particle family, and Photon or P1
#define MCU_TYPE_Particle
#ifdef MCU_TYPE_Particle
#define MCU_TYPE_P1
#endif

//------------------------------------------------------------------
// MCU Pin Usage
//------------------------------------------------------------------
// System Control
#ifdef MCU_TYPE_P1
    #define PIN_BTN_SETUP         46          // Setup button
    #define PIN_BTN_RESET         34          // Reset button
    #define PIN_LED_RED           29          // RGBR, Status RGB LED
    #define PIN_LED_GREEN         32          // RGBG, Status RGB LED
    #define PIN_LED_BLUE          31          // RGBB, Status RGB LED
#endif

// Digital only GPIO pins (D0 - D7, D0-D4 may also be PWM)
#define PIN_LED_LEVEL_B0          D0          // Brightness indicator bit0, may also be PWM
#define PIN_LED_LEVEL_B1          D1          // Brightness indicator bit1, may also be PWM
#define PIN_LED_LEVEL_B2          D2          // Brightness indicator bit2, may also be PWM

#define PIN_EXT_SPI_MISO          D3          // SPI3MISO, may also be PWM
#define PIN_EXT_SPI_SCK           D4          // SPI3SCK
#define PIN_RF_CHIPSELECT         D5          // LT8910 chip select - output
#define PIN_RF_RESET              D6          // LT8910 Reset - output
#define PIN_RF_EOFFLAG            D7          // LT8910 package completion interupt flag - input

// Analog GPIO pins (12-bit A0 - A7), can also be used as digital GPIOs
#define PIN_BTN_DOWN              A0          // Panel button - down
#define PIN_BTN_OK                A1          // Panel button - OK
#define PIN_BTN_UP                A2          // Panel button - up
#define PIN_SEN_DHT               A3          // Sensor: temperature and humidity, DAC2
#define PIN_SEN_PIR               A4          // Sensor: infra red motion, may also be PWM
#define PIN_SEN_LIGHT             A5          // Sensor: ALS, may also be PWM
#define PIN_SEN_MIC               A6          // Sensor: ECT MIC, DAC
#define PIN_ANA_WKP               A7          // J4-1 for testing, may also be PWM


// P1 specific GPIO pins, can also be used as digital GPIOs
/*
#define PIN_ANA_SETUP             P1S0        // 12b GPIO, may also be PWM
#define PIN_ANA_SETUP             P1S1        // 12b GPIO, may also be PWM
#define PIN_ANA_SETUP             P1S2        // 12b GPIO
#define PIN_ANA_SETUP             P1S3        // 12b GPIO
#define PIN_BTN_SETUP             P1S4        // Setup button
#define PIN_BTN_RESET             P1S5        // 12b GPIO, Reset button
*/

// On-board Special pins
/*
#define PIN_OBD_RST               RST         // Active-low reset input
#define PIN_OBD_VBAT              VBAT        // Internal VBAT
#define PIN_OBD_3V3               3V3         // 3.3V DC
*/
#define PIN_BLE_RX                RX          // BLE RX, may also be PWM
#define PIN_BLE_TX                TX          // BLE TX, may also be PWM

//------------------------------------------------------------------
// Sensor Features
//------------------------------------------------------------------
// Uncomment whatever type you're using!
#define SEN_TYPE_DHT              DHT11		   // DHT 11
//#define SEN_TYPE_DHT              DHT22		   // DHT 22 (AM2302)
//#define SEN_TYPE_DHT              DHT21		   // DHT 21 (AM2301)

// Light sensor output scale: y = x / 3.3 * 4096
#define SEN_LIGHT_MIN             0		       // 0 volt
#define SEN_LIGHT_MAX             3720       // 3 volt -> (3 / 3.3 * 4096)

#endif /* xliPinMap_h */
