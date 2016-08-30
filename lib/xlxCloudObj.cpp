/**
 * xlxCloudObj.cpp - Xlight Cloud Object library
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
 *
 * ToDo:
 * 1.
**/

#include "xlxCloudObj.h"

//------------------------------------------------------------------
// Xlight Cloud Object Class
//------------------------------------------------------------------
CloudObjClass::CloudObjClass()
{
  m_SysID = "";
  m_SysVersion = "";
  m_devStatus = STATUS_OFF;
  m_temperature = 0.0;
  m_humidity = 0.0;
  m_brightness = 0;
  m_jpRoot = &(m_jBuf.createObject());
  m_strCldCmd = "";
}

// Initialize Cloud Variables & Functions
void CloudObjClass::InitCloudObj()
{
  if( m_jpRoot->success() ) {
    (*m_jpRoot)["device"] = m_SysID.c_str();
    m_jpData = &(m_jpRoot->createNestedObject("sensor"));
  } else {
    m_jpData = &(JsonObject::invalid());
  }

#ifdef USE_PARTICLE_CLOUD
  Particle.variable(CLV_SysID, &m_SysID, STRING);
  Particle.variable(CLV_TimeZone, &m_tzString, STRING);
  Particle.variable(CLV_DevStatus, &m_devStatus, INT);
  Particle.variable(CLV_JSONData, &m_jsonData, STRING);
  Particle.variable(CLV_LastMessage, &m_lastMsg, STRING);

  Particle.function(CLF_SetTimeZone, &CloudObjClass::CldSetTimeZone, this);
  Particle.function(CLF_PowerSwitch, &CloudObjClass::CldPowerSwitch, this);
  Particle.function(CLF_JSONCommand, &CloudObjClass::CldJSONCommand, this);
  Particle.function(CLF_JSONConfig, &CloudObjClass::CldJSONConfig, this);
  Particle.function(CLF_SetCurTime, &CloudObjClass::CldSetCurrentTime, this);
#endif
}

String CloudObjClass::GetSysID()
{
	return m_SysID;
}

String CloudObjClass::GetSysVersion()
{
	return m_SysVersion;
}

// Here we can either send/publish data on individual data entry basis,
/// or compose data entries into one larger json string and transfer it in main loop.
BOOL CloudObjClass::UpdateTemperature(float value)
{
  if( m_temperature != value ) {
    m_temperature = value;
    if( m_jpData->success() )
    {
      (*m_jpData)["DHTt"] = value;
    }
    return true;
  }
  return false;
}

BOOL CloudObjClass::UpdateHumidity(float value)
{
  if( m_humidity != value ) {
    m_humidity = value;
    if( m_jpData->success() )
    {
      (*m_jpData)["DHTh"] = value;
    }
    return true;
  }
  return false;
}

BOOL CloudObjClass::UpdateBrightness(uint16_t value)
{
  if( m_brightness != value ) {
    m_brightness = value;
    if( m_jpData->success() )
    {
      (*m_jpData)["ALS"] = value;
    }
    return true;
  }
  return false;
}

BOOL CloudObjClass::UpdateMotion(bool value)
{
  if( m_motion != value ) {
    m_motion = value;
    if( m_jpData->success() )
    {
      (*m_jpData)["PIR"] = value;
    }
    return true;
  }
  return false;
}

// Compose JSON Data String
void CloudObjClass::UpdateJSONData()
{
  if( m_jpData->success() ) {
    char buf[SENSORDATA_JSON_SIZE];
    m_jpData->printTo(buf, SENSORDATA_JSON_SIZE);
    m_jsonData = buf;

    // Publish sensor data
#ifdef USE_PARTICLE_CLOUD
    Particle.publish(CLT_NAME_SensorData, m_jsonData, CLT_TTL_SensorData, PRIVATE);
#endif
  }
}

// Publish LOG message and update cloud veriable
BOOL CloudObjClass::PublishLog(const char *msg)
{
  BOOL rc = true;
  m_lastMsg = msg;
#ifdef USE_PARTICLE_CLOUD
  rc = Particle.publish(CLT_NAME_LOGMSG, msg, CLT_TTL_LOGMSG, PRIVATE);
#endif
  return rc;
}

// Concatenate string with regard to the length limitation of cloud API
/// Return value:
/// 0 - string is intact, can be executed
/// 1 - waiting for more input
/// -1 - error
int CloudObjClass::ProcessJSONString(String inStr)
{
  String strTemp = inStr;
  StaticJsonBuffer<COMMAND_JSON_SIZE * 8> lv_jBuf;
  m_jpCldCmd = &(lv_jBuf.parseObject(const_cast<char*>(strTemp.c_str())));
  if (!m_jpCldCmd->success())
  {
		if( m_strCldCmd.length() > 0 ) {

			m_strCldCmd.concat(inStr);
			String debugstr = m_strCldCmd;
			StaticJsonBuffer<COMMAND_JSON_SIZE*3 * 8> lv_jBuf2; //buffer size must be larger than COMMAND_JSON_SIZE
			m_jpCldCmd = &(lv_jBuf2.parseObject(const_cast<char*>(m_strCldCmd.c_str())));
			m_strCldCmd = "";		// Already concatenated
			if (!m_jpCldCmd->success()) {
				SERIAL_LN("Could not parse the concatenated string: %s", debugstr.c_str());
				return -1;
			}
		} else {
			return -1;
		}
  }

	if (m_jpCldCmd->containsKey("x0") ) {
		// Begin of a new string
		m_strCldCmd = (*m_jpCldCmd)["x0"];
		return 1;
	}
	else if (m_jpCldCmd->containsKey("x1")) {
		// Concatenate
		strTemp = (*m_jpCldCmd)["x1"];
		m_strCldCmd.concat(strTemp);
		return 1;
  } else if( m_strCldCmd.length() > 0 ) {
		m_strCldCmd.concat(inStr);
		String debugstr = m_strCldCmd;
		StaticJsonBuffer<COMMAND_JSON_SIZE * 8> lv_jBuf3;
		m_jpCldCmd = &(lv_jBuf3.parseObject(const_cast<char*>(m_strCldCmd.c_str())));

		m_strCldCmd = "";		// Already concatenated
		if (!m_jpCldCmd->success()) {
			SERIAL_LN("Could not parse the string: %s", debugstr.c_str());
			return -1;
		}
  }

  return 0;
}
