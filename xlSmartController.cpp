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
#include "xliPinMap.h"
#include "xlxConfig.h"
#include "xlxLogger.h"
#include "xlxRF24Server.h"
#include "xlxSerialConsole.h"

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
	SERIAL_LN("Rule %u Alarm Triggered", rule_uid);


	//search Rule table for matching UID
	ListNode<RuleRow_t> *RuleRowptr = theSys.Rule_table.search(rule_uid);
	if (RuleRowptr == NULL)
	{
		LOGE(LOGTAG_MSG, "Error, could not locate Rule Row corresponding to triggered Alarm ID");
		return;
	}

	// read UIDs from Rule row
	uint8_t SNT_uid = RuleRowptr->data.SNT_uid;
	uint8_t node_id = RuleRowptr->data.node_id;

	//get SNT_uid from rule rows and find
	ListNode<ScenarioRow_t> *rowptr = theSys.SearchScenario(SNT_uid);

	if (rowptr)
	{
		String ring1_payload = theSys.CreateColorPayload(1, rowptr->data.ring1.State, rowptr->data.ring1.CW, rowptr->data.ring1.WW, rowptr->data.ring1.R, rowptr->data.ring1.G, rowptr->data.ring1.B);
		String ring2_payload = theSys.CreateColorPayload(2, rowptr->data.ring2.State, rowptr->data.ring2.CW, rowptr->data.ring2.WW, rowptr->data.ring2.R, rowptr->data.ring2.G, rowptr->data.ring2.B);
		String ring3_payload = theSys.CreateColorPayload(3, rowptr->data.ring3.State, rowptr->data.ring3.CW, rowptr->data.ring3.WW, rowptr->data.ring3.R, rowptr->data.ring3.G, rowptr->data.ring3.B);

		char buf[64];

		sprintf(buf, "%d;%d;%d;%d;%d;%s", node_id, S_CUSTOM, C_SET, 1, V_STATUS, ring1_payload.c_str());
		String ring1_cmd(buf);
		theSys.ExecuteLightCommand(ring1_cmd);

		sprintf(buf, "%d;%d;%d;%d;%d;%s", node_id, S_CUSTOM, C_SET, 1, V_STATUS, ring2_payload.c_str());
		String ring2_cmd(buf);
		theSys.ExecuteLightCommand(ring2_cmd);

		sprintf(buf, "%d;%d;%d;%d;%d;%s", node_id, S_CUSTOM, C_SET, 1, V_STATUS, ring3_payload.c_str());
		String ring3_cmd(buf);
		theSys.ExecuteLightCommand(ring3_cmd);
	}
	else
	{
		LOGE(LOGTAG_MSG, "Could not change node:%d light's color, scenerio %d not found", node_id, SNT_uid);
		return;
	}
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
  // Open Serial Port
  TheSerial.begin(SERIALPORT_SPEED_DEFAULT);

#ifdef SYS_SERIAL_DEBUG
	// Wait Serial connection so that we can see the starting information
	while(!TheSerial.available()) { Particle.process(); }
	SERIAL_LN(F("SmartController is starting..."));
#endif

	// Get System ID
	m_SysID = System.deviceID();
	m_SysVersion = System.version();
	m_devStatus = STATUS_INIT;

	// Initialize Logger
	theLog.Init(m_SysID);
	theLog.InitFlash(MEM_OFFLINE_DATA_OFFSET, MEM_OFFLINE_DATA_LEN);

	LOGN(LOGTAG_MSG, "SmartController is starting...SysID=%s", m_SysID.c_str());
}

// Second level initialization after loading configuration
/// check RF2.4 & BLE
void SmartControllerClass::InitRadio()
{
	// Check RF2.4
	if( CheckRF() ) {
  	if (IsRFGood())
  	{
  		LOGN(LOGTAG_MSG, F("RF2.4 is working."));
  		SetStatus(STATUS_BMW);
  	}
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
	BOOL oldWAN = IsWANGood();
	BOOL oldLAN = IsLANGood();

	// Check WAN and LAN
	CheckNetwork();

	if (IsWANGood())
	{
		if( !oldWAN ) {	// Only log when status changed
			LOGN(LOGTAG_EVENT, F("WAN is working."));
			SetStatus(STATUS_NWS);
		}

		// Initialize Logger: syslog & cloud log
		// ToDo: substitude network parameters
		//theLog.InitSysLog();
		//theLog.InitCloud();
	}
	else if (IsLANGood())
	{
		if( !oldLAN ) {	// Only log when status changed
			LOGN(LOGTAG_EVENT, F("LAN is working."));
			SetStatus(STATUS_DIS);
		}
	}
	else if (IsRFGood()) {
		SetStatus(STATUS_BMW);
	}
	else {
		SetStatus(STATUS_ERR);
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
	//pinMode(PIN_LED_LEVEL_B0, OUTPUT);
	//pinMode(PIN_LED_LEVEL_B1, OUTPUT);
	//pinMode(PIN_LED_LEVEL_B2, OUTPUT);

	// Set communication pin mode
	pinMode(PIN_BLE_RX, INPUT);
	pinMode(PIN_BLE_TX, OUTPUT);
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
		senLight.begin(SEN_LIGHT_MAX, SEN_LIGHT_MIN);	// Reversed threshold
		LOGD(LOGTAG_MSG, F("Light sensor works."));
	}

	// Brightness indicator
	//indicatorBrightness.configPin(0, PIN_LED_LEVEL_B0);
	//indicatorBrightness.configPin(1, PIN_LED_LEVEL_B1);
	//indicatorBrightness.configPin(2, PIN_LED_LEVEL_B2);
	//indicatorBrightness.setLevel(theConfig.GetBrightIndicator());

	// PIR
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

#ifndef SYS_SERIAL_DEBUG
	ResetSerialPort();
#endif

	return true;
}

void SmartControllerClass::Restart()
{
	SetStatus(STATUS_RST);
	delay(1000);
	System.reset();
}

UC SmartControllerClass::GetStatus()
{
	return (UC)m_devStatus;
}

BOOL SmartControllerClass::SetStatus(UC st)
{
	if( st > STATUS_ERR ) return false;
	LOGN(LOGTAG_STATUS, F("System status changed from %d to %d"), m_devStatus, st);
	if ((UC)m_devStatus != st) {
		m_devStatus = st;
	}
	return true;
}

// Close and reopen serial port to avoid buffer overrun
void SmartControllerClass::ResetSerialPort()
{
	TheSerial.end();
	TheSerial.begin(SERIALPORT_SPEED_DEFAULT);
}

BOOL SmartControllerClass::CheckRF()
{
	// RF Server begins
	m_isRF = theRadio.ServerBegin();
	if( m_isRF ) {
		// Change it if setting is not default value
		theRadio.setPALevel(theConfig.GetRFPowerLevel());
	}
	return m_isRF;
}

// Check Wi-Fi module and connection
BOOL SmartControllerClass::CheckWiFi()
{
	if( WiFi.RSSI() > 0 ) {
		m_isWAN = false;
		m_isLAN = false;
		LOGE(LOGTAG_MSG, F("Wi-Fi chip error!"));
		return false;
	}

	if( !WiFi.ready() ) {
		m_isWAN = false;
		m_isLAN = false;
		LOGE(LOGTAG_EVENT, F("Wi-Fi module error!"));
		return false;
	}
	return true;
}

BOOL SmartControllerClass::CheckNetwork()
{
	// Check Wi-Fi module
	if( !CheckWiFi() ) {
		return false;
	}

	// Check WAN
	m_isWAN = WiFi.resolve("www.google.com");

	// Check LAN if WAN is not OK
	if( !m_isWAN ) {
		m_isLAN = (WiFi.ping(WiFi.gatewayIP(), 3) < 3);
		if( !m_isLAN ) {
			LOGW(LOGTAG_MSG, F("Cannot reach local gateway!"));
			m_isLAN = (WiFi.ping(WiFi.localIP(), 3) < 3);
			if( !m_isLAN ) {
				LOGE(LOGTAG_MSG, F("Cannot reach itself!"));
			}
		}
	} else {
		m_isLAN = true;
	}

	return true;
}

BOOL SmartControllerClass::CheckBLE()
{
	// ToDo: change value of m_isBLE

	return true;
}

BOOL SmartControllerClass::SelfCheck(US ms)
{
	static US tickSaveConfig = 0;				// must be static
	static US tickCheckRadio = 0;				// must be static
	static UC tickAcitveCheck = 0;

	// Check all alarms. This triggers them.
	Alarm.delay(ms);

	// Save config if it was changed
	if (++tickSaveConfig > 5000 / ms) {	// once per 5 seconds
		tickSaveConfig = 0;
		theConfig.SaveConfig();
	}

  // Slow Checking: once per 30 seconds
  if (++tickCheckRadio > 30000 / ms) {
		// Check RF module
		++tickAcitveCheck;
		tickCheckRadio = 0;
    if( !IsRFGood() ) {
      if( CheckRF() ) {
        LOGN(LOGTAG_MSG, F("RF24 module recovered."));
      }
    }

		// Check Network
		if( !IsWANGood() || tickAcitveCheck % 5 == 0 || GetStatus() == STATUS_DIS ) {
			InitNetwork();
		}

		// Daily Cloud Synchronization
		/// TimeSync
		theConfig.CloudTimeSync(false);
  }

	// Check System Status
	if( GetStatus() == STATUS_ERR ) {
		LOGE(LOGTAG_MSG, "System is about to reset due to STATUS_ERR...");
		Restart();
	}

	// ToDo:add any other potential problems to check
	//...

	return true;
}

BOOL SmartControllerClass::CheckRFBaseNetEnableDur()
{
	if( theConfig.GetMaxBaseNetworkDur() > 0 ) {
		if( theRadio.getBaseNetworkDuration() > theConfig.GetMaxBaseNetworkDur() ) {
			theRadio.enableBaseNetwork(false);
			LOGI(LOGTAG_MSG, F("RF base network enable timeout, disable it automatically"));
			return true;
		}
	}

	return false;
}

BOOL SmartControllerClass::IsRFGood()
{
	return (m_isRF && theRadio.isValid());
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
	theRadio.ProcessReceive();

	// Process Console Command
  theConsole.processCommand();

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
	String strCmd = String::format("%d;%d;%d;%d;%d;%d", dev, S_DIMMER, C_SET, 1, V_STATUS, (sw ? 1:0));
	ExecuteLightCommand(strCmd);

	//ToDo: if dev = 0, go through list of devices
	// ToDo:
	//SetStatus();

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
// Format: {"id":90, "offset":-300, "dst":1}
int SmartControllerClass::CldSetTimeZone(String tzStr)
{
	// Parse JSON string
	StaticJsonBuffer<COMMAND_JSON_SIZE * 8> lv_jBuf;
	JsonObject& root = lv_jBuf.parseObject(const_cast<char*>(tzStr.c_str()));
	if (!root.success())
		return -1;
	if (root.size() != 3)  // Expected 3 KVPs
		return -1;
	if(!root.containsKey("id")) return 1;
	if(!root.containsKey("offset")) return 2;
	if(!root.containsKey("dst")) return 3;

	// Set timezone id
	if (!theConfig.SetTimeZoneID((US)root["id"].as<int>()))
		return 1;

	// Set timezone offset
	if (!theConfig.SetTimeZoneOffset((SHORT)root["offset"].as<int>()))
		return 2;

	// Set timezone dst
	if (!theConfig.SetDaylightSaving((UC)root["dst"].as<int>()))
		return 3;

	LOGI(LOGTAG_EVENT, "setTimeZone to %d;%d;%d", root["id"].as<int>(), root["offset"].as<int>(), root["dst"].as<int>());
	return 0;
}

// Format:
/// 1. Date: YYYY-MM-DD
/// 2. Time: hh:mm:ss
/// 3. Date and time: YYYY-MM-DD hh:mm:ss
/// 4. "" or "sync": synchronize with could
int SmartControllerClass::CldSetCurrentTime(String tmStr)
{
	// synchronize with cloud
	tmStr.trim();
	tmStr.toLowerCase();
	if(tmStr.length() == 0 || tmStr == "sync") {
		theConfig.CloudTimeSync();
		return 0;
	}

	US _YYYY = Time.year();
	UC _MM = Time.month();
	UC _DD = Time.day();
	UC _hh = Time.hour();
	UC _mm = Time.minute();
	UC _ss = Time.second();

	int nPos, nPos2, nPos3;
	String strTime = "";
	nPos = tmStr.indexOf('-');
	if( nPos > 0 ) {
		_YYYY = (US)(tmStr.substring(0, nPos).toInt());
		if( _YYYY < 1970 || _YYYY > 2100 ) return 1;
		nPos2 = tmStr.indexOf('-', nPos + 1);
		if( nPos2 < nPos + 1 ) return 2;
		_MM = (UC)(tmStr.substring(nPos + 1, nPos2).toInt());
		if( _MM <= 0 || _MM > 12 ) return 2;
		nPos3 = tmStr.indexOf(' ', nPos2 + 1);
		if( nPos3 < nPos2 + 1 ) {
			_DD = (UC)(tmStr.substring(nPos2 + 1).toInt());
		} else {
			_DD = (UC)(tmStr.substring(nPos2 + 1, nPos3).toInt());
			strTime = tmStr.substring(nPos3 + 1);
		}
		if( _DD <= 0 || _DD > 31 ) return 3;
	} else {
		strTime = tmStr;
	}

	strTime.trim();
	nPos = strTime.indexOf(':');
	if( nPos > 0 ) {
		_hh = (UC)(strTime.substring(0, nPos).toInt());
		if( _hh > 24 ) return 4;
		nPos2 = strTime.indexOf(':', nPos + 1);
		if( nPos2 < nPos + 1 ) return 5;
		_mm = (UC)(strTime.substring(nPos + 1, nPos2).toInt());
		if( _mm > 59 ) return 5;
		_ss = (UC)(strTime.substring(nPos2 + 1).toInt());
		if( _ss > 59 ) return 6;
	}

	time_t myTime = tmConvert_t(_YYYY, _MM, _DD, _hh, _mm, _ss);
	myTime -= theConfig.GetTimeZoneDSTOffset() * 60;	// Convert to local time
	Time.setTime(myTime);
	LOGI(LOGTAG_EVENT, "setTime to %d-%d-%d %d:%d:%d", _YYYY, _MM, _DD, _hh, _mm, _ss);
	return 0;
}

// Format: "dev:sw", where
/// dev = node id, or 0 for all devices, default is 1 (the main lamp)
/// sw = 0/off or 1/on
int SmartControllerClass::CldPowerSwitch(String swStr)
{
	//fast, simple control
	UC bytDev = 1;		// Default value
	BOOL blnOn = false;
	swStr.toLowerCase();
	int nPos = swStr.indexOf(':');
	if( nPos > 0 ) {
		bytDev = (uint8_t)(swStr.substring(0, nPos).toInt());
		if( bytDev > NODEID_MAX_DEVCIE ) return 1;
	}
	String strOn = swStr.substring(nPos + 1);
	blnOn = (strOn == "1" || strOn == "on");
	return DevSoftSwitch(blnOn, bytDev);
}

// Execute Operations, including SerialConsole commands
/// Format: {cmd: '', data: ''}
int SmartControllerClass::CldJSONCommand(String jsonCmd)
{
	SERIAL_LN("Received JSON cmd: %s", jsonCmd.c_str());

	int rc = ProcessJSONString(jsonCmd);
	if (rc < 0) {
		// Error input
		LOGE(LOGTAG_MSG, "Error parsing json cmd: %s", jsonCmd.c_str());
		return 0;
	} else if (rc > 0) {
		// Wait for more...
		return 1;
	}

	if (!(*m_jpCldCmd).containsKey("cmd"))
  {
		LOGE(LOGTAG_MSG, "Error json cmd format: %s", jsonCmd.c_str());
		return 0;
  }

	const COMMAND strCmd = (COMMAND)(*m_jpCldCmd)["cmd"].as<int>();

	//COMMAND 0: Use Serial Interface
	if( strCmd == CMD_SERIAL) {
		// Execute serial port command, and reflect results on cloud variable
		if (!(*m_jpCldCmd).containsKey("data")) {
			LOGE(LOGTAG_MSG, "Error json cmd format: %s", jsonCmd.c_str());
			return 0;
		}

		if( !theConfig.IsCloudSerialEnabled() ) {
			LOGN(LOGTAG_MSG, "Cloud serial command is not allowed. Check system config.");
			return 0;
		}
		const char* strData = (*m_jpCldCmd)["data"];
		theConsole.ExecuteCloudCommand(strData);
	}

	//COMMAND 1: Toggle light switch
	if (strCmd == CMD_POWER) {
		if (!(*m_jpCldCmd).containsKey("node_id") || !(*m_jpCldCmd).containsKey("state")) {
			LOGE(LOGTAG_MSG, "Error json cmd format: %s", jsonCmd.c_str());
			return 0;
		}
		const int node_id = (*m_jpCldCmd)["node_id"].as<int>();
		const int state = (*m_jpCldCmd)["state"].as<int>();

		char buf[64];
		sprintf(buf, "%d;%d;%d;%d;%d;%d", node_id, S_DIMMER, C_SET, 1, V_STATUS, state);
		String strCmd(buf);
		ExecuteLightCommand(strCmd);
	}

	//COMMAND 2: Change light color
	if (strCmd == CMD_COLOR) {
		if (!(*m_jpCldCmd).containsKey("node_id") || !(*m_jpCldCmd).containsKey("ring") || !(*m_jpCldCmd).containsKey("color")) {
			LOGE(LOGTAG_MSG, "Error json cmd format: %s", jsonCmd.c_str());
			return 0;
		}
		const int node_id = (*m_jpCldCmd)["node_id"].as<int>();
		const uint8_t ring = (*m_jpCldCmd)["ring"].as<int>();
		const uint8_t State = (*m_jpCldCmd)["color"][0].as<uint8_t>();
		const uint8_t CW = (*m_jpCldCmd)["color"][1].as<uint8_t>();
		const uint8_t WW = (*m_jpCldCmd)["color"][2].as<uint8_t>();
		const uint8_t R = (*m_jpCldCmd)["color"][3].as<uint8_t>();
		const uint8_t G = (*m_jpCldCmd)["color"][4].as<uint8_t>();
		const uint8_t B = (*m_jpCldCmd)["color"][5].as<uint8_t>();

		String payload = CreateColorPayload(ring, State, CW, WW, R, G, B);

		char buf[64];
		sprintf(buf, "%d;%d;%d;%d;%d;%s", node_id, S_CUSTOM, C_SET, 1, V_VAR1, payload.c_str());
		String strCmd(buf);
		ExecuteLightCommand(strCmd);
	}

	//COMMAND 3: Change brightness
	if (strCmd == CMD_BRIGHTNESS) {
		if (!(*m_jpCldCmd).containsKey("node_id") || !(*m_jpCldCmd).containsKey("value")) {
			LOGE(LOGTAG_MSG, "Error json cmd format: %s", jsonCmd.c_str());
			return 0;
		}
		const int node_id = (*m_jpCldCmd)["node_id"].as<int>();
		const int value = (*m_jpCldCmd)["value"].as<int>();

		char buf[64];
		sprintf(buf, "%d;%d;%d;%d;%d;%d", node_id, S_DIMMER, C_SET, 1, V_DIMMER, value);
		String strCmd(buf);
		ExecuteLightCommand(strCmd);
	}

	//COMMAND 4: Change color with scenerio input
	if (strCmd == CMD_SCENARIO) {
		if (!(*m_jpCldCmd).containsKey("node_id") || !(*m_jpCldCmd).containsKey("SNT_id")) {
			LOGE(LOGTAG_MSG, "Error json cmd format: %s", jsonCmd.c_str());
			return 0;
		}
		const int node_id = (*m_jpCldCmd)["node_id"].as<int>();
		const int SNT_uid = (*m_jpCldCmd)["SNT_id"].as<int>();

		//find hue data of the 3 rings
		ListNode<ScenarioRow_t> *rowptr = SearchScenario(SNT_uid);
		if (rowptr)
		{
			String ring1_payload = CreateColorPayload(1, rowptr->data.ring1.State, rowptr->data.ring1.CW, rowptr->data.ring1.WW, rowptr->data.ring1.R, rowptr->data.ring1.G, rowptr->data.ring1.B);
			String ring2_payload = CreateColorPayload(2, rowptr->data.ring2.State, rowptr->data.ring2.CW, rowptr->data.ring2.WW, rowptr->data.ring2.R, rowptr->data.ring2.G, rowptr->data.ring2.B);
			String ring3_payload = CreateColorPayload(3, rowptr->data.ring3.State, rowptr->data.ring3.CW, rowptr->data.ring3.WW, rowptr->data.ring3.R, rowptr->data.ring3.G, rowptr->data.ring3.B);

			char buf[64];

			sprintf(buf, "%d;%d;%d;%d;%d;%s", node_id, S_CUSTOM, C_SET, 1, V_VAR1, ring1_payload.c_str());
			String ring1_cmd(buf);
			ExecuteLightCommand(ring1_cmd);

			sprintf(buf, "%d;%d;%d;%d;%d;%s", node_id, S_CUSTOM, C_SET, 1, V_VAR1, ring2_payload.c_str());
			String ring2_cmd(buf);
			ExecuteLightCommand(ring2_cmd);

			sprintf(buf, "%d;%d;%d;%d;%d;%s", node_id, S_CUSTOM, C_SET, 1, V_VAR1, ring3_payload.c_str());
			String ring3_cmd(buf);
			ExecuteLightCommand(ring3_cmd);
		}
		else
		{
			LOGE(LOGTAG_MSG, "Could not change node:%d light's color, scenerio %d not found", node_id, SNT_uid);
			return 0;
		}
	}

	return 1;
}

int SmartControllerClass::CldJSONConfig(String jsonData) //future actions
{
  //based on the input (ie whether it is a rule, scenario, or schedule), send the json string(s) to appropriate function.
  //These functions are responsible for adding the item to the respective, appropriate Chain. If multiple json strings coming through,
  //handle each for each respective Chain until end of incoming string

	SERIAL_LN("Received JSON config message: %s", jsonData.c_str());

  int numRows = 0;
  bool bRowsKey = true;

	int rc = ProcessJSONString(jsonData);
	if (rc < 0) {
		// Error input
		LOGE(LOGTAG_MSG, "Error parsing json config message: %s", jsonData.c_str());
		return 0;
	} else if (rc > 0) {
		// Wait for more...
		return 1;
	}

  if (!(*m_jpCldCmd).containsKey("rows"))
  {
    numRows = 1;
    bRowsKey = false;
  }
  else
  {
    numRows = (*m_jpCldCmd)["rows"].as<int>();
  }

  int successCount = 0;
  for (int j = 0; j < numRows; j++)
  {
	  if (!bRowsKey)
	  {
		  if (ParseCmdRow(*m_jpCldCmd))
		  {
			  successCount++;
		  }
	  }
	  else
	  {
		  JsonObject& data = (*m_jpCldCmd)["data"][j];
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
	OP_FLAG op_flag = (OP_FLAG)data["op"].as<int>();
	FLASH_FLAG flash_flag = (FLASH_FLAG)data["fl"].as<int>();
	RUN_FLAG run_flag = (RUN_FLAG)data["run"].as<int>();

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
			row.node_id = data["node_uid"];
			row.SCT_uid = data["SCT_uid"];
			row.SNT_uid = data["SNT_uid"];
			row.notif_uid = data["notif_uid"];

			isSuccess = Change_Rule(row);
			if (!isSuccess)
			{
				LOGE(LOGTAG_MSG, "UID:%s Unable to write row to Rule_t", uidWhole);
				return 0;
			}
			else
			{
				LOGI(LOGTAG_MSG, "UID:%s write row to Rule_t OK", uidWhole);
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
					LOGE(LOGTAG_MSG, "UID:%s Invalid 'weekdays' must between 0 and 7", uidWhole);
					return 0;
				}
			}
			else if (data["isRepeat"] == 0)
			{
				if (data["weekdays"] < 1 || data["weekdays"] > 7)		// [1..7]
				{
					LOGE(LOGTAG_MSG, "UID:%s Invalid 'weekdays' must between 1 and 7", uidWhole);
					return 0;
				}
			}
			else
			{
				LOGE(LOGTAG_MSG, "UID:%s Invalid 'isRepeat' must 0 or 1", uidWhole);
				return 0;
			}

			if (data["hour"] < 0 || data["hour"] > 23)
			{
				LOGE(LOGTAG_MSG, "UID:%s Invalid 'hour' must between 0 and 23", uidWhole);
				return 0;
			}

			if (data["min"] < 0 || data["min"] > 59)
			{
				LOGE(LOGTAG_MSG, "UID:%s Invalid 'min' must between 0 and 59", uidWhole);
				return 0;
			}

			row.weekdays = data["weekdays"];
			row.isRepeat = data["isRepeat"];
			row.hour = data["hour"];
			row.minute = data["min"];
			row.alarm_id = dtINVALID_ALARM_ID;

			isSuccess = Change_Schedule(row);
			if (!isSuccess)
			{
				LOGE(LOGTAG_MSG, "UID:%s Unable to write row to Schedule_t", uidWhole);
				return 0;
			}
			else
			{
				LOGI(LOGTAG_MSG, "UID:%s write row to Schedule_t OK", uidWhole);
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
				LOGE(LOGTAG_MSG, "UID:%s Unable to write row to Scenario_t", uidWhole);
				return 0;
			}
			else
			{
				LOGI(LOGTAG_MSG, "UID:%s write row to Scenario_t OK", uidWhole);
				return 1;
			}
			break;
		}
		case CLS_CONFIGURATION:		// Sys Config
		{
			// ToDo:
			break;
		}
		default:
		{
			LOGE(LOGTAG_MSG, "UID:%s Invalid", uidWhole);
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

bool SmartControllerClass::updateDevStatusRow(MyMessage msg)
{
	//find row to update
	ListNode<DevStatusRow_t> *DevStatusRowPtr = SearchDevStatus(msg.getDestination());
	if (DevStatusRowPtr == NULL)
	{
		LOGE(LOGTAG_MSG, "Error updating DevStatus_Table, didn't find node_id %d", msg.getDestination());
		return false;
	}

	switch (msg.getSensor()) //current possible values are S_CUSTOM and S_DIMMER
	{
	case S_CUSTOM:
		if (msg.getType() == V_VAR1)
		{
			uint64_t payload = msg.getUInt64();

			uint8_t ring_num = (payload >> (8 * 6)) & 0xff;
			Hue_t ring_col;

			ring_col.State = (payload >> (8 * 5)) & 0xff;
			ring_col.CW = (payload >> (8 * 4)) & 0xff;
			ring_col.WW = (payload >> (8 * 3)) & 0xff;
			ring_col.R = (payload >> (8 * 2)) & 0xff;
			ring_col.G = (payload >> (8 * 1)) & 0xff;
			ring_col.B = (payload >> (8 * 0)) & 0xff;

			switch (ring_num)
			{
			case 0: //all rings
				if (ring_col.State == 0) { //if ring_num and state both equal 0, do not re-write colors, only turn rings off
					DevStatusRowPtr->data.ring1.State = 0;
					DevStatusRowPtr->data.ring2.State = 0;
					DevStatusRowPtr->data.ring3.State = 0;
				} else {
					DevStatusRowPtr->data.ring1 = ring_col;
					DevStatusRowPtr->data.ring2 = ring_col;
					DevStatusRowPtr->data.ring3 = ring_col;
				}
				break;
			case 1:
				if (ring_col.State == 0) { //if state=0 don't change colors
					DevStatusRowPtr->data.ring1.State = 0;
				} else {
					DevStatusRowPtr->data.ring1 = ring_col;
				}
				break;
			case 2:
				if (ring_col.State == 0) { //if state=0 don't change colors
					DevStatusRowPtr->data.ring2.State = 0;
				} else {
					DevStatusRowPtr->data.ring2 = ring_col;
				}
				break;
			case 3:
				if (ring_col.State == 0) { //if state=0 don't change colors
					DevStatusRowPtr->data.ring3.State = 0;
				} else {
					DevStatusRowPtr->data.ring3 = ring_col;
				}
				break;
			}
		}
		else
		{
			LOGE(LOGTAG_MSG, "Error updating DevStatus_Table, invalid subtype for S_CUSTOM sensor");
			return false;
		}
		break;

	case S_DIMMER:
		if (msg.getType() == V_STATUS) {
			if (msg.getBool())
			{
				DevStatusRowPtr->data.ring1.State = 1;
				DevStatusRowPtr->data.ring2.State = 1;
				DevStatusRowPtr->data.ring3.State = 1;
			}
			else
			{
				DevStatusRowPtr->data.ring1.State = 0;
				DevStatusRowPtr->data.ring2.State = 0;
				DevStatusRowPtr->data.ring3.State = 0;
			}
		}
		else if (msg.getType() == V_DIMMER) {
			//do nothing, DevStatusRow_t has no brightness attribute to update
		}
		else {
			LOGE(LOGTAG_MSG, "Error updating DevStatus_Table, invalid subtype for S_DIMMER sensor");
			return false;
		}
		break;

	default:
		LOGE(LOGTAG_MSG, "Error updating DevStatus_Table, invalid MyMessage sensor type");
		return false;
	}

	// ToDo: This (below) causes all changed DevStatus rows to be written to memory during SaveConfig().
	// This is very bad for the flash memory in the long run, as the memory will wear down each time the
	// user turns on or off the light
	DevStatusRowPtr->data.flash_flag = UNSAVED; //required
	DevStatusRowPtr->data.run_flag == EXECUTED; //redundant, already should be EXECUTED
	theConfig.SetDSTChanged(true);

	return true;
}

//This function takes in a MyMessage serial input, and sends it to the specified light.
//Upon success, also updates the devstatus table and brightness indicator
bool SmartControllerClass::ExecuteLightCommand(String mySerialStr)
{
	//TESTING SAMPLES
	// Set S_CUSTOM:
	// all rings red: 1;23;1;1;24;1099528339456
	// all rings CW/WW: 1;23;1;1;24;2199006478336
	// all rings off: 1;23;1;1;24;16777215
	// ring 1 red: 1;23;1;1;24;282574505050112
	// Set S_DIMMER, V_STATUS on/off
	// 	1;4;1;1;2;1
	// Set S_DIMMER, V_DIMMER value 0-100
	//  1;4;1;1;3;50

	SERIAL_LN("Attempting to send MyMessage Serial string: %s", mySerialStr.c_str());

	//mysensors serial message
	MyMessage msg;
	if (theRadio.ProcessSend(mySerialStr, msg)) //send message
	{
		SERIAL_LN("Sent message: from:%d dest:%d cmd:%d type:%d sensor:%d payl-len:%d",
			msg.getSender(), msg.getDestination(), msg.getCommand(),
			msg.getType(), msg.getSensor(), msg.getLength());

		if (updateDevStatusRow(msg)) //update devstatus;
		{
			//ToDo: update brightness indicator
			return true;
		}
	}
	return false;
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
			alarm_id = Alarm.alarmRepeat((timeDayOfWeek_t)(int)scheduleRow->data.weekdays, (int)scheduleRow->data.hour, (int)scheduleRow->data.minute, 0, AlarmTimerTriggered);
			Alarm.setAlarmTag(alarm_id, tag);
		}
		else if (scheduleRow->data.weekdays == 0)
		{
			//weekdays == 0 refers to daily repeat
			time_t tmValue = AlarmHMS((int)scheduleRow->data.hour, (int)scheduleRow->data.minute, 0);
			alarm_id = Alarm.alarmRepeat(tmValue, AlarmTimerTriggered);
			Alarm.setAlarmTag(alarm_id, tag);
		}
		else
		{
			LOGN(LOGTAG_MSG, "Cannot create Alarm via UID:%c%d. Incorrect weekday value.", CLS_RULE, tag);
			return false;
		}
	}
	else if (scheduleRow->data.isRepeat == 0) {
		alarm_id = Alarm.alarmOnce((timeDayOfWeek_t)(int)scheduleRow->data.weekdays, (int)scheduleRow->data.hour, (int)scheduleRow->data.minute, 0, AlarmTimerTriggered);
		Alarm.setAlarmTag(alarm_id, tag);
	}
	else {
		LOGN(LOGTAG_MSG, "Cannot create Alarm via UID:%c%d. Incorrect isRepeat value.", CLS_RULE, tag);
		return false;
	}
	//Update that schedule row's alarm_id field with the newly created alarm's alarm_id
	scheduleRow->data.alarm_id = alarm_id;
	LOGI(LOGTAG_MSG, "Alarm %u created via UID:%c%d", alarm_id, CLS_RULE, tag);
	LOGD(LOGTAG_MSG, "Alarm %u creation info:", alarm_id);
	LOGD(LOGTAG_MSG, "-----weekday %d", (int)scheduleRow->data.weekdays);
	LOGD(LOGTAG_MSG, "-----hour %d", (int)scheduleRow->data.hour);
	LOGD(LOGTAG_MSG, "-----min %d", (int)scheduleRow->data.minute);

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
// Table Search Functions (Search chain, then Flash)
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
			theConfig.MemReadScenarioRow(row, MEM_SCENARIOS_OFFSET + uid*SNT_ROW_SIZE);

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

ListNode<DevStatusRow_t>* SmartControllerClass::SearchDevStatus(UC dest_id)
{
	ListNode<DevStatusRow_t> *tmp = DevStatus_table.getRoot();
	while (tmp != NULL)
	{
		if (tmp->data.node_id == dest_id) {
			return tmp;
		}
		tmp = tmp->next;
	}
	//do not need to search in flash because whole table is always loaded

	return NULL;
}

//------------------------------------------------------------------
// Device Operations
//------------------------------------------------------------------
US SmartControllerClass::VerifyDevicePresence(UC _nodeID, UC _devType, uint64_t _identity)
{
	NodeIdRow_t lv_Node;
	// Veirfy identity
	lv_Node.nid = _nodeID;
	if( theConfig.lstNodes.get(&lv_Node) < 0 ) return 0;
	if( isIdentityEmpty(lv_Node.identity) || !isIdentityEqual(lv_Node.identity, &_identity) ) {
		LOGN(LOGTAG_MSG, F("Failed to verify identity for device:%d"), _nodeID);
		return 0;
	}
	// Update timestamp
	lv_Node.recentActive = Time.now();
	theConfig.lstNodes.update(&lv_Node);
	theConfig.SetNIDChanged(true);

	US token = random(65535); // Random number
	if( _nodeID < NODEID_MIN_REMOTE ) {
		// Lamp
		// Search device in Device Status Table, add new item if not exists,
		// since the node has passed verification
		ListNode<DevStatusRow_t> *DevStatusRowPtr = SearchDevStatus(_nodeID);
		if (!DevStatusRowPtr) {
			if( theConfig.InitDevStatus(_nodeID) ) {
				theConfig.SetDSTChanged(true);
				DevStatusRowPtr = SearchDevStatus(_nodeID);
				//ToDo: remove
				SERIAL_LN("Test DevStatus_table2 added item: %d, size: %d", _nodeID, theSys.DevStatus_table.size());
			}
		}
		if(!DevStatusRowPtr) {
			LOGW(LOGTAG_MSG, F("Failed to get item form DST for device:%d"), _nodeID);
			return 0;
		}

		// Update DST item
		DevStatusRowPtr->data.present = 1;
		DevStatusRowPtr->data.type = _devType;
		DevStatusRowPtr->data.token = token;
		DevStatusRowPtr->data.flash_flag = UNSAVED; //required
		DevStatusRowPtr->data.run_flag == EXECUTED; //redundant, already should be EXECUTED
		theConfig.SetDSTChanged(true);
	} else {
		// Remote
		theConfig.m_stMainRemote.node_id = _nodeID;
		theConfig.m_stMainRemote.present = 1;
		theConfig.m_stMainRemote.type = 1;
		theConfig.m_stMainRemote.token = token;
	}

	return token;
}

//------------------------------------------------------------------
// Printing tables/working memory chains
//------------------------------------------------------------------
void SmartControllerClass::print_devStatus_table(int row)
{
	SERIAL_LN("==========================");
	SERIAL_LN("==== DevStatus Row %d ====", row);

	switch (DevStatus_table.get(row).op_flag)
	{
	case 0: SERIAL_LN("op_flag = GET"); break;
	case 1: SERIAL_LN("op_flag = POST"); break;
	case 2: SERIAL_LN("op_flag = PUT"); break;
	case 3: SERIAL_LN("op_flag = DELETE"); break;
	}
	switch (Schedule_table.get(row).flash_flag)
	{
	case 0: SERIAL_LN("flash_flag = UNSAVED"); break;
	case 1: SERIAL_LN("flash_flag = SAVED"); break;
	}
	switch (Schedule_table.get(row).run_flag)
	{
	case 0: SERIAL_LN("run_flag = UNEXECUTED"); break;
	case 1: SERIAL_LN("run_flag = EXECUTED"); break;
	}
	SERIAL_LN("uid = %d, type = %d", DevStatus_table.get(row).uid, DevStatus_table.get(row).type);
	SERIAL_LN("node_id = %d, present = %s", DevStatus_table.get(row).node_id, (DevStatus_table.get(row).present ? "true" : "false"));
	SERIAL_LN("ring1 = %s", hue_to_string(DevStatus_table.get(row).ring1).c_str());
	SERIAL_LN("ring2 = %s", hue_to_string(DevStatus_table.get(row).ring2).c_str());
	SERIAL_LN("ring3 = %s", hue_to_string(DevStatus_table.get(row).ring3).c_str());
}

void SmartControllerClass::print_schedule_table(int row)
{
	SERIAL_LN("====================");
	SERIAL_LN("==== SCT Row %d ====", row);

	switch (Schedule_table.get(row).op_flag)
	{
	case 0: SERIAL_LN("op_flag = GET"); break;
	case 1: SERIAL_LN("op_flag = POST"); break;
	case 2: SERIAL_LN("op_flag = PUT"); break;
	case 3: SERIAL_LN("op_flag = DELETE"); break;
	}
	switch (Schedule_table.get(row).flash_flag)
	{
	case 0: SERIAL_LN("flash_flag = UNSAVED"); break;
	case 1: SERIAL_LN("flash_flag = SAVED"); break;
	}
	switch (Schedule_table.get(row).run_flag)
	{
	case 0: SERIAL_LN("run_flag = UNEXECUTED"); break;
	case 1: SERIAL_LN("run_flag = EXECUTED"); break;
	}
	SERIAL_LN("uid = %d", Schedule_table.get(row).uid);
	SERIAL_LN("weekdays = %d", Schedule_table.get(row).weekdays);
	SERIAL_LN("isRepeat = %s", (Schedule_table.get(row).isRepeat ? "true" : "false"));
	SERIAL_LN("hour = %d", Schedule_table.get(row).hour);
	SERIAL_LN("min = %d", Schedule_table.get(row).minute);
	SERIAL_LN("alarm_id = %u", Schedule_table.get(row).alarm_id);
}

void SmartControllerClass::print_scenario_table(int row)
{
	SERIAL_LN("====================");
	SERIAL_LN("==== SNT	Row %d ====", row);

	switch (Scenario_table.get(row).op_flag)
	{
	case 0: SERIAL_LN("op_flag = GET"); break;
	case 1: SERIAL_LN("op_flag = POST"); break;
	case 2: SERIAL_LN("op_flag = PUT"); break;
	case 3: SERIAL_LN("op_flag = DELETE"); break;
	}
	switch (Scenario_table.get(row).flash_flag)
	{
	case 0: SERIAL_LN("flash_flag = UNSAVED"); break;
	case 1: SERIAL_LN("flash_flag = SAVED"); break;
	}
	switch (Scenario_table.get(row).run_flag)
	{
	case 0: SERIAL_LN("run_flag = UNEXECUTED"); break;
	case 1: SERIAL_LN("run_flag = EXECUTED"); break;
	}
	SERIAL_LN("uid = %d", Scenario_table.get(row).uid);
	SERIAL_LN("ring1 = %s", hue_to_string(Scenario_table.get(row).ring1).c_str());
	SERIAL_LN("ring2 = %s", hue_to_string(Scenario_table.get(row).ring2).c_str());
	SERIAL_LN("ring3 = %s", hue_to_string(Scenario_table.get(row).ring3).c_str());
	SERIAL_LN("filter = %d", Scenario_table.get(row).filter);
}

void SmartControllerClass::print_rule_table(int row)
{
	SERIAL_LN("===================");
	SERIAL_LN("==== RT Row %d ====", row);

	switch (Rule_table.get(row).op_flag)
	{
	case 0: SERIAL_LN("op_flag = GET"); break;
	case 1: SERIAL_LN("op_flag = POST"); break;
	case 2: SERIAL_LN("op_flag = PUT"); break;
	case 3: SERIAL_LN("op_flag = DELETE"); break;
	}
	switch (Rule_table.get(row).flash_flag)
	{
	case 0: SERIAL_LN("flash_flag = UNSAVED"); break;
	case 1: SERIAL_LN("flash_flag = SAVED"); break;
	}
	switch (Rule_table.get(row).run_flag)
	{
	case 0: SERIAL_LN("run_flag = UNEXECUTED"); break;
	case 1: SERIAL_LN("run_flag = EXECUTED"); break;
	}
	SERIAL_LN("uid = %d", Rule_table.get(row).uid);
	SERIAL_LN("node_id = %d", Rule_table.get(row).node_id);
	SERIAL_LN("SCT_uid = %d", Rule_table.get(row).SCT_uid);
	SERIAL_LN("SNT_uid = %d", Rule_table.get(row).SNT_uid);
	SERIAL_LN("notif_uid = %d", Rule_table.get(row).notif_uid);
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

String SmartControllerClass::hue_to_string(Hue_t hue)
{
	String out = "";
	out.concat("State:");
	out.concat(hue.State);
	out.concat("|");
	out.concat("CW:");
	out.concat(hue.CW);
	out.concat("|");
	out.concat("WW:");
	out.concat(hue.WW);
	out.concat("|");
	out.concat("R:");
	out.concat(hue.R);
	out.concat("|");
	out.concat("G:");
	out.concat(hue.G);
	out.concat("|");
	out.concat("B:");
	out.concat(hue.B);

	return out;
}

String SmartControllerClass::CreateColorPayload(uint8_t ring, uint8_t State, uint8_t CW, uint8_t WW, uint8_t R, uint8_t G, uint8_t B)
{
	uint64_t i_payload = (B / 16)* pow(16, 1)  +  (B % 16)* pow(16, 0) +
								 		(G / 16)* pow(16, 3)  +  (G % 16)* pow(16, 2) +
								 		(R / 16)* pow(16, 5)  +  (R % 16)* pow(16, 4) +
								 		(WW / 16)* pow(16, 7)  +  (WW % 16)* pow(16, 6) +
								 		(CW / 16)* pow(16, 9)  +  (CW % 16)* pow(16, 8) +
								 		(State / 16)* pow(16, 11)  +  (State % 16)* pow(16, 10) +
								 		(ring / 16)* pow(16, 13)  +  (ring % 16)* pow(16, 12);

	char buf[21];
	PrintUint64(buf, i_payload, false);
	return String(buf);
}
