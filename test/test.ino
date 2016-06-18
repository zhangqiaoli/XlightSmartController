#ifdef UNIT_TEST_ENABLE

#include "application.h"
#include "unitTest.h"

#include "SparkIntervalTimer.h"

#include "xlSmartController.h"
#include "xliCommon.h"
#include "xliMemoryMap.h"
#include "xliPinMap.h"
#include "xlxCloudObj.h"
#include "xlxConfig.h"
#include "xlxInterfaceRF24.h"
#include "xlxLogger.h"

test(example)
{
  int a =3;
  int b =3;
  assertEqual(a, b);
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
	Test::exclude("*");
	Test::include("SmartControllerClass_*");

	//Additional Setup
	IntervalTimer sysTimer;
	theSys.Init();
	theConfig.LoadConfig();
	theSys.InitPins();
	theSys.InitRadio();
	theSys.InitNetwork();
	theSys.InitCloudObj();
	theSys.InitSensors();
	theSys.Start();
  }

  void loop()
  {
    if (flag)
      Test::run();
  }

#endif
