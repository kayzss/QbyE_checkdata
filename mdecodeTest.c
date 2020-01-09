#include "alsaInterface.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

#include "../include/mWakeupInterface.h"

static void getExtension(char *filename, char *ext) {
	ext = strrchr(filename, '.');
	if (ext) {
		*ext = '\0';
		ext++;
	}
	printf("name=%s \n", filename);
	printf("ext-name=%s \n", ext);

	return ;
}

static int findKey(char *keys, char **text, int number) {
	char *key = strrchr(keys, '_');
	if (key == NULL) {
		return -1;
	}
	char *end = strrchr(keys, '.');
	if (end == NULL) {
		return -1;
	}
	*end = '\0';
	key++;
	int i = 0;
	for (; i < number; i++) {
		int keyLen = strlen(key);
		int textLen = strlen(text[i] + 12);
		if (keyLen == textLen && memcmp(key, text[i] + 12, textLen) == 0) {
//			printf(" findKey %s\n",text[i] + 12);
			*end = '.';
			return i;
		}
	}
	*end = '.';
	return -1;
}
int main(int argc, char **argv) {
	if (argc != 6) {
		printf(" mode.txt key.bin text.txt wav log.txt\n");
		return -1;
	}

	FILE *text = fopen(argv[3], "rb");
	fseek(text, 0, SEEK_SET);
	int number = 0;
	int maxLine = 0;
	char *buf = NULL;
	char buffer[1024];
	while ((buf = fgets(buffer, 1024, text)) != NULL) {
		number++;
		//printf("%s\n",buf);
		int lineSize = strlen(buf);
		if (lineSize > maxLine) {
			maxLine = lineSize;
		}
	}
	//printf("number = %d\n",number);
	fseek(text, 0, SEEK_SET);
	char **tx = (char**) malloc(sizeof(char*) * number);
	int i = 0;
	for (; i < number; i++) {
		tx[i] = (char*) malloc(maxLine + 1 + 4 * 3);
		*((int*) (tx[i])) = 0;
		*(((int*) (tx[i])) + 1) = 0;
		*(((int*) (tx[i])) + 2) = 0;
		fgets(tx[i] + 4 * 3, 1024, text);
		if (tx[i][strlen(tx[i] + 4 * 3) + 3 + 8] == '\n') {
			tx[i][strlen(tx[i] + 4 * 3) + 3 + 8] = '\0';
		}
		int *iswakeup = (int*) tx[i];
		//printf("%d %d %d %s\n",iswakeup[0],iswakeup[1],iswakeup[2],tx[i] + 4 * 3);
	}
	fclose(text);

	FILE *modelPath = fopen(argv[1], "rb");
	fseek(modelPath, 0, SEEK_END);
	int length = ftell(modelPath);
	fseek(modelPath, 0, SEEK_SET);
	char *model = (char*) malloc(length);
	fread(model, length, 1, modelPath);
	fclose(modelPath);

	//FILE *templet = fopen("config/keyword60_cut.bin","rb");
	FILE *tempPath = fopen(argv[2], "rb");
	fseek(tempPath, 0, SEEK_END);
	length = ftell(tempPath);
	fseek(tempPath, 0, SEEK_SET);

	char *templates = (char*) malloc(length);
	fread(templates, length, 1, tempPath);
	fclose(tempPath);

	MWakeupHandle handle;
	mWakeupInit(&handle, model, (int*) templates);
	mWakeupStart(handle);
	DIR *dir = opendir(argv[4]);
	struct dirent *ent = NULL;
	char fPath[2048];
	short data[160];
	length = -1;
	int iswakeup = -1;
	int passcount = 0;
	int nowakeup = 0;
	int totalcount = 0;

//	FILE *wakelog = fopen("./wakeLog", "wb");
//	FILE *falog = fopen("./falsealarmLog", "wb");
	FILE *nowakelog = fopen("./nowakeLog", "wb");
//	FILE *outlog = fopen("./outlog", "wb");
	while (NULL != (ent = readdir(dir))) {
//		char *ext_name;
//		getExtension(ent->d_name, ext_name);
//		printf("ext_name: %s \n", ext_name[0]);
		char filename[256];
		memcpy(filename, ent->d_name, sizeof(ent->d_name));
		char *ext = strrchr(filename, '.');
		if (ext) {
			*ext = '\0';
			ext++;
		}
//		printf("name=%s \n", filename);
//		printf("ext-name=%s \n", ext);

		char wavext[] = "wav";
		if (strcmp(ext, wavext) != 0) {
//			printf("continue.. \n");
			continue;
		}
		if (ent->d_type == DT_REG) {
//			printf("zss %s\n",ent->d_name);
			sprintf(fPath, "%s/%s", argv[4], ent->d_name);
			int keyIndex = findKey(ent->d_name, tx, number);
			keyIndex = 0;
//			printf("keyIndex: %d \n", keyIndex);
			if (keyIndex == -1) {
//				fprintf(outlog, "%s \n", fPath);
				continue;
			}
			char *key = tx[keyIndex];
			int *keyWakeup = (int*) key;
			(*keyWakeup)++;
			FILE *fp = fopen(fPath, "rb");
			fseek(fp, 44, SEEK_SET);
			totalcount++;
			while ((length = fread(data, 1, 320, fp)) > 0) {
				int isWakeup = mWakeupDecode(handle, (char*) data, length);
				if (isWakeup >= 0) {
					printf("%s \n", ent->d_name);
					if (isWakeup == keyIndex) {
						printf("%d --> pass: isWakeup = %d\n", totalcount,
								isWakeup);
//						fprintf(wakelog, "%s \n", fPath);
						passcount++;
						(*(keyWakeup + 1))++;
					}
//					else {
//						printf("keyIndex = %d  --> %d \n\n\n", keyIndex, isWakeup);
//						falsecount++;
//						fprintf(falog, "%s  --> %d \n", fPath, isWakeup);
//						key = tx[keyIndex];
//						keyWakeup = (int*) key;
//						(*(keyWakeup + 2))++;
//					}
					isWakeup = -1;
					goto END;

//					mWakeupStop(handle);
//					mWakeupStart(handle);
				}
			}
			nowakeup++;
			printf("no wake -----> %s \n", fPath);
			fprintf(nowakelog, "%s \n", fPath);
			END: fclose(fp);

			mWakeupStop(handle);
			mWakeupStart(handle);
		}
	}

	printf("passcount:%d \n", passcount);
//	printf("falsecount:%d \n", falsecount);
	printf("totalcount:%d \n", totalcount);
	printf("ratio:%f \n", (float) passcount / totalcount);

//	fclose(wakelog);
//	fclose(falog);
	fclose(nowakelog);
//	fclose(outlog);

	FILE *log = fopen(argv[5], "wb");
	int ii = 0;
	for (; ii < number; ii++) {
		char *key = tx[ii];
		int *keyWakeup = (int*) key;
		fprintf(log, "%s %3d %3d %3d %6.3f \n", key + 12, keyWakeup[0],
				keyWakeup[1], keyWakeup[2],
				(float) keyWakeup[1] / (float) keyWakeup[0]);
		printf("%s %3d %3d %3d %6.3f \n", key + 12, keyWakeup[0], keyWakeup[1],
				keyWakeup[2], (float) keyWakeup[1] / (float) keyWakeup[0]);
		free(tx[ii]);
	}
	free(tx);
	fprintf(log, "\n\n\n");
	fprintf(log, "passcount: %d \n", passcount);
	fprintf(log, "nowakeup: %d \n", nowakeup);
	fprintf(log, "totalcount: %d \n", totalcount);
	fprintf(log, "ratio: %6.3f \n", (float) passcount / totalcount);

	fclose(log);
	closedir(dir);

}
