/**
 * xlxLogger.cpp - Xlight logger library for application data logging
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
 * 1. Define basic interfaces
 * 2. Serial logging
 *
 * ToDo:
 * 1. syslog, refer to psyslog.cpp
 * 2. http/cloud
 * 3. Flash
 * 4. Offline Data Cache in Flash (loop overwrite)
**/

#include "xlxLogger.h"
#include "xlSmartController.h"

// the one and only instance of LoggerClass
LoggerClass theLog = LoggerClass();

//------------------------------------------------------------------
// Xlight Logger Class
//------------------------------------------------------------------
LoggerClass::LoggerClass()
{
  m_level[LOGDEST_SERIAL] = LEVEL_DEBUG;
  m_level[LOGDEST_FLASH] = LEVEL_WARNING;
  m_level[LOGDEST_SYSLOG] = LEVEL_INFO;
  m_level[LOGDEST_CLOUD] = LEVEL_NOTICE;
}

void LoggerClass::Init(String sysid)
{
  m_SysID = sysid;
}

BOOL LoggerClass::InitSerial(UL speed)
{
  Serial.begin(speed);
  return true;
}

BOOL LoggerClass::InitFlash(UL addr, UL size)
{
  //ToDo:
  return true;
}

BOOL LoggerClass::InitSysLog(String host, US port)
{
  //ToDo:
  return true;
}

BOOL LoggerClass::InitCloud(String url, String uid, String key)
{
  //ToDo:
  return true;
}

UC LoggerClass::GetLevel(UC logDest)
{
  if( logDest < LOGDEST_DUMMY)
    return m_level[logDest];

  return LEVEL_EMERGENCY;
}

void LoggerClass::SetLevel(UC logDest, UC logLevel)
{
  if( logDest < LOGDEST_DUMMY)
  {
    if( m_level[logDest] != logLevel )
      m_level[logDest] = logDest;
  }
}

void LoggerClass::WriteLog(UC level, const char *tag, const char *msg, ...)
{
  int nSize = 0;
  char buf[MAX_MESSAGE_LEN+2];

  // Prepare message
  int nPos = snprintf(buf, MAX_MESSAGE_LEN - 1, "%s %02d:%02d:%02d %d %s",
      m_SysID.c_str(), Time.hour(), Time.minute(), Time.second(), level, tag);
  va_list args;
  va_start(args, msg);
  nSize = vsnprintf(buf + nPos, MAX_MESSAGE_LEN - nPos, msg, args);

  // Send message to serial port
  if( level <= m_level[LOGDEST_SERIAL] )
  {
    buf[nSize] = '\r';
    buf[nSize+1] = '\n';
    Serial.println(buf);
  }

  // Output Log to Particle cloud variable
  theSys.m_lastMsg = buf;

  // ToDo: send log to other destinations
  //buf[nSize] = NULL;
}
