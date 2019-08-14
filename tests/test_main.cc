#include <cppunit/TextTestRunner.h>
#include "test_tracker.h"
#include "test_processor.h"
#include "test_json.h"
#include "test_events.h"
#include "test_calibration.h"

using namespace CppUnit;

int main()
{
   TextTestRunner runner;
   runner.addTest(CalibrationTest::suite());
   runner.addTest(JSONTest::suite());
   runner.addTest(EventTest::suite());
   runner.addTest(TrackerTest::suite());
   runner.addTest(ProcessorTest::suite());
   runner.run();
   
   return 0;
}