/**
 * This is the firmware of Xlight SmartController based on Photon/P1 MCU.
 * There are two Protocol Domains:
 * 1. MySensor Protocol: Radio network and BLE communication
 * 2. MQTT Protocol: LAN & WAN
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
 * 1. Setup program skeleton
 * 2. Define major interfaces
 * 3. Define basic data structures
 * 4. Define fundamental status machine
 * 5. Define primary messages
 * 6. Define cloud variables and Functions
 *
 * ToDo:
 * 1. Include MySensor lib
 * 2. Include MQTT lib
**/

//------------------------------------------------------------------
// Include dependency packages below
//------------------------------------------------------------------
#include "application.h"
#include "xlxConfig.h"
#include "xlSmartController.h"
#include "SparkIntervalTimer.h"

//------------------------------------------------------------------
// System level working constants
//------------------------------------------------------------------
// Running Time Environment Parameters
#define RTE_DELAY_PUBLISH         500
#define RTE_DELAY_SYSTIMER        50          // System Timer interval, can be very fast, e.g. 50 means 25ms
#define RTE_DELAY_SELFCHECK       1000        // Self-check interval

//------------------------------------------------------------------
// Program Body Begins Here
//------------------------------------------------------------------
// Define hardware IntervalTimer
IntervalTimer sysTimer;

void SysteTimerCB()
{
  // Change Status Indicator according to system status
  // e.g: fast blink, slow blink, breath, etc
  // ToDo:

  // ToDo: MIC input (tone detection)

  // High speed non-block process
  theSys.FastProcess();
}

void setup()
{
  //theSys: SmartControllerClass
  //theConfig: ConfigClass
  //sysTimer: IntervalTimer (Spark)

  // System Initialization
  theSys.Init();

  // Load Configuration
  theConfig.LoadConfig();

  // Initialize Pins
  theSys.InitPins();

  // Start system timer: callback every n * 0.5ms using hmSec timescale
  //Use TIMER6 to retain PWM capabilities on all pins
  sysTimer.begin(SysteTimerCB, RTE_DELAY_SYSTIMER, hmSec, TIMER6);

  // Initialization Radio Interfaces
  theSys.InitRadio();

  // Initialization network Interfaces
  theSys.InitNetwork();

  // Initiaze Cloud Variables & Functions
  theSys.InitCloudObj();

  // Initialize Sensors
  theSys.InitSensors();

  // System Starts
  theSys.Start();
}

// Notes: approximate RTE_DELAY_SELFCHECK ms per loop
/// If you need more accurate and faster timer, do it with sysTimer
void loop()
{
  static UC tick = 0;

  // Process commands
  theSys.ProcessCommands();

  // Collect data
  theSys.CollectData(tick++);

  // ToDo: transfer data

  // ToDo: status synchronize

  // Self-test & alarm trigger, also insert delay between each loop
  theSys.SelfCheck(RTE_DELAY_SELFCHECK);
}
