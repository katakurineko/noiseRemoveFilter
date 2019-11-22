#define _CRT_SECURE_NO_WARNINGS
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#define new  ::new(_NORMAL_BLOCK, __FILE__, __LINE__)

#include"BitmapImage.h"
#include "MyErrorCodes.h"
#include "commonFunctions.h"

/*�ϊ�����摜�̎w��*/
#define INPUT_FILE_NAME "test.bmp"

int main() {

	int ret = 0;

	MY_ERROR_CODE error = SUCCESS;

	FILE* pInputFile = NULL;
	FILE* pOutputFile = NULL;
	BitmapImage* pOutputBitmapImage = NULL;
	BitmapImage* p8bitBitmapImage = NULL;

	/*�C���v�b�g�t�@�C���ƃA�E�g�v�b�g�t�@�C���̏���*/
	char inputFileName[] = INPUT_FILE_NAME;
	char* outputFileName = strJoin("mean_", inputFileName);
	if (!outputFileName) {
		ret = 1;
		goto END;
	}

	pInputFile = fopen(inputFileName, "rb");
	if (!pInputFile) {
		/*�ϊ��O�̃t�@�C�����J���̂Ɏ��s�����ꍇ�̏���*/
		ret = 1;
		goto END;
	}

	pOutputFile = fopen(outputFileName, "wb");
	if (!pOutputFile) {
		/*�ϊ���̃t�@�C�����J���̂Ɏ��s�����ꍇ�̏���*/
		ret = 1;
		goto END;
	}

	/*�\����bitmapImage�̍쐬*/
	p8bitBitmapImage = bitmapImage();
	if (!p8bitBitmapImage) {
		/*�\����bitmapImage�̍쐬�Ɏ��s�����ꍇ�̏���*/
		ret = 1;
		goto END;
	}

	/*�摜�t�@�C������A�K�v�ȏ����\���̂֓ǂݍ���*/
	error = BitmapImage_load(p8bitBitmapImage, pInputFile);
	if (error) {
		ret = 1;
		goto END;
	}

	/*�m�C�Y����*/
	pOutputBitmapImage = BitmapImage_ReduceNoiseFilter(p8bitBitmapImage, MEAN , 3);

	/*�ϊ���̃t�@�C�����o��*/
	error = BitmapImage_to8bitBitmapFile(pOutputBitmapImage, pOutputFile);
	if (error) {
		/*�ϊ��Ɏ��s�����ꍇ�̏���*/
		ret = 1;
		goto END;
	}

END:
	_CrtDumpMemoryLeaks();
	return 0;
}