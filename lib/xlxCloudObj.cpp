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
