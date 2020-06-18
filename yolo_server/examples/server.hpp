#ifndef SERVER_HPP
#define SERVER_HPP

#include <iostream>
#include <vector>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#define RECO_W_OFFSET 80
#define RECO_H_OFFSET 20

union charint {
    char b[4];
    int i;
};

union chardouble {
    char b[8];
    double d;
};
union charfloat {
    char b[4];
    float f;
};

struct frameBuffer {
    int frmID;
    int dataType;
    double latitude;
    double longtitude;
    double timeCaptured;
    double timeSend;
    int bufferSize;
    char* buffer;
};

struct resBuffer {
    charint resID;
    charint resType;
    chardouble resLatitude;
    chardouble resLongtitude;
    charint markerNum;
    char* buffer;
};

struct recognizedMarker {
    charint markerID;
    charint height, width;
    cv::Point2f corners[4];
    std::string markername;
};

#endif
