#include "test_processor.h"

void ProcessorTest::setUp()
{
    _proc = std::make_unique<Processor>();
}

void ProcessorTest::TestConstructor()
{
    auto p = new Processor("../static/videos/", "../static/videos/");
    _proc.reset(p);
}

void ProcessorTest::TestProcessVideo()
{
    _proc->ProcessVideos();
}

void ProcessorTest::TestTriangulatePoints()
{
    _proc->TriangulatePoints("../calib_config/measure_points.yaml", "../calib_config/stereo_calibration.yaml");
}
