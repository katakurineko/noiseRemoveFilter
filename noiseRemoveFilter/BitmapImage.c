#include "BitmapImage.h"
#include "commonFunctions.h"

static MY_ERROR_CODE Load24bitImageData(BitmapImage* pBitmapImage, FILE* pFile);
static MY_ERROR_CODE Load8bitImageData(BitmapImage* pBitmapImage, FILE* pFile);
static int calcNumberOfPadding(BitmapImage* pBitmapImage);
static MY_BOOL IsBitmapFile(FILE* pFile);
static void printErrorMessage(MyErrorCode errorCode, char* file, int link);
static void writeBitmapFileHeader(BitmapImage* p8bitBitmapImage, FILE* pOutPutFile);
static void writeBitmapInfoHeader(BitmapImage* p8bitBitmapImage, FILE* pOutPutFile);
static void writeColorPaletteOf8bitGrayScale(FILE* pOutPutFile);
static void writeImageDataOf8bitBitmap(BitmapImage* p8bitBitmapImage, FILE* pOutPutFile);
static MY_ERROR_CODE meanNoiseFilter_NotEfficient(BitmapImage* pReturnImg, BitmapImage* pInputImg, unsigned long kernelSizeX, unsigned long kernelSizeY);
static MY_ERROR_CODE meanNoiseFilter_Efficient(BitmapImage* pReturnImg, BitmapImage* pInputImg, unsigned long kernelSizeX, unsigned long kernelSizeY);
static MY_ERROR_CODE medianNoiseFilter(BitmapImage* pReturnImg, BitmapImage* pInputImg, unsigned long kernelSizeX, unsigned long kernelSizeY);

/*空のBitmapImage構造体を返す関数*/
BitmapImage* bitmapImage() {
	size_t sizeOfBitmapImage = sizeof(BitmapImage);
	BitmapImage* pRet = (BitmapImage*)malloc(sizeOfBitmapImage);
	if (!pRet) {
		/*メモリの割当に失敗した際の処理*/
		goto END;
	}
	memset(pRet, 0x00, sizeOfBitmapImage);

END:
	return pRet;
}

/*
BitmapImage構造体が確保されている領域を解放する関数.
*/
void BitmapImageRelease(BitmapImage** ppBitmapImage) {

	BitmapImage* pTemp = *ppBitmapImage;

	if (pTemp) {
		/*pBitmapImageがヌルポインタでない場合の処理*/

		if (pTemp->pImageData) {
			/*pBitmapImage->pImageDataがヌルポインタでない場合の処理*/
			free(pTemp->pImageData);
		}
		free(pTemp);
	}
	*ppBitmapImage = NULL;
}

/*引数pFileにbmp画像ファイルを渡すと、引数pBitmapImageへ必要な情報を読み込む関数*/
MY_ERROR_CODE BitmapImageLoad(BitmapImage* pBitmapImage, FILE* pFile) {
	MY_ERROR_CODE ret = SUCCESS;

	/*ファイルがbitmapか判定*/
	if (!IsBitmapFile(pFile)) {
		printErrorMessage(IS_NOT_BITMAP, __FILE__, __LINE__);
		ret = IS_NOT_BITMAP;
		goto END;
	}


	BITMAPINFOHEADER bitmapInfoHeader = { .biSize = 0,.biWidth = 0,.biHeight = 0,.biPlanes = 0,.biBitCount = 0,.biCompression = 0,.biSizeImage = 0,.biXPelsPerMeter = 0,.biYPelsPerMeter = 0,.biClrUsed = 0,.biClrImportant = 0 };
	/*ファイル情報ヘッダの情報を取得*/
	fread(&bitmapInfoHeader, sizeof(BITMAPINFOHEADER), 1, pFile);

	/*windowsBitmapは40バイトだが、OS/2の場合は12バイトらしいので、
	windowsBitmapかどうかを判定*/
	if (40 != bitmapInfoHeader.biSize) {
		printErrorMessage(IS_NOT_WINDOWS_BITMAP, __FILE__, __LINE__);
		goto END;
	}

	/*構造体BitmapImageに、読み込んだbmpファイルの必要なヘッダ情報を入れる*/
	pBitmapImage->width = bitmapInfoHeader.biWidth;
	pBitmapImage->height = bitmapInfoHeader.biHeight;
	pBitmapImage->compression = bitmapInfoHeader.biCompression;
	pBitmapImage->bitCount = bitmapInfoHeader.biBitCount;

	/*構造体BitmapImageに、読み込んだbmpファイルの画像情報を入れる*/
	if (24 == bitmapInfoHeader.biBitCount) {
		ret = Load24bitImageData(pBitmapImage, pFile);
	}
	else if (8 == bitmapInfoHeader.biBitCount) {
		/*位置指定子をカラーパレッドのぶんだけずらす*/
		fseek(pFile, 256 * 4, SEEK_CUR);

		ret = Load8bitImageData(pBitmapImage, pFile);
	}

	if (ret != SUCCESS) {
		goto END;
	}

	ret = SUCCESS;
END:

	return ret;
}

/*8bitの画像ファイルを出力する関数*/
MY_ERROR_CODE BitmapImageTo8bitBitmapFile(BitmapImage* p8bitBitmapImage, FILE* pOutPutFile) {

	MY_ERROR_CODE ret = SUCCESS;

	if (p8bitBitmapImage->bitCount != 8) {
		/*渡された構造体BitmapImageが8bit形式でなかった場合の処理*/
		printErrorMessage(IS_NOT_8BIT, __FILE__, __LINE__);
		ret = IS_NOT_8BIT;
		goto END;
	}

	/*ファイルヘッダを書き込む*/
	writeBitmapFileHeader(p8bitBitmapImage, pOutPutFile);

	/*情報ヘッダを書き込む*/
	writeBitmapInfoHeader(p8bitBitmapImage, pOutPutFile);

	/*カラーパレッドを書き込む*/
	writeColorPaletteOf8bitGrayScale(pOutPutFile);

	/*画像データを書き込む*/
	writeImageDataOf8bitBitmap(p8bitBitmapImage, pOutPutFile);

END:

	return ret;
}

/*
ノイズ除去処理を行う関数
【引数】
	[i] BitmapImage*		ノイズ除去を行いたい画像データ（8bit形式）
	[i] MyEdgeFilterType	ノイズ除去を行うフィルタの種類
	[i] unsigned long		フィルタのカーネルのx方向のサイズ(奇数のみ)
	[i] unsigned long		フィルタのカーネルのy方向のサイズ(奇数のみ)
【返り値】
	[o] BitmapImage*		ノイズ除去を行った結果の画像データ（8bit形式）
*/
BitmapImage* BitmapImageReduceNoiseFilter(BitmapImage* pInputImg, MyNoiseReductionFilterType filterType, unsigned long kernelSizeX, unsigned long kernelSizeY) {

	BitmapImage* pReturnImg = NULL;

	unsigned long height = abs(pInputImg->height);
	unsigned long width = pInputImg->width;

	/*引数チェック*/
	if (!pInputImg) {
		printErrorMessage(ARGUMENT_ERROR, __FILE__, __LINE__);
		goto error;
	}
	if (8 != pInputImg->bitCount) {
		printErrorMessage(ARGUMENT_ERROR, __FILE__, __LINE__);
		goto error;
	}
	if (NOT_COMPRESSION != pInputImg->compression) {
		printErrorMessage(ARGUMENT_ERROR, __FILE__, __LINE__);
		goto error;
	}
	if (0 == kernelSizeX % 2 || 0 == kernelSizeY % 2) {
		printErrorMessage(ARGUMENT_ERROR, __FILE__, __LINE__);
		goto error;
	}
	if (kernelSizeY > height || kernelSizeX > width) {
		printErrorMessage(ARGUMENT_ERROR, __FILE__, __LINE__);
		goto error;
	}

	pReturnImg = bitmapImage();
	pReturnImg->bitCount = 8;
	pReturnImg->compression = NOT_COMPRESSION;
	pReturnImg->height = height;
	pReturnImg->width = width;

	MY_ERROR_CODE error = SUCCESS;

	switch (filterType)
	{
	case MEAN:
		error = meanNoiseFilter_Efficient(pReturnImg, pInputImg, kernelSizeX, kernelSizeY);
		if (error) {
			goto error;
		}
		break;
	case MEDIAN:
		error = medianNoiseFilter(pReturnImg, pInputImg, kernelSizeX, kernelSizeY);
		if (error) {
			goto error;
		}
		break;
	default:
		error = ARGUMENT_ERROR;
		printErrorMessage(ARGUMENT_ERROR, __FILE__, __LINE__);
		goto error;
	}

	return pReturnImg;

error:
	if (pReturnImg) {
		BitmapImageRelease(&pReturnImg);
	}
	return NULL;
}

/*平均値法によるノイズ除去処理を行う関数(高速化なし)*/
static MY_ERROR_CODE meanNoiseFilter_NotEfficient(BitmapImage* pReturnImg, BitmapImage* pInputImg, unsigned long kernelSizeX, unsigned long kernelSizeY) {

	MyErrorCode ret = SUCCESS;

	unsigned long height = abs(pInputImg->height);
	unsigned long width = pInputImg->width;

	/*引数チェック*/
	if (!pReturnImg || !pInputImg) {
		printErrorMessage(ARGUMENT_ERROR, __FILE__, __LINE__);
		ret = ARGUMENT_ERROR;
		goto END;
	}
	if (8 != pReturnImg->bitCount || 8 != pInputImg->bitCount) {
		printErrorMessage(ARGUMENT_ERROR, __FILE__, __LINE__);
		ret = ARGUMENT_ERROR;
		goto END;
	}
	if (NOT_COMPRESSION != pReturnImg->compression || NOT_COMPRESSION != pInputImg->compression) {
		printErrorMessage(ARGUMENT_ERROR, __FILE__, __LINE__);
		ret = ARGUMENT_ERROR;
		goto END;
	}
	if (0 == kernelSizeX % 2 || 0 == kernelSizeY % 2) {
		printErrorMessage(ARGUMENT_ERROR, __FILE__, __LINE__);
		ret = ARGUMENT_ERROR;
		goto END;
	}
	if (kernelSizeY > height || kernelSizeX > width) {
		printErrorMessage(ARGUMENT_ERROR, __FILE__, __LINE__);
		ret = ARGUMENT_ERROR;
		goto END;
	}

	/*ピクセル数を計算*/
	unsigned long numberOfPixel = width * height;

	/*画像領域の確保*/
	char* pRetImgData = (char*)malloc(numberOfPixel);
	if (!pRetImgData) {
		ret = MALLOC_ERROR;
		free(pRetImgData);
		printErrorMessage(MALLOC_ERROR, __FILE__, __LINE__);
		goto END;
	}
	memset(pRetImgData, 0x00, numberOfPixel);
	pReturnImg->pImageData = pRetImgData;

	unsigned char* pInpImaData = pInputImg->pImageData;

	unsigned long lineFeed = width;
	unsigned long rowIndexStart = 0;
	unsigned long index = 0;

	/*カーネルの要素数*/
	unsigned long elementsNumberOfKernel = kernelSizeX * kernelSizeY;

	unsigned long sumInKernel = 0;

	/*フィルタをかけたことによって削れる画像端のサイズ*/
	unsigned long paddingSizeXByFilter = (kernelSizeX - 1) / 2;
	unsigned long paddingSizeYByFilter = (kernelSizeY - 1) / 2;

	/*縦方向と横方向、それぞれのループ回数*/
	unsigned long numberOfLoopOfY = height - (kernelSizeY - 1);
	unsigned long numberOfLoopOfX = width - (kernelSizeX - 1);

	for (unsigned long y = 0; y < numberOfLoopOfY; y++, rowIndexStart += lineFeed) {

		for (unsigned long x = 0; x < numberOfLoopOfX; x++) {
			index = x + rowIndexStart;

			sumInKernel = 0;
			for (unsigned long yOfKarnel = 0; yOfKarnel < kernelSizeY; yOfKarnel++) {
				for (unsigned long xOfKarnel = 0; xOfKarnel < kernelSizeX; xOfKarnel++) {
					sumInKernel += pInpImaData[index + xOfKarnel + yOfKarnel * lineFeed];
				}
			}
			pRetImgData[index + paddingSizeXByFilter + lineFeed * paddingSizeYByFilter] = (char)(sumInKernel / elementsNumberOfKernel);
		}
	}

END:
	return ret;
}

/*平均値法によるノイズ除去処理を行う関数*/
static MY_ERROR_CODE meanNoiseFilter_Efficient(BitmapImage* pReturnImg, BitmapImage* pInputImg, unsigned long kernelSizeX, unsigned long kernelSizeY) {

	MyErrorCode ret = SUCCESS;

	unsigned long height = abs(pInputImg->height);
	unsigned long width = pInputImg->width;

	/*引数チェック*/
	if (!pReturnImg || !pInputImg) {
		printErrorMessage(ARGUMENT_ERROR, __FILE__, __LINE__);
		ret = ARGUMENT_ERROR;
		goto END;
	}
	if (8 != pReturnImg->bitCount || 8 != pInputImg->bitCount) {
		printErrorMessage(ARGUMENT_ERROR, __FILE__, __LINE__);
		ret = ARGUMENT_ERROR;
		goto END;
	}
	if (NOT_COMPRESSION != pReturnImg->compression || NOT_COMPRESSION != pInputImg->compression) {
		printErrorMessage(ARGUMENT_ERROR, __FILE__, __LINE__);
		ret = ARGUMENT_ERROR;
		goto END;
	}
	if (0 == kernelSizeX % 2 || 0 == kernelSizeY % 2) {
		printErrorMessage(ARGUMENT_ERROR, __FILE__, __LINE__);
		ret = ARGUMENT_ERROR;
		goto END;
	}
	if (kernelSizeY > height || kernelSizeX > width) {
		printErrorMessage(ARGUMENT_ERROR, __FILE__, __LINE__);
		ret = ARGUMENT_ERROR;
		goto END;
	}

	/*ピクセル数を計算*/
	unsigned long numberOfPixel = width * height;

	/*画像領域の確保*/
	char* pRetImgData = (char*)malloc(numberOfPixel);
	if (!pRetImgData) {
		ret = MALLOC_ERROR;
		free(pRetImgData);
		printErrorMessage(MALLOC_ERROR, __FILE__, __LINE__);
		goto END;
	}
	memset(pRetImgData, 0x00, numberOfPixel);
	pReturnImg->pImageData = pRetImgData;

	unsigned char* pInpImaData = pInputImg->pImageData;

	unsigned long lineFeed = width;
	unsigned long indexOfRowStartInImage = 0;
	unsigned long indexOfRowStartInKernel = 0;
	unsigned long indexOfRowEndInKernel = 0;

	/*カーネルの要素数*/
	unsigned long elementsNumberOfKernel = kernelSizeX * kernelSizeY;

	/*カーネル内の輝度値の合計*/
	unsigned long sumInKernel = 0;

	/*フィルタをかけたことによって削れる画像端のサイズ*/
	unsigned long paddingSizeXByFilter = (kernelSizeX - 1) / 2;
	unsigned long paddingSizeYByFilter = (kernelSizeY - 1) / 2;

	/*縦方向と横方向、それぞれのループ回数*/
	unsigned long numberOfLoopOfY = height - (kernelSizeY - 1);
	unsigned long numberOfLoopOfX = width - (kernelSizeX - 1);

	/*一つ前に計算したカーネル内の輝度値の合計から、輝度値の合計を計算するための変数*/
	unsigned long addPart = 0;
	unsigned long removePart = 0;

	/*インプット画像からアウトプット画像へのindex補正*/
	unsigned long indexCorrectionFromInpToOut = 0;

	for (unsigned long y = 0; y < numberOfLoopOfY; y++, indexOfRowStartInImage += lineFeed) {

		indexCorrectionFromInpToOut = indexOfRowStartInImage + paddingSizeXByFilter + paddingSizeYByFilter * lineFeed;

		/*行の中で1番最初のカーネルの計算*/
		indexOfRowStartInKernel = y * lineFeed;
		sumInKernel = 0;
		removePart = 0;
		for (unsigned long yOfKarnel = 0; yOfKarnel < kernelSizeY; yOfKarnel++, indexOfRowStartInKernel += lineFeed) {
			removePart += pInpImaData[indexOfRowStartInKernel];
			for (unsigned long xOfKarnel = 0; xOfKarnel < kernelSizeX; xOfKarnel++) {
				sumInKernel += pInpImaData[indexOfRowStartInKernel + xOfKarnel];
			}
		}
		pRetImgData[indexCorrectionFromInpToOut] = (char)(sumInKernel / elementsNumberOfKernel);

		for (unsigned long x = 1; x < numberOfLoopOfX; x++) {

			/*前回計算したカーネル内の輝度値の合計に足す値を計算*/
			indexOfRowEndInKernel = indexOfRowStartInImage + x + kernelSizeX - 1;
			addPart = 0;
			for (unsigned long yOfKarnel = 0; yOfKarnel < kernelSizeY; yOfKarnel++, indexOfRowEndInKernel += lineFeed) {
				addPart += pInpImaData[indexOfRowEndInKernel];
			}

			/*カーネル内の輝度値の合計を計算*/
			sumInKernel = sumInKernel + addPart - removePart;

			/*平均を計算して、出力画像へ*/
			pRetImgData[x + indexCorrectionFromInpToOut] = (char)(sumInKernel / elementsNumberOfKernel);

			/*前回計算したカーネル内の輝度値の合計から引く値を計算*/
			indexOfRowStartInKernel = indexOfRowStartInImage + x;
			removePart = 0;
			for (unsigned long yOfKarnel = 0; yOfKarnel < kernelSizeY; yOfKarnel++, indexOfRowStartInKernel += lineFeed) {
				removePart += pInpImaData[indexOfRowStartInKernel];
			}

		}
	}

END:
	return ret;
}

/*中央値法によるノイズ除去処理を行う関数*/
static MY_ERROR_CODE medianNoiseFilter(BitmapImage* pReturnImg, BitmapImage* pInputImg, unsigned long kernelSizeX, unsigned long kernelSizeY) {

	MyErrorCode ret = SUCCESS;

	/*カーネルの要素配列*/
	unsigned char* elementArray = NULL;

	unsigned long height = abs(pInputImg->height);
	unsigned long width = pInputImg->width;

	/*引数チェック*/
	if (!pReturnImg || !pInputImg) {
		printErrorMessage(ARGUMENT_ERROR, __FILE__, __LINE__);
		ret = ARGUMENT_ERROR;
		goto END;
	}
	if (8 != pReturnImg->bitCount || 8 != pInputImg->bitCount) {
		printErrorMessage(ARGUMENT_ERROR, __FILE__, __LINE__);
		ret = ARGUMENT_ERROR;
		goto END;
	}
	if (NOT_COMPRESSION != pReturnImg->compression || NOT_COMPRESSION != pInputImg->compression) {
		printErrorMessage(ARGUMENT_ERROR, __FILE__, __LINE__);
		ret = ARGUMENT_ERROR;
		goto END;
	}
	if (0 == kernelSizeX % 2 || 0 == kernelSizeY % 2) {
		printErrorMessage(ARGUMENT_ERROR, __FILE__, __LINE__);
		ret = ARGUMENT_ERROR;
		goto END;
	}
	if (kernelSizeY > height || kernelSizeX > width) {
		printErrorMessage(ARGUMENT_ERROR, __FILE__, __LINE__);
		ret = ARGUMENT_ERROR;
		goto END;
	}

	/*ピクセル数を計算*/
	unsigned long numberOfPixel = width * height;

	/*画像領域の確保*/
	char* pRetImgData = (char*)malloc(numberOfPixel);
	if (!pRetImgData) {
		ret = MALLOC_ERROR;
		free(pRetImgData);
		printErrorMessage(MALLOC_ERROR, __FILE__, __LINE__);
		goto END;
	}
	memset(pRetImgData, 0x00, numberOfPixel);
	pReturnImg->pImageData = pRetImgData;

	unsigned char* pInpImaData = pInputImg->pImageData;

	unsigned long lineFeed = width;
	unsigned long rowIndexStart = 0;
	unsigned long index = 0;

	/*カーネルの要素数*/
	unsigned long elementsNumberOfKernel = kernelSizeX * kernelSizeY;

	elementArray = (unsigned char*)malloc(sizeof(unsigned char)*elementsNumberOfKernel);
	if (!elementArray) {
		printErrorMessage(MALLOC_ERROR, __FILE__, __LINE__);
		ret = MALLOC_ERROR;
		goto END;
	}
	memset(elementArray, 0x00, elementsNumberOfKernel);

	/*フィルタをかけたことによって削れる画像端のサイズ*/
	unsigned long paddingSizeXByFilter = (kernelSizeX - 1) / 2;
	unsigned long paddingSizeYByFilter = (kernelSizeY - 1) / 2;

	/*縦方向と横方向、それぞれのループ回数*/
	unsigned long numberOfLoopOfY = height - (kernelSizeY - 1);
	unsigned long numberOfLoopOfX = width - (kernelSizeX - 1);

	unsigned long count = 0;

	for (unsigned long y = 0; y < numberOfLoopOfY; y++, rowIndexStart += lineFeed) {

		for (unsigned long x = 0; x < numberOfLoopOfX; x++) {
			index = x + rowIndexStart;

			count = 0;
			for (unsigned long yOfKarnel = 0; yOfKarnel < kernelSizeY; yOfKarnel++) {
				for (unsigned long xOfKarnel = 0; xOfKarnel < kernelSizeX; xOfKarnel++) {
					elementArray[count] = pInpImaData[index + xOfKarnel + yOfKarnel * lineFeed];
					count += 1;
				}
			}
			qsort(elementArray, elementsNumberOfKernel, sizeof(unsigned char), compareUnsignedChar);
			pRetImgData[index + paddingSizeXByFilter + lineFeed * paddingSizeYByFilter] = elementArray[elementsNumberOfKernel / 2], sizeof(char);
		}
	}

END:
	if (elementArray) {
		free(elementArray);
	}
	return ret;
}


/*ビットマップファイルヘッダを書き込む関数*/
static void writeBitmapFileHeader(BitmapImage* p8bitBitmapImage, FILE* pOutPutFile) {
	/*ファイル形式の書き込み*/
	fwrite("BM", strlen("BM"), 1, pOutPutFile);

	/*画像データ部のサイズの計算*/
	unsigned long pictureDataSize = (p8bitBitmapImage->width + calcNumberOfPadding(p8bitBitmapImage)) * abs(p8bitBitmapImage->height);

	/*ファイルサイズを計算して書き込み*/
	unsigned long bfSize = OFFSET_TO_IMAGE_DATA_8BIT + pictureDataSize;
	fwrite(&bfSize, sizeof(bfSize), 1, pOutPutFile);

	/*予約領域の書き込み*/
	signed short bfReserved1 = 0;
	fwrite(&bfReserved1, sizeof(bfReserved1), 1, pOutPutFile);
	signed short bfReserved2 = 0;
	fwrite(&bfReserved2, sizeof(bfReserved2), 1, pOutPutFile);

	/*ファイル先頭から画像データまでのオフセットの書き込み*/
	unsigned long bfOffBits = OFFSET_TO_IMAGE_DATA_8BIT;
	fwrite(&bfOffBits, sizeof(bfOffBits), 1, pOutPutFile);
}

/*ビットマップ情報ヘッダを書き込む関数*/
static void writeBitmapInfoHeader(BitmapImage* p8bitBitmapImage, FILE* pOutPutFile) {
	/*情報ヘッダのサイズの書き込み*/
	unsigned long biSize = WINDOWS_BITMAP_INFO_HEADER_SIZE;
	fwrite(&biSize, sizeof(biSize), 1, pOutPutFile);

	unsigned long width = p8bitBitmapImage->width;
	long height = p8bitBitmapImage->height;

	/*画像の幅と高さの書き込み*/
	fwrite(&(width), sizeof(width), 1, pOutPutFile);
	fwrite(&(height), sizeof(height), 1, pOutPutFile);

	/*プレーン数の書き込み*/
	unsigned short biPlanes = BI_PLANES_VALUE;
	fwrite(&biPlanes, sizeof(biPlanes), 1, pOutPutFile);

	/*1画素あたりのデータサイズを書き込み*/
	unsigned short biBitCount = p8bitBitmapImage->bitCount;
	fwrite(&biBitCount, sizeof(biBitCount), 1, pOutPutFile);

	/*圧縮形式を書き込み*/
	unsigned long biCompression = p8bitBitmapImage->compression;
	fwrite(&biCompression, sizeof(biCompression), 1, pOutPutFile);

	/*画像データ部のサイズの計算*/
	unsigned long pictureDataSize = (p8bitBitmapImage->width + calcNumberOfPadding(p8bitBitmapImage)) * abs(p8bitBitmapImage->height);

	/*画像データ部のサイズを書き込み*/
	fwrite(&pictureDataSize, sizeof(pictureDataSize), 1, pOutPutFile);

	/*解像度を書き込み*/
	unsigned long biXPixPerMeter = BI_X_PELS_PER_METER_VALUE;
	unsigned long biYPixPerMeter = BI_Y_PELS_PER_METER_VALUE;
	fwrite(&biXPixPerMeter, sizeof(biXPixPerMeter), 1, pOutPutFile);
	fwrite(&biYPixPerMeter, sizeof(biYPixPerMeter), 1, pOutPutFile);

	/*パレット数の書き込み*/
	unsigned long biClrUsed = BI_CLR_8BIT;
	fwrite(&biClrUsed, sizeof(biClrUsed), 1, pOutPutFile);

	/*重要なパレットのインデックスを書き込み*/
	unsigned long biClrImportant = BI_CLR_IMPORTANT_VALUE;
	fwrite(&biClrImportant, sizeof(biClrImportant), 1, pOutPutFile);
}

/*8bitグレースケールのカラーパレッドを書き込む関数*/
static void writeColorPaletteOf8bitGrayScale(FILE* pOutPutFile) {

	unsigned char rgbColorPalette = 0;
	unsigned char colorPaletteReserved = COLOR_PALETTE_RESERVED;
	size_t sizeOfRgbColorPalette = sizeof(rgbColorPalette);
	size_t sizeOfColorPaletteReserved = sizeof(colorPaletteReserved);

	for (int i = 0; i < BI_CLR_8BIT; i++) {
		for (int j = 0; j < 3; j++) {
			fwrite(&rgbColorPalette, sizeOfRgbColorPalette, 1, pOutPutFile);
		}
		fwrite(&colorPaletteReserved, sizeOfColorPaletteReserved, 1, pOutPutFile);
		rgbColorPalette++;
	}

}

/*8bitビットマップの画像データを書き込む関数*/
static void writeImageDataOf8bitBitmap(BitmapImage* p8bitBitmapImage, FILE* pOutPutFile) {
	char* pImageData = p8bitBitmapImage->pImageData;
	unsigned long width = p8bitBitmapImage->width;
	unsigned long height = abs(p8bitBitmapImage->height);

	/*パディング数を計算*/
	int numberOfPadding = calcNumberOfPadding(p8bitBitmapImage);

	for (unsigned long h = 0; h < height; h++) {
		/*１幅の画像データを書き込む*/
		fwrite(pImageData + width * h, width, 1, pOutPutFile);

		/*パディングを書き込み*/
		for (int padding = 0; padding < numberOfPadding; padding++) {
			fputc(0x00, pOutPutFile);
		}
	}
}

/*24bitビットマップファイルの、パディングを除いた画像データをメモリ上に読み込み、
そのポインタを、引数として渡したpBitmapImageのポインタ変数pImageDataへ代入する関数.*/
static MY_ERROR_CODE Load24bitImageData(BitmapImage* pBitmapImage, FILE* pFile) {

	MY_ERROR_CODE ret = SUCCESS;

	/*24bitか判定*/
	if (pBitmapImage->bitCount != 24) {
		printErrorMessage(IS_NOT_24BIT, __FILE__, __LINE__);
		ret = IS_NOT_24BIT;
		goto END;
	}

	/*パディング数を計算*/
	int numberOfPadding = calcNumberOfPadding(pBitmapImage);

	unsigned long width = pBitmapImage->width;
	unsigned long height = abs(pBitmapImage->height);
	unsigned long bytesPerWidth = width * 3;
	unsigned long sizeOfImageData = bytesPerWidth * height;

	char* p24bitImageData = (char*)malloc(sizeOfImageData);
	if (!p24bitImageData) {
		printErrorMessage(MALLOC_ERROR, __FILE__, __LINE__);
		goto END;
	}

	memset(p24bitImageData, 0x00, sizeOfImageData);

	for (unsigned long h = 0; h < height; h++) {

		/*1幅の画像データを読み込み*/
		fread(p24bitImageData + h * bytesPerWidth, sizeof(char), bytesPerWidth, pFile);

		/*ファイルの位置指定子をパディングの分だけずらす*/
		fseek(pFile, numberOfPadding, SEEK_CUR);
	}

	pBitmapImage->pImageData = p24bitImageData;

END:
	return ret;
}

/*8bitビットマップファイルの、パディングを除いた画像データをメモリ上に読み込み、
そのポインタを、引数として渡したpBitmapImageのポインタ変数pImageDataへ代入する関数.*/
static MY_ERROR_CODE Load8bitImageData(BitmapImage* pBitmapImage, FILE* pFile) {

	MY_ERROR_CODE ret = SUCCESS;

	/*8bitか判定*/
	if (pBitmapImage->bitCount != 8) {
		printErrorMessage(IS_NOT_8BIT, __FILE__, __LINE__);
		ret = IS_NOT_8BIT;
		goto END;
	}

	/*パディング数を計算*/
	int numberOfPadding = calcNumberOfPadding(pBitmapImage);

	unsigned long width = pBitmapImage->width;
	unsigned long height = abs(pBitmapImage->height);
	unsigned long bytesPerWidth = width;
	unsigned long sizeOfImageData = bytesPerWidth * height;

	char* p8bitImageData = (char*)malloc(sizeOfImageData);
	if (!p8bitImageData) {
		printErrorMessage(MALLOC_ERROR, __FILE__, __LINE__);
		goto END;
	}

	memset(p8bitImageData, 0x00, sizeOfImageData);

	for (unsigned long h = 0; h < height; h++) {

		/*1幅の画像データを読み込み*/
		fread(p8bitImageData + h * bytesPerWidth, sizeof(char), bytesPerWidth, pFile);

		/*ファイルの位置指定子をパディングの分だけずらす*/
		fseek(pFile, numberOfPadding, SEEK_CUR);
	}

	pBitmapImage->pImageData = p8bitImageData;

END:
	return ret;
}


/*1幅ごとのパディング数を求める関数*/
static int calcNumberOfPadding(BitmapImage* pBitmapImage) {
	int ret = 0;
	int bytesPerWidth = pBitmapImage->width * (pBitmapImage->bitCount / 8);
	if (bytesPerWidth % 4 != 0)
	{
		ret = (4 - bytesPerWidth % 4);
	}
	return ret;
}

/*引数pFileとして渡されたファイルがbitmapか判定する関数*/
static MY_BOOL IsBitmapFile(FILE* pFile) {

	MY_BOOL ret = MY_FALSE;

	BITMAPFILEHEADER bitmapFileHeader = { .bfType = 0,.bfSize = 0,.bfReserved1 = 0,.bfReserved2 = 0,.bfOffBits = 0 };

	/*ファイルヘッダの情報を取得*/
	fread(&bitmapFileHeader, sizeof(BITMAPFILEHEADER), 1, pFile);

	if (strncmp("BM", (char*)&(bitmapFileHeader.bfType), 2)) {
		/*ファイルヘッダのファイルタイプがBMでなかった際の処理*/
		printErrorMessage(IS_NOT_BITMAP, __FILE__, __LINE__);
		goto END;
	}

	ret = MY_TRUE;

END:
	return ret;
}

/*エラーメッセージを表示する関数*/
static void printErrorMessage(MyErrorCode errorCode, char* file, int link) {

	switch (errorCode) {
	case SUCCESS:
		break;
	case ARGUMENT_ERROR:
		printf("Argument error\n");
		break;
	case IS_NOT_BITMAP:
		printf("Not Bitmap error\n");
		break;
	case IS_NULL_POINTER:
		printf("Null pointer error\n");
		break;
	case MALLOC_ERROR:
		printf("Failed to alloc memory\n");
		break;
	case IS_NOT_WINDOWS_BITMAP:
		printf("Bitmap file is not windowsBitmap\n");
	case IS_NOT_24BIT:
		printf("Not 24bit Bitmap error\n");
		break;
	case IS_NOT_8BIT:
		printf("Not 8bit Bitmap error\n");
		break;
	}

	if (errorCode) {
		printf("The location where the error occurred is %s (%d) \n", file, link);
	}

}

