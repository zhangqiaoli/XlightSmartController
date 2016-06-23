/**
 * xlxSerialConsole.cpp - Xlight Device Management Console via Serial Port
 * This module offers monitoring, testing, device control and setting features
 * in both interactive mode and command line mode.
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
 * 1. Instruction output
 * 2. Select mode: Interactive mode or Command line
 * 3. In interactive mode, navigate menu and follow the screen instruction
 * 4. In command line mode, input the command and press enter. Need user manual
 * 5.
 *
 * Command category:
 * - show: system info, status, variable, statistics, etc.
 * - set: log level, flags, system settings, communication parameters, etc.
 * - test: generate test message or simulate user operation
 * - do: execute command or function, e.g. control the lights, send a message, etc.
 * - sys: system level operation, like reset, recover, safe mode, etc.
 *
 * ToDo:
 * 1.
 *
**/

#include "xlxSerialConsole.h"

#ifndef SERIAL
#define SERAIL        Serial.printf
#endif

#ifndef SERIAL_LN
#define SERAIL_LN     Serial.printlnf
#endif

SerialConsoleClass::SerialConsoleClass()
{

}
