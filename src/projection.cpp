#include "projection.hpp"

#define PI 3.1415926535897931


double Range_Modulo(double x, double min, double max) {
    const double range = max - min;
    return x - range * floor((x - min) / range);
}

/*double Range_Modulo(double x, double lower_limit, double upper_limit) {
    while (x <= lower_limit) {
        x += (upper_limit - lower_limit);
    }
    while (x > upper_limit) {
        x -= (upper_limit - lower_limit);
    }
    return x;
}*/


void Equirectangular_to_Perspective(cv::Mat* dest, cv::Mat& src, double center_lat, double center_lon, int width, int height, double fov_x, double fov_y, bool debug) {
    double x_norm, y_norm, roh, c, lat, lon;
    int x_src, y_src;

    if (!debug) {
        dest->create(height, width, src.type());
    } else {
        *dest = src.clone();
    }

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {

            x_norm = ((double)x / (width - 1) * 2 - 1) * fov_x;
            y_norm = ((double)y / (height - 1) * 2 - 1) * fov_y;

            roh = sqrt(x_norm * x_norm + y_norm * y_norm);
            c = atan(roh);

            lon = asin(cos(c) * sin(center_lon) + (y_norm * sin(c) * cos(center_lon)) / roh);
            lat = center_lat + atan2(x_norm * sin(c), roh * cos(center_lon) * cos(c) - y_norm * sin(center_lon) * sin(c));

            lat = Range_Modulo(lat, -PI / 2.0, PI / 2.0);
            lon = Range_Modulo(lon, -PI, PI);

            lon = (lon / (PI / 2.0) + 1.0) * 0.5;
            lat = (lat / PI + 1.0) * 0.5;

            x_src = (int) floor(lat * src.cols);
            y_src = (int) floor(lon * src.rows);

            if (x_src < 0 || x_src >= src.cols || y_src < 0 || y_src >= src.rows) {
                printf("Pixel out of bounds: x_src = %d, y_src = %d\n", x_src, y_src);
                return;
            }

            if (!debug) {
                dest->at<cv::Vec3b>(y, x) = src.at<cv::Vec3b>(y_src, x_src);
            } else {
                dest->at<cv::Vec3b>(y_src, x_src) = cv::Vec3b(0, 0, 255);
            }
        }
    }
}