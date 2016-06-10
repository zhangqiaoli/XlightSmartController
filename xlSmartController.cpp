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
#include "xliPinMap.h"

#include "Adafruit_DHT.h"
#include "ArduinoJson.h"
#include "LedLevelBar.h"
#include "LightSensor.h"
#include "MotionSensor.h"
#include "TimeAlarms.h"

//------------------------------------------------------------------
// Global Data Structures & Variables
//------------------------------------------------------------------
// make an instance for main program
SmartControllerClass theSys;

DHT senDHT(PIN_SEN_DHT, SEN_TYPE_DHT);
LightSensor senLight(PIN_SEN_LIGHT);

LedLevelBar indicatorBrightness(ledLBarProgress, 3);

MotionSensor senMotion(PIN_SEN_PIR);


//------------------------------------------------------------------
// Smart Controller Class
//------------------------------------------------------------------
SmartControllerClass::SmartControllerClass()
{
  m_isRF = false;
  m_isBLE = false;
  m_isLAN = false;
  m_isWAN = false;
}

// Primitive initialization before loading configuration
void SmartControllerClass::Init()
{
  // Get System ID
  m_SysID = System.deviceID();
  m_devStatus = STATUS_INIT;

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
  // Check WAN and LAN
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
  else if( GetStatus() == STATUS_BMW && IsLANGood() )
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

// Workaround for Paricle Analog Pin mode problem
#ifndef MCU_TYPE_Particle
  pinMode(PIN_BTN_UP, INPUT);
  pinMode(PIN_BTN_OK, INPUT);
  pinMode(PIN_BTN_DOWN, INPUT);
  pinMode(PIN_ANA_WKP, INPUT);

  // Set Sensors pin Mode
  //pinModes are already defined in the ::begin() method of each sensor library, may need to be ommitted from here
  pinMode(PIN_SEN_DHT, INPUT);
  pinMode(PIN_SEN_LIGHT, INPUT);
  pinMode(PIN_SEN_MIC, INPUT);
  pinMode(PIN_SEN_PIR, INPUT);
#endif

  // Brightness level indicator to LS138
  pinMode(PIN_LED_LEVEL_B0, OUTPUT);
  pinMode(PIN_LED_LEVEL_B1, OUTPUT);
  pinMode(PIN_LED_LEVEL_B2, OUTPUT);

  // Set communication pin mode
  pinMode(PIN_BLE_RX, INPUT);
  pinMode(PIN_BLE_TX, OUTPUT);
  pinMode(PIN_EXT_SPI_MISO, INPUT);
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
  if( theConfig.IsSensorEnabled(sensorALS) ) {
    senLight.begin(SEN_LIGHT_MIN, SEN_LIGHT_MAX);
    LOGD(LOGTAG_MSG, "Light sensor works.");
  }


  // Brightness indicator
  indicatorBrightness.configPin(0, PIN_LED_LEVEL_B0);
  indicatorBrightness.configPin(1, PIN_LED_LEVEL_B1);
  indicatorBrightness.configPin(2, PIN_LED_LEVEL_B2);
  indicatorBrightness.setLevel(theConfig.GetBrightIndicator());

  //PIR
  if( theConfig.IsSensorEnabled(sensorPIR) ) {
    senMotion.begin();
    LOGD(LOGTAG_MSG, "Motion sensor works.");
  }


  // ToDo:
  //...
}

void SmartControllerClass::InitCloudObj()
{
  // Set cloud variable initial value
  m_tzString = theConfig.GetTimeZoneJSON();

  CloudObjClass::InitCloudObj();
  LOGN(LOGTAG_MSG, "Cloud Objects registered.");
}

// Get the controller started
BOOL SmartControllerClass::Start()
{
  // ToDo:

  LOGI(LOGTAG_MSG, "SmartController started.");
  return true;
}

String SmartControllerClass::GetSysID()
{
  return m_SysID;
}

UC SmartControllerClass::GetStatus()
{
  return (UC)m_devStatus;
}

void SmartControllerClass::SetStatus(UC st)
{
  LOGN(LOGTAG_STATUS, "System status changed from %d to %d", m_devStatus, st);
  if( (UC)m_devStatus != st )
    m_devStatus = st;
}

BOOL SmartControllerClass::CheckRF()
{
  // ToDo: change value of m_isRF

  return true;
}

BOOL SmartControllerClass::CheckNetwork()
{
  // ToDo: check WAN, change value of m_isWAN

  // ToDo: check LAN, change value of m_isLan

  return true;
}

BOOL SmartControllerClass::CheckBLE()
{
  // ToDo: change value of m_isBLE

  return true;
}

BOOL SmartControllerClass::SelfCheck(UL ms)
{
  UC tickSaveConfig = 0;

  // Check all alarms. This triggers them.
  Alarm.delay(ms);

  // Save config if it was changed
  if( ++tickSaveConfig > 10 ) {
    tickSaveConfig = 0;
    theConfig.SaveConfig();
  }

  // ToDo:
  //...

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

// Process all kinds of commands
void SmartControllerClass::ProcessCommands()
{
  // Check and process RF2.4 messages
  m_cmRF24.CheckMessageBuffer();

  // ToDo: process commands from other sources
}

// Collect data from all enabled sensors
/// use tick to control the collection speed of each sensor,
/// and avoid reading too many data in one loop
void SmartControllerClass::CollectData(UC tick)
{
  BOOL blnReadDHT = false;
  BOOL blnReadALS = false;
  BOOL blnReadPIR = false;

  switch( GetStatus() ) {
    case STATUS_DIS:
    case STATUS_NWS:    // Normal speed
      if( theConfig.IsSensorEnabled(sensorDHT) ) {
        if( tick % SEN_DHT_SPEED_NORMAL == 0 )
          blnReadDHT = true;
      }
      if( theConfig.IsSensorEnabled(sensorALS) ) {
        if( tick % SEN_ALS_SPEED_NORMAL == 0 )
          blnReadALS = true;
      }
      if( theConfig.IsSensorEnabled(sensorPIR) ) {
        if( tick % SEN_PIR_SPEED_NORMAL == 0 )
          blnReadPIR = true;
      }
      break;

    case STATUS_SLP:    // Lower speed in sleep mode
      if( theConfig.IsSensorEnabled(sensorDHT) ) {
        if( tick % SEN_DHT_SPEED_LOW == 0 )
          blnReadDHT = true;
      }
      if( theConfig.IsSensorEnabled(sensorALS) ) {
        if( tick % SEN_ALS_SPEED_LOW == 0 )
          blnReadALS = true;
      }
      if( theConfig.IsSensorEnabled(sensorPIR) ) {
        if( tick % SEN_PIR_SPEED_LOW == 0 )
          blnReadPIR = true;
      }
      break;

    default:
      return;
  }

  // Read from DHT
  if( blnReadDHT ) {
    float t = senDHT.getTempCelcius();
    float h = senDHT.getHumidity();

    if( !isnan(t) ) {
      UpdateTemperature(t);
    }
    if( !isnan(h) ) {
      UpdateHumidity(h);
    }
  }

  // Read from ALS
  if( blnReadALS ) {
    UpdateBrightness(senLight.getLevel());
  }

  // Motion detection
  if( blnReadPIR ) {
	  UpdateMotion(senMotion.getMotion());
  }

  // Update json data and publish on to the cloud
  if( blnReadDHT || blnReadALS || blnReadPIR ) {
    UpdateJSONData();
  }

  // Proximity detection
  // ToDo: Wi-Fi, BLE, etc.
}

//------------------------------------------------------------------
// Device Control Functions
//------------------------------------------------------------------
// Turn the switch of specific device and all devices on or off
/// Input parameters:
///   sw: true = on; false = off
///   dev: device id or 0 (all devices under this controller)
int SmartControllerClass::DevSoftSwitch(BOOL sw, UC dev)
{
  // ToDo:
  //SetStatus();

  return 0;
}

int SmartControllerClass::DevChangeColor()
{
	// ToDo:

	return 0;
}

// High speed system timer process
void SmartControllerClass::FastProcess()
{
  // Refresh LED brightness indicator
  indicatorBrightness.refreshLevelBar();

  // ToDo:
}

//------------------------------------------------------------------
// Cloud interface implementation
//------------------------------------------------------------------
int SmartControllerClass::CldSetTimeZone(String tzStr)
{
  // Parse JSON string
  StaticJsonBuffer<COMMAND_JSON_SIZE> jsonBuf;
  JsonObject& root = jsonBuf.parseObject((char *)tzStr.c_str());
  if( !root.success() )
    return -1;
  if( root.size() != 3 )  // Expected 3 KVPs
    return -1;

  // Set timezone id
  if( !theConfig.SetTimeZoneID((US)root["id"]) )
    return 1;

  // Set timezone offset
  if( !theConfig.SetTimeZoneOffset((SHORT)root["offset"]) )
    return 2;

  // Set timezone dst
  if( !theConfig.SetTimeZoneOffset((UC)root["dst"]) )
    return 3;

  return 0;
}

int SmartControllerClass::CldPowerSwitch(String swStr)
{
  BOOL blnOn;

  swStr.toLowerCase();
  blnOn = (swStr == "0" || swStr == "off");

  // Turn the switch on or off
  //ToDo: send this the command queue, have command queue call DevSoftSwitch instead?
  DevSoftSwitch(blnOn);

  return 0;
}

int SmartControllerClass::CldJSONCommand(String jsonData)
{
	// ToDo: parse JSON string into all variables
	// etc: UID, temp, light, mic, motion, dayofweek, repeat, hour, min, ScenerioUID, hue1, hue2, hue3, filter, on/off

  //based on the input (ie whether it is a rule, scenario, or schedule), send the json string(s) to appropriate function.
  //These functions are responsible for adding the item to the respective, appropriate Chain. If multiple json strings coming through,
  //handle each for each respective Chain until end of incoming string

  return 0;
}
//------------------------------------------------------------------
// Cloud Interface Action Types
//------------------------------------------------------------------

bool SmartControllerClass::Change_Alarm(CMD cmd, uint8_t UID, String weekdays, bool repeat, int hour, int min, uint8_t scenerio_UID)
{
	//Params: UID, weekdays(7), bool repeat, int hour, int min, scenerio_UID

	switch (cmd)
	{
		case DELETE:
			//DELETE
			//traverse through Alarm chain of 8 items, find uid if it exists, and set flag to 'delete'. In theory, this flag change shouldn't need to be hardcoded, and will be a value provided by cloud
      //if uid doesn't exist in Alarm chain, create new row at bottom of chain, with the same uid, with the state flag set to 'delete'
			break;
		case POST:
			//POST
			//add new Alarm row to bottom of Chain. Ensure flag is 'new'. Again, flag change shouldn't need to be hardcoded, this flag value will be provided by cloud
      //before inserting new item at bottom of chain, top item in chain will need to be deleted.
			break;
		case PUT:
			//PUT
			//traverse through Alarm chain of 8 items, find uid if it exists, and set flag to 'update'. In theory, this flag change shouldn't need to be hardcoded, and will be a value provided by cloud
      //if uid doesn't exist in Alarm chain, create new row at bottom of chain, with the same uid, with the state flag set to 'update'
			break;
	}

  //After these JSON enties are dealt with, every few seconds, an action loop will scope the flags for each of the 8 rows.
  //If the flag says 'new', new alarms will be created and the row's state flag will be changed to 'active'
  //If the flag says 'delete', alarms will be deleted.
  //If the flag says 'update', old alarm will be deleted, and new alarm created. Row's state flag will change to 'active'
  //If the flag says 'active', the action loop will ignore it

  //Every 5 seconds, another loop will traverse the list, add any rows with the 'new' flag to any position in the chain that has a 'delete' flag (if no delete flags, row is added to bottom of flash chain);
  // any rows with 'delete' will need the equivalent flash row to be updated with the new flag, 'delete';
  //Similarly, any rows with 'active' will be updated in flash with new flag, active; For 'update' rows in Chain, the flash equivalent will need to be updated with all new info
}

bool SmartControllerClass::Change_Scenerio()
{
	//Possible subactions: DELETE, PUT

  //refer to Change_Alarm comments
}

bool SmartControllerClass::Change_PowerColor()
{
	//Possible subactions: PUT

	//refer to Change_Alarm comments
}

bool SmartControllerClass::Change_Sensor()
{
//ToDo: define later
}

//------------------------------------------------------------------
// Alarm Triggered Actions
//------------------------------------------------------------------
void SmartControllerClass::AlarmTimerTriggered()
{
<<<<<<< b50ec7b7415d0b7e7257b9f5e32849325a980aaf
  //ToDo: get corresponding scenerio UID / notif UID
  //ToDo: add action to command queue, send notif UID to cloud (to send to app)

	AlarmId triggered_id = Alarm.getTriggeredAlarmId();

	//search through rules table to find alarm id, and the scenerio UID

	//add action to command queue
=======
//ToDo: define later
>>>>>>> Updated comments for process of inserting JSON input through CldJsonCommand() into the 3 chains
}
