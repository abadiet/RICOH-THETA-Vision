#include "projection.h"

#define PI 3.1415926535897931


struct Point {
	unsigned int x;
	unsigned int y;
    bool error;
};


struct Projection {
	struct Point** map;
	unsigned int width;
	unsigned int height;
	unsigned int channels;
	unsigned int width_src;
	unsigned int height_src;
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


Projection_t Init_Projection(unsigned int width_src, unsigned int height_src, unsigned int channels, double center_lat, double center_lon, unsigned int width, unsigned int height, double fov_x, double fov_y) {
	unsigned int i, x, y, x_src, y_src;
	double x_norm, y_norm, roh, c, lat, lon;

	Projection_t projec = (Projection_t) malloc(sizeof(struct Projection));
	if (projec == NULL) {
		printf("Failed to allocate memory for Projection_t\n");
		return NULL;
	}

	projec->width = width;
	projec->height = height;
	projec->channels = channels;
	projec->width_src = width_src;
	projec->height_src = height_src;

	projec->map = (struct Point**) malloc((size_t) height * sizeof(struct Point*));
	if (projec->map == NULL) {
		printf("Failed to allocate memory for projec->map\n");
		return NULL;
	}

	for (i = 0; i < height; ++i) {
		projec->map[i] = (struct Point*) malloc((size_t) width * sizeof(struct Point));
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

			x_src = (unsigned int) floor(lat * width_src);
			y_src = (unsigned int) floor(lon * height_src);

			if (x_src >= width_src || y_src >= height_src) {
				printf("Pixel out of bounds: x_src = %d, y_src = %d\n", x_src, y_src);
				projec->map[y][x].error = true;
			} else {
				projec->map[y][x].y = y_src;
				projec->map[y][x].x = x_src;
				projec->map[y][x].error = false;
			}
		}
	}

	return projec;
}


size_t Equirectangular_to_Perspective(Projection_t projec, uint8_t** dst, uint8_t* src, bool debug) {
	unsigned int x_src, y_src, x, y, c;

	if (!debug) {
		*dst = (uint8_t*) malloc((size_t) (projec->channels * projec->width * projec->height) * sizeof(uint8_t));
		if (*dst == NULL) {
			printf("Failed to allocate memory for dst\n");
			return 0;
		}
	} else {
		*dst = (uint8_t*) malloc((size_t) (projec->channels * projec->width_src * projec->height_src) * sizeof(uint8_t));
		*dst = (uint8_t*) memcpy(*dst, src, (size_t) (projec->channels * projec->width_src * projec->height_src) * sizeof(uint8_t));
		if (*dst == NULL) {
			printf("Failed to allocate memory for dst\n");
			return 0;
		}
	}

	for (y = 0; y < projec->height; ++y) {
		for (x = 0; x < projec->width; ++x) {
			x_src = projec->map[y][x].x;
			y_src = projec->map[y][x].y;

			if (!projec->map[y][x].error) {
				if (!debug) {
					for (c = 0; c < projec->channels; c++) {
						(*dst)[y * projec->width * projec->channels + x * projec->channels + c] = src[y_src * projec->width_src * projec->channels + x_src * projec->channels + c];
					}
				} else {
					for (c = 0; c < projec->channels - 1; c++) {
						(*dst)[y_src * projec->width_src * projec->channels + x_src * projec->channels + c] = 0;
					}
					(*dst)[y_src * projec->width_src * projec->channels + x_src * projec->channels + projec->channels - 1] = 255;
				}
			}
		}
	}

	if (debug) {
		return (size_t) (projec->channels * projec->width_src * projec->height_src) * sizeof(uint8_t);
	}
	return (size_t) (projec->channels * projec->width * projec->height) * sizeof(uint8_t);
}


void Free_Projection(Projection_t* projec) {
    unsigned int i;

	for (i = 0; i < (*projec)->height; ++i) {
		free((*projec)->map[i]);
	}
	free((*projec)->map);
	free((*projec));
	*projec = NULL;
}
