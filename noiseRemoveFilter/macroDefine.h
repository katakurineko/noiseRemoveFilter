#ifndef INCLUDE_MACRO_DEFINE
#define INCLUDE_MACRO_DEFINE

/********ビットマップファイルのヘッダ関連********/

/*windowsビットマップの情報ヘッダサイズ*/
#define WINDOWS_BITMAP_INFO_HEADER_SIZE (40)

/*プレーン数*/
#define BI_PLANES_VALUE (1)

/*縦,横方向それぞれの解像度*/
#define BI_X_PELS_PER_METER_VALUE (0)
#define BI_Y_PELS_PER_METER_VALUE (0)

/*8bit形式の場合のパレット数*/
#define BI_CLR_8BIT (256)

/*重要なパレッドのインデックス(0はすべて重要の意味)*/
#define BI_CLR_IMPORTANT_VALUE (0)

/*8bit形式windowsBitmapファイルの、画像データまでのオフセットサイズ
(ファイルヘッダ)14 + (情報ヘッダ)40 + (カラーパレッド)256*4 */
#define OFFSET_TO_IMAGE_DATA_8BIT (1078)

/*圧縮形式(無圧縮)*/
#define NOT_COMPRESSION (0)

/*カラーパレッドの予約領域*/
#define COLOR_PALETTE_RESERVED (0)

/************************************************/

#endif