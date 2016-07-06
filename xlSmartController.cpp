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
// Alarm Triggered Actions
//------------------------------------------------------------------
void AlarmTimerTriggered(uint32_t tag)
{
	uint8_t rule_uid = (uint8_t)tag;

	//search Rule table for matching UID
	ListNode<RuleRow_t> *RuleRowptr = theSys.Rule_table.search(rule_uid);
	if (RuleRowptr == NULL)
	{
		LOGE(LOGTAG_MSG, "Error, could not locate Rule Row corresponding to triggered Alarm ID");
		return;
	}

	// read UIDs from Rule row
	uint8_t SNT_uid = RuleRowptr->data.SNT_uid;
	uint8_t notif_uid = RuleRowptr->data.notif_uid;

	//get SNT_uid from rule rows and find
	ListNode<ScenarioRow_t> *ScenarioRowptr = theSys.SearchScenario(SNT_uid);

	//Todo: search for notification table data

	//ToDo: add action to command queue, send notif UID to cloud (to send to app)
}

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
	m_SysVersion = System.version();
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
		LOGN(LOGTAG_MSG, F("RF2.4 is working."));
		SetStatus(STATUS_BMW);
	}

	// Check BLE
	CheckBLE();
	if (IsBLEGood())
	{
		LOGN(LOGTAG_MSG, F("BLE is working."));
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
		LOGN(LOGTAG_MSG, F("WAN is working."));
		SetStatus(STATUS_NWS);

		// Initialize Logger: syslog & cloud log
		// ToDo: substitude network parameters
		//theLog.InitSysLog();
		//theLog.InitCloud();
	}
	else if (GetStatus() == STATUS_BMW && IsLANGood())
	{
		LOGN(LOGTAG_MSG, F("LAN is working."));
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
		LOGD(LOGTAG_MSG, F("DHT sensor works."));
	}

	// Light
	if (theConfig.IsSensorEnabled(sensorALS)) {
		senLight.begin(SEN_LIGHT_MIN, SEN_LIGHT_MAX);
		LOGD(LOGTAG_MSG, F("Light sensor works."));
	}

	// Brightness indicator
	indicatorBrightness.configPin(0, PIN_LED_LEVEL_B0);
	indicatorBrightness.configPin(1, PIN_LED_LEVEL_B1);
	indicatorBrightness.configPin(2, PIN_LED_LEVEL_B2);
	indicatorBrightness.setLevel(theConfig.GetBrightIndicator());

	//PIR
	if (theConfig.IsSensorEnabled(sensorPIR)) {
		senMotion.begin();
		LOGD(LOGTAG_MSG, F("Motion sensor works."));
	}


	// ToDo:
	//...
}

void SmartControllerClass::InitCloudObj()
{
	// Set cloud variable initial value
	m_tzString = theConfig.GetTimeZoneJSON();

	CloudObjClass::InitCloudObj();
	LOGN(LOGTAG_MSG, F("Cloud Objects registered."));
}

// Get the controller started
BOOL SmartControllerClass::Start()
{
	// ToDo:bring controller up along with all modules (RF, wifi, BLE)

	LOGI(LOGTAG_MSG, F("SmartController started."));
	LOGI(LOGTAG_MSG, "Product Info: %s-%s-%d",
			theConfig.GetOrganization().c_str(), theConfig.GetProductName().c_str(), theConfig.GetVersion());
	LOGI(LOGTAG_MSG, "System Info: %s-%s",
			GetSysID().c_str(), GetSysVersion().c_str());
	return true;
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
	static UC tickSaveConfig = 0;				// must be static

	// Check all alarms. This triggers them.
	Alarm.delay(ms);

	// Save config if it was changed
	if (++tickSaveConfig > 10) {
		tickSaveConfig = 0;
		theConfig.SaveConfig();
	}

	// ToDo:add any other potential problems to check
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

	// ToDo: process commands from other sources (Wifi, BLE)
	// ToDo: Potentially move ReadNewRules here
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

	// ToDo: Proximity detection
	// from all channels including Wi-Fi, BLE, etc. for MAC addresses and distance to device 
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
	//ToDo: this sends data from cloud to appropriate function to change lamp color/on-off
	//ToDo: implement function to receive JSON, parse JSON, populate DevStatus_t row

	BOOL blnOn;

	swStr.toLowerCase();
	blnOn = (swStr == "0" || swStr == "off");

	// Turn the switch on or off
	DevSoftSwitch(blnOn);

	return 0;
}

int SmartControllerClass::CldJSONCommand(String jsonData)
{
  //based on the input (ie whether it is a rule, scenario, or schedule), send the json string(s) to appropriate function.
  //These functions are responsible for adding the item to the respective, appropriate Chain. If multiple json strings coming through,
  //handle each for each respective Chain until end of incoming string

  int numRows = 0;
  bool bRowsKey = true;
  StaticJsonBuffer<COMMAND_JSON_SIZE> jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(const_cast<char*>(jsonData.c_str()));

  if (!root.success())
  {
    LOGE(LOGTAG_MSG, "Error parsing input: %s", jsonData.c_str());
    numRows = 0;
  }

  if (!root.containsKey("rows"))
  {
    numRows = 1;
    bRowsKey = false;
  }
  else
  {
    numRows = root["rows"].as<int>();
  }

  int successCount = 0;
  for (int j = 0; j < numRows; j++)
  {
	  if (!bRowsKey)
	  {
		  if (ParseCmdRow(root))
		  {
			  successCount++;
		  }
	  }
	  else
	  {
		  JsonObject& data = root["data"][j];
		  if (ParseCmdRow(data))
		  {
			  successCount++;
		  }
	  }
  }
  LOGI(LOGTAG_MSG, "%d of %d rows were parsed correctly", successCount, numRows);
  return successCount;
}

bool SmartControllerClass::ParseCmdRow(JsonObject& data)
{
	bool isSuccess = true;
	OP_FLAG op_flag = (OP_FLAG)data["op_flag"].as<int>();
	FLASH_FLAG flash_flag = (FLASH_FLAG)data["flash_flag"].as<int>();
	RUN_FLAG run_flag = (RUN_FLAG)data["run_flag"].as<int>();

	if (op_flag < GET || op_flag > DELETE)
	{
		LOGE(LOGTAG_MSG, "UID:%s Invalid HTTP command: %d", data["uid"], op_flag);
		return 0;
	}

	if (op_flag != GET)
	{
		if (flash_flag != UNSAVED)
		{
			LOGE(LOGTAG_MSG, "UID:%s Invalid FLASH_FLAG", data["uid"]);
			return 0;
		}

		if (run_flag != UNEXECUTED)
		{
			LOGE(LOGTAG_MSG, "UID:%s Invalid RUN_FLAG", data["uid"]);
			return 0;
		}
	}

	//grab first part of uid and store it in uidKey, and convert rest of uid string into int uidNum:
	const char* uidWhole = data["uid"];
	char uidKey = tolower(uidWhole[0]);
	uint8_t uidNum = atoi(&uidWhole[1]);

	switch(uidKey)
	{
		case CLS_RULE:				//rule
		{
			RuleRow_t row;
			row.op_flag = op_flag;
			row.flash_flag = flash_flag;
			row.run_flag = run_flag;
			row.uid = uidNum;
			row.SCT_uid = data["SCT_uid"];
			row.SNT_uid = data["SNT_uid"];
			row.notif_uid = data["notif_uid"];

			isSuccess = Change_Rule(row);
			if (!isSuccess)
			{
				LOGE(LOGTAG_MSG, "UID:%s Unable to write row to Rule_t", data["uid"]);
				return 0;
			}
			else
			{
				LOGI(LOGTAG_MSG, "UID:%s write row to Rule_t OK", data["uid"]);
				return 1;
			}
			break;
		}
		case CLS_SCHEDULE: 		//schedule
		{
			ScheduleRow_t row;
			row.op_flag = op_flag;
			row.flash_flag = flash_flag;
			row.run_flag = run_flag;
			row.uid = uidNum;

			if (data["isRepeat"] == 1)
			{
				if (data["weekdays"] < 0 || data["weekdays"] > 7)		// [0..7]
				{
					LOGE(LOGTAG_MSG, "UID:%s Invalid 'weekdays' must between 0 and 7", data["uid"]);
					return 0;
				}
			}
			else if (data["isRepeat"] == 0)
			{
				if (data["weekdays"] < 1 || data["weekdays"] > 7)		// [1..7]
				{
					LOGE(LOGTAG_MSG, "UID:%s Invalid 'weekdays' must between 1 and 7", data["uid"]);
					return 0;
				}
			}
			else
			{
				LOGE(LOGTAG_MSG, "UID:%s Invalid 'isRepeat' must 0 or 1", data["uid"]);
				return 0;
			}

			if (data["hour"] < 0 || data["hour"] > 23)
			{
				LOGE(LOGTAG_MSG, "UID:%s Invalid 'hour' must between 0 and 23", data["uid"]);
				return 0;
			}

			if (data["min"] < 0 || data["min"] > 59)
			{
				LOGE(LOGTAG_MSG, "UID:%s Invalid 'min' must between 0 and 59", data["uid"]);
				return 0;
			}

			row.weekdays = data["weekdays"];
			row.isRepeat = data["isRepeat"];
			row.hour = data["hour"];
			row.min = data["min"];
			row.alarm_id = dtINVALID_ALARM_ID;

			isSuccess = Change_Schedule(row);
			if (!isSuccess)
			{
				LOGE(LOGTAG_MSG, "UID:%s Unable to write row to Schedule_t", data["uid"]);
				return 0;
			}
			else
			{
				LOGI(LOGTAG_MSG, "UID:%s write row to Schedule_t OK", data["uid"]);
				return 1;
			}
			break;
		}
		case CLS_SCENARIO: 	//scenario
		{
			ScenarioRow_t row;
			row.op_flag = op_flag;
			row.flash_flag = flash_flag;
			row.run_flag = run_flag;
			row.uid = uidNum;

			// Copy JSON array to Hue
			Array2Hue(data["ring1"], row.ring1);
			Array2Hue(data["ring2"], row.ring2);
			Array2Hue(data["ring3"], row.ring3);

			row.filter = data["filter"];

			isSuccess = Change_Scenario(row);
			if (!isSuccess)
			{
				LOGE(LOGTAG_MSG, "UID:%s Unable to write row to Scenario_t", data["uid"]);
				return 0;
			}
			else
			{
				LOGI(LOGTAG_MSG, "UID:%s write row to Scenario_t OK", data["uid"]);
				return 1;
			}
			break;
		}
		default:
		{
			LOGE(LOGTAG_MSG, "UID:%s Invalid", data["uid"]);
		}
	}

	return 0;
}
//------------------------------------------------------------------
// Cloud Interface Action Types
//------------------------------------------------------------------

bool SmartControllerClass::Change_Rule(RuleRow_t row)
{
	int index;
	switch (row.op_flag)
	{
		case DELETE:
			//search rules table for uid
			index = Rule_table.search_uid(row.uid);
			if (index == -1) //uid not found
			{
				//add row
				if (!Rule_table.add(row))
				{
					LOGE(LOGTAG_MSG, "Error occured while adding Rule UID:%c%d", CLS_RULE, row.uid);
					return false;
				}
			}
			else //uid found
			{
				//update row
				if (!Rule_table.set(index, row))
				{
					LOGE(LOGTAG_MSG, "Error occured while updating Rule UID:%c%d", CLS_RULE, row.uid);
					return false;
				}
			}
			break;

		case POST:
		case PUT:
			//search rule table for uid
			index = Rule_table.search_uid(row.uid);
			if (index == -1) //uid not found
			{
				//add row
				if (!Rule_table.add(row))
				{
					LOGE(LOGTAG_MSG, "Error occured while adding Rule UID:%c%d", CLS_RULE, row.uid);
					return false;
				}

				if (row.op_flag == PUT)
				{
					LOGN(LOGTAG_MSG, "PUT found no updatable, Rule added UID:%c%d", CLS_RULE, row.uid);
				}
			}
			else //uid found
			{
				//update row
				if (!Rule_table.set(index, row))
				{
					LOGE(LOGTAG_MSG, "Error occured while updating Rule UID:%c%d", CLS_RULE, row.uid);
					return false;
				}

				if (row.op_flag == POST)
				{
					LOGN(LOGTAG_MSG, "POST found duplicate row, overwriting UID:%c%d", CLS_RULE, row.uid);
				}
			}
			break;
	}
	theConfig.SetRTChanged(true);
	return true;
}

bool SmartControllerClass::Change_Schedule(ScheduleRow_t row)
{
	int index;
	switch (row.op_flag)
	{
		case DELETE:
			//search schedule table for uid
			index = Schedule_table.search_uid(row.uid);
			if (index == -1) //uid not found
			{
				//make room for new row
				if (Schedule_table.isFull())
				{
					if (!Schedule_table.delete_one_outdated_row())
					{
						LOGW(LOGTAG_MSG, "Schedule_t full, cannot process command");
						return false;
					}
				}

				//add row
				if (!Schedule_table.add(row))
				{
					LOGE(LOGTAG_MSG, "Error occured while adding Schedule UID:%c%d", CLS_SCHEDULE, row.uid);
					return false;
				}

			}
			else //uid found
			{
				//bring over old alarm id to new row if it exists
				if (Schedule_table.get(index).run_flag == EXECUTED && Alarm.isAllocated(Schedule_table.get(index).alarm_id))
				{
					//copy old alarm id into new row
					row.alarm_id = Schedule_table.get(index).alarm_id;
				}
				else
				{
					LOGN(LOGTAG_MSG, "Previous alarm not set, replacing Schedule UID:%c%d", CLS_SCHEDULE, row.uid);
				}

				//update row
				if (!Schedule_table.set(index, row))
				{
					LOGE(LOGTAG_MSG, "Error occured while updating Schedule UID:%c%d", CLS_SCHEDULE, row.uid);
					return false;
				}
			}
			break;

		case POST:
		case PUT:
			//search schedule table for uid
			index = Schedule_table.search_uid(row.uid);
			if (index == -1) //uid not found
			{
				//make room for new row
				if (Schedule_table.isFull())
				{
					if (!Schedule_table.delete_one_outdated_row())
					{
						LOGW(LOGTAG_MSG, "Schedule_t full, cannot process command");
						return false;
					}
				}

				//add row
				if (!Schedule_table.add(row))
				{
					LOGE(LOGTAG_MSG, "Error occured while adding Schedule UID:%c%d", CLS_SCHEDULE, row.uid);
					return false;
				}

				if (row.op_flag == PUT)
				{
					LOGN(LOGTAG_MSG, "PUT found no updatable, Schedule added UID:%c%d", CLS_SCHEDULE, row.uid);
				}
			}
			else //uid found
			{
				//bring over old alarm id to new row if it exists
				if (Schedule_table.get(index).run_flag == EXECUTED && Alarm.isAllocated(Schedule_table.get(index).alarm_id))
				{
					//copy old alarm id into new row
					row.alarm_id = Schedule_table.get(index).alarm_id;
				}
				else
				{
					LOGN(LOGTAG_MSG, "Previous alarm not set, replacing Schedule UID:%c%d", CLS_SCHEDULE, row.uid);
				}

				//update row
				if (!Schedule_table.set(index, row))
				{
					LOGE(LOGTAG_MSG, "Error occured while updating Schedule UID:%c%d", CLS_SCHEDULE, row.uid);
					return false;
				}

				if (row.op_flag == POST)
				{
					LOGN(LOGTAG_MSG, "POST found duplicate row, overwriting UID:%c%d", CLS_SCHEDULE, row.uid);
				}
			}
			break;
	}
	theConfig.SetSCTChanged(true);
	return true;
}

bool SmartControllerClass::Change_Scenario(ScenarioRow_t row)
{
	int index;
	switch (row.op_flag)
	{
		case DELETE:
			//search scenario table for uid
			index = Scenario_table.search_uid(row.uid);
			if (index == -1) //uid not found
			{
				//make room for new row
				if (Scenario_table.isFull())
				{
					if (!Scenario_table.delete_one_outdated_row())
					{
						LOGW(LOGTAG_MSG, "Scenario_t full, cannot process command");
						return false;
					}
				}

				//add row
				if (!Scenario_table.add(row))
				{
					LOGE(LOGTAG_MSG, "Error occured while adding Scenario UID:%c%d", CLS_SCENARIO, row.uid);
					return false;
				}
			}
			else //uid found
			{
				//update row
				if (!Scenario_table.set(index, row))
				{
					LOGE(LOGTAG_MSG, "Error occured while updating Scenario UID:%c%d", CLS_SCENARIO, row.uid);
					return false;
				}
			}
			break;

		case POST:
		case PUT:
			//search scenario table for uid
			index = Scenario_table.search_uid(row.uid);
			if (index == -1) //uid not found
			{
				//make room for new row
				if (Scenario_table.isFull())
				{
					if (!Scenario_table.delete_one_outdated_row())
					{
						LOGW(LOGTAG_MSG, "Scenario_t full, cannot process command");
						return false;
					}
				}

				//add row
				if (!Scenario_table.add(row))
				{
					LOGE(LOGTAG_MSG, "Error occured while adding Scenario UID:%c%d", CLS_SCENARIO, row.uid);
					return false;
				}

				if (row.op_flag == PUT)
				{
					LOGN(LOGTAG_MSG, "PUT found no updatable, Scenario added UID:%c%d", CLS_SCENARIO, row.uid);
				}
			}
			else //uid found
			{
				//update row
				if (!Scenario_table.set(index, row))
				{
					LOGE(LOGTAG_MSG, "Error occured while updating Scenario UID:%c%d", CLS_SCENARIO, row.uid);
					return false;
				}

				if (row.op_flag == POST)
				{
					LOGN(LOGTAG_MSG, "POST found duplicate row, overwriting UID:%c%d", CLS_SCENARIO, row.uid);
				}
			}
			break;
	}
	theConfig.SetSNTChanged(true);
	return true;
}

bool SmartControllerClass::Change_Sensor()
{
  	//Possible subactions: GET
    //ToDo: define later

	return 1;
}

//------------------------------------------------------------------
// Acting on new rows in working memory Chains
//------------------------------------------------------------------

void SmartControllerClass::ReadNewRules()
{
	if (theConfig.IsRTChanged())
	{
		ListNode<RuleRow_t> *ruleRowPtr = Rule_table.getRoot();
		while (ruleRowPtr != NULL)
		{
			Action_Rule(ruleRowPtr);
			ruleRowPtr = ruleRowPtr->next;
		} //end of loop
	}
}

bool SmartControllerClass::CreateAlarm(ListNode<ScheduleRow_t>* scheduleRow, uint32_t tag)
{
	//Use weekday, isRepeat, hour, min information to create appropriate alarm
	//The created alarm obj must have a field for rule_id so it has a reference to what created it

	//If isRepeat is 1 AND:
		//if weekdays value is between 1 and 7, the alarm is to be repeated weekly on the specified weekday
		//if weekdays value is 0, alarm is to be repeated daily
	//if isRepeat is 0:
		//alarm is to to be triggered on specified weekday; weekday cannot be 0.

	AlarmId alarm_id;
	if (scheduleRow->data.isRepeat == 1)
	{
		if (scheduleRow->data.weekdays > 0 && scheduleRow->data.weekdays <= 7) {
			//repeat weekly on given weekday
			alarm_id = Alarm.alarmRepeat((timeDayOfWeek_t)scheduleRow->data.weekdays, scheduleRow->data.hour, scheduleRow->data.min, 0, AlarmTimerTriggered);
			Alarm.setAlarmTag(alarm_id, tag);
		}
		else if (scheduleRow->data.weekdays == 0)
		{
			//weekdays == 0 refers to daily repeat
			time_t tmValue = AlarmHMS(scheduleRow->data.hour, scheduleRow->data.min, 0);
			alarm_id = Alarm.alarmRepeat(tmValue, AlarmTimerTriggered);
			Alarm.setAlarmTag(alarm_id, tag);
		}
		else
		{
			LOGN(LOGTAG_MSG, "Cannot create Alarm via UID:%c%s. Incorrect weekday value.", CLS_RULE, tag);
			return false;
		}
	}
	else if (scheduleRow->data.isRepeat == 0) {
		alarm_id = Alarm.alarmOnce((timeDayOfWeek_t)scheduleRow->data.weekdays, scheduleRow->data.hour, scheduleRow->data.min, 0, AlarmTimerTriggered);
		Alarm.setAlarmTag(alarm_id, tag);
	}
	else {
		LOGN(LOGTAG_MSG, "Cannot create Alarm via UID:%c%s. Incorrect isRepeat value.", CLS_RULE, tag);
		return false;
	}
	//Update that schedule row's alarm_id field with the newly created alarm's alarm_id
	scheduleRow->data.alarm_id = alarm_id;
	LOGI(LOGTAG_MSG, "Alarm created via UID:%c%s", CLS_RULE, tag);
	return true;
}

bool SmartControllerClass::DestoryAlarm(AlarmId alarmID, UC SCT_uid)
{
	if(!Alarm.isAllocated(alarmID))
		return false;

	Alarm.disable(alarmID);
	Alarm.free(alarmID);
	LOGN(LOGTAG_MSG, "Destory alarm via UID:%c%d", CLS_SCHEDULE, SCT_uid);

	return true;
	//Note: Remember to always follow up this function call with setting the AlarmID to dtINVALID_ALARM_ID
}

bool SmartControllerClass::Action_Rule(ListNode<RuleRow_t> *rulePtr)
{
	if(!rulePtr)
		return false;

	if (rulePtr->data.run_flag == UNEXECUTED)
	{
		// Process Schedule
		Action_Schedule(rulePtr->data.op_flag, rulePtr->data.SCT_uid, rulePtr->data.uid);

		//Search for SNT data
		ListNode<ScenarioRow_t> *scenarioPtr = SearchScenario(rulePtr->data.SNT_uid);

		// ToDo: search and set other trigger conditions
		//...

		rulePtr->data.run_flag = EXECUTED;
		scenarioPtr->data.run_flag = EXECUTED;
	}

	return true;
}

bool SmartControllerClass::Action_Schedule(OP_FLAG parentFlag, UC uid, UC rule_uid)
{
	bool retVal = true;

	//get SCT_uid from rule rows and find SCT_uid in SCT LinkedList
	ListNode<ScheduleRow_t> *scheduleRow = SearchSchedule(uid);
	if (scheduleRow) //found schedule row in chain or Flash
	{
		if (scheduleRow->data.run_flag == UNEXECUTED)
		{
			switch(scheduleRow->data.op_flag)
			{
			case POST:
			case PUT:
				if(parentFlag == POST || parentFlag == PUT)
				{
					// Recreate Alarm
					DestoryAlarm(scheduleRow->data.alarm_id, uid);
					scheduleRow->data.alarm_id = dtINVALID_ALARM_ID;
					CreateAlarm(scheduleRow, rule_uid);
				}
				break;

			case DELETE:
				// Delete Alarm
				LOGI(LOGTAG_MSG, "Found to be deleted object UID:%c%d", CLS_SCHEDULE, uid);
				DestoryAlarm(scheduleRow->data.alarm_id, uid);
				scheduleRow->data.alarm_id = dtINVALID_ALARM_ID;
				break;

			case GET:			// ToDo:...
					break;
			}
		}
		else	// EXECUTED, double check
		{
			if(parentFlag != DELETE && parentFlag != GET && scheduleRow->data.op_flag != DELETE)
			{
				// The alarm is supposed to exist, otherwise create it
				if (!Alarm.isAllocated(scheduleRow->data.alarm_id))
				{
					CreateAlarm(scheduleRow, rule_uid);
				}
			}
		}

		// Parent Flag Special Check
		if( parentFlag == DELETE ) {
			// Destory Alarm if exists
			DestoryAlarm(scheduleRow->data.alarm_id, uid);
			scheduleRow->data.alarm_id = dtINVALID_ALARM_ID;
		}

		// Set flag anyway
		scheduleRow->data.run_flag = EXECUTED;
	}
	else //schedule row not found in chain or Flash
	{
		LOGW(LOGTAG_MSG, "Cannot find object with UID:%c%d", CLS_SCHEDULE, uid);
		retVal = false;
	}

	return retVal;
}

//------------------------------------------------------------------
// UID Search Functions (Search chain, then Flash)
//------------------------------------------------------------------
ListNode<ScheduleRow_t> *SmartControllerClass::SearchSchedule(UC uid)
{
	//search chain in working memory
	ListNode<ScheduleRow_t> *pObj = Schedule_table.search(uid);

	if(!pObj)
	{
		// Search Flash and validate data entry
		ScheduleRow_t row;
		if (uid < MAX_SCT_ROWS)
		{
			// Find it
			EEPROM.get(MEM_SCHEDULE_OFFSET + uid*SCT_ROW_SIZE, row);

			// flags should be 111
			if(row.uid == uid && row.op_flag == (OP_FLAG)1
							  && row.flash_flag == (FLASH_FLAG)1
							  && row.run_flag == (RUN_FLAG)1)
			{
				// Copy data entry to Working Memory and get the pointer
				row.op_flag = POST;				// op_flag should already be POST==(OP_FLAG)1
				row.run_flag = UNEXECUTED;		// need to create Alarm later
				row.flash_flag = SAVED;			// We know it has a copy in flash
				if (Change_Schedule(row))
				{
					//pObj = Schedule_table.search(uid);
					pObj = Schedule_table.getLast();		// Faster
					LOGN(LOGTAG_MSG, "UID:%c%d copy Flash to Schedule_t OK", CLS_SCHEDULE, uid);
				}
				else
				{
					LOGE(LOGTAG_MSG, "UID:%c%d Unable to copy Flash to Schedule_t", CLS_SCHEDULE, uid);
				}
			}
		}
	}

	return pObj;
}

ListNode<ScenarioRow_t>* SmartControllerClass::SearchScenario(UC uid)
{
	ListNode<ScenarioRow_t> *pObj = Scenario_table.search(uid); //search chain

	if (!pObj) //not found in working memory
	{
		//search Flash and validate data entry
		ScenarioRow_t row;
		if (uid < MAX_SNT_ROWS)
		{
			//find it
			theConfig.P1Flash->read<ScenarioRow_t>(row, MEM_SCENARIOS_OFFSET + uid*SNT_ROW_SIZE);

			//flags should be 111
			if (row.uid == uid && row.op_flag == (OP_FLAG)1
							   && row.flash_flag == (FLASH_FLAG)1
							   && row.run_flag == (RUN_FLAG)1)
			{
				// Copy data entry into working memory and get the pointer
				row.op_flag = POST;
				row.run_flag = UNEXECUTED;
				row.flash_flag = SAVED;			//we know it has a copy in flash
				if (Change_Scenario(row))
				{
					pObj = Scenario_table.getLast();
					LOGN(LOGTAG_MSG, "UID:%c%d copy Flash to Scenario_t OK", CLS_SCENARIO, uid);
				}
				else
				{
					LOGE(LOGTAG_MSG, "UID:%c%d Unable to copy Flash to Scenario_t", CLS_SCENARIO, uid);
				}
			}
		}
	}

	return pObj;
}

//------------------------------------------------------------------
// Place all util func below
//------------------------------------------------------------------
// Copy JSON array to Hue structure
void SmartControllerClass::Array2Hue(JsonArray& data, Hue_t& hue)
{
	hue.State = data[0];
	hue.CW = data[1];
	hue.WW = data[2];
	hue.R = data[3];
	hue.G = data[4];
	hue.B = data[5];
}
