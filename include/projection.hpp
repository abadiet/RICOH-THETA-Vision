#ifndef PROJECTION_HPP
#define PROJECTION_HPP

#include <stdio.h>
#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>


void Equirectangular_to_Perspective(cv::Mat* dest, cv::Mat& src, double center_lat, double center_lon, int width, int height, double R, bool debug);


#endif