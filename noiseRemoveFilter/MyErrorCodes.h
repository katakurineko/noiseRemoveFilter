#ifndef INCLUDE_MY_ERROR_CODES
#define INCLUDE_MY_ERROR_CODES

typedef int MY_ERROR_CODE;

typedef enum MyErrorCode {
	SUCCESS,  /* �ُ�Ȃ� */
	ARGUMENT_ERROR, /*�����G���[*/
	IS_NOT_BITMAP,  /*�t�@�C���̌`����bitmap�łȂ�*/
	IS_NULL_POINTER,  /* �k���|�C���^���n���ꂽ */
	MALLOC_ERROR,  /* �������̊��蓖�ĂɎ��s */
	IS_NOT_WINDOWS_BITMAP, /*WindowsBitmap�`���łȂ�*/
	IS_NOT_24BIT, /* 24bit�`���łȂ� */
	IS_NOT_8BIT, /* 8bit�`���łȂ� */
	UNKNOWN_FILE_TUPE, /*�z�肵�Ă��Ȃ��t�@�C���^�C�v*/
	FILE_OPEN_ERROR /*�t�@�C���̃I�[�v���Ɏ��s*/
}MyErrorCode;

#endif