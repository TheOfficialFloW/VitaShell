#ifndef AMCTRL_H
#define AMCTRL_H

typedef struct {
	int type;
	u8 key[16];
	u8 pad[16];
	int pad_size;
} MAC_KEY;

typedef struct
{
	u32 type;
	u32 seed;
	u8 key[16];
} CIPHER_KEY;

// type:
//	  2: use fuse id
//	  3: use fixed key. MAC need encrypt again
int sceDrmBBMacInit(MAC_KEY *mkey, int type);
int sceDrmBBMacUpdate(MAC_KEY *mkey, u8 *buf, int size);
int sceDrmBBMacFinal(MAC_KEY *mkey, u8 *buf, u8 *vkey);
int sceDrmBBMacFinal2(MAC_KEY *mkey, u8 *out, u8 *vkey);

int bbmac_build_final2(int type, u8 *mac);
int bbmac_getkey(MAC_KEY *mkey, u8 *bbmac, u8 *vkey);
int bbmac_forge(MAC_KEY *mkey, u8 *bbmac, u8 *vkey, u8 *buf);

// type: 1 use fixed key
//	   2 use fuse id
// mode: 1 for encrypt
//	   2 for decrypt
int sceDrmBBCipherInit(CIPHER_KEY *ckey, int type, int mode, u8 *header_key, u8 *version_key, u32 seed);
int sceDrmBBCipherUpdate(CIPHER_KEY *ckey, u8 *data, int size);
int sceDrmBBCipherFinal(CIPHER_KEY *ckey);

// npdrm.prx
int sceNpDrmGetFixedKey(u8 *key, char *npstr, int type);

#endif
