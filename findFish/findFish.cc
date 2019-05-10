
#include <opencv2/opencv.hpp>
#include <opencv2/objdetect.hpp>
#include <opencv2/imgcodecs.hpp>

#include <iostream>

using namespace std;
using namespace cv;

int main()
{
    String vidFileName("video-359182640.mp4");
    VideoCapture cap(vidFileName);
    if (!cap.isOpened()) {
        cerr << "Could not open video file: " << vidFileName << endl;
        return -1;
    }

    auto numFrames = cap.get(cv::CAP_PROP_FRAME_COUNT);
    cout << "numFrames: " << numFrames << endl;

    QRCodeDetector qrDetector;

    int frameNum = 0;
    while (1) {
        Mat frame;
        cap.read(frame);

        if (frame.empty()) {
            // don't know what is up with goPro but sometimes it makes empty frames
            cout << "  Weird empty frame " << endl;
            continue; // skip frame
        }

        frameNum++;
        if (frameNum >= numFrames) {
            break;
        }

        cout << "Doing frame " << frameNum << endl;

        Mat boundBox;
        String url = qrDetector.detectAndDecode(frame, boundBox);
        if (url.length() > 0) {
            cout << "  Found: " << url << " at << "
                 << boundBox.at<float>(0, 0) << ","
                 << boundBox.at<float>(0, 1) << endl;
        }
    }

    cap.release(); // free up the video memory

    destroyAllWindows();
    return 0;
}
