//
// Created by Ads on 2017/1/15.
//

#ifndef UTILS_H
#define UTILS_H

#include "Common.h"
#include <opencv2/opencv.hpp>
#include "include/System.h"
#include "Plane.h"
#include <GLES/gl.h>
#include "Point.h"

struct Detected {
    std::string name;
    float prob;
    int left;
    int right;
    int top;
    int bot;
    double tcap;
    double tsend;
};

void addTextToImage(const string &s, cv::Mat &im, const int r, const int g, const int b);
void printStatus(const int &status, cv::Mat &im);
void drawTrackedPoints(vector<Point> &pointcors, cv::Mat &mat);
void drawDetectedObjects(Detected detecteds[], cv::Mat &im);
Plane* detectPlane(const cv::Mat Tcw, const std::vector<ORB_SLAM2::MapPoint*> &vMPs, const int iterations);
void initProjectionMatrix(int w, int h, double fu, double fv, double u0, double v0, double zNear, double zFar ,float projectionMatrix[]);

void getColMajorMatrixFromMat(float M[],cv::Mat &img);





#endif //ORB_SLAM_AR_PLANE_H
