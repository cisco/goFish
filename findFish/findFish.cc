#include <iostream>
#include <fstream>
#include <dirent.h>
#include <thread>
#include <map>
#include <vector>
#include <csignal> 

#include "resources/includes/JsonBuilder.h"
#include "resources/includes/EventDetector.h"

// Hoping to move the OpenCV out of this file.

/* Included in EventDetector.h */
#include <opencv2/opencv.hpp>
#include <opencv2/objdetect.hpp>
#include <opencv2/imgcodecs.hpp>
/*******************************/



#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <opencv2/tracking.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/xfeatures2d.hpp>

using namespace std;
using namespace cv;

// Directory to save JSON config video_files to.
#define JSON_DIR "static/video-info/"
#define VIDEO_DIR "static/videos/"

#define TRACKING
#define THREADED

//#undef TRACKING
//#undef THREADED

void HandleSignal(int);
vector<string> GetVideosFromDir(string, vector<string>);
void ProcessVideo(string);

int main()
{
    signal(SIGABRT, HandleSignal);


    useOptimized();
    setUseOptimized(true);

    // Keep the application running to continuously check for more files
    do
    {
        vector<string> json_filters = {".json", ".JSON"};
        auto json_files = GetVideosFromDir(JSON_DIR, json_filters);
        vector<string> vid_filters = {".mp4", ".MP4"};
        auto video_files = GetVideosFromDir(VIDEO_DIR, vid_filters);

        // This looks pretty heavy (and is), so consider refining this to reduce CPU processing.
        for(int i = 0; i < json_files.size(); i++)
        {
            string jf = json_files[i].substr(json_files[i].find("DE_")+3, json_files[i].length());
            jf = jf.substr(0, jf.find_last_of("."));

            for(auto it = video_files.begin(); it != video_files.end(); ++it)
                if((*it).find(jf) != string::npos)
                {
                   // cout << "\"" << (*it) << "\" has already been processed!"<<endl;
                    video_files.erase(it);
                    break;
                }
        }

        #ifdef THREADED

        vector<thread> threads;
        threads.resize(video_files.size());
        for(int i = 0; i < video_files.size(); i++)
            threads[i] = thread(ProcessVideo, video_files[i]);

        for(int i = 0; i < threads.size(); i++)
            threads[i].join();

        #else

        for(int i = 0; i < video_files.size(); i++)
            ProcessVideo(video_files[i]);

        #endif

    } while(true); // Could be helpful to add some event notifier to help close this more gracefully.
    
    return 0;
}

void HandleSignal(int signal)
{
    cout << endl << "Got signal: " << signal <<endl;
    exit(signal);
}

vector<string> GetVideosFromDir(string dir, vector<string> filters)
{
    vector<string> video_files;
    DIR *dp;
	struct dirent *d;
	if ((dp = opendir(dir.c_str())) != NULL)
	{
		while ((d = readdir(dp)) != NULL)
            for(auto filter : filters)
                if ((strcmp(string(d->d_name).c_str(), ".") != false &&
                     strcmp(string(d->d_name).c_str(), "..") != false) &&
                     string(d->d_name).find(filter) != string::npos)
                    video_files.push_back(dir + d->d_name);
        closedir(dp);
	}
    return video_files;
}

void ProcessVideo(string file)
{
    string vidFileName = file;
    // Open video.
    VideoCapture cap(vidFileName);
    if (!cap.isOpened()) {
        cerr << "*** Could not open video file: \"" << vidFileName << "\"" << endl;
        return;
    }

    // Clean up the video file name.
    {
        // Now that we've opened the file, scrub it clean of directory names to get the tag name.
        vidFileName = vidFileName.substr(vidFileName.find_last_of("/")+1, vidFileName.length());
        
        // Strip the uneeded V_ from the video.
        if(vidFileName.find("V_") != string::npos)
            vidFileName = vidFileName.substr(vidFileName.find_last_of("V_")+1, vidFileName.length());
    }

    // Get the total number of frames in the video.
    auto totalFrames = cap.get(cv::CAP_PROP_FRAME_COUNT);
    cout << "=== Processing \"" << vidFileName << "\" ===" << endl;
    cout << "  > Total Frames: " << totalFrames << endl;


    // Create the JSON object which will hold all of the detected events.
    JSON DE("DetectedEvents");

    QREvent* detectQR       = nullptr;
    ActivityEvent* activity = nullptr;

    #ifdef TRACKING
    Mat mask;
    Ptr<BackgroundSubtractor> bkgd_sub_ptr = createBackgroundSubtractorKNN();
    #endif

    int currentFrame = 0;
    int currentEvent = 1;
    while(true)
    {
        // Read in the first frame of the video.
        Mat frame;
        cap.read(frame);

        // If the frame is empty, we simply skip this one and move on.
        // TODO: GoPro videos seem to have lots of these due to large amounts of meta-data. This might be the only workaround for now
        if (frame.empty()) continue;

        currentFrame++;
        if (currentFrame >= totalFrames) break;

        // Gets the first QR code it sees, then stops checking once it has one.
        if(!detectQR || !detectQR->DetectedQR())
        {
            detectQR = new QREvent(frame);
            detectQR->StartEvent(currentFrame);
            DE.AddObject(detectQR->GetAsJSON());
        }

       

        #ifdef TRACKING
        // Masking to find contours of moving objects.
        {
            bkgd_sub_ptr->apply(frame, mask);
            

            int morph_elem = 0;
            int morph_size = 10, erode_size = 3;
            Mat kernel = getStructuringElement( morph_elem, Size( 2*erode_size + 1, 2*erode_size+1 ), Point( erode_size, erode_size ) );
            Mat element = getStructuringElement( morph_elem, Size( 2*morph_size + 1, 2*morph_size+1 ), Point( morph_size, morph_size ) );

            erode(mask, mask, kernel, Point(2, 2));
            dilate(mask, mask, element, Point(1, 1));
            morphologyEx( mask, mask, MORPH_CLOSE, element);

            threshold(mask, mask, 250, 255, 3);

            int thresh = 2000;
            RNG rng(12345);

            Mat canny_output;
            vector<vector<Point>> contours;
            vector<Vec4i> hierarchy;

            // Detect edges using canny
            Canny( mask, canny_output, thresh, thresh*2, 5);
            findContours( canny_output, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_TC89_KCOS, Point(0, 0) );

            Mat drawing = Mat::zeros( canny_output.size(), CV_8UC3 );
            for( int i = 0; i< contours.size(); i++ )
            {
                Scalar color = Scalar( rng.uniform(0, 255), rng.uniform(0,255), rng.uniform(0,255) );
                drawContours( frame, contours, i, color, 2, 8, hierarchy, 0, Point() );
            }

             // TODO: This is test code to make sure that Event activity is added to JSON correctly. This should be moved and modified once
            // fish detection is working.
            if(!contours.empty())
            {
                if(!activity)
                { 
                    activity = new ActivityEvent(frame, currentEvent);
                    activity->StartEvent(currentFrame);
                    cout << "Starting Event" << endl;
                }
            }
            else
            {
                if(activity)
                {
                    activity->SetIsActive(false);
                    activity->EndEvent(currentFrame);
                    DE.AddObject(activity->GetAsJSON());
                    cout<< "Added event"<<endl;
                    delete activity;
                    activity = nullptr;
                    currentEvent++;
                }
            }

            //imshow("mask", mask);
            //imshow( "Contours", frame );
        }
        
        // Feature matching test.
        {
            /*
            int minHessian = 400;
            Ptr<xfeatures2d::SURF> detector = xfeatures2d::SURF::create(minHessian);

            vector<KeyPoint> keypoints_1, keypoints_2;

            Mat ref_image;
            ref_image = imread("static/FishImages/ZebraCichlid2.png");   
            Mat descriptors1, descriptors2;

            detector->detectAndCompute(ref_image, noArray(), keypoints_1, descriptors1 );
            detector->detectAndCompute(frame, noArray(), keypoints_2, descriptors2 );

            Ptr<DescriptorMatcher> matcher = DescriptorMatcher::create(DescriptorMatcher::FLANNBASED);
            vector<vector<DMatch>> knn_matches;
            matcher->knnMatch( descriptors1, descriptors2, knn_matches, 100 );
          
            const float ratio_thresh = 0.5f;
            std::vector<DMatch> good_matches;
            for (size_t i = 0; i < knn_matches.size(); i++)
                if (knn_matches[i][0].distance < ratio_thresh * knn_matches[i][1].distance)
                    good_matches.push_back(knn_matches[i][0]);

            Mat img_matches;
            drawMatches(ref_image, keypoints_1, frame, keypoints_2, good_matches, img_matches, Scalar::all(-1),
                        Scalar::all(-1), std::vector<char>(), DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS );
            */
            /* 
                vector<KeyPoint> keypoints;
                detector->detect(frame, keypoints);

                Mat im_with_keypoints;
                drawKeypoints(frame, keypoints, im_with_keypoints, Scalar(0,0,255), DrawMatchesFlags::DRAW_RICH_KEYPOINTS );
                
                
                /// Find contours   
                vector<vector<Point> > contours;
                vector<Vec4i> hierarchy;
                RNG rng(12345);
                findContours( mask, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0, 0) );
                /// Draw contours
                Mat drawing = Mat::zeros( mask.size(), CV_8UC3 );
                for( int i = 0; i< contours.size(); i++ )
                {
                    Scalar color = Scalar( rng.uniform(0, 255), rng.uniform(0,255), rng.uniform(0,255) );
                    drawContours( drawing, contours, i, color, 2, 8, hierarchy, 0, Point() );
                }     
            */ 
        }
        

        // Concatenates videos together (could be useful for combining videos together in this app rather than the browser).
        {
            /*
            int rows = max(frame.rows, frame.rows);
            int cols = frame.cols + frame.cols;

            Mat3b conc(rows, cols, Vec3b(0,0,0));
            Mat3b res(rows, cols, Vec3b(0,0,0));

            frame.copyTo(res(Rect(0,0,frame.cols, frame.rows)));
            frame.copyTo(res(Rect(frame.cols,0,frame.cols, frame.rows)));

            imshow("Detection", res);
            */
        }

        char c = (char)waitKey(25);
        if(c==27)
            break;

        #endif
    }
    if(activity)
    {
        activity->SetIsActive(false);
        activity->EndEvent(currentFrame);
        DE.AddObject(activity->GetAsJSON());
        cout<< "Added event"<<endl;
        delete activity;
        activity = nullptr;
        currentEvent++;
    }
    cout << "=== Finished processing \"" << vidFileName << "\" ===" << endl;
    
    cap.release();
    destroyAllWindows();

    // Construct the JSON object array of all events detected.
    DE.BuildJSONObjectArray();

    // Create the JSON file for this video.
    ofstream configFile;
    configFile.open (JSON_DIR "DE_" + vidFileName.substr(0, vidFileName.find_last_of(".")) + ".json");
    configFile << DE.GetJSON();
    configFile.close();

    // Cleanup pointers
    if(detectQR) 
    {
        delete detectQR;
        detectQR = nullptr;
    }
   
    if(activity)
    {
        delete activity;
        activity = nullptr;
    }
}

