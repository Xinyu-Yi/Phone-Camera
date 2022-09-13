#include "PhoneCamera.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstdio>
#include <unistd.h>
#include <cstring>
#include <cstdlib>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/core/core.hpp>


#define MAX_QUEUE_LEN 4096
#define MAX_IMAGE_SIZE (4 * 1024 * 1024)   // 4MB


static inline void check(int ret_val, const char *err_msg, const int *psocket_need_close=NULL) {
    if (ret_val == -1) {
        perror(err_msg);
        if (psocket_need_close) close(*psocket_need_close);
        exit(-1);
    }
}


PhoneCamera::PhoneCamera(int port) {
    int listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    check(listen_socket, "socket");

    struct sockaddr_in server_addr {
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr = {htonl(INADDR_ANY)}
    };
    struct sockaddr_in client_addr {};
    socklen_t client_addr_length = sizeof(client_addr);
    check(bind(listen_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)), "bind", &listen_socket);
    check(listen(listen_socket, 1), "listen", &listen_socket);

    printf("PhoneCamera started at port %d. Waiting for connection.\n", port);
    client_socket = accept(listen_socket, (struct sockaddr *)&client_addr, &client_addr_length);
    check(client_socket, "accept", &listen_socket);
    printf("Phone connected.\n");

    close(listen_socket);

    // Start the producer thread
    producer_thread = std::thread(&PhoneCamera::run, this);
    is_open = true;
}

PhoneCamera::~PhoneCamera() {
    if (is_open) {
        is_open = false;
        printf("Closing phone connection.\n");
    }
    producer_thread.join();
    check(shutdown(client_socket, SHUT_RDWR), "shutdown", &client_socket);
    close(client_socket);
}

/// <summary>
/// Main function for the producer thread, which receives images and stores them into a buffer.
/// </summary>
void PhoneCamera::run() {
    char *header = new char[22];
    char *raw_image = new char[MAX_IMAGE_SIZE];

    while (is_open) {
        // receive header
        int len = recv(client_socket, header, sizeof(char) * 22, MSG_WAITALL);
        if (len > 0 && !(len == 22 && header[0] == 'b' && header[21] == 'e')) {
            printf("Error: decode header failed\n");
            delete[] header;
            delete[] raw_image;
            close(client_socket);
            exit(-1);
        }
        else if (len == 0) {
            printf("Phone disconnected\n");
            delete[] header;
            delete[] raw_image;
            is_open = false;
            return;
        }
        else if (len < 0) {
            perror("recv");
            delete[] header;
            delete[] raw_image;
            close(client_socket);
            exit(-1);
        }

        // decode header
        header[8] = header[21] = '\0';
        long image_size = strtol(header + 1, NULL, 10);
        double timestamp = strtod(header + 9, NULL);

        // receive image
        len = recv(client_socket, raw_image, image_size, MSG_WAITALL);
        if (len > 0 && len != image_size) {
            printf("Error: decode image failed\n");
            delete[] header;
            delete[] raw_image;
            close(client_socket);
            exit(-1);
        } else if (len == 0) {
            printf("Phone disconnected\n");
            delete[] header;
            delete[] raw_image;
            is_open = false;
            return;
        } else if (len < 0) {
            perror("recv");
            delete[] header;
            delete[] raw_image;
            close(client_socket);
            exit(-1);
        }

        // save image and timestamp
        cv::Mat image = cv::imdecode(cv::_InputArray(raw_image, image_size), cv::IMREAD_COLOR);
        mutex.lock();
        if (image_buf.size() >= MAX_QUEUE_LEN) {
            image_buf.pop();
            timestamp_buf.pop();
        }
        image_buf.push(std::move(image));
        timestamp_buf.push(timestamp);
        // printf("queue len %ld\n", self->image_buf.size());
        mutex.unlock();
    }
}

PhoneCamera &PhoneCamera::operator>>(cv::Mat &output_image) {
    while (isOpened()) {
        mutex.lock();
        if (!image_buf.empty()) {
            output_image = std::move(image_buf.front());
            timestamp = timestamp_buf.front();
            image_buf.pop();
            timestamp_buf.pop();
            mutex.unlock();
            return *this;
        } else {
            mutex.unlock();
            usleep(5000);
        }
    }
    return *this;
}


double PhoneCamera::get(cv::VideoCaptureProperties n) const {
    switch (n) {
        case cv::CAP_PROP_POS_MSEC: return timestamp * 1000;
        default: printf("Error: VideoCaptureProperties not defined\n"); exit(-1);
    }
}

