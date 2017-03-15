/**
 * xlxBLEInterface.cpp - Xlight Bluetooth interface via Serial Port
 * This module works with Bluetooth module and exchange data via serial port.
 *
 * Created by Baoshi Sun <bs.sun@datatellit.com>
 * Copyright (C) 2015-2016 DTIT
 * Full contributor list:
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
 * Version 1.0 - Created by Baoshi Sun <bs.sun@datatellit.com>
 *
 * DESCRIPTION
 * 1. Tested on HC-06
 * 2.
 *
 * ToDo:
 * 1.
 *
**/

#include "xlxBLEInterface.h"
#include "xlSmartController.h"
#include "xlxConfig.h"

#define BLE_CHIP_HC05             5
#define BLE_CHIP_HC06             6
#define BLE_CHIP_TYPE             BLE_CHIP_HC06

#define BLE_CMD_TIMEOUT           1000

#if BLE_CHIP_TYPE == BLE_CHIP_HC05
#define XLIGHT_BLE_BAUD           "38400,0,0"
#define XLIGHT_BLE_ROLE		        '0'		          // 0=>Slave role; 1=>Master role; 2=>Loopback
#else
#define XLIGHT_BLE_BAUD           '4'             // 1=>1200 baudios, 2=>2400, 3=>4800, 4=>9600 (default), 5=>19200, 6=>38400, 7=>57600, 8=>115200
#define XLIGHT_BLE_ROLE		        'S'		          // S=>Slave role; M=>Master role
#endif

// AT commands for HC-05, ending with \r\n
// Input              Output
// AT                 OK
// AT+RESET           OK
// AT+VERSION?        +VERSION:version
//                    OK
// AT+ADDR?           +ADDR:address (NAP:UAP:LAP)
//                    OK
// AT+NAME?           +NAME:name
//                    OK or FAIL
// AT+NAME=name       OK
// AT+PSWD?           +PSWD:password
//                    OK
// AT+PSWD=password   OK
// AT+UART?           +UART=baud,stopbits,parity
//                    OK
// AT+UART=38400,0,0  OK
// AT+ROLE:           +ROLE:0|1|2
//                    OK
// AT+ROLE=0|1|2      OK
// AT+CLASS?          +CLASS:clas
//                    OK or FAIL
// AT+CLASS=class     OK

// AT commands for HC-06
// Input              Output
// AT                 OK
// AT+BAUD1-8         OK1200, OK2400...
// AT+NAMEname        OKsetname
// AT+PINpin          OKsetPIN
// AT+ROLE=S|M        OK+ROLE:S|M

//------------------------------------------------------------------
// the one and only instance of BLEInterfaceClass
BLEInterfaceClass theBLE;

//------------------------------------------------------------------
// Xlight SerialConsole Class
//------------------------------------------------------------------
BLEInterfaceClass::BLEInterfaceClass()
{
  m_bGood = false;
#if BLE_CHIP_TYPE == BLE_CHIP_HC05
  m_speed = SERIALPORT_SPEED_HC05; // 38400
#else
  m_speed = SERIALPORT_SPEED_LOW; // 9600
#endif

  m_bATMode = false;
  m_lastCmd = "";
}

void BLEInterfaceClass::Init(UC _stpin, UC _enpin)
{
  // Open BLE Port
  m_pin_state = _stpin;
  m_pin_en = _enpin;
#if BLE_CHIP_TYPE == BLE_CHIP_HC05
  pinMode(m_pin_state, INPUT);
  pinMode(m_pin_en, OUTPUT);
#endif
  BLEPort.begin(m_speed);

  // Config BLE module
  m_name = theConfig.GetBLEName();
  m_pin = theConfig.GetBLEPin();
  config();
}

void MyDelay1S()
{
#if BLE_CHIP_TYPE == BLE_CHIP_HC05
  delay(250);
  Particle.process();
#else
  delay(250);
  Particle.process();
  delay(250);
  Particle.process();
  delay(250);
  Particle.process();
  delay(250);
#endif
}

void BLEInterfaceClass::config()
{
  // Module configuration:
  m_bGood = FALSE;
#if BLE_CHIP_TYPE == BLE_CHIP_HC05

  // Starts configuration
  digitalWrite(m_pin_en, HIGH);

  // Starts configuration
  BLEPort.println("AT"); MyDelay1S();

  // Adjusting bluetooth name
  BLEPort.printlnf("AT+NAME=%s", m_name.c_str()); MyDelay1S();

  // Baud rate adjust
  BLEPort.printlnf("AT+UART=%s", XLIGHT_BLE_BAUD); MyDelay1S();

  // Password adjust
  BLEPort.printlnf("AT+PSWD=%s", m_pin.c_str()); MyDelay1S();

  // CoD adjust
  BLEPort.printlnf("AT+CLASS=%x", XLIGHT_BLE_CLASS); MyDelay1S();

  // Role adjust
  //BLEPort.printlnf("AT+ROLE=%c", XLIGHT_BLE_ROLE); MyDelay1S();

  // Stop configuration
  digitalWrite(m_pin_en, LOW);

#else

  // Starts configuration
  BLEPort.print("AT"); MyDelay1S();

  // Adjusting bluetooth name
  BLEPort.print("AT+NAME"); BLEPort.print(m_name); MyDelay1S();

  // Baud rate adjust
  BLEPort.print("AT+BAUD"); BLEPort.print(XLIGHT_BLE_BAUD); MyDelay1S();

  // Password adjust
  BLEPort.print("AT+PIN"); BLEPort.print(m_pin); MyDelay1S();

  // Role adjust
  //BLEPort.print("AT+ROLE"); BLEPort.print(XLIGHT_BLE_ROLE); MyDelay1S();
#endif
}

BOOL BLEInterfaceClass::isGood()
{
  return m_bGood;
}

BOOL BLEInterfaceClass::getState()
{
  return digitalRead(m_pin_state);
}

String BLEInterfaceClass::getName()
{
  return m_name;
}

BOOL BLEInterfaceClass::setName(String _name)
{
  if( _name != m_name) {
    m_name = _name;

    // Module configuration:
#if BLE_CHIP_TYPE == BLE_CHIP_HC05
    // Starts configuration
    digitalWrite(m_pin_en, HIGH);
    // Adjusting bluetooth name
    BLEPort.printlnf("AT+NAME=%s", m_name);
    // Stop configuration
    digitalWrite(m_pin_en, LOW);
#else
    // Adjusting bluetooth name
    BLEPort.print("AT+NAME"); BLEPort.print(m_name);
#endif

    return TRUE;
  }
  return FALSE;
}

String BLEInterfaceClass::getPin()
{
  return m_pin;
}

BOOL BLEInterfaceClass::setPin(String _pin)
{
  if( _pin != m_pin) {
    m_pin = _pin;

    // Module configuration:
#if BLE_CHIP_TYPE == BLE_CHIP_HC05
    // Starts configuration
    digitalWrite(m_pin_en, HIGH);
    // Password adjust
    BLEPort.printlnf("AT+PSWD=%s", m_pin);
    // Stop configuration
    digitalWrite(m_pin_en, LOW);
#else
    // Password adjust
    BLEPort.print("AT+PIN"); BLEPort.print(m_pin);
#endif

    return TRUE;
  }
  return FALSE;
}

UL BLEInterfaceClass::getSpeed()
{
  return m_speed;
}

BOOL BLEInterfaceClass::isATMode()
{
  return m_bATMode;
}

void BLEInterfaceClass::setATMode(BOOL on)
{
#if BLE_CHIP_TYPE == BLE_CHIP_HC05
  digitalWrite(m_pin_state, on ? HIGH : LOW);
#else
  if( on ) sendCommand("AT");    // wait for OK
#endif
  m_bATMode = on;
}

void BLEInterfaceClass::processCommand()
{
  char myChar, preChar = 0;

  while( BLEPort.available() > 0 ) {
    myChar = BLEPort.read();
#ifdef SERIAL_DEBUG
    TheSerial.print(myChar);
#endif    
    if( preChar == 'O' && myChar == 'K') {
      m_bGood = TRUE;
    }
    preChar = myChar;
  }
}

BOOL BLEInterfaceClass::sendCommand(String _cmd)
{
  UC len = _cmd.length();
  if( BLEPort.write((UC *)_cmd.c_str(), len) < len ) return false;
  m_lastCmd = _cmd;
  return true;
}
