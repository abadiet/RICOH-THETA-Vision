#include "projection.hpp"

#define PI 3.1415926535897931


double Range_Modulo(double x, double lower_limit, double upper_limit) {
    while (x <= lower_limit) {
        x += (upper_limit - lower_limit);
    }
    while (x > upper_limit) {
        x -= (upper_limit - lower_limit);
    }
    return x;
}


void Equirectangular_to_Perspective(cv::Mat* dest, cv::Mat& src, double center_lat, double center_lon, int width, int height, double R, bool debug) {
    int x, y, x_rela, y_rela, equ_x, equ_y;
    double lat, lon, lat_norm, lon_norm;

    if (!debug) {
	    *dest = cv::Mat(height, width, src.type());
    } else {
	    *dest = src.clone();
    }

    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {

            /* relative position to the perspective center */
            x_rela = x - width / 2;
            y_rela = y - height / 2;

            /* perspective to spherical */
            lat = Range_Modulo(center_lat + atan(x_rela / R), -PI, PI);
            lon = Range_Modulo(center_lon + atan(y_rela / R), -PI / 2.0, PI / 2.0);

            /* spherical to equirectangular */
            lat_norm = (lat + PI) / (2.0 * PI);
            lon_norm = (lon + PI / 2.0) / PI;
            equ_x = (int) (lat_norm * (double) src.cols);
            equ_y = (int) (lon_norm * (double) src.rows);

            if (equ_x >= 0 && equ_x < src.cols && equ_y >= 0 && equ_y < src.rows) {
                if (!debug) {
                    dest->at<cv::Vec3b>(y, x) = src.at<cv::Vec3b>(equ_y, equ_x);
                } else {
                    dest->at<cv::Vec3b>(equ_y, equ_x) = cv::Vec3b(0, 0, 255);
                }
            } else {
                if (!debug) {
                    dest->at<cv::Vec3b>(y, x) = cv::Vec3b(0, 0, 255);
                }
                printf("Pixel out of range: perspective(%d, %d), spherical(%f, %f), spherical(norm)(%f, %f), equirectangular(%d, %d)\n", x, y, lat, lon, lat_norm, lon_norm, equ_x, equ_y);
            }

        }
    }
}