#ifndef LEX_HASH_H
#define LEX_HASH_H

static const struct kword hdr_hash[6] = {
	{ "User-Agent", TT_USER_AGENT },
	{ "A-IM", TT_A_IM },
	{ "Accept-Charset", TT_ACCEPT_CHARSET },
	{ "Accept", TT_ACCEPT },
	{ NULL },
	{ "Host", TT_HOST },
};
static const size_t hdr_hash_cap = 6;

static const struct kword method_hash[15] = {
	{ "HEAD", TT_HEAD },
	{ NULL },
	{ NULL },
	{ "TRACE", TT_TRACE },
	{ "PATCH", TT_PATCH },
	{ "PUT", TT_PUT },
	{ NULL },
	{ "POST", TT_POST },
	{ "CONNECT", TT_CONNECT },
	{ NULL },
	{ "GET", TT_GET },
	{ "DELETE", TT_DELETE },
	{ "OPTIONS", TT_OPTIONS },
	{ NULL },
	{ NULL },
};
static const size_t method_hash_cap = 15;

static const struct kword version_hash[1] = {
	{ "HTTP/1.1", TT_V_1_1 },
};
static const size_t version_hash_cap = 1;

#endif /* LEX_HASH_H */
