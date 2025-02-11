#ifndef HIFICODE_H
#define HIFICODE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <string.h>


#define FNAME "hificode.wav"
#define FL     500
#define FH    5000
#define FS   44100
#define T     1.05
#define HIFI     1
#define STEREO   1
#define DELAY    0
#define FADE     1
#define DIFFR    1
#define BSPL     1
#define CONTRAST 2
#define C_ALLOC(number, type) ((type *) calloc((number), sizeof(type)))
#define TwoPi 6.283185307179586476925287
#define Pi    3.141592653589793238462643
#define HIST  ((1+HIFI)*(1+STEREO))
#define M     64
#define N    176

#define PERSON 1
#define BICYCLE 2
#define VEHICLE 3
#define MOTORBIKE 4

#define LOG_TAG "MyNativeCode"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)


void stereo(double A[M][N]);

void binaural(double A[M][N], double B[M][N]);

double rnd(void);

void newStereo(double A[M][N], char *path);
void newBinaural(double A[M][N], double B[M][N], char *path);

#ifdef __cplusplus
}
#endif

#endif
