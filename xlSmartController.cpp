/**
 * xlSmartController.cpp - contains the major implementation of SmartController.
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
 * 1.
 *
 * ToDo:
**/

#include "xlSmartController.h"
#include "xlxConfig.h"
#include "xlxLogger.h"
#include "xliMemoryMap.h"
#include "xliPinMap.h"
#include "Adafruit_DHT.h"
#include "LightSensor.h"
#include "TimeAlarms.h"

//------------------------------------------------------------------
// Global Data Structures & Variables
//------------------------------------------------------------------
// make an instance for main program
SmartControllerClass theSys = SmartControllerClass();

DHT senDHT(PIN_SEN_DHT, SEN_TYPE_DHT);
LightSensor senLight(PIN_SEN_LIGHT);

//------------------------------------------------------------------
// Smart Controller Class
//------------------------------------------------------------------
SmartControllerClass::SmartControllerClass()
{
  m_Status = STATUS_OFF;
  m_SysID = "";
  m_isRF = false;
  m_isBLE = false;
  m_isLAN = false;
  m_isWAN = false;
}

// Primitive initialization before loading configuration
void SmartControllerClass::Init()
{
  // Get System ID
  m_Status = STATUS_INIT;
  m_SysID = System.deviceID();

  // Initialize Logger: Serial & Flash
  theLog.Init(m_SysID);
  theLog.InitSerial(SERIALPORT_SPEED_DEFAULT);
  theLog.InitFlash(MEM_OFFLINE_DATA_OFFSET, MEM_OFFLINE_DATA_LEN);
}

// Second level initialization after loading configuration
/// check RF2.4 & BLE
void SmartControllerClass::InitRadio()
{
  // Check RF2.4
  CheckRF();
  if( IsRFGood() )
  {
    LOGN(LOGTAG_MSG, "RF2.4 is working.");
    SetStatus(STATUS_BMW);
  }

  // Check BLE
  CheckBLE();
  if( IsBLEGood() )
  {
    LOGN(LOGTAG_MSG, "BLE is working.");
  }
}

// Third level initialization after loading configuration
/// check LAN & WAN
void SmartControllerClass::InitNetwork()
{
  // Check LAN
  CheckNetwork();
  if( IsWANGood() )
  {
    LOGN(LOGTAG_MSG, "WAN is working.");
    SetStatus(STATUS_NWS);

    // Initialize Logger: syslog & cloud log
    // ToDo: substitude network parameters
    //theLog.InitSysLog();
    //theLog.InitCloud();
  }
  else if( m_Status == STATUS_BMW && IsLANGood() )
  {
    LOGN(LOGTAG_MSG, "LAN is working.");
    SetStatus(STATUS_DIS);
  }
}

// Initialize Pins: check the routine with PCB
void SmartControllerClass::InitPins()
{
  // Set Panel pin mode
#ifdef MCU_TYPE_P1
  pinMode(PIN_BTN_SETUP, INPUT);
  pinMode(PIN_BTN_RESET, INPUT);
  pinMode(PIN_LED_RED, OUTPUT);
  pinMode(PIN_LED_GREEN, OUTPUT);
  pinMode(PIN_LED_BLUE, OUTPUT);
#endif
  pinMode(PIN_BTN_UP, INPUT);
  pinMode(PIN_BTN_OK, INPUT);
  pinMode(PIN_BTN_DOWN, INPUT);
  pinMode(PIN_ANA_WKP, INPUT);

  // Set Sensors pin Mode
  pinMode(PIN_SEN_DHT, INPUT);
  pinMode(PIN_SEN_LIGHT, INPUT);
  pinMode(PIN_SEN_MIC, INPUT);
  pinMode(PIN_SEN_PIR, INPUT);

  // Set communication pin mode
  pinMode(PIN_BLE_RX, INPUT);
  pinMode(PIN_BLE_TX, OUTPUT);
  pinMode(PIN_EXT_SPI_MOSI, OUTPUT);
  pinMode(PIN_EXT_SPI_MISO, INPUT);
  pinMode(PIN_EXT_SPI_STCP, INPUT);
  pinMode(PIN_EXT_SPI_CS, OUTPUT);
  pinMode(PIN_RF_CHIPSELECT, OUTPUT);
  pinMode(PIN_RF_RESET, OUTPUT);
  pinMode(PIN_RF_EOFFLAG, INPUT);
}

// Initialize Sensors
void SmartControllerClass::InitSensors()
{
  // DHT
  if( theConfig.IsSensorEnabled(sensorDHT) ) {
    senDHT.begin();
    LOGD(LOGTAG_MSG, "DHT sensor works.");
  }

  // Light
  if( theConfig.IsSensorEnabled(sensorLIGHT) ) {
    senLight.begin(SEN_LIGHT_MIN, SEN_LIGHT_MAX);
    LOGD(LOGTAG_MSG, "Light sensor works.");
  }

  // ToDo:
  //...
}

// Get the controller started
BOOL SmartControllerClass::Start()
{
  // ToDo:

  LOGI(LOGTAG_MSG, "SmartController started.");
  return true;
}

UC SmartControllerClass::GetStatus()
{
  return m_Status;
}

void SmartControllerClass::SetStatus(UC st)
{
  LOGN(LOGTAG_STATUS, "System status changed from %d to %d", m_Status, st);
  if( m_Status != st )
    m_Status = st;
}

BOOL SmartControllerClass::CheckRF()
{
  // ToDo:

  return true;
}

BOOL SmartControllerClass::CheckNetwork()
{
  // ToDo:

  return true;
}

BOOL SmartControllerClass::CheckBLE()
{
  // ToDo:

  return true;
}

BOOL SmartControllerClass::SelfCheck(UL ms)
{
  // Check timers

  // Check all alarms
  Alarm.delay(ms);

  // ToDo:

  return true;
}

BOOL SmartControllerClass::IsRFGood()
{
  return m_isRF;
}

BOOL SmartControllerClass::IsBLEGood()
{
  return m_isBLE;
}

BOOL SmartControllerClass::IsLANGood()
{
  return m_isLAN;
}

BOOL SmartControllerClass::IsWANGood()
{
  return m_isWAN;
}
