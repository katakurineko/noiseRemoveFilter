#ifndef INCLUDE_BITMAP_IMAGE
#define INCLUDE_BITMAP_IMAGE

#include<stdio.h>
#include<stdlib.h>
#include<Windows.h>
#include <math.h>

#include "MyErrorCodes.h"
#include "MyBool.h"
#include "macroDefine.h"

/*画像の情報*/
typedef struct {
	unsigned long width;
	long height;
	unsigned short bitCount;
	unsigned long compression;
	char* pImageData;
}BitmapImage;

/*ノイズフィルタの種類*/
typedef enum {
	MEAN, /*平均値法*/
	MEDIAN /*中央値法*/
}MyNoiseReductionFilterType;

BitmapImage* bitmapImage();
void BitmapImageRelease(BitmapImage** ppBitmapImage);
MY_ERROR_CODE BitmapImageLoad(BitmapImage* pBitmapImage, FILE* pFile);
MY_ERROR_CODE BitmapImageTo8bitBitmapFile(BitmapImage* p8bitBitmapImage, FILE* pOutPutFile);
BitmapImage* BitmapImageReduceNoiseFilter(BitmapImage* pInputImg, MyNoiseReductionFilterType filterType, unsigned long kernelSize);

#endif