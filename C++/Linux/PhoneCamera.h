#ifndef PHONECAMERA_H
#define PHONECAMERA_H

#include <queue>
#include <thread>
#include <mutex>
#include <opencv2/videoio.hpp>


class PhoneCamera {
public:
    /// <summary>
    /// Start a PhoneCamera receiver at a specific port.
    /// The constructor will be blocked till the connection is established.
    /// </summary>
    /// <param name="port">The server port.</param>
    explicit PhoneCamera(int port);

    ~PhoneCamera();

    /// <summary>
    /// Whether the connection is opened.
    /// </summary>
    bool isOpened() const { return is_open; }

    /// <summary>
    /// Get the timestamp in milliseconds with get(cv::CAP_PROP_POS_MSEC).
    /// Currently, only cv::CAP_PROP_POS_MSEC is implemented.
    /// </summary>
    double get(cv::VideoCaptureProperties n) const;

    /// <summary>
    /// Read the next image (may be blocked).
    /// When the connection is closed, this function is undefined.
    /// </summary>
    /// <param name="output_image">Arg to save the returned image.</param>
    PhoneCamera &operator>>(cv::Mat &output_image);

private:
    void run();

private:
    int client_socket = -1;
    double timestamp = 0;
    bool is_open = false;

    std::thread producer_thread;
    std::mutex mutex;
    std::queue<cv::Mat> image_buf;
    std::queue<double> timestamp_buf;
};


#endif // PHONECAMERA_H
