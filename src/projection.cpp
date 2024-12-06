#include "projection.hpp"

#define PI 3.1415926535897931


struct Point {
    int x;
    int y;
};


struct Projection {
    struct Point** map;
    int width;
    int height;
};


double Range_Modulo(double x, double lower_limit, double upper_limit) {
    while (x <= lower_limit) {
        x += (upper_limit - lower_limit);
    }
    while (x > upper_limit) {
        x -= (upper_limit - lower_limit);
    }
    return x;
}


Projection_t Init_Projection(int width_src, int height_src, double center_lat, double center_lon, int width, int height, double fov_x, double fov_y) {
    int x, y, x_src, y_src;
    double x_norm, y_norm, roh, c, lat, lon;

    Projection_t projec = (Projection_t) malloc(sizeof(struct Projection));
    if (projec == NULL) {
        printf("Failed to allocate memory for Projection_t\n");
        return NULL;
    }

    projec->width = width;
    projec->height = height;

    projec->map = (struct Point**) malloc(height * sizeof(struct Point*));
    if (projec->map == NULL) {
        printf("Failed to allocate memory for projec->map\n");
        return NULL;
    }

    for (int i = 0; i < height; ++i) {
        projec->map[i] = (struct Point*) malloc(width * sizeof(struct Point));
        if (projec->map[i] == NULL) {
            printf("Failed to allocate memory for projec->map[%d]\n", i);
            return NULL;
        }
    }

    for (y = 0; y < height; ++y) {
        for (x = 0; x < width; ++x) {

            x_norm = ((double)x / (width - 1) * 2 - 1) * fov_x;
            y_norm = ((double)y / (height - 1) * 2 - 1) * fov_y;

            roh = sqrt(x_norm * x_norm + y_norm * y_norm);
            c = atan(roh);

            lon = asin(cos(c) * sin(center_lon) + (y_norm * sin(c) * cos(center_lon)) / roh);
            lat = center_lat + atan2(x_norm * sin(c), roh * cos(center_lon) * cos(c) - y_norm * sin(center_lon) * sin(c));

            /*lat = Range_Modulo(lat, -PI / 2.0, PI / 2.0);*/
            lon = Range_Modulo(lon, -PI, PI);

            lon = (lon / (PI / 2.0) + 1.0) * 0.5;
            lat = (lat / PI + 1.0) * 0.5;

            x_src = (int) floor(lat * width_src);
            y_src = (int) floor(lon * height_src);

            if (x_src < 0 || x_src >= width_src || y_src < 0 || y_src >= height_src) {
                printf("Pixel out of bounds: x_src = %d, y_src = %d\n", x_src, y_src);
                projec->map[y][x] = {-1, -1};
            } else {
                projec->map[y][x].y = y_src;
                projec->map[y][x].x = x_src;
            }
        }
    }

    return projec;
}


void Equirectangular_to_Perspective(Projection_t projec, cv::Mat* dest, cv::Mat& src, bool debug) {
    int x_src, y_src, x, y;

    if (!debug) {
        dest->create(projec->height, projec->width, src.type());
    } else {
        *dest = src.clone();
    }

    for (y = 0; y < projec->height; ++y) {
        for (x = 0; x < projec->width; ++x) {
            x_src = projec->map[y][x].x;
            y_src = projec->map[y][x].y;

            if (x_src >= 0 && y_src >= 0) {
                if (!debug) {
                    dest->at<cv::Vec3b>(y, x) = src.at<cv::Vec3b>(y_src, x_src);
                } else {
                    dest->at<cv::Vec3b>(y_src, x_src) = cv::Vec3b(0, 0, 255);
                }
            }
        }
    }
}


void Free_Projection(Projection_t* projec) {
    for (int i = 0; i < (*projec)->height; ++i) {
        free((*projec)->map[i]);
    }
    free((*projec)->map);
    free((*projec));
    *projec = NULL;
}


void Equirectangular_to_Perspective(cv::Mat* dest, cv::Mat& src, double center_lat, double center_lon, int width, int height, double fov_x, double fov_y, bool debug) {
    double x_norm, y_norm, roh, c, lat, lon;
    int x_src, y_src, x, y;

    if (!debug) {
        dest->create(height, width, src.type());
    } else {
        *dest = src.clone();
    }

    for (y = 0; y < height; ++y) {
        for (x = 0; x < width; ++x) {

            x_norm = ((double)x / (width - 1) * 2 - 1) * fov_x;
            y_norm = ((double)y / (height - 1) * 2 - 1) * fov_y;

            roh = sqrt(x_norm * x_norm + y_norm * y_norm);
            c = atan(roh);

            lon = asin(cos(c) * sin(center_lon) + (y_norm * sin(c) * cos(center_lon)) / roh);
            lat = center_lat + atan2(x_norm * sin(c), roh * cos(center_lon) * cos(c) - y_norm * sin(center_lon) * sin(c));

            /*lat = Range_Modulo(lat, -PI / 2.0, PI / 2.0);*/
            lon = Range_Modulo(lon, -PI, PI);

            lon = (lon / (PI / 2.0) + 1.0) * 0.5;
            lat = (lat / PI + 1.0) * 0.5;

            x_src = (int) floor(lat * src.cols);
            y_src = (int) floor(lon * src.rows);

            if (x_src < 0 || x_src >= src.cols || y_src < 0 || y_src >= src.rows) {
                printf("Pixel out of bounds: x_src = %d, y_src = %d\n", x_src, y_src);
                continue;
            }

            if (!debug) {
                dest->at<cv::Vec3b>(y, x) = src.at<cv::Vec3b>(y_src, x_src);
            } else {
                dest->at<cv::Vec3b>(y_src, x_src) = cv::Vec3b(0, 0, 255);
            }
        }
    }
}