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
  m_devStatus = STATUS_OFF;
  m_temperature = 0.0;
  m_humidity = 0.0;
  m_brightness = 0;
  m_jpRoot = &(m_jBuf.createObject());
  if( m_jpRoot->success() ) {
    (*m_jpRoot)["device"] = m_SysID.c_str();
    m_jpData = &(m_jpRoot->createNestedObject("sensor"));
  } else {
    m_jpData = &(JsonObject::invalid());
  }
}

// Initialize Cloud Variables & Functions
void CloudObjClass::InitCloudObj()
{
#ifdef USE_PARTICLE_CLOUD
  Particle.variable(CLV_SysID, &m_SysID, STRING);
  Particle.variable(CLV_TimeZone, &m_tzString, STRING);
  Particle.variable(CLV_DevStatus, &m_devStatus, INT);
  Particle.variable(CLV_JSONData, &m_jsonData, STRING);
  Particle.variable(CLV_LastMessage, &m_lastMsg, STRING);

  Particle.function(CLF_SetTimeZone, &CloudObjClass::CldSetTimeZone, this);
  Particle.function(CLF_PowerSwitch, &CloudObjClass::CldPowerSwitch, this);
  Particle.function(CLF_JSONCommand, &CloudObjClass::CldJSONCommand, this);
#endif
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
