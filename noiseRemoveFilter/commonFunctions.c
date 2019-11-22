#include "commonFunctions.h"


/*
�����񌋍���strcat����p����ƃo�b�t�@�I�[�o�[�t���[�̊댯�������邽�߁A���삵���֐�.
����c1�̌��c2���������āA��������������̐擪�̃|�C���^��Ԃ�.
����c1,c2 : �������镶���̐擪�̃|�C���^
*/
char* strJoin(char* c1, char* c2) {

	/*�A�����镶����̕�����('\0'���܂܂Ȃ�)�����ꂼ��擾*/
	size_t length1 = strlen(c1);
	size_t length2 = strlen(c2);

	/*�A����̕�������i�[����̈�̊m��*/
	char* ret = (char*)malloc(length1 + length2 + 1);
	memset(ret, 0x00, length1 + length2 + 1);

	/*�������A��*/
	snprintf(ret, length1 + length2 + 1, "%s%s", c1, c2);

	return ret;
}

/*qsort�֐��ɓn����r�֐�*/
int compareUnsignedChar(const void* a, const void* b) {
	unsigned char aUC = *(unsigned char*)a;
	unsigned char bUC = *(unsigned char*)b;

	if (aUC < bUC) {
		return -1;
	}
	else if (aUC > bUC) {
		return 1;
	}
	else {
		return 0;
	}

}