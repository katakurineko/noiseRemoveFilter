#ifndef INCLUDE_BITMAP_IMAGE
#define INCLUDE_BITMAP_IMAGE

#include<stdio.h>
#include<stdlib.h>
#include<Windows.h>
#include <math.h>

#include "MyErrorCodes.h"
#include "MyBool.h"
#include "macroDefine.h"

/*�摜�̏��*/
typedef struct {
	unsigned long width;
	long height;
	unsigned short bitCount;
	unsigned long compression;
	char* pImageData;
}BitmapImage;

/*�m�C�Y�t�B���^�̎��*/
typedef enum {
	MEAN, /*���ϒl�@*/
	MEDIAN /*�����l�@*/
}MyNoiseReductionFilterType;

BitmapImage* bitmapImage();
void BitmapImageRelease(BitmapImage** ppBitmapImage);
MY_ERROR_CODE BitmapImageLoad(BitmapImage* pBitmapImage, FILE* pFile);
MY_ERROR_CODE BitmapImageTo8bitBitmapFile(BitmapImage* p8bitBitmapImage, FILE* pOutPutFile);
BitmapImage* BitmapImageReduceNoiseFilter(BitmapImage* pInputImg, MyNoiseReductionFilterType filterType, unsigned long kernelSize);

#endif