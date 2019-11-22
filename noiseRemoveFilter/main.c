#define _CRT_SECURE_NO_WARNINGS
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#define new  ::new(_NORMAL_BLOCK, __FILE__, __LINE__)

#include"BitmapImage.h"
#include "MyErrorCodes.h"
#include "commonFunctions.h"

/*変換する画像の指定*/
#define INPUT_FILE_NAME "test.bmp"

int main() {

	int ret = 0;

	MY_ERROR_CODE error = SUCCESS;

	FILE* pInputFile = NULL;
	FILE* pOutputFile = NULL;
	BitmapImage* pOutputBitmapImage = NULL;
	BitmapImage* p8bitBitmapImage = NULL;

	/*インプットファイルとアウトプットファイルの準備*/
	char inputFileName[] = INPUT_FILE_NAME;
	char* outputFileName = strJoin("mean_", inputFileName);
	if (!outputFileName) {
		ret = 1;
		goto END;
	}

	pInputFile = fopen(inputFileName, "rb");
	if (!pInputFile) {
		/*変換前のファイルを開くのに失敗した場合の処理*/
		ret = 1;
		goto END;
	}

	pOutputFile = fopen(outputFileName, "wb");
	if (!pOutputFile) {
		/*変換後のファイルを開くのに失敗した場合の処理*/
		ret = 1;
		goto END;
	}

	/*構造体bitmapImageの作成*/
	p8bitBitmapImage = bitmapImage();
	if (!p8bitBitmapImage) {
		/*構造体bitmapImageの作成に失敗した場合の処理*/
		ret = 1;
		goto END;
	}

	/*画像ファイルから、必要な情報を構造体へ読み込む*/
	error = BitmapImage_load(p8bitBitmapImage, pInputFile);
	if (error) {
		ret = 1;
		goto END;
	}

	/*ノイズ除去*/
	pOutputBitmapImage = BitmapImage_ReduceNoiseFilter(p8bitBitmapImage, MEAN , 3);

	/*変換後のファイルを出力*/
	error = BitmapImage_to8bitBitmapFile(pOutputBitmapImage, pOutputFile);
	if (error) {
		/*変換に失敗した場合の処理*/
		ret = 1;
		goto END;
	}

END:
	_CrtDumpMemoryLeaks();
	return 0;
}