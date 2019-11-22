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

/*�G�b�W���o�̎��*/
typedef enum {
	PREWITT, /*�v�����[�E�B�b�g�@*/
	SOBEL, /*�\�[�x���@*/
	LOG /*�K�E�X�̃��v���V�A���@*/
}MyEdgeFilterType;

/*�m�C�Y�t�B���^�̎��*/
typedef enum {
	MEAN, /*���ϒl�@*/
	MEDIAN /*�����l�@*/
}MyNoiseReductionFilterType;

BitmapImage* bitmapImage();
void BitmapImageRelease(BitmapImage** ppBitmapImage);
MY_ERROR_CODE BitmapImage_load(BitmapImage* pBitmapImage, FILE* pFile);
MY_ERROR_CODE BitmapImage_to8bitBitmapFile(BitmapImage* p8bitBitmapImage, FILE* pOutPutFile);
BitmapImage* BitmapImage_EdgeDetecter(BitmapImage* pInputImg, MyEdgeFilterType filterType);
BitmapImage* BitmapImage_ReduceNoiseFilter(BitmapImage* pInputImg, MyNoiseReductionFilterType filterType, unsigned long kernelSize);

#endif