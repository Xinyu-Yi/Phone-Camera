#include "PhoneCamera.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/core/core.hpp>

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")  


#define MAX_QUEUE_LEN 4096
#define MAX_IMAGE_SIZE (4 * 1024 * 1024)   // 4MB


PhoneCamera::PhoneCamera(const char *port) {
    SOCKET listen_socket = INVALID_SOCKET;
    struct addrinfo *result = NULL;
    struct addrinfo hints;
    WSADATA wsadata;
    int e;  // return value for errors

    // Initialize socket
    e = WSAStartup(MAKEWORD(2, 2), &wsadata);
    if (e != 0) {
        printf("WSAStartup failed with error: %d\n", e);
        exit(1);
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the server address and port
    e = getaddrinfo(NULL, port, &hints, &result);
    if (e != 0) {
        printf("getaddrinfo failed with error: %d\n", e);
        WSACleanup();
        exit(1);
    }

    // Create a SOCKET for connecting to server
    listen_socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (listen_socket == INVALID_SOCKET) {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        exit(1);
    }

    // Setup the TCP listening socket
    e = bind(listen_socket, result->ai_addr, (int)result->ai_addrlen);
    if (e == SOCKET_ERROR) {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(listen_socket);
        WSACleanup();
        exit(1);
    }

    freeaddrinfo(result);

    e = listen(listen_socket, SOMAXCONN);
    if (e == SOCKET_ERROR) {
        printf("listen failed with error: %d\n", WSAGetLastError());
        closesocket(listen_socket);
        WSACleanup();
        exit(1);
    }

    printf("PhoneCamera started at port %s. Waiting for connection.\n", port);

    // Accept a client socket
    client_socket = accept(listen_socket, NULL, NULL);
    if (client_socket == INVALID_SOCKET) {
        printf("accept failed with error: %d\n", WSAGetLastError());
        closesocket(listen_socket);
        WSACleanup();
        exit(1);
    }

    printf("Phone connected.\n");

    // No longer need server socket
    closesocket(listen_socket);

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

    int e = shutdown(client_socket, SD_BOTH);
    if (e == SOCKET_ERROR) {
        printf("shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(client_socket);
        WSACleanup();
        exit(1);
    }

    closesocket(client_socket);
    WSACleanup();
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
            printf("decode header failed\n");
            delete[] header;
            delete[] raw_image;
            closesocket(client_socket);
            WSACleanup();
            exit(1);
        }
        else if (len == 0) {
            printf("Phone disconnected\n");
            delete[] header;
            delete[] raw_image;
            is_open = false;
            return;
        }
        else if (len < 0) {
            printf("recv failed with error: %d\n", WSAGetLastError());
            delete[] header;
            delete[] raw_image;
            closesocket(client_socket);
            WSACleanup();
            exit(1);
        }

        // decode header
        header[8] = header[21] = '\0';
        long image_size = strtol(header + 1, NULL, 10);
        double timestamp = strtod(header + 9, NULL);

        // receive image
        len = recv(client_socket, raw_image, image_size, MSG_WAITALL);
        if (len > 0 && len != image_size) {
            printf("decode image failed\n");
            delete[] header;
            delete[] raw_image;
            closesocket(client_socket);
            WSACleanup();
            exit(1);
        } else if (len == 0) {
            printf("Phone disconnected\n");
            delete[] header;
            delete[] raw_image;
            is_open = false;
            return;
        } else if (len < 0) {
            printf("recv failed with error: %d\n", WSAGetLastError());
            delete[] header;
            delete[] raw_image;
            closesocket(client_socket);
            WSACleanup();
            exit(1);
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
    while (isOpen()) {
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
            Sleep(5);
        }
    }
    return *this;
}


double PhoneCamera::get(cv::VideoCaptureProperties n) const {
    switch (n) {
        case cv::CAP_PROP_POS_MSEC: return timestamp * 1000;
        default: printf("VideoCaptureProperties not defined\n"); exit(1);
    }
}
