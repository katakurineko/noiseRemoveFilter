#ifndef INCLUDE_BITMAP_IMAGE
#define INCLUDE_BITMAP_IMAGE

#include<stdio.h>
#include<stdlib.h>
#include<Windows.h>
#include <math.h>

#include "MyErrorCodes.h"
#include "MyBool.h"
#include "macroDefine.h"

typedef struct {
	unsigned long width;
	long height;
	unsigned short bitCount;
	unsigned long compression;
	char* pImageData;
}BitmapImage;

/*エッジ検出の種類*/
typedef enum {
	PREWITT, /*プリューウィット法*/
	SOBEL, /*ソーベル法*/
	LOG /*ガウスのラプラシアン法*/
}MyEdgeFilterType;

/*ノイズフィルタの種類*/
typedef enum {
	MEAN, /*平均値法*/
	MEDIAN /*中央値法*/
}MyNoiseReductionFilterType;

BitmapImage* bitmapImage();
void BitmapImageRelease(BitmapImage** ppBitmapImage);
MY_ERROR_CODE BitmapImage_load(BitmapImage* pBitmapImage, FILE* pFile);
MY_ERROR_CODE BitmapImage_to8bitBitmapFile(BitmapImage* p8bitBitmapImage, FILE* pOutPutFile);
BitmapImage* BitmapImage_EdgeDetecter(BitmapImage* pInputImg, MyEdgeFilterType filterType);
BitmapImage* BitmapImage_ReduceNoiseFilter(BitmapImage* pInputImg, MyNoiseReductionFilterType filterType, unsigned long kernelSize);

#endif