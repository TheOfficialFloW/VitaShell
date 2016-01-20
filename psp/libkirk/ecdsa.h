// Copyright 2010			Sven Peter <svenpeter@gmail.com>
// Copyright 2007,2008,2010  Segher Boessenkool  <segher@kernel.crashing.org>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#ifndef ECDSA_H__
#define ECDSA_H__ 1

typedef struct {
	u8 p[20];
	u8 a[20];
	u8 b[20];
	u8 N[20];
	u8 Gx[20];
	u8 Gy[20];
}ECDSA_PARAM;

void ecdsa_set_curve(ECDSA_PARAM *param);
void ecdsa_set_N(u8 *N);
void ecdsa_set_pub(u8 *Qx, u8 *Qy);
void ecdsa_set_priv(u8 *k);
int  ecdsa_verify(u8 *hash, u8 *R, u8 *S);
void ecdsa_sign(u8 *hash, u8 *R, u8 *S, u8 *random);
void ecdsa_sign_fixed(u8 *hash, u8 *fixed_m, u8 *fixed_r, u8 *S);
void ecdsa_find_m_k(u8 *sig_r, u8 *sig_s1, u8 *hash1, u8 *sig_s2, u8 *hash2, u8 *N, u8 *ret_m, u8 *ret_k);

void bn_dump(char *msg, u8 *d, u32 n);
int  bn_is_zero(u8 *d, u32 n);
void bn_copy(u8 *d, u8 *a, u32 n);
int  bn_compare(u8 *a, u8 *b, u32 n);
void bn_reduce(u8 *d, u8 *N, u32 n);
void bn_add(u8 *d, u8 *a, u8 *b, u8 *N, u32 n);
void bn_sub(u8 *d, u8 *a, u8 *b, u8 *N, u32 n);
void bn_to_mon(u8 *d, u8 *N, u32 n);
void bn_from_mon(u8 *d, u8 *N, u32 n);
void bn_mon_mul(u8 *d, u8 *a, u8 *b, u8 *N, u32 n);
void bn_mon_inv(u8 *d, u8 *a, u8 *N, u32 n);

void bn_mul(u8 *d, u8 *a, u8 *b, u8 *N, u32 n);
void bn_gcd(u8 *out_d, u8 *in_a, u8 *in_b, u32 n);

#endif

