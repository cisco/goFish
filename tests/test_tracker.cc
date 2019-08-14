#include "test_tracker.h"


void TrackerTest::setUp()
{
    Tracker::Settings config;
    config.bDrawContours = false;
    config.MinThreshold = 200;
    _tracker = std::make_unique<Tracker>(config);
}

void TrackerTest::TestConstructor()
{
    Tracker::Settings config;
    config.bDrawContours = false;
    config.MinThreshold = 200;
    Tracker* t = new Tracker(config);
    
    _tracker.reset(t);
}

void TrackerTest::TestCreateMask()
{
    // TODO: Add a test image / video.
    cv::Mat mat = cv::imread("");
    _tracker->CreateMask(mat);
}

void TrackerTest::TestGetObjectContours()
{
    // TODO: Add a test image / video.
    cv::Mat mat = cv::imread("");
    _tracker->CreateMask(mat);
    _tracker->GetObjectContours(mat);
}

void TrackerTest::TestCheckForActivity()
{
    int i = 0;
    _tracker->CheckForActivity(i);
}

void TrackerTest::TestGetCascades()
{
    _tracker->GetCascades();
}

