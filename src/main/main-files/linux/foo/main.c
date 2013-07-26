/*
    FlexCOS - Copyright (C) 2013 AGSI, Department of Computer Science, FU-Berlin

    FOR MORE INFORMATION AND INSTRUCTION PLEASE VISIT
    http://www.inf.fu-berlin.de/groups/ag-si/smart.html


    This file is part of the FlexCOS project.

    FlexCOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License (version 3) as published by the
    Free Software Foundation.

    Some parts of this software are from different projects. These files carry
    a different license in their header.

    FlexCOS is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
    details. You should have received a copy of the GNU General Public License
    along with FlexCOS; if not it can be viewed here:
    http://www.gnu.org/licenses/gpl-3.0.txt and also obtained by writing to
    AGSI, contact details for whom are available on the FlexCOS WEB site.

*/

#include <const.h>
#include <types.h>
#include <array.h>

#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#include <time.h>

#include <miracl.h>
#include <string.h>

#include <ecc.h>
#include <octet.h>

#include <cryptools.h>

#ifndef MAX_HASH_BYTES
#define MAX_HASH_BYTES 32
#endif


csprng RNG;
csprng *const rng = &RNG;

#define AES_BLKSZ 16

PUBLIC oltype ecp_2_octet(const epoint *, octet *);
PUBLIC oltype octet_2_ecp(const octet *, u32, epoint *);

/**
 *  An alias for 'ecp_2_octet'
 */
PRIVATE inline oltype
octet_put_epoint(octet *o, const epoint *P)
{
	return ecp_2_octet(P, o);
}

PRIVATE inline oltype
octet_put_big(octet *o, const big b)
{
	oltype bytes;
	// TODO check parameter

	// TODO use correct byte length
	bytes = b->len * sizeof(mr_small);
	if (!octet_has_bytes_free(o, bytes))
		return 0;

	o->len += big_to_bytes(bytes, b, octet_end(o), TRUE);
	return bytes;
}

/**
 * @Note Not applicable for CFBn and PCFBn modes.
 */
PRIVATE void
aes_encrypt_all(aes *ae, char *m, u32 length)
{
	char *blk;
	if (length % AES_BLKSZ) return;

	for (blk = m; blk < (m + length); blk += AES_BLKSZ)
		aes_encrypt(ae, blk);
}

PRIVATE void
aes_decrypt_all(aes *ae, char *m, u32 length)
{
	char *blk;
	if (length % AES_BLKSZ) return;

	for (blk = m; blk < (m + length); blk += AES_BLKSZ)
		aes_decrypt(ae, blk);
}

/**
 *  @return number of bytes or zero
 */
PUBLIC oltype
ecp_2_octet(const epoint *P, octet *o)
{
	// TODO bullet proof
	// TODO optional compression

	/* do not use compression */
	octet_put(o, 0x04);
	// FIXME magic number: underlying field size required
	// not sure: is P->X->len too optimistic
	o->len += big_to_bytes(24, P->X, o->val + o->len, TRUE);
	o->len += big_to_bytes(24, P->Y, o->val + o->len, TRUE);

	// TODO correct return value
	return 0;
}

/**
 *  @return Zero on any kind of failure, otherwise number of bytes that have been
 *          read to read point P.
 *
 *  @NOTE:  There is no point validation.
 */
PUBLIC oltype
octet_2_ecp(const octet *o, u32 fsize, epoint *P)
{
	u8  flag;
	oltype pb = 0;

	/* TODO errno E_RANGE here */
	if (o->len < (fsize + 1)) return 0;

	flag = o->val[pb++];

	bytes_to_big(fsize, o->val + pb, P->X);
	pb += fsize;

	/* no compression */
	if (flag == 0x04) {
		/* TODO errno E_RANGE here */
		if (o->len < 2*fsize + 1) return 0;

		bytes_to_big(fsize, o->val + pb, P->Y);
		pb += fsize;

		epoint_set(P->X, P->Y, 0, P);

		return pb;
	/* point is compressed */
	} else if (flag & 0x02) {
		epoint_set(P->X, P->X, flag&0x01, P);
	} else {
		return 0;
	}

	return pb;
}


void
init_fake_rng(csprng *_rng)
{
	u32 i;
	u32 ran;
	octet seed;

	time((time_t *)&ran);

	OCTET_INIT(&seed, 100);
	seed.len=100;
	seed.val[0]=ran;
	seed.val[1]=ran>>8;
	seed.val[2]=ran>>16;
	seed.val[3]=ran>>24;

	for (i=4;i<100;i++) seed.val[i]=i+1;

	CREATE_CSPRNG(_rng, &seed);

	OCTET_KILL(&seed);

	return;
}

struct device {
	/* persistent */
	octet tau;
	octet v;
	octet a;
	struct ecc_pk *pk_tau;
	/* session */
	big   k1;
	octet *pin;
	octet *msg;
};

struct server {
	/* persistent */
	struct ecc_sk *sk;
};

struct public {
	struct ecc_dom *dom;
	struct ecc_pk *pk_foo;
	struct ecc_pk *pk_srv;
};

struct server srv;
struct device dvc;
struct public pub;

/**
 *  Dummy pseudorandom function
 */
PUBLIC err_t
f(const octet *key, const octet *input, octet *out)
{
	char   h[32];
	sha256 sh;
	shs256_init(&sh);
	oltype i;

	for (i = 0; i < key->len; i++)
		shs256_process(&sh, key->val[i]);

	for (i = 0; i < input->len; i++)
		shs256_process(&sh, input->val[i]);

	shs256_hash(&sh, h);

	// XXX side effect
	OCTET_EMPTY(out);
	// FIXME fill output up to its maximum: out->max
	OCTET_JOIN_BYTES(h, MIN(sizeof(h), out->max), out);

	return E_GOOD;
}

/**
 *  KDF: NIST SP 800-56A
 *
 *  run: key << hash(round || input {|| other}) until key has been filled.
 */
PRIVATE err_t
kdf(const octet *input, const octet *other, octet *key)
{
	char   h[32];
	const unsigned char   *c;
	sha256 sh;
	int    i;
	u32    cnt;

	for (cnt = 0x01; octet_bytes_left(key); cnt++) {
		shs256_init(&sh);
		for (i = 3; (int)i >= 0; i--)
			shs256_process(&sh, (cnt >> i));

		octet_each(input, c)
			shs256_process(&sh, *c);

		if (other) octet_each(other, c)
			shs256_process(&sh, *c);

		shs256_hash(&sh, h);
		octet_append(key, h, 32);
	}
	return E_GOOD;
}


/** Encrypt message m using ECIES.
 *
 *  What do we here?
 *
 *  1)  r <- random(0,n-1)
 *  2)  R <- r G         (with G = Generator of E)
 *  3)  S <- r Y         (with Y = ECC public key)
 *  4)  k1||k2 <- KDF(x(S))
 *  5)  c1 <- aes_enc(k1, m)
 *  6)  c2 <- HMAC(k2, c1)
 *
 *  write output
 *  7)  c << R || c1 || c2
 */
PUBLIC err_t
ecies_enc(const struct ecc_dom *dom,
          const struct ecc_pk  *pk,
	  const octet *m, octet *c)
{
	char mmb[MR_BIG_RESERVE(1)] = {0};
	char mmp[MR_ECP_RESERVE(1)] = {0};
	aes    ae;
	epoint *R;
	big    r;
	octet  s;  /* holds serialized shared key */
	octet  e;  /* opaque octet on cipher text area for encrypted message */
	octet  mac;/* opaque octet on cipher text area for HMAC */
	octet  k;  /* key derived from s by: k = KDF(s) */

	// FIXME check available bytes in output octet 'c'
	// return E_RANGE
	// length \approx [2*]<fsize>+1 + <msg-length> + <hash_length>
	// e.g. for EC 256bit and SHA256 without point compression
	// length = 65 + msg + 32

	/* actually an ephemeral key generation. Should we use ecc_gen instead? */
	r = mirvar_mem(mmb, 0);
	R = epoint_init_mem(mmp, 0);

	bigrand(dom->n, r);
	ecurve_mult(r, dom->G, R);
	epoint_norm(R);
	/* Key pair ready: <r, R> */

	/* write point to cipher text. so we can reuse its memory */
	octet_put_epoint(c, R);

	/* calculate a shared key (ECDH-Key-Exchange) */
	ecurve_mult(r, pk->Q, R);
	epoint_norm(R);
	/* big is no longer needed. */
	r = NULL;
	memset(mmb, 0x00, sizeof(mmb));

	OCTET_INIT_FROM_ARRAY(&s, 32, mmb);
	OCTET_INIT_FROM_ARRAY(&k, 64, mmp);

	s.len = big_to_bytes(32, R->X, s.val, TRUE);
	memset(mmp, 0x00, sizeof(mmp));

	kdf(&s, NULL, &k);
	// XXX review MODE decision
	aes_init(&ae, MR_ECB, 32, k.val, NULL);

	if (m->len % AES_BLKSZ) return E_BAD_PARAM;

	// XXX too much Voodoo?
	/* create some opaque objects on cipher text octet */
	OCTET_INIT_FROM_ARRAY(&e, m->len, octet_end(c));
	OCTET_INIT_FROM_ARRAY(&mac, 32, octet_end(c) + m->len);
	octet_append(&e, m->val, m->len);
	aes_encrypt_all(&ae, e.val, e.len);

	c->len += e.len;
	k.val += k.len;
	// MAC1(&e, NULL, &k, mac.max, SHA256, &mac);
	c->len += mac.len;

	// TODO aes_end(...)

	return E_GOOD;
}

PUBLIC err_t
ecies_dec(const struct ecc_dom *dom,
          const struct ecc_sk  *sk,
	  const octet *c, octet *m)
{
	char    mmp[MR_ECP_RESERVE(1)] = {0};
	char    __buff[32]; // used twice: octet string of shared key 's' and hash of cipher text
	char    *emsg;
	aes     ae;
	epoint  *R;
	octet   mac;
	octet   s;
	octet   k;
	oltype  i;
	oltype  plen;
	oltype  mlen;

	R = epoint_init_mem(mmp, 0);
	OCTET_INIT_FROM_ARRAY(&s, 32, __buff);
	OCTET_INIT_FROM_ARRAY(&k, 64, mmp);
	/* read point from cipher text */
	plen = octet_2_ecp(c, dom->fsize, R);

	if (!plen)
		return E_BAD_PARAM;
	if (point_at_infinity(R))
		return E_FAILED;

	ecurve_mult(sk->x, R, R);
	epoint_norm(R);
	s.len = big_to_bytes(32, R->X, s.val, TRUE);
	kdf(&s, NULL, &k);
	aes_init(&ae, MR_ECB, 32, k.val, NULL);

	// XXX magic number: 32 == HASH_SIZE
	mlen = c->len - plen - 32;
	emsg = octet_end(m);
	octet_append(m, c->val + plen, mlen);

	aes_decrypt_all(&ae, emsg, mlen);

	return E_GOOD;
}

/* XXX better name */
PRIVATE err_t
sha256_append(const octet *str, octet *buff)
{
	char hh[MAX_HASH_BYTES];
	sha256 sh;
	u32 i;

	if (buff->len + 32 > buff->max) return E_RANGE;

	shs256_init(&sh);

	for (i = 0; i < str->len; i++)
		shs256_process(&sh, str->val[i]);
	shs256_hash(&sh, hh);

	OCTET_JOIN_BYTES(hh, 32, buff);

	return E_GOOD;
}

#define SIZE_TAU

PUBLIC err_t
ecsign_init(const struct ecc_dom *dom,
            const struct ecc_pk *pk_srv,
            const octet *pin,
	    octet *a,
	    octet *v,
	    octet *t,
	    octet *tau,
	    struct ecc_pk *pk,
	    struct ecc_pk *pk_tau)
{
#define TICKET_SIZE 4*32
	char __mem[TICKET_SIZE];
	struct ecc_sk sk;
	/* put a union around shared stack memory */
	union {
		octet ticket;
		octet sk_raw;
	} tmp = { .ticket = Octet(__mem)};

	/* create server key */
	ecc_gen(dom, pk_tau, &sk);
	/* create random shared key a */
	/* create random revocation key t */
	octet_reset(a);
	octet_reset(t);
	octet_fill_random(a, a->max);
	octet_fill_random(t, t->max);

	/* append pin-hash to ticket */
	octet_put_big(&tmp.ticket, sk.x);
	octet_append(&tmp.ticket, a->val, a->max);
	sha256_append(t, &tmp.ticket);
	sha256_append(pin, &tmp.ticket);

	/* encrypt ticket into tau */
	ecies_enc(dom, pk_srv, &tmp.ticket, tau);

	/* create random PRF seed v */
	/* calculate secret device key from pin and v */
	do {
		octet_reset(v);
		octet_fill_random(v, v->max);
		octet_clear(&tmp.sk_raw);

		tmp.sk_raw.max = dom->fsize;
		f(v, pin, &tmp.sk_raw);

		bytes_to_big(tmp.sk_raw.len, tmp.sk_raw.val, sk.x);
	} while (compare(sk.x, dom->n) >= 0);

	ecc_pk_init(pk);
	ecurve_mult(sk.x, pk->Y, pk->Y);
	ecurve_add(pk_tau->Y, pk->Y);
	epoint_norm(pk->Y);

	return E_GOOD;
}

/**
 *  Assume elliptic curve environment has already been initialized.
 */
PUBLIC err_t
ecsign_2p_init_foo(struct device *dvc, char *pin, const octet *pk_srv)
{
	char mmb[MR_BIG_RESERVE(4)] = {0};
	char mmp[MR_ECP_RESERVE(5)] = {0};
	char mx2[32] = {0};
#define kappa 32
// lambda = EC key size in bytes (e.g. 24 bytes for 192 bit curve)
#define lambda 24
	octet a;   /**< MAC key */
	octet t;   /**< random disable token */
	octet v;   /**< device key for x1 creation */
	octet oct_x1;  /**< device key */
	big x1;
	big x2;  /**< servers key */
	epoint *Y1;  /**< public key x1*G */
	epoint *Y2;  /**< public key x2*G */
	epoint *Y;   /**< public key Y1 + Y2 */
	octet tau;   /**< the holey ticket */
	octet ticket; /**< unencrypted tau */
	octet opin;
	u32 bytes;
	u32 blub;

	char mem[MR_BIG_RESERVE(6)] = {0};
	big k1, k2, r, s, ri;
	big m;
	big const n = pub.dom->n;
	epoint *R;
	epoint *P;
	miracl *mip = get_mip();

	x1 = mirvar_mem(mmb, 0);
	x2 = mirvar_mem(mmb, 1);
	Y1 = epoint_init_mem(mmp, 0);
	Y2 = epoint_init_mem(mmp, 1);
	Y  = epoint_init_mem(mmp, 2);

	R  = epoint_init_mem(mmp, 3);
	P  = epoint_init_mem(mmp, 4);

	k1 = mirvar_mem(mem, 0);
	k2 = mirvar_mem(mem, 1);
	r  = mirvar_mem(mem, 2);
	s  = mirvar_mem(mem, 3);
	ri = mirvar_mem(mem, 4);
	m  = mirvar_mem(mem, 5);

	OCTET_INIT(&a, kappa);
	OCTET_INIT(&t, kappa);
	OCTET_INIT(&v, kappa);
	OCTET_INIT(&opin, strlen(pin));
	OCTET_INIT(&oct_x1, pub.dom->bits/8);

	/* generate random values with security parameter kappa: */
	/* a, t, v */
	OCTET_RAND(rng, kappa, &a);
	OCTET_RAND(rng, kappa, &t);

	do {
		OCTET_RAND(rng, kappa, &v);
		f(&v, &opin, &oct_x1);

		mip->INPLEN = oct_x1.len;
		instr(x1, oct_x1.val);

	} while (compare(x1,n) >= 0);

	/* create ticket: a, b, u, x2 */
	OCTET_INIT(&ticket, kappa + 2 * 32 + sizeof(mx2));
	OCTET_JOIN_OCTET(&a, &ticket);
	OCTET_JOIN_STRING(pin, &opin);
	sha256_append(&opin, &ticket);
	sha256_append(&t, &ticket);

	/* generate x2 random from [0, dom->R] */
	/* n is order of G */
	bigrand(n, x2);
	// FIXME dangerous on max == len
	// XXX method returns signed integer
	// big_to_bytes(...)
	/*  */
	otstr(x2, mx2);
	OCTET_JOIN_BYTES(mx2, sizeof(mx2), &ticket);
	// big_to_bytes()


	/*  write to secret file: a, v, tau, (pk_server)
	 *  write to public file: Y,
	 *  semi public:          Y1, Y2
	 */
	ecurve_mult(x1, pub.dom->G, Y1);
	ecurve_mult(x2, pub.dom->G, Y2);

	epoint_set(NULL, NULL, 0, Y);
	ecurve_add(Y1, Y);
	ecurve_add(Y2, Y);

	/* create a random message */
	bigrand(n, m);

	bigrand(pub.dom->n, k1);
	bigrand(pub.dom->n, k2);

	/* create shared point R */
	ecurve_mult(k1, pub.dom->G, R);
	ecurve_mult(k2, pub.dom->G, P);
	ecurve_add(P, R);
	epoint_get(R,r,r);
	divide(r,n,n);             // r = r mod n

	/* create a sample signature x2 */
	mad(x2,m,m,n,n,s);          // s = x2*m mod n
	mad(k2,r,s,n,n,s);          // s = s + k1*r mod n

	/* create sample signature x1 */
	mad(x1,m,s,n,n,s);          // s = s + x1*m mod n
	mad(k1,r,s,n,n,s);          // s = s + k1*r mod n

	/* verify(m,r,s) */
	xgcd(r,n,ri,ri,ri);
	mad(r,ri,ri,n,n,k1);
	// k should be 1;
	blub = size(k1);

	mad(s,ri,ri,n,n,s);        // s = s*r^-1 mod n
	mad(m,ri,ri,n,n,m);        // m = m*r^-1 mod n
	ecurve_mult(m, Y, Y);      // Y = m*r1 * Y
	ecurve_mult(s, pub.dom->G,  R);
	ecurve_sub(Y, R);

	epoint_get(R,x1,x1);
	divide(x1,n,n);
	blub = compare(r, x1);

	return E_GOOD;
}

PUBLIC err_t
ecsign_2p_start(const octet *m, const octet *pin)
{
	epoint *R1;
	big k1;
	octet g;

	// session
	// g <- <m, R, hash(pin)>

	return E_NO_LOGIC;
}

/**
 * gamma = pk_server encrypted <message, pin hash, point R1, rho>
 * tau   = pk_server encrypted <hmac_key, pin hash, disable hash, key>
 * delta = message hash (32byte at sha256)
 */
PUBLIC err_t
ecsign_2p_serve(octet *gamma, octet *tau, octet *delta, octet *eta)
{
	big m;
	big s;
	big r;
	big n = pub.dom->n;
	big k2;
	big r1x, r1y;
	big x2;
	epoint *R;
	epoint *R1;
	epoint *R2;
	epoint *G = pub.dom->G;

	/* prerequisite */
	/* decrypt gamma = <m, beta, r1_x, r1_y, otp> */
	/* remove OTP from tau */
	/* get hash of tau -> check against black list */
	/* decrypt tau = <a, b, u, x2> */
	/* check HMAC_a(gamma, tau) = delta */
	/* verify PIN: if (beta != b) abort */

	/* signatur creation */
	/* k2 <- random */
	epoint_set(r1x, r1y, 0, R1);
	ecurve_mult(k2, G, R2);
	ecurve_mult(k2, R1, R);

	epoint_get(R, r, r);
	divide(r,n,n);
	/* calculate signature: s = x*m + k2*r */
	mad(x2,m,m,n,n,s);
	mad(k2,r,s,n,n,s);

	return E_GOOD;
}

/**
 *  Process server message.
 */
PUBLIC err_t
ecsign_2p_finish(octet *eta)
{
	big x1;
	big r;
	big r2x, r2y;
	big s;
	big n;
	big m;
	big k1 = dvc.k1;
	epoint *R;

	epoint_set(r2x, r2y, 0, R);
	ecurve_mult(k1, R, R);
	epoint_get(R,r,r);

	/* calculate signature: s = x2*m + k*r */
	mad(k1,r,s,n,n,s);
	// Verify(m, Y2, r, s);

	/* complete signature: s = x*m + k*r */
	mad(x1,m,s,n,n,s);
	// Verify(m, Y, r, s);

	return E_NO_LOGIC;
}

/**
 * Clean up device.
 */
PUBLIC err_t
ecsign_2p_erase()
{
	return E_NO_LOGIC;
}

PUBLIC err_t
flx_curve_activate(struct ecc_dom *c)
{
	miracl *mip = get_mip();

	ecurve_init(c->a, c->b, c->p, MR_PROJECTIVE);
	if (mip->ERNUM) {
		return E_FAILED;
	}

	/* verify that G is a real point on this curve */
	if (!epoint_set(c->G->X, c->G->Y, 0, c->G)) {
		/* point (gx, gy) is not on curve */
		return E_FAILED;
	}

	return E_GOOD;
}

/*
 * This kind of override is possible since standard file IO is disabled in MIRACL
 */
#ifdef MR_NO_FILE_IO
PRIVATE int
cinnum(big num, FILE *fp)
{
	static char tmp[512/4] = {0};
	int c;
	u32 i;
	miracl *mip = get_mip();

	i = 0;
	c = 0;

	while ((c = fgetc(fp))) {
		if (c == '\n')
			break;
		else
			tmp[i++] = c;
	}

	mip->INPLEN = i;

	c = cinstr(num, tmp);
	memset(tmp, 0x00, i);

	return c;
}
#endif

/**
 * @return order of G
 */
PUBLIC err_t
flx_curve_load(const char *fname, struct ecc_dom *curve)
{
	FILE *fp;
	miracl *mip;

	CHECK_PARAM__NOT_NULL(fname);

	mip = get_mip();

	fp = fopen(fname, "r");
	if (!fp) {
		fprintf(stderr, "could not open file: %s\n", strerror(errno));
		return NULL;
	}
	fscanf(fp, "%hd\n", &curve->bits);

	// FIXME fsize = roundup(log_2(curve->p)/8)
	curve->fsize = curve->bits/8;

	/* reinitialization does not hurt */
	ecc_dom_init(curve);

	mip->IOBASE = 16;

	cinnum(curve->p, fp);
	cinnum(curve->a, fp);
	cinnum(curve->b, fp);
	cinnum(curve->n, fp);
	cinnum(curve->G->X, fp);
	cinnum(curve->G->Y, fp);

	return E_GOOD;
}

PRIVATE void
pwd(void)
{
	char *cwd;
	cwd = getcwd(0, 0);
	if (!cwd)
		fprintf(stderr, "getcwd failed: %s\n", strerror(errno));
	else
		printf("cwd: %s\n", cwd);
	free(cwd);
}

u8 map_plus_one(u8 v) {
	return v + 1;
}

int
main(void)
{
	miracl *mip;
	struct ecc_dom curve = {0};
	struct ecc_pk  pk_srv;
	struct ecc_pk  pk_dvc;
	struct ecc_pk  pk_tau;
	struct ecc_sk  sk_srv;
	struct device  dvc;
	struct hmac    hm;
	char   __pin[] = "12345";
	char   __msg[] = "FooBar-16bytes!";
	char   __enc[1024];
	char   __dec[1024];
	char   __mma[32];
	char   __mmv[32];
	char   __mmt[32];
	char   __tau[256];
	char   __key[32];
	octet  msg     = Octet(__msg);
	octet  cipher  = Octet(__enc);
	octet  dec     = Octet(__dec);
	octet  pin     = Octet(__pin);
	octet  dvc_a   = Octet(__mma);
	octet  dvc_t   = Octet(__mmt);
	octet  dvc_v   = Octet(__mmv);
	octet  dvc_tau = Octet(__tau);

	array  m       = CArray(__msg);
	array  key     = CArray(__key);
	int    i;

	hmac_init(&hm, &key);
	hmac_hash(&hm, __key);

	m.len = m.max;
	array_map(&m, map_plus_one);

	pwd();

	msg.len = msg.max;
	pin.len = pin.max - 1;


	init_fake_rng(rng);
	printf("Play around with PRNG\n");
	for (i = 0; i < 2; i++)
		printf("%d\n", strong_rng(rng));

	mip = mirsys(192/8, 256);
	mip->TRACER = TRUE;

	flx_curve_load("src/resources/curves/common.ecs", &curve);
	if (flx_curve_activate(&curve)) {
		fprintf(stderr, "could not activate curve");
		return 1;
	}

	if (ecc_gen(&curve, &pk_srv, &sk_srv)) {
		fprintf(stderr, "could not generate key pair\n");
		return 2;
	}

	ecies_enc(&curve, &pk_srv, &msg, &cipher);
	ecies_dec(&curve, &sk_srv, &cipher, &dec);

	pub.dom = &curve;

	ecsign_init(&curve, &pk_srv, &pin,
			&dvc_a, &dvc_v, &dvc_t, &dvc_tau,
			&pk_dvc, &pk_tau);

	//ecsign_2p_init(&dvc, "Password", NULL);
	//ecsign_2p_start();

	return 0;
}
