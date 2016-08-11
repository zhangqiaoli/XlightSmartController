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
  //String in ="";
  /// Format_1: Single row
  theSys.CldJSONConfig("{'op':1, 'fl':0, 'run':0, 'uid':'s1','ring1':[1,8,8,8,8,8], 'ring2':[1,8,8,8,8,8], 'ring3':[1,8,8,8,8,8], 'filter':0}");
  theSys.CldJSONCmd("{'cmd':'serial', 'data':'?'}");
  theSys.CldJSONCmd("{'cmd':'serial', 'data':'? show'}");
  theSys.CldJSONCmd("{'cmd':'serial', 'data':'check rf'}");
  theSys.CldJSONCmd("{'cmd':'serial', 'data':'check wifi'}");
  theSys.CldJSONCmd("{'cmd':'serial', 'data':'show debug'}");
  theSys.CldJSONCmd("{'cmd':'serial', 'data':'show net'}");
  theSys.CldJSONCmd("{'cmd':'serial', 'data':'ping'}");
  theSys.CldJSONCmd("{'cmd':'serial', 'data':'sys reset'}");

  /// Format_2: Multiple rows
  //theSys.CldJSONConfig("{'rows':2, 'data': [{'op':1, 'fl':0, 'run':0, 'uid':'s1', 'hue':0x0101080808080800}, {'op':1, 'fl':0, 'run':0, 'uid':'s1', 'hue':0x0201080808080800}, {'op':1, 'fl':0, 'run':0, 'uid':'s1', 'hue':0x0301080808080800}]");

  /// Format_3: Concatenate strings
  //// First string
  theSys.CldJSONConfig("{'x0':'{\"op\":1, \"fl\":0, \"run\":0, \"uid\":\"s1\",\"ring1\":[1,8,8,8,8,8], '}");
  //// Strings in middle
  theSys.CldJSONConfig("{'x1': '\"ring2\":[1,8,8,8,8,8], \"ring3\":[1,8,8,8,8,8], '}");
  theSys.CldJSONConfig("\"filter\":0}");
  //// Last string: same as Format_1 or Format_2

  //// Format_3 test case
  theSys.CldJSONCmd("{'x0': '{\"cmd\":\"serial\", '}");
  theSys.CldJSONCmd("{'x1': '\"data\":\"check '}");
  theSys.CldJSONCmd("wifi\"}");
  //// Format_3 test case
  theSys.CldJSONCmd("{'x0': ' '}");
  theSys.CldJSONCmd("{'cmd':'serial', 'data':'show net'}");
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
