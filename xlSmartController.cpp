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
	if (IsRFGood())
	{
		LOGN(LOGTAG_MSG, "RF2.4 is working.");
		SetStatus(STATUS_BMW);
	}

	// Check BLE
	CheckBLE();
	if (IsBLEGood())
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
	if (IsWANGood())
	{
		LOGN(LOGTAG_MSG, "WAN is working.");
		SetStatus(STATUS_NWS);

		// Initialize Logger: syslog & cloud log
		// ToDo: substitude network parameters
		//theLog.InitSysLog();
		//theLog.InitCloud();
	}
	else if (GetStatus() == STATUS_BMW && IsLANGood())
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
	if (theConfig.IsSensorEnabled(sensorDHT)) {
		senDHT.begin();
		LOGD(LOGTAG_MSG, "DHT sensor works.");
	}

	// Light
	if (theConfig.IsSensorEnabled(sensorALS)) {
		senLight.begin(SEN_LIGHT_MIN, SEN_LIGHT_MAX);
		LOGD(LOGTAG_MSG, "Light sensor works.");
	}


	// Brightness indicator
	indicatorBrightness.configPin(0, PIN_LED_LEVEL_B0);
	indicatorBrightness.configPin(1, PIN_LED_LEVEL_B1);
	indicatorBrightness.configPin(2, PIN_LED_LEVEL_B2);
	indicatorBrightness.setLevel(theConfig.GetBrightIndicator());

	//PIR
	if (theConfig.IsSensorEnabled(sensorPIR)) {
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
	if ((UC)m_devStatus != st)
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
	if (++tickSaveConfig > 10) {
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

	switch (GetStatus()) {
	case STATUS_DIS:
	case STATUS_NWS:    // Normal speed
		if (theConfig.IsSensorEnabled(sensorDHT)) {
			if (tick % SEN_DHT_SPEED_NORMAL == 0)
				blnReadDHT = true;
		}
		if (theConfig.IsSensorEnabled(sensorALS)) {
			if (tick % SEN_ALS_SPEED_NORMAL == 0)
				blnReadALS = true;
		}
		if (theConfig.IsSensorEnabled(sensorPIR)) {
			if (tick % SEN_PIR_SPEED_NORMAL == 0)
				blnReadPIR = true;
		}
		break;

	case STATUS_SLP:    // Lower speed in sleep mode
		if (theConfig.IsSensorEnabled(sensorDHT)) {
			if (tick % SEN_DHT_SPEED_LOW == 0)
				blnReadDHT = true;
		}
		if (theConfig.IsSensorEnabled(sensorALS)) {
			if (tick % SEN_ALS_SPEED_LOW == 0)
				blnReadALS = true;
		}
		if (theConfig.IsSensorEnabled(sensorPIR)) {
			if (tick % SEN_PIR_SPEED_LOW == 0)
				blnReadPIR = true;
		}
		break;

	default:
		return;
	}

	// Read from DHT
	if (blnReadDHT) {
		float t = senDHT.getTempCelcius();
		float h = senDHT.getHumidity();

		if (!isnan(t)) {
			UpdateTemperature(t);
		}
		if (!isnan(h)) {
			UpdateHumidity(h);
		}
	}

	// Read from ALS
	if (blnReadALS) {
		UpdateBrightness(senLight.getLevel());
	}

	// Motion detection
	if (blnReadPIR) {
		UpdateMotion(senMotion.getMotion());
	}

	// Update json data and publish on to the cloud
	if (blnReadDHT || blnReadALS || blnReadPIR) {
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
	if (!root.success())
		return -1;
	if (root.size() != 3)  // Expected 3 KVPs
		return -1;

	// Set timezone id
	if (!theConfig.SetTimeZoneID((US)root["id"]))
		return 1;

	// Set timezone offset
	if (!theConfig.SetTimeZoneOffset((SHORT)root["offset"]))
		return 2;

	// Set timezone dst
	if (!theConfig.SetTimeZoneOffset((UC)root["dst"]))
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
  int counter = 0;
  StaticJsonBuffer<500> jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(const_cast<char*>(jsonData.c_str()));

  if (!root.success()) {
    LOGE(LOGTAG_MSG, "Error parsing input.");
    counter = 0;
  }

  if (!root.containsKey("rows")) {
    counter = 1;
  } else {
    counter = root["rows"].as<int>();
  }

  int isSuccess;
	for (int j = 0; j < counter; j++) {
    JsonObject& data = root["data"][j];
    isSuccess = 1;

    if ( (strcmp(data["op_flag"].asString(), "GET") != 0) && (strcmp(data["op_flag"].asString(), "POST") != 0) && (strcmp(data["op_flag"].asString(), "PUT") != 0) && (strcmp(data["op_flag"].asString(), "DELETE") != 0) ) {
      LOGE(LOGTAG_MSG, "Invalid HTTP command.");
      isSuccess = 0;
    }

    if (strcmp(data["flash_flag"].asString(), "UNSAVED") != 0) {
      LOGE(LOGTAG_MSG, "Invalid FLASH_FLAG. Should be unsaved upon entry.");
      isSuccess = 0;
    }

    if (strcmp(data["run_flag"].asString(), "UNEXECUTED") != 0) {
      LOGE(LOGTAG_MSG, "Invalid RUN_FLAG. Should be unexecuted upon entry.");
      isSuccess = 0;
    }

		if (isSuccess == 1) {
			//grab first part of uid and store it in uidKey:
      String uidWhole = data["uid"].asString();
      int pos = uidWhole.indexOf(".");
      String uidKey = uidWhole.substring(0, pos);

			if (strcmp(uidKey,"1") == 0) //rule
			{
				RuleRow_t row;
				row.op_flag = (OP_FLAG)data["op_flag"].as<int>();
				row.flash_flag = (FLASH_FLAG)data["flash_flag"].as<int>();
				row.run_flag = (RUN_FLAG)data["run_flag"].as<int>();
				row.uid = data["uid"];
				row.SCT_uid = data["SCT_uid"];
				row.alarm_id = data["alarm_id"];
				row.SNT_uid = data["SNT_uid"];
				row.notif_uid = data["notif_uid"];

				isSuccess = Change_Rule(row);

				if (isSuccess = 0) {
					LOGE(LOGTAG_MSG, "Unable to write to Rule table.");
				}
				else {
					LOGN(LOGTAG_MSG, "Success. Able to write to Rule table.");
				}
			}
			else if (strcmp(uidKey,"2") == 0) //schedule
			{
				ScheduleRow_t row;
				row.op_flag = (OP_FLAG)data["op_flag"].as<int>();
				row.flash_flag = (FLASH_FLAG)data["flash_flag"].as<int>();
				row.run_flag = (RUN_FLAG)data["run_flag"].as<int>();
				row.uid = data["uid"];

				//TODO: extract weekday/repeat info from byte passed in
				//row.weekdays = data["weekdays"];
				//row.isRepeat = data["isRepeat"];

				row.hour = data["hour"];
				row.min = data["min"];

				isSuccess = Change_Schedule(row);

				if (isSuccess = 0) {
					LOGE(LOGTAG_MSG, "Unable to write to Schedule table.");
				}
				else {
					LOGN(LOGTAG_MSG, "Success. Able to write to Schedule table.");
				}
			}
			else if (strcmp(uidKey,"3") == 0) //scenario
			{
				ScenarioRow_t row;
				row.op_flag = (OP_FLAG)data["op_flag"].as<int>();
				row.flash_flag = (FLASH_FLAG)data["flash_flag"].as<int>();
				row.run_flag = (RUN_FLAG)data["run_flag"].as<int>();
				row.uid = data["uid"];

				row.ring1.State = data["ring1"][0];
        row.ring1.CW = data["ring1"][1];
        row.ring1.WW = data["ring1"][2];
        row.ring1.R = data["ring1"][3];
        row.ring1.G = data["ring1"][4];
        row.ring1.B = data["ring1"][5];

        row.ring2.State = data["ring1"][0];
        row.ring2.CW = data["ring1"][1];
        row.ring2.WW = data["ring1"][2];
        row.ring2.R = data["ring1"][3];
        row.ring2.G = data["ring1"][4];
        row.ring2.B = data["ring1"][5];

        row.ring3.State = data["ring1"][0];
        row.ring3.CW = data["ring1"][1];
        row.ring3.WW = data["ring1"][2];
        row.ring3.R = data["ring1"][3];
        row.ring3.G = data["ring1"][4];
        row.ring3.B = data["ring1"][5];

				row.filter = data["filter"];

				isSuccess = Change_Scenario(row);

				if (isSuccess = 0) {
					LOGE(LOGTAG_MSG, "Unable to write to Scenario table.");
				}
				else {
					LOGN(LOGTAG_MSG, "Sucess. Able to write to Scenario table.");
				}
			}
			else
			{
				LOGE(LOGTAG_MSG, "Invalid UID. Could not determine further action.");
			}
		}
		else { //if isSuccess != 1
			LOGE(LOGTAG_MSG, "Incorrect flag values: Unable to read the values in.");
		}
	} //end of for loop
	return 1;
}
//------------------------------------------------------------------
// Cloud Interface Action Types
//------------------------------------------------------------------

//After these JSON enties are dealt with, every few seconds, an action loop will scope the flags for each of the 8 rows.
//If the state_flag says 'POST', new alarms will be created and the row's run_flag will be changed to 'EXECUTED'
//If the flag says 'DELETE', alarm will be deleted, and run_flag will be changed to 'EXECUTED'

//Every 5 seconds, another loop will traverse the working memory chain, and check if m_isChanged == true (this flag gets changed everytime a change is made to the chains)
//This loop will traverse the chain and write any rows with flash_flag = 0 to flash memory, and it will change those rows' flash_flag to 1
//sidenote: the process for assigning UIDs to each row will be taken care of by the cloud. Whenever the cloud sends a delete request to working memory through
//CldJSONCommand, it now knows the UID has been freed, and can be used for subsequent 'new' rows to be sent. In this manner, the flash table will simply update
//itself using UID, naturally replacing any deleted rows and always having an updated, clean table.
//any rows with flash_flag = 1 will simply be ignored

bool SmartControllerClass::Change_Rule(RuleRow_t row)
{
	return true;
}

bool SmartControllerClass::Change_Schedule(ScheduleRow_t row)
{
	//Params: UID, weekdays(7), bool repeat, int hour, int min, scenerio_UID

	switch (row.op_flag)
	{
	case DELETE:
		//DELETE

		//traverse through Alarm chain of 8 items, find uid if it exists. state_flag is delete, run_flag is 0, flash_flag is 0
  //if uid doesn't exist in Alarm chain, create new row at bottom of chain, with the same uid, with the same flags as above
  //before inserting to bottom of chain, topmost item which has flash_flag = 1 will be deleted to keep length of chain to 8
		break;
	case POST:
	case PUT:
		//PUT and POST
		//traverse through Alarm chain of 8 items, find uid if it exists, and delete old alarm
  //overwrite old row, set run_flag to 0, flash_flag to 0, state_flag to put
  //if uid doesn't exist in Alarm chain, create new row at bottom of chain, with the same uid; run_flag is 0, flash_flag is 0, run_flag is 0
  //before inserting to bottom of chain, topmost item which has flash_flag = 1 will be deleted to keep length of chain to 8
		break;
	}
}

bool SmartControllerClass::Change_Scenario(ScenarioRow_t row)
{
	//Possible subactions: DELETE, PUT, POST

  //refer to Change_Alarm comments
}

bool SmartControllerClass::Change_PowerColor(DevStatus_t row) //? link with CldPowerSwitch
{
	//Possible subactions: PUT

	//refer to Change_Schedule comments
}

bool SmartControllerClass::Change_Sensor() // get temperature?
{
	//Possible subactions: GET
	//ToDo: define later
}

//------------------------------------------------------------------
// Alarm Triggered Actions
//------------------------------------------------------------------
void SmartControllerClass::AlarmTimerTriggered()
{

	//ToDo: get corresponding scenerio UID / notif UID
	//ToDo: add action to command queue, send notif UID to cloud (to send to app)

	  //AlarmId triggered_id = Alarm.getTriggeredAlarmId();

	  //search through rules table to find alarm id, and the scenerio UID

	  //add action to command queue

}
