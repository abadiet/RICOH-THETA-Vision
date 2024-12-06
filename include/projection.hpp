#ifndef PROJECTION_HPP
#define PROJECTION_HPP

#include <stdio.h>
#include <stdlib.h>
#include <opencv2/opencv.hpp>


typedef struct Projection* Projection_t;


Projection_t Init_Projection(int width_src, int height_src, double center_lat, double center_lon, int width, int height, double fov_x, double fov_y);

void Equirectangular_to_Perspective(Projection_t projec, cv::Mat* dest, cv::Mat& src, bool debug);

void Free_Projection(Projection_t* projec);

void Equirectangular_to_Perspective(cv::Mat* dest, cv::Mat& src, double center_lat, double center_lon, int width, int height, double fov_x, double fov_y, bool debug);



#endif