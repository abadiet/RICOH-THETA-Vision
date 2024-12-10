#ifndef PROJECTION_HPP
#define PROJECTION_HPP

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>


#if defined(__cplusplus)
extern "C" {
#endif


typedef struct Projection* Projection_t;


Projection_t Init_Projection(unsigned int width_src, unsigned int height_src, unsigned int channels, double center_lat, double center_lon, unsigned int width, unsigned int height, double fov_x, double fov_y);

size_t Equirectangular_to_Perspective(Projection_t projec, uint8_t** dst, uint8_t* src, bool debug);

void Free_Projection(Projection_t* projec);


#if defined(__cplusplus)
}
#endif

#endif