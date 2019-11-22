#ifndef INCLUDE_MY_ERROR_CODES
#define INCLUDE_MY_ERROR_CODES

typedef int MY_ERROR_CODE;

typedef enum MyErrorCode {
	SUCCESS,  /* 異常なし */
	ARGUMENT_ERROR, /*引数エラー*/
	IS_NOT_BITMAP,  /*ファイルの形式がbitmapでない*/
	IS_NULL_POINTER,  /* ヌルポインタが渡された */
	MALLOC_ERROR,  /* メモリの割り当てに失敗 */
	IS_NOT_WINDOWS_BITMAP, /*WindowsBitmap形式でない*/
	IS_NOT_24BIT, /* 24bit形式でない */
	IS_NOT_8BIT, /* 8bit形式でない */
	UNKNOWN_FILE_TUPE, /*想定していないファイルタイプ*/
	FILE_OPEN_ERROR /*ファイルのオープンに失敗*/
}MyErrorCode;

#endif