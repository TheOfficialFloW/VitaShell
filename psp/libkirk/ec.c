// Copyright 2007,2008,2010  Segher Boessenkool  <segher@kernel.crashing.org>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "kirk_engine.h"
#include "ecdsa.h"

struct point {
	u8 x[20];
	u8 y[20];
};

static u8 ec_p[20];
static u8 ec_a[20];	// mon
static u8 ec_b[20];	// mon
static u8 ec_N[20];
static struct point ec_G;	// mon
static struct point ec_Q;	// mon
static u8 ec_k[20]; // private key

static void elt_copy(u8 *d, u8 *a)
{
	memcpy(d, a, 20);
}

static void elt_zero(u8 *d)
{
	memset(d, 0, 20);
}

static int elt_is_zero(u8 *d)
{
	u32 i;

	for (i = 0; i < 20; i++)
		if (d[i] != 0)
			return 0;

	return 1;
}

static void elt_add(u8 *d, u8 *a, u8 *b)
{
	bn_add(d, a, b, ec_p, 20);
}

static void elt_sub(u8 *d, u8 *a, u8 *b)
{
	bn_sub(d, a, b, ec_p, 20);
}

static void elt_mul(u8 *d, u8 *a, u8 *b)
{
	bn_mon_mul(d, a, b, ec_p, 20);
}

static void elt_square(u8 *d, u8 *a)
{
	elt_mul(d, a, a);
}

static void elt_inv(u8 *d, u8 *a)
{
	u8 s[20];
	elt_copy(s, a);
	bn_mon_inv(d, s, ec_p, 20);
}

static void point_to_mon(struct point *p)
{
	bn_to_mon(p->x, ec_p, 20);
	bn_to_mon(p->y, ec_p, 20);
}

static void point_from_mon(struct point *p)
{
	bn_from_mon(p->x, ec_p, 20);
	bn_from_mon(p->y, ec_p, 20);
}

static void point_zero(struct point *p)
{
	elt_zero(p->x);
	elt_zero(p->y);
}

static int point_is_zero(struct point *p)
{
	return elt_is_zero(p->x) && elt_is_zero(p->y);
}

static void point_double(struct point *r, struct point *p)
{
	u8 s[20], t[20];
	struct point pp;
	u8 *px, *py, *rx, *ry;

	pp = *p;

	px = pp.x;
	py = pp.y;
	rx = r->x;
	ry = r->y;

	if (elt_is_zero(py)) {
		point_zero(r);
		return;
	}

	elt_square(t, px);	// t = px*px
	elt_add(s, t, t);	// s = 2*px*px
	elt_add(s, s, t);	// s = 3*px*px
	elt_add(s, s, ec_a);// s = 3*px*px + a
	elt_add(t, py, py);	// t = 2*py
	elt_inv(t, t);		// t = 1/(2*py)
	elt_mul(s, s, t);	// s = (3*px*px+a)/(2*py)

	elt_square(rx, s);	// rx = s*s
	elt_add(t, px, px);	// t = 2*px
	elt_sub(rx, rx, t);	// rx = s*s - 2*px

	elt_sub(t, px, rx);	// t = -(rx-px)
	elt_mul(ry, s, t);	// ry = -s*(rx-px)
	elt_sub(ry, ry, py);// ry = -s*(rx-px) - py
}

static void point_add(struct point *r, struct point *p, struct point *q)
{
	u8 s[20], t[20], u[20];
	u8 *px, *py, *qx, *qy, *rx, *ry;
	struct point pp, qq;

	pp = *p;
	qq = *q;

	px = pp.x;
	py = pp.y;
	qx = qq.x;
	qy = qq.y;
	rx = r->x;
	ry = r->y;

	if (point_is_zero(&pp)) {
		elt_copy(rx, qx);
		elt_copy(ry, qy);
		return;
	}

	if (point_is_zero(&qq)) {
		elt_copy(rx, px);
		elt_copy(ry, py);
		return;
	}

	elt_sub(u, qx, px);

	if (elt_is_zero(u)) {
		elt_sub(u, qy, py);
		if (elt_is_zero(u))
			point_double(r, &pp);
		else
			point_zero(r);

		return;
	}

	elt_inv(t, u);		// t = 1/(qx-px)
	elt_sub(u, qy, py);	// u = qy-py
	elt_mul(s, t, u);	// s = (qy-py)/(qx-px)

	elt_square(rx, s);	// rx = s*s
	elt_add(t, px, qx);	// t = px+qx
	elt_sub(rx, rx, t);	// rx = s*s - (px+qx)

	elt_sub(t, px, rx);	// t = -(rx-px)
	elt_mul(ry, s, t);	// ry = -s*(rx-px)
	elt_sub(ry, ry, py);// ry = -s*(rx-px) - py
}

static void point_mul(struct point *d, u8 *a, struct point *b)
{
	u32 i;
	u8 mask;

	point_zero(d);

	for (i = 0; i < 20; i++)
		for (mask = 0x80; mask != 0; mask >>= 1) {
			point_double(d, d);
			if ((a[i] & mask) != 0)
				point_add(d, d, b);
		}
}

static void generate_ecdsa(u8 *R, u8 *S, u8 *k, u8 *hash, u8 *random)
{
	u8 e[20];
	u8 kk[20];
	u8 m[20];
	u8 minv[20];
	struct point mG;

	memcpy(e, hash, 20);
	bn_reduce(e, ec_N, 20);

	if(random==NULL){
		do{
			kirk_CMD14(m, 20);
		}while(bn_compare(m, ec_N, 20) >= 0);
	}else{
		memcpy(m, random, 20);
	}

	point_mul(&mG, m, &ec_G);
	point_from_mon(&mG);
	elt_copy(R, mG.x);

	bn_copy(kk, k, 20);
	bn_reduce(kk, ec_N, 20);
	bn_to_mon(m, ec_N, 20);
	bn_to_mon(e, ec_N, 20);
	bn_to_mon(R, ec_N, 20);
	bn_to_mon(kk, ec_N, 20);

	bn_mon_mul(S, R, kk, ec_N, 20);
	bn_add(kk, S, e, ec_N, 20);
	bn_mon_inv(minv, m, ec_N, 20);
	bn_mon_mul(S, minv, kk, ec_N, 20);

	bn_from_mon(R, ec_N, 20);
	bn_from_mon(S, ec_N, 20);
}

static int check_ecdsa(struct point *Q, u8 *R, u8 *S, u8 *hash)
{
	u8 Sinv[20];
	u8 e[20];
	u8 w1[20], w2[20];
	struct point r1, r2, r3;
	u8 rr[20];

	memcpy(e, hash, 20);
	bn_reduce(e, ec_N, 20);

	// Sinv = INV(s)
	bn_to_mon(S, ec_N, 20);
	bn_mon_inv(Sinv, S, ec_N, 20);

	// w1 = e*Sinv
	bn_to_mon(e, ec_N, 20);
	bn_mon_mul(w1, e, Sinv, ec_N, 20);
	bn_from_mon(w1, ec_N, 20);

	// w2 = R*Sinv
	bn_to_mon(R, ec_N, 20);
	bn_mon_mul(w2, R, Sinv, ec_N, 20);
	bn_from_mon(w2, ec_N, 20);

	// r1 = w1*G
	point_mul(&r1, w1, &ec_G);
	// r2 = w2*Q
	point_mul(&r2, w2, Q);
	// r3 = r1+r2
	point_add(&r3, &r1, &r2);

	point_from_mon(&r3);
	memcpy(rr, r3.x, 20);
	bn_reduce(rr, ec_N, 20);

	bn_from_mon(R, ec_N, 20);
	bn_from_mon(S, ec_N, 20);

	return bn_compare(rr, R, 20);
}

void ecdsa_sign_fixed(u8 *hash, u8 *fixed_m, u8 *fixed_r, u8 *S)
{
	u8 minv[20], m[20], k[20], r[20], z[20];

	memcpy(z, hash, 20);
	memcpy(k, ec_k, 20);
	memcpy(m, fixed_m, 20);
	memcpy(r, fixed_r, 20);

	bn_to_mon(m, ec_N, 20);
	bn_mon_inv(minv, m, ec_N, 20);

	bn_to_mon(k, ec_N, 20);
	bn_to_mon(r, ec_N, 20);
	bn_mon_mul(z, k, r, ec_N, 20);
	bn_from_mon(z, ec_N, 20);

	bn_add(z, z, hash, ec_N, 20);

	bn_to_mon(z, ec_N, 20);
	bn_mon_mul(S, minv, z, ec_N, 20);
	bn_from_mon(S, ec_N, 20);
}

void ecdsa_set_curve(ECDSA_PARAM *param)
{
	memcpy(ec_p, param->p, 20);
	memcpy(ec_a, param->a, 20);
	memcpy(ec_b, param->b, 20);
	memcpy(ec_N, param->N, 20);
	memcpy(ec_G.x, param->Gx, 20);
	memcpy(ec_G.y, param->Gy, 20);

	bn_to_mon(ec_a, ec_p, 20);
	bn_to_mon(ec_b, ec_p, 20);

	point_to_mon(&ec_G);
}

void ecdsa_set_N(u8 *N)
{
	memcpy(ec_N, N, 20);
}

void ecdsa_set_pub(u8 *Qx, u8 *Qy)
{
	memcpy(ec_Q.x, Qx, 20);
	memcpy(ec_Q.y, Qy, 20);
	point_to_mon(&ec_Q);
}

void ecdsa_set_priv(u8 *k)
{
	memcpy(ec_k, k, sizeof ec_k);
}

int ecdsa_verify(u8 *hash, u8 *R, u8 *S)
{
	return check_ecdsa(&ec_Q, R, S, hash);
}

void ecdsa_sign(u8 *hash, u8 *R, u8 *S, u8 *random)
{
	generate_ecdsa(R, S, ec_k, hash, random);
}

/*************************************************************/

// calculate the random and private key from signs with same r value
void ecdsa_find_m_k(u8 *sig_r, u8 *sig_s1, u8 *hash1, u8 *sig_s2, u8 *hash2, u8 *N, u8 *ret_m, u8 *ret_k)
{
	u8 e1[20], e2[20];
	u8 s1[20], s2[20];
	u8 sinv[20], m[20];
	u8 r[20], rinv[20], kk[20];

	// e1
	memcpy(e1, hash1, 20);
	// e2
	memcpy(e2, hash2, 20);
	// s1, s2
	memcpy(s1, sig_s1, 20);
	memcpy(s2, sig_s2, 20);

	// restore random m
	// s1 = s1-s2
	bn_sub(s1, s1, s2, N, 20);
	// e1 = e1-e2
	bn_sub(e1, e1, e2, N, 20);

	bn_to_mon(s1, N, 20);
	bn_to_mon(e1, N, 20);

	// m = (e1-e2)/(s1-s2)
	bn_mon_inv(sinv, s1, N, 20);
	bn_mon_mul(m, sinv, e1, N, 20);
	bn_from_mon(m, N, 20);

	bn_dump("random m", m, 20);
	memcpy(ret_m, m, 20);

	// restore private key
	memcpy(e1, hash1, 20);
	memcpy(s1, sig_s1, 20);
	memcpy(r, sig_r, 20);

	// kk = m*s
	bn_to_mon(s1, N, 20);
	bn_to_mon(m, N, 20);
	bn_mon_mul(kk, m, s1, N, 20);

	// kk = m*s-e
	bn_from_mon(kk, N, 20);
	bn_sub(kk, kk, e1, N, 20);
	bn_to_mon(kk, N, 20);

	// kk = (m*s-e)/r
	bn_to_mon(r, N, 20);
	bn_mon_inv(rinv, r, N, 20);
	bn_mon_mul(kk, rinv, kk, N, 20);
	bn_from_mon(kk, N, 20);

	bn_dump("private key", kk, 20);
	memcpy(ret_k, kk, 20);

}

