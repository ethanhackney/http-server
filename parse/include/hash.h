#ifndef LEX_HASH_H
#define LEX_HASH_H

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

#endif /* LEX_HASH_H */
