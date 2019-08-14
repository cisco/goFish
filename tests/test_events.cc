#include "test_events.h"
#include "JsonBuilder.h"

void EventTest::setUp()
{
    _event = std::make_unique<QREvent>();
    _event = std::make_unique<ActivityEvent>(0, -1, -1);
}

void EventTest::TestConstructors()
{
    auto f = [this](EventBuilder* e){ 
        _event.reset(e);
    };
    
    f(new QREvent());
    f(new ActivityEvent(0, -1, -1));
}

void EventTest::TestStartEvent()
{
    auto f = [this](EventBuilder* e, int i){ 
        _event.reset(e);
        _event->StartEvent(i);
    };
    
    f(new QREvent(), 0);
    CPPUNIT_ASSERT_EQUAL(0, _event->GetRange().first);
    CPPUNIT_ASSERT_EQUAL(-1, _event->GetRange().second);

    f(new ActivityEvent(0, -1, -1), 0);
    CPPUNIT_ASSERT_EQUAL(0, _event->GetRange().first);
    CPPUNIT_ASSERT_EQUAL(-1, _event->GetRange().second);
}

void EventTest::TestCheckFrame()
{
    auto f = [this](EventBuilder* e, cv::Mat& m, int i){ 
        _event.reset(e);
        _event->CheckFrame(m, i);
    };
    
    auto mat = cv::Mat();
    f(new QREvent(), mat, 0);
    f(new ActivityEvent(0, -1, -1), mat, 0);
}

void EventTest::TestEndEvent()
{
    auto f = [this](EventBuilder* e, int i, int j){ 
        _event.reset(e);
        _event->StartEvent(i);
        _event->EndEvent(j);
    };
    
    f(new QREvent(), 0, 10);
    CPPUNIT_ASSERT_EQUAL(0, _event->GetRange().first);
    CPPUNIT_ASSERT_EQUAL(10, _event->GetRange().second);

    f(new ActivityEvent(0, -1, -1), 0, 10);
    CPPUNIT_ASSERT_EQUAL(0, _event->GetRange().first);
    CPPUNIT_ASSERT_EQUAL(10, _event->GetRange().second);
}

void EventTest::TestGetAsJSON()
{
    auto f = [this](EventBuilder* e, int i, int j){ 
        _event.reset(e);
        _event->StartEvent(i);
        _event->EndEvent(j);
    };
    
    f(new QREvent(), 0, 10);
    CPPUNIT_ASSERT_EQUAL(std::string("{}"), _event->GetAsJSON().GetJSON());

    f(new ActivityEvent(0, -1, -1), 0, 10);
    CPPUNIT_ASSERT_EQUAL(std::string("{\"Event_Activity_0\":{\"frame_end\":10,\"frame_start\":0}}"), _event->GetAsJSON().GetJSON());
}