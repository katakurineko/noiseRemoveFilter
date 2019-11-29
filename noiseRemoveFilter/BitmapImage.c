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

/*���BitmapImage�\���̂�Ԃ��֐�*/
BitmapImage* bitmapImage() {
	size_t sizeOfBitmapImage = sizeof(BitmapImage);
	BitmapImage* pRet = (BitmapImage*)malloc(sizeOfBitmapImage);
	if (!pRet) {
		/*�������̊����Ɏ��s�����ۂ̏���*/
		goto END;
	}
	memset(pRet, 0x00, sizeOfBitmapImage);

END:
	return pRet;
}

/*
BitmapImage�\���̂��m�ۂ���Ă���̈���������֐�.
*/
void BitmapImageRelease(BitmapImage** ppBitmapImage) {

	BitmapImage* pTemp = *ppBitmapImage;

	if (pTemp) {
		/*pBitmapImage���k���|�C���^�łȂ��ꍇ�̏���*/

		if (pTemp->pImageData) {
			/*pBitmapImage->pImageData���k���|�C���^�łȂ��ꍇ�̏���*/
			free(pTemp->pImageData);
		}
		free(pTemp);
	}
	*ppBitmapImage = NULL;
}

/*����pFile��bmp�摜�t�@�C����n���ƁA����pBitmapImage�֕K�v�ȏ���ǂݍ��ފ֐�*/
MY_ERROR_CODE BitmapImageLoad(BitmapImage* pBitmapImage, FILE* pFile) {
	MY_ERROR_CODE ret = SUCCESS;

	/*�t�@�C����bitmap������*/
	if (!IsBitmapFile(pFile)) {
		printErrorMessage(IS_NOT_BITMAP, __FILE__, __LINE__);
		ret = IS_NOT_BITMAP;
		goto END;
	}


	BITMAPINFOHEADER bitmapInfoHeader = { .biSize = 0,.biWidth = 0,.biHeight = 0,.biPlanes = 0,.biBitCount = 0,.biCompression = 0,.biSizeImage = 0,.biXPelsPerMeter = 0,.biYPelsPerMeter = 0,.biClrUsed = 0,.biClrImportant = 0 };
	/*�t�@�C�����w�b�_�̏����擾*/
	fread(&bitmapInfoHeader, sizeof(BITMAPINFOHEADER), 1, pFile);

	/*windowsBitmap��40�o�C�g�����AOS/2�̏ꍇ��12�o�C�g�炵���̂ŁA
	windowsBitmap���ǂ����𔻒�*/
	if (40 != bitmapInfoHeader.biSize) {
		printErrorMessage(IS_NOT_WINDOWS_BITMAP, __FILE__, __LINE__);
		goto END;
	}

	/*�\����BitmapImage�ɁA�ǂݍ���bmp�t�@�C���̕K�v�ȃw�b�_��������*/
	pBitmapImage->width = bitmapInfoHeader.biWidth;
	pBitmapImage->height = bitmapInfoHeader.biHeight;
	pBitmapImage->compression = bitmapInfoHeader.biCompression;
	pBitmapImage->bitCount = bitmapInfoHeader.biBitCount;

	/*�\����BitmapImage�ɁA�ǂݍ���bmp�t�@�C���̉摜��������*/
	if (24 == bitmapInfoHeader.biBitCount) {
		ret = Load24bitImageData(pBitmapImage, pFile);
	}
	else if (8 == bitmapInfoHeader.biBitCount) {
		/*�ʒu�w��q���J���[�p���b�h�̂Ԃ񂾂����炷*/
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

/*8bit�̉摜�t�@�C�����o�͂���֐�*/
MY_ERROR_CODE BitmapImageTo8bitBitmapFile(BitmapImage* p8bitBitmapImage, FILE* pOutPutFile) {

	MY_ERROR_CODE ret = SUCCESS;

	if (p8bitBitmapImage->bitCount != 8) {
		/*�n���ꂽ�\����BitmapImage��8bit�`���łȂ������ꍇ�̏���*/
		printErrorMessage(IS_NOT_8BIT, __FILE__, __LINE__);
		ret = IS_NOT_8BIT;
		goto END;
	}

	/*�t�@�C���w�b�_����������*/
	writeBitmapFileHeader(p8bitBitmapImage, pOutPutFile);

	/*���w�b�_����������*/
	writeBitmapInfoHeader(p8bitBitmapImage, pOutPutFile);

	/*�J���[�p���b�h����������*/
	writeColorPaletteOf8bitGrayScale(pOutPutFile);

	/*�摜�f�[�^����������*/
	writeImageDataOf8bitBitmap(p8bitBitmapImage, pOutPutFile);

END:

	return ret;
}

/*
�m�C�Y�����������s���֐�
�y�����z
	[i] BitmapImage*		�m�C�Y�������s�������摜�f�[�^�i8bit�`���j
	[i] MyEdgeFilterType	�m�C�Y�������s���t�B���^�̎��
	[i] unsigned long		�t�B���^�̃J�[�l����x�����̃T�C�Y(��̂�)
	[i] unsigned long		�t�B���^�̃J�[�l����y�����̃T�C�Y(��̂�)
�y�Ԃ�l�z
	[o] BitmapImage*		�m�C�Y�������s�������ʂ̉摜�f�[�^�i8bit�`���j
*/
BitmapImage* BitmapImageReduceNoiseFilter(BitmapImage* pInputImg, MyNoiseReductionFilterType filterType, unsigned long kernelSizeX, unsigned long kernelSizeY) {

	BitmapImage* pReturnImg = NULL;

	unsigned long height = abs(pInputImg->height);
	unsigned long width = pInputImg->width;

	/*�����`�F�b�N*/
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

/*���ϒl�@�ɂ��m�C�Y�����������s���֐�(�������Ȃ�)*/
static MY_ERROR_CODE meanNoiseFilter_NotEfficient(BitmapImage* pReturnImg, BitmapImage* pInputImg, unsigned long kernelSizeX, unsigned long kernelSizeY) {

	MyErrorCode ret = SUCCESS;

	unsigned long height = abs(pInputImg->height);
	unsigned long width = pInputImg->width;

	/*�����`�F�b�N*/
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

	/*�s�N�Z�������v�Z*/
	unsigned long numberOfPixel = width * height;

	/*�摜�̈�̊m��*/
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

	/*�J�[�l���̗v�f��*/
	unsigned long elementsNumberOfKernel = kernelSizeX * kernelSizeY;

	unsigned long sumInKernel = 0;

	/*�t�B���^�����������Ƃɂ���č���摜�[�̃T�C�Y*/
	unsigned long paddingSizeXByFilter = (kernelSizeX - 1) / 2;
	unsigned long paddingSizeYByFilter = (kernelSizeY - 1) / 2;

	/*�c�����Ɖ������A���ꂼ��̃��[�v��*/
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

/*���ϒl�@�ɂ��m�C�Y�����������s���֐�*/
static MY_ERROR_CODE meanNoiseFilter_Efficient(BitmapImage* pReturnImg, BitmapImage* pInputImg, unsigned long kernelSizeX, unsigned long kernelSizeY) {

	MyErrorCode ret = SUCCESS;

	unsigned long height = abs(pInputImg->height);
	unsigned long width = pInputImg->width;

	/*�����`�F�b�N*/
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

	/*�s�N�Z�������v�Z*/
	unsigned long numberOfPixel = width * height;

	/*�摜�̈�̊m��*/
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

	/*�J�[�l���̗v�f��*/
	unsigned long elementsNumberOfKernel = kernelSizeX * kernelSizeY;

	/*�J�[�l�����̋P�x�l�̍��v*/
	unsigned long sumInKernel = 0;

	/*�t�B���^�����������Ƃɂ���č���摜�[�̃T�C�Y*/
	unsigned long paddingSizeXByFilter = (kernelSizeX - 1) / 2;
	unsigned long paddingSizeYByFilter = (kernelSizeY - 1) / 2;

	/*�c�����Ɖ������A���ꂼ��̃��[�v��*/
	unsigned long numberOfLoopOfY = height - (kernelSizeY - 1);
	unsigned long numberOfLoopOfX = width - (kernelSizeX - 1);

	/*��O�Ɍv�Z�����J�[�l�����̋P�x�l�̍��v����A�P�x�l�̍��v���v�Z���邽�߂̕ϐ�*/
	unsigned long addPart = 0;
	unsigned long removePart = 0;

	/*�C���v�b�g�摜����A�E�g�v�b�g�摜�ւ�index�␳*/
	unsigned long indexCorrectionFromInpToOut = 0;

	for (unsigned long y = 0; y < numberOfLoopOfY; y++, indexOfRowStartInImage += lineFeed) {

		indexCorrectionFromInpToOut = indexOfRowStartInImage + paddingSizeXByFilter + paddingSizeYByFilter * lineFeed;

		/*�s�̒���1�ԍŏ��̃J�[�l���̌v�Z*/
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

			/*�O��v�Z�����J�[�l�����̋P�x�l�̍��v�ɑ����l���v�Z*/
			indexOfRowEndInKernel = indexOfRowStartInImage + x + kernelSizeX - 1;
			addPart = 0;
			for (unsigned long yOfKarnel = 0; yOfKarnel < kernelSizeY; yOfKarnel++, indexOfRowEndInKernel += lineFeed) {
				addPart += pInpImaData[indexOfRowEndInKernel];
			}

			/*�J�[�l�����̋P�x�l�̍��v���v�Z*/
			sumInKernel = sumInKernel + addPart - removePart;

			/*���ς��v�Z���āA�o�͉摜��*/
			pRetImgData[x + indexCorrectionFromInpToOut] = (char)(sumInKernel / elementsNumberOfKernel);

			/*�O��v�Z�����J�[�l�����̋P�x�l�̍��v��������l���v�Z*/
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

/*�����l�@�ɂ��m�C�Y�����������s���֐�*/
static MY_ERROR_CODE medianNoiseFilter(BitmapImage* pReturnImg, BitmapImage* pInputImg, unsigned long kernelSizeX, unsigned long kernelSizeY) {

	MyErrorCode ret = SUCCESS;

	/*�J�[�l���̗v�f�z��*/
	unsigned char* elementArray = NULL;

	unsigned long height = abs(pInputImg->height);
	unsigned long width = pInputImg->width;

	/*�����`�F�b�N*/
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

	/*�s�N�Z�������v�Z*/
	unsigned long numberOfPixel = width * height;

	/*�摜�̈�̊m��*/
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

	/*�J�[�l���̗v�f��*/
	unsigned long elementsNumberOfKernel = kernelSizeX * kernelSizeY;

	elementArray = (unsigned char*)malloc(sizeof(unsigned char)*elementsNumberOfKernel);
	if (!elementArray) {
		printErrorMessage(MALLOC_ERROR, __FILE__, __LINE__);
		ret = MALLOC_ERROR;
		goto END;
	}
	memset(elementArray, 0x00, elementsNumberOfKernel);

	/*�t�B���^�����������Ƃɂ���č���摜�[�̃T�C�Y*/
	unsigned long paddingSizeXByFilter = (kernelSizeX - 1) / 2;
	unsigned long paddingSizeYByFilter = (kernelSizeY - 1) / 2;

	/*�c�����Ɖ������A���ꂼ��̃��[�v��*/
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


/*�r�b�g�}�b�v�t�@�C���w�b�_���������ފ֐�*/
static void writeBitmapFileHeader(BitmapImage* p8bitBitmapImage, FILE* pOutPutFile) {
	/*�t�@�C���`���̏�������*/
	fwrite("BM", strlen("BM"), 1, pOutPutFile);

	/*�摜�f�[�^���̃T�C�Y�̌v�Z*/
	unsigned long pictureDataSize = (p8bitBitmapImage->width + calcNumberOfPadding(p8bitBitmapImage)) * abs(p8bitBitmapImage->height);

	/*�t�@�C���T�C�Y���v�Z���ď�������*/
	unsigned long bfSize = OFFSET_TO_IMAGE_DATA_8BIT + pictureDataSize;
	fwrite(&bfSize, sizeof(bfSize), 1, pOutPutFile);

	/*�\��̈�̏�������*/
	signed short bfReserved1 = 0;
	fwrite(&bfReserved1, sizeof(bfReserved1), 1, pOutPutFile);
	signed short bfReserved2 = 0;
	fwrite(&bfReserved2, sizeof(bfReserved2), 1, pOutPutFile);

	/*�t�@�C���擪����摜�f�[�^�܂ł̃I�t�Z�b�g�̏�������*/
	unsigned long bfOffBits = OFFSET_TO_IMAGE_DATA_8BIT;
	fwrite(&bfOffBits, sizeof(bfOffBits), 1, pOutPutFile);
}

/*�r�b�g�}�b�v���w�b�_���������ފ֐�*/
static void writeBitmapInfoHeader(BitmapImage* p8bitBitmapImage, FILE* pOutPutFile) {
	/*���w�b�_�̃T�C�Y�̏�������*/
	unsigned long biSize = WINDOWS_BITMAP_INFO_HEADER_SIZE;
	fwrite(&biSize, sizeof(biSize), 1, pOutPutFile);

	unsigned long width = p8bitBitmapImage->width;
	long height = p8bitBitmapImage->height;

	/*�摜�̕��ƍ����̏�������*/
	fwrite(&(width), sizeof(width), 1, pOutPutFile);
	fwrite(&(height), sizeof(height), 1, pOutPutFile);

	/*�v���[�����̏�������*/
	unsigned short biPlanes = BI_PLANES_VALUE;
	fwrite(&biPlanes, sizeof(biPlanes), 1, pOutPutFile);

	/*1��f������̃f�[�^�T�C�Y����������*/
	unsigned short biBitCount = p8bitBitmapImage->bitCount;
	fwrite(&biBitCount, sizeof(biBitCount), 1, pOutPutFile);

	/*���k�`������������*/
	unsigned long biCompression = p8bitBitmapImage->compression;
	fwrite(&biCompression, sizeof(biCompression), 1, pOutPutFile);

	/*�摜�f�[�^���̃T�C�Y�̌v�Z*/
	unsigned long pictureDataSize = (p8bitBitmapImage->width + calcNumberOfPadding(p8bitBitmapImage)) * abs(p8bitBitmapImage->height);

	/*�摜�f�[�^���̃T�C�Y����������*/
	fwrite(&pictureDataSize, sizeof(pictureDataSize), 1, pOutPutFile);

	/*�𑜓x����������*/
	unsigned long biXPixPerMeter = BI_X_PELS_PER_METER_VALUE;
	unsigned long biYPixPerMeter = BI_Y_PELS_PER_METER_VALUE;
	fwrite(&biXPixPerMeter, sizeof(biXPixPerMeter), 1, pOutPutFile);
	fwrite(&biYPixPerMeter, sizeof(biYPixPerMeter), 1, pOutPutFile);

	/*�p���b�g���̏�������*/
	unsigned long biClrUsed = BI_CLR_8BIT;
	fwrite(&biClrUsed, sizeof(biClrUsed), 1, pOutPutFile);

	/*�d�v�ȃp���b�g�̃C���f�b�N�X����������*/
	unsigned long biClrImportant = BI_CLR_IMPORTANT_VALUE;
	fwrite(&biClrImportant, sizeof(biClrImportant), 1, pOutPutFile);
}

/*8bit�O���[�X�P�[���̃J���[�p���b�h���������ފ֐�*/
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

/*8bit�r�b�g�}�b�v�̉摜�f�[�^���������ފ֐�*/
static void writeImageDataOf8bitBitmap(BitmapImage* p8bitBitmapImage, FILE* pOutPutFile) {
	char* pImageData = p8bitBitmapImage->pImageData;
	unsigned long width = p8bitBitmapImage->width;
	unsigned long height = abs(p8bitBitmapImage->height);

	/*�p�f�B���O�����v�Z*/
	int numberOfPadding = calcNumberOfPadding(p8bitBitmapImage);

	for (unsigned long h = 0; h < height; h++) {
		/*�P���̉摜�f�[�^����������*/
		fwrite(pImageData + width * h, width, 1, pOutPutFile);

		/*�p�f�B���O����������*/
		for (int padding = 0; padding < numberOfPadding; padding++) {
			fputc(0x00, pOutPutFile);
		}
	}
}

/*24bit�r�b�g�}�b�v�t�@�C���́A�p�f�B���O���������摜�f�[�^����������ɓǂݍ��݁A
���̃|�C���^���A�����Ƃ��ēn����pBitmapImage�̃|�C���^�ϐ�pImageData�֑������֐�.*/
static MY_ERROR_CODE Load24bitImageData(BitmapImage* pBitmapImage, FILE* pFile) {

	MY_ERROR_CODE ret = SUCCESS;

	/*24bit������*/
	if (pBitmapImage->bitCount != 24) {
		printErrorMessage(IS_NOT_24BIT, __FILE__, __LINE__);
		ret = IS_NOT_24BIT;
		goto END;
	}

	/*�p�f�B���O�����v�Z*/
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

		/*1���̉摜�f�[�^��ǂݍ���*/
		fread(p24bitImageData + h * bytesPerWidth, sizeof(char), bytesPerWidth, pFile);

		/*�t�@�C���̈ʒu�w��q���p�f�B���O�̕��������炷*/
		fseek(pFile, numberOfPadding, SEEK_CUR);
	}

	pBitmapImage->pImageData = p24bitImageData;

END:
	return ret;
}

/*8bit�r�b�g�}�b�v�t�@�C���́A�p�f�B���O���������摜�f�[�^����������ɓǂݍ��݁A
���̃|�C���^���A�����Ƃ��ēn����pBitmapImage�̃|�C���^�ϐ�pImageData�֑������֐�.*/
static MY_ERROR_CODE Load8bitImageData(BitmapImage* pBitmapImage, FILE* pFile) {

	MY_ERROR_CODE ret = SUCCESS;

	/*8bit������*/
	if (pBitmapImage->bitCount != 8) {
		printErrorMessage(IS_NOT_8BIT, __FILE__, __LINE__);
		ret = IS_NOT_8BIT;
		goto END;
	}

	/*�p�f�B���O�����v�Z*/
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

		/*1���̉摜�f�[�^��ǂݍ���*/
		fread(p8bitImageData + h * bytesPerWidth, sizeof(char), bytesPerWidth, pFile);

		/*�t�@�C���̈ʒu�w��q���p�f�B���O�̕��������炷*/
		fseek(pFile, numberOfPadding, SEEK_CUR);
	}

	pBitmapImage->pImageData = p8bitImageData;

END:
	return ret;
}


/*1�����Ƃ̃p�f�B���O�������߂�֐�*/
static int calcNumberOfPadding(BitmapImage* pBitmapImage) {
	int ret = 0;
	int bytesPerWidth = pBitmapImage->width * (pBitmapImage->bitCount / 8);
	if (bytesPerWidth % 4 != 0)
	{
		ret = (4 - bytesPerWidth % 4);
	}
	return ret;
}

/*����pFile�Ƃ��ēn���ꂽ�t�@�C����bitmap�����肷��֐�*/
static MY_BOOL IsBitmapFile(FILE* pFile) {

	MY_BOOL ret = MY_FALSE;

	BITMAPFILEHEADER bitmapFileHeader = { .bfType = 0,.bfSize = 0,.bfReserved1 = 0,.bfReserved2 = 0,.bfOffBits = 0 };

	/*�t�@�C���w�b�_�̏����擾*/
	fread(&bitmapFileHeader, sizeof(BITMAPFILEHEADER), 1, pFile);

	if (strncmp("BM", (char*)&(bitmapFileHeader.bfType), 2)) {
		/*�t�@�C���w�b�_�̃t�@�C���^�C�v��BM�łȂ������ۂ̏���*/
		printErrorMessage(IS_NOT_BITMAP, __FILE__, __LINE__);
		goto END;
	}

	ret = MY_TRUE;

END:
	return ret;
}

/*�G���[���b�Z�[�W��\������֐�*/
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

