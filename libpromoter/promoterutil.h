#ifndef _PSP2_PROMOTERUTIL_H_
#define _PSP2_PROMOTERUTIL_H_

#include <psp2/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ScePromoterUtilityLAUpdate {
	char titleid[12]; // target app
	char path[128]; // directory of extracted LA update data
} ScePromoterUtilityLAUpdate;

int scePromoterUtilityInit(void);
int scePromoterUtilityExit(void);
int scePromoterUtilityDeletePkg(void *unk);
int scePromoterUtilityUpdateLiveArea(ScePromoterUtilityLAUpdate *args);
int scePromoterUtilityPromotePkg(char *path, int unk);
int scePromoterUtilityPromotePkgWithRif(const char *path, int unk);
int scePromoterUtilityGetState(int *state);
int scePromoterUtilityGetResult(int *res);

#ifdef __cplusplus
}
#endif

#endif /* _PSP2_PROMOTERUTIL_H_ */