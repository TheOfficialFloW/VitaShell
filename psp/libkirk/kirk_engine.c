/* 
	KIRK ENGINE CODE
	Thx for coyotebean, Davee, hitchhikr, kgsws, Mathieulh, SilverSpring, Proxima
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "crypto.h"
#include "ecdsa.h"

/* ------------------------- KEY VAULT ------------------------- */



// ECDSA param of kirk1
ECDSA_PARAM ecdsa_kirk1 = {
	.p  = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x00,0x00,0x00,0x01,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF},
	.a  = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x00,0x00,0x00,0x01,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFC},
	.b  = {0x65,0xD1,0x48,0x8C,0x03,0x59,0xE2,0x34,0xAD,0xC9,0x5B,0xD3,0x90,0x80,0x14,0xBD,0x91,0xA5,0x25,0xF9},
	.N  = {0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x00,0x01,0xb5,0xc6,0x17,0xf2,0x90,0xea,0xe1,0xdb,0xad,0x8f},
	.Gx = {0x22,0x59,0xAC,0xEE,0x15,0x48,0x9C,0xB0,0x96,0xA8,0x82,0xF0,0xAE,0x1C,0xF9,0xFD,0x8E,0xE5,0xF8,0xFA},
	.Gy = {0x60,0x43,0x58,0x45,0x6D,0x0A,0x1C,0xB2,0x90,0x8D,0xE9,0x0F,0x27,0xD7,0x5C,0x82,0xBE,0xC1,0x08,0xC0},
};

// ECDSA private key of kirk1
u8 priv_key_kirk1[20]  = {0xF3,0x92,0xE2,0x64,0x90,0xB8,0x0F,0xD8,0x89,0xF2,0xD9,0x72,0x2C,0x1F,0x34,0xD7,0x27,0x4F,0x98,0x3D};

// ECDSA public key of kirk1
u8 pub_key_kirk1_x[20] = {0xED,0x9C,0xE5,0x82,0x34,0xE6,0x1A,0x53,0xC6,0x85,0xD6,0x4D,0x51,0xD0,0x23,0x6B,0xC3,0xB5,0xD4,0xB9 };
u8 pub_key_kirk1_y[20] = {0x04,0x9D,0xF1,0xA0,0x75,0xC0,0xE0,0x4F,0xB3,0x44,0x85,0x8B,0x61,0xB7,0x9B,0x69,0xA6,0x3D,0x2C,0x39 };


u8 kirk1_ec_m_header[20] = {0x7D,0x7E,0x46,0x85,0x4D,0x07,0xFA,0x0B,0xC6,0xE8,0xED,0x62,0x99,0x89,0x26,0x14,0x39,0x5F,0xEA,0xFC};
u8 kirk1_ec_r_header[20] = {0x66,0x4f,0xe1,0xf2,0xe9,0xd6,0x63,0x36,0xf7,0x33,0x0b,0xca,0xb9,0x55,0x6d,0xb6,0xeb,0xe8,0x05,0xdc};

u8 kirk1_ec_m_data[20] = {0x15,0xee,0x16,0x24,0x12,0x09,0x58,0x16,0x0f,0x8b,0x1a,0x21,0x33,0x7a,0x38,0xf8,0x29,0xf7,0x2e,0x58};
u8 kirk1_ec_r_data[20] = {0xcd,0xe3,0x88,0xa6,0x3c,0x1d,0x57,0xdc,0x5e,0x94,0xee,0xac,0x2e,0x6c,0x9f,0x2e,0x81,0xc7,0x1c,0x58};

// ECDSA param of applations
ECDSA_PARAM ecdsa_app = {
	.p  = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x00,0x00,0x00,0x01,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF},
	.a  = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x00,0x00,0x00,0x01,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFC},
	.b  = {0xA6,0x8B,0xED,0xC3,0x34,0x18,0x02,0x9C,0x1D,0x3C,0xE3,0x3B,0x9A,0x32,0x1F,0xCC,0xBB,0x9E,0x0F,0x0B},
	.N  = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFE,0xFF,0xFF,0xB5,0xAE,0x3C,0x52,0x3E,0x63,0x94,0x4F,0x21,0x27},
	.Gx = {0x12,0x8E,0xC4,0x25,0x64,0x87,0xFD,0x8F,0xDF,0x64,0xE2,0x43,0x7B,0xC0,0xA1,0xF6,0xD5,0xAF,0xDE,0x2C},
	.Gy = {0x59,0x58,0x55,0x7E,0xB1,0xDB,0x00,0x12,0x60,0x42,0x55,0x24,0xDB,0xC3,0x79,0xD5,0xAC,0x5F,0x4A,0xDF},
};

// ECDSA private key of EDATA
u8 priv_key_edata[20] = {0xe5,0xc4,0xd0,0xa8,0x24,0x9a,0x6f,0x27,0xe5,0xe0,0xc9,0xd5,0x34,0xf4,0xda,0x15,0x22,0x3f,0x42,0xad};


// AES key for kirk1
u8 kirk1_key[16] =  {0x98, 0xC9, 0x40, 0x97, 0x5C, 0x1D, 0x10, 0xE8, 0x7F, 0xE6, 0x0E, 0xA3, 0xFD, 0x03, 0xA8, 0xBA};

// AES key for kirk4/7
u8 kirk7_key03[] = {0x98, 0x02, 0xC4, 0xE6, 0xEC, 0x9E, 0x9E, 0x2F, 0xFC, 0x63, 0x4C, 0xE4, 0x2F, 0xBB, 0x46, 0x68};
u8 kirk7_key04[] = {0x99, 0x24, 0x4C, 0xD2, 0x58, 0xF5, 0x1B, 0xCB, 0xB0, 0x61, 0x9C, 0xA7, 0x38, 0x30, 0x07, 0x5F};
u8 kirk7_key05[] = {0x02, 0x25, 0xD7, 0xBA, 0x63, 0xEC, 0xB9, 0x4A, 0x9D, 0x23, 0x76, 0x01, 0xB3, 0xF6, 0xAC, 0x17};
u8 kirk7_key0C[] = {0x84, 0x85, 0xC8, 0x48, 0x75, 0x08, 0x43, 0xBC, 0x9B, 0x9A, 0xEC, 0xA7, 0x9C, 0x7F, 0x60, 0x18};
u8 kirk7_key0D[] = {0xB5, 0xB1, 0x6E, 0xDE, 0x23, 0xA9, 0x7B, 0x0E, 0xA1, 0x7C, 0xDB, 0xA2, 0xDC, 0xDE, 0xC4, 0x6E};
u8 kirk7_key0E[] = {0xC8, 0x71, 0xFD, 0xB3, 0xBC, 0xC5, 0xD2, 0xF2, 0xE2, 0xD7, 0x72, 0x9D, 0xDF, 0x82, 0x68, 0x82};
u8 kirk7_key0F[] = {0x0A, 0xBB, 0x33, 0x6C, 0x96, 0xD4, 0xCD, 0xD8, 0xCB, 0x5F, 0x4B, 0xE0, 0xBA, 0xDB, 0x9E, 0x03};
u8 kirk7_key10[] = {0x32, 0x29, 0x5B, 0xD5, 0xEA, 0xF7, 0xA3, 0x42, 0x16, 0xC8, 0x8E, 0x48, 0xFF, 0x50, 0xD3, 0x71};
u8 kirk7_key11[] = {0x46, 0xF2, 0x5E, 0x8E, 0x4D, 0x2A, 0xA5, 0x40, 0x73, 0x0B, 0xC4, 0x6E, 0x47, 0xEE, 0x6F, 0x0A};
u8 kirk7_key12[] = {0x5D, 0xC7, 0x11, 0x39, 0xD0, 0x19, 0x38, 0xBC, 0x02, 0x7F, 0xDD, 0xDC, 0xB0, 0x83, 0x7D, 0x9D};
u8 kirk7_key38[] = {0x12, 0x46, 0x8D, 0x7E, 0x1C, 0x42, 0x20, 0x9B, 0xBA, 0x54, 0x26, 0x83, 0x5E, 0xB0, 0x33, 0x03};
u8 kirk7_key39[] = {0xC4, 0x3B, 0xB6, 0xD6, 0x53, 0xEE, 0x67, 0x49, 0x3E, 0xA9, 0x5F, 0xBC, 0x0C, 0xED, 0x6F, 0x8A};
u8 kirk7_key3A[] = {0x2C, 0xC3, 0xCF, 0x8C, 0x28, 0x78, 0xA5, 0xA6, 0x63, 0xE2, 0xAF, 0x2D, 0x71, 0x5E, 0x86, 0xBA};
u8 kirk7_key4B[] = {0x0C, 0xFD, 0x67, 0x9A, 0xF9, 0xB4, 0x72, 0x4F, 0xD7, 0x8D, 0xD6, 0xE9, 0x96, 0x42, 0x28, 0x8B}; //1.xx game eboot.bin
u8 kirk7_key53[] = {0xAF, 0xFE, 0x8E, 0xB1, 0x3D, 0xD1, 0x7E, 0xD8, 0x0A, 0x61, 0x24, 0x1C, 0x95, 0x92, 0x56, 0xB6};
u8 kirk7_key57[] = {0x1C, 0x9B, 0xC4, 0x90, 0xE3, 0x06, 0x64, 0x81, 0xFA, 0x59, 0xFD, 0xB6, 0x00, 0xBB, 0x28, 0x70};
u8 kirk7_key5D[] = {0x11, 0x5A, 0x5D, 0x20, 0xD5, 0x3A, 0x8D, 0xD3, 0x9C, 0xC5, 0xAF, 0x41, 0x0F, 0x0F, 0x18, 0x6F};
u8 kirk7_key63[] = {0x9C, 0x9B, 0x13, 0x72, 0xF8, 0xC6, 0x40, 0xCF, 0x1C, 0x62, 0xF5, 0xD5, 0x92, 0xDD, 0xB5, 0x82};
u8 kirk7_key64[] = {0x03, 0xB3, 0x02, 0xE8, 0x5F, 0xF3, 0x81, 0xB1, 0x3B, 0x8D, 0xAA, 0x2A, 0x90, 0xFF, 0x5E, 0x61};


/* ------------------------- INTERNAL STUFF ------------------------- */

typedef struct header_keys
{
	u8 AES[16];
	u8 CMAC[16];
}header_keys;

AES_ctx aes_kirk1;

int is_kirk_initialized = 0;



/* ------------------------- IMPLEMENTATION ------------------------- */

/*****************************************************************************/
/* KIRK initial															  */
/*****************************************************************************/


int kirk_init()
{
	AES_set_key(&aes_kirk1, kirk1_key, 128);
	is_kirk_initialized = 1;
	srand(time(0));
	return KIRK_OPERATION_SUCCESS;
}

/*****************************************************************************/
/* KIRK 0x01																 */
/*****************************************************************************/

int kirk_CMD0(void* outbuff, void* inbuff, int size)
{
	KIRK_CMD1_HEADER *header;
	header_keys *keys;
	AES_ctx k1, cmac_key;
	int chk_size;
	u8 cmac_header_hash[16];
	u8 cmac_data_hash[16];

	if(is_kirk_initialized == 0)
		kirk_init();

	header = (KIRK_CMD1_HEADER*)outbuff;
	memcpy(outbuff, inbuff, size);

	if(header->mode != KIRK_MODE_CMD1)
		return KIRK_INVALID_MODE;

	//0-15 AES key, 16-31 CMAC key
	keys = (header_keys *)outbuff;

	//Make sure data is 16 aligned
	chk_size = header->data_size;
	if(chk_size%16)
		chk_size += 16-(chk_size%16);

	//Encrypt data
	AES_set_key(&k1, keys->AES, 128);
	AES_cbc_encrypt(&k1, inbuff+sizeof(KIRK_CMD1_HEADER)+header->data_offset, outbuff+sizeof(KIRK_CMD1_HEADER)+header->data_offset, chk_size);

	if(header->ecdsa==1){
		//ECDSA hash
		u8 *sign_s, *sign_r;
		u8 sign_e[20];

		ecdsa_set_curve(&ecdsa_kirk1);
		ecdsa_set_priv(priv_key_kirk1);

		SHA1(outbuff+0x60, 0x30, sign_e);
		sign_r = outbuff+0x10;
		sign_s = outbuff+0x10+0x14;
		ecdsa_sign(sign_e, sign_r, sign_s, NULL);

		SHA1(outbuff+0x60, 0x30+chk_size+header->data_offset, sign_e);
		sign_r = outbuff+0x10+0x28;
		sign_s = outbuff+0x10+0x3C;
		ecdsa_sign(sign_e, sign_r, sign_s, NULL);


		//Encrypt keys
		AES_cbc_encrypt(&aes_kirk1, inbuff, outbuff, 16);
	}else{
		//CMAC hash
		AES_set_key(&cmac_key, keys->CMAC, 128);

		AES_CMAC(&cmac_key, outbuff+0x60, 0x30, cmac_header_hash);
		AES_CMAC(&cmac_key, outbuff+0x60, 0x30+chk_size+header->data_offset, cmac_data_hash);

		memcpy(header->CMAC_header_hash, cmac_header_hash, 16);
		memcpy(header->CMAC_data_hash, cmac_data_hash, 16);

		//Encrypt keys
		AES_cbc_encrypt(&aes_kirk1, inbuff, outbuff, 16*2);
	}

	return KIRK_OPERATION_SUCCESS;
}

int kirk_CMD1(void* outbuff, void* inbuff, int size)
{
	KIRK_CMD1_HEADER* header;
	header_keys keys; //0-15 AES key, 16-31 CMAC key
	AES_ctx k1;
	int retv;

	if(is_kirk_initialized == 0)
		kirk_init();

	header = (KIRK_CMD1_HEADER*)inbuff;
	if(header->mode != KIRK_MODE_CMD1)
		return KIRK_INVALID_MODE;

	if(size < header->data_size)
		size = header->data_size;
	size = (size+15)&~15;

	//decrypt AES & CMAC key to temp buffer
	AES_cbc_decrypt(&aes_kirk1, inbuff, (u8*)&keys, 16*2);

	if(header->ecdsa==0){
		retv = kirk_CMD10(inbuff, size);
		if(retv != KIRK_OPERATION_SUCCESS)
			return retv;
	}else if(header->ecdsa==1){
		u8 *sign_s, *sign_r;
		u8 sign_e[20];

		ecdsa_set_curve(&ecdsa_kirk1);
		ecdsa_set_pub(pub_key_kirk1_x, pub_key_kirk1_y);

		SHA1(inbuff+0x60, 0x30, sign_e);
		sign_r = inbuff+0x10;
		sign_s = inbuff+0x10+0x14;
		retv = ecdsa_verify(sign_e, sign_r, sign_s);
		if(retv){
			return KIRK_HEADER_HASH_INVALID;
		}

		size = 0x30+header->data_size+header->data_offset;
		size = (size+15)&~15;
		SHA1(inbuff+0x60, size, sign_e);
		sign_r = inbuff+0x10+0x28;
		sign_s = inbuff+0x10+0x3C;
		retv = ecdsa_verify(sign_e, sign_r, sign_s);
		if(retv){
			return KIRK_HEADER_HASH_INVALID;
		}
	}

	AES_set_key(&k1, keys.AES, 128);
	AES_cbc_decrypt(&k1, inbuff+sizeof(KIRK_CMD1_HEADER)+header->data_offset, outbuff, header->data_size);

	return KIRK_OPERATION_SUCCESS;
}

/*****************************************************************************/
/* KIRK 0x04 0x05 0x06 0x07 0x08 0x09										*/
/*****************************************************************************/


u8* kirk_4_7_get_key(int key_type)
{
	switch(key_type)
	{
		case(0x03): return kirk7_key03; break;
		case(0x04): return kirk7_key04; break;
		case(0x05): return kirk7_key05; break;
		case(0x0C): return kirk7_key0C; break;
		case(0x0D): return kirk7_key0D; break;
		case(0x0E): return kirk7_key0E; break;
		case(0x0F): return kirk7_key0F; break;
		case(0x10): return kirk7_key10; break;
		case(0x11): return kirk7_key11; break;
		case(0x12): return kirk7_key12; break;
		case(0x38): return kirk7_key38; break;
		case(0x39): return kirk7_key39; break;
		case(0x3A): return kirk7_key3A; break;
		case(0x4B): return kirk7_key4B; break;
		case(0x53): return kirk7_key53; break;
		case(0x57): return kirk7_key57; break;
		case(0x5D): return kirk7_key5D; break;
		case(0x63): return kirk7_key63; break;
		case(0x64): return kirk7_key64; break;
		default:
			return NULL;
			//need to get the real error code for that, placeholder now :)
	}
}

int kirk_CMD4(void* outbuff, void* inbuff, int size)
{
	KIRK_AES128CBC_HEADER *header;
	AES_ctx aesKey;
	u8 *key;

	if(is_kirk_initialized == 0)
		kirk_init();
	
	header = (KIRK_AES128CBC_HEADER*)inbuff;

	if(header->mode != KIRK_MODE_ENCRYPT_CBC)
		return KIRK_INVALID_MODE;

	if(header->data_size==0)
		return KIRK_DATA_SIZE_ZERO;
	
	key = kirk_4_7_get_key(header->keyseed);
	if(key==NULL)
		return KIRK_INVALID_SIZE;

	//Set the key
	AES_set_key(&aesKey, key, 128);
 	AES_cbc_encrypt(&aesKey, inbuff+0x14, outbuff+0x14, header->data_size);

	memcpy(outbuff, inbuff, 0x14);
	*(u32*)outbuff = KIRK_MODE_DECRYPT_CBC;

	return KIRK_OPERATION_SUCCESS;
}

int kirk_CMD7(void* outbuff, void* inbuff, int size)
{
	KIRK_AES128CBC_HEADER *header;
	AES_ctx aesKey;
	u8 *key;

	if(is_kirk_initialized == 0)
		kirk_init();
	
	header = (KIRK_AES128CBC_HEADER*)inbuff;

	if(header->mode != KIRK_MODE_DECRYPT_CBC)
		return KIRK_INVALID_MODE;

	if(header->data_size==0)
		return KIRK_DATA_SIZE_ZERO;

	key = kirk_4_7_get_key(header->keyseed);
	if(key==NULL)
		return KIRK_INVALID_SIZE;

	size = size < header->data_size ? size : header->data_size;

	//Set the key
	AES_set_key(&aesKey, key, 128);
 	AES_cbc_decrypt(&aesKey, inbuff+0x14, outbuff, header->data_size);

	return KIRK_OPERATION_SUCCESS;
}

/*****************************************************************************/
/* KIRK 0x0A																 */
/*****************************************************************************/

int kirk_CMD10(void* inbuff, int insize)
{
	KIRK_CMD1_HEADER *header;
	header_keys keys; //0-15 AES key, 16-31 CMAC key
	AES_ctx cmac_key;
	u8 cmac_header_hash[16];
	u8 cmac_data_hash[16];
	int chk_size;

	if(is_kirk_initialized == 0)
		kirk_init();
	
	header = (KIRK_CMD1_HEADER*)inbuff;

	if(!(header->mode == KIRK_MODE_CMD1 || header->mode == KIRK_MODE_CMD2 || header->mode == KIRK_MODE_CMD3))
		return KIRK_INVALID_MODE;

	if(header->data_size==0)
		return KIRK_DATA_SIZE_ZERO;
	
	if(header->mode == KIRK_MODE_CMD1) {
		//decrypt AES & CMAC key to temp buffer
		AES_cbc_decrypt(&aes_kirk1, inbuff, (u8*)&keys, 32);

		AES_set_key(&cmac_key, keys.CMAC, 128);
		AES_CMAC(&cmac_key, inbuff+0x60, 0x30, cmac_header_hash);
	
		//Make sure data is 16 aligned
		chk_size = header->data_size;
		if(chk_size % 16)
			chk_size += 16 - (chk_size % 16);

		AES_CMAC(&cmac_key, inbuff+0x60, 0x30 + chk_size + header->data_offset, cmac_data_hash);
	
		if(memcmp(cmac_header_hash, header->CMAC_header_hash, 16) != 0) {
			printf("header hash invalid\n");
			return KIRK_HEADER_HASH_INVALID;
		}

		if(memcmp(cmac_data_hash, header->CMAC_data_hash, 16) != 0) {
			printf("data hash invalid\n");
			return KIRK_DATA_HASH_INVALID;
		}

		return KIRK_OPERATION_SUCCESS;
	}

	//Checks for cmd 2 & 3 not included right now
	return KIRK_SIG_CHECK_INVALID;
}

/*****************************************************************************/
/* KIRK 0x0B																 */
/*****************************************************************************/

int kirk_CMD11(void* outbuff, void* inbuff, int size)
{
	int data_size;

	if(is_kirk_initialized == 0)
		kirk_init();

	data_size = *(int*)(inbuff);
	if(data_size==0 || size==0)
		return KIRK_DATA_SIZE_ZERO;

	size = size<data_size ? size : data_size;
	SHA1(inbuff+4, size, outbuff);

	return KIRK_OPERATION_SUCCESS;
}

/*****************************************************************************/
/* KIRK 0x0C: generate priv and pub										  */
/*****************************************************************************/


int kirk_CMD12(void *outbuff, int size)
{
	u8 *priv_key = (u8*)outbuff;
	//u8 *pub_key = (u8*)outbuff+20;

	if(size!=0x3c)
		return KIRK_INVALID_SIZE;

	ecdsa_set_curve(&ecdsa_app);

	kirk_CMD14(priv_key, 20);



	return 0;
}


/*****************************************************************************/
/* KIRK 0x0E																 */
/*****************************************************************************/

int kirk_CMD14(void* outbuff, int size)
{
	int i;
	u8* buf = (u8*)outbuff;

	if(is_kirk_initialized == 0)
		kirk_init();

	for(i=0; i<size; i++){
		buf[i] = rand()%256;
	}

	return KIRK_OPERATION_SUCCESS;
}

/*****************************************************************************/
/* sceUtilsBufferCopyWithRange											   */
/*****************************************************************************/


int sceUtilsBufferCopyWithRange(void* outbuff, int outsize, void* inbuff, int insize, int cmd)
{
	switch(cmd)
	{
		case KIRK_CMD_DECRYPT_PRIVATE: 
			 if(insize % 16) return SUBCWR_NOT_16_ALGINED;
			 int ret = kirk_CMD1(outbuff, inbuff, insize); 
			 if(ret == KIRK_HEADER_HASH_INVALID) return SUBCWR_HEADER_HASH_INVALID;
			 return ret;
			 break;
		case KIRK_CMD_ENCRYPT_IV_0: return kirk_CMD4(outbuff, inbuff, insize); break;
		case KIRK_CMD_DECRYPT_IV_0: return kirk_CMD7(outbuff, inbuff, insize); break;
		case KIRK_CMD_PRIV_SIG_CHECK: return kirk_CMD10(inbuff, insize); break;
		case KIRK_CMD_SHA1_HASH: return kirk_CMD11(outbuff, inbuff, insize); break;
	}
	return -1;
}

int kirk_decrypt_keys(u8 *keys, void *inbuff)
{
	AES_cbc_decrypt(&aes_kirk1, inbuff, (u8*)keys, 16*2); //decrypt AES & CMAC key to temp buffer
	return 0;
}

int kirk_forge(u8* inbuff, int insize)
{
   KIRK_CMD1_HEADER* header = (KIRK_CMD1_HEADER*)inbuff;
   AES_ctx cmac_key;
   u8 cmac_header_hash[16];
   u8 cmac_data_hash[16];
   int chk_size;

   if(is_kirk_initialized == 0) return KIRK_NOT_INITIALIZED;
   if(!(header->mode == KIRK_MODE_CMD1 || header->mode == KIRK_MODE_CMD2 || header->mode == KIRK_MODE_CMD3)) return KIRK_INVALID_MODE;
   if(header->data_size == 0) return KIRK_DATA_SIZE_ZERO;

   if(header->mode == KIRK_MODE_CMD1){
	  header_keys keys; //0-15 AES key, 16-31 CMAC key

	  AES_cbc_decrypt(&aes_kirk1, inbuff, (u8*)&keys, 32); //decrypt AES & CMAC key to temp buffer
	  AES_set_key(&cmac_key, keys.CMAC, 128);
	  AES_CMAC(&cmac_key, inbuff+0x60, 0x30, cmac_header_hash);
	  if(memcmp(cmac_header_hash, header->CMAC_header_hash, 16) != 0) return KIRK_HEADER_HASH_INVALID;

	  //Make sure data is 16 aligned
	  chk_size = header->data_size;
	  if(chk_size % 16) chk_size += 16 - (chk_size % 16);
	  AES_CMAC(&cmac_key, inbuff+0x60, 0x30 + chk_size + header->data_offset, cmac_data_hash);

	  if(memcmp(cmac_data_hash, header->CMAC_data_hash, 16) != 0) {
	  //printf("data hash invalid, correcting...\n");
	} else {
		 printf("data hash is already valid!\n");
		 return 100;
	  }
	  // Forge collision for data hash
	memcpy(cmac_data_hash,header->CMAC_data_hash,0x10);
	AES_CMAC_forge(&cmac_key, inbuff+0x60, 0x30+ chk_size + header->data_offset, cmac_data_hash);
	  //printf("Last row in bad file should be :\n"); for(i=0;i<0x10;i++) printf("%02x", cmac_data_hash[i]);
	  //printf("\n\n");

	  return KIRK_OPERATION_SUCCESS;
   }
   return KIRK_SIG_CHECK_INVALID; //Checks for cmd 2 & 3 not included right now
}