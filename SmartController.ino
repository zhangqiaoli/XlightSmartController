//#define UNIT_TEST_ENABLE //toggle unit testing

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

#ifdef UNIT_TEST_ENABLE
	#include "test.ino"
#else

//------------------------------------------------------------------
// Include dependency packages below
//------------------------------------------------------------------
#include "application.h"
#include "xlxConfig.h"
#include "xlSmartController.h"
#include "xlxSerialConsole.h"
#include "SparkIntervalTimer.h"

//------------------------------------------------------------------
// Program Body Begins Here
//------------------------------------------------------------------

// SINGLETONS:
// theSys: SmartControllerClass
// theConfig: ConfigClass
// theConsole: SerialConsoleClass
// theLog: LoggerClass
// theRadio: RF24ServerClass

// Define hardware IntervalTimer
IntervalTimer sysTimer;

void SysteTimerCB()
{
	static UC fastTick = 0;				// must be static

  // Change Status Indicator according to system status
  // e.g: fast blink, slow blink, breath, etc
  // ToDo:

  // ToDo: MIC input (tone detection)

  // High speed non-block process
	if (++fastTick > RTE_TICK_FASTPROCESS) {
		fastTick = 0;
  	theSys.FastProcess();
	}
}

void setup()
{
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

	// Initiaze Cloud Variables & Functions
	///It is fine to call this function when the cloud is disconnected - Objects will be registered next time the cloud is connected
  theSys.InitCloudObj();

	// Wait the system started
	while( millis() < 5000 ) {
		Particle.process();
	}

  // Initialization network Interfaces
  theSys.InitNetwork();

  // Initialize Sensors
  theSys.InitSensors();

	// Initialize Serial Console
  theConsole.Init();

  // System Starts
  theSys.Start();
}

// Notes: approximate RTE_DELAY_SELFCHECK ms per loop
/// If you need more accurate and faster timer, do it with sysTimer
void loop()
{
	static UL lastTick = millis();
  static UC tick = 0;

  // Process commands
  IF_MAINLOOP_TIMER( theSys.ProcessCommands(), "ProcessCommands" );

  // Collect data
	if( millis() - lastTick >= 1000 ) {
		lastTick = millis();
  	IF_MAINLOOP_TIMER( theSys.CollectData(tick++), "CollectData" );
	}

	// Act on new Rules in Rules chain
	IF_MAINLOOP_TIMER( theSys.ReadNewRules(), "ReadNewRules" );

  // ToDo: transfer data

  // ToDo: status synchronize

  // Self-test & alarm trigger, also insert delay between each loop
  IF_MAINLOOP_TIMER( theSys.SelfCheck(RTE_DELAY_SELFCHECK), "SelfCheck" );

}

#endif
