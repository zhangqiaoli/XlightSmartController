#ifdef UNIT_TEST_ENABLE

#include "application.h"
#include "unitTest.h"

#include "SparkIntervalTimer.h"

#include "xlSmartController.h"
#include "xliCommon.h"
#include "xliMemoryMap.h"
#include "xliPinMap.h"
#include "xliConfig.h"

#include "xlxCloudObj.h"
#include "xlxConfig.h"
#include "xlxLogger.h"
#include "xlxSerialConsole.h"

test(example)
{
  theSys.CldJSONCommand("{\"op_flag\":1, \"flash_flag\":0, \"run_flag\":0, \"uid\":\"s1\", \"ring1\":[8,8,8,8,8], \"ring2\":[8,8,8,8,8], \"ring3\":[8,8,8,8,8], \"filter\":0}");
}

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
// CloudObjClass Tests
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>


//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
// SmartControllerClass Tests
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>


//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
// ConfigClass Tests
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>


//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
// RF24InterfaceClass Tests
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>


//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
// LoggerClass Tests
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>


//><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><>
// Intergration Tests
//><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><>


//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
// Call Start Func to Init Tests
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
  bool flag;

  int start(String input)
  {
    flag = true;
    return 1;
  }

  void setup()
  {
    Particle.function("RunUnitTests", start);
    flag = false;
    Serial.begin(9600);

	//Test output location
	Test::out = &Serial;

	//Test Selection (use ::exclude(char *pattern) and ::include(char *pattern))
	//Test::exclude("*");
	//Test::include("SmartControllerClass_*");

	//Additional Setup
  for(int i = 10; i > 0; i--)
  {
    Serial.println(i);
    delay(500);
  }
  Serial.println ("starting setup functions");
	//IntervalTimer sysTimer;
	theSys.Init();
	theConfig.LoadConfig();
	theSys.InitPins();
	theSys.InitRadio();
	theSys.InitNetwork();
	theSys.InitCloudObj();
	theSys.InitSensors();
  theConsole.Init();
	theSys.Start();
	while (Time.now() < 2000) {
		Particle.process();
	}
  }

  void loop()
  {
    //Serial.print (".");
    static UC tick = 0;

    IF_MAINLOOP_TIMER( theSys.ProcessCommands(), "ProcessCommands" );

    IF_MAINLOOP_TIMER( theSys.CollectData(tick++), "CollectData" );

  	IF_MAINLOOP_TIMER( theSys.ReadNewRules(), "ReadNewRules" );

    IF_MAINLOOP_TIMER( theSys.SelfCheck(RTE_DELAY_SELFCHECK), "SelfCheck" );

    if (flag)
      Test::run();
  }

#endif
