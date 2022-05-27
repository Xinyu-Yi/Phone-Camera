#include "../core/PhoneCamera.h"
#include "opencv2/highgui.hpp"
#include <iostream>
#include <fstream>


using namespace std;


int main() {
    cout << "Press A to start recording, B to stop recording, C to take a photo, Q to quit." << endl;

    PhoneCamera phoneCamera("8888");
    cv::Mat image;
    double timestamp = 0;

    bool recording = false;
    bool running = true;
    bool take_photo = false;
    
    const int fourcc = cv::VideoWriter::fourcc('X', 'V', 'I', 'D');
    
    int video_id = 0;
    int pic_id = 0;
    cv::VideoWriter output_video;
    ofstream output_timestamp;


    while (phoneCamera.isOpen() && running) {
        phoneCamera >> image;
        timestamp = phoneCamera.get(cv::CAP_PROP_POS_MSEC);
        cv::imshow("image", image);
        int c = cv::waitKey(1);

        switch (c) {
            case 'A': case 'a': recording = true; break;
            case 'B': case 'b': recording = false; break;
            case 'C': case 'c': take_photo = true; break;
            case 'Q': case 'q': running = false; break;
            default: break;
        }

        if (recording) {
            if (!output_video.isOpened()) {
                output_video.open(to_string(video_id) + ".mp4", fourcc, 30, cv::Size(image.size[1], image.size[0]));
                if (!output_video.isOpened()) {
                    cerr << "cannot create video file" <<endl;
                    exit(1);
                }

                output_timestamp.open(to_string(video_id) + "_timestamp.txt");
                if (!output_timestamp.is_open()) {
                    cerr << "cannot create timestamp file" << endl;
                    exit(1);
                }

                cout << "recording ..." << endl;
                video_id++;
            }
            output_video << image;
            output_timestamp << timestamp << "\n";
        }
        else {
            if (output_video.isOpened()) {
                output_video.release();
                output_timestamp.close();
                cout << "save video/timestamp to " << video_id - 1 << ".mp4" << endl;
            }
        }

        if (take_photo) {
            string file = to_string(pic_id++) + ".png";
            cv::imwrite(file, image);
            cout << "save pic to " << file << endl;
            take_photo = false;
        }
    }
    if (output_video.isOpened()) {
        output_video.release();
        output_timestamp.close();
        cout << "save video/timestamp to " << video_id - 1 << ".mp4" << endl;
    }
    return 0;
}