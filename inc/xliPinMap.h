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

    // P1 specific GPIO pins, can also be used as digital GPIOs
    #define PIN_MOSI_LED          A3 //40          // P1S0, 12b GPIO, may also be PWM
    #define PIN_LATCH_LED         A4 //41          // P1S1, 12b GPIO, may also be PWM
    #define PIN_SCK_LED           A5 //42          // P1S2, 12b GPIO
#endif

// Digital only GPIO pins (D0 - D7, D0-D4 may also be PWM)
#define PIN_SEN_DHT               D0          // Sensor: temperature and humidity
#define PIN_KNOB_A_PHASE          D2          // Knob: A_PHASE
#define PIN_KNOB_B_PHASE          D3          // Knob: B_PHASE
#define PIN_KNOB_BUTTON           D4          // Knob: button
#define PIN_SEN_PIR               D7          // Sensor: infra red motion, may also be PWM

// Analog GPIO pins (12-bit A0 - A7), can also be used as digital GPIOs
#define PIN_RF24_CE		   		      A0
#define PIN_RF24_CS		   	 	      A2
#define PIN_SEN_MIC               A6          // Sensor: ECT MIC, DAC
#define PIN_SEN_LIGHT             A7          // Sensor: ALS, may also be PWM

/*
  RF2.4 Module
  SPARK CORE    SHIELD SHIELD    NRF24L01+
  GND           GND              1 (GND)
  3V3 (3.3V)    3.3V             2 (3V3)
  A0 (CSN)      9  (D6)          3 (CE)
  A2 (SS)       10 (SS)          4 (CSN)
  A3 (SCK)      13 (SCK)         5 (SCK)
  A5 (MOSI)     11 (MOSI)        6 (MOSI)
  A4 (MISO)     12 (MISO)        7 (MISO)
  *A1(IRQ)      PC3(IRQ)          8(IRQ)

  NOTE: Also place a 10-100uF cap across the power inputs of
        the NRF24L01+.  I/O o fthe NRF24 is 5V tolerant, but
        do NOT connect more than 3.3V to pin 2(3V3)!!!
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
