#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include <limits.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <poll.h>
#include <sys/resource.h>
static const char* simpler_fl[] = {"input.smplr", 0};
static const char* simpler_at(long line) { static char b[300]; snprintf(b, sizeof b, "%s:%ld", simpler_fl[line / 1000000], line % 1000000); return b; }
typedef struct { int tag; long v0; long v1; long v2; long v3; long v4; long v5; long v6; long v7; } Obj;
long mk(int tag, long v0, long v1, long v2) { Obj* o = malloc(4 * sizeof(long)); o->tag = tag; o->v0 = v0; o->v1 = v1; o->v2 = v2; return (long)(intptr_t)o; }
long mk8(int tag, long v0, long v1, long v2, long v3, long v4, long v5, long v6, long v7) { Obj* o = malloc(sizeof(Obj)); o->tag = tag; o->v0 = v0; o->v1 = v1; o->v2 = v2; o->v3 = v3; o->v4 = v4; o->v5 = v5; o->v6 = v6; o->v7 = v7; return (long)(intptr_t)o; }
typedef struct { long* data; long len; long cap; } SList;
long l_new() { SList* l = (SList*)malloc(sizeof(SList)); l->cap = 8; l->len = 0; l->data = (long*)malloc(sizeof(long) * l->cap); return (long)(intptr_t)l; }
long l_push(long lp, long x) { SList* l = (SList*)(intptr_t)lp; if (l->len == l->cap) { l->cap = l->cap * 2; l->data = (long*)realloc(l->data, sizeof(long) * l->cap); } l->data[l->len] = x; l->len = l->len + 1; return 0; }
long l_with(long lp, long x) { l_push(lp, x); return lp; }
long l_at(long lp, long i) { return ((SList*)(intptr_t)lp)->data[i]; }
long l_len(long lp) { return ((SList*)(intptr_t)lp)->len; }
typedef struct { const char* p; long nb, nl, cl, cb; } SL; static SL sl_e[8]; static int sl_k[2]; static SL* sl_last = sl_e; static SL* s_info_slow(const char* p) { for (int k = 0; k < 8; k++) if (sl_e[k].p == p) { sl_last = &sl_e[k]; return sl_last; } long n = strlen(p), m = 0; for (long k = 0; k < n; k++) m += ((unsigned char)p[k] & 192) == 128; int big = n >= 512; SL* e = &sl_e[big * 4 + sl_k[big]]; sl_k[big] = (sl_k[big] + 1) & 3; sl_last = e; e->p = p; e->nb = n; e->nl = n - m; e->cl = 0; e->cb = 0; return e; } static inline SL* s_info(const char* p) { if (sl_last->p == p) return sl_last; return s_info_slow(p); }
static long s_off_slow(SL* e, long i) { if (i >= e->nl) return e->nb; if (i <= 0) return 0; const unsigned char* p = (const unsigned char*)e->p; long l = e->cl, b = e->cb; if (i < l - i) { l = 0; b = 0; } while (l < i) { b++; while ((p[b] & 192) == 128) b++; l++; } while (l > i) { b--; while (b > 0 && (p[b] & 192) == 128) b--; l--; } e->cl = l; e->cb = b; return b; } static inline long s_off(SL* e, long i) { if (e->nb == e->nl) return i; return s_off_slow(e, i); }
static char* s_ar; static long s_arn; static char* s_alloc(long n) { if (n > 256) return (char*)malloc(n); if (s_arn < n) { s_ar = (char*)malloc(65536); s_arn = 65536; } char* r = s_ar; s_ar += n; s_arn -= n; return r; } static long s_part(const char* p, long a, long b) { long n = b - a; char* r = s_alloc(n + 1); memcpy(r, p + a, n); r[n] = 0; return (long)(intptr_t)r; }
static char s_one[512]; static char* s_mb[1024]; static long s_let(const char* p, long a, long b) { long n = b - a; if (n == 1) { if (!s_one[2]) for (int k = 0; k < 256; k++) s_one[2 * k] = (char)k; return (long)(intptr_t)&s_one[2 * (unsigned char)p[a]]; } if (n < 1) return s_part(p, a, a); unsigned h = 0; for (long k = 0; k < n; k++) h = h * 31 + (unsigned char)p[a + k]; char** e = &s_mb[h & 1023]; if (*e && strncmp(*e, p + a, n) == 0 && (*e)[n] == 0) return (long)(intptr_t)*e; *e = (char*)(intptr_t)s_part(p, a, b); return (long)(intptr_t)*e; }
static inline long s_each(long s, long* b) { const unsigned char* p = (const unsigned char*)(intptr_t)s; long a = *b, e = a + 1; while ((p[e] & 192) == 128) e++; *b = e; return s_let((const char*)p, a, e); } long s_lenp(const char* p) { return s_info(p)->nl; }
static inline long s_len(long s) { return s_info((const char*)(intptr_t)s)->nl; }
long l_setc(long lp, long i, long x, long line) { SList* l = (SList*)(intptr_t)lp; if (i < 0 || i >= l->len) { fprintf(stderr, "%s: list index %ld, but the list has %ld items\n", simpler_at(line), i, l->len); exit(1); } l->data[i] = x; return 0; }
long l_removec(long lp, long i, long line) { SList* l = (SList*)(intptr_t)lp; if (i < 0 || i >= l->len) { fprintf(stderr, "%s: list index %ld, but the list has %ld items\n", simpler_at(line), i, l->len); exit(1); } long x = l->data[i]; for (long k = i; k + 1 < l->len; k++) l->data[k] = l->data[k + 1]; l->len = l->len - 1; return x; }
long l_atc(long lp, long i, long line) { SList* l = (SList*)(intptr_t)lp; if (i < 0 || i >= l->len) { fprintf(stderr, "%s: list index %ld, but the list has %ld items\n", simpler_at(line), i, l->len); exit(1); } return l->data[i]; }
static inline long s_atc(long s, long i, long line) { const char* p = (const char*)(intptr_t)s; SL* e = s_info(p); if (i < 0 || i >= e->nl) { fprintf(stderr, "%s: text index %ld, but the text has %ld characters\n", simpler_at(line), i, e->nl); exit(1); } long a = s_off(e, i); long b = a + 1; while (((unsigned char)p[b] & 192) == 128) b++; return s_let(p, a, b); }
static inline long s_slicec(long s, long a, long b, long line) { const char* p = (const char*)(intptr_t)s; SL* e = s_info(p); if (a < 0 || b > e->nl) { fprintf(stderr, "%s: slice %ld to %ld, but the text has %ld characters\n", simpler_at(line), a, b, e->nl); exit(1); } if (b < a) b = a; long x = s_off(e, a); return s_part(p, x, s_off(e, b)); }
long s_isint(long s) { const char* p = (const char*)(intptr_t)s; while (*p == 32 || *p == 9 || *p == 13 || *p == 10) p++; if (*p == 45 || *p == 43) p++; if (*p < 48 || *p > 57) return 0; while (*p >= 48 && *p <= 57) p++; while (*p == 32 || *p == 9 || *p == 13 || *p == 10) p++; return *p == 0; }
long s_tointc(long s, long line) { if (!s_isint(s)) { fprintf(stderr, "%s: \"%s\" is not a whole number; test with .isInt first\n", simpler_at(line), (const char*)(intptr_t)s); exit(1); } return atol((const char*)(intptr_t)s); }
long i_div(long a, long b, long line) { if (b == 0) { fprintf(stderr, "%s: division by zero\n", simpler_at(line)); exit(1); } return a / b; }
long i_mod(long a, long b, long line) { if (b == 0) { fprintf(stderr, "%s: division by zero\n", simpler_at(line)); exit(1); } return a % b; }
static inline long s_at(long s, long i) { const char* p = (const char*)(intptr_t)s; SL* e = s_info(p); if (i < 0 || i >= e->nl) return (long)(intptr_t)""; long a = s_off(e, i); long b = a + 1; while (((unsigned char)p[b] & 192) == 128) b++; return s_let(p, a, b); }
long s_code(long s) { const unsigned char* p = (const unsigned char*)(intptr_t)s; long c = p[0]; int n = c >= 240 ? 3 : c >= 224 ? 2 : c >= 192 ? 1 : 0; long v = c & (63 >> n); for (int k = 1; k <= n; k++) { if ((p[k] & 192) != 128) return c; v = (v << 6) | (p[k] & 63); } return n ? v : c; }
static inline long s_slice(long s, long a, long b) { const char* p = (const char*)(intptr_t)s; SL* e = s_info(p); if (b < a) b = a; long x = s_off(e, a); return s_part(p, x, s_off(e, b)); }
long s_concat(long a, long b) { const char* x = (const char*)(intptr_t)a; const char* y = (const char*)(intptr_t)b; char* r = (char*)malloc(strlen(x) + strlen(y) + 1); strcpy(r, x); strcat(r, y); return (long)(intptr_t)r; }
static inline long s_eq(long a, long b) { const char* x = (const char*)(intptr_t)a; const char* y = (const char*)(intptr_t)b; return x == y || (x[0] == y[0] && strcmp(x, y) == 0); }
long i_tostr(long n) { char* r = (char*)malloc(24); sprintf(r, "%ld", n); return (long)(intptr_t)r; }
double l2d(long x) { union { long l; double d; } u; u.l = x; return u.d; }
long d2l(double x) { union { long l; double d; } u; u.d = x; return u.l; }
long f_tostr(long x) { char* r = (char*)malloc(32); snprintf(r, 32, "%.15g", l2d(x)); return (long)(intptr_t)r; }
long s_tofloat(long s) { return d2l(atof((const char*)(intptr_t)s)); }
long i_tof(long n) { return d2l((double)n); }
long f_toi(long x) { return (long)l2d(x); }
long s_toint(long s) { return atol((const char*)(intptr_t)s); }
long s_split(long s, long d) { const char* p = (const char*)(intptr_t)s; const char* dl = (const char*)(intptr_t)d; long dn = strlen(dl); long l = l_new(); while (1) { const char* q = dn ? strstr(p, dl) : 0; long n = q ? (long)(q - p) : (long)strlen(p); char* r = (char*)malloc(n + 1); memcpy(r, p, n); r[n] = 0; l_push(l, (long)(intptr_t)r); if (!q) break; p = q + dn; } return l; }
long l_range(long a, long b) { long l = l_new(); for (long k = a; k < b; k++) l_push(l, k); return l; }
long s_starts(long s, long t) { const char* p = (const char*)(intptr_t)s; const char* q = (const char*)(intptr_t)t; return strncmp(p, q, strlen(q)) == 0; }
long s_ends(long s, long t) { const char* p = (const char*)(intptr_t)s; const char* q = (const char*)(intptr_t)t; size_t a = strlen(p); size_t b = strlen(q); return b <= a && strcmp(p + a - b, q) == 0; }
long s_contains(long h, long n) { return strstr((const char*)(intptr_t)h, (const char*)(intptr_t)n) != 0; }
long s_trim(long s) { const char* p = (const char*)(intptr_t)s; long n = strlen(p); long a = 0; while (a < n && (p[a] == 32 || p[a] == 9 || p[a] == 13 || p[a] == 10)) a = a + 1; long b = n; while (b > a && (p[b - 1] == 32 || p[b - 1] == 9 || p[b - 1] == 13 || p[b - 1] == 10)) b = b - 1; long len = b - a; char* r = (char*)malloc(len + 1); for (long k = 0; k < len; k++) r[k] = p[a + k]; r[len] = 0; return (long)(intptr_t)r; }
long s_find(long h, long n) { const char* p = (const char*)(intptr_t)h; const char* q = strstr(p, (const char*)(intptr_t)n); if (!q) return -1; long m = 0; for (const char* c = p; c < q; c++) m += c == p || ((unsigned char)*c & 192) != 128; return m; }
static const char* js_ws(const char* p) { while (*p == 32 || *p == 9 || *p == 10 || *p == 13) p++; return p; }
static const char* js_str(const char* p) { p++; while (*p && *p != 34) { if (*p == 92 && p[1]) p++; p++; } if (*p) p++; return p; }
static const char* js_skip(const char* p) { p = js_ws(p); if (*p == 34) return js_str(p); if (*p == 123 || *p == 91) { long d = 0; while (*p) { if (*p == 34) { p = js_str(p); continue; } if (*p == 123 || *p == 91) d++; if (*p == 125 || *p == 93) { d--; if (d == 0) return p + 1; } p++; } return p; } while (*p && *p != 44 && *p != 125 && *p != 93 && *p != 32 && *p != 9 && *p != 10 && *p != 13) p++; return p; }
static long js_hex(const char* p) { long u = 0; for (int k = 0; k < 4; k++) { char h = p[k]; long v; if (h >= 48 && h <= 57) v = h - 48; else if ((h | 32) >= 97 && (h | 32) <= 102) v = (h | 32) - 87; else return -1; u = u * 16 + v; } return u; }
static long js_val(const char* p) { p = js_ws(p); const char* e = js_skip(p); char* r = (char*)malloc((e - p) + 1); long o = 0; if (*p != 34) { o = e - p; memcpy(r, p, o); r[o] = 0; return (long)(intptr_t)r; } p++; while (*p && *p != 34) { if (*p != 92 || p[1] == 0) { r[o++] = *p++; continue; } p++; char c = *p++; if (c == 110) r[o++] = 10; else if (c == 116) r[o++] = 9; else if (c == 114) r[o++] = 13; else if (c == 98) r[o++] = 8; else if (c == 102) r[o++] = 12; else if (c == 117) { long u = js_hex(p); if (u < 0) continue; p += 4; if (u >= 0xD800 && u <= 0xDBFF && p[0] == 92 && p[1] == 117) { long lo = js_hex(p + 2); if (lo >= 0xDC00 && lo <= 0xDFFF) { u = 0x10000 + ((u - 0xD800) << 10) + (lo - 0xDC00); p += 6; } } if (u == 0) continue; if (u < 0x80) r[o++] = (char)u; else if (u < 0x800) { r[o++] = (char)(0xC0 | (u >> 6)); r[o++] = (char)(0x80 | (u & 63)); } else if (u < 0x10000) { r[o++] = (char)(0xE0 | (u >> 12)); r[o++] = (char)(0x80 | ((u >> 6) & 63)); r[o++] = (char)(0x80 | (u & 63)); } else { r[o++] = (char)(0xF0 | (u >> 18)); r[o++] = (char)(0x80 | ((u >> 12) & 63)); r[o++] = (char)(0x80 | ((u >> 6) & 63)); r[o++] = (char)(0x80 | (u & 63)); } } else r[o++] = c; } r[o] = 0; return (long)(intptr_t)r; }
static const char* js_next(const char* p) { p = js_ws(js_skip(p)); return *p == 44 ? js_ws(p + 1) : 0; }
long s_json(long t, long k) { const char* p = js_ws((const char*)(intptr_t)t); const char* key = (const char*)(intptr_t)k; if (*p == 123) { p = js_ws(p + 1); while (p && *p == 34) { long kv = js_val(p); p = js_ws(js_str(p)); if (*p == 58) p++; if (strcmp((const char*)(intptr_t)kv, key) == 0) return js_val(p); p = js_next(p); } } else if (*p == 91) { char* end; long want = strtol(key, &end, 10); long i = 0; if (end != key && *end == 0 && want >= 0) { p = js_ws(p + 1); while (p && *p && *p != 93) { if (i == want) return js_val(p); i++; p = js_next(p); } } } return (long)(intptr_t)strdup(""); }
long s_jsonlist(long t, long keys) { long l = l_new(); const char* p = js_ws((const char*)(intptr_t)t); if (*p == 91 && !keys) { p = js_ws(p + 1); while (p && *p && *p != 93) { l_push(l, js_val(p)); p = js_next(p); } } else if (*p == 123) { p = js_ws(p + 1); while (p && *p == 34) { if (keys) l_push(l, js_val(p)); p = js_ws(js_str(p)); if (*p == 58) p++; if (!keys) l_push(l, js_val(p)); p = js_next(p); } } return l; }
typedef struct { long len; unsigned char* data; } SBytes;
long simpler_readbytes(const char* path) { SBytes* y = (SBytes*)malloc(sizeof(SBytes)); y->len = 0; size_t cap = 4096, got = 0, n; struct stat st; FILE* f = fopen(path, "rb"); if (f && fstat(fileno(f), &st) == 0 && st.st_size > 0) cap = (size_t)st.st_size + 1; y->data = (unsigned char*)malloc(cap); if (f) { while ((n = fread(y->data + got, 1, cap - got, f)) > 0) { got += n; if (got == cap) { cap *= 2; y->data = (unsigned char*)realloc(y->data, cap); } } fclose(f); } y->len = (long)got; return (long)(intptr_t)y; }
long y_find(long yp, long v, long from) { SBytes* y = (SBytes*)(intptr_t)yp; if (from < 0) from = 0; if (from >= y->len || v < 0 || v > 255) return -1; unsigned char* q = (unsigned char*)memchr(y->data + from, (int)v, (size_t)(y->len - from)); return q ? (long)(q - y->data) : -1; }
long y_len(long yp) { return ((SBytes*)(intptr_t)yp)->len; }
long y_atc(long yp, long i, long line) { SBytes* y = (SBytes*)(intptr_t)yp; if (i < 0 || i >= y->len) { fprintf(stderr, "%s: byte %ld, but there are %ld bytes\n", simpler_at(line), i, y->len); exit(1); } return y->data[i]; }
static SBytes* y_need(long yp, long i, long n, long line, const char* what) { SBytes* y = (SBytes*)(intptr_t)yp; if (i < 0 || n < 0 || i + n > y->len) { fprintf(stderr, "%s: %s wants %ld bytes from place %ld, but there are %ld\n", simpler_at(line), what, n, i, y->len); exit(1); } return y; }
long y_text(long yp, long i, long n, long line) { SBytes* y = y_need(yp, i, n, line, "textAt"); char* r = (char*)malloc(n + 1); long o = 0; for (long k = 0; k < n; k++) { unsigned char v = y->data[i + k]; if (v == 0) break; r[o++] = (char)v; } r[o] = 0; return (long)(intptr_t)r; }
long y_int(long yp, long i, long n, long line) { if (n < 1 || n > 8) { fprintf(stderr, "%s: intAt reads 1 to 8 bytes, got %ld\n", simpler_at(line), n); exit(1); } SBytes* y = y_need(yp, i, n, line, "intAt"); unsigned long u = 0; for (long k = n - 1; k >= 0; k--) u = (u << 8) | y->data[i + k]; return (long)u; }
long y_float(long yp, long i, long line) { SBytes* y = y_need(yp, i, 8, line, "floatAt"); unsigned long u = 0; for (long k = 7; k >= 0; k--) u = (u << 8) | y->data[i + k]; return (long)u; }
long simpler_writebytes(long path, long lp, long line) { SList* l = (SList*)(intptr_t)lp; for (long k = 0; k < l->len; k++) if (l->data[k] < 0 || l->data[k] > 255) { fprintf(stderr, "%s: a byte is a number from 0 to 255, but place %ld has %ld\n", simpler_at(line), k, l->data[k]); exit(1); } FILE* f = fopen((const char*)(intptr_t)path, "wb"); if (f) { for (long k = 0; k < l->len; k++) fputc((int)l->data[k], f); fclose(f); } return 0; }
static SList* b_need(long lp, long i, long n, long line, const char* what) { SList* l = (SList*)(intptr_t)lp; if (i < 0 || n < 0 || i + n > l->len) { fprintf(stderr, "%s: %s wants %ld bytes from place %ld, but the list has %ld\n", simpler_at(line), what, n, i, l->len); exit(1); } return l; }
long b_text(long lp, long i, long n, long line) { SList* l = b_need(lp, i, n, line, "textAt"); char* r = (char*)malloc(n + 1); long o = 0; for (long k = 0; k < n; k++) { long v = l->data[i + k] & 255; if (v == 0) break; r[o++] = (char)v; } r[o] = 0; return (long)(intptr_t)r; }
long b_int(long lp, long i, long n, long line) { if (n < 1 || n > 8) { fprintf(stderr, "%s: intAt reads 1 to 8 bytes, got %ld\n", simpler_at(line), n); exit(1); } SList* l = b_need(lp, i, n, line, "intAt"); unsigned long u = 0; for (long k = n - 1; k >= 0; k--) u = (u << 8) | (unsigned long)(l->data[i + k] & 255); return (long)u; }
long b_float(long lp, long i, long line) { SList* l = b_need(lp, i, 8, line, "floatAt"); unsigned long u = 0; for (long k = 7; k >= 0; k--) u = (u << 8) | (unsigned long)(l->data[i + k] & 255); return (long)u; }
long s_bytes(long s) { const unsigned char* p = (const unsigned char*)(intptr_t)s; long l = l_new(); while (*p) { l_push(l, *p); p++; } return l; }
enum { RC = 1, RANY, RSET, RBOL, REOL, RWB, RNWB, RSTART, REND, RGRP, RALT, RREP, RBREF, RLOOK };
#define RE_MAXG 32
#define RE_INF 1073741824
typedef struct RNode { int t, lo, hi, lazy, neg, g, nrg; long c; long* rg; struct RNode* sub; struct RNode* alt; struct RNode* next; } RNode;
typedef struct { RNode* root; int ng, fi, fm, fs, open, head; unsigned char first[256]; char* key; long line; const char* pp; const char* b; const char* e; const char* end; const char* cs[RE_MAXG]; const char* ce[RE_MAXG]; long steps, limit; const char* base; } RE;
typedef struct RK { int kind; RNode* n; struct RK* up; long count; const char* last; } RK;
static void re_die(RE* R, const char* why) { fprintf(stderr, "%s: the pattern \"%s\" %s\n", simpler_at(R->line), R->key, why); exit(1); }
static RNode* re_new(int t) { RNode* n = (RNode*)calloc(1, sizeof(RNode)); n->t = t; return n; }
static const char* re_dec(const char* p, long* c) { const unsigned char* u = (const unsigned char*)p; if (u[0] < 0x80) { *c = u[0]; return p + 1; } if (u[0] >= 0xC0 && u[0] < 0xE0 && (u[1] & 0xC0) == 0x80) { *c = ((long)(u[0] & 31) << 6) | (u[1] & 63); return p + 2; } if (u[0] >= 0xE0 && u[0] < 0xF0 && (u[1] & 0xC0) == 0x80 && (u[2] & 0xC0) == 0x80) { *c = ((long)(u[0] & 15) << 12) | ((long)(u[1] & 63) << 6) | (u[2] & 63); return p + 3; } if (u[0] >= 0xF0 && (u[1] & 0xC0) == 0x80 && (u[2] & 0xC0) == 0x80 && (u[3] & 0xC0) == 0x80) { *c = ((long)(u[0] & 7) << 18) | ((long)(u[1] & 63) << 12) | ((long)(u[2] & 63) << 6) | (u[3] & 63); return p + 4; } *c = 0x110000 + u[0]; return p + 1; }
static const unsigned int re_wt[] = {170,170,178,179,181,181,185,186,188,190,192,214,216,246,248,705,710,721,736,740,748,748,750,750,880,884,886,893,895,895,902,902,904,1013,1015,1153,1162,1369,1376,1416,1488,1522,1568,1610,1632,1641,1646,1647,1649,1747,1749,1749,1765,1766,1774,1788,1791,1791,1808,1808,1810,1839,1869,1957,1969,2026,2036,2037,2042,2042,2048,2069,2074,2074,2084,2084,2088,2088,2112,2136,2144,2183,2185,2190,2208,2249,2308,2361,2365,2365,2384,2384,2392,2401,2406,2415,2417,2432,2437,2489,2493,2493,2510,2510,2524,2529,2534,2545,2548,2553,2556,2556,2565,2617,2649,2671,2674,2676,2693,2745,2749,2749,2768,2785,2790,2799,2809,2809,2821,2873,2877,2877,2908,2913,2918,2927,2929,2935,2947,3001,3024,3024,3046,3058,3077,3129,3133,3133,3160,3169,3174,3183,3192,3198,3200,3200,3205,3257,3261,3261,3293,3297,3302,3314,3332,3386,3389,3389,3406,3406,3412,3414,3416,3425,3430,3448,3450,3455,3461,3526,3558,3567,3585,3632,3634,3635,3648,3654,3664,3673,3713,3760,3762,3763,3773,3782,3792,3840,3872,3891,3904,3948,3976,3980,4096,4138,4159,4169,4176,4181,4186,4189,4193,4193,4197,4198,4206,4208,4213,4225,4238,4238,4240,4249,4256,4346,4348,4954,4969,5007,5024,5117,5121,5740,5743,5759,5761,5786,5792,5866,5870,5905,5919,5937,5952,5969,5984,6000,6016,6067,6103,6103,6108,6108,6112,6137,6160,6276,6279,6312,6314,6430,6470,6618,6656,6678,6688,6740,6784,6809,6823,6823,6917,6963,6981,6988,6992,7001,7043,7072,7086,7141,7168,7203,7232,7293,7296,7359,7401,7404,7406,7411,7413,7414,7418,7615,7680,8124,8126,8126,8130,8140,8144,8155,8160,8172,8178,8188,8304,8313,8319,8329,8336,8348,8450,8450,8455,8455,8458,8467,8469,8469,8473,8477,8484,8484,8486,8486,8488,8488,8490,8493,8495,8505,8508,8511,8517,8521,8526,8526,8528,8585,9312,9371,9450,9471,10102,10131,11264,11492,11499,11502,11506,11507,11517,11517,11520,11631,11648,11742,11823,11823,12293,12295,12321,12329,12337,12341,12344,12348,12353,12438,12445,12447,12449,12538,12540,12686,12690,12693,12704,12735,12784,12799,12832,12841,12872,12879,12881,12895,12928,12937,12977,12991,13312,19903,19968,42124,42192,42237,42240,42508,42512,42606,42623,42653,42656,42735,42775,42783,42786,42888,42891,43009,43011,43013,43015,43018,43020,43042,43056,43061,43072,43123,43138,43187,43216,43225,43250,43255,43259,43259,43261,43262,43264,43301,43312,43334,43360,43388,43396,43442,43471,43481,43488,43492,43494,43560,43584,43586,43588,43595,43600,43609,43616,43638,43642,43642,43646,43695,43697,43697,43701,43702,43705,43709,43712,43712,43714,43741,43744,43754,43762,43764,43777,43866,43868,43881,43888,44002,44016,55291,63744,64285,64287,64296,64298,64433,64467,64829,64848,64967,65008,65019,65136,65276,65296,65305,65313,65338,65345,65370,65382,65500,65536,65786,65799,65843,65856,65912,65930,65931,66176,66256,66273,66421,66432,66461,66464,66511,66513,66915,66928,67669,67672,67702,67705,67867,67872,67897,67968,68096,68112,68149,68160,68168,68192,68222,68224,68295,68297,68324,68331,68335,68352,68405,68416,68497,68521,68899,68912,68965,68975,68997,69216,69289,69296,69316,69376,69445,69457,69460,69488,69505,69552,69622,69635,69687,69714,69743,69745,69746,69749,69749,69763,69807,69840,69881,69891,69926,69942,69951,69956,69956,69959,70002,70006,70006,70019,70066,70081,70084,70096,70106,70108,70108,70113,70187,70207,70208,70272,70312,70320,70366,70384,70393,70405,70457,70461,70461,70480,70480,70493,70497,70528,70583,70609,70609,70611,70611,70656,70708,70727,70730,70736,70745,70751,70831,70852,70853,70855,71086,71128,71131,71168,71215,71236,71257,71296,71338,71352,71352,71360,71450,71472,71483,71488,71723,71840,71983,71999,71999,72001,72001,72016,72144,72161,72161,72163,72163,72192,72192,72203,72242,72250,72250,72272,72272,72284,72329,72349,72349,72368,72440,72640,72672,72688,72750,72768,72768,72784,72812,72818,72847,72960,73008,73030,73030,73040,73097,73112,73458,73474,73474,73476,73523,73552,73561,73648,73684,73728,74862,74880,77808,77824,78895,78913,78918,78944,90397,90416,92777,92784,92909,92928,92975,92992,92995,93008,93548,93552,93846,93952,94026,94032,94032,94099,94177,94179,94179,94208,113817,118000,118009,119488,119539,119648,120512,120514,120538,120540,120570,120572,120596,120598,120628,120630,120654,120656,120686,120688,120712,120714,120744,120746,120770,120772,120831,122624,122666,122928,122989,123136,123180,123191,123214,123536,123565,123584,123627,123632,123641,124112,124139,124144,124397,124400,124410,124896,125135,125184,125251,125259,125273,126065,126123,126125,126127,126129,126253,126255,126651,127232,127244,130032,195101}; static int re_word(long c) { if (c < 128) return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'; int lo = 0, hi = (int)(sizeof(re_wt) / sizeof(re_wt[0]) / 2) - 1; while (lo <= hi) { int mid = (lo + hi) / 2; if (c < (long)re_wt[2 * mid]) hi = mid - 1; else if (c > (long)re_wt[2 * mid + 1]) lo = mid + 1; else return 1; } return 0; }
static long re_low(long c) { if ((c >= 'A' && c <= 'Z') || (c >= 0xC0 && c <= 0xDE && c != 0xD7)) return c + 32; return c; }
static long re_up(long c) { if ((c >= 'a' && c <= 'z') || (c >= 0xE0 && c <= 0xFE && c != 0xF7)) return c - 32; return c; }
static int re_cls(long cls, long c) { int r; long l = cls | 32; if (l == 'd') r = c >= '0' && c <= '9'; else if (l == 'w') r = re_word(c); else r = c == 32 || (c >= 9 && c <= 13); return (cls & 32) ? r : !r; }
static int re_inset(RE* R, RNode* n, long c) { long l = re_low(c), u = re_up(c); for (int i = 0; i < n->nrg; i += 2) { long lo = n->rg[i], hi = n->rg[i + 1]; if (lo == -1) { if (re_cls(hi, c)) return 1; } else if (c >= lo && c <= hi) return 1; else if (R->fi && ((l >= lo && l <= hi) || (u >= lo && u <= hi))) return 1; } return 0; }
static void re_setadd(RNode* n, long lo, long hi) { n->rg = (long*)realloc(n->rg, sizeof(long) * (n->nrg + 2)); n->rg[n->nrg++] = lo; n->rg[n->nrg++] = hi; }
static long re_hex(RE* R, int digits) { long u = 0; for (int k = 0; k < digits; k++) { char h = *R->pp; long v; if (h >= '0' && h <= '9') v = h - '0'; else if ((h | 32) >= 'a' && (h | 32) <= 'f') v = (h | 32) - 'a' + 10; else re_die(R, "has a \\x or \\u without its hex digits"); u = u * 16 + v; R->pp++; } return u; }
static long re_escc(RE* R, int* cls) { long c; *cls = 0; if (!*R->pp) re_die(R, "ends in a backslash"); R->pp = re_dec(R->pp, &c); switch (c) { case 'n': return 10; case 't': return 9; case 'r': return 13; case 'f': return 12; case 'v': return 11; case 'e': return 27; case 'b': return 8; case 'x': return re_hex(R, 2); case 'u': return re_hex(R, 4); case 'd': case 'D': case 'w': case 'W': case 's': case 'S': *cls = (int)c; return 0; } if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) re_die(R, "has a backslash before a letter that means nothing"); return c; }
static RNode* re_set(RE* R) { RNode* n = re_new(RSET); int first = 1; if (*R->pp == '^') { n->neg = 1; R->pp++; } while (1) { long lo, hi; int cls = 0; if (!*R->pp) re_die(R, "has a [ with no ]"); if (*R->pp == ']' && !first) { R->pp++; break; } first = 0; if (*R->pp == '\\') { R->pp++; lo = re_escc(R, &cls); } else R->pp = re_dec(R->pp, &lo); if (cls) { re_setadd(n, -1, cls); continue; } hi = lo; if (*R->pp == '-' && R->pp[1] && R->pp[1] != ']') { int c2 = 0; R->pp++; if (*R->pp == '\\') { R->pp++; hi = re_escc(R, &c2); if (c2) re_die(R, "has a range that ends in a class"); } else R->pp = re_dec(R->pp, &hi); if (hi < lo) re_die(R, "has a range that runs backwards"); } re_setadd(n, lo, hi); } return n; }
static RNode* re_alt(RE* R, int depth);
static RNode* re_atom(RE* R, int depth) {
  long c; char c0 = *R->pp; RNode* n;
  if (c0 == '(') { n = re_new(RGRP); R->pp++; int cap = 1;
    if (*R->pp == '?') { char f = R->pp[1]; cap = 0;
      if (f == ':') R->pp += 2;
      else if (f == '=' || f == '!') { n->t = RLOOK; n->c = 1; n->neg = f == '!'; R->pp += 2; }
      else if (f == '<' && (R->pp[2] == '=' || R->pp[2] == '!')) { n->t = RLOOK; n->c = 2; n->neg = R->pp[2] == '!'; R->pp += 3; }
      else if (f == '<' || (f == 'P' && R->pp[2] == '<')) { while (*R->pp && *R->pp != '>') R->pp++; if (!*R->pp) re_die(R, "has a group name with no >"); R->pp++; cap = 1; }
      else { const char* q = R->pp + 1; while (*q == 'i' || *q == 'm' || *q == 's') q++; if (*q != ')' || q == R->pp + 1) re_die(R, "has a (? form that is not known"); for (q = R->pp + 1; *q != ')'; q++) { if (*q == 'i') R->fi = 1; if (*q == 'm') R->fm = 1; if (*q == 's') R->fs = 1; } R->pp = q + 1; return n; } }
    if (cap) { if (R->ng + 1 >= RE_MAXG) re_die(R, "has more than 31 groups"); n->g = ++R->ng; }
    n->sub = re_alt(R, depth + 1); if (*R->pp != ')') re_die(R, "has a ( with no )"); R->pp++; return n; }
  if (c0 == '[') { R->pp++; return re_set(R); }
  if (c0 == '.') { R->pp++; return re_new(RANY); }
  if (c0 == '^') { R->pp++; return re_new(RBOL); }
  if (c0 == '$') { R->pp++; return re_new(REOL); }
  if (c0 == '*' || c0 == '+' || c0 == '?') re_die(R, "has a * + or ? with nothing before it");
  if (c0 == '\\') { int cls; char e = R->pp[1]; R->pp++;
    if (e == 'b') { R->pp++; return re_new(RWB); } if (e == 'B') { R->pp++; return re_new(RNWB); }
    if (e == 'A') { R->pp++; return re_new(RSTART); } if (e == 'Z' || e == 'z') { R->pp++; return re_new(REND); }
    if (e >= '1' && e <= '9') { R->pp++; n = re_new(RBREF); n->g = e - '0'; return n; }
    c = re_escc(R, &cls); if (cls) { n = re_new(RSET); re_setadd(n, -1, cls); return n; } n = re_new(RC); n->c = c; return n; }
  R->pp = re_dec(R->pp, &c); n = re_new(RC); n->c = c; return n; }
static RNode* re_seq(RE* R, int depth) { RNode* head = 0; RNode** tail = &head; while (*R->pp && *R->pp != '|' && *R->pp != ')') { RNode* a = re_atom(R, depth); while (1) { long lo, hi; const char* q = R->pp; char c = *q; if (c == '*') { lo = 0; hi = RE_INF; q++; } else if (c == '+') { lo = 1; hi = RE_INF; q++; } else if (c == '?') { lo = 0; hi = 1; q++; } else if (c == '{' && ((q[1] >= '0' && q[1] <= '9') || (q[1] == ',' && q[2] >= '0' && q[2] <= '9'))) { char* t = (char*)q + 1; lo = 0; if (*t != ',') lo = strtol(q + 1, &t, 10); hi = lo; if (*t == ',') { t++; if (*t == '}') hi = RE_INF; else if (*t >= '0' && *t <= '9') hi = strtol(t, &t, 10); else break; } if (*t != '}') break; q = t + 1; if (hi < lo) re_die(R, "has a count that runs backwards"); if (lo > 1000 || (hi != RE_INF && hi > 1000)) re_die(R, "has a count above 1000"); } else break; RNode* r = re_new(RREP); r->lo = (int)lo; r->hi = (int)hi; r->sub = a; if (*q == '?') { r->lazy = 1; q++; } else if (*q == '+') q++; R->pp = q; a = r; } *tail = a; tail = &a->next; } return head; }
static RNode* re_alt(RE* R, int depth) { if (depth > 200) re_die(R, "has groups more than 200 deep"); RNode* first = re_seq(R, depth); if (*R->pp != '|') return first; RNode* head = re_new(RALT); RNode* cur = head; head->sub = first; while (*R->pp == '|') { RNode* a = re_new(RALT); R->pp++; a->sub = re_seq(R, depth); cur->alt = a; cur = a; } return head; }
static void re_enc(RE* R, long c) { if (c < 0x80) R->first[c] = 1; else if (c < 0x800) R->first[0xC0 | (c >> 6)] = 1; else if (c < 0x10000) R->first[0xE0 | (c >> 12)] = 1; else if (c < 0x110000) R->first[0xF0 | (c >> 18)] = 1; else R->first[(c - 0x110000) & 255] = 1; }
static int re_first(RE* R, RNode* n) { for (; n; n = n->next) { int nul = 1; switch (n->t) { case RC: re_enc(R, n->c); if (R->fi) { re_enc(R, re_low(n->c)); re_enc(R, re_up(n->c)); } nul = 0; break; case RANY: memset(R->first, 1, 256); nul = 0; break; case RSET: for (long b = 0; b < 128; b++) if (re_inset(R, n, b) != n->neg) R->first[b] = 1; memset(R->first + 128, 1, 128); nul = 0; break; case RGRP: nul = re_first(R, n->sub); break; case RALT: nul = 0; for (RNode* a = n; a; a = a->alt) if (re_first(R, a->sub)) nul = 1; break; case RREP: nul = re_first(R, n->sub); if (n->lo == 0) nul = 1; break; case RBREF: memset(R->first, 1, 256); break; default: break; } if (!nul) return 0; } return 1; }
static RE* re_comp(long pat, long line) { static RE* cache[512]; const char* src = (const char*)(intptr_t)pat; unsigned long h = 5381; for (const unsigned char* q = (const unsigned char*)src; *q; q++) h = h * 33 + *q; RE* R = cache[h & 511]; if (R && strcmp(R->key, src) == 0) { R->line = line; return R; } R = (RE*)calloc(1, sizeof(RE)); R->key = strdup(src); R->pp = R->key; R->line = line; R->root = re_alt(R, 0); if (*R->pp == ')') re_die(R, "has a ) with no ("); R->open = re_first(R, R->root); R->head = R->root && !R->fm && (R->root->t == RSTART || R->root->t == RBOL); if (R->root && R->root->t == RSTART) R->head = 1; cache[h & 511] = R; return R; }
static int re_m(RE* R, RNode* n, const char* p, RK* k);
static const char* re_one(RE* R, RNode* n, const char* p) { long c; const char* q; if (p >= R->e) return 0; q = re_dec(p, &c); if (n->t == RC) return (c == n->c || (R->fi && re_low(c) == re_low(n->c))) ? q : 0; if (n->t == RANY) return (c != 10 || R->fs) ? q : 0; return re_inset(R, n, c) != n->neg ? q : 0; }
static int re_rep(RE* R, RNode* n, const char* p, RK* k, long count, const char* last) {
  RNode* s = n->sub;
  if (s && !s->next && (s->t == RC || s->t == RANY || s->t == RSET)) { const char* q = p; const char* r; long c = 0;
    if (!n->lazy) { while (c < n->hi && (r = re_one(R, s, q))) { q = r; c++; } if (c < n->lo) return 0; while (1) { if (re_m(R, n->next, q, k)) return 1; if (c == n->lo || q == p) return 0; q--; while (q > p && ((unsigned char)*q & 0xC0) == 0x80) q--; c--; } }
    while (c < n->lo) { r = re_one(R, s, q); if (!r) return 0; q = r; c++; }
    while (1) { if (re_m(R, n->next, q, k)) return 1; if (c >= n->hi) return 0; r = re_one(R, s, q); if (!r) return 0; q = r; c++; } }
  if (count > 0 && p == last && count >= n->lo) return re_m(R, n->next, p, k);
  RK kk = { 1, n, k, count + 1, p };
  if (!n->lazy) { if (count < n->hi && re_m(R, s, p, &kk)) return 1; return count >= n->lo ? re_m(R, n->next, p, k) : 0; }
  if (count >= n->lo && re_m(R, n->next, p, k)) return 1;
  return count < n->hi ? re_m(R, s, p, &kk) : 0; }
static int re_k(RE* R, const char* p, RK* k) {
  if (!k) { R->end = p; return 1; }
  if (k->kind == 0) { RNode* n = k->n; if (n->g > 0) { const char* os = R->cs[n->g]; const char* oe = R->ce[n->g]; R->cs[n->g] = k->last; R->ce[n->g] = p; if (re_m(R, n->next, p, k->up)) return 1; R->cs[n->g] = os; R->ce[n->g] = oe; return 0; } return re_m(R, n->next, p, k->up); }
  if (k->kind == 1) return re_rep(R, k->n, p, k->up, k->count, k->last);
  if (k->kind == 2) return 1;
  return p == k->last; }
static int re_look(RE* R, RNode* n, const char* p, RK* k) { const char* scs[RE_MAXG]; const char* sce[RE_MAXG]; int ok = 0; memcpy(scs, R->cs, sizeof(scs)); memcpy(sce, R->ce, sizeof(sce)); if (n->c == 1) { RK kk = { 2, 0, 0, 0, 0 }; ok = re_m(R, n->sub, p, &kk); } else { RK kk = { 3, 0, 0, 0, p }; const char* q = p; while (1) { if (re_m(R, n->sub, q, &kk)) { ok = 1; break; } if (q == R->b) break; q--; while (q > R->b && ((unsigned char)*q & 0xC0) == 0x80) q--; } } if (ok != n->neg && re_m(R, n->next, p, k)) return 1; memcpy(R->cs, scs, sizeof(scs)); memcpy(R->ce, sce, sizeof(sce)); return 0; }
static long re_budget = 0;
static int re_m(RE* R, RNode* n, const char* p, RK* k) {
  char here;
  if (++R->steps > R->limit) re_die(R, "takes too many steps on this text");
  if (R->base - &here > re_budget) re_die(R, "goes too deep on this text");
  while (n && n->t <= RSET) { p = re_one(R, n, p); if (!p) return 0; n = n->next; }
  if (!n) return re_k(R, p, k);
  switch (n->t) {
    case RBOL: return (p == R->b || (R->fm && p[-1] == 10)) ? re_m(R, n->next, p, k) : 0;
    case REOL: return (p == R->e || (R->fm && *p == 10) || (p + 1 == R->e && *p == 10)) ? re_m(R, n->next, p, k) : 0;
    case RSTART: return p == R->b ? re_m(R, n->next, p, k) : 0;
    case REND: return p == R->e ? re_m(R, n->next, p, k) : 0;
    case RWB: case RNWB: { long ca = 0, cb = 0; if (p > R->b) { const char* q = p - 1; while (q > R->b && ((unsigned char)*q & 192) == 128) q--; re_dec(q, &ca); } if (p < R->e) re_dec(p, &cb); int a = p > R->b && re_word(ca); int b = p < R->e && re_word(cb); return ((a != b) == (n->t == RWB)) ? re_m(R, n->next, p, k) : 0; }
    case RGRP: { RK kk = { 0, n, k, 0, p }; return re_m(R, n->sub, p, &kk); }
    case RALT: { for (RNode* a = n; a; a = a->alt) if (re_m(R, a->sub, p, k)) return 1; return 0; }
    case RREP: return re_rep(R, n, p, k, 0, 0);
    case RBREF: { const char* s = n->g <= R->ng ? R->cs[n->g] : 0; const char* e = n->g <= R->ng ? R->ce[n->g] : 0; long len; if (!s || !e) return 0; len = e - s; if (R->e - p < len) return 0; for (long i = 0; i < len; i++) if (s[i] != p[i] && !(R->fi && (unsigned char)s[i] < 128 && re_low((unsigned char)s[i]) == re_low((unsigned char)p[i]))) return 0; return re_m(R, n->next, p + len, k); }
    case RLOOK: return re_look(R, n, p, k);
  }
  return 0; }
static const char* re_step(RE* R, const char* p) { if (p >= R->e) return R->e + 1; p++; while (p < R->e && ((unsigned char)*p & 0xC0) == 0x80) p++; return p; }
static const char* re_search(RE* R, const char* from) { char mark; const char* p = from; RNode* f = R->root; int lit = f && f->t == RC && !R->fi && f->c < 128 && f->c > 0; R->steps = 0; R->base = &mark; while (1) { if (lit) { p = (const char*)memchr(p, (int)f->c, (size_t)(R->e - p)); if (!p) return 0; } else if (!R->open) { while (p < R->e && !R->first[(unsigned char)*p]) p++; if (p >= R->e) return 0; } for (int i = 0; i <= R->ng; i++) { R->cs[i] = 0; R->ce[i] = 0; } if (re_m(R, R->root, p, 0)) { R->cs[0] = p; R->ce[0] = R->end; return p; } if (p >= R->e || R->head) return 0; p = re_step(R, p); } }
static void re_stack(void) { struct rlimit rl; long cur = 8388608; long want = 67108864; if (getrlimit(RLIMIT_STACK, &rl) == 0) {
#ifdef __linux__
if (rl.rlim_cur != RLIM_INFINITY && (long)rl.rlim_cur < want && (rl.rlim_max == RLIM_INFINITY || (long)rl.rlim_max >= want)) { rl.rlim_cur = (rlim_t)want; setrlimit(RLIMIT_STACK, &rl); getrlimit(RLIMIT_STACK, &rl); }
#endif
cur = (rl.rlim_cur == RLIM_INFINITY || (long)rl.rlim_cur > want) ? want : (long)rl.rlim_cur; } re_budget = cur / 2; }
static RE* re_open(long s, long pat, long line) { RE* R = re_comp(pat, line); if (!re_budget) re_stack(); R->b = (const char*)(intptr_t)s; R->e = R->b + strlen(R->b); R->limit = 1000000 + 100 * (long)(R->e - R->b); return R; }
static long re_str(const char* a, const char* b) { long n = (a && b && b > a) ? (long)(b - a) : 0; char* r = (char*)malloc(n + 1); if (n) memcpy(r, a, n); r[n] = 0; return (long)(intptr_t)r; }
static long re_groups(RE* R) { long l = l_new(); for (int i = 0; i <= R->ng; i++) l_push(l, re_str(R->cs[i], R->ce[i])); return l; }
static const char* re_after(RE* R) { return R->ce[0] > R->cs[0] ? R->ce[0] : re_step(R, R->ce[0]); }
long re_test(long s, long pat, long line) { RE* R = re_open(s, pat, line); return re_search(R, R->b) != 0; }
long re_find(long s, long pat, long line) { RE* R = re_open(s, pat, line); if (!re_search(R, R->b)) return l_new(); return re_groups(R); }
long re_all(long s, long pat, long line) { RE* R = re_open(s, pat, line); long out = l_new(); const char* p = R->b; while (p <= R->e && re_search(R, p)) { l_push(out, re_groups(R)); p = re_after(R); } return out; }
long re_split(long s, long pat, long line) { RE* R = re_open(s, pat, line); long out = l_new(); const char* p = R->b; const char* last = p; while (p <= R->e && re_search(R, p)) { if (R->ce[0] == R->cs[0]) { p = re_after(R); continue; } l_push(out, re_str(last, R->cs[0])); last = R->ce[0]; p = last; } l_push(out, re_str(last, R->e)); return out; }
static void re_put(char** b, size_t* n, size_t* cap, const char* a, size_t len) { if (*n + len + 1 > *cap) { *cap = (*n + len + 1) * 2; *b = (char*)realloc(*b, *cap); } if (len) memcpy(*b + *n, a, len); *n += len; (*b)[*n] = 0; }
long re_replace(long s, long pat, long to, long line) { RE* R = re_open(s, pat, line); const char* t = (const char*)(intptr_t)to; size_t n = 0, cap = 64; char* out = (char*)malloc(cap); const char* p = R->b; const char* last = p; out[0] = 0; while (p <= R->e && re_search(R, p)) { re_put(&out, &n, &cap, last, (size_t)(R->cs[0] - last)); for (const char* q = t; *q; q++) { if (*q == '$' && q[1] == '$') { re_put(&out, &n, &cap, q, 1); q++; } else if (*q == '$' && q[1] >= '0' && q[1] <= '9') { int g = q[1] - '0'; q++; if (q[1] >= '0' && q[1] <= '9' && g * 10 + (q[1] - '0') <= R->ng) { g = g * 10 + (q[1] - '0'); q++; } if (g <= R->ng && R->cs[g] && R->ce[g] > R->cs[g]) re_put(&out, &n, &cap, R->cs[g], (size_t)(R->ce[g] - R->cs[g])); } else re_put(&out, &n, &cap, q, 1); } last = R->ce[0]; p = re_after(R); } if (last < R->e) re_put(&out, &n, &cap, last, (size_t)(R->e - last)); return (long)(intptr_t)out; }
long s_case(long s, long up) { const unsigned char* p = (const unsigned char*)(intptr_t)s; long n = strlen((const char*)p); unsigned char* r = (unsigned char*)malloc(n + 1); for (long i = 0; i <= n; i++) { unsigned char c = p[i]; if (up && c >= 97 && c <= 122) c = c - 32; if (!up && c >= 65 && c <= 90) c = c + 32; if (i > 0 && p[i - 1] == 195) { if (up && c >= 160 && c <= 190 && c != 183) c = c - 32; if (!up && c >= 128 && c <= 158 && c != 151) c = c + 32; } r[i] = c; } return (long)(intptr_t)r; }
long l_join(long lp, long sep) { SList* s = (SList*)(intptr_t)lp; const char* q = (const char*)(intptr_t)sep; long m = strlen(q); long n = 1; for (long i = 0; i < s->len; i++) n += strlen((const char*)(intptr_t)s->data[i]) + m; char* r = (char*)malloc(n); r[0] = 0; for (long i = 0; i < s->len; i++) { if (i > 0) strcat(r, q); strcat(r, (const char*)(intptr_t)s->data[i]); } return (long)(intptr_t)r; }
long s_padleft(long s, long n) { const char* p = (const char*)(intptr_t)s; SL* e = s_info(p); long nb = e->nb; long pad = n - e->nl; if (pad <= 0) return s; char* r = (char*)malloc(nb + pad + 1); memset(r, 32, pad); memcpy(r + pad, p, nb + 1); return (long)(intptr_t)r; }
long s_padright(long s, long n) { const char* p = (const char*)(intptr_t)s; SL* e = s_info(p); long nb = e->nb; long pad = n - e->nl; if (pad <= 0) return s; char* r = (char*)malloc(nb + pad + 1); memcpy(r, p, nb); memset(r + nb, 32, pad); r[nb + pad] = 0; return (long)(intptr_t)r; }
long s_replace(long s, long from, long to) { const char* p = (const char*)(intptr_t)s; const char* f = (const char*)(intptr_t)from; const char* t = (const char*)(intptr_t)to; long fl = strlen(f); if (fl == 0) return s; long tl = strlen(t); long pl = strlen(p); long count = 0; const char* q = p; while ((q = strstr(q, f)) != 0) { count++; q += fl; } char* r = (char*)malloc(pl + count * (tl - fl) + 1); char* w = r; const char* a = p; while (1) { const char* hit = strstr(a, f); if (hit == 0) { strcpy(w, a); break; } long pre = hit - a; memcpy(w, a, pre); w += pre; memcpy(w, t, tl); w += tl; a = hit + fl; } return (long)(intptr_t)r; }
typedef struct { long* keys; long* vals; unsigned long* hs; int* tab; long len; long cap; long tcap; } SMap;
static unsigned long m_hash(const char* s) { unsigned long h = 1469598103934665603UL; while (*s) { h ^= (unsigned char)*s++; h *= 1099511628211UL; } return h; }
static long m_findh(SMap* m, const char* key, unsigned long h) { long mask = m->tcap - 1; long j = (long)(h & (unsigned long)mask); while (m->tab[j]) { long i = m->tab[j] - 1; if (m->hs[i] == h && strcmp((const char*)(intptr_t)m->keys[i], key) == 0) return i; j = (j + 1) & mask; } return -1; }
long m_find(long mp, long k) { const char* key = (const char*)(intptr_t)k; return m_findh((SMap*)(intptr_t)mp, key, m_hash(key)); }
long Map() { SMap* m = (SMap*)malloc(sizeof(SMap)); m->cap = 8; m->len = 0; m->keys = (long*)malloc(sizeof(long) * m->cap); m->vals = (long*)malloc(sizeof(long) * m->cap); m->hs = (unsigned long*)malloc(sizeof(unsigned long) * m->cap); m->tcap = 16; m->tab = (int*)calloc(m->tcap, sizeof(int)); return (long)(intptr_t)m; }
long m_set(long mp, long k, long v) { SMap* m = (SMap*)(intptr_t)mp; const char* key = (const char*)(intptr_t)k; unsigned long h = m_hash(key); long i = m_findh(m, key, h); if (i >= 0) { m->vals[i] = v; return 0; } if (m->len == m->cap) { m->cap = m->cap * 2; m->keys = (long*)realloc(m->keys, sizeof(long) * m->cap); m->vals = (long*)realloc(m->vals, sizeof(long) * m->cap); m->hs = (unsigned long*)realloc(m->hs, sizeof(unsigned long) * m->cap); } if ((m->len + 1) * 2 > m->tcap) { m->tcap = m->tcap * 2; free(m->tab); m->tab = (int*)calloc(m->tcap, sizeof(int)); for (long q = 0; q < m->len; q++) { long j = (long)(m->hs[q] & (unsigned long)(m->tcap - 1)); while (m->tab[j]) j = (j + 1) & (m->tcap - 1); m->tab[j] = (int)(q + 1); } } m->keys[m->len] = k; m->vals[m->len] = v; m->hs[m->len] = h; long j = (long)(h & (unsigned long)(m->tcap - 1)); while (m->tab[j]) j = (j + 1) & (m->tcap - 1); m->tab[j] = (int)(m->len + 1); m->len = m->len + 1; return 0; }
long m_get(long mp, long k) { long i = m_find(mp, k); if (i >= 0) return ((SMap*)(intptr_t)mp)->vals[i]; return 0; }
long m_gets(long mp, long k) { long i = m_find(mp, k); if (i >= 0) return ((SMap*)(intptr_t)mp)->vals[i]; return (long)(intptr_t)""; }
long m_getp(long mp, long k, long line) { long i = m_find(mp, k); if (i >= 0) return ((SMap*)(intptr_t)mp)->vals[i]; fprintf(stderr, "%s: the map has no key \"%s\"; ask .has(key) first\n", simpler_at(line), (const char*)(intptr_t)k); exit(1); return 0; }
long m_has(long mp, long k) { return m_find(mp, k) >= 0; }
long m_keys(long mp) { SMap* m = (SMap*)(intptr_t)mp; long l = l_new(); for (long i = 0; i < m->len; i++) l_push(l, m->keys[i]); return l; }
long m_byvalue(long mp) { SMap* m = (SMap*)(intptr_t)mp; long n = m->len; long c = n > 0 ? n : 1; long* idx = (long*)malloc(sizeof(long) * c); for (long i = 0; i < n; i++) idx[i] = i; for (long i = 1; i < n; i++) { long t = idx[i]; long j = i - 1; while (j >= 0 && m->vals[idx[j]] < m->vals[t]) { idx[j + 1] = idx[j]; j = j - 1; } idx[j + 1] = t; } long l = l_new(); for (long i = 0; i < n; i++) l_push(l, m->keys[idx[i]]); return l; }
int cmp_long(const void* a, const void* b) { long x = *(const long*)a; long y = *(const long*)b; if (x < y) return -1; if (x > y) return 1; return 0; }
int cmp_str(const void* a, const void* b) { return strcmp((const char*)(intptr_t)(*(const long*)a), (const char*)(intptr_t)(*(const long*)b)); }
long l_sort(long lp, long isStr) { SList* s = (SList*)(intptr_t)lp; long n = s->len; long c = n > 0 ? n : 1; long* d = (long*)malloc(sizeof(long) * c); for (long i = 0; i < n; i++) d[i] = s->data[i]; qsort(d, n, sizeof(long), isStr ? cmp_str : cmp_long); long r = l_new(); for (long i = 0; i < n; i++) l_push(r, d[i]); return r; }
const char* simpler_read(const char* path) { FILE* f = fopen(path, "rb"); if (!f) return ""; size_t cap = 4096, got = 0, n; char* buf = (char*)malloc(cap + 1); while ((n = fread(buf + got, 1, cap - got, f)) > 0) { got += n; if (got == cap) { cap *= 2; buf = (char*)realloc(buf, cap + 1); } } buf[got] = 0; fclose(f); return buf; }
long simpler_write(long path, long content) { FILE* f = fopen((const char*)(intptr_t)path, "w"); if (f) { fputs((const char*)(intptr_t)content, f); fclose(f); } return 0; }
long simpler_append(long path, long content) { FILE* f = fopen((const char*)(intptr_t)path, "a"); if (f) { fputs((const char*)(intptr_t)content, f); fclose(f); } return 0; }
long simpler_args(int argc, char** argv) { long l = l_new(); for (int k = 1; k < argc; k++) l_push(l, (long)(intptr_t)argv[k]); return l; }
static int simpler_glob(const char* pat, const char* s) { const char* star = 0; const char* mark = 0; while (*s) { if (*pat == '*') { star = pat++; mark = s; } else if (*pat == *s) { pat++; s++; } else if (star && *mark != '/') { pat = star + 1; s = ++mark; } else return 0; } while (*pat == '*') pat++; return *pat == 0; }
static void simpler_inside(const char* dir, const char* p, long line, const char* verb) { char rd[PATH_MAX], rp[PATH_MAX], tmp[PATH_MAX]; if (strlen(p) >= PATH_MAX || !realpath(dir, rd)) return; if (!realpath(p, rp)) { struct stat st; int link = lstat(p, &st) == 0 && S_ISLNK(st.st_mode); strcpy(tmp, p); char* sl = strrchr(tmp, '/'); if (!sl) strcpy(tmp, "."); else if (sl == tmp) tmp[1] = 0; else *sl = 0; if (!link && !realpath(tmp, rp)) return; if (link) rp[0] = 0; } size_t n = strlen(rd); if (n == 1 || (strncmp(rd, rp, n) == 0 && (rp[n] == 0 || rp[n] == '/'))) return; if ((strncmp(rd, "/sys", 4) == 0 && strncmp(rp, "/sys/", 5) == 0) || (strncmp(rd, "/proc", 5) == 0 && strncmp(rp, "/proc/", 6) == 0)) return; fprintf(stderr, "%s: this program may not %s \"%s\": a link leads out of \"%s\"\n", simpler_at(line), verb, p, dir); exit(1); }
int simpler_argc = 0; char** simpler_argv = 0; static const char* simpler_home(const char* p) { const char* h = getenv("HOME"); if (p[0] != '~' || p[1] != '/' || !h) return p; char* r = (char*)malloc(strlen(h) + strlen(p) + 1); strcpy(r, h); strcat(r, p + 1); return r; } long simpler_may(long path, const char** ok, long line, const char* verb) { const char* p = simpler_home((const char*)(intptr_t)path); path = (long)(intptr_t)p; for (int k = 0; ok[k]; k++) { const char* e = simpler_home(ok[k]); size_t n = strlen(e); if (strcmp(e, "<args>") == 0) { for (int a = 1; a < simpler_argc; a++) if (strcmp(simpler_argv[a], p) == 0) return path; continue; } if (strcmp(e, p) == 0) return path; if (strchr(e, '*') && simpler_glob(e, p) && strstr(p, "..") == 0) { char d[PATH_MAX]; size_t m = strcspn(e, "*"); while (m > 0 && e[m - 1] != '/') m--; if (m == 0) strcpy(d, "."); else if (m < PATH_MAX) { memcpy(d, e, m); d[m] = 0; } else d[0] = 0; if (d[0]) simpler_inside(d, p, line, verb); return path; } if (strcmp(e, "./") == 0 && p[0] != '/' && strstr(p, "..") == 0) { simpler_inside(".", p, line, verb); return path; } if (n > 0 && e[n - 1] == '/' && strncmp(e, p, n) == 0 && strstr(p, "..") == 0) { simpler_inside(e, p, line, verb); return path; } } fprintf(stderr, "%s: this program may not %s \"%s\"\n", simpler_at(line), verb, p); exit(1); return 0; }
long simpler_dir(long path) { const char* p = (const char*)(intptr_t)path; size_t n = strlen(p); if (n > 0 && p[n - 1] == '/') return path; char* r = (char*)malloc(n + 2); strcpy(r, p); r[n] = '/'; r[n + 1] = 0; return (long)(intptr_t)r; }
long simpler_list(long path) { const char* p = (const char*)(intptr_t)path; long l = l_new(); DIR* d = opendir(p); if (!d) return l; struct dirent* e; while ((e = readdir(d)) != 0) { if (strcmp(e->d_name, ".") == 0 || strcmp(e->d_name, "..") == 0) continue; size_t n = strlen(p) + strlen(e->d_name) + 2; char* full = (char*)malloc(n); strcpy(full, p); strcat(full, e->d_name); struct stat st; char* name = (char*)malloc(strlen(e->d_name) + 2); strcpy(name, e->d_name); if (stat(full, &st) == 0 && S_ISDIR(st.st_mode)) strcat(name, "/"); free(full); l_push(l, (long)(intptr_t)name); } closedir(d); SList* s = (SList*)(intptr_t)l; qsort(s->data, s->len, sizeof(long), cmp_str); return l; }
long simpler_exists(long path) { struct stat st; return stat((const char*)(intptr_t)path, &st) == 0; }
long simpler_rename(long a, long b, long line) { if (rename((const char*)(intptr_t)a, (const char*)(intptr_t)b) != 0) { fprintf(stderr, "%s: could not rename \"%s\": %s\n", simpler_at(line), (const char*)(intptr_t)a, strerror(errno)); exit(1); } return 0; }
long simpler_delete(long a, long line) { if (remove((const char*)(intptr_t)a) != 0) { fprintf(stderr, "%s: could not delete \"%s\": %s\n", simpler_at(line), (const char*)(intptr_t)a, strerror(errno)); exit(1); } return 0; }
const char* simpler_rd[] = {"*.smplr", 0};
const char* simpler_wr[] = {0};
const char* simpler_rn[] = {0};
const char* simpler_ft[] = {0};
const char* simpler_ml[] = {0};
static int simpler_quiet = 0; long simpler_exec(char** av, int capture, int* code, long input) { int fd[2] = {0, 0}; FILE* in = 0; if (input) { in = tmpfile(); if (in) { fputs((const char*)(intptr_t)input, in); fflush(in); rewind(in); } } if (capture && pipe(fd) != 0) { *code = 127; return (long)(intptr_t)""; } fflush(stdout); pid_t pid = fork(); if (pid < 0) { *code = 127; return (long)(intptr_t)""; } if (pid == 0) { if (simpler_quiet && !freopen("/dev/null", "w", stderr)) _exit(127); if (in) dup2(fileno(in), 0); if (capture) { dup2(fd[1], 1); close(fd[0]); close(fd[1]); } execvp(av[0], av); _exit(127); } if (in) fclose(in); long cap = 1024; long len = 0; char* buf = (char*)malloc(cap); if (capture) { close(fd[1]); long n; while ((n = read(fd[0], buf + len, cap - len - 1)) > 0) { len += n; if (len + 1 >= cap) { cap *= 2; buf = (char*)realloc(buf, cap); } } close(fd[0]); } buf[len] = 0; int st = 0; waitpid(pid, &st, 0); *code = WIFEXITED(st) ? WEXITSTATUS(st) : 128; return (long)(intptr_t)buf; }
char** simpler_argv_of(long name, long args) { SList* l = (SList*)(intptr_t)args; char** av = (char**)malloc(sizeof(char*) * (l->len + 2)); av[0] = (char*)(intptr_t)name; for (long k = 0; k < l->len; k++) av[k + 1] = (char*)(intptr_t)l->data[k]; av[l->len + 1] = 0; return av; }
long simpler_runok(long name, long line) { const char* p = (const char*)(intptr_t)name; for (int k = 0; simpler_rn[k]; k++) if (strcmp(simpler_rn[k], p) == 0) return name; fprintf(stderr, "%s: this program may not run \"%s\"\n", simpler_at(line), p); exit(1); return 0; }
long simpler_send(long to, long subject, long body, long line) { const char* t = (const char*)(intptr_t)to; const char* s = (const char*)(intptr_t)subject; const char* b = (const char*)(intptr_t)body; size_t tn = strlen(t); int ok = 0; const char* at = strchr(t, 64); int clean = at && at != t && at[1] && strchr(at + 1, 64) == 0 && t[0] != '-'; for (const char* c = t; *c; c++) if ((unsigned char)*c <= 32 || *c == ',' || *c == ';' || *c == '<' || *c == '>' || *c == 127) clean = 0; for (int k = 0; clean && simpler_ml[k]; k++) { const char* e = simpler_ml[k]; size_t n = strlen(e); if (strcmp(e, t) == 0 || (e[0] == 64 && tn > n && strcmp(t + tn - n, e) == 0) || (strchr(e, '*') && simpler_glob(e, t))) ok = 1; } if (!ok) { fprintf(stderr, "%s: this program may not mail \"%s\"\n", simpler_at(line), t); exit(1); } if (strchr(s, 10) || strchr(s, 13)) { fprintf(stderr, "%s: the subject of a mail is one line\n", simpler_at(line)); exit(1); } const char* prog = getenv("SIMPLER_SENDMAIL"); if (!prog || !*prog) prog = "sendmail"; char* msg = (char*)malloc(tn + strlen(s) + strlen(b) + 32); sprintf(msg, "To: %s\nSubject: %s\n\n%s\n", t, s, b); char* av[] = {(char*)prog, (char*)"-i", (char*)"--", (char*)t, 0}; int code = 0; simpler_exec(av, 1, &code, (long)(intptr_t)msg); if (code != 0) { fprintf(stderr, "%s: could not send mail to \"%s\" (%s ended with status %d)\n", simpler_at(line), t, prog, code); exit(1); } return 0; }
long simpler_run(long name, long args, long input, long line) { int code = 0; long out = simpler_exec(simpler_argv_of(simpler_runok(name, line), args), 1, &code, input); if (code != 0) { fprintf(stderr, "%s: \"%s\" ended with status %d\n", simpler_at(line), (const char*)(intptr_t)name, code); exit(1); } return out; }
long simpler_try(long name, long args, long line) { int code = 0; simpler_quiet = 1; long out = simpler_exec(simpler_argv_of(simpler_runok(name, line), args), 1, &code, 0); simpler_quiet = 0; return code == 0 ? out : (long)(intptr_t)""; }
long simpler_status(long name, long args, long line) { int code = 0; simpler_exec(simpler_argv_of(simpler_runok(name, line), args), 0, &code, 0); return code; }
long simpler_fetchok(long url, long line) { const char* u = (const char*)(intptr_t)url; const char* h = 0; if (strncmp(u, "https://", 8) == 0) h = u + 8; else if (strncmp(u, "http://", 7) == 0) h = u + 7; if (h) { size_t n = strcspn(h, "/?#"); if (memchr(h, 64, n) == 0 && memchr(h, 92, n) == 0) { size_t hn = strcspn(h, ":/?#"); char hb[256]; if (hn < sizeof hb) { memcpy(hb, h, hn); hb[hn] = 0; for (int k = 0; simpler_ft[k]; k++) if (strcmp(simpler_ft[k], hb) == 0 || (strchr(simpler_ft[k], '*') && simpler_glob(simpler_ft[k], hb))) return url; } } } fprintf(stderr, "%s: this program may not fetch \"%s\"\n", simpler_at(line), u); exit(1); return 0; }
long i_char(long n) { char* o = (char*)malloc(5); int k = 0; if (n > 0 && n < 128) o[k++] = (char)n; else if (n >= 128 && n < 2048) { o[k++] = (char)(192 | (n >> 6)); o[k++] = (char)(128 | (n & 63)); } else if (n >= 2048 && n < 65536) { o[k++] = (char)(224 | (n >> 12)); o[k++] = (char)(128 | ((n >> 6) & 63)); o[k++] = (char)(128 | (n & 63)); } else if (n >= 65536 && n < 1114112) { o[k++] = (char)(240 | (n >> 18)); o[k++] = (char)(128 | ((n >> 12) & 63)); o[k++] = (char)(128 | ((n >> 6) & 63)); o[k++] = (char)(128 | (n & 63)); } o[k] = 0; return (long)(intptr_t)o; }
long simpler_millis() { struct timespec ts; clock_gettime(CLOCK_REALTIME, &ts); return (long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000; }
long simpler_start(long name, long args, long line) { char** av = simpler_argv_of(simpler_runok(name, line), args); int fd[2]; if (pipe(fd) != 0) return -1; fflush(stdout); pid_t p = fork(); if (p < 0) return -1; if (p == 0) { close(fd[0]); setsid(); pid_t q = fork(); if (q != 0) { if (write(fd[1], &q, sizeof q) < 0) _exit(1); _exit(0); } close(fd[1]); if (!freopen("/dev/null", "r", stdin)) _exit(127); execvp(av[0], av); _exit(127); } close(fd[1]); pid_t q = -1; if (read(fd[0], &q, sizeof q) != (long)sizeof q) q = -1; close(fd[0]); waitpid(p, 0, 0); return (long)q; }
static long simpler_hdr = 0; long simpler_curl(long url, long body, long post, long* st) { char* av[48]; int n = 0; av[n++] = "curl"; av[n++] = "-sS"; av[n++] = "--max-time"; av[n++] = "30"; av[n++] = "-w"; av[n++] = "\n%{http_code}"; if (simpler_hdr) { SList* h = (SList*)(intptr_t)simpler_hdr; for (long k = 0; k < h->len && k < 16; k++) { av[n++] = "-H"; av[n++] = (char*)(intptr_t)h->data[k]; } } if (post) { av[n++] = "--data-raw"; av[n++] = (char*)(intptr_t)body; } av[n++] = "--"; av[n++] = (char*)(intptr_t)url; av[n] = 0; int code = 0; char* out = (char*)(intptr_t)simpler_exec(av, 1, &code, 0); char* nl = strrchr(out, 10); *st = (nl && code == 0) ? atol(nl + 1) : 0; if (nl) *nl = 0; return (long)(intptr_t)out; }
long simpler_fetch(long url, long body, long post, long line) { simpler_fetchok(url, line); long st = 0; long out = simpler_curl(url, body, post, &st); if (st < 200 || st > 299) { fprintf(stderr, "%s: could not fetch \"%s\" (status %ld)\n", simpler_at(line), (const char*)(intptr_t)url, st); exit(1); } return out; }
long simpler_reach(long url, long line) { simpler_fetchok(url, line); long st = 0; fflush(stderr); int keep = dup(2); FILE* nul = fopen("/dev/null", "w"); if (nul) dup2(fileno(nul), 2); simpler_curl(url, 0, 0, &st); if (nul) { dup2(keep, 2); fclose(nul); } close(keep); return st; }
static struct termios simpler_tio; static int simpler_tty = 0; static int simpler_err = -1; static FILE* simpler_errf = 0;
typedef struct { char g[6]; unsigned short st; } SCell; static SCell* simpler_back = 0; static SCell* simpler_front = 0; static long simpler_tr = 0, simpler_tc = 0; static int simpler_all = 1; static volatile sig_atomic_t simpler_winched = 0; static char simpler_sty[1024][40]; static int simpler_nsty = 1; static char* simpler_ob = 0; static long simpler_on = 0, simpler_ocap = 0;
static void simpler_o(const char* s) { long n = (long)strlen(s); if (simpler_on + n + 1 > simpler_ocap) { simpler_ocap = (simpler_on + n + 1) * 2 + 4096; simpler_ob = (char*)realloc(simpler_ob, simpler_ocap); } memcpy(simpler_ob + simpler_on, s, n); simpler_on += n; }
static void simpler_term_blank(SCell* a) { long n = simpler_tr * simpler_tc; memset(a, 0, n * sizeof(SCell)); for (long k = 0; k < n; k++) a[k].g[0] = 32; }
static void simpler_term_fit(void) { struct winsize w; long r = 24, c = 80; if (ioctl(1, TIOCGWINSZ, &w) == 0 && w.ws_row && w.ws_col) { r = w.ws_row; c = w.ws_col; } if (simpler_back && r == simpler_tr && c == simpler_tc) return; free(simpler_back); free(simpler_front); simpler_tr = r; simpler_tc = c; simpler_back = (SCell*)malloc(r * c * sizeof(SCell)); simpler_front = (SCell*)malloc(r * c * sizeof(SCell)); simpler_term_blank(simpler_back); simpler_term_blank(simpler_front); simpler_all = 1; }
static void simpler_term_flush(void) { fflush(stdout); if (!simpler_back) return; char b[64]; simpler_on = 0; if (simpler_all) { simpler_o("\033[0m\033[2J"); simpler_term_blank(simpler_front); simpler_all = 0; } long cur = -1, pr = -1, pc = -1; for (long r = 0; r < simpler_tr; r++) for (long c = 0; c < simpler_tc; c++) { SCell* k = simpler_back + r * simpler_tc + c; SCell* f = simpler_front + r * simpler_tc + c; if (memcmp(k, f, sizeof(SCell)) == 0) continue; if (pr == r && c > pc && c - pc <= 4) { long g = pc; while (g < c && simpler_back[r * simpler_tc + g].st == cur) g++; if (g == c) { for (g = pc; g < c; g++) simpler_o(simpler_back[r * simpler_tc + g].g); pc = c; } } if (pr != r || pc != c) { snprintf(b, sizeof b, "\033[%ld;%ldH", r + 1, c + 1); simpler_o(b); } if (k->st != cur) { simpler_o("\033[0"); if (k->st) { simpler_o(";"); simpler_o(simpler_sty[k->st]); } simpler_o("m"); cur = k->st; } simpler_o(k->g); *f = *k; pr = r; pc = c + 1; } if (simpler_on == 0) return; simpler_o("\033[0m"); long off = 0; while (off < simpler_on) { ssize_t n = write(1, simpler_ob + off, simpler_on - off); if (n <= 0) { if (n < 0 && errno == EINTR) continue; break; } off += n; } }
static void simpler_term_end(void) { if (!simpler_tty) return; simpler_tty = 0; fputs("\033[0m\033[?25h\033[?1049l", stdout); fflush(stdout); tcsetattr(0, TCSAFLUSH, &simpler_tio); if (simpler_err >= 0) { fflush(stderr); dup2(simpler_err, 2); close(simpler_err); simpler_err = -1; if (simpler_errf) { char b[512]; size_t n; rewind(simpler_errf); while ((n = fread(b, 1, sizeof b, simpler_errf)) > 0) fwrite(b, 1, n, stderr); fclose(simpler_errf); simpler_errf = 0; } } }
static void simpler_winch(int g) { (void)g; simpler_winched = 1; }
static void simpler_quit(int g) { exit(128 + g); }
static void simpler_term_on(void) { if (simpler_tty) return; if (tcgetattr(0, &simpler_tio) != 0) { fprintf(stderr, "sys.term needs a terminal on standard input\n"); exit(1); } struct termios r = simpler_tio; cfmakeraw(&r); r.c_lflag |= ISIG; tcsetattr(0, TCSADRAIN, &r); static int once = 0; if (!once) { once = 1; atexit(simpler_term_end); struct sigaction sa; memset(&sa, 0, sizeof sa); sa.sa_handler = simpler_winch; sigaction(SIGWINCH, &sa, 0); sa.sa_handler = simpler_quit; sigaction(SIGINT, &sa, 0); sigaction(SIGTERM, &sa, 0); } simpler_errf = tmpfile(); if (simpler_errf) { fflush(stderr); simpler_err = dup(2); dup2(fileno(simpler_errf), 2); } simpler_tty = 1; fputs("\033[?1049h\033[?25l", stdout); fflush(stdout); simpler_term_fit(); simpler_all = 1; }
long simpler_term_stop(void) { simpler_term_end(); return 0; }
long simpler_term_size(long cols) { if (!simpler_back) simpler_term_fit(); return cols ? simpler_tc : simpler_tr; }
long simpler_term_clear(void) { simpler_term_on(); simpler_term_blank(simpler_back); return 0; }
static const char* simpler_style(const char* w, long n) { static const char* k[] = {"bold","1","dim","2","underline","4","reverse","7","black","30","red","31","green","32","yellow","33","blue","34","magenta","35","cyan","36","white","37","gray","90",0}; for (int i = 0; k[i]; i += 2) if ((long)strlen(k[i]) == n && strncmp(k[i], w, n) == 0) return k[i + 1]; int bg = n > 2 && w[0] == 'o' && w[1] == 'n'; long v = 0, d = 0; for (long i = bg ? 2 : 0; i < n; i++) { if (w[i] < '0' || w[i] > '9' || ++d > 3) return 0; v = v * 10 + (w[i] - '0'); } if (d == 0 || v > 255) return 0; static char b[32]; snprintf(b, sizeof b, "%d;5;%ld", bg ? 48 : 38, v); return b; }
long simpler_term_put(long row, long col, long text, long style, long line) { simpler_term_on(); const char* t = (const char*)(intptr_t)text; const char* y = (const char*)(intptr_t)style; long room = simpler_tc - col; if (row < 0 || col < 0 || row >= simpler_tr || room <= 0) return 0; char sg[40]; long sn = 0; while (*y) { while (*y == 32) y++; long n = 0; while (y[n] && y[n] != 32) n++; if (n == 0) break; const char* c = simpler_style(y, n); if (!c) { fprintf(stderr, "%s: no style named \"%.*s\"; the styles are bold, dim, underline, reverse, black, red, green, yellow, blue, magenta, cyan, white, gray, a colour number 0 to 255, and on with a number for the background (on52)\n", simpler_at(line), (int)n, y); exit(1); } long cn = (long)strlen(c); if (sn + cn + 2 < (long)sizeof sg) { if (sn) sg[sn++] = 59; memcpy(sg + sn, c, cn); sn += cn; } y += n; } sg[sn] = 0; static int last = 0; int st = 0; if (sn) { if (strcmp(simpler_sty[last], sg) == 0) st = last; else { for (int k = 1; k < simpler_nsty && !st; k++) if (strcmp(simpler_sty[k], sg) == 0) st = k; if (!st && simpler_nsty < 1024) { st = simpler_nsty++; strcpy(simpler_sty[st], sg); } last = st; } } SCell* k = simpler_back + row * simpler_tc + col - 1; long gn = 5; for (; *t; t++) { unsigned char ch = (unsigned char)*t; if ((ch & 0xC0) != 0x80) { if (room == 0) break; room--; k++; memset(k, 0, sizeof(SCell)); k->st = (unsigned short)st; gn = 0; } if (gn < 4) k->g[gn++] = (char)(ch < 32 || ch == 127 ? 63 : ch); } return 0; }
static long simpler_keyname(const char* k) { return (long)(intptr_t)k; }
long simpler_term_key(void) { simpler_term_on(); if (simpler_winched) { simpler_winched = 0; simpler_term_fit(); return simpler_keyname("resize"); } simpler_term_flush(); unsigned char b[16]; ssize_t n = read(0, b, 1); if (n < 0) { if (errno != EINTR) return simpler_keyname("eof"); simpler_winched = 0; simpler_term_fit(); return simpler_keyname("resize"); } if (n == 0) return simpler_keyname("eof"); unsigned char c = b[0]; if (c == 27) { struct pollfd pf = {0, POLLIN, 0}; long m = 0; while (m < 8 && poll(&pf, 1, m == 0 ? 30 : 0) > 0 && read(0, b + m, 1) == 1) { m++; if (m > 1 && b[m - 1] >= 64) break; } if (m == 0) return simpler_keyname("esc"); unsigned char e = b[m - 1]; if (m >= 2 && (b[0] == '[' || b[0] == 'O')) { if (e == 'A') return simpler_keyname("up"); if (e == 'B') return simpler_keyname("down"); if (e == 'C') return simpler_keyname("right"); if (e == 'D') return simpler_keyname("left"); if (e == 'H') return simpler_keyname("home"); if (e == 'F') return simpler_keyname("end"); if (e == '~') { if (b[1] == '1' || b[1] == '7') return simpler_keyname("home"); if (b[1] == '4' || b[1] == '8') return simpler_keyname("end"); if (b[1] == '3') return simpler_keyname("delete"); if (b[1] == '5') return simpler_keyname("pgup"); if (b[1] == '6') return simpler_keyname("pgdn"); } } return simpler_keyname("other"); } if (c == 13 || c == 10) return simpler_keyname("enter"); if (c == 9) return simpler_keyname("tab"); if (c == 127 || c == 8) return simpler_keyname("backspace"); char* r = (char*)malloc(8); if (c < 32) { sprintf(r, "ctrl-%c", c + 96); return (long)(intptr_t)r; } long len = c >= 0xF0 ? 4 : c >= 0xE0 ? 3 : c >= 0xC0 ? 2 : 1; r[0] = (char)c; for (long i = 1; i < len; i++) if (read(0, r + i, 1) != 1) { len = i; break; } r[len] = 0; return (long)(intptr_t)r; }
long simpler_now() { return (long)time(0); }
long simpler_fetchwith(long url, long hdrs, long line) { SList* h = (SList*)(intptr_t)hdrs; for (long k = 0; k < h->len; k++) { const char* x = (const char*)(intptr_t)h->data[k]; if (k >= 16 || x[0] == '@' || !strchr(x, ':') || strchr(x, 10) || strchr(x, 13)) { fprintf(stderr, "%s: a header is one line like \"Name: value\", and there are at most 16; got \"%s\"\n", simpler_at(line), x); exit(1); } } simpler_hdr = hdrs; long out = simpler_fetch(url, 0, 0, line); simpler_hdr = 0; return out; }
long simpler_stamp(long t, long pat) { time_t tt = (time_t)t; struct tm* m = localtime(&tt); char* r = (char*)malloc(256); if (strftime(r, 256, (const char*)(intptr_t)pat, m) == 0) r[0] = 0; return (long)(intptr_t)r; }
long simpler_secs(long text) { const char* s = (const char*)(intptr_t)text; struct tm m; memset(&m, 0, sizeof m); int y, mo, d, h = 0, mi = 0, se = 0, n = 0; if (sscanf(s, "%d-%d-%d%n", &y, &mo, &d, &n) != 3 || mo < 1 || mo > 12 || d < 1 || d > 31) return 0; s += n; if (*s == 'T' || *s == ' ') { n = 0; if (sscanf(s + 1, "%d:%d%n", &h, &mi, &n) != 2) return 0; s += 1 + n; n = 0; if (*s == ':' && sscanf(s + 1, "%d%n", &se, &n) == 1) s += 1 + n; if (*s == '.') { s++; while (*s >= '0' && *s <= '9') s++; } } m.tm_year = y - 1900; m.tm_mon = mo - 1; m.tm_mday = d; m.tm_hour = h; m.tm_min = mi; m.tm_sec = se; m.tm_isdst = -1; if (*s == 'Z') return (long)timegm(&m); if (*s == '+' || *s == '-') { int oh = 0, om = 0; if (sscanf(s + 1, "%d:%d", &oh, &om) < 1) return 0; if (oh > 99) { om = oh % 100; oh = oh / 100; } long off = oh * 3600L + om * 60L; return (long)timegm(&m) - (*s == '-' ? -off : off); } return (long)mktime(&m); }
long simpler_date(long t) { time_t tt = (time_t)t; struct tm* m = localtime(&tt); char* r = (char*)malloc(32); strftime(r, 32, "%Y-%m-%d %H:%M:%S", m); return (long)(intptr_t)r; }
long simpler_setenv(long name, long value) { const char* n = (const char*)(intptr_t)name; if (!n[0] || strchr(n, 61)) return 0; setenv(n, (const char*)(intptr_t)value, 1); return 1; }
static char* simpler_selfp = 0; void simpler_self_init() { const char* e = getenv("SIMPLER_SELF"); if (e) { simpler_selfp = strdup(e); unsetenv("SIMPLER_SELF"); } } long simpler_self() { if (!simpler_selfp) { char b[4096]; ssize_t n = readlink("/proc/self/exe", b, sizeof b - 1); if (n > 0) { b[n] = 0; simpler_selfp = strdup(b); } else { char* r = realpath(simpler_argv[0], 0); simpler_selfp = r ? r : simpler_argv[0]; } } return (long)(intptr_t)simpler_selfp; }
long simpler_env(long name) { const char* v = getenv((const char*)(intptr_t)name); return (long)(intptr_t)(v ? v : ""); }
long simpler_stdin() { long cap = 1024; long len = 0; char* buf = (char*)malloc(cap); int c; while ((c = getchar()) != EOF) { if (len + 1 >= cap) { cap = cap * 2; buf = (char*)realloc(buf, cap); } buf[len] = (char)c; len = len + 1; } buf[len] = 0; return (long)(intptr_t)buf; }
long fail(long msg) { fprintf(stderr, "%s\n", (const char*)(intptr_t)msg); exit(1); return 0; }
long simpler_exit(long c) { exit((int)c); return 0; }
long simpler_failed = 0;
long s_assert(long c) { if (!c) simpler_failed = 1; return 0; }
enum { T_Num, T_FloatLit, T_Var, T_StrLit, T_ListLit, T_Bin, T_Call, T_Match, T_Field, T_Method, T_Each };
long Num(long v0) { return mk(T_Num, v0, 0, 0); }
long FloatLit(long v0) { return mk(T_FloatLit, v0, 0, 0); }
long Var(long v0) { return mk(T_Var, v0, 0, 0); }
long StrLit(long v0) { return mk(T_StrLit, v0, 0, 0); }
long ListLit(long v0) { return mk(T_ListLit, v0, 0, 0); }
long Bin(long v0, long v1, long v2) { return mk(T_Bin, v0, v1, v2); }
long Call(long v0, long v1) { return mk(T_Call, v0, v1, 0); }
long Match(long v0, long v1) { return mk(T_Match, v0, v1, 0); }
long Field(long v0, long v1) { return mk(T_Field, v0, v1, 0); }
long Method(long v0, long v1, long v2) { return mk(T_Method, v0, v1, v2); }
long Each(long v0, long v1, long v2) { return mk(T_Each, v0, v1, v2); }
enum { T_Let, T_Bare, T_If, T_While, T_Ret, T_Line, T_Jump };
long Let(long v0, long v1, long v2) { return mk(T_Let, v0, v1, v2); }
long Bare(long v0) { return mk(T_Bare, v0, 0, 0); }
long If(long v0, long v1, long v2) { return mk(T_If, v0, v1, v2); }
long While(long v0, long v1) { return mk(T_While, v0, v1, 0); }
long Ret(long v0) { return mk(T_Ret, v0, 0, 0); }
long Line(long v0) { return mk(T_Line, v0, 0, 0); }
long Jump(long v0) { return mk(T_Jump, v0, 0, 0); }
enum { T_Ident, T_Str, T_Int, T_Float, T_Punct, T_Eof };
long Ident(long v0) { return mk(T_Ident, v0, 0, 0); }
long Str(long v0) { return mk(T_Str, v0, 0, 0); }
long Int(long v0) { return mk(T_Int, v0, 0, 0); }
long Float(long v0) { return mk(T_Float, v0, 0, 0); }
long Punct(long v0) { return mk(T_Punct, v0, 0, 0); }
long Eof() { return mk(T_Eof, 0, 0, 0); }
typedef struct { long tag; long binds; long body; } R_Arm;
long Arm(long tag, long binds, long body) { R_Arm* _o = malloc(sizeof(R_Arm)); _o->tag = tag; _o->binds = binds; _o->body = body; return (long)(intptr_t)_o; }
typedef struct { long name; long params; long ptypes; long ret; long effs; long body; long line; } R_Fn;
long Fn(long name, long params, long ptypes, long ret, long effs, long body, long line) { R_Fn* _o = malloc(sizeof(R_Fn)); _o->name = name; _o->params = params; _o->ptypes = ptypes; _o->ret = ret; _o->effs = effs; _o->body = body; _o->line = line; return (long)(intptr_t)_o; }
typedef struct { long toks; long lines; long files; } R_Lexed;
long Lexed(long toks, long lines, long files) { R_Lexed* _o = malloc(sizeof(R_Lexed)); _o->toks = toks; _o->lines = lines; _o->files = files; return (long)(intptr_t)_o; }
typedef struct { long name; long lx; long names; } R_Mod;
long Mod(long name, long lx, long names) { R_Mod* _o = malloc(sizeof(R_Mod)); _o->name = name; _o->lx = lx; _o->names = names; return (long)(intptr_t)_o; }
typedef struct { long cname; long arity; long ptypes; } R_Case;
long Case(long cname, long arity, long ptypes) { R_Case* _o = malloc(sizeof(R_Case)); _o->cname = cname; _o->arity = arity; _o->ptypes = ptypes; return (long)(intptr_t)_o; }
typedef struct { long name; long cases; long line; } R_TyDef;
long TyDef(long name, long cases, long line) { R_TyDef* _o = malloc(sizeof(R_TyDef)); _o->name = name; _o->cases = cases; _o->line = line; return (long)(intptr_t)_o; }
typedef struct { long name; long fields; long ftypes; long line; } R_RecDef;
long RecDef(long name, long fields, long ftypes, long line) { R_RecDef* _o = malloc(sizeof(R_RecDef)); _o->name = name; _o->fields = fields; _o->ftypes = ftypes; _o->line = line; return (long)(intptr_t)_o; }
typedef struct { long types; long records; long fns; long reads; long writes; long runs; long fetches; long mails; long files; } R_Prog;
long Prog(long types, long records, long fns, long reads, long writes, long runs, long fetches, long mails, long files) { R_Prog* _o = malloc(sizeof(R_Prog)); _o->types = types; _o->records = records; _o->fns = fns; _o->reads = reads; _o->writes = writes; _o->runs = runs; _o->fetches = fetches; _o->mails = mails; _o->files = files; return (long)(intptr_t)_o; }
typedef struct { long names; long tys; } R_TyEnv;
long TyEnv(long names, long tys) { R_TyEnv* _o = malloc(sizeof(R_TyEnv)); _o->names = names; _o->tys = tys; return (long)(intptr_t)_o; }
typedef struct { long recNames; long records; long types; long fns; long fnNames; long fnRets; long boxedNullary; long retP; long mutP; long reads; long writes; long runs; long fetches; long mails; long files; } R_Sigs;
long Sigs(long recNames, long records, long types, long fns, long fnNames, long fnRets, long boxedNullary, long retP, long mutP, long reads, long writes, long runs, long fetches, long mails, long files) { R_Sigs* _o = malloc(sizeof(R_Sigs)); _o->recNames = recNames; _o->records = records; _o->types = types; _o->fns = fns; _o->fnNames = fnNames; _o->fnRets = fnRets; _o->boxedNullary = boxedNullary; _o->retP = retP; _o->mutP = mutP; _o->reads = reads; _o->writes = writes; _o->runs = runs; _o->fetches = fetches; _o->mails = mails; _o->files = files; return (long)(intptr_t)_o; }
typedef struct { long env; long sigs; } R_Ctx;
long Ctx(long env, long sigs) { R_Ctx* _o = malloc(sizeof(R_Ctx)); _o->env = env; _o->sigs = sigs; return (long)(intptr_t)_o; }
typedef struct { long node; long next; } R_Parsed;
long Parsed(long node, long next) { R_Parsed* _o = malloc(sizeof(R_Parsed)); _o->node = node; _o->next = next; return (long)(intptr_t)_o; }
typedef struct { long node; long next; } R_PStmt;
long PStmt(long node, long next) { R_PStmt* _o = malloc(sizeof(R_PStmt)); _o->node = node; _o->next = next; return (long)(intptr_t)_o; }
typedef struct { long arm; long next; } R_PArm;
long PArm(long arm, long next) { R_PArm* _o = malloc(sizeof(R_PArm)); _o->arm = arm; _o->next = next; return (long)(intptr_t)_o; }
typedef struct { long fn; long next; } R_PFn;
long PFn(long fn, long next) { R_PFn* _o = malloc(sizeof(R_PFn)); _o->fn = fn; _o->next = next; return (long)(intptr_t)_o; }
typedef struct { long ty; long next; } R_PTy;
long PTy(long ty, long next) { R_PTy* _o = malloc(sizeof(R_PTy)); _o->ty = ty; _o->next = next; return (long)(intptr_t)_o; }
typedef struct { long rec; long next; } R_PRec;
long PRec(long rec, long next) { R_PRec* _o = malloc(sizeof(R_PRec)); _o->rec = rec; _o->next = next; return (long)(intptr_t)_o; }
typedef struct { long names; long types; long next; } R_PNames;
long PNames(long names, long types, long next) { R_PNames* _o = malloc(sizeof(R_PNames)); _o->names = names; _o->types = types; _o->next = next; return (long)(intptr_t)_o; }
typedef struct { long ret; long effs; long next; } R_PRet;
long PRet(long ret, long effs, long next) { R_PRet* _o = malloc(sizeof(R_PRet)); _o->ret = ret; _o->effs = effs; _o->next = next; return (long)(intptr_t)_o; }
typedef struct { long body; long next; } R_PBody;
long PBody(long body, long next) { R_PBody* _o = malloc(sizeof(R_PBody)); _o->body = body; _o->next = next; return (long)(intptr_t)_o; }
typedef struct { long ty; long next; } R_PType;
long PType(long ty, long next) { R_PType* _o = malloc(sizeof(R_PType)); _o->ty = ty; _o->next = next; return (long)(intptr_t)_o; }
typedef struct { long pre; long ops; } R_Hoist;
long Hoist(long pre, long ops) { R_Hoist* _o = malloc(sizeof(R_Hoist)); _o->pre = pre; _o->ops = ops; return (long)(intptr_t)_o; }
typedef struct { long list; long next; } R_PArgs;
long PArgs(long list, long next) { R_PArgs* _o = malloc(sizeof(R_PArgs)); _o->list = list; _o->next = next; return (long)(intptr_t)_o; }
typedef struct { long kinds; long acts; long fns; } R_What;
long What(long kinds, long acts, long fns) { R_What* _o = malloc(sizeof(R_What)); _o->kinds = kinds; _o->acts = acts; _o->fns = fns; return (long)(intptr_t)_o; }
long u_isMayLine(long toks, long i);
long u_isStrTok(long toks, long i);
long u_strAt(long toks, long i);
long u_parseMay(long toks, long i, long reads, long writes, long runs, long fetches, long mails);
long u_mayTable(long cname, long names);
long u_globOk(long pat, long s);
long u_globName(long names, long s);
long u_mayName(long s, long names);
long u_mayCheck(long arg, long names, long verb, long word, long ctx);
long u_hostOf(long url);
long u_runCheck(long arg, long ctx);
long u_mailCheck(long arg, long ctx);
long u_fetchCheck(long arg, long ctx);
long u_mayText(long names);
long u_whatProg(long prog, long sigs);
long u_whatKind(long w, long kind);
long u_whatAdd(long w, long kind, long act, long fn);
long u_whatBody(long body, long fn, long w, long sigs);
long u_whatStmt(long s, long fn, long w, long sigs);
long u_whatExpr(long e, long fn, long w, long sigs);
long u_whatAll(long es, long fn, long w, long sigs);
long u_whatArms(long arms, long fn, long w, long sigs);
long u_whatField(long recv, long fld, long fn, long w, long sigs);
long u_whatMethod(long recv, long name, long args, long fn, long w, long sigs);
long u_whatPath(long e);
long u_isTest(long name);
long u_emitTests(long fns);
long u_buildSigs(long prog);
long u_spot(long files, long line);
long u_hasMod(long mods, long name);
long u_modNameOk(long name);
long u_takeUses(long lx, long mine, long queue, long queueLines);
long u_qualify(long lx, long mine);
long u_makeMod(long name, long src, long files, long queue, long queueLines);
long u_joinMods(long mods, long mainLx);
long u_parse(long toks);
long u_tokAt(long toks, long i);
long u_lineOf(long toks, long i);
long u_show(long toks, long i);
long u_showPunct(long p);
long u_failLine(long toks, long i, long msg);
long u_failTok(long toks, long i, long msg);
long u_once(long seen, long name, long toks, long i);
long u_expect(long toks, long i, long p);
long u_isKeyword(long toks, long i);
long u_expectIdent(long toks, long i, long what);
long u_sep(long toks, long j, long close);
long u_parseType(long toks, long i);
long u_isTypeDef(long toks, long i);
long u_isRecordDef(long toks, long i);
long u_parseRecord(long toks, long i);
long u_parseTypeDef(long toks, long i, long seen);
long u_parseFn(long toks, long i);
long u_parseParams(long toks, long i);
long u_parseRet(long toks, long j);
long u_parseBlock(long toks, long i);
long u_parseStmts(long toks, long start);
long u_checkUseful(long s, long toks, long i);
long u_checkValueLine(long e, long toks, long i);
long u_plainValue(long e);
long u_parseStmt(long toks, long i);
long u_fieldName(long e);
long u_fieldRecv(long e);
long u_isSetField(long name);
long u_parseFor(long toks, long i);
long u_parseReturn(long toks, long i);
long u_isTypedAssign(long toks, long i);
long u_parseTypedLet(long toks, long i);
long u_parseIf(long toks, long i);
long u_parseWhile(long toks, long i);
long u_parseExpr(long toks, long pos);
long u_parseAnd(long toks, long pos);
long u_parseCmp(long toks, long pos);
long u_parseAdd(long toks, long pos);
long u_parseTerm(long toks, long pos);
long u_parsePostfix(long toks, long pos);
long u_parseEach(long toks, long recv, long i);
long u_parseFactor(long toks, long pos);
long u_parsePunctFactor(long toks, long pos);
long u_parseList(long toks, long pos);
long u_parseIdentFactor(long toks, long pos);
long u_parseParen(long toks, long pos);
long u_parseArgs(long toks, long i);
long u_parseMatch(long toks, long scrut, long i);
long u_parseArm(long toks, long j);
long u_emitType(long t);
long u_emitEnumType(long t);
long u_emitBoxedType(long t);
long u_hasPayload(long t);
long u_emitRecord(long rec);
long u_recParams(long rec);
long u_slotName(long bi);
long u_ctorParams(long arity);
long u_ctorArgs(long arity);
long u_mangle(long name);
long u_emitProto(long f);
long u_emitFn(long f, long sigs);
long u_fnCtx(long f, long sigs);
long u_paramDecls(long f);
long u_newEnv();
long u_envPut(long env, long name, long ty);
long u_envOld(long env, long name);
long u_envOlds(long env, long names);
long u_envRestore(long env, long names, long olds);
long u_envGet(long env, long name);
long u_seedEnv(long body, long ctx);
long u_seedStmt(long s, long ctx);
long u_seedLet(long name, long ty0, long ctx);
long u_seedIf(long t, long el, long ctx);
long u_seedExpr(long e, long ctx);
long u_seedEach(long recv, long param, long body, long ctx);
long u_inferType(long e, long ctx);
long u_letType(long ty, long e, long ctx);
long u_callRet(long name, long ctx);
long u_variantOf(long name, long ctx);
long u_hasCase(long t, long name);
long u_isRec(long ty, long ctx);
long u_collectLets(long body);
long u_collectBody(long body, long names);
long u_collectStmt(long s, long names);
long u_collectExpr(long e, long names);
long u_collectIf(long t, long el, long names);
long u_pushUnique(long names, long name);
long u_hasName(long names, long name);
long u_emitStmt(long s, long asReturn, long ctx);
long u_emitRet(long e, long ctx);
long u_emitIf(long c, long t, long el, long asReturn, long ctx);
long u_emitWhile(long c, long b, long ctx);
long u_emitBlock(long body, long asReturn, long ctx);
long u_emitBare(long e, long asReturn, long ctx);
long u_stmtOnly(long e);
long u_stmtExpr(long e, long ctx);
long u_exprStmt(long e, long ctx);
long u_emitCallStmt(long name, long args, long e, long ctx);
long u_emitMethodStmt(long name, long args, long e, long ctx);
long u_emitPrint(long arg, long ctx);
long u_emitEach(long recv, long param, long body, long ctx);
long u_isListType(long t);
long u_loopItem(long t);
long u_elemOf(long t);
long u_isMapType(long t);
long u_mapValueOf(long t);
long u_emitReturn(long e, long ctx);
long u_retVal(long e, long ctx);
long u_emitSwitch(long scrut, long arms, long ctx);
long u_emitMatchExpr(long scrut, long arms, long ctx);
long u_emitArm(long a, long scrutC, long boxed, long styp, long pre, long post, long ctx);
long u_payloadType(long tyName, long caseTag, long idx, long ctx);
long u_casePayloadType(long t, long caseTag, long idx);
long u_boxField(long scrutC, long field);
long u_checkExhaustive(long styp, long arms, long ctx);
long u_checkArities(long t, long arms, long ctx);
long u_setLine(long n, long ctx);
long u_emitLine(long n, long ctx);
long u_failAt(long msg, long ctx);
long u_plainType(long t);
long u_isHanded(long s, long ctx);
long u_holdOf(long s, long ctx);
long u_setHolding(long s, long d, long ctx);
long u_oneLess(long d);
long u_holdDepth(long e, long ctx);
long u_minLevel(long es, long ctx);
long u_pathLen(long e);
long u_setHanded(long s, long ctx);
long u_fixedType(long t, long ctx, long depth);
long u_handed(long e, long ctx);
long u_handedField(long recv, long fld, long ctx);
long u_callHanded(long name, long args, long ctx);
long u_markFn(long f, long ctx);
long u_markAgain(long f, long ctx);
long u_paramFacts(long f, long sigs);
long u_typeBase(long t);
long u_typeKnown(long t, long sigs);
long u_noType(long t);
long u_checkTypes(long prog, long sigs);
long u_jumpBody(long fl, long body, long inLoop);
long u_jumpStmt(long fl, long s, long inLoop, long line);
long u_jumpAt(long fl, long w, long inLoop, long line);
long u_jumpExpr(long fl, long e, long inLoop, long line);
long u_retPass(long prog, long sigs);
long u_lastHanded(long body, long ctx);
long u_lastStmt(long s, long prev, long ctx);
long u_anyRets(long body, long ctx);
long u_stmtRets(long s, long ctx);
long u_eachRets(long e, long ctx);
long u_anyHanded(long es, long ctx);
long u_armsHanded(long scrut, long arms, long ctx);
long u_bindArm(long styp, long a, long ctx);
long u_rootVar(long e);
long u_markBody(long body, long ctx);
long u_markStmt(long s, long ctx);
long u_markLet(long name, long e, long ctx);
long u_markExpr(long e, long ctx);
long u_markAll(long es, long ctx);
long u_markEach(long recv, long param, long body, long ctx);
long u_markMatch(long scrut, long arms, long ctx);
long u_markMethod(long recv, long name, long args, long ctx);
long u_checkEffects(long f, long ctx);
long u_effBody(long body, long used, long ctx);
long u_effStmt(long s, long used, long ctx);
long u_effIf(long c, long t, long el, long used, long ctx);
long u_effWhile(long c, long b, long used, long ctx);
long u_effExpr(long e, long used, long ctx);
long u_effMethodE(long recv, long name, long args, long used, long ctx);
long u_effFieldE(long recv, long fld, long used, long ctx);
long u_effCallE(long name, long args, long used, long ctx);
long u_effBin(long a, long b, long used, long ctx);
long u_effMatchE(long scrut, long arms, long used, long ctx);
long u_effEachE(long recv, long param, long body, long used, long ctx);
long u_effArgs(long args, long used, long ctx);
long u_addAll(long src, long dst);
long u_caseArityIn(long t, long tag);
long u_checkCases(long t, long arms, long ctx);
long u_armCovers(long arms, long cname);
long u_armHasBind(long arms);
long u_emitExpr(long e, long ctx);
long u_calm(long e);
long u_hoist(long ops, long ctx);
long u_inOrder(long pre, long c);
long u_emitBin(long op, long a, long b, long ctx);
long u_emitCall(long name, long args, long ctx);
long u_emitMethod(long recv, long name, long args, long ctx);
long u_emitList(long es, long ctx);
long u_emitList0(long es, long ctx);
long u_listType(long es, long ctx);
long u_emitBin0(long op, long a, long b, long ctx);
long u_textOp(long op, long a, long b, long ctx);
long u_isFloatArith(long op);
long u_checkBinOp(long op, long a, long b, long ctx);
long u_mixedFloat(long a, long b);
long u_isArithOp(long op);
long u_knownCall(long name, long ctx);
long u_emitCall0(long name, long args, long ctx);
long u_checkReturn(long f, long ctx);
long u_checkReturnStmt(long s, long ret, long ctx);
long u_checkRetExpr(long e, long ret, long ctx);
long u_isMatchExpr(long e);
long u_checkCall(long name, long args, long ctx);
long u_checkArgTypes(long name, long ptypes, long args, long ctx);
long u_compatible(long want, long got, long ctx);
long u_intLike(long t);
long u_notInt(long t, long ctx);
long u_varType(long s, long ctx);
long u_isVariant(long t, long ctx);
long u_emitTermField(long fld, long ctx);
long u_emitField(long recv, long fld, long ctx);
long u_isCapField(long fld);
long u_isBuiltinField(long fld);
long u_isMathField(long fld);
long u_recHasField(long t, long fld, long ctx);
long u_useUFCS(long t, long fld, long ctx);
long u_capNeed(long name);
long u_checkCap(long recv, long name, long ctx);
long u_wantType(long e, long want, long name, long ctx);
long u_capShow(long t);
long u_checkField(long t, long fld, long ctx);
long u_isNumType(long t);
long u_isTextField(long fld);
long u_isTextMethod(long name);
long u_checkMethod(long t, long name, long args, long ctx);
long u_isSysField(long fld);
long u_checkSysField(long t, long fld, long ctx);
long u_methodArgs(long name);
long u_emitSetField(long recv, long fld, long args, long ctx);
long u_emitMethod0(long recv, long name, long args, long ctx);
long u_cEscape(long s);
long u_caseArity(long name, long ctx);
long u_emitVar(long s, long ctx);
long u_emitArgs(long args, long ctx);
long u_binType(long op, long a, long b, long ctx);
long u_exprType(long e, long ctx);
long u_matchType(long scrut, long arms, long ctx);
long u_fieldType(long recv, long fld, long ctx);
long u_recFieldType(long tyName, long fld, long ctx);
long u_fieldTypeIn(long rec, long fld);
long u_methodRet(long recv, long name, long ctx);
long u_isMain(long name);
long u_notEof(long toks, long i);
long u_inBlock(long toks, long j);
long u_inArgs(long toks, long j);
long u_isIdent(long toks, long i);
long u_isWord(long toks, long i, long w);
long u_isAssign(long toks, long i);
long u_identAt(long toks, long i);
long u_punctAt(long toks, long i);
long u_isPunct(long toks, long i, long op);
long u_isCmpOp(long toks, long i);
long u_isAddOp(long toks, long i);
long u_isMulOp(long toks, long i);
long u_lex(long src, long files);
long u_esc(long src, long j);
long u_isComment(long src, long i, long n);
long u_isTwoCharOp(long src, long i, long n);
long u_twoIs(long src, long i, long n, long a, long b);
long u_isDigit(long c);
long u_cTaken();
long u_safeName(long s, long taken);
long u_isAlpha(long c);
long u_isAlnum(long c);
long u_isSpace(long c);
int main(int argc, char** argv) {
  simpler_argc = argc; simpler_argv = argv; simpler_self_init();
  long src = 0;
  long files = 0;
  long queue = 0;
  long queueLines = 0;
  long mainUses = 0;
  long mainLx = 0;
  long mods = 0;
  long q = 0;
  long name = 0;
  long file = 0;
  long msrc = 0;
  long lexed = 0;
  long prog = 0;
  long sigs = 0;
  long fl = 0;
  long testing = 0;

  src = (long)(intptr_t)simpler_read((const char*)(intptr_t)simpler_may((long)(intptr_t)"input.smplr", simpler_rd, 219, "read"));

  files = l_with(l_new(), (long)(intptr_t)"input.smplr");

  queue = l_new();

  queueLines = l_new();

  mainUses = l_new();

  mainLx = u_qualify(u_takeUses(u_lex(src, files), mainUses, queue, queueLines), mainUses);

  mods = l_new();

  q = 0L;

  while ((q < l_len(queue))) {

  name = l_atc(queue, q, 228);

  if ((!u_hasMod(mods, name))) {

  file = s_concat(name, (long)(intptr_t)".smplr");

  if ((!simpler_exists(simpler_may(file, simpler_rd, 231, "read")))) {

  fail(s_concat(s_concat(s_concat(s_concat(s_concat(u_spot(files, l_atc(queueLines, q, 232)), (long)(intptr_t)"no module named "), name), (long)(intptr_t)": the file "), file), (long)(intptr_t)" was not found"));
  }

  msrc = (long)(intptr_t)simpler_read((const char*)(intptr_t)simpler_may(file, simpler_rd, 234, "read"));

  l_push(files, file);

  l_push(mods, u_makeMod(name, msrc, files, queue, queueLines));
  }

  q = (q + 1L);
  }

  lexed = mainLx;

  if ((l_len(mods) > 0L)) {

  lexed = u_joinMods(mods, mainLx);
  }

  prog = u_parse(lexed);

  sigs = u_buildSigs(prog);

  if (((l_len(simpler_args(argc, argv)) > 0L) && s_eq(l_atc(simpler_args(argc, argv), 0L, 247), (long)(intptr_t)"what"))) {

  if ((!u_hasName(((R_Sigs*)(intptr_t)sigs)->fnNames, (long)(intptr_t)"main"))) {

  fail((long)(intptr_t)"input.smplr:1: no main function found");
  }

  u_retPass(prog, sigs);

  {
  long _lst = ((R_Prog*)(intptr_t)prog)->fns;
  long _n = l_len(_lst);
  for (long _i = 0; _i < _n; _i = _i + 1) {
  long f = l_at(_lst, _i);

  u_emitFn(f, sigs);
  }
  }

  (printf("%s\n", (const char*)(intptr_t)u_whatProg(prog, sigs)), 0L);

  return 0L;
  }

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"#include <stdio.h>");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"#include <stdlib.h>");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"#include <stdint.h>");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"#include <math.h>");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"#include <string.h>");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"#include <dirent.h>");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"#include <sys/stat.h>");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"#include <errno.h>");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"#include <unistd.h>");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"#include <sys/wait.h>");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"#include <time.h>");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"#include <limits.h>");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"#include <termios.h>");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"#include <sys/ioctl.h>");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"#include <signal.h>");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"#include <poll.h>");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"#include <sys/resource.h>");

  fl = (long)(intptr_t)"static const char* simpler_fl[] = {";

  q = 0L;

  while ((q < l_len(files))) {

  fl = s_concat(({ long _t412 = s_concat(fl, (long)(intptr_t)"\""); long _t413 = l_atc(files, q, 279); s_concat(_t412, _t413); }), (long)(intptr_t)"\", ");

  q = (q + 1L);
  }

  (printf("%s\n", (const char*)(intptr_t)s_concat(fl, (long)(intptr_t)"0};")), 0L);

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"static const char* simpler_at(long line) { static char b[300]; snprintf(b, sizeof b, \"%s:%ld\", simpler_fl[line / 1000000], line % 1000000); return b; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"typedef struct { int tag; long v0; long v1; long v2; long v3; long v4; long v5; long v6; long v7; } Obj;");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long mk(int tag, long v0, long v1, long v2) { Obj* o = malloc(4 * sizeof(long)); o->tag = tag; o->v0 = v0; o->v1 = v1; o->v2 = v2; return (long)(intptr_t)o; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long mk8(int tag, long v0, long v1, long v2, long v3, long v4, long v5, long v6, long v7) { Obj* o = malloc(sizeof(Obj)); o->tag = tag; o->v0 = v0; o->v1 = v1; o->v2 = v2; o->v3 = v3; o->v4 = v4; o->v5 = v5; o->v6 = v6; o->v7 = v7; return (long)(intptr_t)o; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"typedef struct { long* data; long len; long cap; } SList;");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long l_new() { SList* l = (SList*)malloc(sizeof(SList)); l->cap = 8; l->len = 0; l->data = (long*)malloc(sizeof(long) * l->cap); return (long)(intptr_t)l; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long l_push(long lp, long x) { SList* l = (SList*)(intptr_t)lp; if (l->len == l->cap) { l->cap = l->cap * 2; l->data = (long*)realloc(l->data, sizeof(long) * l->cap); } l->data[l->len] = x; l->len = l->len + 1; return 0; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long l_with(long lp, long x) { l_push(lp, x); return lp; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long l_at(long lp, long i) { return ((SList*)(intptr_t)lp)->data[i]; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long l_len(long lp) { return ((SList*)(intptr_t)lp)->len; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"typedef struct { const char* p; long nb, nl, cl, cb; } SL; static SL sl_e[8]; static int sl_k[2]; static SL* sl_last = sl_e; static SL* s_info_slow(const char* p) { for (int k = 0; k < 8; k++) if (sl_e[k].p == p) { sl_last = &sl_e[k]; return sl_last; } long n = strlen(p), m = 0; for (long k = 0; k < n; k++) m += ((unsigned char)p[k] & 192) == 128; int big = n >= 512; SL* e = &sl_e[big * 4 + sl_k[big]]; sl_k[big] = (sl_k[big] + 1) & 3; sl_last = e; e->p = p; e->nb = n; e->nl = n - m; e->cl = 0; e->cb = 0; return e; } static inline SL* s_info(const char* p) { if (sl_last->p == p) return sl_last; return s_info_slow(p); }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"static long s_off_slow(SL* e, long i) { if (i >= e->nl) return e->nb; if (i <= 0) return 0; const unsigned char* p = (const unsigned char*)e->p; long l = e->cl, b = e->cb; if (i < l - i) { l = 0; b = 0; } while (l < i) { b++; while ((p[b] & 192) == 128) b++; l++; } while (l > i) { b--; while (b > 0 && (p[b] & 192) == 128) b--; l--; } e->cl = l; e->cb = b; return b; } static inline long s_off(SL* e, long i) { if (e->nb == e->nl) return i; return s_off_slow(e, i); }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"static char* s_ar; static long s_arn; static char* s_alloc(long n) { if (n > 256) return (char*)malloc(n); if (s_arn < n) { s_ar = (char*)malloc(65536); s_arn = 65536; } char* r = s_ar; s_ar += n; s_arn -= n; return r; } static long s_part(const char* p, long a, long b) { long n = b - a; char* r = s_alloc(n + 1); memcpy(r, p + a, n); r[n] = 0; return (long)(intptr_t)r; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"static char s_one[512]; static char* s_mb[1024]; static long s_let(const char* p, long a, long b) { long n = b - a; if (n == 1) { if (!s_one[2]) for (int k = 0; k < 256; k++) s_one[2 * k] = (char)k; return (long)(intptr_t)&s_one[2 * (unsigned char)p[a]]; } if (n < 1) return s_part(p, a, a); unsigned h = 0; for (long k = 0; k < n; k++) h = h * 31 + (unsigned char)p[a + k]; char** e = &s_mb[h & 1023]; if (*e && strncmp(*e, p + a, n) == 0 && (*e)[n] == 0) return (long)(intptr_t)*e; *e = (char*)(intptr_t)s_part(p, a, b); return (long)(intptr_t)*e; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"static inline long s_each(long s, long* b) { const unsigned char* p = (const unsigned char*)(intptr_t)s; long a = *b, e = a + 1; while ((p[e] & 192) == 128) e++; *b = e; return s_let((const char*)p, a, e); } long s_lenp(const char* p) { return s_info(p)->nl; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"static inline long s_len(long s) { return s_info((const char*)(intptr_t)s)->nl; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long l_setc(long lp, long i, long x, long line) { SList* l = (SList*)(intptr_t)lp; if (i < 0 || i >= l->len) { fprintf(stderr, \"%s: list index %ld, but the list has %ld items\\n\", simpler_at(line), i, l->len); exit(1); } l->data[i] = x; return 0; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long l_removec(long lp, long i, long line) { SList* l = (SList*)(intptr_t)lp; if (i < 0 || i >= l->len) { fprintf(stderr, \"%s: list index %ld, but the list has %ld items\\n\", simpler_at(line), i, l->len); exit(1); } long x = l->data[i]; for (long k = i; k + 1 < l->len; k++) l->data[k] = l->data[k + 1]; l->len = l->len - 1; return x; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long l_atc(long lp, long i, long line) { SList* l = (SList*)(intptr_t)lp; if (i < 0 || i >= l->len) { fprintf(stderr, \"%s: list index %ld, but the list has %ld items\\n\", simpler_at(line), i, l->len); exit(1); } return l->data[i]; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"static inline long s_atc(long s, long i, long line) { const char* p = (const char*)(intptr_t)s; SL* e = s_info(p); if (i < 0 || i >= e->nl) { fprintf(stderr, \"%s: text index %ld, but the text has %ld characters\\n\", simpler_at(line), i, e->nl); exit(1); } long a = s_off(e, i); long b = a + 1; while (((unsigned char)p[b] & 192) == 128) b++; return s_let(p, a, b); }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"static inline long s_slicec(long s, long a, long b, long line) { const char* p = (const char*)(intptr_t)s; SL* e = s_info(p); if (a < 0 || b > e->nl) { fprintf(stderr, \"%s: slice %ld to %ld, but the text has %ld characters\\n\", simpler_at(line), a, b, e->nl); exit(1); } if (b < a) b = a; long x = s_off(e, a); return s_part(p, x, s_off(e, b)); }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long s_isint(long s) { const char* p = (const char*)(intptr_t)s; while (*p == 32 || *p == 9 || *p == 13 || *p == 10) p++; if (*p == 45 || *p == 43) p++; if (*p < 48 || *p > 57) return 0; while (*p >= 48 && *p <= 57) p++; while (*p == 32 || *p == 9 || *p == 13 || *p == 10) p++; return *p == 0; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long s_tointc(long s, long line) { if (!s_isint(s)) { fprintf(stderr, \"%s: \\\"%s\\\" is not a whole number; test with .isInt first\\n\", simpler_at(line), (const char*)(intptr_t)s); exit(1); } return atol((const char*)(intptr_t)s); }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long i_div(long a, long b, long line) { if (b == 0) { fprintf(stderr, \"%s: division by zero\\n\", simpler_at(line)); exit(1); } return a / b; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long i_mod(long a, long b, long line) { if (b == 0) { fprintf(stderr, \"%s: division by zero\\n\", simpler_at(line)); exit(1); } return a % b; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"static inline long s_at(long s, long i) { const char* p = (const char*)(intptr_t)s; SL* e = s_info(p); if (i < 0 || i >= e->nl) return (long)(intptr_t)\"\"; long a = s_off(e, i); long b = a + 1; while (((unsigned char)p[b] & 192) == 128) b++; return s_let(p, a, b); }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long s_code(long s) { const unsigned char* p = (const unsigned char*)(intptr_t)s; long c = p[0]; int n = c >= 240 ? 3 : c >= 224 ? 2 : c >= 192 ? 1 : 0; long v = c & (63 >> n); for (int k = 1; k <= n; k++) { if ((p[k] & 192) != 128) return c; v = (v << 6) | (p[k] & 63); } return n ? v : c; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"static inline long s_slice(long s, long a, long b) { const char* p = (const char*)(intptr_t)s; SL* e = s_info(p); if (b < a) b = a; long x = s_off(e, a); return s_part(p, x, s_off(e, b)); }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long s_concat(long a, long b) { const char* x = (const char*)(intptr_t)a; const char* y = (const char*)(intptr_t)b; char* r = (char*)malloc(strlen(x) + strlen(y) + 1); strcpy(r, x); strcat(r, y); return (long)(intptr_t)r; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"static inline long s_eq(long a, long b) { const char* x = (const char*)(intptr_t)a; const char* y = (const char*)(intptr_t)b; return x == y || (x[0] == y[0] && strcmp(x, y) == 0); }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long i_tostr(long n) { char* r = (char*)malloc(24); sprintf(r, \"%ld\", n); return (long)(intptr_t)r; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"double l2d(long x) { union { long l; double d; } u; u.l = x; return u.d; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long d2l(double x) { union { long l; double d; } u; u.d = x; return u.l; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long f_tostr(long x) { char* r = (char*)malloc(32); snprintf(r, 32, \"%.15g\", l2d(x)); return (long)(intptr_t)r; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long s_tofloat(long s) { return d2l(atof((const char*)(intptr_t)s)); }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long i_tof(long n) { return d2l((double)n); }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long f_toi(long x) { return (long)l2d(x); }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long s_toint(long s) { return atol((const char*)(intptr_t)s); }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long s_split(long s, long d) { const char* p = (const char*)(intptr_t)s; const char* dl = (const char*)(intptr_t)d; long dn = strlen(dl); long l = l_new(); while (1) { const char* q = dn ? strstr(p, dl) : 0; long n = q ? (long)(q - p) : (long)strlen(p); char* r = (char*)malloc(n + 1); memcpy(r, p, n); r[n] = 0; l_push(l, (long)(intptr_t)r); if (!q) break; p = q + dn; } return l; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long l_range(long a, long b) { long l = l_new(); for (long k = a; k < b; k++) l_push(l, k); return l; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long s_starts(long s, long t) { const char* p = (const char*)(intptr_t)s; const char* q = (const char*)(intptr_t)t; return strncmp(p, q, strlen(q)) == 0; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long s_ends(long s, long t) { const char* p = (const char*)(intptr_t)s; const char* q = (const char*)(intptr_t)t; size_t a = strlen(p); size_t b = strlen(q); return b <= a && strcmp(p + a - b, q) == 0; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long s_contains(long h, long n) { return strstr((const char*)(intptr_t)h, (const char*)(intptr_t)n) != 0; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long s_trim(long s) { const char* p = (const char*)(intptr_t)s; long n = strlen(p); long a = 0; while (a < n && (p[a] == 32 || p[a] == 9 || p[a] == 13 || p[a] == 10)) a = a + 1; long b = n; while (b > a && (p[b - 1] == 32 || p[b - 1] == 9 || p[b - 1] == 13 || p[b - 1] == 10)) b = b - 1; long len = b - a; char* r = (char*)malloc(len + 1); for (long k = 0; k < len; k++) r[k] = p[a + k]; r[len] = 0; return (long)(intptr_t)r; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long s_find(long h, long n) { const char* p = (const char*)(intptr_t)h; const char* q = strstr(p, (const char*)(intptr_t)n); if (!q) return -1; long m = 0; for (const char* c = p; c < q; c++) m += c == p || ((unsigned char)*c & 192) != 128; return m; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"static const char* js_ws(const char* p) { while (*p == 32 || *p == 9 || *p == 10 || *p == 13) p++; return p; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"static const char* js_str(const char* p) { p++; while (*p && *p != 34) { if (*p == 92 && p[1]) p++; p++; } if (*p) p++; return p; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"static const char* js_skip(const char* p) { p = js_ws(p); if (*p == 34) return js_str(p); if (*p == 123 || *p == 91) { long d = 0; while (*p) { if (*p == 34) { p = js_str(p); continue; } if (*p == 123 || *p == 91) d++; if (*p == 125 || *p == 93) { d--; if (d == 0) return p + 1; } p++; } return p; } while (*p && *p != 44 && *p != 125 && *p != 93 && *p != 32 && *p != 9 && *p != 10 && *p != 13) p++; return p; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"static long js_hex(const char* p) { long u = 0; for (int k = 0; k < 4; k++) { char h = p[k]; long v; if (h >= 48 && h <= 57) v = h - 48; else if ((h | 32) >= 97 && (h | 32) <= 102) v = (h | 32) - 87; else return -1; u = u * 16 + v; } return u; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"static long js_val(const char* p) { p = js_ws(p); const char* e = js_skip(p); char* r = (char*)malloc((e - p) + 1); long o = 0; if (*p != 34) { o = e - p; memcpy(r, p, o); r[o] = 0; return (long)(intptr_t)r; } p++; while (*p && *p != 34) { if (*p != 92 || p[1] == 0) { r[o++] = *p++; continue; } p++; char c = *p++; if (c == 110) r[o++] = 10; else if (c == 116) r[o++] = 9; else if (c == 114) r[o++] = 13; else if (c == 98) r[o++] = 8; else if (c == 102) r[o++] = 12; else if (c == 117) { long u = js_hex(p); if (u < 0) continue; p += 4; if (u >= 0xD800 && u <= 0xDBFF && p[0] == 92 && p[1] == 117) { long lo = js_hex(p + 2); if (lo >= 0xDC00 && lo <= 0xDFFF) { u = 0x10000 + ((u - 0xD800) << 10) + (lo - 0xDC00); p += 6; } } if (u == 0) continue; if (u < 0x80) r[o++] = (char)u; else if (u < 0x800) { r[o++] = (char)(0xC0 | (u >> 6)); r[o++] = (char)(0x80 | (u & 63)); } else if (u < 0x10000) { r[o++] = (char)(0xE0 | (u >> 12)); r[o++] = (char)(0x80 | ((u >> 6) & 63)); r[o++] = (char)(0x80 | (u & 63)); } else { r[o++] = (char)(0xF0 | (u >> 18)); r[o++] = (char)(0x80 | ((u >> 12) & 63)); r[o++] = (char)(0x80 | ((u >> 6) & 63)); r[o++] = (char)(0x80 | (u & 63)); } } else r[o++] = c; } r[o] = 0; return (long)(intptr_t)r; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"static const char* js_next(const char* p) { p = js_ws(js_skip(p)); return *p == 44 ? js_ws(p + 1) : 0; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long s_json(long t, long k) { const char* p = js_ws((const char*)(intptr_t)t); const char* key = (const char*)(intptr_t)k; if (*p == 123) { p = js_ws(p + 1); while (p && *p == 34) { long kv = js_val(p); p = js_ws(js_str(p)); if (*p == 58) p++; if (strcmp((const char*)(intptr_t)kv, key) == 0) return js_val(p); p = js_next(p); } } else if (*p == 91) { char* end; long want = strtol(key, &end, 10); long i = 0; if (end != key && *end == 0 && want >= 0) { p = js_ws(p + 1); while (p && *p && *p != 93) { if (i == want) return js_val(p); i++; p = js_next(p); } } } return (long)(intptr_t)strdup(\"\"); }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long s_jsonlist(long t, long keys) { long l = l_new(); const char* p = js_ws((const char*)(intptr_t)t); if (*p == 91 && !keys) { p = js_ws(p + 1); while (p && *p && *p != 93) { l_push(l, js_val(p)); p = js_next(p); } } else if (*p == 123) { p = js_ws(p + 1); while (p && *p == 34) { if (keys) l_push(l, js_val(p)); p = js_ws(js_str(p)); if (*p == 58) p++; if (!keys) l_push(l, js_val(p)); p = js_next(p); } } return l; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"typedef struct { long len; unsigned char* data; } SBytes;");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long simpler_readbytes(const char* path) { SBytes* y = (SBytes*)malloc(sizeof(SBytes)); y->len = 0; size_t cap = 4096, got = 0, n; struct stat st; FILE* f = fopen(path, \"rb\"); if (f && fstat(fileno(f), &st) == 0 && st.st_size > 0) cap = (size_t)st.st_size + 1; y->data = (unsigned char*)malloc(cap); if (f) { while ((n = fread(y->data + got, 1, cap - got, f)) > 0) { got += n; if (got == cap) { cap *= 2; y->data = (unsigned char*)realloc(y->data, cap); } } fclose(f); } y->len = (long)got; return (long)(intptr_t)y; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long y_find(long yp, long v, long from) { SBytes* y = (SBytes*)(intptr_t)yp; if (from < 0) from = 0; if (from >= y->len || v < 0 || v > 255) return -1; unsigned char* q = (unsigned char*)memchr(y->data + from, (int)v, (size_t)(y->len - from)); return q ? (long)(q - y->data) : -1; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long y_len(long yp) { return ((SBytes*)(intptr_t)yp)->len; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long y_atc(long yp, long i, long line) { SBytes* y = (SBytes*)(intptr_t)yp; if (i < 0 || i >= y->len) { fprintf(stderr, \"%s: byte %ld, but there are %ld bytes\\n\", simpler_at(line), i, y->len); exit(1); } return y->data[i]; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"static SBytes* y_need(long yp, long i, long n, long line, const char* what) { SBytes* y = (SBytes*)(intptr_t)yp; if (i < 0 || n < 0 || i + n > y->len) { fprintf(stderr, \"%s: %s wants %ld bytes from place %ld, but there are %ld\\n\", simpler_at(line), what, n, i, y->len); exit(1); } return y; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long y_text(long yp, long i, long n, long line) { SBytes* y = y_need(yp, i, n, line, \"textAt\"); char* r = (char*)malloc(n + 1); long o = 0; for (long k = 0; k < n; k++) { unsigned char v = y->data[i + k]; if (v == 0) break; r[o++] = (char)v; } r[o] = 0; return (long)(intptr_t)r; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long y_int(long yp, long i, long n, long line) { if (n < 1 || n > 8) { fprintf(stderr, \"%s: intAt reads 1 to 8 bytes, got %ld\\n\", simpler_at(line), n); exit(1); } SBytes* y = y_need(yp, i, n, line, \"intAt\"); unsigned long u = 0; for (long k = n - 1; k >= 0; k--) u = (u << 8) | y->data[i + k]; return (long)u; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long y_float(long yp, long i, long line) { SBytes* y = y_need(yp, i, 8, line, \"floatAt\"); unsigned long u = 0; for (long k = 7; k >= 0; k--) u = (u << 8) | y->data[i + k]; return (long)u; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long simpler_writebytes(long path, long lp, long line) { SList* l = (SList*)(intptr_t)lp; for (long k = 0; k < l->len; k++) if (l->data[k] < 0 || l->data[k] > 255) { fprintf(stderr, \"%s: a byte is a number from 0 to 255, but place %ld has %ld\\n\", simpler_at(line), k, l->data[k]); exit(1); } FILE* f = fopen((const char*)(intptr_t)path, \"wb\"); if (f) { for (long k = 0; k < l->len; k++) fputc((int)l->data[k], f); fclose(f); } return 0; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"static SList* b_need(long lp, long i, long n, long line, const char* what) { SList* l = (SList*)(intptr_t)lp; if (i < 0 || n < 0 || i + n > l->len) { fprintf(stderr, \"%s: %s wants %ld bytes from place %ld, but the list has %ld\\n\", simpler_at(line), what, n, i, l->len); exit(1); } return l; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long b_text(long lp, long i, long n, long line) { SList* l = b_need(lp, i, n, line, \"textAt\"); char* r = (char*)malloc(n + 1); long o = 0; for (long k = 0; k < n; k++) { long v = l->data[i + k] & 255; if (v == 0) break; r[o++] = (char)v; } r[o] = 0; return (long)(intptr_t)r; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long b_int(long lp, long i, long n, long line) { if (n < 1 || n > 8) { fprintf(stderr, \"%s: intAt reads 1 to 8 bytes, got %ld\\n\", simpler_at(line), n); exit(1); } SList* l = b_need(lp, i, n, line, \"intAt\"); unsigned long u = 0; for (long k = n - 1; k >= 0; k--) u = (u << 8) | (unsigned long)(l->data[i + k] & 255); return (long)u; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long b_float(long lp, long i, long line) { SList* l = b_need(lp, i, 8, line, \"floatAt\"); unsigned long u = 0; for (long k = 7; k >= 0; k--) u = (u << 8) | (unsigned long)(l->data[i + k] & 255); return (long)u; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long s_bytes(long s) { const unsigned char* p = (const unsigned char*)(intptr_t)s; long l = l_new(); while (*p) { l_push(l, *p); p++; } return l; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"enum { RC = 1, RANY, RSET, RBOL, REOL, RWB, RNWB, RSTART, REND, RGRP, RALT, RREP, RBREF, RLOOK };");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"#define RE_MAXG 32");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"#define RE_INF 1073741824");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"typedef struct RNode { int t, lo, hi, lazy, neg, g, nrg; long c; long* rg; struct RNode* sub; struct RNode* alt; struct RNode* next; } RNode;");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"typedef struct { RNode* root; int ng, fi, fm, fs, open, head; unsigned char first[256]; char* key; long line; const char* pp; const char* b; const char* e; const char* end; const char* cs[RE_MAXG]; const char* ce[RE_MAXG]; long steps, limit; const char* base; } RE;");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"typedef struct RK { int kind; RNode* n; struct RK* up; long count; const char* last; } RK;");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"static void re_die(RE* R, const char* why) { fprintf(stderr, \"%s: the pattern \\\"%s\\\" %s\\n\", simpler_at(R->line), R->key, why); exit(1); }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"static RNode* re_new(int t) { RNode* n = (RNode*)calloc(1, sizeof(RNode)); n->t = t; return n; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"static const char* re_dec(const char* p, long* c) { const unsigned char* u = (const unsigned char*)p; if (u[0] < 0x80) { *c = u[0]; return p + 1; } if (u[0] >= 0xC0 && u[0] < 0xE0 && (u[1] & 0xC0) == 0x80) { *c = ((long)(u[0] & 31) << 6) | (u[1] & 63); return p + 2; } if (u[0] >= 0xE0 && u[0] < 0xF0 && (u[1] & 0xC0) == 0x80 && (u[2] & 0xC0) == 0x80) { *c = ((long)(u[0] & 15) << 12) | ((long)(u[1] & 63) << 6) | (u[2] & 63); return p + 3; } if (u[0] >= 0xF0 && (u[1] & 0xC0) == 0x80 && (u[2] & 0xC0) == 0x80 && (u[3] & 0xC0) == 0x80) { *c = ((long)(u[0] & 7) << 18) | ((long)(u[1] & 63) << 12) | ((long)(u[2] & 63) << 6) | (u[3] & 63); return p + 4; } *c = 0x110000 + u[0]; return p + 1; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"static const unsigned int re_wt[] = {170,170,178,179,181,181,185,186,188,190,192,214,216,246,248,705,710,721,736,740,748,748,750,750,880,884,886,893,895,895,902,902,904,1013,1015,1153,1162,1369,1376,1416,1488,1522,1568,1610,1632,1641,1646,1647,1649,1747,1749,1749,1765,1766,1774,1788,1791,1791,1808,1808,1810,1839,1869,1957,1969,2026,2036,2037,2042,2042,2048,2069,2074,2074,2084,2084,2088,2088,2112,2136,2144,2183,2185,2190,2208,2249,2308,2361,2365,2365,2384,2384,2392,2401,2406,2415,2417,2432,2437,2489,2493,2493,2510,2510,2524,2529,2534,2545,2548,2553,2556,2556,2565,2617,2649,2671,2674,2676,2693,2745,2749,2749,2768,2785,2790,2799,2809,2809,2821,2873,2877,2877,2908,2913,2918,2927,2929,2935,2947,3001,3024,3024,3046,3058,3077,3129,3133,3133,3160,3169,3174,3183,3192,3198,3200,3200,3205,3257,3261,3261,3293,3297,3302,3314,3332,3386,3389,3389,3406,3406,3412,3414,3416,3425,3430,3448,3450,3455,3461,3526,3558,3567,3585,3632,3634,3635,3648,3654,3664,3673,3713,3760,3762,3763,3773,3782,3792,3840,3872,3891,3904,3948,3976,3980,4096,4138,4159,4169,4176,4181,4186,4189,4193,4193,4197,4198,4206,4208,4213,4225,4238,4238,4240,4249,4256,4346,4348,4954,4969,5007,5024,5117,5121,5740,5743,5759,5761,5786,5792,5866,5870,5905,5919,5937,5952,5969,5984,6000,6016,6067,6103,6103,6108,6108,6112,6137,6160,6276,6279,6312,6314,6430,6470,6618,6656,6678,6688,6740,6784,6809,6823,6823,6917,6963,6981,6988,6992,7001,7043,7072,7086,7141,7168,7203,7232,7293,7296,7359,7401,7404,7406,7411,7413,7414,7418,7615,7680,8124,8126,8126,8130,8140,8144,8155,8160,8172,8178,8188,8304,8313,8319,8329,8336,8348,8450,8450,8455,8455,8458,8467,8469,8469,8473,8477,8484,8484,8486,8486,8488,8488,8490,8493,8495,8505,8508,8511,8517,8521,8526,8526,8528,8585,9312,9371,9450,9471,10102,10131,11264,11492,11499,11502,11506,11507,11517,11517,11520,11631,11648,11742,11823,11823,12293,12295,12321,12329,12337,12341,12344,12348,12353,12438,12445,12447,12449,12538,12540,12686,12690,12693,12704,12735,12784,12799,12832,12841,12872,12879,12881,12895,12928,12937,12977,12991,13312,19903,19968,42124,42192,42237,42240,42508,42512,42606,42623,42653,42656,42735,42775,42783,42786,42888,42891,43009,43011,43013,43015,43018,43020,43042,43056,43061,43072,43123,43138,43187,43216,43225,43250,43255,43259,43259,43261,43262,43264,43301,43312,43334,43360,43388,43396,43442,43471,43481,43488,43492,43494,43560,43584,43586,43588,43595,43600,43609,43616,43638,43642,43642,43646,43695,43697,43697,43701,43702,43705,43709,43712,43712,43714,43741,43744,43754,43762,43764,43777,43866,43868,43881,43888,44002,44016,55291,63744,64285,64287,64296,64298,64433,64467,64829,64848,64967,65008,65019,65136,65276,65296,65305,65313,65338,65345,65370,65382,65500,65536,65786,65799,65843,65856,65912,65930,65931,66176,66256,66273,66421,66432,66461,66464,66511,66513,66915,66928,67669,67672,67702,67705,67867,67872,67897,67968,68096,68112,68149,68160,68168,68192,68222,68224,68295,68297,68324,68331,68335,68352,68405,68416,68497,68521,68899,68912,68965,68975,68997,69216,69289,69296,69316,69376,69445,69457,69460,69488,69505,69552,69622,69635,69687,69714,69743,69745,69746,69749,69749,69763,69807,69840,69881,69891,69926,69942,69951,69956,69956,69959,70002,70006,70006,70019,70066,70081,70084,70096,70106,70108,70108,70113,70187,70207,70208,70272,70312,70320,70366,70384,70393,70405,70457,70461,70461,70480,70480,70493,70497,70528,70583,70609,70609,70611,70611,70656,70708,70727,70730,70736,70745,70751,70831,70852,70853,70855,71086,71128,71131,71168,71215,71236,71257,71296,71338,71352,71352,71360,71450,71472,71483,71488,71723,71840,71983,71999,71999,72001,72001,72016,72144,72161,72161,72163,72163,72192,72192,72203,72242,72250,72250,72272,72272,72284,72329,72349,72349,72368,72440,72640,72672,72688,72750,72768,72768,72784,72812,72818,72847,72960,73008,73030,73030,73040,73097,73112,73458,73474,73474,73476,73523,73552,73561,73648,73684,73728,74862,74880,77808,77824,78895,78913,78918,78944,90397,90416,92777,92784,92909,92928,92975,92992,92995,93008,93548,93552,93846,93952,94026,94032,94032,94099,94177,94179,94179,94208,113817,118000,118009,119488,119539,119648,120512,120514,120538,120540,120570,120572,120596,120598,120628,120630,120654,120656,120686,120688,120712,120714,120744,120746,120770,120772,120831,122624,122666,122928,122989,123136,123180,123191,123214,123536,123565,123584,123627,123632,123641,124112,124139,124144,124397,124400,124410,124896,125135,125184,125251,125259,125273,126065,126123,126125,126127,126129,126253,126255,126651,127232,127244,130032,195101}; static int re_word(long c) { if (c < 128) return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'; int lo = 0, hi = (int)(sizeof(re_wt) / sizeof(re_wt[0]) / 2) - 1; while (lo <= hi) { int mid = (lo + hi) / 2; if (c < (long)re_wt[2 * mid]) hi = mid - 1; else if (c > (long)re_wt[2 * mid + 1]) lo = mid + 1; else return 1; } return 0; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"static long re_low(long c) { if ((c >= 'A' && c <= 'Z') || (c >= 0xC0 && c <= 0xDE && c != 0xD7)) return c + 32; return c; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"static long re_up(long c) { if ((c >= 'a' && c <= 'z') || (c >= 0xE0 && c <= 0xFE && c != 0xF7)) return c - 32; return c; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"static int re_cls(long cls, long c) { int r; long l = cls | 32; if (l == 'd') r = c >= '0' && c <= '9'; else if (l == 'w') r = re_word(c); else r = c == 32 || (c >= 9 && c <= 13); return (cls & 32) ? r : !r; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"static int re_inset(RE* R, RNode* n, long c) { long l = re_low(c), u = re_up(c); for (int i = 0; i < n->nrg; i += 2) { long lo = n->rg[i], hi = n->rg[i + 1]; if (lo == -1) { if (re_cls(hi, c)) return 1; } else if (c >= lo && c <= hi) return 1; else if (R->fi && ((l >= lo && l <= hi) || (u >= lo && u <= hi))) return 1; } return 0; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"static void re_setadd(RNode* n, long lo, long hi) { n->rg = (long*)realloc(n->rg, sizeof(long) * (n->nrg + 2)); n->rg[n->nrg++] = lo; n->rg[n->nrg++] = hi; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"static long re_hex(RE* R, int digits) { long u = 0; for (int k = 0; k < digits; k++) { char h = *R->pp; long v; if (h >= '0' && h <= '9') v = h - '0'; else if ((h | 32) >= 'a' && (h | 32) <= 'f') v = (h | 32) - 'a' + 10; else re_die(R, \"has a \\\\x or \\\\u without its hex digits\"); u = u * 16 + v; R->pp++; } return u; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"static long re_escc(RE* R, int* cls) { long c; *cls = 0; if (!*R->pp) re_die(R, \"ends in a backslash\"); R->pp = re_dec(R->pp, &c); switch (c) { case 'n': return 10; case 't': return 9; case 'r': return 13; case 'f': return 12; case 'v': return 11; case 'e': return 27; case 'b': return 8; case 'x': return re_hex(R, 2); case 'u': return re_hex(R, 4); case 'd': case 'D': case 'w': case 'W': case 's': case 'S': *cls = (int)c; return 0; } if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) re_die(R, \"has a backslash before a letter that means nothing\"); return c; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"static RNode* re_set(RE* R) { RNode* n = re_new(RSET); int first = 1; if (*R->pp == '^') { n->neg = 1; R->pp++; } while (1) { long lo, hi; int cls = 0; if (!*R->pp) re_die(R, \"has a [ with no ]\"); if (*R->pp == ']' && !first) { R->pp++; break; } first = 0; if (*R->pp == '\\\\') { R->pp++; lo = re_escc(R, &cls); } else R->pp = re_dec(R->pp, &lo); if (cls) { re_setadd(n, -1, cls); continue; } hi = lo; if (*R->pp == '-' && R->pp[1] && R->pp[1] != ']') { int c2 = 0; R->pp++; if (*R->pp == '\\\\') { R->pp++; hi = re_escc(R, &c2); if (c2) re_die(R, \"has a range that ends in a class\"); } else R->pp = re_dec(R->pp, &hi); if (hi < lo) re_die(R, \"has a range that runs backwards\"); } re_setadd(n, lo, hi); } return n; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"static RNode* re_alt(RE* R, int depth);");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"static RNode* re_atom(RE* R, int depth) {");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"  long c; char c0 = *R->pp; RNode* n;");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"  if (c0 == '(') { n = re_new(RGRP); R->pp++; int cap = 1;");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"    if (*R->pp == '?') { char f = R->pp[1]; cap = 0;");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"      if (f == ':') R->pp += 2;");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"      else if (f == '=' || f == '!') { n->t = RLOOK; n->c = 1; n->neg = f == '!'; R->pp += 2; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"      else if (f == '<' && (R->pp[2] == '=' || R->pp[2] == '!')) { n->t = RLOOK; n->c = 2; n->neg = R->pp[2] == '!'; R->pp += 3; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"      else if (f == '<' || (f == 'P' && R->pp[2] == '<')) { while (*R->pp && *R->pp != '>') R->pp++; if (!*R->pp) re_die(R, \"has a group name with no >\"); R->pp++; cap = 1; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"      else { const char* q = R->pp + 1; while (*q == 'i' || *q == 'm' || *q == 's') q++; if (*q != ')' || q == R->pp + 1) re_die(R, \"has a (? form that is not known\"); for (q = R->pp + 1; *q != ')'; q++) { if (*q == 'i') R->fi = 1; if (*q == 'm') R->fm = 1; if (*q == 's') R->fs = 1; } R->pp = q + 1; return n; } }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"    if (cap) { if (R->ng + 1 >= RE_MAXG) re_die(R, \"has more than 31 groups\"); n->g = ++R->ng; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"    n->sub = re_alt(R, depth + 1); if (*R->pp != ')') re_die(R, \"has a ( with no )\"); R->pp++; return n; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"  if (c0 == '[') { R->pp++; return re_set(R); }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"  if (c0 == '.') { R->pp++; return re_new(RANY); }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"  if (c0 == '^') { R->pp++; return re_new(RBOL); }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"  if (c0 == '$') { R->pp++; return re_new(REOL); }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"  if (c0 == '*' || c0 == '+' || c0 == '?') re_die(R, \"has a * + or ? with nothing before it\");");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"  if (c0 == '\\\\') { int cls; char e = R->pp[1]; R->pp++;");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"    if (e == 'b') { R->pp++; return re_new(RWB); } if (e == 'B') { R->pp++; return re_new(RNWB); }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"    if (e == 'A') { R->pp++; return re_new(RSTART); } if (e == 'Z' || e == 'z') { R->pp++; return re_new(REND); }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"    if (e >= '1' && e <= '9') { R->pp++; n = re_new(RBREF); n->g = e - '0'; return n; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"    c = re_escc(R, &cls); if (cls) { n = re_new(RSET); re_setadd(n, -1, cls); return n; } n = re_new(RC); n->c = c; return n; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"  R->pp = re_dec(R->pp, &c); n = re_new(RC); n->c = c; return n; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"static RNode* re_seq(RE* R, int depth) { RNode* head = 0; RNode** tail = &head; while (*R->pp && *R->pp != '|' && *R->pp != ')') { RNode* a = re_atom(R, depth); while (1) { long lo, hi; const char* q = R->pp; char c = *q; if (c == '*') { lo = 0; hi = RE_INF; q++; } else if (c == '+') { lo = 1; hi = RE_INF; q++; } else if (c == '?') { lo = 0; hi = 1; q++; } else if (c == '{' && ((q[1] >= '0' && q[1] <= '9') || (q[1] == ',' && q[2] >= '0' && q[2] <= '9'))) { char* t = (char*)q + 1; lo = 0; if (*t != ',') lo = strtol(q + 1, &t, 10); hi = lo; if (*t == ',') { t++; if (*t == '}') hi = RE_INF; else if (*t >= '0' && *t <= '9') hi = strtol(t, &t, 10); else break; } if (*t != '}') break; q = t + 1; if (hi < lo) re_die(R, \"has a count that runs backwards\"); if (lo > 1000 || (hi != RE_INF && hi > 1000)) re_die(R, \"has a count above 1000\"); } else break; RNode* r = re_new(RREP); r->lo = (int)lo; r->hi = (int)hi; r->sub = a; if (*q == '?') { r->lazy = 1; q++; } else if (*q == '+') q++; R->pp = q; a = r; } *tail = a; tail = &a->next; } return head; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"static RNode* re_alt(RE* R, int depth) { if (depth > 200) re_die(R, \"has groups more than 200 deep\"); RNode* first = re_seq(R, depth); if (*R->pp != '|') return first; RNode* head = re_new(RALT); RNode* cur = head; head->sub = first; while (*R->pp == '|') { RNode* a = re_new(RALT); R->pp++; a->sub = re_seq(R, depth); cur->alt = a; cur = a; } return head; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"static void re_enc(RE* R, long c) { if (c < 0x80) R->first[c] = 1; else if (c < 0x800) R->first[0xC0 | (c >> 6)] = 1; else if (c < 0x10000) R->first[0xE0 | (c >> 12)] = 1; else if (c < 0x110000) R->first[0xF0 | (c >> 18)] = 1; else R->first[(c - 0x110000) & 255] = 1; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"static int re_first(RE* R, RNode* n) { for (; n; n = n->next) { int nul = 1; switch (n->t) { case RC: re_enc(R, n->c); if (R->fi) { re_enc(R, re_low(n->c)); re_enc(R, re_up(n->c)); } nul = 0; break; case RANY: memset(R->first, 1, 256); nul = 0; break; case RSET: for (long b = 0; b < 128; b++) if (re_inset(R, n, b) != n->neg) R->first[b] = 1; memset(R->first + 128, 1, 128); nul = 0; break; case RGRP: nul = re_first(R, n->sub); break; case RALT: nul = 0; for (RNode* a = n; a; a = a->alt) if (re_first(R, a->sub)) nul = 1; break; case RREP: nul = re_first(R, n->sub); if (n->lo == 0) nul = 1; break; case RBREF: memset(R->first, 1, 256); break; default: break; } if (!nul) return 0; } return 1; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"static RE* re_comp(long pat, long line) { static RE* cache[512]; const char* src = (const char*)(intptr_t)pat; unsigned long h = 5381; for (const unsigned char* q = (const unsigned char*)src; *q; q++) h = h * 33 + *q; RE* R = cache[h & 511]; if (R && strcmp(R->key, src) == 0) { R->line = line; return R; } R = (RE*)calloc(1, sizeof(RE)); R->key = strdup(src); R->pp = R->key; R->line = line; R->root = re_alt(R, 0); if (*R->pp == ')') re_die(R, \"has a ) with no (\"); R->open = re_first(R, R->root); R->head = R->root && !R->fm && (R->root->t == RSTART || R->root->t == RBOL); if (R->root && R->root->t == RSTART) R->head = 1; cache[h & 511] = R; return R; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"static int re_m(RE* R, RNode* n, const char* p, RK* k);");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"static const char* re_one(RE* R, RNode* n, const char* p) { long c; const char* q; if (p >= R->e) return 0; q = re_dec(p, &c); if (n->t == RC) return (c == n->c || (R->fi && re_low(c) == re_low(n->c))) ? q : 0; if (n->t == RANY) return (c != 10 || R->fs) ? q : 0; return re_inset(R, n, c) != n->neg ? q : 0; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"static int re_rep(RE* R, RNode* n, const char* p, RK* k, long count, const char* last) {");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"  RNode* s = n->sub;");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"  if (s && !s->next && (s->t == RC || s->t == RANY || s->t == RSET)) { const char* q = p; const char* r; long c = 0;");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"    if (!n->lazy) { while (c < n->hi && (r = re_one(R, s, q))) { q = r; c++; } if (c < n->lo) return 0; while (1) { if (re_m(R, n->next, q, k)) return 1; if (c == n->lo || q == p) return 0; q--; while (q > p && ((unsigned char)*q & 0xC0) == 0x80) q--; c--; } }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"    while (c < n->lo) { r = re_one(R, s, q); if (!r) return 0; q = r; c++; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"    while (1) { if (re_m(R, n->next, q, k)) return 1; if (c >= n->hi) return 0; r = re_one(R, s, q); if (!r) return 0; q = r; c++; } }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"  if (count > 0 && p == last && count >= n->lo) return re_m(R, n->next, p, k);");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"  RK kk = { 1, n, k, count + 1, p };");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"  if (!n->lazy) { if (count < n->hi && re_m(R, s, p, &kk)) return 1; return count >= n->lo ? re_m(R, n->next, p, k) : 0; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"  if (count >= n->lo && re_m(R, n->next, p, k)) return 1;");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"  return count < n->hi ? re_m(R, s, p, &kk) : 0; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"static int re_k(RE* R, const char* p, RK* k) {");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"  if (!k) { R->end = p; return 1; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"  if (k->kind == 0) { RNode* n = k->n; if (n->g > 0) { const char* os = R->cs[n->g]; const char* oe = R->ce[n->g]; R->cs[n->g] = k->last; R->ce[n->g] = p; if (re_m(R, n->next, p, k->up)) return 1; R->cs[n->g] = os; R->ce[n->g] = oe; return 0; } return re_m(R, n->next, p, k->up); }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"  if (k->kind == 1) return re_rep(R, k->n, p, k->up, k->count, k->last);");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"  if (k->kind == 2) return 1;");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"  return p == k->last; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"static int re_look(RE* R, RNode* n, const char* p, RK* k) { const char* scs[RE_MAXG]; const char* sce[RE_MAXG]; int ok = 0; memcpy(scs, R->cs, sizeof(scs)); memcpy(sce, R->ce, sizeof(sce)); if (n->c == 1) { RK kk = { 2, 0, 0, 0, 0 }; ok = re_m(R, n->sub, p, &kk); } else { RK kk = { 3, 0, 0, 0, p }; const char* q = p; while (1) { if (re_m(R, n->sub, q, &kk)) { ok = 1; break; } if (q == R->b) break; q--; while (q > R->b && ((unsigned char)*q & 0xC0) == 0x80) q--; } } if (ok != n->neg && re_m(R, n->next, p, k)) return 1; memcpy(R->cs, scs, sizeof(scs)); memcpy(R->ce, sce, sizeof(sce)); return 0; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"static long re_budget = 0;");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"static int re_m(RE* R, RNode* n, const char* p, RK* k) {");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"  char here;");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"  if (++R->steps > R->limit) re_die(R, \"takes too many steps on this text\");");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"  if (R->base - &here > re_budget) re_die(R, \"goes too deep on this text\");");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"  while (n && n->t <= RSET) { p = re_one(R, n, p); if (!p) return 0; n = n->next; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"  if (!n) return re_k(R, p, k);");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"  switch (n->t) {");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"    case RBOL: return (p == R->b || (R->fm && p[-1] == 10)) ? re_m(R, n->next, p, k) : 0;");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"    case REOL: return (p == R->e || (R->fm && *p == 10) || (p + 1 == R->e && *p == 10)) ? re_m(R, n->next, p, k) : 0;");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"    case RSTART: return p == R->b ? re_m(R, n->next, p, k) : 0;");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"    case REND: return p == R->e ? re_m(R, n->next, p, k) : 0;");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"    case RWB: case RNWB: { long ca = 0, cb = 0; if (p > R->b) { const char* q = p - 1; while (q > R->b && ((unsigned char)*q & 192) == 128) q--; re_dec(q, &ca); } if (p < R->e) re_dec(p, &cb); int a = p > R->b && re_word(ca); int b = p < R->e && re_word(cb); return ((a != b) == (n->t == RWB)) ? re_m(R, n->next, p, k) : 0; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"    case RGRP: { RK kk = { 0, n, k, 0, p }; return re_m(R, n->sub, p, &kk); }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"    case RALT: { for (RNode* a = n; a; a = a->alt) if (re_m(R, a->sub, p, k)) return 1; return 0; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"    case RREP: return re_rep(R, n, p, k, 0, 0);");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"    case RBREF: { const char* s = n->g <= R->ng ? R->cs[n->g] : 0; const char* e = n->g <= R->ng ? R->ce[n->g] : 0; long len; if (!s || !e) return 0; len = e - s; if (R->e - p < len) return 0; for (long i = 0; i < len; i++) if (s[i] != p[i] && !(R->fi && (unsigned char)s[i] < 128 && re_low((unsigned char)s[i]) == re_low((unsigned char)p[i]))) return 0; return re_m(R, n->next, p + len, k); }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"    case RLOOK: return re_look(R, n, p, k);");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"  }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"  return 0; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"static const char* re_step(RE* R, const char* p) { if (p >= R->e) return R->e + 1; p++; while (p < R->e && ((unsigned char)*p & 0xC0) == 0x80) p++; return p; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"static const char* re_search(RE* R, const char* from) { char mark; const char* p = from; RNode* f = R->root; int lit = f && f->t == RC && !R->fi && f->c < 128 && f->c > 0; R->steps = 0; R->base = &mark; while (1) { if (lit) { p = (const char*)memchr(p, (int)f->c, (size_t)(R->e - p)); if (!p) return 0; } else if (!R->open) { while (p < R->e && !R->first[(unsigned char)*p]) p++; if (p >= R->e) return 0; } for (int i = 0; i <= R->ng; i++) { R->cs[i] = 0; R->ce[i] = 0; } if (re_m(R, R->root, p, 0)) { R->cs[0] = p; R->ce[0] = R->end; return p; } if (p >= R->e || R->head) return 0; p = re_step(R, p); } }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"static void re_stack(void) { struct rlimit rl; long cur = 8388608; long want = 67108864; if (getrlimit(RLIMIT_STACK, &rl) == 0) {");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"#ifdef __linux__");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"if (rl.rlim_cur != RLIM_INFINITY && (long)rl.rlim_cur < want && (rl.rlim_max == RLIM_INFINITY || (long)rl.rlim_max >= want)) { rl.rlim_cur = (rlim_t)want; setrlimit(RLIMIT_STACK, &rl); getrlimit(RLIMIT_STACK, &rl); }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"#endif");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"cur = (rl.rlim_cur == RLIM_INFINITY || (long)rl.rlim_cur > want) ? want : (long)rl.rlim_cur; } re_budget = cur / 2; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"static RE* re_open(long s, long pat, long line) { RE* R = re_comp(pat, line); if (!re_budget) re_stack(); R->b = (const char*)(intptr_t)s; R->e = R->b + strlen(R->b); R->limit = 1000000 + 100 * (long)(R->e - R->b); return R; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"static long re_str(const char* a, const char* b) { long n = (a && b && b > a) ? (long)(b - a) : 0; char* r = (char*)malloc(n + 1); if (n) memcpy(r, a, n); r[n] = 0; return (long)(intptr_t)r; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"static long re_groups(RE* R) { long l = l_new(); for (int i = 0; i <= R->ng; i++) l_push(l, re_str(R->cs[i], R->ce[i])); return l; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"static const char* re_after(RE* R) { return R->ce[0] > R->cs[0] ? R->ce[0] : re_step(R, R->ce[0]); }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long re_test(long s, long pat, long line) { RE* R = re_open(s, pat, line); return re_search(R, R->b) != 0; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long re_find(long s, long pat, long line) { RE* R = re_open(s, pat, line); if (!re_search(R, R->b)) return l_new(); return re_groups(R); }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long re_all(long s, long pat, long line) { RE* R = re_open(s, pat, line); long out = l_new(); const char* p = R->b; while (p <= R->e && re_search(R, p)) { l_push(out, re_groups(R)); p = re_after(R); } return out; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long re_split(long s, long pat, long line) { RE* R = re_open(s, pat, line); long out = l_new(); const char* p = R->b; const char* last = p; while (p <= R->e && re_search(R, p)) { if (R->ce[0] == R->cs[0]) { p = re_after(R); continue; } l_push(out, re_str(last, R->cs[0])); last = R->ce[0]; p = last; } l_push(out, re_str(last, R->e)); return out; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"static void re_put(char** b, size_t* n, size_t* cap, const char* a, size_t len) { if (*n + len + 1 > *cap) { *cap = (*n + len + 1) * 2; *b = (char*)realloc(*b, *cap); } if (len) memcpy(*b + *n, a, len); *n += len; (*b)[*n] = 0; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long re_replace(long s, long pat, long to, long line) { RE* R = re_open(s, pat, line); const char* t = (const char*)(intptr_t)to; size_t n = 0, cap = 64; char* out = (char*)malloc(cap); const char* p = R->b; const char* last = p; out[0] = 0; while (p <= R->e && re_search(R, p)) { re_put(&out, &n, &cap, last, (size_t)(R->cs[0] - last)); for (const char* q = t; *q; q++) { if (*q == '$' && q[1] == '$') { re_put(&out, &n, &cap, q, 1); q++; } else if (*q == '$' && q[1] >= '0' && q[1] <= '9') { int g = q[1] - '0'; q++; if (q[1] >= '0' && q[1] <= '9' && g * 10 + (q[1] - '0') <= R->ng) { g = g * 10 + (q[1] - '0'); q++; } if (g <= R->ng && R->cs[g] && R->ce[g] > R->cs[g]) re_put(&out, &n, &cap, R->cs[g], (size_t)(R->ce[g] - R->cs[g])); } else re_put(&out, &n, &cap, q, 1); } last = R->ce[0]; p = re_after(R); } if (last < R->e) re_put(&out, &n, &cap, last, (size_t)(R->e - last)); return (long)(intptr_t)out; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long s_case(long s, long up) { const unsigned char* p = (const unsigned char*)(intptr_t)s; long n = strlen((const char*)p); unsigned char* r = (unsigned char*)malloc(n + 1); for (long i = 0; i <= n; i++) { unsigned char c = p[i]; if (up && c >= 97 && c <= 122) c = c - 32; if (!up && c >= 65 && c <= 90) c = c + 32; if (i > 0 && p[i - 1] == 195) { if (up && c >= 160 && c <= 190 && c != 183) c = c - 32; if (!up && c >= 128 && c <= 158 && c != 151) c = c + 32; } r[i] = c; } return (long)(intptr_t)r; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long l_join(long lp, long sep) { SList* s = (SList*)(intptr_t)lp; const char* q = (const char*)(intptr_t)sep; long m = strlen(q); long n = 1; for (long i = 0; i < s->len; i++) n += strlen((const char*)(intptr_t)s->data[i]) + m; char* r = (char*)malloc(n); r[0] = 0; for (long i = 0; i < s->len; i++) { if (i > 0) strcat(r, q); strcat(r, (const char*)(intptr_t)s->data[i]); } return (long)(intptr_t)r; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long s_padleft(long s, long n) { const char* p = (const char*)(intptr_t)s; SL* e = s_info(p); long nb = e->nb; long pad = n - e->nl; if (pad <= 0) return s; char* r = (char*)malloc(nb + pad + 1); memset(r, 32, pad); memcpy(r + pad, p, nb + 1); return (long)(intptr_t)r; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long s_padright(long s, long n) { const char* p = (const char*)(intptr_t)s; SL* e = s_info(p); long nb = e->nb; long pad = n - e->nl; if (pad <= 0) return s; char* r = (char*)malloc(nb + pad + 1); memcpy(r, p, nb); memset(r + nb, 32, pad); r[nb + pad] = 0; return (long)(intptr_t)r; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long s_replace(long s, long from, long to) { const char* p = (const char*)(intptr_t)s; const char* f = (const char*)(intptr_t)from; const char* t = (const char*)(intptr_t)to; long fl = strlen(f); if (fl == 0) return s; long tl = strlen(t); long pl = strlen(p); long count = 0; const char* q = p; while ((q = strstr(q, f)) != 0) { count++; q += fl; } char* r = (char*)malloc(pl + count * (tl - fl) + 1); char* w = r; const char* a = p; while (1) { const char* hit = strstr(a, f); if (hit == 0) { strcpy(w, a); break; } long pre = hit - a; memcpy(w, a, pre); w += pre; memcpy(w, t, tl); w += tl; a = hit + fl; } return (long)(intptr_t)r; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"typedef struct { long* keys; long* vals; unsigned long* hs; int* tab; long len; long cap; long tcap; } SMap;");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"static unsigned long m_hash(const char* s) { unsigned long h = 1469598103934665603UL; while (*s) { h ^= (unsigned char)*s++; h *= 1099511628211UL; } return h; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"static long m_findh(SMap* m, const char* key, unsigned long h) { long mask = m->tcap - 1; long j = (long)(h & (unsigned long)mask); while (m->tab[j]) { long i = m->tab[j] - 1; if (m->hs[i] == h && strcmp((const char*)(intptr_t)m->keys[i], key) == 0) return i; j = (j + 1) & mask; } return -1; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long m_find(long mp, long k) { const char* key = (const char*)(intptr_t)k; return m_findh((SMap*)(intptr_t)mp, key, m_hash(key)); }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long Map() { SMap* m = (SMap*)malloc(sizeof(SMap)); m->cap = 8; m->len = 0; m->keys = (long*)malloc(sizeof(long) * m->cap); m->vals = (long*)malloc(sizeof(long) * m->cap); m->hs = (unsigned long*)malloc(sizeof(unsigned long) * m->cap); m->tcap = 16; m->tab = (int*)calloc(m->tcap, sizeof(int)); return (long)(intptr_t)m; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long m_set(long mp, long k, long v) { SMap* m = (SMap*)(intptr_t)mp; const char* key = (const char*)(intptr_t)k; unsigned long h = m_hash(key); long i = m_findh(m, key, h); if (i >= 0) { m->vals[i] = v; return 0; } if (m->len == m->cap) { m->cap = m->cap * 2; m->keys = (long*)realloc(m->keys, sizeof(long) * m->cap); m->vals = (long*)realloc(m->vals, sizeof(long) * m->cap); m->hs = (unsigned long*)realloc(m->hs, sizeof(unsigned long) * m->cap); } if ((m->len + 1) * 2 > m->tcap) { m->tcap = m->tcap * 2; free(m->tab); m->tab = (int*)calloc(m->tcap, sizeof(int)); for (long q = 0; q < m->len; q++) { long j = (long)(m->hs[q] & (unsigned long)(m->tcap - 1)); while (m->tab[j]) j = (j + 1) & (m->tcap - 1); m->tab[j] = (int)(q + 1); } } m->keys[m->len] = k; m->vals[m->len] = v; m->hs[m->len] = h; long j = (long)(h & (unsigned long)(m->tcap - 1)); while (m->tab[j]) j = (j + 1) & (m->tcap - 1); m->tab[j] = (int)(m->len + 1); m->len = m->len + 1; return 0; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long m_get(long mp, long k) { long i = m_find(mp, k); if (i >= 0) return ((SMap*)(intptr_t)mp)->vals[i]; return 0; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long m_gets(long mp, long k) { long i = m_find(mp, k); if (i >= 0) return ((SMap*)(intptr_t)mp)->vals[i]; return (long)(intptr_t)\"\"; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long m_getp(long mp, long k, long line) { long i = m_find(mp, k); if (i >= 0) return ((SMap*)(intptr_t)mp)->vals[i]; fprintf(stderr, \"%s: the map has no key \\\"%s\\\"; ask .has(key) first\\n\", simpler_at(line), (const char*)(intptr_t)k); exit(1); return 0; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long m_has(long mp, long k) { return m_find(mp, k) >= 0; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long m_keys(long mp) { SMap* m = (SMap*)(intptr_t)mp; long l = l_new(); for (long i = 0; i < m->len; i++) l_push(l, m->keys[i]); return l; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long m_byvalue(long mp) { SMap* m = (SMap*)(intptr_t)mp; long n = m->len; long c = n > 0 ? n : 1; long* idx = (long*)malloc(sizeof(long) * c); for (long i = 0; i < n; i++) idx[i] = i; for (long i = 1; i < n; i++) { long t = idx[i]; long j = i - 1; while (j >= 0 && m->vals[idx[j]] < m->vals[t]) { idx[j + 1] = idx[j]; j = j - 1; } idx[j + 1] = t; } long l = l_new(); for (long i = 0; i < n; i++) l_push(l, m->keys[idx[i]]); return l; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"int cmp_long(const void* a, const void* b) { long x = *(const long*)a; long y = *(const long*)b; if (x < y) return -1; if (x > y) return 1; return 0; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"int cmp_str(const void* a, const void* b) { return strcmp((const char*)(intptr_t)(*(const long*)a), (const char*)(intptr_t)(*(const long*)b)); }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long l_sort(long lp, long isStr) { SList* s = (SList*)(intptr_t)lp; long n = s->len; long c = n > 0 ? n : 1; long* d = (long*)malloc(sizeof(long) * c); for (long i = 0; i < n; i++) d[i] = s->data[i]; qsort(d, n, sizeof(long), isStr ? cmp_str : cmp_long); long r = l_new(); for (long i = 0; i < n; i++) l_push(r, d[i]); return r; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"const char* simpler_read(const char* path) { FILE* f = fopen(path, \"rb\"); if (!f) return \"\"; size_t cap = 4096, got = 0, n; char* buf = (char*)malloc(cap + 1); while ((n = fread(buf + got, 1, cap - got, f)) > 0) { got += n; if (got == cap) { cap *= 2; buf = (char*)realloc(buf, cap + 1); } } buf[got] = 0; fclose(f); return buf; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long simpler_write(long path, long content) { FILE* f = fopen((const char*)(intptr_t)path, \"w\"); if (f) { fputs((const char*)(intptr_t)content, f); fclose(f); } return 0; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long simpler_append(long path, long content) { FILE* f = fopen((const char*)(intptr_t)path, \"a\"); if (f) { fputs((const char*)(intptr_t)content, f); fclose(f); } return 0; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long simpler_args(int argc, char** argv) { long l = l_new(); for (int k = 1; k < argc; k++) l_push(l, (long)(intptr_t)argv[k]); return l; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"static int simpler_glob(const char* pat, const char* s) { const char* star = 0; const char* mark = 0; while (*s) { if (*pat == '*') { star = pat++; mark = s; } else if (*pat == *s) { pat++; s++; } else if (star && *mark != '/') { pat = star + 1; s = ++mark; } else return 0; } while (*pat == '*') pat++; return *pat == 0; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"static void simpler_inside(const char* dir, const char* p, long line, const char* verb) { char rd[PATH_MAX], rp[PATH_MAX], tmp[PATH_MAX]; if (strlen(p) >= PATH_MAX || !realpath(dir, rd)) return; if (!realpath(p, rp)) { struct stat st; int link = lstat(p, &st) == 0 && S_ISLNK(st.st_mode); strcpy(tmp, p); char* sl = strrchr(tmp, '/'); if (!sl) strcpy(tmp, \".\"); else if (sl == tmp) tmp[1] = 0; else *sl = 0; if (!link && !realpath(tmp, rp)) return; if (link) rp[0] = 0; } size_t n = strlen(rd); if (n == 1 || (strncmp(rd, rp, n) == 0 && (rp[n] == 0 || rp[n] == '/'))) return; if ((strncmp(rd, \"/sys\", 4) == 0 && strncmp(rp, \"/sys/\", 5) == 0) || (strncmp(rd, \"/proc\", 5) == 0 && strncmp(rp, \"/proc/\", 6) == 0)) return; fprintf(stderr, \"%s: this program may not %s \\\"%s\\\": a link leads out of \\\"%s\\\"\\n\", simpler_at(line), verb, p, dir); exit(1); }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"int simpler_argc = 0; char** simpler_argv = 0; static const char* simpler_home(const char* p) { const char* h = getenv(\"HOME\"); if (p[0] != '~' || p[1] != '/' || !h) return p; char* r = (char*)malloc(strlen(h) + strlen(p) + 1); strcpy(r, h); strcat(r, p + 1); return r; } long simpler_may(long path, const char** ok, long line, const char* verb) { const char* p = simpler_home((const char*)(intptr_t)path); path = (long)(intptr_t)p; for (int k = 0; ok[k]; k++) { const char* e = simpler_home(ok[k]); size_t n = strlen(e); if (strcmp(e, \"<args>\") == 0) { for (int a = 1; a < simpler_argc; a++) if (strcmp(simpler_argv[a], p) == 0) return path; continue; } if (strcmp(e, p) == 0) return path; if (strchr(e, '*') && simpler_glob(e, p) && strstr(p, \"..\") == 0) { char d[PATH_MAX]; size_t m = strcspn(e, \"*\"); while (m > 0 && e[m - 1] != '/') m--; if (m == 0) strcpy(d, \".\"); else if (m < PATH_MAX) { memcpy(d, e, m); d[m] = 0; } else d[0] = 0; if (d[0]) simpler_inside(d, p, line, verb); return path; } if (strcmp(e, \"./\") == 0 && p[0] != '/' && strstr(p, \"..\") == 0) { simpler_inside(\".\", p, line, verb); return path; } if (n > 0 && e[n - 1] == '/' && strncmp(e, p, n) == 0 && strstr(p, \"..\") == 0) { simpler_inside(e, p, line, verb); return path; } } fprintf(stderr, \"%s: this program may not %s \\\"%s\\\"\\n\", simpler_at(line), verb, p); exit(1); return 0; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long simpler_dir(long path) { const char* p = (const char*)(intptr_t)path; size_t n = strlen(p); if (n > 0 && p[n - 1] == '/') return path; char* r = (char*)malloc(n + 2); strcpy(r, p); r[n] = '/'; r[n + 1] = 0; return (long)(intptr_t)r; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long simpler_list(long path) { const char* p = (const char*)(intptr_t)path; long l = l_new(); DIR* d = opendir(p); if (!d) return l; struct dirent* e; while ((e = readdir(d)) != 0) { if (strcmp(e->d_name, \".\") == 0 || strcmp(e->d_name, \"..\") == 0) continue; size_t n = strlen(p) + strlen(e->d_name) + 2; char* full = (char*)malloc(n); strcpy(full, p); strcat(full, e->d_name); struct stat st; char* name = (char*)malloc(strlen(e->d_name) + 2); strcpy(name, e->d_name); if (stat(full, &st) == 0 && S_ISDIR(st.st_mode)) strcat(name, \"/\"); free(full); l_push(l, (long)(intptr_t)name); } closedir(d); SList* s = (SList*)(intptr_t)l; qsort(s->data, s->len, sizeof(long), cmp_str); return l; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long simpler_exists(long path) { struct stat st; return stat((const char*)(intptr_t)path, &st) == 0; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long simpler_rename(long a, long b, long line) { if (rename((const char*)(intptr_t)a, (const char*)(intptr_t)b) != 0) { fprintf(stderr, \"%s: could not rename \\\"%s\\\": %s\\n\", simpler_at(line), (const char*)(intptr_t)a, strerror(errno)); exit(1); } return 0; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long simpler_delete(long a, long line) { if (remove((const char*)(intptr_t)a) != 0) { fprintf(stderr, \"%s: could not delete \\\"%s\\\": %s\\n\", simpler_at(line), (const char*)(intptr_t)a, strerror(errno)); exit(1); } return 0; }");

  (printf("%s\n", (const char*)(intptr_t)u_mayTable((long)(intptr_t)"simpler_rd", ((R_Prog*)(intptr_t)prog)->reads)), 0L);

  (printf("%s\n", (const char*)(intptr_t)u_mayTable((long)(intptr_t)"simpler_wr", ((R_Prog*)(intptr_t)prog)->writes)), 0L);

  (printf("%s\n", (const char*)(intptr_t)u_mayTable((long)(intptr_t)"simpler_rn", ((R_Prog*)(intptr_t)prog)->runs)), 0L);

  (printf("%s\n", (const char*)(intptr_t)u_mayTable((long)(intptr_t)"simpler_ft", ((R_Prog*)(intptr_t)prog)->fetches)), 0L);

  (printf("%s\n", (const char*)(intptr_t)u_mayTable((long)(intptr_t)"simpler_ml", ((R_Prog*)(intptr_t)prog)->mails)), 0L);

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"static int simpler_quiet = 0; long simpler_exec(char** av, int capture, int* code, long input) { int fd[2] = {0, 0}; FILE* in = 0; if (input) { in = tmpfile(); if (in) { fputs((const char*)(intptr_t)input, in); fflush(in); rewind(in); } } if (capture && pipe(fd) != 0) { *code = 127; return (long)(intptr_t)\"\"; } fflush(stdout); pid_t pid = fork(); if (pid < 0) { *code = 127; return (long)(intptr_t)\"\"; } if (pid == 0) { if (simpler_quiet && !freopen(\"/dev/null\", \"w\", stderr)) _exit(127); if (in) dup2(fileno(in), 0); if (capture) { dup2(fd[1], 1); close(fd[0]); close(fd[1]); } execvp(av[0], av); _exit(127); } if (in) fclose(in); long cap = 1024; long len = 0; char* buf = (char*)malloc(cap); if (capture) { close(fd[1]); long n; while ((n = read(fd[0], buf + len, cap - len - 1)) > 0) { len += n; if (len + 1 >= cap) { cap *= 2; buf = (char*)realloc(buf, cap); } } close(fd[0]); } buf[len] = 0; int st = 0; waitpid(pid, &st, 0); *code = WIFEXITED(st) ? WEXITSTATUS(st) : 128; return (long)(intptr_t)buf; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"char** simpler_argv_of(long name, long args) { SList* l = (SList*)(intptr_t)args; char** av = (char**)malloc(sizeof(char*) * (l->len + 2)); av[0] = (char*)(intptr_t)name; for (long k = 0; k < l->len; k++) av[k + 1] = (char*)(intptr_t)l->data[k]; av[l->len + 1] = 0; return av; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long simpler_runok(long name, long line) { const char* p = (const char*)(intptr_t)name; for (int k = 0; simpler_rn[k]; k++) if (strcmp(simpler_rn[k], p) == 0) return name; fprintf(stderr, \"%s: this program may not run \\\"%s\\\"\\n\", simpler_at(line), p); exit(1); return 0; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long simpler_send(long to, long subject, long body, long line) { const char* t = (const char*)(intptr_t)to; const char* s = (const char*)(intptr_t)subject; const char* b = (const char*)(intptr_t)body; size_t tn = strlen(t); int ok = 0; const char* at = strchr(t, 64); int clean = at && at != t && at[1] && strchr(at + 1, 64) == 0 && t[0] != '-'; for (const char* c = t; *c; c++) if ((unsigned char)*c <= 32 || *c == ',' || *c == ';' || *c == '<' || *c == '>' || *c == 127) clean = 0; for (int k = 0; clean && simpler_ml[k]; k++) { const char* e = simpler_ml[k]; size_t n = strlen(e); if (strcmp(e, t) == 0 || (e[0] == 64 && tn > n && strcmp(t + tn - n, e) == 0) || (strchr(e, '*') && simpler_glob(e, t))) ok = 1; } if (!ok) { fprintf(stderr, \"%s: this program may not mail \\\"%s\\\"\\n\", simpler_at(line), t); exit(1); } if (strchr(s, 10) || strchr(s, 13)) { fprintf(stderr, \"%s: the subject of a mail is one line\\n\", simpler_at(line)); exit(1); } const char* prog = getenv(\"SIMPLER_SENDMAIL\"); if (!prog || !*prog) prog = \"sendmail\"; char* msg = (char*)malloc(tn + strlen(s) + strlen(b) + 32); sprintf(msg, \"To: %s\\nSubject: %s\\n\\n%s\\n\", t, s, b); char* av[] = {(char*)prog, (char*)\"-i\", (char*)\"--\", (char*)t, 0}; int code = 0; simpler_exec(av, 1, &code, (long)(intptr_t)msg); if (code != 0) { fprintf(stderr, \"%s: could not send mail to \\\"%s\\\" (%s ended with status %d)\\n\", simpler_at(line), t, prog, code); exit(1); } return 0; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long simpler_run(long name, long args, long input, long line) { int code = 0; long out = simpler_exec(simpler_argv_of(simpler_runok(name, line), args), 1, &code, input); if (code != 0) { fprintf(stderr, \"%s: \\\"%s\\\" ended with status %d\\n\", simpler_at(line), (const char*)(intptr_t)name, code); exit(1); } return out; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long simpler_try(long name, long args, long line) { int code = 0; simpler_quiet = 1; long out = simpler_exec(simpler_argv_of(simpler_runok(name, line), args), 1, &code, 0); simpler_quiet = 0; return code == 0 ? out : (long)(intptr_t)\"\"; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long simpler_status(long name, long args, long line) { int code = 0; simpler_exec(simpler_argv_of(simpler_runok(name, line), args), 0, &code, 0); return code; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long simpler_fetchok(long url, long line) { const char* u = (const char*)(intptr_t)url; const char* h = 0; if (strncmp(u, \"https://\", 8) == 0) h = u + 8; else if (strncmp(u, \"http://\", 7) == 0) h = u + 7; if (h) { size_t n = strcspn(h, \"/?#\"); if (memchr(h, 64, n) == 0 && memchr(h, 92, n) == 0) { size_t hn = strcspn(h, \":/?#\"); char hb[256]; if (hn < sizeof hb) { memcpy(hb, h, hn); hb[hn] = 0; for (int k = 0; simpler_ft[k]; k++) if (strcmp(simpler_ft[k], hb) == 0 || (strchr(simpler_ft[k], '*') && simpler_glob(simpler_ft[k], hb))) return url; } } } fprintf(stderr, \"%s: this program may not fetch \\\"%s\\\"\\n\", simpler_at(line), u); exit(1); return 0; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long i_char(long n) { char* o = (char*)malloc(5); int k = 0; if (n > 0 && n < 128) o[k++] = (char)n; else if (n >= 128 && n < 2048) { o[k++] = (char)(192 | (n >> 6)); o[k++] = (char)(128 | (n & 63)); } else if (n >= 2048 && n < 65536) { o[k++] = (char)(224 | (n >> 12)); o[k++] = (char)(128 | ((n >> 6) & 63)); o[k++] = (char)(128 | (n & 63)); } else if (n >= 65536 && n < 1114112) { o[k++] = (char)(240 | (n >> 18)); o[k++] = (char)(128 | ((n >> 12) & 63)); o[k++] = (char)(128 | ((n >> 6) & 63)); o[k++] = (char)(128 | (n & 63)); } o[k] = 0; return (long)(intptr_t)o; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long simpler_millis() { struct timespec ts; clock_gettime(CLOCK_REALTIME, &ts); return (long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long simpler_start(long name, long args, long line) { char** av = simpler_argv_of(simpler_runok(name, line), args); int fd[2]; if (pipe(fd) != 0) return -1; fflush(stdout); pid_t p = fork(); if (p < 0) return -1; if (p == 0) { close(fd[0]); setsid(); pid_t q = fork(); if (q != 0) { if (write(fd[1], &q, sizeof q) < 0) _exit(1); _exit(0); } close(fd[1]); if (!freopen(\"/dev/null\", \"r\", stdin)) _exit(127); execvp(av[0], av); _exit(127); } close(fd[1]); pid_t q = -1; if (read(fd[0], &q, sizeof q) != (long)sizeof q) q = -1; close(fd[0]); waitpid(p, 0, 0); return (long)q; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"static long simpler_hdr = 0; long simpler_curl(long url, long body, long post, long* st) { char* av[48]; int n = 0; av[n++] = \"curl\"; av[n++] = \"-sS\"; av[n++] = \"--max-time\"; av[n++] = \"30\"; av[n++] = \"-w\"; av[n++] = \"\\n%{http_code}\"; if (simpler_hdr) { SList* h = (SList*)(intptr_t)simpler_hdr; for (long k = 0; k < h->len && k < 16; k++) { av[n++] = \"-H\"; av[n++] = (char*)(intptr_t)h->data[k]; } } if (post) { av[n++] = \"--data-raw\"; av[n++] = (char*)(intptr_t)body; } av[n++] = \"--\"; av[n++] = (char*)(intptr_t)url; av[n] = 0; int code = 0; char* out = (char*)(intptr_t)simpler_exec(av, 1, &code, 0); char* nl = strrchr(out, 10); *st = (nl && code == 0) ? atol(nl + 1) : 0; if (nl) *nl = 0; return (long)(intptr_t)out; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long simpler_fetch(long url, long body, long post, long line) { simpler_fetchok(url, line); long st = 0; long out = simpler_curl(url, body, post, &st); if (st < 200 || st > 299) { fprintf(stderr, \"%s: could not fetch \\\"%s\\\" (status %ld)\\n\", simpler_at(line), (const char*)(intptr_t)url, st); exit(1); } return out; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long simpler_reach(long url, long line) { simpler_fetchok(url, line); long st = 0; fflush(stderr); int keep = dup(2); FILE* nul = fopen(\"/dev/null\", \"w\"); if (nul) dup2(fileno(nul), 2); simpler_curl(url, 0, 0, &st); if (nul) { dup2(keep, 2); fclose(nul); } close(keep); return st; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"static struct termios simpler_tio; static int simpler_tty = 0; static int simpler_err = -1; static FILE* simpler_errf = 0;");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"typedef struct { char g[6]; unsigned short st; } SCell; static SCell* simpler_back = 0; static SCell* simpler_front = 0; static long simpler_tr = 0, simpler_tc = 0; static int simpler_all = 1; static volatile sig_atomic_t simpler_winched = 0; static char simpler_sty[1024][40]; static int simpler_nsty = 1; static char* simpler_ob = 0; static long simpler_on = 0, simpler_ocap = 0;");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"static void simpler_o(const char* s) { long n = (long)strlen(s); if (simpler_on + n + 1 > simpler_ocap) { simpler_ocap = (simpler_on + n + 1) * 2 + 4096; simpler_ob = (char*)realloc(simpler_ob, simpler_ocap); } memcpy(simpler_ob + simpler_on, s, n); simpler_on += n; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"static void simpler_term_blank(SCell* a) { long n = simpler_tr * simpler_tc; memset(a, 0, n * sizeof(SCell)); for (long k = 0; k < n; k++) a[k].g[0] = 32; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"static void simpler_term_fit(void) { struct winsize w; long r = 24, c = 80; if (ioctl(1, TIOCGWINSZ, &w) == 0 && w.ws_row && w.ws_col) { r = w.ws_row; c = w.ws_col; } if (simpler_back && r == simpler_tr && c == simpler_tc) return; free(simpler_back); free(simpler_front); simpler_tr = r; simpler_tc = c; simpler_back = (SCell*)malloc(r * c * sizeof(SCell)); simpler_front = (SCell*)malloc(r * c * sizeof(SCell)); simpler_term_blank(simpler_back); simpler_term_blank(simpler_front); simpler_all = 1; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"static void simpler_term_flush(void) { fflush(stdout); if (!simpler_back) return; char b[64]; simpler_on = 0; if (simpler_all) { simpler_o(\"\\033[0m\\033[2J\"); simpler_term_blank(simpler_front); simpler_all = 0; } long cur = -1, pr = -1, pc = -1; for (long r = 0; r < simpler_tr; r++) for (long c = 0; c < simpler_tc; c++) { SCell* k = simpler_back + r * simpler_tc + c; SCell* f = simpler_front + r * simpler_tc + c; if (memcmp(k, f, sizeof(SCell)) == 0) continue; if (pr == r && c > pc && c - pc <= 4) { long g = pc; while (g < c && simpler_back[r * simpler_tc + g].st == cur) g++; if (g == c) { for (g = pc; g < c; g++) simpler_o(simpler_back[r * simpler_tc + g].g); pc = c; } } if (pr != r || pc != c) { snprintf(b, sizeof b, \"\\033[%ld;%ldH\", r + 1, c + 1); simpler_o(b); } if (k->st != cur) { simpler_o(\"\\033[0\"); if (k->st) { simpler_o(\";\"); simpler_o(simpler_sty[k->st]); } simpler_o(\"m\"); cur = k->st; } simpler_o(k->g); *f = *k; pr = r; pc = c + 1; } if (simpler_on == 0) return; simpler_o(\"\\033[0m\"); long off = 0; while (off < simpler_on) { ssize_t n = write(1, simpler_ob + off, simpler_on - off); if (n <= 0) { if (n < 0 && errno == EINTR) continue; break; } off += n; } }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"static void simpler_term_end(void) { if (!simpler_tty) return; simpler_tty = 0; fputs(\"\\033[0m\\033[?25h\\033[?1049l\", stdout); fflush(stdout); tcsetattr(0, TCSAFLUSH, &simpler_tio); if (simpler_err >= 0) { fflush(stderr); dup2(simpler_err, 2); close(simpler_err); simpler_err = -1; if (simpler_errf) { char b[512]; size_t n; rewind(simpler_errf); while ((n = fread(b, 1, sizeof b, simpler_errf)) > 0) fwrite(b, 1, n, stderr); fclose(simpler_errf); simpler_errf = 0; } } }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"static void simpler_winch(int g) { (void)g; simpler_winched = 1; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"static void simpler_quit(int g) { exit(128 + g); }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"static void simpler_term_on(void) { if (simpler_tty) return; if (tcgetattr(0, &simpler_tio) != 0) { fprintf(stderr, \"sys.term needs a terminal on standard input\\n\"); exit(1); } struct termios r = simpler_tio; cfmakeraw(&r); r.c_lflag |= ISIG; tcsetattr(0, TCSADRAIN, &r); static int once = 0; if (!once) { once = 1; atexit(simpler_term_end); struct sigaction sa; memset(&sa, 0, sizeof sa); sa.sa_handler = simpler_winch; sigaction(SIGWINCH, &sa, 0); sa.sa_handler = simpler_quit; sigaction(SIGINT, &sa, 0); sigaction(SIGTERM, &sa, 0); } simpler_errf = tmpfile(); if (simpler_errf) { fflush(stderr); simpler_err = dup(2); dup2(fileno(simpler_errf), 2); } simpler_tty = 1; fputs(\"\\033[?1049h\\033[?25l\", stdout); fflush(stdout); simpler_term_fit(); simpler_all = 1; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long simpler_term_stop(void) { simpler_term_end(); return 0; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long simpler_term_size(long cols) { if (!simpler_back) simpler_term_fit(); return cols ? simpler_tc : simpler_tr; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long simpler_term_clear(void) { simpler_term_on(); simpler_term_blank(simpler_back); return 0; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"static const char* simpler_style(const char* w, long n) { static const char* k[] = {\"bold\",\"1\",\"dim\",\"2\",\"underline\",\"4\",\"reverse\",\"7\",\"black\",\"30\",\"red\",\"31\",\"green\",\"32\",\"yellow\",\"33\",\"blue\",\"34\",\"magenta\",\"35\",\"cyan\",\"36\",\"white\",\"37\",\"gray\",\"90\",0}; for (int i = 0; k[i]; i += 2) if ((long)strlen(k[i]) == n && strncmp(k[i], w, n) == 0) return k[i + 1]; int bg = n > 2 && w[0] == 'o' && w[1] == 'n'; long v = 0, d = 0; for (long i = bg ? 2 : 0; i < n; i++) { if (w[i] < '0' || w[i] > '9' || ++d > 3) return 0; v = v * 10 + (w[i] - '0'); } if (d == 0 || v > 255) return 0; static char b[32]; snprintf(b, sizeof b, \"%d;5;%ld\", bg ? 48 : 38, v); return b; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long simpler_term_put(long row, long col, long text, long style, long line) { simpler_term_on(); const char* t = (const char*)(intptr_t)text; const char* y = (const char*)(intptr_t)style; long room = simpler_tc - col; if (row < 0 || col < 0 || row >= simpler_tr || room <= 0) return 0; char sg[40]; long sn = 0; while (*y) { while (*y == 32) y++; long n = 0; while (y[n] && y[n] != 32) n++; if (n == 0) break; const char* c = simpler_style(y, n); if (!c) { fprintf(stderr, \"%s: no style named \\\"%.*s\\\"; the styles are bold, dim, underline, reverse, black, red, green, yellow, blue, magenta, cyan, white, gray, a colour number 0 to 255, and on with a number for the background (on52)\\n\", simpler_at(line), (int)n, y); exit(1); } long cn = (long)strlen(c); if (sn + cn + 2 < (long)sizeof sg) { if (sn) sg[sn++] = 59; memcpy(sg + sn, c, cn); sn += cn; } y += n; } sg[sn] = 0; static int last = 0; int st = 0; if (sn) { if (strcmp(simpler_sty[last], sg) == 0) st = last; else { for (int k = 1; k < simpler_nsty && !st; k++) if (strcmp(simpler_sty[k], sg) == 0) st = k; if (!st && simpler_nsty < 1024) { st = simpler_nsty++; strcpy(simpler_sty[st], sg); } last = st; } } SCell* k = simpler_back + row * simpler_tc + col - 1; long gn = 5; for (; *t; t++) { unsigned char ch = (unsigned char)*t; if ((ch & 0xC0) != 0x80) { if (room == 0) break; room--; k++; memset(k, 0, sizeof(SCell)); k->st = (unsigned short)st; gn = 0; } if (gn < 4) k->g[gn++] = (char)(ch < 32 || ch == 127 ? 63 : ch); } return 0; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"static long simpler_keyname(const char* k) { return (long)(intptr_t)k; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long simpler_term_key(void) { simpler_term_on(); if (simpler_winched) { simpler_winched = 0; simpler_term_fit(); return simpler_keyname(\"resize\"); } simpler_term_flush(); unsigned char b[16]; ssize_t n = read(0, b, 1); if (n < 0) { if (errno != EINTR) return simpler_keyname(\"eof\"); simpler_winched = 0; simpler_term_fit(); return simpler_keyname(\"resize\"); } if (n == 0) return simpler_keyname(\"eof\"); unsigned char c = b[0]; if (c == 27) { struct pollfd pf = {0, POLLIN, 0}; long m = 0; while (m < 8 && poll(&pf, 1, m == 0 ? 30 : 0) > 0 && read(0, b + m, 1) == 1) { m++; if (m > 1 && b[m - 1] >= 64) break; } if (m == 0) return simpler_keyname(\"esc\"); unsigned char e = b[m - 1]; if (m >= 2 && (b[0] == '[' || b[0] == 'O')) { if (e == 'A') return simpler_keyname(\"up\"); if (e == 'B') return simpler_keyname(\"down\"); if (e == 'C') return simpler_keyname(\"right\"); if (e == 'D') return simpler_keyname(\"left\"); if (e == 'H') return simpler_keyname(\"home\"); if (e == 'F') return simpler_keyname(\"end\"); if (e == '~') { if (b[1] == '1' || b[1] == '7') return simpler_keyname(\"home\"); if (b[1] == '4' || b[1] == '8') return simpler_keyname(\"end\"); if (b[1] == '3') return simpler_keyname(\"delete\"); if (b[1] == '5') return simpler_keyname(\"pgup\"); if (b[1] == '6') return simpler_keyname(\"pgdn\"); } } return simpler_keyname(\"other\"); } if (c == 13 || c == 10) return simpler_keyname(\"enter\"); if (c == 9) return simpler_keyname(\"tab\"); if (c == 127 || c == 8) return simpler_keyname(\"backspace\"); char* r = (char*)malloc(8); if (c < 32) { sprintf(r, \"ctrl-%c\", c + 96); return (long)(intptr_t)r; } long len = c >= 0xF0 ? 4 : c >= 0xE0 ? 3 : c >= 0xC0 ? 2 : 1; r[0] = (char)c; for (long i = 1; i < len; i++) if (read(0, r + i, 1) != 1) { len = i; break; } r[len] = 0; return (long)(intptr_t)r; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long simpler_now() { return (long)time(0); }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long simpler_fetchwith(long url, long hdrs, long line) { SList* h = (SList*)(intptr_t)hdrs; for (long k = 0; k < h->len; k++) { const char* x = (const char*)(intptr_t)h->data[k]; if (k >= 16 || x[0] == '@' || !strchr(x, ':') || strchr(x, 10) || strchr(x, 13)) { fprintf(stderr, \"%s: a header is one line like \\\"Name: value\\\", and there are at most 16; got \\\"%s\\\"\\n\", simpler_at(line), x); exit(1); } } simpler_hdr = hdrs; long out = simpler_fetch(url, 0, 0, line); simpler_hdr = 0; return out; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long simpler_stamp(long t, long pat) { time_t tt = (time_t)t; struct tm* m = localtime(&tt); char* r = (char*)malloc(256); if (strftime(r, 256, (const char*)(intptr_t)pat, m) == 0) r[0] = 0; return (long)(intptr_t)r; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long simpler_secs(long text) { const char* s = (const char*)(intptr_t)text; struct tm m; memset(&m, 0, sizeof m); int y, mo, d, h = 0, mi = 0, se = 0, n = 0; if (sscanf(s, \"%d-%d-%d%n\", &y, &mo, &d, &n) != 3 || mo < 1 || mo > 12 || d < 1 || d > 31) return 0; s += n; if (*s == 'T' || *s == ' ') { n = 0; if (sscanf(s + 1, \"%d:%d%n\", &h, &mi, &n) != 2) return 0; s += 1 + n; n = 0; if (*s == ':' && sscanf(s + 1, \"%d%n\", &se, &n) == 1) s += 1 + n; if (*s == '.') { s++; while (*s >= '0' && *s <= '9') s++; } } m.tm_year = y - 1900; m.tm_mon = mo - 1; m.tm_mday = d; m.tm_hour = h; m.tm_min = mi; m.tm_sec = se; m.tm_isdst = -1; if (*s == 'Z') return (long)timegm(&m); if (*s == '+' || *s == '-') { int oh = 0, om = 0; if (sscanf(s + 1, \"%d:%d\", &oh, &om) < 1) return 0; if (oh > 99) { om = oh % 100; oh = oh / 100; } long off = oh * 3600L + om * 60L; return (long)timegm(&m) - (*s == '-' ? -off : off); } return (long)mktime(&m); }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long simpler_date(long t) { time_t tt = (time_t)t; struct tm* m = localtime(&tt); char* r = (char*)malloc(32); strftime(r, 32, \"%Y-%m-%d %H:%M:%S\", m); return (long)(intptr_t)r; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long simpler_setenv(long name, long value) { const char* n = (const char*)(intptr_t)name; if (!n[0] || strchr(n, 61)) return 0; setenv(n, (const char*)(intptr_t)value, 1); return 1; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"static char* simpler_selfp = 0; void simpler_self_init() { const char* e = getenv(\"SIMPLER_SELF\"); if (e) { simpler_selfp = strdup(e); unsetenv(\"SIMPLER_SELF\"); } } long simpler_self() { if (!simpler_selfp) { char b[4096]; ssize_t n = readlink(\"/proc/self/exe\", b, sizeof b - 1); if (n > 0) { b[n] = 0; simpler_selfp = strdup(b); } else { char* r = realpath(simpler_argv[0], 0); simpler_selfp = r ? r : simpler_argv[0]; } } return (long)(intptr_t)simpler_selfp; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long simpler_env(long name) { const char* v = getenv((const char*)(intptr_t)name); return (long)(intptr_t)(v ? v : \"\"); }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long simpler_stdin() { long cap = 1024; long len = 0; char* buf = (char*)malloc(cap); int c; while ((c = getchar()) != EOF) { if (len + 1 >= cap) { cap = cap * 2; buf = (char*)realloc(buf, cap); } buf[len] = (char)c; len = len + 1; } buf[len] = 0; return (long)(intptr_t)buf; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long fail(long msg) { fprintf(stderr, \"%s\\n\", (const char*)(intptr_t)msg); exit(1); return 0; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long simpler_exit(long c) { exit((int)c); return 0; }");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long simpler_failed = 0;");

  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long s_assert(long c) { if (!c) simpler_failed = 1; return 0; }");

  {
  long _lst = ((R_Prog*)(intptr_t)prog)->types;
  long _n = l_len(_lst);
  for (long _i = 0; _i < _n; _i = _i + 1) {
  long t = l_at(_lst, _i);

  (printf("%s\n", (const char*)(intptr_t)u_emitType(t)), 0L);
  }
  }

  {
  long _lst = ((R_Prog*)(intptr_t)prog)->records;
  long _n = l_len(_lst);
  for (long _i = 0; _i < _n; _i = _i + 1) {
  long r = l_at(_lst, _i);

  (printf("%s\n", (const char*)(intptr_t)u_emitRecord(r)), 0L);
  }
  }

  {
  long _lst = ((R_Prog*)(intptr_t)prog)->fns;
  long _n = l_len(_lst);
  for (long _i = 0; _i < _n; _i = _i + 1) {
  long f = l_at(_lst, _i);

  if ((!u_isMain(((R_Fn*)(intptr_t)f)->name))) {

  (printf("%s\n", (const char*)(intptr_t)u_emitProto(f)), 0L);
  }
  }
  }

  testing = ((l_len(simpler_args(argc, argv)) > 0L) && s_eq(l_atc(simpler_args(argc, argv), 0L, 555), (long)(intptr_t)"test"));

  if (((!testing) && (!u_hasName(((R_Sigs*)(intptr_t)sigs)->fnNames, (long)(intptr_t)"main")))) {

  fail((long)(intptr_t)"input.smplr:1: no main function found");
  }

  u_retPass(prog, sigs);

  {
  long _lst = ((R_Prog*)(intptr_t)prog)->fns;
  long _n = l_len(_lst);
  for (long _i = 0; _i < _n; _i = _i + 1) {
  long f = l_at(_lst, _i);

  if ((!(testing && u_isMain(((R_Fn*)(intptr_t)f)->name)))) {

  (printf("%s\n", (const char*)(intptr_t)u_emitFn(f, sigs)), 0L);
  }
  }
  }

  if (testing) {

  (printf("%s\n", (const char*)(intptr_t)u_emitTests(((R_Prog*)(intptr_t)prog)->fns)), 0L);
  }
  return 0;
}
long u_isMayLine(long toks, long i) {

  return (((u_isWord(toks, i, (long)(intptr_t)"reads") || u_isWord(toks, i, (long)(intptr_t)"writes")) && (u_isStrTok(toks, (i + 1L)) || u_isWord(toks, (i + 1L), (long)(intptr_t)"args"))) || (((u_isWord(toks, i, (long)(intptr_t)"runs") || u_isWord(toks, i, (long)(intptr_t)"fetches")) || u_isWord(toks, i, (long)(intptr_t)"mails")) && u_isStrTok(toks, (i + 1L))));
  return 0;
}
long u_isStrTok(long toks, long i) {

  switch (((Obj*)(intptr_t)u_tokAt(toks, i))->tag) {
  case T_Ident: { long s = ((Obj*)(intptr_t)u_tokAt(toks, i))->v0; return false; }
  case T_Int: { long v = ((Obj*)(intptr_t)u_tokAt(toks, i))->v0; return false; }
  case T_Float: { long f = ((Obj*)(intptr_t)u_tokAt(toks, i))->v0; return false; }
  case T_Str: { long s = ((Obj*)(intptr_t)u_tokAt(toks, i))->v0; return true; }
  case T_Punct: { long p = ((Obj*)(intptr_t)u_tokAt(toks, i))->v0; return false; }
  case T_Eof: { return false; }
  }
  return 0;
  return 0;
}
long u_strAt(long toks, long i) {

  switch (((Obj*)(intptr_t)u_tokAt(toks, i))->tag) {
  case T_Ident: { long s = ((Obj*)(intptr_t)u_tokAt(toks, i))->v0; return (long)(intptr_t)"?"; }
  case T_Int: { long v = ((Obj*)(intptr_t)u_tokAt(toks, i))->v0; return (long)(intptr_t)"?"; }
  case T_Float: { long f = ((Obj*)(intptr_t)u_tokAt(toks, i))->v0; return (long)(intptr_t)"?"; }
  case T_Str: { long s = ((Obj*)(intptr_t)u_tokAt(toks, i))->v0; return s; }
  case T_Punct: { long p = ((Obj*)(intptr_t)u_tokAt(toks, i))->v0; return (long)(intptr_t)"?"; }
  case T_Eof: { return (long)(intptr_t)"?"; }
  }
  return 0;
  return 0;
}
long u_parseMay(long toks, long i, long reads, long writes, long runs, long fetches, long mails) {
  long w = 0;
  long ml = 0;
  long rn = 0;
  long ft = 0;
  long j = 0;
  long more = 0;
  long name = 0;

  w = u_isWord(toks, i, (long)(intptr_t)"writes");

  ml = u_isWord(toks, i, (long)(intptr_t)"mails");

  rn = (u_isWord(toks, i, (long)(intptr_t)"runs") || ml);

  ft = u_isWord(toks, i, (long)(intptr_t)"fetches");

  j = (i + 1L);

  more = true;

  while (more) {

  name = (long)(intptr_t)"<args>";

  if ((((!u_isWord(toks, j, (long)(intptr_t)"args")) || rn) || ft)) {

  if (((!u_isStrTok(toks, j)) && (rn || ft))) {

  u_failTok(toks, j, (long)(intptr_t)"expected a name in quotes");
  }

  if ((!u_isStrTok(toks, j))) {

  u_failTok(toks, j, (long)(intptr_t)"expected a file name in quotes, or the word args");
  }

  name = u_strAt(toks, j);
  }

  if (w) {

  l_push(writes, name);
  } else {
  if (ml) {

  l_push(mails, name);
  } else {
  if (rn) {

  l_push(runs, name);
  } else {
  if (ft) {

  l_push(fetches, name);
  } else {

  l_push(reads, name);
  }
  }
  }
  }

  j = (j + 1L);

  more = u_isPunct(toks, j, (long)(intptr_t)",");

  if (more) {

  j = (j + 1L);
  }
  }

  return j;
  return 0;
}
long u_mayTable(long cname, long names) {
  long out = 0;

  out = s_concat(s_concat((long)(intptr_t)"const char* ", cname), (long)(intptr_t)"[] = {");

  {
  long _lst = names;
  long _n = l_len(_lst);
  for (long _i = 0; _i < _n; _i = _i + 1) {
  long n = l_at(_lst, _i);

  out = s_concat(({ long _t26 = s_concat(out, (long)(intptr_t)"\""); long _t27 = u_cEscape(n); s_concat(_t26, _t27); }), (long)(intptr_t)"\", ");
  }
  }

  return s_concat(out, (long)(intptr_t)"0};");
  return 0;
}
long u_globOk(long pat, long s) {
  long p = 0;
  long i = 0;
  long star = 0;
  long mark = 0;

  p = 0L;

  i = 0L;

  star = (0L - 1L);

  mark = 0L;

  while ((i < s_len(s))) {

  if (((p < s_len(pat)) && s_eq(s_atc(pat, p, 664), (long)(intptr_t)"*"))) {

  star = p;

  mark = i;

  p = (p + 1L);
  } else {
  if (((p < s_len(pat)) && ({ long _t45 = s_atc(pat, p, 667); long _t46 = s_atc(s, i, 667); s_eq(_t45, _t46); }))) {

  p = (p + 1L);

  i = (i + 1L);
  } else {
  if (((star >= 0L) && (!s_eq(s_atc(s, mark, 670), (long)(intptr_t)"/")))) {

  p = (star + 1L);

  mark = (mark + 1L);

  i = mark;
  } else {

  return false;
  }
  }
  }
  }

  while (((p < s_len(pat)) && s_eq(s_atc(pat, p, 679), (long)(intptr_t)"*"))) {

  p = (p + 1L);
  }

  return (p == s_len(pat));
  return 0;
}
long u_globName(long names, long s) {
  long r = 0;

  r = u_hasName(names, s);

  {
  long _lst = names;
  long _n = l_len(_lst);
  for (long _i = 0; _i < _n; _i = _i + 1) {
  long e = l_at(_lst, _i);

  if ((s_contains(e, (long)(intptr_t)"*") && u_globOk(e, s))) {

  r = true;
  }
  }
  }

  return r;
  return 0;
}
long u_mayName(long s, long names) {
  long r = 0;
  long n = 0;

  r = false;

  {
  long _lst = names;
  long _n = l_len(_lst);
  for (long _i = 0; _i < _n; _i = _i + 1) {
  long e = l_at(_lst, _i);

  if (s_eq(e, s)) {

  r = true;
  }

  if (((s_contains(e, (long)(intptr_t)"*") && u_globOk(e, s)) && (!s_contains(s, (long)(intptr_t)"..")))) {

  r = true;
  }

  if (s_eq(e, (long)(intptr_t)"./")) {

  if ((s_len(s) > 0L)) {

  if (((!s_eq(s_atc(s, 0L, 708), (long)(intptr_t)"/")) && (!s_contains(s, (long)(intptr_t)"..")))) {

  r = true;
  }
  }
  }

  n = s_len(e);

  if ((n > 0L)) {

  if ((s_len(s) >= n)) {

  if (s_eq(s_atc(e, (n - 1L), 716), (long)(intptr_t)"/")) {

  if (s_eq(s_slicec(s, 0L, n, 717), e)) {

  if ((!s_contains(s, (long)(intptr_t)".."))) {

  r = true;
  }
  }
  }
  }
  }
  }
  }

  return r;
  return 0;
}
long u_mayCheck(long arg, long names, long verb, long word, long ctx) {
  long p = 0;
  long lit = 0;

  if ((l_len(names) == 0L)) {

  u_failAt(s_concat(s_concat(s_concat(s_concat((long)(intptr_t)"this program may not ", verb), (long)(intptr_t)" a file; say which at the top, as in: "), word), (long)(intptr_t)" \"notes.txt\""), ctx);
  }

  p = u_whatPath(arg);

  if (s_eq(s_atc(p, 0L, 735), (long)(intptr_t)"\"")) {

  lit = s_slicec(p, 1L, (s_len(p) - 1L), 736);

  if (s_eq(verb, (long)(intptr_t)"list")) {

  if ((s_len(lit) == 0L)) {

  lit = (long)(intptr_t)"/";
  } else {

  if ((!s_eq(s_atc(lit, (s_len(lit) - 1L), 741), (long)(intptr_t)"/"))) {

  lit = s_concat(lit, (long)(intptr_t)"/");
  }
  }
  }

  if ((!u_mayName(lit, names))) {

  u_failAt(s_concat(s_concat(s_concat(s_concat(s_concat(s_concat((long)(intptr_t)"this program may not ", verb), (long)(intptr_t)" "), p), (long)(intptr_t)"; add it to the "), word), (long)(intptr_t)" line at the top"), ctx);
  }
  }

  return 0L;
  return 0;
}
long u_hostOf(long url) {
  long rest = 0;
  long k = 0;
  long c = 0;

  rest = (long)(intptr_t)"";

  if (s_starts(url, (long)(intptr_t)"https://")) {

  rest = s_slicec(url, 8L, s_len(url), 757);
  }

  if (s_starts(url, (long)(intptr_t)"http://")) {

  rest = s_slicec(url, 7L, s_len(url), 760);
  }

  k = 0L;

  while ((k < s_len(rest))) {

  c = s_atc(rest, k, 764);

  if ((((s_eq(c, (long)(intptr_t)"/") || s_eq(c, (long)(intptr_t)":")) || s_eq(c, (long)(intptr_t)"?")) || s_eq(c, (long)(intptr_t)"#"))) {

  return s_slicec(rest, 0L, k, 766);
  }

  k = (k + 1L);
  }

  return rest;
  return 0;
}
long u_runCheck(long arg, long ctx) {
  long p = 0;

  if ((l_len(((R_Sigs*)(intptr_t)((R_Ctx*)(intptr_t)ctx)->sigs)->runs) == 0L)) {

  u_failAt((long)(intptr_t)"this program may not run another program; say which at the top, as in: runs \"git\"", ctx);
  }

  p = u_whatPath(arg);

  if (s_eq(s_atc(p, 0L, 779), (long)(intptr_t)"\"")) {

  if ((!({ long _t20 = ((R_Sigs*)(intptr_t)((R_Ctx*)(intptr_t)ctx)->sigs)->runs; long _t21 = s_slicec(p, 1L, (s_len(p) - 1L), 780); u_hasName(_t20, _t21); }))) {

  u_failAt(s_concat(s_concat((long)(intptr_t)"this program may not run ", p), (long)(intptr_t)"; add it to the runs line at the top"), ctx);
  }
  }

  return 0L;
  return 0;
}
long u_mailCheck(long arg, long ctx) {
  long p = 0;
  long a = 0;
  long ok = 0;

  if ((l_len(((R_Sigs*)(intptr_t)((R_Ctx*)(intptr_t)ctx)->sigs)->mails) == 0L)) {

  u_failAt((long)(intptr_t)"this program may not send mail; say to whom at the top, as in: mails \"me@example.com\"", ctx);
  }

  p = u_whatPath(arg);

  if (s_eq(s_atc(p, 0L, 793), (long)(intptr_t)"\"")) {

  a = s_slicec(p, 1L, (s_len(p) - 1L), 794);

  ok = false;

  {
  long _lst = ((R_Sigs*)(intptr_t)((R_Ctx*)(intptr_t)ctx)->sigs)->mails;
  long _n = l_len(_lst);
  for (long _i = 0; _i < _n; _i = _i + 1) {
  long m = l_at(_lst, _i);

  if (((s_eq(m, a) || (s_starts(m, (long)(intptr_t)"@") && s_ends(a, m))) || (s_contains(m, (long)(intptr_t)"*") && u_globOk(m, a)))) {

  ok = true;
  }
  }
  }

  if ((!ok)) {

  u_failAt(s_concat(s_concat((long)(intptr_t)"this program may not mail ", p), (long)(intptr_t)"; add it to the mails line at the top"), ctx);
  }
  }

  return 0L;
  return 0;
}
long u_fetchCheck(long arg, long ctx) {
  long p = 0;
  long h = 0;

  if ((l_len(((R_Sigs*)(intptr_t)((R_Ctx*)(intptr_t)ctx)->sigs)->fetches) == 0L)) {

  u_failAt((long)(intptr_t)"this program may not fetch from the network; say which hosts at the top, as in: fetches \"example.com\"", ctx);
  }

  p = u_whatPath(arg);

  if (s_eq(s_atc(p, 0L, 813), (long)(intptr_t)"\"")) {

  h = u_hostOf(s_slicec(p, 1L, (s_len(p) - 1L), 814));

  if ((!u_globName(((R_Sigs*)(intptr_t)((R_Ctx*)(intptr_t)ctx)->sigs)->fetches, h))) {

  u_failAt(s_concat(s_concat((long)(intptr_t)"this program may not fetch from \"", h), (long)(intptr_t)"\"; add it to the fetches line at the top"), ctx);
  }
  }

  return 0L;
  return 0;
}
long u_mayText(long names) {
  long out = 0;

  out = (long)(intptr_t)"";

  {
  long _lst = names;
  long _n = l_len(_lst);
  for (long _i = 0; _i < _n; _i = _i + 1) {
  long n = l_at(_lst, _i);

  if ((!s_eq(out, (long)(intptr_t)""))) {

  out = s_concat(out, (long)(intptr_t)", ");
  }

  if (s_eq(n, (long)(intptr_t)"<args>")) {

  out = s_concat(out, (long)(intptr_t)"any file named on the command line");
  } else {

  if ((s_len(n) > 0L)) {

  if (s_eq(s_atc(n, (s_len(n) - 1L), 833), (long)(intptr_t)"/")) {

  out = s_concat(out, (long)(intptr_t)"anything under ");
  }
  }

  out = s_concat(s_concat(s_concat(out, (long)(intptr_t)"\""), n), (long)(intptr_t)"\"");
  }
  }
  }

  return out;
  return 0;
}
long u_whatProg(long prog, long sigs) {
  long ks = 0;
  long as = 0;
  long fs = 0;
  long w = 0;
  long out = 0;
  long muts = 0;

  ks = l_new();

  as = l_new();

  fs = l_new();

  w = What(ks, as, fs);

  {
  long _lst = ((R_Prog*)(intptr_t)prog)->fns;
  long _n = l_len(_lst);
  for (long _i = 0; _i < _n; _i = _i + 1) {
  long f = l_at(_lst, _i);

  ({ long _t91 = ((R_Fn*)(intptr_t)f)->body; long _t92 = ((R_Fn*)(intptr_t)f)->name; u_whatBody(_t91, _t92, w, sigs); });
  }
  }

  out = (long)(intptr_t)"";

  {
  long _lst = l_with(l_with(l_with(l_with(l_with(l_with(l_with(l_with(l_with(l_with(l_new(), (long)(intptr_t)"screen"), (long)(intptr_t)"files"), (long)(intptr_t)"program"), (long)(intptr_t)"net"), (long)(intptr_t)"clock"), (long)(intptr_t)"env"), (long)(intptr_t)"term"), (long)(intptr_t)"mail"), (long)(intptr_t)"stdin"), (long)(intptr_t)"args");
  long _n = l_len(_lst);
  for (long _i = 0; _i < _n; _i = _i + 1) {
  long kind = l_at(_lst, _i);

  out = s_concat(out, u_whatKind(w, kind));
  }
  }

  muts = (long)(intptr_t)"";

  {
  long _lst = ((R_Prog*)(intptr_t)prog)->fns;
  long _n = l_len(_lst);
  for (long _i = 0; _i < _n; _i = _i + 1) {
  long f = l_at(_lst, _i);

  if (u_hasName(((R_Fn*)(intptr_t)f)->effs, (long)(intptr_t)"Mut")) {

  if ((!s_eq(muts, (long)(intptr_t)""))) {

  muts = s_concat(muts, (long)(intptr_t)", ");
  }

  muts = s_concat(muts, s_replace(((R_Fn*)(intptr_t)f)->name, (long)(intptr_t)"__", (long)(intptr_t)"."));
  }
  }
  }

  if ((!s_eq(muts, (long)(intptr_t)""))) {

  out = s_concat(s_concat(s_concat(out, (long)(intptr_t)"lists   changes a list or map it was handed  (in "), muts), (long)(intptr_t)")\n");
  }

  if ((l_len(((R_Prog*)(intptr_t)prog)->reads) > 0L)) {

  out = s_concat(({ long _t111 = s_concat(out, (long)(intptr_t)"may read   "); long _t112 = u_mayText(((R_Prog*)(intptr_t)prog)->reads); s_concat(_t111, _t112); }), (long)(intptr_t)"\n");
  }

  if ((l_len(((R_Prog*)(intptr_t)prog)->writes) > 0L)) {

  out = s_concat(({ long _t115 = s_concat(out, (long)(intptr_t)"may write  "); long _t116 = u_mayText(((R_Prog*)(intptr_t)prog)->writes); s_concat(_t115, _t116); }), (long)(intptr_t)"\n");
  }

  if ((l_len(((R_Prog*)(intptr_t)prog)->runs) > 0L)) {

  out = s_concat(({ long _t119 = s_concat(out, (long)(intptr_t)"may run    "); long _t120 = u_mayText(((R_Prog*)(intptr_t)prog)->runs); s_concat(_t119, _t120); }), (long)(intptr_t)"\n");
  }

  if ((l_len(((R_Prog*)(intptr_t)prog)->fetches) > 0L)) {

  out = s_concat(({ long _t123 = s_concat(out, (long)(intptr_t)"may fetch  "); long _t124 = u_mayText(((R_Prog*)(intptr_t)prog)->fetches); s_concat(_t123, _t124); }), (long)(intptr_t)"\n");
  }

  if ((l_len(((R_Prog*)(intptr_t)prog)->mails) > 0L)) {

  out = s_concat(({ long _t127 = s_concat(out, (long)(intptr_t)"may mail   "); long _t128 = u_mayText(((R_Prog*)(intptr_t)prog)->mails); s_concat(_t127, _t128); }), (long)(intptr_t)"\n");
  }

  if (s_contains(out, (long)(intptr_t)"term    takes")) {

  out = s_replace(out, (long)(intptr_t)"screen  not used\n", (long)(intptr_t)"screen  prints no lines; term draws on it\n");
  }

  return s_concat(out, (long)(intptr_t)"nothing else: a program reaches the world only through what is listed here");
  return 0;
}
long u_whatKind(long w, long kind) {
  long out = 0;
  long done = 0;
  long k = 0;
  long act = 0;
  long who = 0;
  long j = 0;

  out = (long)(intptr_t)"";

  done = l_new();

  k = 0L;

  while ((k < l_len(((R_What*)(intptr_t)w)->acts))) {

  act = l_atc(((R_What*)(intptr_t)w)->acts, k, 895);

  if ((s_eq(l_atc(((R_What*)(intptr_t)w)->kinds, k, 896), kind) && (!u_hasName(done, act)))) {

  l_push(done, act);

  who = (long)(intptr_t)"";

  j = k;

  while ((j < l_len(((R_What*)(intptr_t)w)->acts))) {

  if ((s_eq(l_atc(((R_What*)(intptr_t)w)->kinds, j, 901), kind) && s_eq(l_atc(((R_What*)(intptr_t)w)->acts, j, 901), act))) {

  if ((!s_eq(who, (long)(intptr_t)""))) {

  who = s_concat(who, (long)(intptr_t)", ");
  }

  who = s_concat(who, s_replace(l_atc(((R_What*)(intptr_t)w)->fns, j, 905), (long)(intptr_t)"__", (long)(intptr_t)"."));
  }

  j = (j + 1L);
  }

  out = s_concat(s_concat(s_concat(s_concat(s_concat(out, s_padright(kind, 8L)), act), (long)(intptr_t)"  (in "), who), (long)(intptr_t)")\n");
  }

  k = (k + 1L);
  }

  if (s_eq(out, (long)(intptr_t)"")) {

  out = s_concat(s_padright(kind, 8L), (long)(intptr_t)"not used\n");
  }

  return out;
  return 0;
}
long u_whatAdd(long w, long kind, long act, long fn) {
  long k = 0;

  k = 0L;

  while ((k < l_len(((R_What*)(intptr_t)w)->acts))) {

  if (((s_eq(l_atc(((R_What*)(intptr_t)w)->kinds, k, 922), kind) && s_eq(l_atc(((R_What*)(intptr_t)w)->acts, k, 922), act)) && s_eq(l_atc(((R_What*)(intptr_t)w)->fns, k, 922), fn))) {

  return 0L;
  }

  k = (k + 1L);
  }

  l_push(((R_What*)(intptr_t)w)->kinds, kind);

  l_push(((R_What*)(intptr_t)w)->acts, act);

  l_push(((R_What*)(intptr_t)w)->fns, fn);

  return 0L;
  return 0;
}
long u_whatBody(long body, long fn, long w, long sigs) {

  {
  long _lst = body;
  long _n = l_len(_lst);
  for (long _i = 0; _i < _n; _i = _i + 1) {
  long s = l_at(_lst, _i);

  u_whatStmt(s, fn, w, sigs);
  }
  }

  return 0L;
  return 0;
}
long u_whatStmt(long s, long fn, long w, long sigs) {

  switch (((Obj*)(intptr_t)s)->tag) {
  case T_Let: { long name = ((Obj*)(intptr_t)s)->v0; long ty = ((Obj*)(intptr_t)s)->v1; long e = ((Obj*)(intptr_t)s)->v2; return u_whatExpr(e, fn, w, sigs); }
  case T_Bare: { long e = ((Obj*)(intptr_t)s)->v0; return u_whatExpr(e, fn, w, sigs); }
  case T_If: { long c = ((Obj*)(intptr_t)s)->v0; long t = ((Obj*)(intptr_t)s)->v1; long el = ((Obj*)(intptr_t)s)->v2; return ({ long _t156 = ({ long _t154 = u_whatExpr(c, fn, w, sigs); long _t155 = u_whatBody(t, fn, w, sigs); (_t154 + _t155); }); long _t157 = u_whatBody(el, fn, w, sigs); (_t156 + _t157); }); }
  case T_While: { long c = ((Obj*)(intptr_t)s)->v0; long b = ((Obj*)(intptr_t)s)->v1; return ({ long _t163 = u_whatExpr(c, fn, w, sigs); long _t164 = u_whatBody(b, fn, w, sigs); (_t163 + _t164); }); }
  case T_Ret: { long e = ((Obj*)(intptr_t)s)->v0; return u_whatExpr(e, fn, w, sigs); }
  case T_Line: { long n = ((Obj*)(intptr_t)s)->v0; return 0L; }
  case T_Jump: { long w = ((Obj*)(intptr_t)s)->v0; return 0L; }
  }
  return 0;
  return 0;
}
long u_whatExpr(long e, long fn, long w, long sigs) {

  switch (((Obj*)(intptr_t)e)->tag) {
  case T_Num: { long v = ((Obj*)(intptr_t)e)->v0; return 0L; }
  case T_FloatLit: { long s = ((Obj*)(intptr_t)e)->v0; return 0L; }
  case T_Var: { long s = ((Obj*)(intptr_t)e)->v0; return 0L; }
  case T_StrLit: { long s = ((Obj*)(intptr_t)e)->v0; return 0L; }
  case T_ListLit: { long es = ((Obj*)(intptr_t)e)->v0; return u_whatAll(es, fn, w, sigs); }
  case T_Bin: { long op = ((Obj*)(intptr_t)e)->v0; long a = ((Obj*)(intptr_t)e)->v1; long b = ((Obj*)(intptr_t)e)->v2; return ({ long _t239 = u_whatExpr(a, fn, w, sigs); long _t240 = u_whatExpr(b, fn, w, sigs); (_t239 + _t240); }); }
  case T_Call: { long name = ((Obj*)(intptr_t)e)->v0; long args = ((Obj*)(intptr_t)e)->v1; return u_whatAll(args, fn, w, sigs); }
  case T_Match: { long scrut = ((Obj*)(intptr_t)e)->v0; long arms = ((Obj*)(intptr_t)e)->v1; return ({ long _t250 = u_whatExpr(scrut, fn, w, sigs); long _t251 = u_whatArms(arms, fn, w, sigs); (_t250 + _t251); }); }
  case T_Field: { long recv = ((Obj*)(intptr_t)e)->v0; long fld = ((Obj*)(intptr_t)e)->v1; return u_whatField(recv, fld, fn, w, sigs); }
  case T_Method: { long recv = ((Obj*)(intptr_t)e)->v0; long name = ((Obj*)(intptr_t)e)->v1; long args = ((Obj*)(intptr_t)e)->v2; return u_whatMethod(recv, name, args, fn, w, sigs); }
  case T_Each: { long recv = ((Obj*)(intptr_t)e)->v0; long param = ((Obj*)(intptr_t)e)->v1; long body = ((Obj*)(intptr_t)e)->v2; return ({ long _t267 = u_whatExpr(recv, fn, w, sigs); long _t268 = u_whatBody(body, fn, w, sigs); (_t267 + _t268); }); }
  }
  return 0;
  return 0;
}
long u_whatAll(long es, long fn, long w, long sigs) {

  {
  long _lst = es;
  long _n = l_len(_lst);
  for (long _i = 0; _i < _n; _i = _i + 1) {
  long x = l_at(_lst, _i);

  u_whatExpr(x, fn, w, sigs);
  }
  }

  return 0L;
  return 0;
}
long u_whatArms(long arms, long fn, long w, long sigs) {

  {
  long _lst = arms;
  long _n = l_len(_lst);
  for (long _i = 0; _i < _n; _i = _i + 1) {
  long a = l_at(_lst, _i);

  u_whatExpr(((R_Arm*)(intptr_t)a)->body, fn, w, sigs);
  }
  }

  return 0L;
  return 0;
}
long u_whatField(long recv, long fld, long fn, long w, long sigs) {

  if (s_eq(fld, (long)(intptr_t)"stdin")) {

  u_whatAdd(w, (long)(intptr_t)"stdin", (long)(intptr_t)"reads all of standard input", fn);
  }

  if (s_eq(fld, (long)(intptr_t)"args")) {

  u_whatAdd(w, (long)(intptr_t)"args", (long)(intptr_t)"reads the command line", fn);
  }

  if (s_eq(fld, (long)(intptr_t)"self")) {

  u_whatAdd(w, (long)(intptr_t)"env", (long)(intptr_t)"reads its own path", fn);
  }

  if ((s_eq(fld, (long)(intptr_t)"now") || s_eq(fld, (long)(intptr_t)"millis"))) {

  u_whatAdd(w, (long)(intptr_t)"clock", (long)(intptr_t)"reads the time", fn);
  }

  if (s_eq(fld, (long)(intptr_t)"term")) {

  u_whatAdd(w, (long)(intptr_t)"term", (long)(intptr_t)"takes the whole terminal and reads keys", fn);
  }

  return u_whatExpr(recv, fn, w, sigs);
  return 0;
}
long u_whatMethod(long recv, long name, long args, long fn, long w, long sigs) {

  if ((l_len(args) > 0L)) {

  if (((s_eq(name, (long)(intptr_t)"print") || s_eq(name, (long)(intptr_t)"part")) || s_eq(name, (long)(intptr_t)"error"))) {

  u_whatAdd(w, (long)(intptr_t)"screen", (long)(intptr_t)"prints", fn);
  }

  if (s_eq(name, (long)(intptr_t)"read")) {

  u_whatAdd(w, (long)(intptr_t)"files", s_concat((long)(intptr_t)"reads ", u_whatPath(l_atc(args, 0L, 1007))), fn);
  }

  if (s_eq(name, (long)(intptr_t)"write")) {

  u_whatAdd(w, (long)(intptr_t)"files", s_concat((long)(intptr_t)"writes ", u_whatPath(l_atc(args, 0L, 1010))), fn);
  }

  if (s_eq(name, (long)(intptr_t)"readBytes")) {

  u_whatAdd(w, (long)(intptr_t)"files", s_concat((long)(intptr_t)"reads the bytes of ", u_whatPath(l_atc(args, 0L, 1013))), fn);
  }

  if (s_eq(name, (long)(intptr_t)"writeBytes")) {

  u_whatAdd(w, (long)(intptr_t)"files", s_concat((long)(intptr_t)"writes bytes to ", u_whatPath(l_atc(args, 0L, 1016))), fn);
  }

  if (s_eq(name, (long)(intptr_t)"append")) {

  u_whatAdd(w, (long)(intptr_t)"files", s_concat((long)(intptr_t)"adds to ", u_whatPath(l_atc(args, 0L, 1019))), fn);
  }

  if (s_eq(name, (long)(intptr_t)"list")) {

  u_whatAdd(w, (long)(intptr_t)"files", s_concat((long)(intptr_t)"lists the folder ", u_whatPath(l_atc(args, 0L, 1022))), fn);
  }

  if (s_eq(name, (long)(intptr_t)"exists")) {

  u_whatAdd(w, (long)(intptr_t)"files", s_concat((long)(intptr_t)"looks for ", u_whatPath(l_atc(args, 0L, 1025))), fn);
  }

  if (s_eq(name, (long)(intptr_t)"rename")) {

  u_whatAdd(w, (long)(intptr_t)"files", s_concat((long)(intptr_t)"renames ", u_whatPath(l_atc(args, 0L, 1028))), fn);
  }

  if (s_eq(name, (long)(intptr_t)"delete")) {

  u_whatAdd(w, (long)(intptr_t)"files", s_concat((long)(intptr_t)"deletes ", u_whatPath(l_atc(args, 0L, 1031))), fn);
  }

  if (s_eq(name, (long)(intptr_t)"send")) {

  u_whatAdd(w, (long)(intptr_t)"mail", s_concat((long)(intptr_t)"sends to ", s_replace(u_whatPath(l_atc(args, 0L, 1034)), (long)(intptr_t)"a path", (long)(intptr_t)"an address")), fn);
  }

  if (((((s_eq(name, (long)(intptr_t)"run") || s_eq(name, (long)(intptr_t)"status")) || s_eq(name, (long)(intptr_t)"start")) || s_eq(name, (long)(intptr_t)"try")) || s_eq(name, (long)(intptr_t)"feed"))) {

  u_whatAdd(w, (long)(intptr_t)"program", s_concat((long)(intptr_t)"runs ", u_whatPath(l_atc(args, 0L, 1037))), fn);
  }

  if (((s_eq(name, (long)(intptr_t)"fetch") || s_eq(name, (long)(intptr_t)"reach")) || s_eq(name, (long)(intptr_t)"fetchWith"))) {

  u_whatAdd(w, (long)(intptr_t)"net", s_concat((long)(intptr_t)"fetches ", u_whatPath(l_atc(args, 0L, 1040))), fn);
  }

  if (s_eq(name, (long)(intptr_t)"post")) {

  u_whatAdd(w, (long)(intptr_t)"net", s_concat((long)(intptr_t)"sends to ", u_whatPath(l_atc(args, 0L, 1043))), fn);
  }

  if (s_eq(name, (long)(intptr_t)"var")) {

  u_whatAdd(w, (long)(intptr_t)"env", s_concat((long)(intptr_t)"reads the variable ", u_whatPath(l_atc(args, 0L, 1046))), fn);
  }

  if (s_eq(name, (long)(intptr_t)"setVar")) {

  u_whatAdd(w, (long)(intptr_t)"env", s_concat(s_concat((long)(intptr_t)"sets the variable ", u_whatPath(l_atc(args, 0L, 1049))), (long)(intptr_t)" for itself and the programs it runs"), fn);
  }
  }

  u_whatExpr(recv, fn, w, sigs);

  return u_whatAll(args, fn, w, sigs);
  return 0;
}
long u_whatPath(long e) {

  switch (((Obj*)(intptr_t)e)->tag) {
  case T_StrLit: { long s = ((Obj*)(intptr_t)e)->v0; return s_concat(s_concat((long)(intptr_t)"\"", s), (long)(intptr_t)"\""); }
  case T_Num: { long v = ((Obj*)(intptr_t)e)->v0; return (long)(intptr_t)"a path made while running"; }
  case T_FloatLit: { long s = ((Obj*)(intptr_t)e)->v0; return (long)(intptr_t)"a path made while running"; }
  case T_Var: { long s = ((Obj*)(intptr_t)e)->v0; return (long)(intptr_t)"a path made while running"; }
  case T_ListLit: { long es = ((Obj*)(intptr_t)e)->v0; return (long)(intptr_t)"a path made while running"; }
  case T_Bin: { long op = ((Obj*)(intptr_t)e)->v0; long a = ((Obj*)(intptr_t)e)->v1; long b = ((Obj*)(intptr_t)e)->v2; return (long)(intptr_t)"a path made while running"; }
  case T_Call: { long name = ((Obj*)(intptr_t)e)->v0; long args = ((Obj*)(intptr_t)e)->v1; return (long)(intptr_t)"a path made while running"; }
  case T_Match: { long scrut = ((Obj*)(intptr_t)e)->v0; long arms = ((Obj*)(intptr_t)e)->v1; return (long)(intptr_t)"a path made while running"; }
  case T_Field: { long recv = ((Obj*)(intptr_t)e)->v0; long fld = ((Obj*)(intptr_t)e)->v1; return (long)(intptr_t)"a path made while running"; }
  case T_Method: { long recv = ((Obj*)(intptr_t)e)->v0; long name = ((Obj*)(intptr_t)e)->v1; long args = ((Obj*)(intptr_t)e)->v2; return (long)(intptr_t)"a path made while running"; }
  case T_Each: { long recv = ((Obj*)(intptr_t)e)->v0; long param = ((Obj*)(intptr_t)e)->v1; long body = ((Obj*)(intptr_t)e)->v2; return (long)(intptr_t)"a path made while running"; }
  }
  return 0;
  return 0;
}
long u_isTest(long name) {

  return ((s_len(name) > 5L) && s_eq(s_slicec(name, 0L, 5L, 1074), (long)(intptr_t)"test_"));
  return 0;
}
long u_emitTests(long fns) {
  long out = 0;
  long c = 0;

  out = (long)(intptr_t)"int main(void) {\n  long failures = 0;\n  long total = 0;";

  {
  long _lst = fns;
  long _n = l_len(_lst);
  for (long _i = 0; _i < _n; _i = _i + 1) {
  long f = l_at(_lst, _i);

  if (u_isTest(((R_Fn*)(intptr_t)f)->name)) {

  c = u_mangle(((R_Fn*)(intptr_t)f)->name);

  out = s_concat(s_concat(s_concat(out, (long)(intptr_t)"\n  simpler_failed = 0; total = total + 1; "), c), (long)(intptr_t)"();");

  out = s_concat(({ long _t36 = s_concat(({ long _t34 = s_concat(out, (long)(intptr_t)"\n  if (simpler_failed) { printf(\"FAIL "); long _t35 = ((R_Fn*)(intptr_t)f)->name; s_concat(_t34, _t35); }), (long)(intptr_t)"\\n\"); failures = failures + 1; } else { printf(\"PASS "); long _t37 = ((R_Fn*)(intptr_t)f)->name; s_concat(_t36, _t37); }), (long)(intptr_t)"\\n\"); }");
  }
  }
  }

  return s_concat(out, (long)(intptr_t)"\n  printf(\"\\n%ld/%ld passed\\n\", total - failures, total);\n  return failures ? 1 : 0;\n}");
  return 0;
}
long u_buildSigs(long prog) {
  long recNames = 0;
  long fnNames = 0;
  long fnRets = 0;
  long boxedNullary = 0;
  long retP = 0;
  long mutP = 0;

  recNames = l_new();

  {
  long _lst = ((R_Prog*)(intptr_t)prog)->records;
  long _n = l_len(_lst);
  for (long _i = 0; _i < _n; _i = _i + 1) {
  long r = l_at(_lst, _i);

  l_push(recNames, ((R_RecDef*)(intptr_t)r)->name);
  }
  }

  fnNames = l_new();

  fnRets = l_new();

  {
  long _lst = ((R_Prog*)(intptr_t)prog)->fns;
  long _n = l_len(_lst);
  for (long _i = 0; _i < _n; _i = _i + 1) {
  long f = l_at(_lst, _i);

  l_push(fnNames, ((R_Fn*)(intptr_t)f)->name);

  l_push(fnRets, ((R_Fn*)(intptr_t)f)->ret);
  }
  }

  boxedNullary = l_new();

  retP = l_new();

  mutP = l_new();

  {
  long _lst = ((R_Prog*)(intptr_t)prog)->types;
  long _n = l_len(_lst);
  for (long _i = 0; _i < _n; _i = _i + 1) {
  long t = l_at(_lst, _i);

  if (u_hasPayload(t)) {

  {
  long _lst = ((R_TyDef*)(intptr_t)t)->cases;
  long _n = l_len(_lst);
  for (long _i = 0; _i < _n; _i = _i + 1) {
  long c = l_at(_lst, _i);

  if ((((R_Case*)(intptr_t)c)->arity == 0L)) {

  l_push(boxedNullary, ((R_Case*)(intptr_t)c)->cname);
  }
  }
  }
  }
  }
  }

  return ({ long _t105 = ((R_Prog*)(intptr_t)prog)->records; long _t106 = ((R_Prog*)(intptr_t)prog)->types; long _t107 = ((R_Prog*)(intptr_t)prog)->fns; long _t108 = ((R_Prog*)(intptr_t)prog)->reads; long _t109 = ((R_Prog*)(intptr_t)prog)->writes; long _t110 = ((R_Prog*)(intptr_t)prog)->runs; long _t111 = ((R_Prog*)(intptr_t)prog)->fetches; long _t112 = ((R_Prog*)(intptr_t)prog)->mails; Sigs(recNames, _t105, _t106, _t107, fnNames, fnRets, boxedNullary, retP, mutP, _t108, _t109, _t110, _t111, _t112, ((R_Prog*)(intptr_t)prog)->files); });
  return 0;
}
long u_spot(long files, long line) {
  long m = 0;
  long name = 0;

  m = i_div(line, 1000000L, 1125);

  name = (long)(intptr_t)"input.smplr";

  if ((m < l_len(files))) {

  name = l_atc(files, m, 1128);
  }

  return s_concat(({ long _t21 = s_concat(name, (long)(intptr_t)":"); long _t22 = i_tostr(i_mod(line, 1000000L, 1130)); s_concat(_t21, _t22); }), (long)(intptr_t)": ");
  return 0;
}
long u_hasMod(long mods, long name) {
  long found = 0;

  found = false;

  {
  long _lst = mods;
  long _n = l_len(_lst);
  for (long _i = 0; _i < _n; _i = _i + 1) {
  long m = l_at(_lst, _i);

  if (s_eq(((R_Mod*)(intptr_t)m)->name, name)) {

  found = true;
  }
  }
  }

  return found;
  return 0;
}
long u_modNameOk(long name) {
  long ok = 0;
  long k = 0;

  ok = (((s_len(name) > 0L) && (!s_eq(name, (long)(intptr_t)"input"))) && (!s_contains(name, (long)(intptr_t)"__")));

  k = 0L;

  while ((k < s_len(name))) {

  if ((!u_isAlnum(s_code(s_atc(name, k, 1147))))) {

  ok = false;
  }

  k = (k + 1L);
  }

  return ok;
  return 0;
}
long u_takeUses(long lx, long mine, long queue, long queueLines) {
  long toks = 0;
  long lines = 0;
  long depth = 0;
  long i = 0;
  long n = 0;
  long name = 0;

  toks = l_new();

  lines = l_new();

  depth = 0L;

  i = 0L;

  n = l_len(((R_Lexed*)(intptr_t)lx)->toks);

  while ((i < n)) {

  if (u_isPunct(lx, i, (long)(intptr_t)"{")) {

  depth = (depth + 1L);
  }

  if (u_isPunct(lx, i, (long)(intptr_t)"}")) {

  depth = (depth - 1L);
  }

  if ((((depth == 0L) && u_isWord(lx, i, (long)(intptr_t)"uses")) && u_isStrTok(lx, (i + 1L)))) {

  name = u_strAt(lx, (i + 1L));

  if ((!u_modNameOk(name))) {

  u_failLine(lx, (i + 1L), (long)(intptr_t)"a module name has only letters, digits and _");
  }

  l_push(mine, name);

  l_push(queue, name);

  l_push(queueLines, l_atc(((R_Lexed*)(intptr_t)lx)->lines, i, 1177));

  i = (i + 2L);
  } else {

  l_push(toks, l_atc(((R_Lexed*)(intptr_t)lx)->toks, i, 1180));

  l_push(lines, l_atc(((R_Lexed*)(intptr_t)lx)->lines, i, 1181));

  i = (i + 1L);
  }
  }

  return Lexed(toks, lines, ((R_Lexed*)(intptr_t)lx)->files);
  return 0;
}
long u_qualify(long lx, long mine) {
  long toks = 0;
  long lines = 0;
  long i = 0;
  long n = 0;
  long id = 0;
  long afterDot = 0;
  long named = 0;

  toks = l_new();

  lines = l_new();

  i = 0L;

  n = l_len(((R_Lexed*)(intptr_t)lx)->toks);

  while ((i < n)) {

  id = (long)(intptr_t)"";

  if (u_isIdent(lx, i)) {

  id = u_identAt(lx, i);
  }

  afterDot = false;

  if ((i > 0L)) {

  afterDot = u_isPunct(lx, (i - 1L), (long)(intptr_t)".");
  }

  if ((((!s_eq(id, (long)(intptr_t)"")) && (!afterDot)) && u_hasName(mine, id))) {

  named = false;

  if (u_isPunct(lx, (i + 1L), (long)(intptr_t)".")) {

  named = u_isIdent(lx, (i + 2L));
  }

  if ((!named)) {

  u_failLine(lx, i, s_concat(s_concat(s_concat(id, (long)(intptr_t)" is a module here: write "), id), (long)(intptr_t)".name for something in it, and give a variable another name"));
  }

  l_push(toks, Ident(({ long _t62 = s_concat(id, (long)(intptr_t)"__"); long _t63 = u_identAt(lx, (i + 2L)); s_concat(_t62, _t63); })));

  l_push(lines, l_atc(((R_Lexed*)(intptr_t)lx)->lines, i, 1212));

  i = (i + 3L);
  } else {

  l_push(toks, l_atc(((R_Lexed*)(intptr_t)lx)->toks, i, 1215));

  l_push(lines, l_atc(((R_Lexed*)(intptr_t)lx)->lines, i, 1216));

  i = (i + 1L);
  }
  }

  return Lexed(toks, lines, ((R_Lexed*)(intptr_t)lx)->files);
  return 0;
}
long u_makeMod(long name, long src, long files, long queue, long queueLines) {
  long mine = 0;
  long lx = 0;
  long p = 0;
  long first = 0;
  long names = 0;
  long fields = 0;

  mine = l_new();

  lx = u_qualify(u_takeUses(u_lex(src, files), mine, queue, queueLines), mine);

  p = u_parse(lx);

  first = u_spot(files, (((l_len(files) - 1L) * 1000000L) + 1L));

  if ((({ long _t120 = ({ long _t118 = ({ long _t116 = ({ long _t114 = l_len(((R_Prog*)(intptr_t)p)->reads); long _t115 = l_len(((R_Prog*)(intptr_t)p)->writes); (_t114 + _t115); }); long _t117 = l_len(((R_Prog*)(intptr_t)p)->runs); (_t116 + _t117); }); long _t119 = l_len(((R_Prog*)(intptr_t)p)->fetches); (_t118 + _t119); }); long _t121 = l_len(((R_Prog*)(intptr_t)p)->mails); (_t120 + _t121); }) > 0L)) {

  fail(s_concat(first, (long)(intptr_t)"a module has no reads, writes, runs, fetches or mails line; those belong to the program that uses it"));
  }

  names = l_new();

  fields = l_new();

  {
  long _lst = ((R_Prog*)(intptr_t)p)->records;
  long _n = l_len(_lst);
  for (long _i = 0; _i < _n; _i = _i + 1) {
  long r = l_at(_lst, _i);

  l_push(names, ((R_RecDef*)(intptr_t)r)->name);

  {
  long _lst = ((R_RecDef*)(intptr_t)r)->fields;
  long _n = l_len(_lst);
  for (long _i = 0; _i < _n; _i = _i + 1) {
  long f = l_at(_lst, _i);

  l_push(fields, f);
  }
  }
  }
  }

  {
  long _lst = ((R_Prog*)(intptr_t)p)->types;
  long _n = l_len(_lst);
  for (long _i = 0; _i < _n; _i = _i + 1) {
  long t = l_at(_lst, _i);

  l_push(names, ((R_TyDef*)(intptr_t)t)->name);

  {
  long _lst = ((R_TyDef*)(intptr_t)t)->cases;
  long _n = l_len(_lst);
  for (long _i = 0; _i < _n; _i = _i + 1) {
  long c = l_at(_lst, _i);

  l_push(names, ((R_Case*)(intptr_t)c)->cname);
  }
  }
  }
  }

  {
  long _lst = ((R_Prog*)(intptr_t)p)->fns;
  long _n = l_len(_lst);
  for (long _i = 0; _i < _n; _i = _i + 1) {
  long f = l_at(_lst, _i);

  if (s_eq(((R_Fn*)(intptr_t)f)->name, (long)(intptr_t)"main")) {

  fail(s_concat(u_spot(files, ((R_Fn*)(intptr_t)f)->line), (long)(intptr_t)"a module has no main; main belongs to the program that uses it"));
  }

  l_push(names, ((R_Fn*)(intptr_t)f)->name);
  }
  }

  {
  long _lst = names;
  long _n = l_len(_lst);
  for (long _i = 0; _i < _n; _i = _i + 1) {
  long nm = l_at(_lst, _i);

  if (u_hasName(fields, nm)) {

  fail(s_concat(s_concat(first, (long)(intptr_t)"in a module, a function or type may not have the name of a field: "), nm));
  }

  if ((((((s_eq(nm, (long)(intptr_t)"Str") || s_eq(nm, (long)(intptr_t)"Int")) || s_eq(nm, (long)(intptr_t)"Float")) || s_eq(nm, (long)(intptr_t)"Bool")) || s_eq(nm, (long)(intptr_t)"List")) || s_eq(nm, (long)(intptr_t)"Map"))) {

  fail(s_concat(s_concat(first, (long)(intptr_t)"in a module, nothing may have the name of a built-in type: "), nm));
  }
  }
  }

  return Mod(name, lx, names);
  return 0;
}
long u_joinMods(long mods, long mainLx) {
  long toks = 0;
  long lines = 0;
  long lx = 0;
  long pre = 0;
  long i = 0;
  long n = 0;
  long id = 0;
  long afterDot = 0;

  toks = l_new();

  lines = l_new();

  {
  long _lst = mods;
  long _n = l_len(_lst);
  for (long _i = 0; _i < _n; _i = _i + 1) {
  long m = l_at(_lst, _i);

  lx = ((R_Mod*)(intptr_t)m)->lx;

  pre = s_concat(((R_Mod*)(intptr_t)m)->name, (long)(intptr_t)"__");

  i = 0L;

  n = (l_len(((R_Lexed*)(intptr_t)lx)->toks) - 1L);

  while ((i < n)) {

  id = (long)(intptr_t)"";

  if (u_isIdent(lx, i)) {

  id = u_identAt(lx, i);
  }

  afterDot = false;

  if ((i > 0L)) {

  afterDot = u_isPunct(lx, (i - 1L), (long)(intptr_t)".");
  }

  if ((((!s_eq(id, (long)(intptr_t)"")) && (!afterDot)) && u_hasName(((R_Mod*)(intptr_t)m)->names, id))) {

  l_push(toks, Ident(s_concat(pre, id)));
  } else {

  l_push(toks, l_atc(((R_Lexed*)(intptr_t)lx)->toks, i, 1285));
  }

  l_push(lines, l_atc(((R_Lexed*)(intptr_t)lx)->lines, i, 1287));

  i = (i + 1L);
  }
  }
  }

  i = 0L;

  while ((i < l_len(((R_Lexed*)(intptr_t)mainLx)->toks))) {

  l_push(toks, l_atc(((R_Lexed*)(intptr_t)mainLx)->toks, i, 1293));

  l_push(lines, l_atc(((R_Lexed*)(intptr_t)mainLx)->lines, i, 1294));

  i = (i + 1L);
  }

  return Lexed(toks, lines, ((R_Lexed*)(intptr_t)mainLx)->files);
  return 0;
}
long u_parse(long toks) {
  long types = 0;
  long records = 0;
  long fns = 0;
  long seen = 0;
  long i = 0;
  long reads = 0;
  long writes = 0;
  long runs = 0;
  long fetches = 0;
  long mails = 0;
  long pr = 0;
  long pt = 0;
  long pf = 0;

  types = l_new();

  records = l_new();

  fns = l_new();

  seen = l_new();

  i = 0L;

  reads = l_new();

  writes = l_new();

  runs = l_new();

  fetches = l_new();

  mails = l_new();

  while (u_notEof(toks, i)) {

  if (u_isMayLine(toks, i)) {

  i = u_parseMay(toks, i, reads, writes, runs, fetches, mails);
  } else {

  if (u_isIdent(toks, i)) {

  u_once(seen, u_identAt(toks, i), toks, i);
  }

  if (u_isTypeDef(toks, i)) {

  if ((!u_isWord(toks, (i + 2L), (long)(intptr_t)"type"))) {

  u_failTok(toks, (i + 2L), (long)(intptr_t)"expected the word type");
  }

  u_expect(toks, (i + 3L), (long)(intptr_t)"{");

  if (u_isRecordDef(toks, i)) {

  pr = u_parseRecord(toks, i);

  l_push(records, ((R_PRec*)(intptr_t)pr)->rec);

  i = ((R_PRec*)(intptr_t)pr)->next;
  } else {

  pt = u_parseTypeDef(toks, i, seen);

  l_push(types, ((R_PTy*)(intptr_t)pt)->ty);

  i = ((R_PTy*)(intptr_t)pt)->next;
  }
  } else {

  if ((!(u_isIdent(toks, i) && u_isPunct(toks, (i + 1L), (long)(intptr_t)"(")))) {

  u_failTok(toks, i, (long)(intptr_t)"expected a function or a type");
  }

  pf = u_parseFn(toks, i);

  l_push(fns, ((R_PFn*)(intptr_t)pf)->fn);

  i = ((R_PFn*)(intptr_t)pf)->next;
  }
  }
  }

  return Prog(types, records, fns, reads, writes, runs, fetches, mails, ((R_Lexed*)(intptr_t)toks)->files);
  return 0;
}
long u_tokAt(long toks, long i) {
  long r = 0;

  r = Eof();

  if ((i < l_len(((R_Lexed*)(intptr_t)toks)->toks))) {

  r = l_atc(((R_Lexed*)(intptr_t)toks)->toks, i, 1350);
  }

  return r;
  return 0;
}
long u_lineOf(long toks, long i) {
  long r = 0;

  r = ({ long _t13 = ((R_Lexed*)(intptr_t)toks)->lines; long _t14 = (l_len(((R_Lexed*)(intptr_t)toks)->lines) - 1L); l_atc(_t13, _t14, 1356); });

  if ((i < l_len(((R_Lexed*)(intptr_t)toks)->lines))) {

  r = l_atc(((R_Lexed*)(intptr_t)toks)->lines, i, 1358);
  }

  return r;
  return 0;
}
long u_show(long toks, long i) {

  switch (((Obj*)(intptr_t)u_tokAt(toks, i))->tag) {
  case T_Ident: { long s = ((Obj*)(intptr_t)u_tokAt(toks, i))->v0; return s; }
  case T_Int: { long v = ((Obj*)(intptr_t)u_tokAt(toks, i))->v0; return i_tostr(v); }
  case T_Float: { long f = ((Obj*)(intptr_t)u_tokAt(toks, i))->v0; return f; }
  case T_Str: { long s = ((Obj*)(intptr_t)u_tokAt(toks, i))->v0; return (long)(intptr_t)"a text"; }
  case T_Punct: { long p = ((Obj*)(intptr_t)u_tokAt(toks, i))->v0; return u_showPunct(p); }
  case T_Eof: { return (long)(intptr_t)"the end of the file"; }
  }
  return 0;
  return 0;
}
long u_showPunct(long p) {
  long r = 0;

  r = p;

  if (s_eq(p, (long)(intptr_t)"s(")) {

  r = (long)(intptr_t)"(";
  }

  return r;
  return 0;
}
long u_failLine(long toks, long i, long msg) {

  return fail(({ long _t11 = u_spot(((R_Lexed*)(intptr_t)toks)->files, u_lineOf(toks, i)); long _t12 = s_replace(msg, (long)(intptr_t)"__", (long)(intptr_t)"."); s_concat(_t11, _t12); }));
  return 0;
}
long u_failTok(long toks, long i, long msg) {

  return u_failLine(toks, i, ({ long _t11 = s_concat(msg, (long)(intptr_t)", found "); long _t12 = u_show(toks, i); s_concat(_t11, _t12); }));
  return 0;
}
long u_once(long seen, long name, long toks, long i) {

  if (u_hasName(seen, name)) {

  u_failLine(toks, i, s_concat(s_concat((long)(intptr_t)"the name ", name), (long)(intptr_t)" is used twice"));
  }

  l_push(seen, name);

  return name;
  return 0;
}
long u_expect(long toks, long i, long p) {

  if ((!u_isPunct(toks, i, p))) {

  u_failTok(toks, i, s_concat((long)(intptr_t)"expected ", p));
  }

  return (i + 1L);
  return 0;
}
long u_isKeyword(long toks, long i) {

  return ((((((u_isWord(toks, i, (long)(intptr_t)"if") || u_isWord(toks, i, (long)(intptr_t)"else")) || u_isWord(toks, i, (long)(intptr_t)"while")) || u_isWord(toks, i, (long)(intptr_t)"return")) || u_isWord(toks, i, (long)(intptr_t)"for")) || u_isWord(toks, i, (long)(intptr_t)"break")) || u_isWord(toks, i, (long)(intptr_t)"continue"));
  return 0;
}
long u_expectIdent(long toks, long i, long what) {

  if (((!u_isIdent(toks, i)) || u_isKeyword(toks, i))) {

  u_failTok(toks, i, s_concat((long)(intptr_t)"expected ", what));
  }

  return u_identAt(toks, i);
  return 0;
}
long u_sep(long toks, long j, long close) {
  long r = 0;

  r = j;

  if (u_isPunct(toks, j, (long)(intptr_t)",")) {

  r = (j + 1L);
  } else {

  if ((!u_isPunct(toks, j, close))) {

  u_failTok(toks, j, s_concat((long)(intptr_t)"expected , or ", close));
  }
  }

  return r;
  return 0;
}
long u_parseType(long toks, long i) {
  long ty = 0;
  long k = 0;
  long inner = 0;
  long more = 0;

  ty = u_expectIdent(toks, i, (long)(intptr_t)"a type");

  k = (i + 1L);

  if (u_isPunct(toks, k, (long)(intptr_t)"[")) {

  inner = u_parseType(toks, (k + 1L));

  ty = s_concat(({ long _t36 = s_concat(ty, (long)(intptr_t)"["); long _t37 = ((R_PType*)(intptr_t)inner)->ty; s_concat(_t36, _t37); }), (long)(intptr_t)"]");

  k = ((R_PType*)(intptr_t)inner)->next;

  while (u_isPunct(toks, k, (long)(intptr_t)",")) {

  more = u_parseType(toks, (k + 1L));

  k = ((R_PType*)(intptr_t)more)->next;
  }

  k = u_expect(toks, k, (long)(intptr_t)"]");
  }

  if (s_eq(ty, (long)(intptr_t)"Map[Int]")) {

  ty = (long)(intptr_t)"Map";
  }

  if (s_eq(ty, (long)(intptr_t)"String")) {

  ty = (long)(intptr_t)"Str";
  }

  return PType(ty, k);
  return 0;
}
long u_isTypeDef(long toks, long i) {

  return (u_isIdent(toks, i) && u_isPunct(toks, (i + 1L), (long)(intptr_t)"="));
  return 0;
}
long u_isRecordDef(long toks, long i) {

  return u_isPunct(toks, (i + 5L), (long)(intptr_t)":");
  return 0;
}
long u_parseRecord(long toks, long i) {
  long name = 0;
  long fields = 0;
  long ftypes = 0;
  long j = 0;
  long ty = 0;
  long pt = 0;

  name = u_identAt(toks, i);

  fields = l_new();

  ftypes = l_new();

  j = (i + 4L);

  while (u_inBlock(toks, j)) {

  u_once(fields, u_expectIdent(toks, j, (long)(intptr_t)"a field name"), toks, j);

  j = (j + 1L);

  ty = (long)(intptr_t)"Int";

  if (u_isPunct(toks, j, (long)(intptr_t)":")) {

  pt = u_parseType(toks, (j + 1L));

  ty = ((R_PType*)(intptr_t)pt)->ty;

  j = ((R_PType*)(intptr_t)pt)->next;
  }

  l_push(ftypes, ty);

  if (u_isPunct(toks, j, (long)(intptr_t)",")) {

  j = (j + 1L);
  }
  }

  return ({ long _t50 = RecDef(name, fields, ftypes, u_lineOf(toks, i)); long _t51 = u_expect(toks, j, (long)(intptr_t)"}"); PRec(_t50, _t51); });
  return 0;
}
long u_parseTypeDef(long toks, long i, long seen) {
  long name = 0;
  long cases = 0;
  long j = 0;
  long cn = 0;
  long ptypes = 0;
  long pt = 0;

  name = u_identAt(toks, i);

  cases = l_new();

  j = (i + 4L);

  while (u_inBlock(toks, j)) {

  cn = u_once(seen, u_expectIdent(toks, j, (long)(intptr_t)"a case name"), toks, j);

  j = (j + 1L);

  ptypes = l_new();

  if (u_isPunct(toks, j, (long)(intptr_t)"(")) {

  j = (j + 1L);

  while (u_inArgs(toks, j)) {

  pt = u_parseType(toks, j);

  l_push(ptypes, ((R_PType*)(intptr_t)pt)->ty);

  j = u_sep(toks, ((R_PType*)(intptr_t)pt)->next, (long)(intptr_t)")");
  }

  j = u_expect(toks, j, (long)(intptr_t)")");
  }

  l_push(cases, Case(cn, l_len(ptypes), ptypes));
  }

  return ({ long _t52 = TyDef(name, cases, u_lineOf(toks, i)); long _t53 = u_expect(toks, j, (long)(intptr_t)"}"); PTy(_t52, _t53); });
  return 0;
}
long u_parseFn(long toks, long i) {
  long name = 0;
  long pp = 0;
  long rt = 0;
  long pb = 0;

  name = u_identAt(toks, i);

  pp = u_parseParams(toks, (i + 1L));

  rt = u_parseRet(toks, ((R_PNames*)(intptr_t)pp)->next);

  pb = u_parseBlock(toks, ((R_PRet*)(intptr_t)rt)->next);

  return ({ long _t34 = ({ long _t28 = ((R_PNames*)(intptr_t)pp)->names; long _t29 = ((R_PNames*)(intptr_t)pp)->types; long _t30 = ((R_PRet*)(intptr_t)rt)->ret; long _t31 = ((R_PRet*)(intptr_t)rt)->effs; long _t32 = ((R_PBody*)(intptr_t)pb)->body; long _t33 = u_lineOf(toks, i); Fn(name, _t28, _t29, _t30, _t31, _t32, _t33); }); long _t35 = ((R_PBody*)(intptr_t)pb)->next; PFn(_t34, _t35); });
  return 0;
}
long u_parseParams(long toks, long i) {
  long names = 0;
  long types = 0;
  long j = 0;
  long ty = 0;
  long pt = 0;

  names = l_new();

  types = l_new();

  j = (i + 1L);

  while (u_inArgs(toks, j)) {

  u_once(names, u_expectIdent(toks, j, (long)(intptr_t)"a parameter name"), toks, j);

  j = (j + 1L);

  ty = (long)(intptr_t)"";

  if (u_isPunct(toks, j, (long)(intptr_t)":")) {

  pt = u_parseType(toks, (j + 1L));

  ty = ((R_PType*)(intptr_t)pt)->ty;

  j = ((R_PType*)(intptr_t)pt)->next;
  }

  l_push(types, ty);

  j = u_sep(toks, j, (long)(intptr_t)")");
  }

  return PNames(names, types, u_expect(toks, j, (long)(intptr_t)")"));
  return 0;
}
long u_parseRet(long toks, long j) {
  long ret = 0;
  long effs = 0;
  long k = 0;
  long pt = 0;
  long eff = 0;

  ret = (long)(intptr_t)"Int";

  effs = l_new();

  k = j;

  if (u_isPunct(toks, k, (long)(intptr_t)":")) {

  pt = u_parseType(toks, (k + 1L));

  ret = ((R_PType*)(intptr_t)pt)->ty;

  k = ((R_PType*)(intptr_t)pt)->next;
  }

  while (u_isPunct(toks, k, (long)(intptr_t)"!")) {

  eff = u_expectIdent(toks, (k + 1L), (long)(intptr_t)"an effect name");

  if ((!((s_eq(eff, (long)(intptr_t)"IO") || s_eq(eff, (long)(intptr_t)"Fail")) || s_eq(eff, (long)(intptr_t)"Mut")))) {

  u_failLine(toks, (k + 1L), s_concat(s_concat((long)(intptr_t)"no effect named ", eff), (long)(intptr_t)"; the effects are !IO, !Fail and !Mut"));
  }

  l_push(effs, eff);

  k = (k + 2L);
  }

  if (u_isPunct(toks, k, (long)(intptr_t)":")) {

  u_failLine(toks, k, (long)(intptr_t)"the return type comes before the effects, as in f(x : Int) : Int !IO {");
  }

  return PRet(ret, effs, k);
  return 0;
}
long u_parseBlock(long toks, long i) {

  return u_parseStmts(toks, u_expect(toks, i, (long)(intptr_t)"{"));
  return 0;
}
long u_parseStmts(long toks, long start) {
  long body = 0;
  long j = 0;
  long p = 0;

  body = l_new();

  j = start;

  while (u_inBlock(toks, j)) {

  l_push(body, Line(u_lineOf(toks, j)));

  p = u_parseStmt(toks, j);

  if ((!u_isPunct(toks, ((R_PStmt*)(intptr_t)p)->next, (long)(intptr_t)"}"))) {

  u_checkUseful(((R_PStmt*)(intptr_t)p)->node, toks, j);
  }

  l_push(body, ((R_PStmt*)(intptr_t)p)->node);

  j = ((R_PStmt*)(intptr_t)p)->next;
  }

  return PBody(body, u_expect(toks, j, (long)(intptr_t)"}"));
  return 0;
}
long u_checkUseful(long s, long toks, long i) {

  switch (((Obj*)(intptr_t)s)->tag) {
  case T_Bare: { long e = ((Obj*)(intptr_t)s)->v0; return u_checkValueLine(e, toks, i); }
  case T_Let: { long name = ((Obj*)(intptr_t)s)->v0; long ty = ((Obj*)(intptr_t)s)->v1; long e = ((Obj*)(intptr_t)s)->v2; return 0L; }
  case T_If: { long c = ((Obj*)(intptr_t)s)->v0; long t = ((Obj*)(intptr_t)s)->v1; long el = ((Obj*)(intptr_t)s)->v2; return 0L; }
  case T_While: { long c = ((Obj*)(intptr_t)s)->v0; long b = ((Obj*)(intptr_t)s)->v1; return 0L; }
  case T_Ret: { long e = ((Obj*)(intptr_t)s)->v0; return 0L; }
  case T_Line: { long n = ((Obj*)(intptr_t)s)->v0; return 0L; }
  case T_Jump: { long w = ((Obj*)(intptr_t)s)->v0; return 0L; }
  }
  return 0;
  return 0;
}
long u_checkValueLine(long e, long toks, long i) {

  if (u_plainValue(e)) {

  u_failLine(toks, i, (long)(intptr_t)"this line does nothing; a value alone belongs on the last line of a block");
  }

  return 0L;
  return 0;
}
long u_plainValue(long e) {

  switch (((Obj*)(intptr_t)e)->tag) {
  case T_Num: { long v = ((Obj*)(intptr_t)e)->v0; return true; }
  case T_FloatLit: { long s = ((Obj*)(intptr_t)e)->v0; return true; }
  case T_Var: { long s = ((Obj*)(intptr_t)e)->v0; return true; }
  case T_StrLit: { long s = ((Obj*)(intptr_t)e)->v0; return true; }
  case T_ListLit: { long es = ((Obj*)(intptr_t)e)->v0; return true; }
  case T_Bin: { long op = ((Obj*)(intptr_t)e)->v0; long a = ((Obj*)(intptr_t)e)->v1; long b = ((Obj*)(intptr_t)e)->v2; return true; }
  case T_Call: { long name = ((Obj*)(intptr_t)e)->v0; long args = ((Obj*)(intptr_t)e)->v1; return false; }
  case T_Match: { long scrut = ((Obj*)(intptr_t)e)->v0; long arms = ((Obj*)(intptr_t)e)->v1; return false; }
  case T_Field: { long recv = ((Obj*)(intptr_t)e)->v0; long fld = ((Obj*)(intptr_t)e)->v1; return false; }
  case T_Method: { long recv = ((Obj*)(intptr_t)e)->v0; long name = ((Obj*)(intptr_t)e)->v1; long args = ((Obj*)(intptr_t)e)->v2; return false; }
  case T_Each: { long recv = ((Obj*)(intptr_t)e)->v0; long param = ((Obj*)(intptr_t)e)->v1; long body = ((Obj*)(intptr_t)e)->v2; return false; }
  }
  return 0;
  return 0;
}
long u_parseStmt(long toks, long i) {
  long ae = 0;
  long e = 0;
  long v = 0;

  if (u_isWord(toks, i, (long)(intptr_t)"if")) {

  return u_parseIf(toks, i);
  }

  if (u_isWord(toks, i, (long)(intptr_t)"while")) {

  return u_parseWhile(toks, i);
  }

  if (u_isWord(toks, i, (long)(intptr_t)"return")) {

  return u_parseReturn(toks, i);
  }

  if (u_isWord(toks, i, (long)(intptr_t)"break")) {

  return PStmt(Jump((long)(intptr_t)"break"), (i + 1L));
  }

  if (u_isWord(toks, i, (long)(intptr_t)"continue")) {

  return PStmt(Jump((long)(intptr_t)"continue"), (i + 1L));
  }

  if (((u_isWord(toks, i, (long)(intptr_t)"for") && u_isIdent(toks, (i + 1L))) && u_isWord(toks, (i + 2L), (long)(intptr_t)"in"))) {

  return u_parseFor(toks, i);
  }

  if (u_isTypedAssign(toks, i)) {

  return u_parseTypedLet(toks, i);
  }

  if (u_isAssign(toks, i)) {

  ae = u_parseExpr(toks, (i + 2L));

  return ({ long _t50 = ({ long _t48 = u_expectIdent(toks, i, (long)(intptr_t)"a name"); long _t49 = ((R_Parsed*)(intptr_t)ae)->node; Let(_t48, (long)(intptr_t)"", _t49); }); long _t51 = ((R_Parsed*)(intptr_t)ae)->next; PStmt(_t50, _t51); });
  }

  e = u_parseExpr(toks, i);

  if ((u_isPunct(toks, ((R_Parsed*)(intptr_t)e)->next, (long)(intptr_t)"=") && (!s_eq(u_fieldName(((R_Parsed*)(intptr_t)e)->node), (long)(intptr_t)"")))) {

  v = u_parseExpr(toks, (((R_Parsed*)(intptr_t)e)->next + 1L));

  return ({ long _t59 = Bare(({ long _t56 = u_fieldRecv(((R_Parsed*)(intptr_t)e)->node); long _t57 = s_concat((long)(intptr_t)"=", u_fieldName(((R_Parsed*)(intptr_t)e)->node)); long _t58 = l_with(l_new(), ((R_Parsed*)(intptr_t)v)->node); Method(_t56, _t57, _t58); })); long _t60 = ((R_Parsed*)(intptr_t)v)->next; PStmt(_t59, _t60); });
  }

  return ({ long _t64 = Bare(((R_Parsed*)(intptr_t)e)->node); long _t65 = ((R_Parsed*)(intptr_t)e)->next; PStmt(_t64, _t65); });
  return 0;
}
long u_fieldName(long e) {

  switch (((Obj*)(intptr_t)e)->tag) {
  case T_Field: { long recv = ((Obj*)(intptr_t)e)->v0; long fld = ((Obj*)(intptr_t)e)->v1; return fld; }
  case T_Var: { long s = ((Obj*)(intptr_t)e)->v0; return (long)(intptr_t)""; }
  case T_Method: { long recv = ((Obj*)(intptr_t)e)->v0; long name = ((Obj*)(intptr_t)e)->v1; long args = ((Obj*)(intptr_t)e)->v2; return (long)(intptr_t)""; }
  case T_ListLit: { long es = ((Obj*)(intptr_t)e)->v0; return (long)(intptr_t)""; }
  case T_Call: { long name = ((Obj*)(intptr_t)e)->v0; long args = ((Obj*)(intptr_t)e)->v1; return (long)(intptr_t)""; }
  case T_Match: { long scrut = ((Obj*)(intptr_t)e)->v0; long arms = ((Obj*)(intptr_t)e)->v1; return (long)(intptr_t)""; }
  case T_Bin: { long op = ((Obj*)(intptr_t)e)->v0; long a = ((Obj*)(intptr_t)e)->v1; long b = ((Obj*)(intptr_t)e)->v2; return (long)(intptr_t)""; }
  case T_Each: { long recv = ((Obj*)(intptr_t)e)->v0; long param = ((Obj*)(intptr_t)e)->v1; long body = ((Obj*)(intptr_t)e)->v2; return (long)(intptr_t)""; }
  case T_Num: { long v = ((Obj*)(intptr_t)e)->v0; return (long)(intptr_t)""; }
  case T_FloatLit: { long s = ((Obj*)(intptr_t)e)->v0; return (long)(intptr_t)""; }
  case T_StrLit: { long s = ((Obj*)(intptr_t)e)->v0; return (long)(intptr_t)""; }
  }
  return 0;
  return 0;
}
long u_fieldRecv(long e) {

  switch (((Obj*)(intptr_t)e)->tag) {
  case T_Field: { long recv = ((Obj*)(intptr_t)e)->v0; long fld = ((Obj*)(intptr_t)e)->v1; return recv; }
  case T_Var: { long s = ((Obj*)(intptr_t)e)->v0; return e; }
  case T_Method: { long recv = ((Obj*)(intptr_t)e)->v0; long name = ((Obj*)(intptr_t)e)->v1; long args = ((Obj*)(intptr_t)e)->v2; return e; }
  case T_ListLit: { long es = ((Obj*)(intptr_t)e)->v0; return e; }
  case T_Call: { long name = ((Obj*)(intptr_t)e)->v0; long args = ((Obj*)(intptr_t)e)->v1; return e; }
  case T_Match: { long scrut = ((Obj*)(intptr_t)e)->v0; long arms = ((Obj*)(intptr_t)e)->v1; return e; }
  case T_Bin: { long op = ((Obj*)(intptr_t)e)->v0; long a = ((Obj*)(intptr_t)e)->v1; long b = ((Obj*)(intptr_t)e)->v2; return e; }
  case T_Each: { long recv = ((Obj*)(intptr_t)e)->v0; long param = ((Obj*)(intptr_t)e)->v1; long body = ((Obj*)(intptr_t)e)->v2; return e; }
  case T_Num: { long v = ((Obj*)(intptr_t)e)->v0; return e; }
  case T_FloatLit: { long s = ((Obj*)(intptr_t)e)->v0; return e; }
  case T_StrLit: { long s = ((Obj*)(intptr_t)e)->v0; return e; }
  }
  return 0;
  return 0;
}
long u_isSetField(long name) {

  return s_starts(name, (long)(intptr_t)"=");
  return 0;
}
long u_parseFor(long toks, long i) {
  long param = 0;
  long lo = 0;
  long recv = 0;
  long k = 0;
  long more = 0;
  long hi = 0;
  long b = 0;

  param = u_expectIdent(toks, (i + 1L), (long)(intptr_t)"a name for each item");

  lo = u_parseAdd(toks, (i + 3L));

  recv = ((R_Parsed*)(intptr_t)lo)->node;

  k = ((R_Parsed*)(intptr_t)lo)->next;

  if (u_isPunct(toks, k, (long)(intptr_t)"..")) {

  k = (k + 1L);

  more = 0L;

  if (u_isPunct(toks, k, (long)(intptr_t)"=")) {

  more = 1L;

  k = (k + 1L);
  }

  hi = u_parseAdd(toks, k);

  recv = Call((long)(intptr_t)"l_range", ({ long _t47 = ((R_Parsed*)(intptr_t)lo)->node; long _t50 = ({ long _t48 = ((R_Parsed*)(intptr_t)hi)->node; long _t49 = Num(more); Bin((long)(intptr_t)"+", _t48, _t49); }); l_with(l_with(l_new(), _t47), _t50); }));

  k = ((R_Parsed*)(intptr_t)hi)->next;
  }

  k = u_expect(toks, k, (long)(intptr_t)"{");

  b = u_parseStmts(toks, k);

  return ({ long _t57 = Bare(Each(recv, param, ((R_PBody*)(intptr_t)b)->body)); long _t58 = ((R_PBody*)(intptr_t)b)->next; PStmt(_t57, _t58); });
  return 0;
}
long u_parseReturn(long toks, long i) {
  long re = 0;

  if (u_isPunct(toks, (i + 1L), (long)(intptr_t)"}")) {

  return PStmt(Ret(Num(0L)), (i + 1L));
  }

  re = u_parseExpr(toks, (i + 1L));

  return ({ long _t17 = Ret(((R_Parsed*)(intptr_t)re)->node); long _t18 = ((R_Parsed*)(intptr_t)re)->next; PStmt(_t17, _t18); });
  return 0;
}
long u_isTypedAssign(long toks, long i) {

  return (u_isIdent(toks, i) && u_isPunct(toks, (i + 1L), (long)(intptr_t)":"));
  return 0;
}
long u_parseTypedLet(long toks, long i) {
  long name = 0;
  long pt = 0;
  long ae = 0;

  name = u_expectIdent(toks, i, (long)(intptr_t)"a name");

  pt = u_parseType(toks, (i + 2L));

  ae = u_parseExpr(toks, u_expect(toks, ((R_PType*)(intptr_t)pt)->next, (long)(intptr_t)"="));

  return ({ long _t23 = ({ long _t21 = ((R_PType*)(intptr_t)pt)->ty; long _t22 = ((R_Parsed*)(intptr_t)ae)->node; Let(name, _t21, _t22); }); long _t24 = ((R_Parsed*)(intptr_t)ae)->next; PStmt(_t23, _t24); });
  return 0;
}
long u_parseIf(long toks, long i) {
  long c = 0;
  long tb = 0;
  long els = 0;
  long nxt = 0;
  long inner = 0;
  long eb = 0;

  c = u_parseExpr(toks, (i + 1L));

  tb = u_parseBlock(toks, ((R_Parsed*)(intptr_t)c)->next);

  els = l_new();

  nxt = ((R_PBody*)(intptr_t)tb)->next;

  if (u_isWord(toks, ((R_PBody*)(intptr_t)tb)->next, (long)(intptr_t)"else")) {

  if (u_isWord(toks, (((R_PBody*)(intptr_t)tb)->next + 1L), (long)(intptr_t)"if")) {

  inner = u_parseIf(toks, (((R_PBody*)(intptr_t)tb)->next + 1L));

  l_push(els, ((R_PStmt*)(intptr_t)inner)->node);

  nxt = ((R_PStmt*)(intptr_t)inner)->next;
  } else {

  eb = u_parseBlock(toks, (((R_PBody*)(intptr_t)tb)->next + 1L));

  els = ((R_PBody*)(intptr_t)eb)->body;

  nxt = ((R_PBody*)(intptr_t)eb)->next;
  }
  }

  return PStmt(({ long _t43 = ((R_Parsed*)(intptr_t)c)->node; long _t44 = ((R_PBody*)(intptr_t)tb)->body; If(_t43, _t44, els); }), nxt);
  return 0;
}
long u_parseWhile(long toks, long i) {
  long c = 0;
  long b = 0;

  c = u_parseExpr(toks, (i + 1L));

  b = u_parseBlock(toks, ((R_Parsed*)(intptr_t)c)->next);

  return ({ long _t20 = ({ long _t18 = ((R_Parsed*)(intptr_t)c)->node; long _t19 = ((R_PBody*)(intptr_t)b)->body; While(_t18, _t19); }); long _t21 = ((R_PBody*)(intptr_t)b)->next; PStmt(_t20, _t21); });
  return 0;
}
long u_parseExpr(long toks, long pos) {
  long p = 0;
  long node = 0;
  long i = 0;
  long r = 0;

  p = u_parseAnd(toks, pos);

  node = ((R_Parsed*)(intptr_t)p)->node;

  i = ((R_Parsed*)(intptr_t)p)->next;

  while (u_isPunct(toks, i, (long)(intptr_t)"||")) {

  r = u_parseAnd(toks, (i + 1L));

  node = Bin((long)(intptr_t)"||", node, ((R_Parsed*)(intptr_t)r)->node);

  i = ((R_Parsed*)(intptr_t)r)->next;
  }

  return Parsed(node, i);
  return 0;
}
long u_parseAnd(long toks, long pos) {
  long p = 0;
  long node = 0;
  long i = 0;
  long r = 0;

  p = u_parseCmp(toks, pos);

  node = ((R_Parsed*)(intptr_t)p)->node;

  i = ((R_Parsed*)(intptr_t)p)->next;

  while (u_isPunct(toks, i, (long)(intptr_t)"&&")) {

  r = u_parseCmp(toks, (i + 1L));

  node = Bin((long)(intptr_t)"&&", node, ((R_Parsed*)(intptr_t)r)->node);

  i = ((R_Parsed*)(intptr_t)r)->next;
  }

  return Parsed(node, i);
  return 0;
}
long u_parseCmp(long toks, long pos) {
  long p = 0;
  long node = 0;
  long i = 0;
  long op = 0;
  long r = 0;

  p = u_parseAdd(toks, pos);

  node = ((R_Parsed*)(intptr_t)p)->node;

  i = ((R_Parsed*)(intptr_t)p)->next;

  while (u_isCmpOp(toks, i)) {

  op = u_punctAt(toks, i);

  r = u_parseAdd(toks, (i + 1L));

  if (s_eq(op, (long)(intptr_t)"!=")) {

  node = Field(Bin((long)(intptr_t)"==", node, ((R_Parsed*)(intptr_t)r)->node), (long)(intptr_t)"not");
  } else {

  node = Bin(op, node, ((R_Parsed*)(intptr_t)r)->node);
  }

  i = ((R_Parsed*)(intptr_t)r)->next;
  }

  return Parsed(node, i);
  return 0;
}
long u_parseAdd(long toks, long pos) {
  long p = 0;
  long node = 0;
  long i = 0;
  long op = 0;
  long r = 0;

  p = u_parseTerm(toks, pos);

  node = ((R_Parsed*)(intptr_t)p)->node;

  i = ((R_Parsed*)(intptr_t)p)->next;

  while (u_isAddOp(toks, i)) {

  op = u_punctAt(toks, i);

  r = u_parseTerm(toks, (i + 1L));

  node = Bin(op, node, ((R_Parsed*)(intptr_t)r)->node);

  i = ((R_Parsed*)(intptr_t)r)->next;
  }

  return Parsed(node, i);
  return 0;
}
long u_parseTerm(long toks, long pos) {
  long p = 0;
  long node = 0;
  long i = 0;
  long op = 0;
  long r = 0;

  p = u_parsePostfix(toks, pos);

  node = ((R_Parsed*)(intptr_t)p)->node;

  i = ((R_Parsed*)(intptr_t)p)->next;

  while (u_isMulOp(toks, i)) {

  op = u_punctAt(toks, i);

  r = u_parsePostfix(toks, (i + 1L));

  node = Bin(op, node, ((R_Parsed*)(intptr_t)r)->node);

  i = ((R_Parsed*)(intptr_t)r)->next;
  }

  return Parsed(node, i);
  return 0;
}
long u_parsePostfix(long toks, long pos) {
  long p = 0;
  long node = 0;
  long i = 0;
  long pm = 0;
  long pe = 0;
  long name = 0;
  long pa = 0;

  p = u_parseFactor(toks, pos);

  node = ((R_Parsed*)(intptr_t)p)->node;

  i = ((R_Parsed*)(intptr_t)p)->next;

  if (u_isPunct(toks, i, (long)(intptr_t)"?")) {

  i = (i + 1L);
  }

  while (u_isPunct(toks, i, (long)(intptr_t)".")) {

  if (u_isWord(toks, (i + 1L), (long)(intptr_t)"match")) {

  pm = u_parseMatch(toks, node, i);

  node = ((R_Parsed*)(intptr_t)pm)->node;

  i = ((R_Parsed*)(intptr_t)pm)->next;
  } else {

  if (u_isWord(toks, (i + 1L), (long)(intptr_t)"each")) {

  pe = u_parseEach(toks, node, i);

  node = ((R_Parsed*)(intptr_t)pe)->node;

  i = ((R_Parsed*)(intptr_t)pe)->next;
  } else {

  name = u_expectIdent(toks, (i + 1L), (long)(intptr_t)"a name after the dot");

  if (u_isPunct(toks, (i + 2L), (long)(intptr_t)"(")) {

  pa = u_parseArgs(toks, (i + 2L));

  node = Method(node, name, ((R_PArgs*)(intptr_t)pa)->list);

  i = ((R_PArgs*)(intptr_t)pa)->next;
  } else {

  node = Field(node, name);

  i = (i + 2L);
  }
  }
  }

  if (u_isPunct(toks, i, (long)(intptr_t)"?")) {

  i = (i + 1L);
  }
  }

  return Parsed(node, i);
  return 0;
}
long u_parseEach(long toks, long recv, long i) {
  long param = 0;
  long b = 0;

  u_expect(toks, (i + 2L), (long)(intptr_t)"{");

  param = u_expectIdent(toks, (i + 3L), (long)(intptr_t)"a name for each item");

  if ((!u_isWord(toks, (i + 4L), (long)(intptr_t)"in"))) {

  u_failTok(toks, (i + 4L), (long)(intptr_t)"expected the word in");
  }

  b = u_parseStmts(toks, (i + 5L));

  return ({ long _t24 = Each(recv, param, ((R_PBody*)(intptr_t)b)->body); long _t25 = ((R_PBody*)(intptr_t)b)->next; Parsed(_t24, _t25); });
  return 0;
}
long u_parseFactor(long toks, long pos) {

  switch (((Obj*)(intptr_t)u_tokAt(toks, pos))->tag) {
  case T_Int: { long v = ((Obj*)(intptr_t)u_tokAt(toks, pos))->v0; return Parsed(Num(v), (pos + 1L)); }
  case T_Float: { long s = ((Obj*)(intptr_t)u_tokAt(toks, pos))->v0; return Parsed(FloatLit(s), (pos + 1L)); }
  case T_Str: { long s = ((Obj*)(intptr_t)u_tokAt(toks, pos))->v0; return Parsed(StrLit(s), (pos + 1L)); }
  case T_Ident: { long s = ((Obj*)(intptr_t)u_tokAt(toks, pos))->v0; return u_parseIdentFactor(toks, pos); }
  case T_Punct: { long p = ((Obj*)(intptr_t)u_tokAt(toks, pos))->v0; return u_parsePunctFactor(toks, pos); }
  case T_Eof: { return Parsed(Num(u_failTok(toks, pos, (long)(intptr_t)"expected a value")), pos); }
  }
  return 0;
  return 0;
}
long u_parsePunctFactor(long toks, long pos) {
  long g = 0;
  long f = 0;

  if ((u_isPunct(toks, pos, (long)(intptr_t)"(") || u_isPunct(toks, pos, (long)(intptr_t)"s("))) {

  return u_parseParen(toks, pos);
  }

  if (u_isPunct(toks, pos, (long)(intptr_t)"[")) {

  return u_parseList(toks, pos);
  }

  if (u_isPunct(toks, pos, (long)(intptr_t)"!")) {

  g = u_parsePostfix(toks, (pos + 1L));

  return ({ long _t30 = Field(((R_Parsed*)(intptr_t)g)->node, (long)(intptr_t)"not"); long _t31 = ((R_Parsed*)(intptr_t)g)->next; Parsed(_t30, _t31); });
  }

  if ((!u_isPunct(toks, pos, (long)(intptr_t)"-"))) {

  u_failTok(toks, pos, (long)(intptr_t)"expected a value");
  }

  if ((u_isIdent(toks, (pos + 1L)) || u_isPunct(toks, (pos + 1L), (long)(intptr_t)"("))) {

  g = u_parsePostfix(toks, (pos + 1L));

  return ({ long _t39 = ({ long _t37 = Num(0L); long _t38 = ((R_Parsed*)(intptr_t)g)->node; Bin((long)(intptr_t)"u-", _t37, _t38); }); long _t40 = ((R_Parsed*)(intptr_t)g)->next; Parsed(_t39, _t40); });
  }

  f = u_parseFactor(toks, (pos + 1L));

  return ({ long _t49 = ({ long _t47 = Num(0L); long _t48 = ((R_Parsed*)(intptr_t)f)->node; Bin((long)(intptr_t)"u-", _t47, _t48); }); long _t50 = ((R_Parsed*)(intptr_t)f)->next; Parsed(_t49, _t50); });
  return 0;
}
long u_parseList(long toks, long pos) {
  long elems = 0;
  long j = 0;
  long e = 0;

  elems = l_new();

  j = (pos + 1L);

  while (((!u_isPunct(toks, j, (long)(intptr_t)"]")) && u_notEof(toks, j))) {

  e = u_parseExpr(toks, j);

  l_push(elems, ((R_Parsed*)(intptr_t)e)->node);

  j = u_sep(toks, ((R_Parsed*)(intptr_t)e)->next, (long)(intptr_t)"]");
  }

  return ({ long _t26 = ListLit(elems); long _t27 = u_expect(toks, j, (long)(intptr_t)"]"); Parsed(_t26, _t27); });
  return 0;
}
long u_parseIdentFactor(long toks, long pos) {
  long name = 0;
  long pt = 0;
  long r = 0;
  long pa = 0;

  name = u_expectIdent(toks, pos, (long)(intptr_t)"a value");

  if ((s_eq(name, (long)(intptr_t)"Map") && u_isPunct(toks, (pos + 1L), (long)(intptr_t)"["))) {

  pt = u_parseType(toks, pos);

  if ((u_isPunct(toks, ((R_PType*)(intptr_t)pt)->next, (long)(intptr_t)"(") && u_isPunct(toks, (((R_PType*)(intptr_t)pt)->next + 1L), (long)(intptr_t)")"))) {

  return ({ long _t26 = Call(((R_PType*)(intptr_t)pt)->ty, l_new()); long _t27 = (((R_PType*)(intptr_t)pt)->next + 2L); Parsed(_t26, _t27); });
  }
  }

  r = Parsed(Var(name), (pos + 1L));

  if (u_isPunct(toks, (pos + 1L), (long)(intptr_t)"(")) {

  pa = u_parseArgs(toks, (pos + 1L));

  r = ({ long _t32 = Call(name, ((R_PArgs*)(intptr_t)pa)->list); long _t33 = ((R_PArgs*)(intptr_t)pa)->next; Parsed(_t32, _t33); });
  }

  return r;
  return 0;
}
long u_parseParen(long toks, long pos) {
  long inner = 0;

  inner = u_parseExpr(toks, (pos + 1L));

  return ({ long _t13 = ((R_Parsed*)(intptr_t)inner)->node; long _t14 = u_expect(toks, ((R_Parsed*)(intptr_t)inner)->next, (long)(intptr_t)")"); Parsed(_t13, _t14); });
  return 0;
}
long u_parseArgs(long toks, long i) {
  long list = 0;
  long j = 0;
  long e = 0;

  list = l_new();

  j = (i + 1L);

  while (u_inArgs(toks, j)) {

  if (u_isAssign(toks, j)) {

  j = (j + 2L);
  }

  e = u_parseExpr(toks, j);

  l_push(list, ((R_Parsed*)(intptr_t)e)->node);

  j = u_sep(toks, ((R_Parsed*)(intptr_t)e)->next, (long)(intptr_t)")");
  }

  return PArgs(list, u_expect(toks, j, (long)(intptr_t)")"));
  return 0;
}
long u_parseMatch(long toks, long scrut, long i) {
  long arms = 0;
  long j = 0;
  long pa = 0;

  arms = l_new();

  j = u_expect(toks, (i + 2L), (long)(intptr_t)"{");

  while (u_inBlock(toks, j)) {

  pa = u_parseArm(toks, j);

  l_push(arms, ((R_PArm*)(intptr_t)pa)->arm);

  j = ((R_PArm*)(intptr_t)pa)->next;
  }

  return ({ long _t28 = Match(scrut, arms); long _t29 = u_expect(toks, j, (long)(intptr_t)"}"); Parsed(_t28, _t29); });
  return 0;
}
long u_parseArm(long toks, long j) {
  long tag = 0;
  long binds = 0;
  long k = 0;
  long ae = 0;

  tag = u_expectIdent(toks, j, (long)(intptr_t)"a case name");

  binds = l_new();

  k = (j + 1L);

  if (u_isPunct(toks, k, (long)(intptr_t)"(")) {

  k = (k + 1L);

  while (u_inArgs(toks, k)) {

  l_push(binds, u_expectIdent(toks, k, (long)(intptr_t)"a name"));

  k = u_sep(toks, (k + 1L), (long)(intptr_t)")");
  }

  k = u_expect(toks, k, (long)(intptr_t)")");
  }

  k = u_expect(toks, k, (long)(intptr_t)"->");

  if (u_isPunct(toks, k, (long)(intptr_t)"{")) {

  u_failLine(toks, k, s_concat(s_concat((long)(intptr_t)"match case ", tag), (long)(intptr_t)": an arm is one expression, not a { } block; call a function for more"));
  }

  ae = u_parseExpr(toks, k);

  return ({ long _t44 = Arm(tag, binds, ((R_Parsed*)(intptr_t)ae)->node); long _t45 = ((R_Parsed*)(intptr_t)ae)->next; PArm(_t44, _t45); });
  return 0;
}
long u_emitType(long t) {
  long r = 0;

  r = u_emitEnumType(t);

  if (u_hasPayload(t)) {

  r = u_emitBoxedType(t);
  }

  return r;
  return 0;
}
long u_emitEnumType(long t) {
  long out = 0;
  long k = 0;
  long m = 0;

  out = (long)(intptr_t)"enum {";

  k = 0L;

  m = l_len(((R_TyDef*)(intptr_t)t)->cases);

  while ((k < m)) {

  if ((k > 0L)) {

  out = s_concat(out, (long)(intptr_t)",");
  }

  out = ({ long _t27 = s_concat(out, (long)(intptr_t)" "); long _t28 = ((R_Case*)(intptr_t)l_atc(((R_TyDef*)(intptr_t)t)->cases, k, 2008))->cname; s_concat(_t27, _t28); });

  k = (k + 1L);
  }

  return s_concat(out, (long)(intptr_t)" };");
  return 0;
}
long u_emitBoxedType(long t) {
  long out = 0;
  long k = 0;
  long m = 0;
  long cn = 0;
  long ar = 0;
  long mkName = 0;

  out = (long)(intptr_t)"enum {";

  k = 0L;

  m = l_len(((R_TyDef*)(intptr_t)t)->cases);

  while ((k < m)) {

  if ((k > 0L)) {

  out = s_concat(out, (long)(intptr_t)",");
  }

  out = ({ long _t47 = s_concat(out, (long)(intptr_t)" T_"); long _t48 = ((R_Case*)(intptr_t)l_atc(((R_TyDef*)(intptr_t)t)->cases, k, 2022))->cname; s_concat(_t47, _t48); });

  k = (k + 1L);
  }

  out = s_concat(out, (long)(intptr_t)" };");

  k = 0L;

  while ((k < m)) {

  cn = ((R_Case*)(intptr_t)l_atc(((R_TyDef*)(intptr_t)t)->cases, k, 2028))->cname;

  ar = ((R_Case*)(intptr_t)l_atc(((R_TyDef*)(intptr_t)t)->cases, k, 2029))->arity;

  if ((ar > 8L)) {

  fail(s_concat(({ long _t57 = s_concat(s_concat((long)(intptr_t)"the case ", cn), (long)(intptr_t)" holds "); long _t58 = i_tostr(ar); s_concat(_t57, _t58); }), (long)(intptr_t)" values; a case holds at most 8, so put them in a record"));
  }

  mkName = (long)(intptr_t)"mk";

  if ((ar > 3L)) {

  mkName = (long)(intptr_t)"mk8";
  }

  out = s_concat(({ long _t65 = s_concat(s_concat(s_concat(s_concat(s_concat(({ long _t63 = s_concat(s_concat(s_concat(out, (long)(intptr_t)"\nlong "), cn), (long)(intptr_t)"("); long _t64 = u_ctorParams(ar); s_concat(_t63, _t64); }), (long)(intptr_t)") { return "), mkName), (long)(intptr_t)"(T_"), cn), (long)(intptr_t)", "); long _t66 = u_ctorArgs(ar); s_concat(_t65, _t66); }), (long)(intptr_t)"); }");

  k = (k + 1L);
  }

  return out;
  return 0;
}
long u_hasPayload(long t) {
  long has = 0;
  long k = 0;
  long m = 0;

  has = false;

  k = 0L;

  m = l_len(((R_TyDef*)(intptr_t)t)->cases);

  while ((k < m)) {

  if ((((R_Case*)(intptr_t)l_atc(((R_TyDef*)(intptr_t)t)->cases, k, 2048))->arity > 0L)) {

  has = true;
  }

  k = (k + 1L);
  }

  return has;
  return 0;
}
long u_emitRecord(long rec) {
  long out = 0;
  long k = 0;
  long m = 0;
  long fn = 0;

  out = (long)(intptr_t)"typedef struct {";

  k = 0L;

  m = l_len(((R_RecDef*)(intptr_t)rec)->fields);

  while ((k < m)) {

  out = s_concat(({ long _t35 = s_concat(out, (long)(intptr_t)" long "); long _t36 = l_atc(((R_RecDef*)(intptr_t)rec)->fields, k, 2061); s_concat(_t35, _t36); }), (long)(intptr_t)";");

  k = (k + 1L);
  }

  out = s_concat(({ long _t39 = s_concat(s_concat(out, (long)(intptr_t)" } "), (long)(intptr_t)"R_"); long _t40 = ((R_RecDef*)(intptr_t)rec)->name; s_concat(_t39, _t40); }), (long)(intptr_t)";");

  out = s_concat(({ long _t48 = s_concat(({ long _t46 = s_concat(({ long _t44 = s_concat(({ long _t42 = s_concat(out, (long)(intptr_t)"\nlong "); long _t43 = ((R_RecDef*)(intptr_t)rec)->name; s_concat(_t42, _t43); }), (long)(intptr_t)"("); long _t45 = u_recParams(rec); s_concat(_t44, _t45); }), (long)(intptr_t)") { R_"); long _t47 = ((R_RecDef*)(intptr_t)rec)->name; s_concat(_t46, _t47); }), (long)(intptr_t)"* _o = malloc(sizeof(R_"); long _t49 = ((R_RecDef*)(intptr_t)rec)->name; s_concat(_t48, _t49); }), (long)(intptr_t)"));");

  k = 0L;

  while ((k < m)) {

  fn = l_atc(((R_RecDef*)(intptr_t)rec)->fields, k, 2068);

  out = s_concat(s_concat(s_concat(s_concat(s_concat(out, (long)(intptr_t)" _o->"), fn), (long)(intptr_t)" = "), fn), (long)(intptr_t)";");

  k = (k + 1L);
  }

  return s_concat(out, (long)(intptr_t)" return (long)(intptr_t)_o; }");
  return 0;
}
long u_recParams(long rec) {
  long out = 0;
  long k = 0;
  long m = 0;

  out = (long)(intptr_t)"";

  k = 0L;

  m = l_len(((R_RecDef*)(intptr_t)rec)->fields);

  while ((k < m)) {

  if ((k > 0L)) {

  out = s_concat(out, (long)(intptr_t)", ");
  }

  out = ({ long _t27 = s_concat(out, (long)(intptr_t)"long "); long _t28 = l_atc(((R_RecDef*)(intptr_t)rec)->fields, k, 2083); s_concat(_t27, _t28); });

  k = (k + 1L);
  }

  return out;
  return 0;
}
long u_slotName(long bi) {

  return s_concat((long)(intptr_t)"v", i_tostr(bi));
  return 0;
}
long u_ctorParams(long arity) {
  long out = 0;
  long k = 0;

  out = (long)(intptr_t)"";

  k = 0L;

  while ((k < arity)) {

  if ((k > 0L)) {

  out = s_concat(out, (long)(intptr_t)", ");
  }

  out = ({ long _t23 = s_concat(out, (long)(intptr_t)"long "); long _t24 = u_slotName(k); s_concat(_t23, _t24); });

  k = (k + 1L);
  }

  return out;
  return 0;
}
long u_ctorArgs(long arity) {
  long out = 0;
  long k = 0;
  long top = 0;
  long slot = 0;

  out = (long)(intptr_t)"";

  k = 0L;

  top = 3L;

  if ((arity > 3L)) {

  top = 8L;
  }

  while ((k < top)) {

  if ((k > 0L)) {

  out = s_concat(out, (long)(intptr_t)", ");
  }

  slot = (long)(intptr_t)"0";

  if ((k < arity)) {

  slot = u_slotName(k);
  }

  out = s_concat(out, slot);

  k = (k + 1L);
  }

  return out;
  return 0;
}
long u_mangle(long name) {

  return s_concat((long)(intptr_t)"u_", name);
  return 0;
}
long u_emitProto(long f) {

  return s_concat(({ long _t9 = s_concat(s_concat((long)(intptr_t)"long ", u_mangle(((R_Fn*)(intptr_t)f)->name)), (long)(intptr_t)"("); long _t10 = u_paramDecls(f); s_concat(_t9, _t10); }), (long)(intptr_t)");");
  return 0;
}
long u_emitFn(long f, long sigs) {
  long rt = 0;
  long sig = 0;
  long nm = 0;
  long out = 0;
  long ctx = 0;
  long decls = 0;
  long d = 0;
  long dn = 0;
  long k = 0;
  long m = 0;
  long last = 0;

  rt = (long)(intptr_t)"long";

  sig = u_paramDecls(f);

  if (u_isMain(((R_Fn*)(intptr_t)f)->name)) {

  rt = (long)(intptr_t)"int";

  sig = (long)(intptr_t)"int argc, char** argv";
  }

  nm = ((R_Fn*)(intptr_t)f)->name;

  if ((!u_isMain(((R_Fn*)(intptr_t)f)->name))) {

  nm = u_mangle(((R_Fn*)(intptr_t)f)->name);
  }

  out = s_concat(s_concat(s_concat(s_concat(s_concat(rt, (long)(intptr_t)" "), nm), (long)(intptr_t)"("), sig), (long)(intptr_t)") {");

  if (u_isMain(((R_Fn*)(intptr_t)f)->name)) {

  out = s_concat(out, (long)(intptr_t)"\n  simpler_argc = argc; simpler_argv = argv; simpler_self_init();");
  }

  ctx = u_fnCtx(f, sigs);

  u_checkReturn(f, ctx);

  u_checkEffects(f, ctx);

  decls = u_collectLets(((R_Fn*)(intptr_t)f)->body);

  d = 0L;

  dn = l_len(decls);

  while ((d < dn)) {

  if ((!({ long _t74 = ((R_Fn*)(intptr_t)f)->params; long _t75 = l_atc(decls, d, 2163); u_hasName(_t74, _t75); }))) {

  out = s_concat(({ long _t77 = s_concat(out, (long)(intptr_t)"\n  long "); long _t78 = l_atc(decls, d, 2164); s_concat(_t77, _t78); }), (long)(intptr_t)" = 0;");
  }

  d = (d + 1L);
  }

  k = 0L;

  m = l_len(((R_Fn*)(intptr_t)f)->body);

  while ((k < m)) {

  last = (((k + 1L) == m) && (!u_isMain(((R_Fn*)(intptr_t)f)->name)));

  out = ({ long _t85 = s_concat(out, (long)(intptr_t)"\n"); long _t86 = u_emitStmt(l_atc(((R_Fn*)(intptr_t)f)->body, k, 2172), last, ctx); s_concat(_t85, _t86); });

  k = (k + 1L);
  }

  return s_concat(out, (long)(intptr_t)"\n  return 0;\n}");
  return 0;
}
long u_fnCtx(long f, long sigs) {
  long env = 0;
  long pk = 0;
  long pn = 0;
  long ctx = 0;

  env = u_newEnv();

  pk = 0L;

  pn = l_len(((R_Fn*)(intptr_t)f)->params);

  while ((pk < pn)) {

  ({ long _t36 = l_atc(((R_Fn*)(intptr_t)f)->params, pk, 2184); long _t37 = l_atc(((R_Fn*)(intptr_t)f)->ptypes, pk, 2184); u_envPut(env, _t36, _t37); });

  pk = (pk + 1L);
  }

  ctx = Ctx(env, sigs);

  u_envPut(env, (long)(intptr_t)"__line__", i_tostr(((R_Fn*)(intptr_t)f)->line));

  if (u_isMain(((R_Fn*)(intptr_t)f)->name)) {

  u_envPut(env, (long)(intptr_t)"__ret__", (long)(intptr_t)"__main__");

  if ((l_len(((R_Fn*)(intptr_t)f)->params) > 0L)) {

  u_envPut(env, l_atc(((R_Fn*)(intptr_t)f)->params, 0L, 2192), (long)(intptr_t)"Sys");
  }
  } else {

  if (u_hasName(((R_Fn*)(intptr_t)f)->ptypes, (long)(intptr_t)"Sys")) {

  u_failAt((long)(intptr_t)"only main holds the world; hand down sys.screen or sys.files", ctx);
  }

  u_envPut(env, (long)(intptr_t)"__ret__", ((R_Fn*)(intptr_t)f)->ret);
  }

  u_seedEnv(((R_Fn*)(intptr_t)f)->body, ctx);

  u_setLine(((R_Fn*)(intptr_t)f)->line, ctx);

  return ctx;
  return 0;
}
long u_paramDecls(long f) {
  long out = 0;
  long k = 0;
  long m = 0;

  out = (long)(intptr_t)"";

  k = 0L;

  m = l_len(((R_Fn*)(intptr_t)f)->params);

  while ((k < m)) {

  if ((k > 0L)) {

  out = s_concat(out, (long)(intptr_t)", ");
  }

  out = ({ long _t27 = s_concat(out, (long)(intptr_t)"long "); long _t28 = l_atc(((R_Fn*)(intptr_t)f)->params, k, 2213); s_concat(_t27, _t28); });

  k = (k + 1L);
  }

  return out;
  return 0;
}
long u_newEnv() {
  long ns = 0;
  long ts = 0;

  ns = l_new();

  ts = l_new();

  return TyEnv(ns, ts);
  return 0;
}
long u_envPut(long env, long name, long ty) {

  l_push(((R_TyEnv*)(intptr_t)env)->names, name);

  l_push(((R_TyEnv*)(intptr_t)env)->tys, ty);

  return 0L;
  return 0;
}
long u_envOld(long env, long name) {
  long r = 0;

  r = (long)(intptr_t)"__gone__";

  if (u_hasName(((R_TyEnv*)(intptr_t)env)->names, name)) {

  r = u_envGet(env, name);
  }

  return r;
  return 0;
}
long u_envOlds(long env, long names) {
  long olds = 0;

  olds = l_new();

  {
  long _lst = names;
  long _n = l_len(_lst);
  for (long _i = 0; _i < _n; _i = _i + 1) {
  long n = l_at(_lst, _i);

  l_push(olds, u_envOld(env, n));
  }
  }

  return olds;
  return 0;
}
long u_envRestore(long env, long names, long olds) {
  long k = 0;

  k = 0L;

  while ((k < l_len(names))) {

  ({ long _t19 = l_atc(names, k, 2254); long _t20 = l_atc(olds, k, 2254); u_envPut(env, _t19, _t20); });

  k = (k + 1L);
  }

  return 0L;
  return 0;
}
long u_envGet(long env, long name) {
  long r = 0;
  long k = 0;
  long m = 0;

  r = (long)(intptr_t)"Int";

  k = 0L;

  m = l_len(((R_TyEnv*)(intptr_t)env)->names);

  while ((k < m)) {

  if (s_eq(l_atc(((R_TyEnv*)(intptr_t)env)->names, k, 2265), name)) {

  r = l_atc(((R_TyEnv*)(intptr_t)env)->tys, k, 2266);
  }

  k = (k + 1L);
  }

  return r;
  return 0;
}
long u_seedEnv(long body, long ctx) {
  long k = 0;
  long m = 0;

  k = 0L;

  m = l_len(body);

  while ((k < m)) {

  u_seedStmt(l_atc(body, k, 2280), ctx);

  k = (k + 1L);
  }

  return 0L;
  return 0;
}
long u_seedStmt(long s, long ctx) {

  switch (((Obj*)(intptr_t)s)->tag) {
  case T_Let: { long name = ((Obj*)(intptr_t)s)->v0; long ty = ((Obj*)(intptr_t)s)->v1; long e = ((Obj*)(intptr_t)s)->v2; return u_seedLet(name, u_letType(ty, e, ctx), ctx); }
  case T_Bare: { long e = ((Obj*)(intptr_t)s)->v0; return u_seedExpr(e, ctx); }
  case T_If: { long c = ((Obj*)(intptr_t)s)->v0; long t = ((Obj*)(intptr_t)s)->v1; long el = ((Obj*)(intptr_t)s)->v2; return u_seedIf(t, el, ctx); }
  case T_While: { long c = ((Obj*)(intptr_t)s)->v0; long b = ((Obj*)(intptr_t)s)->v1; return u_seedEnv(b, ctx); }
  case T_Ret: { long e = ((Obj*)(intptr_t)s)->v0; return u_seedExpr(e, ctx); }
  case T_Line: { long n = ((Obj*)(intptr_t)s)->v0; return u_setLine(n, ctx); }
  case T_Jump: { long w = ((Obj*)(intptr_t)s)->v0; return 0L; }
  }
  return 0;
  return 0;
}
long u_seedLet(long name, long ty0, long ctx) {
  long ty = 0;
  long old = 0;

  ty = ty0;

  if ((s_eq(ty0, (long)(intptr_t)"List") && s_starts(u_envOld(((R_Ctx*)(intptr_t)ctx)->env, name), (long)(intptr_t)"List["))) {

  ty = u_envOld(((R_Ctx*)(intptr_t)ctx)->env, name);
  }

  if (s_eq(ty, (long)(intptr_t)"List")) {

  u_failAt(s_concat(s_concat((long)(intptr_t)"an empty [] needs a type, as in ", name), (long)(intptr_t)" : List[Int] = []"), ctx);
  }

  old = u_envOld(((R_Ctx*)(intptr_t)ctx)->env, name);

  if ((!s_eq(old, (long)(intptr_t)"__gone__"))) {

  if ((!u_compatible(old, ty, ctx))) {

  u_failAt(s_concat(s_concat(s_concat(s_concat(s_concat(name, (long)(intptr_t)" is "), old), (long)(intptr_t)" and cannot become "), ty), (long)(intptr_t)"; use a new name"), ctx);
  }
  }

  return u_envPut(((R_Ctx*)(intptr_t)ctx)->env, name, ty);
  return 0;
}
long u_seedIf(long t, long el, long ctx) {

  u_seedEnv(t, ctx);

  u_seedEnv(el, ctx);

  return 0L;
  return 0;
}
long u_seedExpr(long e, long ctx) {

  switch (((Obj*)(intptr_t)e)->tag) {
  case T_Each: { long recv = ((Obj*)(intptr_t)e)->v0; long param = ((Obj*)(intptr_t)e)->v1; long body = ((Obj*)(intptr_t)e)->v2; return u_seedEach(recv, param, body, ctx); }
  case T_Num: { long v = ((Obj*)(intptr_t)e)->v0; return 0L; }
  case T_FloatLit: { long s = ((Obj*)(intptr_t)e)->v0; return 0L; }
  case T_Var: { long s = ((Obj*)(intptr_t)e)->v0; return 0L; }
  case T_StrLit: { long s = ((Obj*)(intptr_t)e)->v0; return 0L; }
  case T_ListLit: { long es = ((Obj*)(intptr_t)e)->v0; return 0L; }
  case T_Bin: { long op = ((Obj*)(intptr_t)e)->v0; long a = ((Obj*)(intptr_t)e)->v1; long b = ((Obj*)(intptr_t)e)->v2; return 0L; }
  case T_Call: { long name = ((Obj*)(intptr_t)e)->v0; long args = ((Obj*)(intptr_t)e)->v1; return 0L; }
  case T_Match: { long scrut = ((Obj*)(intptr_t)e)->v0; long arms = ((Obj*)(intptr_t)e)->v1; return 0L; }
  case T_Field: { long recv = ((Obj*)(intptr_t)e)->v0; long fld = ((Obj*)(intptr_t)e)->v1; return 0L; }
  case T_Method: { long recv = ((Obj*)(intptr_t)e)->v0; long name = ((Obj*)(intptr_t)e)->v1; long args = ((Obj*)(intptr_t)e)->v2; return 0L; }
  }
  return 0;
  return 0;
}
long u_seedEach(long recv, long param, long body, long ctx) {
  long old = 0;

  old = u_envOld(((R_Ctx*)(intptr_t)ctx)->env, param);

  u_envPut(((R_Ctx*)(intptr_t)ctx)->env, param, u_loopItem(u_exprType(recv, ctx)));

  u_seedEnv(body, ctx);

  u_envPut(((R_Ctx*)(intptr_t)ctx)->env, param, old);

  return 0L;
  return 0;
}
long u_inferType(long e, long ctx) {

  return u_exprType(e, ctx);
  return 0;
}
long u_letType(long ty, long e, long ctx) {
  long r = 0;

  r = u_inferType(e, ctx);

  if ((!u_typeKnown(ty, ((R_Ctx*)(intptr_t)ctx)->sigs))) {

  u_failAt(u_noType(ty), ctx);
  }

  if ((!s_eq(ty, (long)(intptr_t)""))) {

  r = ty;
  }

  return r;
  return 0;
}
long u_callRet(long name, long ctx) {
  long r = 0;
  long vt = 0;
  long k = 0;
  long m = 0;

  r = (long)(intptr_t)"Int";

  if (s_eq(name, (long)(intptr_t)"Map")) {

  r = (long)(intptr_t)"Map";
  }

  if (u_isMapType(name)) {

  r = name;
  }

  if (s_eq(name, (long)(intptr_t)"l_range")) {

  r = (long)(intptr_t)"List[Int]";
  }

  if (u_hasName(((R_Sigs*)(intptr_t)((R_Ctx*)(intptr_t)ctx)->sigs)->recNames, name)) {

  r = name;
  }

  vt = u_variantOf(name, ctx);

  if ((!s_eq(vt, (long)(intptr_t)""))) {

  r = vt;
  }

  k = 0L;

  m = l_len(((R_Sigs*)(intptr_t)((R_Ctx*)(intptr_t)ctx)->sigs)->fnNames);

  while ((k < m)) {

  if (s_eq(l_atc(((R_Sigs*)(intptr_t)((R_Ctx*)(intptr_t)ctx)->sigs)->fnNames, k, 2389), name)) {

  r = l_atc(((R_Sigs*)(intptr_t)((R_Ctx*)(intptr_t)ctx)->sigs)->fnRets, k, 2390);
  }

  k = (k + 1L);
  }

  return r;
  return 0;
}
long u_variantOf(long name, long ctx) {
  long r = 0;
  long types = 0;
  long k = 0;
  long m = 0;
  long t = 0;

  r = (long)(intptr_t)"";

  types = ((R_Sigs*)(intptr_t)((R_Ctx*)(intptr_t)ctx)->sigs)->types;

  k = 0L;

  m = l_len(types);

  while ((k < m)) {

  t = l_atc(types, k, 2404);

  if (u_hasCase(t, name)) {

  r = ((R_TyDef*)(intptr_t)t)->name;
  }

  k = (k + 1L);
  }

  return r;
  return 0;
}
long u_hasCase(long t, long name) {
  long found = 0;
  long k = 0;
  long m = 0;

  found = false;

  k = 0L;

  m = l_len(((R_TyDef*)(intptr_t)t)->cases);

  while ((k < m)) {

  if (s_eq(((R_Case*)(intptr_t)l_atc(((R_TyDef*)(intptr_t)t)->cases, k, 2418))->cname, name)) {

  found = true;
  }

  k = (k + 1L);
  }

  return found;
  return 0;
}
long u_isRec(long ty, long ctx) {

  return u_hasName(((R_Sigs*)(intptr_t)((R_Ctx*)(intptr_t)ctx)->sigs)->recNames, ty);
  return 0;
}
long u_collectLets(long body) {
  long names = 0;

  names = l_new();

  u_collectBody(body, names);

  return names;
  return 0;
}
long u_collectBody(long body, long names) {
  long k = 0;
  long m = 0;

  k = 0L;

  m = l_len(body);

  while ((k < m)) {

  u_collectStmt(l_atc(body, k, 2442), names);

  k = (k + 1L);
  }

  return 0L;
  return 0;
}
long u_collectStmt(long s, long names) {

  switch (((Obj*)(intptr_t)s)->tag) {
  case T_Let: { long name = ((Obj*)(intptr_t)s)->v0; long ty = ((Obj*)(intptr_t)s)->v1; long e = ((Obj*)(intptr_t)s)->v2; return u_pushUnique(names, name); }
  case T_Bare: { long e = ((Obj*)(intptr_t)s)->v0; return u_collectExpr(e, names); }
  case T_If: { long c = ((Obj*)(intptr_t)s)->v0; long t = ((Obj*)(intptr_t)s)->v1; long el = ((Obj*)(intptr_t)s)->v2; return u_collectIf(t, el, names); }
  case T_While: { long c = ((Obj*)(intptr_t)s)->v0; long b = ((Obj*)(intptr_t)s)->v1; return u_collectBody(b, names); }
  case T_Ret: { long e = ((Obj*)(intptr_t)s)->v0; return u_collectExpr(e, names); }
  case T_Line: { long n = ((Obj*)(intptr_t)s)->v0; return 0L; }
  case T_Jump: { long w = ((Obj*)(intptr_t)s)->v0; return 0L; }
  }
  return 0;
  return 0;
}
long u_collectExpr(long e, long names) {

  switch (((Obj*)(intptr_t)e)->tag) {
  case T_Each: { long recv = ((Obj*)(intptr_t)e)->v0; long param = ((Obj*)(intptr_t)e)->v1; long body = ((Obj*)(intptr_t)e)->v2; return u_collectBody(body, names); }
  case T_Num: { long v = ((Obj*)(intptr_t)e)->v0; return 0L; }
  case T_FloatLit: { long s = ((Obj*)(intptr_t)e)->v0; return 0L; }
  case T_Var: { long s = ((Obj*)(intptr_t)e)->v0; return 0L; }
  case T_StrLit: { long s = ((Obj*)(intptr_t)e)->v0; return 0L; }
  case T_ListLit: { long es = ((Obj*)(intptr_t)e)->v0; return 0L; }
  case T_Bin: { long op = ((Obj*)(intptr_t)e)->v0; long a = ((Obj*)(intptr_t)e)->v1; long b = ((Obj*)(intptr_t)e)->v2; return 0L; }
  case T_Call: { long name = ((Obj*)(intptr_t)e)->v0; long args = ((Obj*)(intptr_t)e)->v1; return 0L; }
  case T_Match: { long scrut = ((Obj*)(intptr_t)e)->v0; long arms = ((Obj*)(intptr_t)e)->v1; return 0L; }
  case T_Field: { long recv = ((Obj*)(intptr_t)e)->v0; long fld = ((Obj*)(intptr_t)e)->v1; return 0L; }
  case T_Method: { long recv = ((Obj*)(intptr_t)e)->v0; long name = ((Obj*)(intptr_t)e)->v1; long args = ((Obj*)(intptr_t)e)->v2; return 0L; }
  }
  return 0;
  return 0;
}
long u_collectIf(long t, long el, long names) {

  u_collectBody(t, names);

  u_collectBody(el, names);

  return 0L;
  return 0;
}
long u_pushUnique(long names, long name) {

  if ((!u_hasName(names, name))) {

  l_push(names, name);
  }

  return 0L;
  return 0;
}
long u_hasName(long names, long name) {
  long found = 0;
  long k = 0;
  long m = 0;

  found = false;

  k = 0L;

  m = l_len(names);

  while ((k < m)) {

  if (s_eq(l_atc(names, k, 2496), name)) {

  found = true;
  }

  k = (k + 1L);
  }

  return found;
  return 0;
}
long u_emitStmt(long s, long asReturn, long ctx) {

  switch (((Obj*)(intptr_t)s)->tag) {
  case T_Let: { long name = ((Obj*)(intptr_t)s)->v0; long ty = ((Obj*)(intptr_t)s)->v1; long e = ((Obj*)(intptr_t)s)->v2; return s_concat(({ long _t140 = s_concat(s_concat((long)(intptr_t)"  ", name), (long)(intptr_t)" = "); long _t141 = u_emitExpr(e, ctx); s_concat(_t140, _t141); }), (long)(intptr_t)";"); }
  case T_Bare: { long e = ((Obj*)(intptr_t)s)->v0; return u_emitBare(e, asReturn, ctx); }
  case T_If: { long c = ((Obj*)(intptr_t)s)->v0; long t = ((Obj*)(intptr_t)s)->v1; long el = ((Obj*)(intptr_t)s)->v2; return u_emitIf(c, t, el, asReturn, ctx); }
  case T_While: { long c = ((Obj*)(intptr_t)s)->v0; long b = ((Obj*)(intptr_t)s)->v1; return u_emitWhile(c, b, ctx); }
  case T_Ret: { long e = ((Obj*)(intptr_t)s)->v0; return u_emitRet(e, ctx); }
  case T_Line: { long n = ((Obj*)(intptr_t)s)->v0; return u_emitLine(n, ctx); }
  case T_Jump: { long w = ((Obj*)(intptr_t)s)->v0; return s_concat(s_concat((long)(intptr_t)"  ", w), (long)(intptr_t)";"); }
  }
  return 0;
  return 0;
}
long u_emitRet(long e, long ctx) {
  long ret = 0;

  ret = u_envGet(((R_Ctx*)(intptr_t)ctx)->env, (long)(intptr_t)"__ret__");

  if ((!s_eq(ret, (long)(intptr_t)"__main__"))) {

  u_checkRetExpr(e, ret, ctx);
  }

  return u_emitReturn(e, ctx);
  return 0;
}
long u_emitIf(long c, long t, long el, long asReturn, long ctx) {
  long cc = 0;
  long out = 0;

  cc = u_emitExpr(c, ctx);

  out = s_concat(({ long _t22 = s_concat(s_concat((long)(intptr_t)"  if (", cc), (long)(intptr_t)") {"); long _t23 = u_emitBlock(t, asReturn, ctx); s_concat(_t22, _t23); }), (long)(intptr_t)"\n  }");

  if ((l_len(el) > 0L)) {

  out = s_concat(({ long _t26 = s_concat(out, (long)(intptr_t)" else {"); long _t27 = u_emitBlock(el, asReturn, ctx); s_concat(_t26, _t27); }), (long)(intptr_t)"\n  }");
  }

  return out;
  return 0;
}
long u_emitWhile(long c, long b, long ctx) {
  long cc = 0;

  cc = u_emitExpr(c, ctx);

  return s_concat(({ long _t16 = s_concat(s_concat((long)(intptr_t)"  while (", cc), (long)(intptr_t)") {"); long _t17 = u_emitBlock(b, false, ctx); s_concat(_t16, _t17); }), (long)(intptr_t)"\n  }");
  return 0;
}
long u_emitBlock(long body, long asReturn, long ctx) {
  long out = 0;
  long k = 0;
  long m = 0;

  out = (long)(intptr_t)"";

  k = 0L;

  m = l_len(body);

  while ((k < m)) {

  out = ({ long _t25 = s_concat(out, (long)(intptr_t)"\n"); long _t26 = u_emitStmt(l_atc(body, k, 2550), (asReturn && ((k + 1L) == m)), ctx); s_concat(_t25, _t26); });

  k = (k + 1L);
  }

  return out;
  return 0;
}
long u_emitBare(long e, long asReturn, long ctx) {
  long r = 0;

  r = u_stmtExpr(e, ctx);

  if ((asReturn && (!u_stmtOnly(e)))) {

  r = u_emitReturn(e, ctx);
  }

  return r;
  return 0;
}
long u_stmtOnly(long e) {

  switch (((Obj*)(intptr_t)e)->tag) {
  case T_Each: { long recv = ((Obj*)(intptr_t)e)->v0; long param = ((Obj*)(intptr_t)e)->v1; long body = ((Obj*)(intptr_t)e)->v2; return true; }
  case T_Method: { long recv = ((Obj*)(intptr_t)e)->v0; long name = ((Obj*)(intptr_t)e)->v1; long args = ((Obj*)(intptr_t)e)->v2; return s_eq(name, (long)(intptr_t)"print"); }
  case T_Match: { long scrut = ((Obj*)(intptr_t)e)->v0; long arms = ((Obj*)(intptr_t)e)->v1; return false; }
  case T_Num: { long v = ((Obj*)(intptr_t)e)->v0; return false; }
  case T_FloatLit: { long s = ((Obj*)(intptr_t)e)->v0; return false; }
  case T_Var: { long s = ((Obj*)(intptr_t)e)->v0; return false; }
  case T_StrLit: { long s = ((Obj*)(intptr_t)e)->v0; return false; }
  case T_ListLit: { long es = ((Obj*)(intptr_t)e)->v0; return false; }
  case T_Bin: { long op = ((Obj*)(intptr_t)e)->v0; long a = ((Obj*)(intptr_t)e)->v1; long b = ((Obj*)(intptr_t)e)->v2; return false; }
  case T_Call: { long name = ((Obj*)(intptr_t)e)->v0; long args = ((Obj*)(intptr_t)e)->v1; return false; }
  case T_Field: { long recv = ((Obj*)(intptr_t)e)->v0; long fld = ((Obj*)(intptr_t)e)->v1; return false; }
  }
  return 0;
  return 0;
}
long u_stmtExpr(long e, long ctx) {

  switch (((Obj*)(intptr_t)e)->tag) {
  case T_Each: { long recv = ((Obj*)(intptr_t)e)->v0; long param = ((Obj*)(intptr_t)e)->v1; long body = ((Obj*)(intptr_t)e)->v2; return u_emitEach(recv, param, body, ctx); }
  case T_Call: { long name = ((Obj*)(intptr_t)e)->v0; long args = ((Obj*)(intptr_t)e)->v1; return u_emitCallStmt(name, args, e, ctx); }
  case T_Num: { long v = ((Obj*)(intptr_t)e)->v0; return u_exprStmt(e, ctx); }
  case T_FloatLit: { long s = ((Obj*)(intptr_t)e)->v0; return u_exprStmt(e, ctx); }
  case T_Var: { long s = ((Obj*)(intptr_t)e)->v0; return u_exprStmt(e, ctx); }
  case T_StrLit: { long s = ((Obj*)(intptr_t)e)->v0; return u_exprStmt(e, ctx); }
  case T_ListLit: { long es = ((Obj*)(intptr_t)e)->v0; return u_exprStmt(e, ctx); }
  case T_Bin: { long op = ((Obj*)(intptr_t)e)->v0; long a = ((Obj*)(intptr_t)e)->v1; long b = ((Obj*)(intptr_t)e)->v2; return u_exprStmt(e, ctx); }
  case T_Match: { long scrut = ((Obj*)(intptr_t)e)->v0; long arms = ((Obj*)(intptr_t)e)->v1; return u_exprStmt(e, ctx); }
  case T_Field: { long recv = ((Obj*)(intptr_t)e)->v0; long fld = ((Obj*)(intptr_t)e)->v1; return u_exprStmt(e, ctx); }
  case T_Method: { long recv = ((Obj*)(intptr_t)e)->v0; long name = ((Obj*)(intptr_t)e)->v1; long args = ((Obj*)(intptr_t)e)->v2; return u_emitMethodStmt(name, args, e, ctx); }
  }
  return 0;
  return 0;
}
long u_exprStmt(long e, long ctx) {

  return s_concat(s_concat((long)(intptr_t)"  ", u_emitExpr(e, ctx)), (long)(intptr_t)";");
  return 0;
}
long u_emitCallStmt(long name, long args, long e, long ctx) {
  long r = 0;

  r = s_concat(s_concat((long)(intptr_t)"  ", u_emitExpr(e, ctx)), (long)(intptr_t)";");

  if ((s_eq(name, (long)(intptr_t)"print") && (l_len(args) == 1L))) {

  r = s_concat(s_concat((long)(intptr_t)"  ", u_emitPrint(l_atc(args, 0L, 2608), ctx)), (long)(intptr_t)";");
  }

  return r;
  return 0;
}
long u_emitMethodStmt(long name, long args, long e, long ctx) {
  long r = 0;

  r = s_concat(s_concat((long)(intptr_t)"  ", u_emitExpr(e, ctx)), (long)(intptr_t)";");

  if (((s_eq(name, (long)(intptr_t)"print") && (l_len(args) == 1L)) && u_calm(l_atc(args, 0L, 2617)))) {

  r = s_concat(s_concat((long)(intptr_t)"  ", u_emitPrint(l_atc(args, 0L, 2618), ctx)), (long)(intptr_t)";");
  }

  return r;
  return 0;
}
long u_emitPrint(long arg, long ctx) {
  long fmt = 0;
  long cast = 0;
  long val = 0;
  long t = 0;

  fmt = (long)(intptr_t)"%ld";

  cast = (long)(intptr_t)"";

  val = u_emitExpr(arg, ctx);

  t = u_exprType(arg, ctx);

  if (s_eq(t, (long)(intptr_t)"Str")) {

  fmt = (long)(intptr_t)"%s";

  cast = (long)(intptr_t)"(const char*)(intptr_t)";
  }

  if (s_eq(t, (long)(intptr_t)"Float")) {

  fmt = (long)(intptr_t)"%s";

  cast = (long)(intptr_t)"(const char*)(intptr_t)";

  val = s_concat(s_concat((long)(intptr_t)"f_tostr(", val), (long)(intptr_t)")");
  }

  if (s_eq(t, (long)(intptr_t)"Bool")) {

  fmt = (long)(intptr_t)"%s";

  val = s_concat(s_concat((long)(intptr_t)"(", val), (long)(intptr_t)" ? \"true\" : \"false\")");
  }

  return s_concat(s_concat(s_concat(s_concat(s_concat((long)(intptr_t)"printf(\"", fmt), (long)(intptr_t)"\\n\", "), cast), val), (long)(intptr_t)")");
  return 0;
}
long u_emitEach(long recv, long param, long body, long ctx) {
  long recvC = 0;
  long old = 0;
  long rt = 0;
  long len = 0;
  long at = 0;
  long out = 0;

  recvC = u_emitExpr(recv, ctx);

  old = u_envOld(((R_Ctx*)(intptr_t)ctx)->env, param);

  u_envPut(((R_Ctx*)(intptr_t)ctx)->env, param, u_loopItem(u_exprType(recv, ctx)));

  rt = u_exprType(recv, ctx);

  len = (long)(intptr_t)"l_len";

  at = (long)(intptr_t)"l_at";

  if (s_eq(rt, (long)(intptr_t)"Str")) {

  len = (long)(intptr_t)"s_len";

  at = (long)(intptr_t)"s_at";
  }

  if (((!s_eq(rt, (long)(intptr_t)"Str")) && (!u_isListType(rt)))) {

  u_failAt(s_concat((long)(intptr_t)"a loop goes over a list or the letters of a text, not ", rt), ctx);
  }

  out = (long)(intptr_t)"  {";

  out = s_concat(s_concat(s_concat(out, (long)(intptr_t)"\n  long _lst = "), recvC), (long)(intptr_t)";");

  if (s_eq(rt, (long)(intptr_t)"Str")) {

  out = s_concat(out, (long)(intptr_t)"\n  long _b = 0;");

  out = s_concat(out, (long)(intptr_t)"\n  while (((const char*)(intptr_t)_lst)[_b]) {");

  out = s_concat(s_concat(s_concat(out, (long)(intptr_t)"\n  long "), param), (long)(intptr_t)" = s_each(_lst, &_b);");
  } else {

  out = s_concat(s_concat(s_concat(out, (long)(intptr_t)"\n  long _n = "), len), (long)(intptr_t)"(_lst);");

  out = s_concat(out, (long)(intptr_t)"\n  for (long _i = 0; _i < _n; _i = _i + 1) {");

  out = s_concat(s_concat(s_concat(s_concat(s_concat(out, (long)(intptr_t)"\n  long "), param), (long)(intptr_t)" = "), at), (long)(intptr_t)"(_lst, _i);");
  }

  out = s_concat(out, u_emitBlock(body, false, ctx));

  u_envPut(((R_Ctx*)(intptr_t)ctx)->env, param, old);

  out = s_concat(out, (long)(intptr_t)"\n  }");

  out = s_concat(out, (long)(intptr_t)"\n  }");

  return out;
  return 0;
}
long u_isListType(long t) {
  long r = 0;

  r = false;

  if ((s_len(t) > 3L)) {

  if (s_eq(s_slicec(t, 0L, 4L, 2685), (long)(intptr_t)"List")) {

  r = true;
  }
  }

  return r;
  return 0;
}
long u_loopItem(long t) {
  long r = 0;

  r = u_elemOf(t);

  if (s_eq(t, (long)(intptr_t)"Str")) {

  r = (long)(intptr_t)"Str";
  }

  return r;
  return 0;
}
long u_elemOf(long t) {
  long r = 0;

  r = (long)(intptr_t)"Int";

  if (u_isListType(t)) {

  r = s_slicec(t, 5L, (s_len(t) - 1L), 2704);
  }

  return r;
  return 0;
}
long u_isMapType(long t) {
  long r = 0;

  r = false;

  if ((s_len(t) > 4L)) {

  if (s_eq(s_slicec(t, 0L, 4L, 2713), (long)(intptr_t)"Map[")) {

  r = true;
  }
  }

  return r;
  return 0;
}
long u_mapValueOf(long t) {
  long r = 0;

  r = (long)(intptr_t)"Int";

  if (u_isMapType(t)) {

  r = s_slicec(t, 4L, (s_len(t) - 1L), 2723);
  }

  return r;
  return 0;
}
long u_emitReturn(long e, long ctx) {

  switch (((Obj*)(intptr_t)e)->tag) {
  case T_Match: { long scrut = ((Obj*)(intptr_t)e)->v0; long arms = ((Obj*)(intptr_t)e)->v1; return u_emitSwitch(scrut, arms, ctx); }
  case T_Num: { long v = ((Obj*)(intptr_t)e)->v0; return u_retVal(e, ctx); }
  case T_FloatLit: { long s = ((Obj*)(intptr_t)e)->v0; return u_retVal(e, ctx); }
  case T_Var: { long s = ((Obj*)(intptr_t)e)->v0; return u_retVal(e, ctx); }
  case T_StrLit: { long s = ((Obj*)(intptr_t)e)->v0; return u_retVal(e, ctx); }
  case T_ListLit: { long es = ((Obj*)(intptr_t)e)->v0; return u_retVal(e, ctx); }
  case T_Bin: { long op = ((Obj*)(intptr_t)e)->v0; long a = ((Obj*)(intptr_t)e)->v1; long b = ((Obj*)(intptr_t)e)->v2; return u_retVal(e, ctx); }
  case T_Call: { long name = ((Obj*)(intptr_t)e)->v0; long args = ((Obj*)(intptr_t)e)->v1; return u_retVal(e, ctx); }
  case T_Field: { long recv = ((Obj*)(intptr_t)e)->v0; long fld = ((Obj*)(intptr_t)e)->v1; return u_retVal(e, ctx); }
  case T_Method: { long recv = ((Obj*)(intptr_t)e)->v0; long name = ((Obj*)(intptr_t)e)->v1; long args = ((Obj*)(intptr_t)e)->v2; return u_retVal(e, ctx); }
  case T_Each: { long recv = ((Obj*)(intptr_t)e)->v0; long param = ((Obj*)(intptr_t)e)->v1; long body = ((Obj*)(intptr_t)e)->v2; return u_retVal(e, ctx); }
  }
  return 0;
  return 0;
}
long u_retVal(long e, long ctx) {

  return s_concat(s_concat((long)(intptr_t)"  return ", u_emitExpr(e, ctx)), (long)(intptr_t)";");
  return 0;
}
long u_emitSwitch(long scrut, long arms, long ctx) {
  long boxed = 0;
  long scrutC = 0;
  long styp = 0;
  long head = 0;
  long out = 0;
  long k = 0;
  long m = 0;

  boxed = u_armHasBind(arms);

  scrutC = u_emitExpr(scrut, ctx);

  styp = u_exprType(scrut, ctx);

  u_checkExhaustive(styp, arms, ctx);

  head = scrutC;

  if (boxed) {

  head = u_boxField(scrutC, (long)(intptr_t)"tag");
  }

  out = s_concat(s_concat((long)(intptr_t)"  switch (", head), (long)(intptr_t)") {");

  k = 0L;

  m = l_len(arms);

  while ((k < m)) {

  out = ({ long _t47 = s_concat(out, (long)(intptr_t)"\n"); long _t48 = u_emitArm(l_atc(arms, k, 2761), scrutC, boxed, styp, (long)(intptr_t)"return ", (long)(intptr_t)";", ctx); s_concat(_t47, _t48); });

  k = (k + 1L);
  }

  out = s_concat(out, (long)(intptr_t)"\n  }");

  return s_concat(out, (long)(intptr_t)"\n  return 0;");
  return 0;
}
long u_emitMatchExpr(long scrut, long arms, long ctx) {
  long boxed = 0;
  long scrutC = 0;
  long styp = 0;
  long head = 0;
  long out = 0;
  long k = 0;
  long m = 0;

  boxed = u_armHasBind(arms);

  scrutC = u_emitExpr(scrut, ctx);

  styp = u_exprType(scrut, ctx);

  u_checkExhaustive(styp, arms, ctx);

  head = scrutC;

  if (boxed) {

  head = u_boxField(scrutC, (long)(intptr_t)"tag");
  }

  out = s_concat(s_concat((long)(intptr_t)"({ long _m = 0; switch (", head), (long)(intptr_t)") {");

  k = 0L;

  m = l_len(arms);

  while ((k < m)) {

  out = s_concat(out, u_emitArm(l_atc(arms, k, 2785), scrutC, boxed, styp, (long)(intptr_t)"_m = ", (long)(intptr_t)"; break;", ctx));

  k = (k + 1L);
  }

  return s_concat(out, (long)(intptr_t)" } _m; })");
  return 0;
}
long u_emitArm(long a, long scrutC, long boxed, long styp, long pre, long post, long ctx) {
  long r = 0;
  long olds = 0;
  long decls = 0;
  long bi = 0;

  r = (long)(intptr_t)"";

  olds = u_envOlds(((R_Ctx*)(intptr_t)ctx)->env, ((R_Arm*)(intptr_t)a)->binds);

  if (boxed) {

  decls = (long)(intptr_t)"";

  bi = 0L;

  while ((bi < l_len(((R_Arm*)(intptr_t)a)->binds))) {

  ({ long _t40 = l_atc(((R_Arm*)(intptr_t)a)->binds, bi, 2800); long _t41 = u_payloadType(styp, ((R_Arm*)(intptr_t)a)->tag, bi, ctx); u_envPut(((R_Ctx*)(intptr_t)ctx)->env, _t40, _t41); });

  decls = s_concat(({ long _t45 = s_concat(({ long _t43 = s_concat(decls, (long)(intptr_t)" long "); long _t44 = l_atc(((R_Arm*)(intptr_t)a)->binds, bi, 2801); s_concat(_t43, _t44); }), (long)(intptr_t)" = "); long _t46 = u_boxField(scrutC, u_slotName(bi)); s_concat(_t45, _t46); }), (long)(intptr_t)";");

  bi = (bi + 1L);
  }

  r = s_concat(s_concat(({ long _t49 = s_concat(s_concat(s_concat(s_concat(s_concat((long)(intptr_t)"  case T_", ((R_Arm*)(intptr_t)a)->tag), (long)(intptr_t)": {"), decls), (long)(intptr_t)" "), pre); long _t50 = u_emitExpr(((R_Arm*)(intptr_t)a)->body, ctx); s_concat(_t49, _t50); }), post), (long)(intptr_t)" }");
  } else {

  r = s_concat(({ long _t52 = s_concat(s_concat(s_concat((long)(intptr_t)"  case ", ((R_Arm*)(intptr_t)a)->tag), (long)(intptr_t)": "), pre); long _t53 = u_emitExpr(((R_Arm*)(intptr_t)a)->body, ctx); s_concat(_t52, _t53); }), post);
  }

  u_envRestore(((R_Ctx*)(intptr_t)ctx)->env, ((R_Arm*)(intptr_t)a)->binds, olds);

  return r;
  return 0;
}
long u_payloadType(long tyName, long caseTag, long idx, long ctx) {
  long r = 0;
  long types = 0;
  long k = 0;
  long m = 0;
  long t = 0;

  r = (long)(intptr_t)"Int";

  types = ((R_Sigs*)(intptr_t)((R_Ctx*)(intptr_t)ctx)->sigs)->types;

  k = 0L;

  m = l_len(types);

  while ((k < m)) {

  t = l_atc(types, k, 2819);

  if (s_eq(((R_TyDef*)(intptr_t)t)->name, tyName)) {

  r = u_casePayloadType(t, caseTag, idx);
  }

  k = (k + 1L);
  }

  return r;
  return 0;
}
long u_casePayloadType(long t, long caseTag, long idx) {
  long r = 0;
  long k = 0;
  long m = 0;
  long c = 0;

  r = (long)(intptr_t)"Int";

  k = 0L;

  m = l_len(((R_TyDef*)(intptr_t)t)->cases);

  while ((k < m)) {

  c = l_atc(((R_TyDef*)(intptr_t)t)->cases, k, 2833);

  if (s_eq(((R_Case*)(intptr_t)c)->cname, caseTag)) {

  if ((idx < l_len(((R_Case*)(intptr_t)c)->ptypes))) {

  r = l_atc(((R_Case*)(intptr_t)c)->ptypes, idx, 2836);
  }
  }

  k = (k + 1L);
  }

  return r;
  return 0;
}
long u_boxField(long scrutC, long field) {

  return s_concat(s_concat(s_concat((long)(intptr_t)"((Obj*)(intptr_t)", scrutC), (long)(intptr_t)")->"), field);
  return 0;
}
long u_checkExhaustive(long styp, long arms, long ctx) {
  long types = 0;
  long k = 0;
  long m = 0;
  long t = 0;

  types = ((R_Sigs*)(intptr_t)((R_Ctx*)(intptr_t)ctx)->sigs)->types;

  k = 0L;

  m = l_len(types);

  while ((k < m)) {

  t = l_atc(types, k, 2855);

  if (s_eq(((R_TyDef*)(intptr_t)t)->name, styp)) {

  u_checkCases(t, arms, ctx);

  u_checkArities(t, arms, ctx);
  }

  k = (k + 1L);
  }

  return 0L;
  return 0;
}
long u_checkArities(long t, long arms, long ctx) {
  long k = 0;
  long m = 0;
  long a = 0;
  long ar = 0;

  k = 0L;

  m = l_len(arms);

  while ((k < m)) {

  a = l_atc(arms, k, 2870);

  ar = u_caseArityIn(t, ((R_Arm*)(intptr_t)a)->tag);

  if ((ar < 0L)) {

  u_failAt(({ long _t34 = s_concat(((R_TyDef*)(intptr_t)t)->name, (long)(intptr_t)" has no case "); long _t35 = ((R_Arm*)(intptr_t)a)->tag; s_concat(_t34, _t35); }), ctx);
  }

  if ((!(ar < 0L))) {

  if ((!(l_len(((R_Arm*)(intptr_t)a)->binds) == ar))) {

  u_failAt(({ long _t41 = s_concat(({ long _t39 = s_concat(s_concat((long)(intptr_t)"match case ", ((R_Arm*)(intptr_t)a)->tag), (long)(intptr_t)" binds "); long _t40 = i_tostr(l_len(((R_Arm*)(intptr_t)a)->binds)); s_concat(_t39, _t40); }), (long)(intptr_t)", expected "); long _t42 = i_tostr(ar); s_concat(_t41, _t42); }), ctx);
  }
  }

  k = (k + 1L);
  }

  return 0L;
  return 0;
}
long u_setLine(long n, long ctx) {

  return u_envPut(((R_Ctx*)(intptr_t)ctx)->env, (long)(intptr_t)"__line__", i_tostr(n));
  return 0;
}
long u_emitLine(long n, long ctx) {

  u_setLine(n, ctx);

  return (long)(intptr_t)"";
  return 0;
}
long u_failAt(long msg, long ctx) {

  return fail(({ long _t14 = ({ long _t12 = ((R_Sigs*)(intptr_t)((R_Ctx*)(intptr_t)ctx)->sigs)->files; long _t13 = s_tointc(u_envGet(((R_Ctx*)(intptr_t)ctx)->env, (long)(intptr_t)"__line__"), 2897); u_spot(_t12, _t13); }); long _t15 = s_replace(msg, (long)(intptr_t)"__", (long)(intptr_t)"."); s_concat(_t14, _t15); }));
  return 0;
}
long u_plainType(long t) {

  return (((((((((((((s_eq(t, (long)(intptr_t)"Int") || s_eq(t, (long)(intptr_t)"Str")) || s_eq(t, (long)(intptr_t)"Bool")) || s_eq(t, (long)(intptr_t)"Float")) || s_eq(t, (long)(intptr_t)"Screen")) || s_eq(t, (long)(intptr_t)"Files")) || s_eq(t, (long)(intptr_t)"Mail")) || s_eq(t, (long)(intptr_t)"Programs")) || s_eq(t, (long)(intptr_t)"Net")) || s_eq(t, (long)(intptr_t)"Clock")) || s_eq(t, (long)(intptr_t)"Env")) || s_eq(t, (long)(intptr_t)"Term")) || s_eq(t, (long)(intptr_t)"Bytes")) || s_eq(t, (long)(intptr_t)"Sys"));
  return 0;
}
long u_isHanded(long s, long ctx) {

  return ({ long _t10 = ((R_TyEnv*)(intptr_t)((R_Ctx*)(intptr_t)ctx)->env)->names; long _t11 = s_concat((long)(intptr_t)"__h__", s); u_hasName(_t10, _t11); });
  return 0;
}
long u_holdOf(long s, long ctx) {
  long r = 0;

  r = 0L;

  if (({ long _t14 = ((R_TyEnv*)(intptr_t)((R_Ctx*)(intptr_t)ctx)->env)->names; long _t15 = s_concat((long)(intptr_t)"__i__", s); u_hasName(_t14, _t15); })) {

  r = s_tointc(u_envGet(((R_Ctx*)(intptr_t)ctx)->env, s_concat((long)(intptr_t)"__i__", s)), 2925);
  }

  return r;
  return 0;
}
long u_setHolding(long s, long d, long ctx) {
  long cur = 0;

  if (((!s_eq(s, (long)(intptr_t)"")) && (!u_isHanded(s, ctx)))) {

  cur = u_holdOf(s, ctx);

  if (((cur == 0L) || (d < cur))) {

  ({ long _t17 = s_concat((long)(intptr_t)"__i__", s); long _t18 = i_tostr(d); u_envPut(((R_Ctx*)(intptr_t)ctx)->env, _t17, _t18); });
  }
  }

  return 0L;
  return 0;
}
long u_oneLess(long d) {
  long r = 0;

  r = 0L;

  if ((d > 1L)) {

  r = (d - 1L);
  }

  return r;
  return 0;
}
long u_holdDepth(long e, long ctx) {

  switch (((Obj*)(intptr_t)e)->tag) {
  case T_Var: { long s = ((Obj*)(intptr_t)e)->v0; return u_holdOf(s, ctx); }
  case T_Field: { long recv = ((Obj*)(intptr_t)e)->v0; long fld = ((Obj*)(intptr_t)e)->v1; return u_oneLess(u_holdDepth(recv, ctx)); }
  case T_Method: { long recv = ((Obj*)(intptr_t)e)->v0; long name = ((Obj*)(intptr_t)e)->v1; long args = ((Obj*)(intptr_t)e)->v2; return u_oneLess(u_holdDepth(recv, ctx)); }
  case T_ListLit: { long es = ((Obj*)(intptr_t)e)->v0; return (1L + u_minLevel(es, ctx)); }
  case T_Call: { long name = ((Obj*)(intptr_t)e)->v0; long args = ((Obj*)(intptr_t)e)->v1; return 0L; }
  case T_Match: { long scrut = ((Obj*)(intptr_t)e)->v0; long arms = ((Obj*)(intptr_t)e)->v1; return 0L; }
  case T_Bin: { long op = ((Obj*)(intptr_t)e)->v0; long a = ((Obj*)(intptr_t)e)->v1; long b = ((Obj*)(intptr_t)e)->v2; return 0L; }
  case T_Each: { long recv = ((Obj*)(intptr_t)e)->v0; long param = ((Obj*)(intptr_t)e)->v1; long body = ((Obj*)(intptr_t)e)->v2; return 0L; }
  case T_Num: { long v = ((Obj*)(intptr_t)e)->v0; return 0L; }
  case T_FloatLit: { long s = ((Obj*)(intptr_t)e)->v0; return 0L; }
  case T_StrLit: { long s = ((Obj*)(intptr_t)e)->v0; return 0L; }
  }
  return 0;
  return 0;
}
long u_minLevel(long es, long ctx) {
  long r = 0;
  long l = 0;
  long d = 0;

  r = (0L - 1L);

  {
  long _lst = es;
  long _n = l_len(_lst);
  for (long _i = 0; _i < _n; _i = _i + 1) {
  long x = l_at(_lst, _i);

  l = (0L - 1L);

  if (u_handed(x, ctx)) {

  l = 0L;
  } else {

  d = u_holdDepth(x, ctx);

  if ((d > 0L)) {

  l = d;
  }
  }

  if (((l >= 0L) && ((r < 0L) || (l < r)))) {

  r = l;
  }
  }
  }

  return r;
  return 0;
}
long u_pathLen(long e) {

  switch (((Obj*)(intptr_t)e)->tag) {
  case T_Field: { long recv = ((Obj*)(intptr_t)e)->v0; long fld = ((Obj*)(intptr_t)e)->v1; return (1L + u_pathLen(recv)); }
  case T_Method: { long recv = ((Obj*)(intptr_t)e)->v0; long name = ((Obj*)(intptr_t)e)->v1; long args = ((Obj*)(intptr_t)e)->v2; return (1L + u_pathLen(recv)); }
  case T_Var: { long s = ((Obj*)(intptr_t)e)->v0; return 0L; }
  case T_ListLit: { long es = ((Obj*)(intptr_t)e)->v0; return 0L; }
  case T_Call: { long name = ((Obj*)(intptr_t)e)->v0; long args = ((Obj*)(intptr_t)e)->v1; return 0L; }
  case T_Match: { long scrut = ((Obj*)(intptr_t)e)->v0; long arms = ((Obj*)(intptr_t)e)->v1; return 0L; }
  case T_Bin: { long op = ((Obj*)(intptr_t)e)->v0; long a = ((Obj*)(intptr_t)e)->v1; long b = ((Obj*)(intptr_t)e)->v2; return 0L; }
  case T_Each: { long recv = ((Obj*)(intptr_t)e)->v0; long param = ((Obj*)(intptr_t)e)->v1; long body = ((Obj*)(intptr_t)e)->v2; return 0L; }
  case T_Num: { long v = ((Obj*)(intptr_t)e)->v0; return 0L; }
  case T_FloatLit: { long s = ((Obj*)(intptr_t)e)->v0; return 0L; }
  case T_StrLit: { long s = ((Obj*)(intptr_t)e)->v0; return 0L; }
  }
  return 0;
  return 0;
}
long u_setHanded(long s, long ctx) {

  if ((!u_isHanded(s, ctx))) {

  u_envPut(((R_Ctx*)(intptr_t)ctx)->env, s_concat((long)(intptr_t)"__h__", s), (long)(intptr_t)"1");
  }

  return 0L;
  return 0;
}
long u_fixedType(long t, long ctx, long depth) {

  return u_plainType(t);
  return 0;
}
long u_handed(long e, long ctx) {

  if (u_fixedType(u_exprType(e, ctx), ctx, 0L)) {

  return false;
  }

  switch (((Obj*)(intptr_t)e)->tag) {
  case T_Var: { long s = ((Obj*)(intptr_t)e)->v0; return u_isHanded(s, ctx); }
  case T_Field: { long recv = ((Obj*)(intptr_t)e)->v0; long fld = ((Obj*)(intptr_t)e)->v1; return (u_handedField(recv, fld, ctx) || (u_holdDepth(recv, ctx) == 1L)); }
  case T_Method: { long recv = ((Obj*)(intptr_t)e)->v0; long name = ((Obj*)(intptr_t)e)->v1; long args = ((Obj*)(intptr_t)e)->v2; return ((s_eq(name, (long)(intptr_t)"at") || s_eq(name, (long)(intptr_t)"get")) && (u_handed(recv, ctx) || (u_holdDepth(recv, ctx) == 1L))); }
  case T_ListLit: { long es = ((Obj*)(intptr_t)e)->v0; return false; }
  case T_Call: { long name = ((Obj*)(intptr_t)e)->v0; long args = ((Obj*)(intptr_t)e)->v1; return u_callHanded(name, args, ctx); }
  case T_Match: { long scrut = ((Obj*)(intptr_t)e)->v0; long arms = ((Obj*)(intptr_t)e)->v1; return u_armsHanded(scrut, arms, ctx); }
  case T_Bin: { long op = ((Obj*)(intptr_t)e)->v0; long a = ((Obj*)(intptr_t)e)->v1; long b = ((Obj*)(intptr_t)e)->v2; return false; }
  case T_Each: { long recv = ((Obj*)(intptr_t)e)->v0; long param = ((Obj*)(intptr_t)e)->v1; long body = ((Obj*)(intptr_t)e)->v2; return false; }
  case T_Num: { long v = ((Obj*)(intptr_t)e)->v0; return false; }
  case T_FloatLit: { long s = ((Obj*)(intptr_t)e)->v0; return false; }
  case T_StrLit: { long s = ((Obj*)(intptr_t)e)->v0; return false; }
  }
  return 0;
  return 0;
}
long u_handedField(long recv, long fld, long ctx) {

  if (((((s_eq(fld, (long)(intptr_t)"keys") || s_eq(fld, (long)(intptr_t)"byValue")) || s_eq(fld, (long)(intptr_t)"jsonItems")) || s_eq(fld, (long)(intptr_t)"jsonKeys")) || s_eq(fld, (long)(intptr_t)"bytes"))) {

  return false;
  }

  if (s_eq(fld, (long)(intptr_t)"sort")) {

  if (u_plainType(u_elemOf(u_exprType(recv, ctx)))) {

  return false;
  }
  }

  if (u_useUFCS(u_exprType(recv, ctx), fld, ctx)) {

  return u_callHanded(fld, l_with(l_new(), recv), ctx);
  }

  return u_handed(recv, ctx);
  return 0;
}
long u_callHanded(long name, long args, long ctx) {
  long r = 0;
  long j = 0;

  if ((!u_hasName(((R_Sigs*)(intptr_t)((R_Ctx*)(intptr_t)ctx)->sigs)->fnNames, name))) {

  return u_anyHanded(args, ctx);
  }

  r = false;

  j = 0L;

  while ((j < l_len(args))) {

  if ((({ long _t27 = ((R_Sigs*)(intptr_t)((R_Ctx*)(intptr_t)ctx)->sigs)->retP; long _t30 = ({ long _t28 = s_concat(name, (long)(intptr_t)":"); long _t29 = i_tostr(j); s_concat(_t28, _t29); }); u_hasName(_t27, _t30); }) && u_handed(l_atc(args, j, 3060), ctx))) {

  r = true;
  }

  j = (j + 1L);
  }

  return r;
  return 0;
}
long u_markFn(long f, long ctx) {
  long k = 0;

  k = 0L;

  while ((k < l_len(((R_Fn*)(intptr_t)f)->params))) {

  if ((!u_plainType(l_atc(((R_Fn*)(intptr_t)f)->ptypes, k, 3071)))) {

  u_setHanded(l_atc(((R_Fn*)(intptr_t)f)->params, k, 3072), ctx);
  }

  k = (k + 1L);
  }

  return u_markAgain(f, ctx);
  return 0;
}
long u_markAgain(long f, long ctx) {

  u_markBody(((R_Fn*)(intptr_t)f)->body, ctx);

  u_markBody(((R_Fn*)(intptr_t)f)->body, ctx);

  return u_markBody(((R_Fn*)(intptr_t)f)->body, ctx);
  return 0;
}
long u_paramFacts(long f, long sigs) {
  long found = 0;
  long k = 0;
  long key = 0;
  long needR = 0;
  long needM = 0;
  long ctx = 0;
  long used = 0;

  found = false;

  k = 0L;

  while ((k < l_len(((R_Fn*)(intptr_t)f)->params))) {

  key = ({ long _t42 = s_concat(((R_Fn*)(intptr_t)f)->name, (long)(intptr_t)":"); long _t43 = i_tostr(k); s_concat(_t42, _t43); });

  needR = ((!u_plainType(((R_Fn*)(intptr_t)f)->ret)) && (!u_hasName(((R_Sigs*)(intptr_t)sigs)->retP, key)));

  needM = (u_hasName(((R_Fn*)(intptr_t)f)->effs, (long)(intptr_t)"Mut") && (!u_hasName(((R_Sigs*)(intptr_t)sigs)->mutP, key)));

  if (((!u_plainType(l_atc(((R_Fn*)(intptr_t)f)->ptypes, k, 3097))) && (needR || needM))) {

  ctx = u_fnCtx(f, sigs);

  u_setHanded(l_atc(((R_Fn*)(intptr_t)f)->params, k, 3099), ctx);

  u_markAgain(f, ctx);

  if ((needR && (u_lastHanded(((R_Fn*)(intptr_t)f)->body, ctx) || u_anyRets(((R_Fn*)(intptr_t)f)->body, ctx)))) {

  l_push(((R_Sigs*)(intptr_t)sigs)->retP, key);

  found = true;
  }

  used = l_new();

  u_effBody(((R_Fn*)(intptr_t)f)->body, used, ctx);

  if ((needM && u_hasName(used, (long)(intptr_t)"Mut"))) {

  l_push(((R_Sigs*)(intptr_t)sigs)->mutP, key);

  found = true;
  }
  }

  k = (k + 1L);
  }

  return found;
  return 0;
}
long u_typeBase(long t) {
  long r = 0;

  r = t;

  if ((s_len(t) > 5L)) {

  if (s_eq(s_slicec(t, 0L, 5L, 3121), (long)(intptr_t)"List[")) {

  r = u_typeBase(s_slicec(t, 5L, (s_len(t) - 1L), 3122));
  }
  }

  if ((s_len(t) > 4L)) {

  if (s_eq(s_slicec(t, 0L, 4L, 3126), (long)(intptr_t)"Map[")) {

  r = u_typeBase(s_slicec(t, 4L, (s_len(t) - 1L), 3127));
  }
  }

  return r;
  return 0;
}
long u_typeKnown(long t, long sigs) {
  long b = 0;
  long r = 0;

  b = u_typeBase(t);

  r = ((((s_eq(b, (long)(intptr_t)"") || u_plainType(b)) || s_eq(b, (long)(intptr_t)"List")) || s_eq(b, (long)(intptr_t)"Map")) || u_hasName(((R_Sigs*)(intptr_t)sigs)->recNames, b));

  {
  long _lst = ((R_Sigs*)(intptr_t)sigs)->types;
  long _n = l_len(_lst);
  for (long _i = 0; _i < _n; _i = _i + 1) {
  long td = l_at(_lst, _i);

  if (s_eq(((R_TyDef*)(intptr_t)td)->name, b)) {

  r = true;
  }
  }
  }

  return r;
  return 0;
}
long u_noType(long t) {
  long b = 0;
  long cut = 0;

  b = u_typeBase(t);

  if ((s_len(b) > 2L)) {

  cut = s_slicec(b, 0L, (s_len(b) - 2L), 3148);

  if (s_eq(u_safeName(cut, u_cTaken()), b)) {

  b = cut;
  }
  }

  return s_concat(s_concat((long)(intptr_t)"no type named ", b), (long)(intptr_t)"; the types are Int, Float, Str, Bool, List[T], Map, Map[T] and the ones the program declares");
  return 0;
}
long u_checkTypes(long prog, long sigs) {

  {
  long _lst = ((R_Prog*)(intptr_t)prog)->records;
  long _n = l_len(_lst);
  for (long _i = 0; _i < _n; _i = _i + 1) {
  long r = l_at(_lst, _i);

  {
  long _lst = ((R_RecDef*)(intptr_t)r)->ftypes;
  long _n = l_len(_lst);
  for (long _i = 0; _i < _n; _i = _i + 1) {
  long t = l_at(_lst, _i);

  if ((!u_typeKnown(t, sigs))) {

  fail(({ long _t103 = u_spot(((R_Sigs*)(intptr_t)sigs)->files, ((R_RecDef*)(intptr_t)r)->line); long _t104 = u_noType(t); s_concat(_t103, _t104); }));
  }
  }
  }
  }
  }

  {
  long _lst = ((R_Prog*)(intptr_t)prog)->types;
  long _n = l_len(_lst);
  for (long _i = 0; _i < _n; _i = _i + 1) {
  long td = l_at(_lst, _i);

  {
  long _lst = ((R_TyDef*)(intptr_t)td)->cases;
  long _n = l_len(_lst);
  for (long _i = 0; _i < _n; _i = _i + 1) {
  long c = l_at(_lst, _i);

  {
  long _lst = ((R_Case*)(intptr_t)c)->ptypes;
  long _n = l_len(_lst);
  for (long _i = 0; _i < _n; _i = _i + 1) {
  long t = l_at(_lst, _i);

  if ((!u_typeKnown(t, sigs))) {

  fail(({ long _t115 = u_spot(((R_Sigs*)(intptr_t)sigs)->files, ((R_TyDef*)(intptr_t)td)->line); long _t116 = u_noType(t); s_concat(_t115, _t116); }));
  }
  }
  }
  }
  }
  }
  }

  {
  long _lst = ((R_Prog*)(intptr_t)prog)->fns;
  long _n = l_len(_lst);
  for (long _i = 0; _i < _n; _i = _i + 1) {
  long f = l_at(_lst, _i);

  {
  long _lst = ((R_Fn*)(intptr_t)f)->ptypes;
  long _n = l_len(_lst);
  for (long _i = 0; _i < _n; _i = _i + 1) {
  long t = l_at(_lst, _i);

  if ((!u_typeKnown(t, sigs))) {

  fail(({ long _t126 = u_spot(((R_Sigs*)(intptr_t)sigs)->files, ((R_Fn*)(intptr_t)f)->line); long _t127 = u_noType(t); s_concat(_t126, _t127); }));
  }
  }
  }

  if ((!u_typeKnown(((R_Fn*)(intptr_t)f)->ret, sigs))) {

  fail(({ long _t131 = u_spot(((R_Sigs*)(intptr_t)sigs)->files, ((R_Fn*)(intptr_t)f)->line); long _t132 = u_noType(((R_Fn*)(intptr_t)f)->ret); s_concat(_t131, _t132); }));
  }
  }
  }

  return 0L;
  return 0;
}
long u_jumpBody(long fl, long body, long inLoop) {
  long line = 0;

  line = 0L;

  {
  long _lst = body;
  long _n = l_len(_lst);
  for (long _i = 0; _i < _n; _i = _i + 1) {
  long s = l_at(_lst, _i);

  line = u_jumpStmt(fl, s, inLoop, line);
  }
  }

  return 0L;
  return 0;
}
long u_jumpStmt(long fl, long s, long inLoop, long line) {

  switch (((Obj*)(intptr_t)s)->tag) {
  case T_Let: { long name = ((Obj*)(intptr_t)s)->v0; long ty = ((Obj*)(intptr_t)s)->v1; long e = ((Obj*)(intptr_t)s)->v2; return line; }
  case T_Bare: { long e = ((Obj*)(intptr_t)s)->v0; return u_jumpExpr(fl, e, inLoop, line); }
  case T_If: { long c = ((Obj*)(intptr_t)s)->v0; long t = ((Obj*)(intptr_t)s)->v1; long el = ((Obj*)(intptr_t)s)->v2; return ({ long _t149 = (line + u_jumpBody(fl, t, inLoop)); long _t150 = u_jumpBody(fl, el, inLoop); (_t149 + _t150); }); }
  case T_While: { long c = ((Obj*)(intptr_t)s)->v0; long b = ((Obj*)(intptr_t)s)->v1; return (line + u_jumpBody(fl, b, true)); }
  case T_Ret: { long e = ((Obj*)(intptr_t)s)->v0; return line; }
  case T_Line: { long n = ((Obj*)(intptr_t)s)->v0; return n; }
  case T_Jump: { long w = ((Obj*)(intptr_t)s)->v0; return u_jumpAt(fl, w, inLoop, line); }
  }
  return 0;
  return 0;
}
long u_jumpAt(long fl, long w, long inLoop, long line) {

  if ((!inLoop)) {

  fail(s_concat(s_concat(u_spot(fl, line), w), (long)(intptr_t)" is only for inside a loop"));
  }

  return line;
  return 0;
}
long u_jumpExpr(long fl, long e, long inLoop, long line) {

  switch (((Obj*)(intptr_t)e)->tag) {
  case T_Num: { long n = ((Obj*)(intptr_t)e)->v0; return line; }
  case T_FloatLit: { long t = ((Obj*)(intptr_t)e)->v0; return line; }
  case T_Var: { long n = ((Obj*)(intptr_t)e)->v0; return line; }
  case T_StrLit: { long t = ((Obj*)(intptr_t)e)->v0; return line; }
  case T_ListLit: { long es = ((Obj*)(intptr_t)e)->v0; return line; }
  case T_Bin: { long op = ((Obj*)(intptr_t)e)->v0; long a = ((Obj*)(intptr_t)e)->v1; long b = ((Obj*)(intptr_t)e)->v2; return line; }
  case T_Call: { long name = ((Obj*)(intptr_t)e)->v0; long args = ((Obj*)(intptr_t)e)->v1; return line; }
  case T_Match: { long scrut = ((Obj*)(intptr_t)e)->v0; long arms = ((Obj*)(intptr_t)e)->v1; return line; }
  case T_Field: { long recv = ((Obj*)(intptr_t)e)->v0; long fld = ((Obj*)(intptr_t)e)->v1; return line; }
  case T_Method: { long recv = ((Obj*)(intptr_t)e)->v0; long name = ((Obj*)(intptr_t)e)->v1; long args = ((Obj*)(intptr_t)e)->v2; return line; }
  case T_Each: { long recv = ((Obj*)(intptr_t)e)->v0; long param = ((Obj*)(intptr_t)e)->v1; long body = ((Obj*)(intptr_t)e)->v2; return (line + u_jumpBody(fl, body, true)); }
  }
  return 0;
  return 0;
}
long u_retPass(long prog, long sigs) {
  long changed = 0;

  u_checkTypes(prog, sigs);

  {
  long _lst = ((R_Prog*)(intptr_t)prog)->fns;
  long _n = l_len(_lst);
  for (long _i = 0; _i < _n; _i = _i + 1) {
  long f = l_at(_lst, _i);

  u_jumpBody(((R_Sigs*)(intptr_t)sigs)->files, ((R_Fn*)(intptr_t)f)->body, false);
  }
  }

  changed = true;

  while (changed) {

  changed = false;

  {
  long _lst = ((R_Prog*)(intptr_t)prog)->fns;
  long _n = l_len(_lst);
  for (long _i = 0; _i < _n; _i = _i + 1) {
  long f = l_at(_lst, _i);

  if ((!u_isMain(((R_Fn*)(intptr_t)f)->name))) {

  if (u_paramFacts(f, sigs)) {

  changed = true;
  }
  }
  }
  }
  }

  return 0L;
  return 0;
}
long u_lastHanded(long body, long ctx) {
  long r = 0;

  r = false;

  {
  long _lst = body;
  long _n = l_len(_lst);
  for (long _i = 0; _i < _n; _i = _i + 1) {
  long s = l_at(_lst, _i);

  r = u_lastStmt(s, r, ctx);
  }
  }

  return r;
  return 0;
}
long u_lastStmt(long s, long prev, long ctx) {

  switch (((Obj*)(intptr_t)s)->tag) {
  case T_Line: { long n = ((Obj*)(intptr_t)s)->v0; return prev; }
  case T_Jump: { long w = ((Obj*)(intptr_t)s)->v0; return prev; }
  case T_Bare: { long e = ((Obj*)(intptr_t)s)->v0; return u_handed(e, ctx); }
  case T_Ret: { long e = ((Obj*)(intptr_t)s)->v0; return u_handed(e, ctx); }
  case T_If: { long c = ((Obj*)(intptr_t)s)->v0; long t = ((Obj*)(intptr_t)s)->v1; long el = ((Obj*)(intptr_t)s)->v2; return (u_lastHanded(t, ctx) || u_lastHanded(el, ctx)); }
  case T_Let: { long name = ((Obj*)(intptr_t)s)->v0; long ty = ((Obj*)(intptr_t)s)->v1; long e = ((Obj*)(intptr_t)s)->v2; return false; }
  case T_While: { long c = ((Obj*)(intptr_t)s)->v0; long b = ((Obj*)(intptr_t)s)->v1; return false; }
  }
  return 0;
  return 0;
}
long u_anyRets(long body, long ctx) {
  long r = 0;

  r = false;

  {
  long _lst = body;
  long _n = l_len(_lst);
  for (long _i = 0; _i < _n; _i = _i + 1) {
  long s = l_at(_lst, _i);

  if (u_stmtRets(s, ctx)) {

  r = true;
  }
  }
  }

  return r;
  return 0;
}
long u_stmtRets(long s, long ctx) {

  switch (((Obj*)(intptr_t)s)->tag) {
  case T_Ret: { long e = ((Obj*)(intptr_t)s)->v0; return u_handed(e, ctx); }
  case T_If: { long c = ((Obj*)(intptr_t)s)->v0; long t = ((Obj*)(intptr_t)s)->v1; long el = ((Obj*)(intptr_t)s)->v2; return (u_anyRets(t, ctx) || u_anyRets(el, ctx)); }
  case T_While: { long c = ((Obj*)(intptr_t)s)->v0; long b = ((Obj*)(intptr_t)s)->v1; return u_anyRets(b, ctx); }
  case T_Bare: { long e = ((Obj*)(intptr_t)s)->v0; return u_eachRets(e, ctx); }
  case T_Let: { long name = ((Obj*)(intptr_t)s)->v0; long ty = ((Obj*)(intptr_t)s)->v1; long e = ((Obj*)(intptr_t)s)->v2; return false; }
  case T_Line: { long n = ((Obj*)(intptr_t)s)->v0; return false; }
  case T_Jump: { long w = ((Obj*)(intptr_t)s)->v0; return false; }
  }
  return 0;
  return 0;
}
long u_eachRets(long e, long ctx) {

  switch (((Obj*)(intptr_t)e)->tag) {
  case T_Each: { long recv = ((Obj*)(intptr_t)e)->v0; long param = ((Obj*)(intptr_t)e)->v1; long body = ((Obj*)(intptr_t)e)->v2; return u_anyRets(body, ctx); }
  case T_Var: { long s = ((Obj*)(intptr_t)e)->v0; return false; }
  case T_Field: { long recv = ((Obj*)(intptr_t)e)->v0; long fld = ((Obj*)(intptr_t)e)->v1; return false; }
  case T_Method: { long recv = ((Obj*)(intptr_t)e)->v0; long name = ((Obj*)(intptr_t)e)->v1; long args = ((Obj*)(intptr_t)e)->v2; return false; }
  case T_ListLit: { long es = ((Obj*)(intptr_t)e)->v0; return false; }
  case T_Call: { long name = ((Obj*)(intptr_t)e)->v0; long args = ((Obj*)(intptr_t)e)->v1; return false; }
  case T_Match: { long scrut = ((Obj*)(intptr_t)e)->v0; long arms = ((Obj*)(intptr_t)e)->v1; return false; }
  case T_Bin: { long op = ((Obj*)(intptr_t)e)->v0; long a = ((Obj*)(intptr_t)e)->v1; long b = ((Obj*)(intptr_t)e)->v2; return false; }
  case T_Num: { long v = ((Obj*)(intptr_t)e)->v0; return false; }
  case T_FloatLit: { long s = ((Obj*)(intptr_t)e)->v0; return false; }
  case T_StrLit: { long s = ((Obj*)(intptr_t)e)->v0; return false; }
  }
  return 0;
  return 0;
}
long u_anyHanded(long es, long ctx) {
  long r = 0;

  r = false;

  {
  long _lst = es;
  long _n = l_len(_lst);
  for (long _i = 0; _i < _n; _i = _i + 1) {
  long x = l_at(_lst, _i);

  if (u_handed(x, ctx)) {

  r = true;
  }
  }
  }

  return r;
  return 0;
}
long u_armsHanded(long scrut, long arms, long ctx) {
  long r = 0;
  long styp = 0;
  long olds = 0;

  r = false;

  styp = u_exprType(scrut, ctx);

  {
  long _lst = arms;
  long _n = l_len(_lst);
  for (long _i = 0; _i < _n; _i = _i + 1) {
  long a = l_at(_lst, _i);

  olds = u_bindArm(styp, a, ctx);

  if (u_handed(((R_Arm*)(intptr_t)a)->body, ctx)) {

  r = true;
  }

  u_envRestore(((R_Ctx*)(intptr_t)ctx)->env, ((R_Arm*)(intptr_t)a)->binds, olds);
  }
  }

  return r;
  return 0;
}
long u_bindArm(long styp, long a, long ctx) {
  long olds = 0;
  long bi = 0;

  olds = u_envOlds(((R_Ctx*)(intptr_t)ctx)->env, ((R_Arm*)(intptr_t)a)->binds);

  bi = 0L;

  while ((bi < l_len(((R_Arm*)(intptr_t)a)->binds))) {

  ({ long _t21 = l_atc(((R_Arm*)(intptr_t)a)->binds, bi, 3336); long _t22 = u_payloadType(styp, ((R_Arm*)(intptr_t)a)->tag, bi, ctx); u_envPut(((R_Ctx*)(intptr_t)ctx)->env, _t21, _t22); });

  bi = (bi + 1L);
  }

  return olds;
  return 0;
}
long u_rootVar(long e) {

  switch (((Obj*)(intptr_t)e)->tag) {
  case T_Var: { long s = ((Obj*)(intptr_t)e)->v0; return s; }
  case T_Field: { long recv = ((Obj*)(intptr_t)e)->v0; long fld = ((Obj*)(intptr_t)e)->v1; return u_rootVar(recv); }
  case T_Method: { long recv = ((Obj*)(intptr_t)e)->v0; long name = ((Obj*)(intptr_t)e)->v1; long args = ((Obj*)(intptr_t)e)->v2; return u_rootVar(recv); }
  case T_ListLit: { long es = ((Obj*)(intptr_t)e)->v0; return (long)(intptr_t)""; }
  case T_Call: { long name = ((Obj*)(intptr_t)e)->v0; long args = ((Obj*)(intptr_t)e)->v1; return (long)(intptr_t)""; }
  case T_Match: { long scrut = ((Obj*)(intptr_t)e)->v0; long arms = ((Obj*)(intptr_t)e)->v1; return (long)(intptr_t)""; }
  case T_Bin: { long op = ((Obj*)(intptr_t)e)->v0; long a = ((Obj*)(intptr_t)e)->v1; long b = ((Obj*)(intptr_t)e)->v2; return (long)(intptr_t)""; }
  case T_Each: { long recv = ((Obj*)(intptr_t)e)->v0; long param = ((Obj*)(intptr_t)e)->v1; long body = ((Obj*)(intptr_t)e)->v2; return (long)(intptr_t)""; }
  case T_Num: { long v = ((Obj*)(intptr_t)e)->v0; return (long)(intptr_t)""; }
  case T_FloatLit: { long s = ((Obj*)(intptr_t)e)->v0; return (long)(intptr_t)""; }
  case T_StrLit: { long s = ((Obj*)(intptr_t)e)->v0; return (long)(intptr_t)""; }
  }
  return 0;
  return 0;
}
long u_markBody(long body, long ctx) {

  {
  long _lst = body;
  long _n = l_len(_lst);
  for (long _i = 0; _i < _n; _i = _i + 1) {
  long s = l_at(_lst, _i);

  u_markStmt(s, ctx);
  }
  }

  return 0L;
  return 0;
}
long u_markStmt(long s, long ctx) {

  switch (((Obj*)(intptr_t)s)->tag) {
  case T_Let: { long name = ((Obj*)(intptr_t)s)->v0; long ty = ((Obj*)(intptr_t)s)->v1; long e = ((Obj*)(intptr_t)s)->v2; return u_markLet(name, e, ctx); }
  case T_Bare: { long e = ((Obj*)(intptr_t)s)->v0; return u_markExpr(e, ctx); }
  case T_If: { long c = ((Obj*)(intptr_t)s)->v0; long t = ((Obj*)(intptr_t)s)->v1; long el = ((Obj*)(intptr_t)s)->v2; return ({ long _t153 = ({ long _t151 = u_markExpr(c, ctx); long _t152 = u_markBody(t, ctx); (_t151 + _t152); }); long _t154 = u_markBody(el, ctx); (_t153 + _t154); }); }
  case T_While: { long c = ((Obj*)(intptr_t)s)->v0; long b = ((Obj*)(intptr_t)s)->v1; return ({ long _t160 = u_markExpr(c, ctx); long _t161 = u_markBody(b, ctx); (_t160 + _t161); }); }
  case T_Ret: { long e = ((Obj*)(intptr_t)s)->v0; return u_markExpr(e, ctx); }
  case T_Line: { long n = ((Obj*)(intptr_t)s)->v0; return 0L; }
  case T_Jump: { long w = ((Obj*)(intptr_t)s)->v0; return 0L; }
  }
  return 0;
  return 0;
}
long u_markLet(long name, long e, long ctx) {
  long hd = 0;

  if (u_handed(e, ctx)) {

  u_setHanded(name, ctx);
  }

  hd = u_holdDepth(e, ctx);

  if ((hd > 0L)) {

  u_setHolding(name, hd, ctx);
  }

  return u_markExpr(e, ctx);
  return 0;
}
long u_markExpr(long e, long ctx) {

  switch (((Obj*)(intptr_t)e)->tag) {
  case T_Each: { long recv = ((Obj*)(intptr_t)e)->v0; long param = ((Obj*)(intptr_t)e)->v1; long body = ((Obj*)(intptr_t)e)->v2; return u_markEach(recv, param, body, ctx); }
  case T_Match: { long scrut = ((Obj*)(intptr_t)e)->v0; long arms = ((Obj*)(intptr_t)e)->v1; return u_markMatch(scrut, arms, ctx); }
  case T_Method: { long recv = ((Obj*)(intptr_t)e)->v0; long name = ((Obj*)(intptr_t)e)->v1; long args = ((Obj*)(intptr_t)e)->v2; return u_markMethod(recv, name, args, ctx); }
  case T_Field: { long recv = ((Obj*)(intptr_t)e)->v0; long fld = ((Obj*)(intptr_t)e)->v1; return u_markExpr(recv, ctx); }
  case T_ListLit: { long es = ((Obj*)(intptr_t)e)->v0; return u_markAll(es, ctx); }
  case T_Call: { long name = ((Obj*)(intptr_t)e)->v0; long args = ((Obj*)(intptr_t)e)->v1; return u_markAll(args, ctx); }
  case T_Bin: { long op = ((Obj*)(intptr_t)e)->v0; long a = ((Obj*)(intptr_t)e)->v1; long b = ((Obj*)(intptr_t)e)->v2; return ({ long _t248 = u_markExpr(a, ctx); long _t249 = u_markExpr(b, ctx); (_t248 + _t249); }); }
  case T_Var: { long s = ((Obj*)(intptr_t)e)->v0; return 0L; }
  case T_Num: { long v = ((Obj*)(intptr_t)e)->v0; return 0L; }
  case T_FloatLit: { long s = ((Obj*)(intptr_t)e)->v0; return 0L; }
  case T_StrLit: { long s = ((Obj*)(intptr_t)e)->v0; return 0L; }
  }
  return 0;
  return 0;
}
long u_markAll(long es, long ctx) {

  {
  long _lst = es;
  long _n = l_len(_lst);
  for (long _i = 0; _i < _n; _i = _i + 1) {
  long x = l_at(_lst, _i);

  u_markExpr(x, ctx);
  }
  }

  return 0L;
  return 0;
}
long u_markEach(long recv, long param, long body, long ctx) {
  long et = 0;
  long ed = 0;
  long old = 0;

  et = u_loopItem(u_exprType(recv, ctx));

  if ((!u_plainType(et))) {

  ed = u_holdDepth(recv, ctx);

  if ((u_handed(recv, ctx) || (ed == 1L))) {

  u_setHanded(param, ctx);
  }

  if ((ed > 1L)) {

  u_setHolding(param, (ed - 1L), ctx);
  }
  }

  u_markExpr(recv, ctx);

  old = u_envOld(((R_Ctx*)(intptr_t)ctx)->env, param);

  u_envPut(((R_Ctx*)(intptr_t)ctx)->env, param, et);

  u_markBody(body, ctx);

  return u_envPut(((R_Ctx*)(intptr_t)ctx)->env, param, old);
  return 0;
}
long u_markMatch(long scrut, long arms, long ctx) {
  long h = 0;
  long styp = 0;
  long olds = 0;

  h = (u_handed(scrut, ctx) || (u_holdDepth(scrut, ctx) > 0L));

  u_markExpr(scrut, ctx);

  styp = u_exprType(scrut, ctx);

  {
  long _lst = arms;
  long _n = l_len(_lst);
  for (long _i = 0; _i < _n; _i = _i + 1) {
  long a = l_at(_lst, _i);

  olds = u_bindArm(styp, a, ctx);

  if (h) {

  {
  long _lst = ((R_Arm*)(intptr_t)a)->binds;
  long _n = l_len(_lst);
  for (long _i = 0; _i < _n; _i = _i + 1) {
  long b = l_at(_lst, _i);

  if ((!u_plainType(u_envGet(((R_Ctx*)(intptr_t)ctx)->env, b)))) {

  u_setHanded(b, ctx);
  }
  }
  }
  }

  u_markExpr(((R_Arm*)(intptr_t)a)->body, ctx);

  u_envRestore(((R_Ctx*)(intptr_t)ctx)->env, ((R_Arm*)(intptr_t)a)->binds, olds);
  }
  }

  return 0L;
  return 0;
}
long u_markMethod(long recv, long name, long args, long ctx) {
  long root = 0;
  long ml = 0;

  if ((((s_eq(name, (long)(intptr_t)"push") || s_eq(name, (long)(intptr_t)"set")) || s_eq(name, (long)(intptr_t)"remove")) || u_isSetField(name))) {

  root = u_rootVar(recv);

  ml = u_minLevel(args, ctx);

  if ((ml >= 0L)) {

  u_setHolding(root, ((ml + 1L) + u_pathLen(recv)), ctx);
  }
  }

  u_markExpr(recv, ctx);

  return u_markAll(args, ctx);
  return 0;
}
long u_checkEffects(long f, long ctx) {
  long used = 0;
  long k = 0;
  long m = 0;
  long e = 0;

  if ((!u_isMain(((R_Fn*)(intptr_t)f)->name))) {

  u_markFn(f, ctx);

  used = l_new();

  u_effBody(((R_Fn*)(intptr_t)f)->body, used, ctx);

  k = 0L;

  m = l_len(used);

  while ((k < m)) {

  e = l_atc(used, k, 3470);

  if (((!u_hasName(((R_Fn*)(intptr_t)f)->effs, e)) && (!s_eq(e, (long)(intptr_t)"Fail")))) {

  if (s_eq(e, (long)(intptr_t)"Mut")) {

  u_failAt(s_concat(((R_Fn*)(intptr_t)f)->name, (long)(intptr_t)" changes a list or map it was handed; add !Mut after its return type"), ctx);
  }

  u_failAt(s_concat(s_concat((long)(intptr_t)"uses !", e), (long)(intptr_t)" but does not declare it"), ctx);
  }

  k = (k + 1L);
  }
  }

  return 0L;
  return 0;
}
long u_effBody(long body, long used, long ctx) {
  long k = 0;
  long m = 0;

  k = 0L;

  m = l_len(body);

  while ((k < m)) {

  u_effStmt(l_atc(body, k, 3488), used, ctx);

  k = (k + 1L);
  }

  return 0L;
  return 0;
}
long u_effStmt(long s, long used, long ctx) {

  switch (((Obj*)(intptr_t)s)->tag) {
  case T_Let: { long name = ((Obj*)(intptr_t)s)->v0; long ty = ((Obj*)(intptr_t)s)->v1; long e = ((Obj*)(intptr_t)s)->v2; return u_effExpr(e, used, ctx); }
  case T_Bare: { long e = ((Obj*)(intptr_t)s)->v0; return u_effExpr(e, used, ctx); }
  case T_If: { long c = ((Obj*)(intptr_t)s)->v0; long t = ((Obj*)(intptr_t)s)->v1; long el = ((Obj*)(intptr_t)s)->v2; return u_effIf(c, t, el, used, ctx); }
  case T_While: { long c = ((Obj*)(intptr_t)s)->v0; long b = ((Obj*)(intptr_t)s)->v1; return u_effWhile(c, b, used, ctx); }
  case T_Ret: { long e = ((Obj*)(intptr_t)s)->v0; return u_effExpr(e, used, ctx); }
  case T_Line: { long n = ((Obj*)(intptr_t)s)->v0; return 0L; }
  case T_Jump: { long w = ((Obj*)(intptr_t)s)->v0; return 0L; }
  }
  return 0;
  return 0;
}
long u_effIf(long c, long t, long el, long used, long ctx) {

  u_effExpr(c, used, ctx);

  u_effBody(t, used, ctx);

  u_effBody(el, used, ctx);

  return 0L;
  return 0;
}
long u_effWhile(long c, long b, long used, long ctx) {

  u_effExpr(c, used, ctx);

  u_effBody(b, used, ctx);

  return 0L;
  return 0;
}
long u_effExpr(long e, long used, long ctx) {

  switch (((Obj*)(intptr_t)e)->tag) {
  case T_Method: { long recv = ((Obj*)(intptr_t)e)->v0; long name = ((Obj*)(intptr_t)e)->v1; long args = ((Obj*)(intptr_t)e)->v2; return u_effMethodE(recv, name, args, used, ctx); }
  case T_Call: { long name = ((Obj*)(intptr_t)e)->v0; long args = ((Obj*)(intptr_t)e)->v1; return u_effCallE(name, args, used, ctx); }
  case T_Bin: { long op = ((Obj*)(intptr_t)e)->v0; long a = ((Obj*)(intptr_t)e)->v1; long b = ((Obj*)(intptr_t)e)->v2; return u_effBin(a, b, used, ctx); }
  case T_Match: { long scrut = ((Obj*)(intptr_t)e)->v0; long arms = ((Obj*)(intptr_t)e)->v1; return u_effMatchE(scrut, arms, used, ctx); }
  case T_Field: { long recv = ((Obj*)(intptr_t)e)->v0; long fld = ((Obj*)(intptr_t)e)->v1; return u_effFieldE(recv, fld, used, ctx); }
  case T_Each: { long recv = ((Obj*)(intptr_t)e)->v0; long param = ((Obj*)(intptr_t)e)->v1; long body = ((Obj*)(intptr_t)e)->v2; return u_effEachE(recv, param, body, used, ctx); }
  case T_ListLit: { long es = ((Obj*)(intptr_t)e)->v0; return u_effArgs(es, used, ctx); }
  case T_Num: { long v = ((Obj*)(intptr_t)e)->v0; return 0L; }
  case T_FloatLit: { long s = ((Obj*)(intptr_t)e)->v0; return 0L; }
  case T_Var: { long s = ((Obj*)(intptr_t)e)->v0; return 0L; }
  case T_StrLit: { long s = ((Obj*)(intptr_t)e)->v0; return 0L; }
  }
  return 0;
  return 0;
}
long u_effMethodE(long recv, long name, long args, long used, long ctx) {

  if ((((((s_eq(name, (long)(intptr_t)"print") || s_eq(name, (long)(intptr_t)"part")) || s_eq(name, (long)(intptr_t)"error")) || s_eq(name, (long)(intptr_t)"fetchWith")) || s_eq(name, (long)(intptr_t)"readBytes")) || s_eq(name, (long)(intptr_t)"writeBytes"))) {

  l_push(used, (long)(intptr_t)"IO");
  }

  if (((((((((s_eq(name, (long)(intptr_t)"send") || s_eq(name, (long)(intptr_t)"run")) || s_eq(name, (long)(intptr_t)"status")) || s_eq(name, (long)(intptr_t)"start")) || s_eq(name, (long)(intptr_t)"try")) || s_eq(name, (long)(intptr_t)"feed")) || s_eq(name, (long)(intptr_t)"fetch")) || s_eq(name, (long)(intptr_t)"post")) || s_eq(name, (long)(intptr_t)"reach"))) {

  l_push(used, (long)(intptr_t)"IO");
  }

  if (s_eq(name, (long)(intptr_t)"read")) {

  l_push(used, (long)(intptr_t)"IO");

  l_push(used, (long)(intptr_t)"Fail");
  }

  if (s_eq(name, (long)(intptr_t)"write")) {

  l_push(used, (long)(intptr_t)"IO");
  }

  if (((((s_eq(name, (long)(intptr_t)"append") || s_eq(name, (long)(intptr_t)"list")) || s_eq(name, (long)(intptr_t)"exists")) || s_eq(name, (long)(intptr_t)"rename")) || s_eq(name, (long)(intptr_t)"delete"))) {

  l_push(used, (long)(intptr_t)"IO");
  }

  if ((((s_eq(name, (long)(intptr_t)"push") || s_eq(name, (long)(intptr_t)"set")) || s_eq(name, (long)(intptr_t)"remove")) || u_isSetField(name))) {

  if (u_handed(recv, ctx)) {

  l_push(used, (long)(intptr_t)"Mut");
  }
  }

  u_effExpr(recv, used, ctx);

  u_effArgs(args, used, ctx);

  return 0L;
  return 0;
}
long u_effFieldE(long recv, long fld, long used, long ctx) {

  if (u_useUFCS(u_exprType(recv, ctx), fld, ctx)) {

  return u_effCallE(fld, l_with(l_new(), recv), used, ctx);
  }

  return u_effExpr(recv, used, ctx);
  return 0;
}
long u_effCallE(long name, long args, long used, long ctx) {
  long fns = 0;
  long k = 0;
  long m = 0;
  long g = 0;
  long j = 0;

  fns = ((R_Sigs*)(intptr_t)((R_Ctx*)(intptr_t)ctx)->sigs)->fns;

  k = 0L;

  m = l_len(fns);

  while ((k < m)) {

  g = l_atc(fns, k, 3575);

  if (s_eq(((R_Fn*)(intptr_t)g)->name, name)) {

  {
  long _lst = ((R_Fn*)(intptr_t)g)->effs;
  long _n = l_len(_lst);
  for (long _i = 0; _i < _n; _i = _i + 1) {
  long x = l_at(_lst, _i);

  if ((!s_eq(x, (long)(intptr_t)"Mut"))) {

  l_push(used, x);
  }
  }
  }

  if (u_hasName(((R_Fn*)(intptr_t)g)->effs, (long)(intptr_t)"Mut")) {

  j = 0L;

  while ((j < l_len(args))) {

  if ((({ long _t62 = ((R_Sigs*)(intptr_t)((R_Ctx*)(intptr_t)ctx)->sigs)->mutP; long _t65 = ({ long _t63 = s_concat(name, (long)(intptr_t)":"); long _t64 = i_tostr(j); s_concat(_t63, _t64); }); u_hasName(_t62, _t65); }) && u_handed(l_atc(args, j, 3585), ctx))) {

  l_push(used, (long)(intptr_t)"Mut");
  }

  j = (j + 1L);
  }
  }
  }

  k = (k + 1L);
  }

  u_effArgs(args, used, ctx);

  return 0L;
  return 0;
}
long u_effBin(long a, long b, long used, long ctx) {

  u_effExpr(a, used, ctx);

  u_effExpr(b, used, ctx);

  return 0L;
  return 0;
}
long u_effMatchE(long scrut, long arms, long used, long ctx) {
  long styp = 0;
  long olds = 0;

  u_effExpr(scrut, used, ctx);

  styp = u_exprType(scrut, ctx);

  {
  long _lst = arms;
  long _n = l_len(_lst);
  for (long _i = 0; _i < _n; _i = _i + 1) {
  long a = l_at(_lst, _i);

  olds = u_bindArm(styp, a, ctx);

  u_effExpr(((R_Arm*)(intptr_t)a)->body, used, ctx);

  u_envRestore(((R_Ctx*)(intptr_t)ctx)->env, ((R_Arm*)(intptr_t)a)->binds, olds);
  }
  }

  return 0L;
  return 0;
}
long u_effEachE(long recv, long param, long body, long used, long ctx) {
  long old = 0;

  u_effExpr(recv, used, ctx);

  old = u_envOld(((R_Ctx*)(intptr_t)ctx)->env, param);

  u_envPut(((R_Ctx*)(intptr_t)ctx)->env, param, u_loopItem(u_exprType(recv, ctx)));

  u_effBody(body, used, ctx);

  u_envPut(((R_Ctx*)(intptr_t)ctx)->env, param, old);

  return 0L;
  return 0;
}
long u_effArgs(long args, long used, long ctx) {
  long k = 0;
  long m = 0;

  k = 0L;

  m = l_len(args);

  while ((k < m)) {

  u_effExpr(l_atc(args, k, 3628), used, ctx);

  k = (k + 1L);
  }

  return 0L;
  return 0;
}
long u_addAll(long src, long dst) {
  long k = 0;
  long m = 0;

  k = 0L;

  m = l_len(src);

  while ((k < m)) {

  l_push(dst, l_atc(src, k, 3638));

  k = (k + 1L);
  }

  return 0L;
  return 0;
}
long u_caseArityIn(long t, long tag) {
  long r = 0;
  long k = 0;
  long m = 0;
  long c = 0;

  r = (0L - 1L);

  k = 0L;

  m = l_len(((R_TyDef*)(intptr_t)t)->cases);

  while ((k < m)) {

  c = l_atc(((R_TyDef*)(intptr_t)t)->cases, k, 3649);

  if (s_eq(((R_Case*)(intptr_t)c)->cname, tag)) {

  r = ((R_Case*)(intptr_t)c)->arity;
  }

  k = (k + 1L);
  }

  return r;
  return 0;
}
long u_checkCases(long t, long arms, long ctx) {
  long k = 0;
  long m = 0;
  long c = 0;

  k = 0L;

  m = l_len(((R_TyDef*)(intptr_t)t)->cases);

  while ((k < m)) {

  c = l_atc(((R_TyDef*)(intptr_t)t)->cases, k, 3662);

  if ((!u_armCovers(arms, ((R_Case*)(intptr_t)c)->cname))) {

  u_failAt(({ long _t28 = s_concat(s_concat((long)(intptr_t)"non-exhaustive match in ", ((R_TyDef*)(intptr_t)t)->name), (long)(intptr_t)", missing case: "); long _t29 = ((R_Case*)(intptr_t)c)->cname; s_concat(_t28, _t29); }), ctx);
  }

  k = (k + 1L);
  }

  return 0L;
  return 0;
}
long u_armCovers(long arms, long cname) {
  long found = 0;
  long k = 0;
  long m = 0;

  found = false;

  k = 0L;

  m = l_len(arms);

  while ((k < m)) {

  if (s_eq(((R_Arm*)(intptr_t)l_atc(arms, k, 3676))->tag, cname)) {

  found = true;
  }

  k = (k + 1L);
  }

  return found;
  return 0;
}
long u_armHasBind(long arms) {
  long has = 0;
  long k = 0;
  long m = 0;

  has = false;

  k = 0L;

  m = l_len(arms);

  while ((k < m)) {

  if ((l_len(((R_Arm*)(intptr_t)l_atc(arms, k, 3689))->binds) > 0L)) {

  has = true;
  }

  k = (k + 1L);
  }

  return has;
  return 0;
}
long u_emitExpr(long e, long ctx) {

  switch (((Obj*)(intptr_t)e)->tag) {
  case T_Num: { long v = ((Obj*)(intptr_t)e)->v0; return s_concat(i_tostr(v), (long)(intptr_t)"L"); }
  case T_FloatLit: { long s = ((Obj*)(intptr_t)e)->v0; return s_concat(s_concat((long)(intptr_t)"d2l(", s), (long)(intptr_t)")"); }
  case T_Var: { long s = ((Obj*)(intptr_t)e)->v0; return u_emitVar(s, ctx); }
  case T_StrLit: { long s = ((Obj*)(intptr_t)e)->v0; return s_concat(s_concat((long)(intptr_t)"(long)(intptr_t)\"", u_cEscape(s)), (long)(intptr_t)"\""); }
  case T_ListLit: { long es = ((Obj*)(intptr_t)e)->v0; return u_emitList(es, ctx); }
  case T_Bin: { long op = ((Obj*)(intptr_t)e)->v0; long a = ((Obj*)(intptr_t)e)->v1; long b = ((Obj*)(intptr_t)e)->v2; return u_emitBin(op, a, b, ctx); }
  case T_Call: { long name = ((Obj*)(intptr_t)e)->v0; long args = ((Obj*)(intptr_t)e)->v1; return u_emitCall(name, args, ctx); }
  case T_Match: { long scrut = ((Obj*)(intptr_t)e)->v0; long arms = ((Obj*)(intptr_t)e)->v1; return u_emitMatchExpr(scrut, arms, ctx); }
  case T_Field: { long recv = ((Obj*)(intptr_t)e)->v0; long fld = ((Obj*)(intptr_t)e)->v1; return u_emitField(recv, fld, ctx); }
  case T_Method: { long recv = ((Obj*)(intptr_t)e)->v0; long name = ((Obj*)(intptr_t)e)->v1; long args = ((Obj*)(intptr_t)e)->v2; return u_emitMethod(recv, name, args, ctx); }
  case T_Each: { long recv = ((Obj*)(intptr_t)e)->v0; long param = ((Obj*)(intptr_t)e)->v1; long body = ((Obj*)(intptr_t)e)->v2; return s_concat((long)(intptr_t)"0", i_tostr(u_failAt((long)(intptr_t)"an each loop has no value", ctx))); }
  }
  return 0;
  return 0;
}
long u_calm(long e) {

  switch (((Obj*)(intptr_t)e)->tag) {
  case T_Num: { long v = ((Obj*)(intptr_t)e)->v0; return true; }
  case T_FloatLit: { long s = ((Obj*)(intptr_t)e)->v0; return true; }
  case T_Var: { long s = ((Obj*)(intptr_t)e)->v0; return true; }
  case T_StrLit: { long s = ((Obj*)(intptr_t)e)->v0; return true; }
  case T_ListLit: { long es = ((Obj*)(intptr_t)e)->v0; return (l_len(es) == 0L); }
  case T_Field: { long recv = ((Obj*)(intptr_t)e)->v0; long fld = ((Obj*)(intptr_t)e)->v1; return (u_isCapField(fld) && u_calm(recv)); }
  case T_Bin: { long op = ((Obj*)(intptr_t)e)->v0; long a = ((Obj*)(intptr_t)e)->v1; long b = ((Obj*)(intptr_t)e)->v2; return ((((!s_eq(op, (long)(intptr_t)"/")) && (!s_eq(op, (long)(intptr_t)"%"))) && u_calm(a)) && u_calm(b)); }
  case T_Call: { long name = ((Obj*)(intptr_t)e)->v0; long args = ((Obj*)(intptr_t)e)->v1; return false; }
  case T_Match: { long scrut = ((Obj*)(intptr_t)e)->v0; long arms = ((Obj*)(intptr_t)e)->v1; return false; }
  case T_Method: { long recv = ((Obj*)(intptr_t)e)->v0; long name = ((Obj*)(intptr_t)e)->v1; long args = ((Obj*)(intptr_t)e)->v2; return false; }
  case T_Each: { long recv = ((Obj*)(intptr_t)e)->v0; long param = ((Obj*)(intptr_t)e)->v1; long body = ((Obj*)(intptr_t)e)->v2; return false; }
  }
  return 0;
  return 0;
}
long u_hoist(long ops, long ctx) {
  long busy = 0;
  long pre = 0;
  long out = 0;
  long c = 0;
  long ty = 0;
  long name = 0;

  busy = 0L;

  {
  long _lst = ops;
  long _n = l_len(_lst);
  for (long _i = 0; _i < _n; _i = _i + 1) {
  long o = l_at(_lst, _i);

  if ((!u_calm(o))) {

  busy = (busy + 1L);
  }
  }
  }

  pre = (long)(intptr_t)"";

  out = l_new();

  {
  long _lst = ops;
  long _n = l_len(_lst);
  for (long _i = 0; _i < _n; _i = _i + 1) {
  long o = l_at(_lst, _i);

  if (((busy > 1L) && (!u_calm(o)))) {

  c = u_emitExpr(o, ctx);

  ty = u_exprType(o, ctx);

  name = s_concat((long)(intptr_t)"_t", i_tostr(l_len(((R_TyEnv*)(intptr_t)((R_Ctx*)(intptr_t)ctx)->env)->names)));

  u_envPut(((R_Ctx*)(intptr_t)ctx)->env, name, ty);

  pre = s_concat(s_concat(s_concat(s_concat(s_concat(pre, (long)(intptr_t)"long "), name), (long)(intptr_t)" = "), c), (long)(intptr_t)"; ");

  l_push(out, Var(name));
  } else {

  l_push(out, o);
  }
  }
  }

  return Hoist(pre, out);
  return 0;
}
long u_inOrder(long pre, long c) {
  long r = 0;

  r = c;

  if ((!s_eq(pre, (long)(intptr_t)""))) {

  r = s_concat(s_concat(s_concat((long)(intptr_t)"({ ", pre), c), (long)(intptr_t)"; })");
  }

  return r;
  return 0;
}
long u_emitBin(long op, long a, long b, long ctx) {
  long h = 0;

  if (((s_eq(op, (long)(intptr_t)"&&") || s_eq(op, (long)(intptr_t)"||")) || s_eq(op, (long)(intptr_t)"u-"))) {

  return u_emitBin0(op, a, b, ctx);
  }

  h = u_hoist(l_with(l_with(l_new(), a), b), ctx);

  return ({ long _t23 = ((R_Hoist*)(intptr_t)h)->pre; long _t26 = ({ long _t24 = l_atc(((R_Hoist*)(intptr_t)h)->ops, 0L, 3777); long _t25 = l_atc(((R_Hoist*)(intptr_t)h)->ops, 1L, 3777); u_emitBin0(op, _t24, _t25, ctx); }); u_inOrder(_t23, _t26); });
  return 0;
}
long u_emitCall(long name, long args, long ctx) {
  long h = 0;

  h = u_hoist(args, ctx);

  return ({ long _t15 = ((R_Hoist*)(intptr_t)h)->pre; long _t16 = u_emitCall0(name, ((R_Hoist*)(intptr_t)h)->ops, ctx); u_inOrder(_t15, _t16); });
  return 0;
}
long u_emitMethod(long recv, long name, long args, long ctx) {
  long all = 0;
  long h = 0;
  long rest = 0;
  long k = 0;

  if ((s_eq(name, (long)(intptr_t)"slice") && (l_len(args) == 1L))) {

  return u_emitMethod(recv, name, ({ long _t43 = l_atc(args, 0L, 3788); long _t44 = Field(recv, (long)(intptr_t)"length"); l_with(l_with(l_new(), _t43), _t44); }), ctx);
  }

  if ((s_eq(name, (long)(intptr_t)"and") || s_eq(name, (long)(intptr_t)"or"))) {

  return u_emitMethod0(recv, name, args, ctx);
  }

  all = l_with(l_new(), recv);

  {
  long _lst = args;
  long _n = l_len(_lst);
  for (long _i = 0; _i < _n; _i = _i + 1) {
  long a = l_at(_lst, _i);

  l_push(all, a);
  }
  }

  h = u_hoist(all, ctx);

  rest = l_new();

  k = 1L;

  while ((k < l_len(((R_Hoist*)(intptr_t)h)->ops))) {

  l_push(rest, l_atc(((R_Hoist*)(intptr_t)h)->ops, k, 3801));

  k = (k + 1L);
  }

  return ({ long _t61 = ((R_Hoist*)(intptr_t)h)->pre; long _t62 = u_emitMethod0(l_atc(((R_Hoist*)(intptr_t)h)->ops, 0L, 3804), name, rest, ctx); u_inOrder(_t61, _t62); });
  return 0;
}
long u_emitList(long es, long ctx) {
  long h = 0;

  h = u_hoist(es, ctx);

  return ({ long _t14 = ((R_Hoist*)(intptr_t)h)->pre; long _t15 = u_emitList0(((R_Hoist*)(intptr_t)h)->ops, ctx); u_inOrder(_t14, _t15); });
  return 0;
}
long u_emitList0(long es, long ctx) {
  long r = 0;
  long k = 0;
  long m = 0;

  r = (long)(intptr_t)"l_new()";

  k = 0L;

  m = l_len(es);

  while ((k < m)) {

  r = s_concat(({ long _t24 = s_concat(s_concat((long)(intptr_t)"l_with(", r), (long)(intptr_t)", "); long _t25 = u_emitExpr(l_atc(es, k, 3818), ctx); s_concat(_t24, _t25); }), (long)(intptr_t)")");

  k = (k + 1L);
  }

  return r;
  return 0;
}
long u_listType(long es, long ctx) {
  long r = 0;

  r = (long)(intptr_t)"List";

  if ((l_len(es) > 0L)) {

  r = s_concat(s_concat((long)(intptr_t)"List[", u_exprType(l_atc(es, 0L, 3827), ctx)), (long)(intptr_t)"]");
  }

  return r;
  return 0;
}
long u_emitBin0(long op, long a, long b, long ctx) {
  long ca = 0;
  long cb = 0;
  long r = 0;
  long inner = 0;

  u_checkBinOp(op, a, b, ctx);

  ca = u_emitExpr(a, ctx);

  cb = u_emitExpr(b, ctx);

  r = s_concat(s_concat(s_concat(s_concat(s_concat(s_concat((long)(intptr_t)"(", ca), (long)(intptr_t)" "), op), (long)(intptr_t)" "), cb), (long)(intptr_t)")");

  if (s_eq(op, (long)(intptr_t)"/")) {

  r = s_concat(({ long _t59 = s_concat(s_concat(s_concat(s_concat((long)(intptr_t)"i_div(", ca), (long)(intptr_t)", "), cb), (long)(intptr_t)", "); long _t60 = u_envGet(((R_Ctx*)(intptr_t)ctx)->env, (long)(intptr_t)"__line__"); s_concat(_t59, _t60); }), (long)(intptr_t)")");
  }

  if (s_eq(op, (long)(intptr_t)"%")) {

  r = s_concat(({ long _t63 = s_concat(s_concat(s_concat(s_concat((long)(intptr_t)"i_mod(", ca), (long)(intptr_t)", "), cb), (long)(intptr_t)", "); long _t64 = u_envGet(((R_Ctx*)(intptr_t)ctx)->env, (long)(intptr_t)"__line__"); s_concat(_t63, _t64); }), (long)(intptr_t)")");
  }

  if (s_eq(op, (long)(intptr_t)"==")) {

  if (s_eq(u_exprType(a, ctx), (long)(intptr_t)"Str")) {

  r = s_concat(s_concat(s_concat(s_concat((long)(intptr_t)"s_eq(", ca), (long)(intptr_t)", "), cb), (long)(intptr_t)")");
  }
  }

  if (s_eq(u_exprType(a, ctx), (long)(intptr_t)"Float")) {

  inner = s_concat(s_concat(s_concat(s_concat(s_concat(s_concat((long)(intptr_t)"(l2d(", ca), (long)(intptr_t)") "), op), (long)(intptr_t)" l2d("), cb), (long)(intptr_t)")) ");

  r = inner;

  if (s_eq(op, (long)(intptr_t)"%")) {

  inner = s_concat(s_concat(s_concat(s_concat((long)(intptr_t)"fmod(l2d(", ca), (long)(intptr_t)"), l2d("), cb), (long)(intptr_t)"))");
  }

  if (u_isFloatArith(op)) {

  r = s_concat(s_concat((long)(intptr_t)"d2l(", inner), (long)(intptr_t)")");
  }
  }

  if (u_textOp(op, a, b, ctx)) {

  r = s_concat(s_concat(s_concat(s_concat(s_concat(s_concat((long)(intptr_t)"(strcmp((const char*)(intptr_t)", ca), (long)(intptr_t)", (const char*)(intptr_t)"), cb), (long)(intptr_t)") "), op), (long)(intptr_t)" 0)");

  if (s_eq(op, (long)(intptr_t)"+")) {

  r = s_concat(s_concat(s_concat(s_concat((long)(intptr_t)"s_concat(", ca), (long)(intptr_t)", "), cb), (long)(intptr_t)")");
  }
  }

  if (s_eq(op, (long)(intptr_t)"u-")) {

  if ((!(s_eq(u_exprType(b, ctx), (long)(intptr_t)"Int") || s_eq(u_exprType(b, ctx), (long)(intptr_t)"Float")))) {

  u_failAt(s_concat((long)(intptr_t)"- needs a number, got ", u_exprType(b, ctx)), ctx);
  }

  r = s_concat(s_concat((long)(intptr_t)"(-", cb), (long)(intptr_t)")");

  if (s_eq(u_exprType(b, ctx), (long)(intptr_t)"Float")) {

  r = s_concat(s_concat((long)(intptr_t)"d2l(-l2d(", cb), (long)(intptr_t)"))");
  }
  }

  return r;
  return 0;
}
long u_textOp(long op, long a, long b, long ctx) {
  long r = 0;

  r = false;

  if (((((s_eq(op, (long)(intptr_t)"+") || s_eq(op, (long)(intptr_t)"<")) || s_eq(op, (long)(intptr_t)">")) || s_eq(op, (long)(intptr_t)"<=")) || s_eq(op, (long)(intptr_t)">="))) {

  if (s_eq(u_exprType(a, ctx), (long)(intptr_t)"Str")) {

  if (s_eq(u_exprType(b, ctx), (long)(intptr_t)"Str")) {

  r = true;
  }
  }
  }

  return r;
  return 0;
}
long u_isFloatArith(long op) {

  return ((((s_eq(op, (long)(intptr_t)"+") || s_eq(op, (long)(intptr_t)"-")) || s_eq(op, (long)(intptr_t)"*")) || s_eq(op, (long)(intptr_t)"/")) || s_eq(op, (long)(intptr_t)"%"));
  return 0;
}
long u_checkBinOp(long op, long a, long b, long ctx) {
  long ta = 0;
  long tb = 0;

  if (u_textOp(op, a, b, ctx)) {

  return 0L;
  }

  if (u_isArithOp(op)) {

  ta = u_exprType(a, ctx);

  tb = u_exprType(b, ctx);

  if (s_eq(op, (long)(intptr_t)"+")) {

  if ((s_eq(ta, (long)(intptr_t)"Str") || s_eq(tb, (long)(intptr_t)"Str"))) {

  u_failAt((long)(intptr_t)"+ adds two numbers or joins two texts; turn the number into text with .toStr", ctx);
  }
  }

  if (u_notInt(ta, ctx)) {

  u_failAt(s_concat(s_concat((long)(intptr_t)"operator ", op), (long)(intptr_t)" needs Int operands"), ctx);
  }

  if (u_notInt(tb, ctx)) {

  u_failAt(s_concat(s_concat((long)(intptr_t)"operator ", op), (long)(intptr_t)" needs Int operands"), ctx);
  }

  if (u_mixedFloat(ta, tb)) {

  u_failAt(s_concat(s_concat((long)(intptr_t)"operator ", op), (long)(intptr_t)" mixes Int and Float; convert with .toFloat or .toInt"), ctx);
  }
  }

  return 0L;
  return 0;
}
long u_mixedFloat(long a, long b) {
  long r = 0;

  r = false;

  if ((s_eq(a, (long)(intptr_t)"Float") && (!s_eq(b, (long)(intptr_t)"Float")))) {

  r = true;
  }

  if ((s_eq(b, (long)(intptr_t)"Float") && (!s_eq(a, (long)(intptr_t)"Float")))) {

  r = true;
  }

  return r;
  return 0;
}
long u_isArithOp(long op) {

  return ((((((((s_eq(op, (long)(intptr_t)"+") || s_eq(op, (long)(intptr_t)"-")) || s_eq(op, (long)(intptr_t)"*")) || s_eq(op, (long)(intptr_t)"/")) || s_eq(op, (long)(intptr_t)"<")) || s_eq(op, (long)(intptr_t)">")) || s_eq(op, (long)(intptr_t)"<=")) || s_eq(op, (long)(intptr_t)">=")) || s_eq(op, (long)(intptr_t)"%"));
  return 0;
}
long u_knownCall(long name, long ctx) {
  long r = 0;

  r = (u_hasName(((R_Sigs*)(intptr_t)((R_Ctx*)(intptr_t)ctx)->sigs)->fnNames, name) || u_hasName(((R_Sigs*)(intptr_t)((R_Ctx*)(intptr_t)ctx)->sigs)->recNames, name));

  if ((!s_eq(u_variantOf(name, ctx), (long)(intptr_t)""))) {

  r = true;
  }

  if ((((((s_eq(name, (long)(intptr_t)"Map") || s_eq(name, (long)(intptr_t)"fail")) || s_eq(name, (long)(intptr_t)"exit")) || s_eq(name, (long)(intptr_t)"exit_c")) || s_eq(name, (long)(intptr_t)"assert")) || s_eq(name, (long)(intptr_t)"l_range"))) {

  r = true;
  }

  return r;
  return 0;
}
long u_emitCall0(long name, long args, long ctx) {
  long cn = 0;

  if (u_isMapType(name)) {

  if ((!u_typeKnown(name, ((R_Ctx*)(intptr_t)ctx)->sigs))) {

  u_failAt(u_noType(name), ctx);
  }

  return (long)(intptr_t)"Map()";
  }

  if ((!u_knownCall(name, ctx))) {

  u_failAt(s_concat((long)(intptr_t)"no function named ", name), ctx);
  }

  u_checkCall(name, args, ctx);

  cn = name;

  if (s_eq(name, (long)(intptr_t)"assert")) {

  cn = (long)(intptr_t)"s_assert";

  if ((!(l_len(args) == 1L))) {

  u_failAt((long)(intptr_t)"assert takes one Bool", ctx);
  }

  if ((!s_eq(u_exprType(l_atc(args, 0L, 3974), ctx), (long)(intptr_t)"Bool"))) {

  u_failAt(s_concat((long)(intptr_t)"assert needs a Bool, got ", u_exprType(l_atc(args, 0L, 3975), ctx)), ctx);
  }
  }

  if (((s_eq(name, (long)(intptr_t)"exit") || s_eq(name, (long)(intptr_t)"exit_c")) && (!u_hasName(((R_Sigs*)(intptr_t)((R_Ctx*)(intptr_t)ctx)->sigs)->fnNames, name)))) {

  cn = (long)(intptr_t)"simpler_exit";

  if ((!(l_len(args) == 1L))) {

  u_failAt((long)(intptr_t)"exit takes the exit status, one Int", ctx);
  }

  if ((!s_eq(u_exprType(l_atc(args, 0L, 3983), ctx), (long)(intptr_t)"Int"))) {

  u_failAt(s_concat((long)(intptr_t)"exit takes the exit status as an Int, got ", u_exprType(l_atc(args, 0L, 3984), ctx)), ctx);
  }
  }

  if (u_hasName(((R_Sigs*)(intptr_t)((R_Ctx*)(intptr_t)ctx)->sigs)->fnNames, name)) {

  cn = u_mangle(name);
  }

  return s_concat(({ long _t60 = s_concat(cn, (long)(intptr_t)"("); long _t61 = u_emitArgs(args, ctx); s_concat(_t60, _t61); }), (long)(intptr_t)")");
  return 0;
}
long u_checkReturn(long f, long ctx) {
  long n = 0;

  if ((!u_isMain(((R_Fn*)(intptr_t)f)->name))) {

  n = l_len(((R_Fn*)(intptr_t)f)->body);

  if ((n > 0L)) {

  ({ long _t17 = l_atc(((R_Fn*)(intptr_t)f)->body, (n - 1L), 4000); long _t18 = ((R_Fn*)(intptr_t)f)->ret; u_checkReturnStmt(_t17, _t18, ctx); });
  }
  }

  return 0L;
  return 0;
}
long u_checkReturnStmt(long s, long ret, long ctx) {

  switch (((Obj*)(intptr_t)s)->tag) {
  case T_Bare: { long e = ((Obj*)(intptr_t)s)->v0; return u_checkRetExpr(e, ret, ctx); }
  case T_Let: { long name = ((Obj*)(intptr_t)s)->v0; long ty = ((Obj*)(intptr_t)s)->v1; long v = ((Obj*)(intptr_t)s)->v2; return 0L; }
  case T_If: { long c = ((Obj*)(intptr_t)s)->v0; long t = ((Obj*)(intptr_t)s)->v1; long el = ((Obj*)(intptr_t)s)->v2; return 0L; }
  case T_While: { long c = ((Obj*)(intptr_t)s)->v0; long b = ((Obj*)(intptr_t)s)->v1; return 0L; }
  case T_Ret: { long e = ((Obj*)(intptr_t)s)->v0; return u_checkRetExpr(e, ret, ctx); }
  case T_Line: { long n = ((Obj*)(intptr_t)s)->v0; return 0L; }
  case T_Jump: { long w = ((Obj*)(intptr_t)s)->v0; return 0L; }
  }
  return 0;
  return 0;
}
long u_checkRetExpr(long e, long ret, long ctx) {
  long at = 0;

  if ((!u_isMatchExpr(e))) {

  at = u_exprType(e, ctx);

  if ((!u_compatible(ret, at, ctx))) {

  u_failAt(s_concat(s_concat(s_concat((long)(intptr_t)"return type mismatch: declared ", ret), (long)(intptr_t)", got "), at), ctx);
  }
  }

  return 0L;
  return 0;
}
long u_isMatchExpr(long e) {

  switch (((Obj*)(intptr_t)e)->tag) {
  case T_Match: { long scrut = ((Obj*)(intptr_t)e)->v0; long arms = ((Obj*)(intptr_t)e)->v1; return true; }
  case T_Num: { long v = ((Obj*)(intptr_t)e)->v0; return false; }
  case T_FloatLit: { long s = ((Obj*)(intptr_t)e)->v0; return false; }
  case T_Var: { long s = ((Obj*)(intptr_t)e)->v0; return false; }
  case T_StrLit: { long s = ((Obj*)(intptr_t)e)->v0; return false; }
  case T_ListLit: { long es = ((Obj*)(intptr_t)e)->v0; return false; }
  case T_Bin: { long op = ((Obj*)(intptr_t)e)->v0; long a = ((Obj*)(intptr_t)e)->v1; long b = ((Obj*)(intptr_t)e)->v2; return false; }
  case T_Call: { long name = ((Obj*)(intptr_t)e)->v0; long args = ((Obj*)(intptr_t)e)->v1; return false; }
  case T_Field: { long recv = ((Obj*)(intptr_t)e)->v0; long fld = ((Obj*)(intptr_t)e)->v1; return false; }
  case T_Method: { long recv = ((Obj*)(intptr_t)e)->v0; long name = ((Obj*)(intptr_t)e)->v1; long args = ((Obj*)(intptr_t)e)->v2; return false; }
  case T_Each: { long recv = ((Obj*)(intptr_t)e)->v0; long param = ((Obj*)(intptr_t)e)->v1; long body = ((Obj*)(intptr_t)e)->v2; return false; }
  }
  return 0;
  return 0;
}
long u_checkCall(long name, long args, long ctx) {
  long fns = 0;
  long k = 0;
  long m = 0;
  long f = 0;
  long recs = 0;
  long rec = 0;
  long types = 0;
  long ar = 0;

  fns = ((R_Sigs*)(intptr_t)((R_Ctx*)(intptr_t)ctx)->sigs)->fns;

  k = 0L;

  m = l_len(fns);

  while ((k < m)) {

  f = l_atc(fns, k, 4053);

  if (s_eq(((R_Fn*)(intptr_t)f)->name, name)) {

  if ((!({ long _t64 = l_len(args); long _t65 = l_len(((R_Fn*)(intptr_t)f)->params); (_t64 == _t65); }))) {

  u_failAt(({ long _t69 = s_concat(({ long _t67 = s_concat(name, (long)(intptr_t)" takes "); long _t68 = i_tostr(l_len(((R_Fn*)(intptr_t)f)->params)); s_concat(_t67, _t68); }), (long)(intptr_t)" values, got "); long _t70 = i_tostr(l_len(args)); s_concat(_t69, _t70); }), ctx);
  }

  u_checkArgTypes(name, ((R_Fn*)(intptr_t)f)->ptypes, args, ctx);
  }

  k = (k + 1L);
  }

  recs = ((R_Sigs*)(intptr_t)((R_Ctx*)(intptr_t)ctx)->sigs)->records;

  k = 0L;

  m = l_len(recs);

  while ((k < m)) {

  rec = l_atc(recs, k, 4066);

  if (s_eq(((R_RecDef*)(intptr_t)rec)->name, name)) {

  if ((!({ long _t80 = l_len(args); long _t81 = l_len(((R_RecDef*)(intptr_t)rec)->ftypes); (_t80 == _t81); }))) {

  u_failAt(s_concat(({ long _t85 = s_concat(({ long _t83 = s_concat(name, (long)(intptr_t)" has "); long _t84 = i_tostr(l_len(((R_RecDef*)(intptr_t)rec)->ftypes)); s_concat(_t83, _t84); }), (long)(intptr_t)" fields, got "); long _t86 = i_tostr(l_len(args)); s_concat(_t85, _t86); }), (long)(intptr_t)": a missing field or one too many"), ctx);
  }

  u_checkArgTypes(name, ((R_RecDef*)(intptr_t)rec)->ftypes, args, ctx);
  }

  k = (k + 1L);
  }

  types = ((R_Sigs*)(intptr_t)((R_Ctx*)(intptr_t)ctx)->sigs)->types;

  k = 0L;

  m = l_len(types);

  while ((k < m)) {

  ar = u_caseArityIn(l_atc(types, k, 4079), name);

  if (((!(ar < 0L)) && (!(l_len(args) == ar)))) {

  u_failAt(({ long _t98 = s_concat(({ long _t96 = s_concat(s_concat((long)(intptr_t)"case ", name), (long)(intptr_t)" takes "); long _t97 = i_tostr(ar); s_concat(_t96, _t97); }), (long)(intptr_t)" values, got "); long _t99 = i_tostr(l_len(args)); s_concat(_t98, _t99); }), ctx);
  }

  k = (k + 1L);
  }

  return 0L;
  return 0;
}
long u_checkArgTypes(long name, long ptypes, long args, long ctx) {
  long n = 0;
  long k = 0;
  long pt = 0;
  long at = 0;

  n = l_len(args);

  if ((n == l_len(ptypes))) {

  k = 0L;

  while ((k < n)) {

  pt = l_atc(ptypes, k, 4093);

  at = u_exprType(l_atc(args, k, 4094), ctx);

  if ((!u_compatible(pt, at, ctx))) {

  u_failAt(s_concat(s_concat(s_concat(s_concat(s_concat(s_concat(s_concat((long)(intptr_t)"argument ", i_tostr((k + 1L))), (long)(intptr_t)" of "), name), (long)(intptr_t)": expects "), pt), (long)(intptr_t)", got "), at), ctx);
  }

  k = (k + 1L);
  }
  }

  return 0L;
  return 0;
}
long u_compatible(long want, long got, long ctx) {
  long r = 0;

  r = false;

  if (s_eq(want, got)) {

  r = true;
  }

  if (s_eq(want, (long)(intptr_t)"")) {

  r = true;
  }

  if (s_eq(got, (long)(intptr_t)"")) {

  r = true;
  }

  if ((u_intLike(want) && u_intLike(got))) {

  r = true;
  }

  if ((u_isListType(want) && u_isListType(got))) {

  r = true;
  }

  return r;
  return 0;
}
long u_intLike(long t) {

  return (s_eq(t, (long)(intptr_t)"Int") || s_eq(t, (long)(intptr_t)"Bool"));
  return 0;
}
long u_notInt(long t, long ctx) {
  long r = 0;

  r = false;

  if (s_eq(t, (long)(intptr_t)"Str")) {

  r = true;
  }

  if (u_isListType(t)) {

  r = true;
  }

  if (u_isRec(t, ctx)) {

  r = true;
  }

  if (u_isVariant(t, ctx)) {

  r = true;
  }

  return r;
  return 0;
}
long u_varType(long s, long ctx) {
  long r = 0;
  long vt = 0;

  r = u_envGet(((R_Ctx*)(intptr_t)ctx)->env, s);

  if ((s_eq(s, (long)(intptr_t)"true") || s_eq(s, (long)(intptr_t)"false"))) {

  r = (long)(intptr_t)"Bool";
  }

  vt = u_variantOf(s, ctx);

  if ((!s_eq(vt, (long)(intptr_t)""))) {

  r = vt;
  }

  return r;
  return 0;
}
long u_isVariant(long t, long ctx) {
  long found = 0;
  long types = 0;
  long k = 0;
  long m = 0;

  found = false;

  types = ((R_Sigs*)(intptr_t)((R_Ctx*)(intptr_t)ctx)->sigs)->types;

  k = 0L;

  m = l_len(types);

  while ((k < m)) {

  if (s_eq(((R_TyDef*)(intptr_t)l_atc(types, k, 4170))->name, t)) {

  found = true;
  }

  k = (k + 1L);
  }

  return found;
  return 0;
}
long u_emitTermField(long fld, long ctx) {
  long r = 0;

  r = (long)(intptr_t)"";

  if (s_eq(fld, (long)(intptr_t)"key")) {

  r = (long)(intptr_t)"simpler_term_key()";
  }

  if (s_eq(fld, (long)(intptr_t)"rows")) {

  r = (long)(intptr_t)"simpler_term_size(0)";
  }

  if (s_eq(fld, (long)(intptr_t)"cols")) {

  r = (long)(intptr_t)"simpler_term_size(1)";
  }

  if (s_eq(fld, (long)(intptr_t)"clear")) {

  r = (long)(intptr_t)"simpler_term_clear()";
  }

  if (s_eq(fld, (long)(intptr_t)"stop")) {

  r = (long)(intptr_t)"simpler_term_stop()";
  }

  if (s_eq(r, (long)(intptr_t)"")) {

  u_failAt(s_concat((long)(intptr_t)"a Term has key, rows, cols, clear, stop and put(row, col, text, style); it has no ", fld), ctx);
  }

  return r;
  return 0;
}
long u_emitField(long recv, long fld, long ctx) {
  long t = 0;
  long recvC = 0;
  long r = 0;
  long flag = 0;

  t = u_exprType(recv, ctx);

  if (s_eq(t, (long)(intptr_t)"Term")) {

  return u_emitTermField(fld, ctx);
  }

  u_checkSysField(t, fld, ctx);

  u_checkField(t, fld, ctx);

  recvC = u_emitExpr(recv, ctx);

  r = s_concat(s_concat(s_concat(s_concat(s_concat((long)(intptr_t)"((R_", t), (long)(intptr_t)"*)(intptr_t)"), recvC), (long)(intptr_t)")->"), fld);

  if ((!u_isRec(t, ctx))) {

  if (s_eq(fld, (long)(intptr_t)"length")) {

  r = s_concat(s_concat((long)(intptr_t)"s_len(", recvC), (long)(intptr_t)")");

  if (s_eq(t, (long)(intptr_t)"Bytes")) {

  r = s_concat(s_concat((long)(intptr_t)"y_len(", recvC), (long)(intptr_t)")");
  }

  if (u_isListType(t)) {

  r = s_concat(s_concat((long)(intptr_t)"l_len(", recvC), (long)(intptr_t)")");
  }
  }

  if (s_eq(fld, (long)(intptr_t)"code")) {

  r = s_concat(s_concat((long)(intptr_t)"s_code(", recvC), (long)(intptr_t)")");
  }

  if (s_eq(fld, (long)(intptr_t)"toStr")) {

  r = s_concat(s_concat((long)(intptr_t)"i_tostr(", recvC), (long)(intptr_t)")");

  if (s_eq(t, (long)(intptr_t)"Float")) {

  r = s_concat(s_concat((long)(intptr_t)"f_tostr(", recvC), (long)(intptr_t)")");
  }
  }

  if (s_eq(fld, (long)(intptr_t)"isInt")) {

  r = s_concat(s_concat((long)(intptr_t)"s_isint(", recvC), (long)(intptr_t)")");
  }

  if (s_eq(fld, (long)(intptr_t)"toInt")) {

  r = s_concat(({ long _t183 = s_concat(s_concat((long)(intptr_t)"s_tointc(", recvC), (long)(intptr_t)", "); long _t184 = u_envGet(((R_Ctx*)(intptr_t)ctx)->env, (long)(intptr_t)"__line__"); s_concat(_t183, _t184); }), (long)(intptr_t)")");

  if (s_eq(t, (long)(intptr_t)"Float")) {

  r = s_concat(s_concat((long)(intptr_t)"f_toi(", recvC), (long)(intptr_t)")");
  }
  }

  if (s_eq(fld, (long)(intptr_t)"toFloat")) {

  r = s_concat(s_concat((long)(intptr_t)"i_tof(", recvC), (long)(intptr_t)")");

  if (s_eq(t, (long)(intptr_t)"Str")) {

  r = s_concat(s_concat((long)(intptr_t)"s_tofloat(", recvC), (long)(intptr_t)")");
  }
  }

  if (s_eq(fld, (long)(intptr_t)"trim")) {

  r = s_concat(s_concat((long)(intptr_t)"s_trim(", recvC), (long)(intptr_t)")");
  }

  if (s_eq(fld, (long)(intptr_t)"upper")) {

  r = s_concat(s_concat((long)(intptr_t)"s_case(", recvC), (long)(intptr_t)", 1)");
  }

  if (s_eq(fld, (long)(intptr_t)"bytes")) {

  r = s_concat(s_concat((long)(intptr_t)"s_bytes(", recvC), (long)(intptr_t)")");
  }

  if (s_eq(fld, (long)(intptr_t)"jsonItems")) {

  r = s_concat(s_concat((long)(intptr_t)"s_jsonlist(", recvC), (long)(intptr_t)", 0)");
  }

  if (s_eq(fld, (long)(intptr_t)"jsonKeys")) {

  r = s_concat(s_concat((long)(intptr_t)"s_jsonlist(", recvC), (long)(intptr_t)", 1)");
  }

  if (s_eq(fld, (long)(intptr_t)"lower")) {

  r = s_concat(s_concat((long)(intptr_t)"s_case(", recvC), (long)(intptr_t)", 0)");
  }

  if (s_eq(fld, (long)(intptr_t)"sin")) {

  r = s_concat(s_concat((long)(intptr_t)"d2l(sin(l2d(", recvC), (long)(intptr_t)")))");
  }

  if (s_eq(fld, (long)(intptr_t)"cos")) {

  r = s_concat(s_concat((long)(intptr_t)"d2l(cos(l2d(", recvC), (long)(intptr_t)")))");
  }

  if (s_eq(fld, (long)(intptr_t)"tan")) {

  r = s_concat(s_concat((long)(intptr_t)"d2l(tan(l2d(", recvC), (long)(intptr_t)")))");
  }

  if (s_eq(fld, (long)(intptr_t)"asin")) {

  r = s_concat(s_concat((long)(intptr_t)"d2l(asin(l2d(", recvC), (long)(intptr_t)")))");
  }

  if (s_eq(fld, (long)(intptr_t)"acos")) {

  r = s_concat(s_concat((long)(intptr_t)"d2l(acos(l2d(", recvC), (long)(intptr_t)")))");
  }

  if (s_eq(fld, (long)(intptr_t)"sqrt")) {

  r = s_concat(s_concat((long)(intptr_t)"d2l(sqrt(l2d(", recvC), (long)(intptr_t)")))");
  }

  if (s_eq(fld, (long)(intptr_t)"floor")) {

  r = s_concat(s_concat((long)(intptr_t)"d2l(floor(l2d(", recvC), (long)(intptr_t)")))");
  }

  if (s_eq(fld, (long)(intptr_t)"abs")) {

  r = s_concat(s_concat((long)(intptr_t)"d2l(fabs(l2d(", recvC), (long)(intptr_t)")))");
  }

  if (s_eq(fld, (long)(intptr_t)"atan")) {

  r = s_concat(s_concat((long)(intptr_t)"d2l(atan(l2d(", recvC), (long)(intptr_t)")))");
  }

  if ((s_eq(fld, (long)(intptr_t)"ceil") || s_eq(fld, (long)(intptr_t)"ceil_c"))) {

  r = s_concat(s_concat((long)(intptr_t)"d2l(ceil(l2d(", recvC), (long)(intptr_t)")))");
  }

  if ((s_eq(fld, (long)(intptr_t)"round") || s_eq(fld, (long)(intptr_t)"round_c"))) {

  r = s_concat(s_concat((long)(intptr_t)"d2l(round(l2d(", recvC), (long)(intptr_t)")))");
  }

  if (s_eq(fld, (long)(intptr_t)"ln")) {

  r = s_concat(s_concat((long)(intptr_t)"d2l(log(l2d(", recvC), (long)(intptr_t)")))");
  }

  if (s_eq(fld, (long)(intptr_t)"exp")) {

  r = s_concat(s_concat((long)(intptr_t)"d2l(exp(l2d(", recvC), (long)(intptr_t)")))");
  }

  if (s_eq(fld, (long)(intptr_t)"not")) {

  r = s_concat(s_concat((long)(intptr_t)"(!", recvC), (long)(intptr_t)")");
  }

  if (u_isCapField(fld)) {

  r = (long)(intptr_t)"0";
  }

  if (s_eq(fld, (long)(intptr_t)"args")) {

  r = (long)(intptr_t)"simpler_args(argc, argv)";
  }

  if (s_eq(fld, (long)(intptr_t)"stdin")) {

  r = (long)(intptr_t)"simpler_stdin()";
  }

  if (s_eq(fld, (long)(intptr_t)"now")) {

  if ((!s_eq(t, (long)(intptr_t)"Clock"))) {

  u_failAt(s_concat((long)(intptr_t)".now needs a Clock handed down from main, got ", u_capShow(t)), ctx);
  }

  r = (long)(intptr_t)"simpler_now()";
  }

  if (s_eq(fld, (long)(intptr_t)"self")) {

  if ((!s_eq(t, (long)(intptr_t)"Env"))) {

  u_failAt(s_concat((long)(intptr_t)".self is the program's own path and needs sys.env, got ", u_capShow(t)), ctx);
  }

  r = (long)(intptr_t)"simpler_self()";
  }

  if (s_eq(fld, (long)(intptr_t)"millis")) {

  if ((!s_eq(t, (long)(intptr_t)"Clock"))) {

  u_failAt(s_concat((long)(intptr_t)".millis needs a Clock handed down from main, got ", u_capShow(t)), ctx);
  }

  r = (long)(intptr_t)"simpler_millis()";
  }

  if ((s_eq(fld, (long)(intptr_t)"char") || s_eq(fld, (long)(intptr_t)"char_c"))) {

  if ((!s_eq(t, (long)(intptr_t)"Int"))) {

  u_failAt(s_concat((long)(intptr_t)".char turns a number into its character and needs an Int, got ", u_capShow(t)), ctx);
  }

  r = s_concat(s_concat((long)(intptr_t)"i_char(", recvC), (long)(intptr_t)")");
  }

  if (s_eq(fld, (long)(intptr_t)"keys")) {

  r = s_concat(s_concat((long)(intptr_t)"m_keys(", recvC), (long)(intptr_t)")");
  }

  if (s_eq(fld, (long)(intptr_t)"byValue")) {

  r = s_concat(s_concat((long)(intptr_t)"m_byvalue(", recvC), (long)(intptr_t)")");
  }

  if (s_eq(fld, (long)(intptr_t)"sort")) {

  flag = (long)(intptr_t)"0";

  if (s_eq(u_elemOf(t), (long)(intptr_t)"Str")) {

  flag = (long)(intptr_t)"1";
  }

  r = s_concat(s_concat(s_concat(s_concat((long)(intptr_t)"l_sort(", recvC), (long)(intptr_t)", "), flag), (long)(intptr_t)")");
  }
  }

  if (u_useUFCS(t, fld, ctx)) {

  r = s_concat(s_concat(s_concat(u_mangle(fld), (long)(intptr_t)"("), recvC), (long)(intptr_t)")");
  }

  return r;
  return 0;
}
long u_isCapField(long fld) {

  return (((((((s_eq(fld, (long)(intptr_t)"screen") || s_eq(fld, (long)(intptr_t)"files")) || s_eq(fld, (long)(intptr_t)"mail")) || s_eq(fld, (long)(intptr_t)"programs")) || s_eq(fld, (long)(intptr_t)"net")) || s_eq(fld, (long)(intptr_t)"clock")) || s_eq(fld, (long)(intptr_t)"env")) || s_eq(fld, (long)(intptr_t)"term"));
  return 0;
}
long u_isBuiltinField(long fld) {

  return ((((((((((((((((((((((((s_eq(fld, (long)(intptr_t)"length") || s_eq(fld, (long)(intptr_t)"code")) || s_eq(fld, (long)(intptr_t)"toStr")) || s_eq(fld, (long)(intptr_t)"toInt")) || s_eq(fld, (long)(intptr_t)"isInt")) || s_eq(fld, (long)(intptr_t)"toFloat")) || s_eq(fld, (long)(intptr_t)"trim")) || s_eq(fld, (long)(intptr_t)"upper")) || s_eq(fld, (long)(intptr_t)"lower")) || s_eq(fld, (long)(intptr_t)"jsonItems")) || s_eq(fld, (long)(intptr_t)"jsonKeys")) || s_eq(fld, (long)(intptr_t)"bytes")) || s_eq(fld, (long)(intptr_t)"not")) || s_eq(fld, (long)(intptr_t)"keys")) || s_eq(fld, (long)(intptr_t)"byValue")) || s_eq(fld, (long)(intptr_t)"sort")) || s_eq(fld, (long)(intptr_t)"args")) || s_eq(fld, (long)(intptr_t)"stdin")) || s_eq(fld, (long)(intptr_t)"now")) || s_eq(fld, (long)(intptr_t)"millis")) || s_eq(fld, (long)(intptr_t)"self")) || s_eq(fld, (long)(intptr_t)"char")) || s_eq(fld, (long)(intptr_t)"char_c")) || u_isMathField(fld)) || u_isCapField(fld));
  return 0;
}
long u_isMathField(long fld) {

  return ((((((((((((((s_eq(fld, (long)(intptr_t)"sin") || s_eq(fld, (long)(intptr_t)"cos")) || s_eq(fld, (long)(intptr_t)"tan")) || s_eq(fld, (long)(intptr_t)"asin")) || s_eq(fld, (long)(intptr_t)"acos")) || s_eq(fld, (long)(intptr_t)"atan")) || s_eq(fld, (long)(intptr_t)"sqrt")) || s_eq(fld, (long)(intptr_t)"floor")) || s_eq(fld, (long)(intptr_t)"ceil")) || s_eq(fld, (long)(intptr_t)"round")) || s_eq(fld, (long)(intptr_t)"ceil_c")) || s_eq(fld, (long)(intptr_t)"round_c")) || s_eq(fld, (long)(intptr_t)"ln")) || s_eq(fld, (long)(intptr_t)"exp")) || s_eq(fld, (long)(intptr_t)"abs"));
  return 0;
}
long u_recHasField(long t, long fld, long ctx) {
  long found = 0;
  long recs = 0;
  long k = 0;
  long m = 0;
  long rec = 0;

  found = false;

  recs = ((R_Sigs*)(intptr_t)((R_Ctx*)(intptr_t)ctx)->sigs)->records;

  k = 0L;

  m = l_len(recs);

  while ((k < m)) {

  rec = l_atc(recs, k, 4382);

  if (s_eq(((R_RecDef*)(intptr_t)rec)->name, t)) {

  if (u_hasName(((R_RecDef*)(intptr_t)rec)->fields, fld)) {

  found = true;
  }
  }

  k = (k + 1L);
  }

  return found;
  return 0;
}
long u_useUFCS(long t, long fld, long ctx) {
  long r = 0;

  r = false;

  if (u_hasName(((R_Sigs*)(intptr_t)((R_Ctx*)(intptr_t)ctx)->sigs)->fnNames, fld)) {

  if ((!u_isBuiltinField(fld))) {

  if ((!u_recHasField(t, fld, ctx))) {

  r = true;
  }
  }
  }

  return r;
  return 0;
}
long u_capNeed(long name) {
  long r = 0;

  r = (long)(intptr_t)"";

  if (((s_eq(name, (long)(intptr_t)"print") || s_eq(name, (long)(intptr_t)"part")) || s_eq(name, (long)(intptr_t)"error"))) {

  r = (long)(intptr_t)"Screen";
  }

  if (((((((((s_eq(name, (long)(intptr_t)"read") || s_eq(name, (long)(intptr_t)"write")) || s_eq(name, (long)(intptr_t)"readBytes")) || s_eq(name, (long)(intptr_t)"writeBytes")) || s_eq(name, (long)(intptr_t)"append")) || s_eq(name, (long)(intptr_t)"list")) || s_eq(name, (long)(intptr_t)"exists")) || s_eq(name, (long)(intptr_t)"rename")) || s_eq(name, (long)(intptr_t)"delete"))) {

  r = (long)(intptr_t)"Files";
  }

  if (s_eq(name, (long)(intptr_t)"send")) {

  r = (long)(intptr_t)"Mail";
  }

  if (((((s_eq(name, (long)(intptr_t)"run") || s_eq(name, (long)(intptr_t)"status")) || s_eq(name, (long)(intptr_t)"start")) || s_eq(name, (long)(intptr_t)"try")) || s_eq(name, (long)(intptr_t)"feed"))) {

  r = (long)(intptr_t)"Programs";
  }

  if ((((s_eq(name, (long)(intptr_t)"fetch") || s_eq(name, (long)(intptr_t)"post")) || s_eq(name, (long)(intptr_t)"reach")) || s_eq(name, (long)(intptr_t)"fetchWith"))) {

  r = (long)(intptr_t)"Net";
  }

  if (((s_eq(name, (long)(intptr_t)"date") || s_eq(name, (long)(intptr_t)"dateAs")) || s_eq(name, (long)(intptr_t)"secsOf"))) {

  r = (long)(intptr_t)"Clock";
  }

  if ((s_eq(name, (long)(intptr_t)"var") || s_eq(name, (long)(intptr_t)"setVar"))) {

  r = (long)(intptr_t)"Env";
  }

  return r;
  return 0;
}
long u_checkCap(long recv, long name, long ctx) {
  long need = 0;
  long t = 0;

  need = u_capNeed(name);

  if ((!s_eq(need, (long)(intptr_t)""))) {

  t = u_exprType(recv, ctx);

  if ((!s_eq(t, need))) {

  u_failAt(({ long _t21 = s_concat(s_concat(s_concat(s_concat((long)(intptr_t)".", name), (long)(intptr_t)" needs a "), need), (long)(intptr_t)" handed down from main, got "); long _t22 = u_capShow(t); s_concat(_t21, _t22); }), ctx);
  }
  }

  return 0L;
  return 0;
}
long u_wantType(long e, long want, long name, long ctx) {
  long got = 0;

  got = u_exprType(e, ctx);

  if ((!u_compatible(want, got, ctx))) {

  u_failAt(s_concat(s_concat(s_concat(s_concat(name, (long)(intptr_t)" expects "), want), (long)(intptr_t)", got "), got), ctx);
  }

  return 0L;
  return 0;
}
long u_capShow(long t) {
  long r = 0;

  r = t;

  if (s_eq(t, (long)(intptr_t)"")) {

  r = (long)(intptr_t)"a value with no type";
  }

  return r;
  return 0;
}
long u_checkField(long t, long fld, long ctx) {

  if ((!u_useUFCS(t, fld, ctx))) {

  if ((u_isRec(t, ctx) && (!u_recHasField(t, fld, ctx)))) {

  u_failAt(s_concat(s_concat(t, (long)(intptr_t)" has no field "), fld), ctx);
  }

  if (u_isVariant(t, ctx)) {

  u_failAt(s_concat(s_concat(s_concat(t, (long)(intptr_t)" is a variant and has no field "), fld), (long)(intptr_t)"; read it with match"), ctx);
  }

  if ((u_isNumType(t) && u_isTextField(fld))) {

  u_failAt(s_concat(s_concat(t, (long)(intptr_t)" has no method "), fld), ctx);
  }

  if ((s_eq(t, (long)(intptr_t)"Bytes") && (!s_eq(fld, (long)(intptr_t)"length")))) {

  u_failAt(s_concat((long)(intptr_t)"Bytes has length, at(i), intAt(i, n), floatAt(i), textAt(i, n) and findByte(n, from); no ", fld), ctx);
  }

  if (((!u_isRec(t, ctx)) && (!u_isBuiltinField(fld)))) {

  u_failAt(s_concat((long)(intptr_t)"no field or function named ", fld), ctx);
  }
  }

  return 0L;
  return 0;
}
long u_isNumType(long t) {

  return ((s_eq(t, (long)(intptr_t)"Int") || s_eq(t, (long)(intptr_t)"Float")) || s_eq(t, (long)(intptr_t)"Bool"));
  return 0;
}
long u_isTextField(long fld) {

  return (((((((((((s_eq(fld, (long)(intptr_t)"length") || s_eq(fld, (long)(intptr_t)"trim")) || s_eq(fld, (long)(intptr_t)"upper")) || s_eq(fld, (long)(intptr_t)"lower")) || s_eq(fld, (long)(intptr_t)"jsonItems")) || s_eq(fld, (long)(intptr_t)"jsonKeys")) || s_eq(fld, (long)(intptr_t)"bytes")) || s_eq(fld, (long)(intptr_t)"code")) || s_eq(fld, (long)(intptr_t)"isInt")) || s_eq(fld, (long)(intptr_t)"keys")) || s_eq(fld, (long)(intptr_t)"byValue")) || s_eq(fld, (long)(intptr_t)"sort"));
  return 0;
}
long u_isTextMethod(long name) {

  return ((((((((((((((((((s_eq(name, (long)(intptr_t)"concat") || s_eq(name, (long)(intptr_t)"slice")) || s_eq(name, (long)(intptr_t)"split")) || s_eq(name, (long)(intptr_t)"contains")) || s_eq(name, (long)(intptr_t)"find")) || s_eq(name, (long)(intptr_t)"json")) || s_eq(name, (long)(intptr_t)"reTest")) || s_eq(name, (long)(intptr_t)"reFind")) || s_eq(name, (long)(intptr_t)"reAll")) || s_eq(name, (long)(intptr_t)"reSplit")) || s_eq(name, (long)(intptr_t)"reReplace")) || s_eq(name, (long)(intptr_t)"join")) || s_eq(name, (long)(intptr_t)"startsWith")) || s_eq(name, (long)(intptr_t)"endsWith")) || s_eq(name, (long)(intptr_t)"replace")) || s_eq(name, (long)(intptr_t)"at")) || s_eq(name, (long)(intptr_t)"push")) || s_eq(name, (long)(intptr_t)"padLeft")) || s_eq(name, (long)(intptr_t)"padRight"));
  return 0;
}
long u_checkMethod(long t, long name, long args, long ctx) {
  long at = 0;
  long pt = 0;

  if (true) {

  if ((u_isNumType(t) && u_isTextMethod(name))) {

  u_failAt(s_concat(s_concat(t, (long)(intptr_t)" has no method "), name), ctx);
  }

  if ((((((s_eq(t, (long)(intptr_t)"Bytes") && (!s_eq(name, (long)(intptr_t)"at"))) && (!s_eq(name, (long)(intptr_t)"intAt"))) && (!s_eq(name, (long)(intptr_t)"floatAt"))) && (!s_eq(name, (long)(intptr_t)"textAt"))) && (!s_eq(name, (long)(intptr_t)"findByte")))) {

  u_failAt(s_concat((long)(intptr_t)"Bytes has length, at(i), intAt(i, n), floatAt(i), textAt(i, n) and findByte(n, from); no ", name), ctx);
  }

  if ((s_eq(name, (long)(intptr_t)"at") && (l_len(args) == 1L))) {

  at = u_exprType(l_atc(args, 0L, 4510), ctx);

  if ((s_eq(at, (long)(intptr_t)"Str") || s_eq(at, (long)(intptr_t)"Float"))) {

  u_failAt(s_concat((long)(intptr_t)"at expects Int, got ", at), ctx);
  }
  }

  if (((s_eq(name, (long)(intptr_t)"push") && u_isListType(t)) && (l_len(args) == 1L))) {

  pt = u_exprType(l_atc(args, 0L, 4516), ctx);

  if ((!u_compatible(u_elemOf(t), pt, ctx))) {

  u_failAt(s_concat(s_concat(s_concat((long)(intptr_t)"push expects ", u_elemOf(t)), (long)(intptr_t)", got "), pt), ctx);
  }
  }

  if (((s_eq(name, (long)(intptr_t)"set") && u_isListType(t)) && (l_len(args) == 2L))) {

  if ((!s_eq(u_exprType(l_atc(args, 0L, 4522), ctx), (long)(intptr_t)"Int"))) {

  u_failAt(s_concat((long)(intptr_t)"set on a list expects the place as an Int, got ", u_exprType(l_atc(args, 0L, 4523), ctx)), ctx);
  }

  pt = u_exprType(l_atc(args, 1L, 4525), ctx);

  if ((!u_compatible(u_elemOf(t), pt, ctx))) {

  u_failAt(s_concat(s_concat(s_concat((long)(intptr_t)"set expects ", u_elemOf(t)), (long)(intptr_t)", got "), pt), ctx);
  }
  }

  if ((s_eq(name, (long)(intptr_t)"join") && (l_len(args) == 1L))) {

  if ((!s_eq(t, (long)(intptr_t)"List[Str]"))) {

  u_failAt(s_concat(s_concat((long)(intptr_t)"join puts a list of texts together; ", t), (long)(intptr_t)" has no method join"), ctx);
  }

  if ((!s_eq(u_exprType(l_atc(args, 0L, 4534), ctx), (long)(intptr_t)"Str"))) {

  u_failAt(s_concat((long)(intptr_t)"join expects the text to put between, got ", u_exprType(l_atc(args, 0L, 4535), ctx)), ctx);
  }
  }

  if (s_eq(name, (long)(intptr_t)"findByte")) {

  if ((((!s_eq(t, (long)(intptr_t)"Bytes")) || (!s_eq(u_exprType(l_atc(args, 0L, 4539), ctx), (long)(intptr_t)"Int"))) || (!s_eq(u_exprType(l_atc(args, 1L, 4539), ctx), (long)(intptr_t)"Int")))) {

  u_failAt((long)(intptr_t)"findByte(n, from) gives the place of the next byte n in Bytes, or -1", ctx);
  }
  }

  if (((s_eq(name, (long)(intptr_t)"textAt") || s_eq(name, (long)(intptr_t)"intAt")) || s_eq(name, (long)(intptr_t)"floatAt"))) {

  if (((!s_eq(t, (long)(intptr_t)"List[Int]")) && (!s_eq(t, (long)(intptr_t)"Bytes")))) {

  u_failAt(s_concat(s_concat(name, (long)(intptr_t)" reads from Bytes or a List[Int]; got "), t), ctx);
  }

  {
  long _lst = args;
  long _n = l_len(_lst);
  for (long _i = 0; _i < _n; _i = _i + 1) {
  long a = l_at(_lst, _i);

  if ((!s_eq(u_exprType(a, ctx), (long)(intptr_t)"Int"))) {

  u_failAt(({ long _t124 = s_concat(name, (long)(intptr_t)" expects Int, got "); long _t125 = u_exprType(a, ctx); s_concat(_t124, _t125); }), ctx);
  }
  }
  }
  }

  if ((s_eq(name, (long)(intptr_t)"writeBytes") && (l_len(args) == 2L))) {

  if ((!s_eq(u_exprType(l_atc(args, 1L, 4554), ctx), (long)(intptr_t)"List[Int]"))) {

  u_failAt(s_concat((long)(intptr_t)"writeBytes takes a path and a List[Int] of bytes, got ", u_exprType(l_atc(args, 1L, 4555), ctx)), ctx);
  }
  }

  if ((((s_eq(name, (long)(intptr_t)"print") || s_eq(name, (long)(intptr_t)"part")) || s_eq(name, (long)(intptr_t)"error")) && (l_len(args) == 1L))) {

  if (s_eq(u_exprType(l_atc(args, 0L, 4559), ctx), (long)(intptr_t)"Bytes")) {

  u_failAt((long)(intptr_t)"Bytes cannot be printed; print a part with textAt(i, n) or a number with at(i)", ctx);
  }
  }

  if (((((s_eq(name, (long)(intptr_t)"reTest") || s_eq(name, (long)(intptr_t)"reFind")) || s_eq(name, (long)(intptr_t)"reAll")) || s_eq(name, (long)(intptr_t)"reSplit")) || s_eq(name, (long)(intptr_t)"reReplace"))) {

  if ((!s_eq(t, (long)(intptr_t)"Str"))) {

  u_failAt(s_concat(s_concat(name, (long)(intptr_t)" looks for a pattern in a text; got "), t), ctx);
  }

  {
  long _lst = args;
  long _n = l_len(_lst);
  for (long _i = 0; _i < _n; _i = _i + 1) {
  long a = l_at(_lst, _i);

  if ((!s_eq(u_exprType(a, ctx), (long)(intptr_t)"Str"))) {

  u_failAt(({ long _t140 = s_concat(name, (long)(intptr_t)" takes its pattern as a text, got "); long _t141 = u_exprType(a, ctx); s_concat(_t140, _t141); }), ctx);
  }
  }
  }
  }

  if ((s_eq(name, (long)(intptr_t)"json") && (l_len(args) == 1L))) {

  if (((!s_eq(t, (long)(intptr_t)"Str")) || (!s_eq(u_exprType(l_atc(args, 0L, 4574), ctx), (long)(intptr_t)"Str")))) {

  u_failAt((long)(intptr_t)"json takes a key or a place as a text, and gives the value there as a text", ctx);
  }
  }

  if ((s_eq(name, (long)(intptr_t)"find") && (l_len(args) == 1L))) {

  if (((!s_eq(t, (long)(intptr_t)"Str")) || (!s_eq(u_exprType(l_atc(args, 0L, 4579), ctx), (long)(intptr_t)"Str")))) {

  u_failAt((long)(intptr_t)"find looks for a text in a text and gives its place, or -1", ctx);
  }
  }

  if (s_eq(name, (long)(intptr_t)"remove")) {

  if ((!u_isListType(t))) {

  u_failAt(s_concat(s_concat((long)(intptr_t)"remove takes an item out of a list; ", t), (long)(intptr_t)" has no method remove"), ctx);
  }

  if ((!s_eq(u_exprType(l_atc(args, 0L, 4587), ctx), (long)(intptr_t)"Int"))) {

  u_failAt(s_concat((long)(intptr_t)"remove expects the place as an Int, got ", u_exprType(l_atc(args, 0L, 4588), ctx)), ctx);
  }
  }
  }

  return 0L;
  return 0;
}
long u_isSysField(long fld) {

  return ((u_isCapField(fld) || s_eq(fld, (long)(intptr_t)"args")) || s_eq(fld, (long)(intptr_t)"stdin"));
  return 0;
}
long u_checkSysField(long t, long fld, long ctx) {

  if (u_isSysField(fld)) {

  if ((!u_isRec(t, ctx))) {

  if ((!s_eq(t, (long)(intptr_t)"Sys"))) {

  u_failAt(s_concat(s_concat((long)(intptr_t)".", fld), (long)(intptr_t)" exists only on main's world value; pass it down as a parameter"), ctx);
  }
  }
  }

  return 0L;
  return 0;
}
long u_methodArgs(long name) {
  long r = 0;

  r = (0L - 1L);

  if (((((((((((((((((((((((((((((u_isTextMethod(name) || s_eq(name, (long)(intptr_t)"print")) || s_eq(name, (long)(intptr_t)"read")) || s_eq(name, (long)(intptr_t)"list")) || s_eq(name, (long)(intptr_t)"exists")) || s_eq(name, (long)(intptr_t)"delete")) || s_eq(name, (long)(intptr_t)"remove")) || s_eq(name, (long)(intptr_t)"fetch")) || s_eq(name, (long)(intptr_t)"reach")) || s_eq(name, (long)(intptr_t)"part")) || s_eq(name, (long)(intptr_t)"error")) || s_eq(name, (long)(intptr_t)"atan2")) || s_eq(name, (long)(intptr_t)"pow")) || s_eq(name, (long)(intptr_t)"atan2_c")) || s_eq(name, (long)(intptr_t)"pow_c")) || s_eq(name, (long)(intptr_t)"readBytes")) || s_eq(name, (long)(intptr_t)"floatAt")) || s_eq(name, (long)(intptr_t)"secsOf")) || s_eq(name, (long)(intptr_t)"date")) || s_eq(name, (long)(intptr_t)"var")) || s_eq(name, (long)(intptr_t)"mod")) || s_eq(name, (long)(intptr_t)"atan2")) || s_eq(name, (long)(intptr_t)"get")) || s_eq(name, (long)(intptr_t)"has")) || s_eq(name, (long)(intptr_t)"ge")) || s_eq(name, (long)(intptr_t)"le")) || s_eq(name, (long)(intptr_t)"and")) || s_eq(name, (long)(intptr_t)"or")) || s_eq(name, (long)(intptr_t)"send"))) {

  r = 1L;
  }

  if (((((((((((((((((((s_eq(name, (long)(intptr_t)"setVar") || s_eq(name, (long)(intptr_t)"slice")) || s_eq(name, (long)(intptr_t)"replace")) || s_eq(name, (long)(intptr_t)"write")) || s_eq(name, (long)(intptr_t)"append")) || s_eq(name, (long)(intptr_t)"rename")) || s_eq(name, (long)(intptr_t)"set")) || s_eq(name, (long)(intptr_t)"run")) || s_eq(name, (long)(intptr_t)"status")) || s_eq(name, (long)(intptr_t)"start")) || s_eq(name, (long)(intptr_t)"try")) || s_eq(name, (long)(intptr_t)"post")) || s_eq(name, (long)(intptr_t)"fetchWith")) || s_eq(name, (long)(intptr_t)"dateAs")) || s_eq(name, (long)(intptr_t)"writeBytes")) || s_eq(name, (long)(intptr_t)"textAt")) || s_eq(name, (long)(intptr_t)"intAt")) || s_eq(name, (long)(intptr_t)"findByte")) || s_eq(name, (long)(intptr_t)"reReplace"))) {

  r = 2L;
  }

  if ((s_eq(name, (long)(intptr_t)"feed") || s_eq(name, (long)(intptr_t)"send"))) {

  r = 3L;
  }

  return r;
  return 0;
}
long u_emitSetField(long recv, long fld, long args, long ctx) {
  long t = 0;
  long recvC = 0;

  t = u_exprType(recv, ctx);

  if ((!u_isRec(t, ctx))) {

  u_failAt(s_concat((long)(intptr_t)"only a field of a record can be set with =, and this is ", u_capShow(t)), ctx);
  }

  u_checkField(t, fld, ctx);

  ({ long _t24 = l_atc(args, 0L, 4631); long _t25 = u_fieldType(recv, fld, ctx); long _t26 = s_concat((long)(intptr_t)".", fld); u_wantType(_t24, _t25, _t26, ctx); });

  recvC = u_emitExpr(recv, ctx);

  return s_concat(({ long _t31 = s_concat(s_concat(s_concat(s_concat(s_concat(s_concat((long)(intptr_t)"(((R_", t), (long)(intptr_t)"*)(intptr_t)"), recvC), (long)(intptr_t)")->"), fld), (long)(intptr_t)" = "); long _t32 = u_emitExpr(l_atc(args, 0L, 4633), ctx); s_concat(_t31, _t32); }), (long)(intptr_t)", 0L)");
  return 0;
}
long u_emitMethod0(long recv, long name, long args, long ctx) {
  long rv = 0;
  long t = 0;
  long want = 0;
  long recvC = 0;
  long ln = 0;
  long r = 0;
  long mid = 0;
  long cf = 0;
  long mv = 0;

  if (u_isSetField(name)) {

  return u_emitSetField(recv, s_slicec(name, 1L, s_len(name), 4638), args, ctx);
  }

  rv = u_rootVar(recv);

  if ((((((((!s_eq(rv, (long)(intptr_t)"")) && s_eq(u_capNeed(name), (long)(intptr_t)"")) && (!s_eq(rv, (long)(intptr_t)"sys"))) && (!s_eq(rv, (long)(intptr_t)"true"))) && (!s_eq(rv, (long)(intptr_t)"false"))) && s_eq(u_variantOf(rv, ctx), (long)(intptr_t)"")) && s_eq(u_envOld(((R_Ctx*)(intptr_t)ctx)->env, rv), (long)(intptr_t)"__gone__"))) {

  u_failAt(s_concat(s_concat(s_concat(s_concat((long)(intptr_t)"no variable named ", rv), (long)(intptr_t)"; if it is a module, put  uses \""), rv), (long)(intptr_t)"\"  at the top"), ctx);
  }

  t = u_exprType(recv, ctx);

  if (s_eq(t, (long)(intptr_t)"Term")) {

  if (((!s_eq(name, (long)(intptr_t)"put")) || (!(l_len(args) == 4L)))) {

  u_failAt((long)(intptr_t)"a Term has key, rows, cols, clear, stop and put(row, col, text, style)", ctx);
  }

  u_wantType(l_atc(args, 0L, 4649), (long)(intptr_t)"Int", (long)(intptr_t)"put", ctx);

  u_wantType(l_atc(args, 1L, 4650), (long)(intptr_t)"Int", (long)(intptr_t)"put", ctx);

  u_wantType(l_atc(args, 2L, 4651), (long)(intptr_t)"Str", (long)(intptr_t)"put", ctx);

  u_wantType(l_atc(args, 3L, 4652), (long)(intptr_t)"Str", (long)(intptr_t)"put", ctx);

  return s_concat(({ long _t306 = s_concat(({ long _t304 = s_concat(({ long _t302 = s_concat(({ long _t300 = s_concat(s_concat((long)(intptr_t)"simpler_term_put(", u_emitExpr(l_atc(args, 0L, 4653), ctx)), (long)(intptr_t)", "); long _t301 = u_emitExpr(l_atc(args, 1L, 4653), ctx); s_concat(_t300, _t301); }), (long)(intptr_t)", "); long _t303 = u_emitExpr(l_atc(args, 2L, 4653), ctx); s_concat(_t302, _t303); }), (long)(intptr_t)", "); long _t305 = u_emitExpr(l_atc(args, 3L, 4653), ctx); s_concat(_t304, _t305); }), (long)(intptr_t)", "); long _t307 = u_envGet(((R_Ctx*)(intptr_t)ctx)->env, (long)(intptr_t)"__line__"); s_concat(_t306, _t307); }), (long)(intptr_t)")");
  }

  u_checkCap(recv, name, ctx);

  want = u_methodArgs(name);

  if ((want < 0L)) {

  u_failAt(s_concat((long)(intptr_t)"no method named ", name), ctx);
  }

  if ((!(l_len(args) == want))) {

  u_failAt(({ long _t316 = s_concat(({ long _t314 = s_concat(name, (long)(intptr_t)" takes "); long _t315 = i_tostr(want); s_concat(_t314, _t315); }), (long)(intptr_t)" values, got "); long _t317 = i_tostr(l_len(args)); s_concat(_t316, _t317); }), ctx);
  }

  if (s_eq(name, (long)(intptr_t)"print")) {

  if (s_eq(u_exprType(l_atc(args, 0L, 4664), ctx), (long)(intptr_t)"Bytes")) {

  u_failAt((long)(intptr_t)"Bytes cannot be printed; print a part with textAt(i, n) or a number with at(i)", ctx);
  }

  u_emitExpr(recv, ctx);

  return s_concat(s_concat((long)(intptr_t)"(", u_emitPrint(l_atc(args, 0L, 4668), ctx)), (long)(intptr_t)", 0L)");
  }

  u_checkMethod(t, name, args, ctx);

  recvC = u_emitExpr(recv, ctx);

  ln = u_envGet(((R_Ctx*)(intptr_t)ctx)->env, (long)(intptr_t)"__line__");

  r = (long)(intptr_t)"0";

  if (s_eq(name, (long)(intptr_t)"concat")) {

  r = s_concat(({ long _t329 = s_concat(s_concat((long)(intptr_t)"s_concat(", recvC), (long)(intptr_t)", "); long _t330 = u_emitExpr(l_atc(args, 0L, 4675), ctx); s_concat(_t329, _t330); }), (long)(intptr_t)")");
  }

  if (s_eq(name, (long)(intptr_t)"slice")) {

  r = s_concat(s_concat(s_concat(({ long _t335 = s_concat(({ long _t333 = s_concat(s_concat((long)(intptr_t)"s_slicec(", recvC), (long)(intptr_t)", "); long _t334 = u_emitExpr(l_atc(args, 0L, 4678), ctx); s_concat(_t333, _t334); }), (long)(intptr_t)", "); long _t336 = u_emitExpr(l_atc(args, 1L, 4678), ctx); s_concat(_t335, _t336); }), (long)(intptr_t)", "), ln), (long)(intptr_t)")");
  }

  if (s_eq(name, (long)(intptr_t)"at")) {

  r = s_concat(s_concat(s_concat(({ long _t339 = s_concat(s_concat((long)(intptr_t)"s_atc(", recvC), (long)(intptr_t)", "); long _t340 = u_emitExpr(l_atc(args, 0L, 4681), ctx); s_concat(_t339, _t340); }), (long)(intptr_t)", "), ln), (long)(intptr_t)")");

  if (s_eq(t, (long)(intptr_t)"Bytes")) {

  r = s_concat(s_concat(s_concat(({ long _t343 = s_concat(s_concat((long)(intptr_t)"y_atc(", recvC), (long)(intptr_t)", "); long _t344 = u_emitExpr(l_atc(args, 0L, 4683), ctx); s_concat(_t343, _t344); }), (long)(intptr_t)", "), ln), (long)(intptr_t)")");
  }

  if (u_isListType(t)) {

  r = s_concat(s_concat(s_concat(({ long _t347 = s_concat(s_concat((long)(intptr_t)"l_atc(", recvC), (long)(intptr_t)", "); long _t348 = u_emitExpr(l_atc(args, 0L, 4686), ctx); s_concat(_t347, _t348); }), (long)(intptr_t)", "), ln), (long)(intptr_t)")");
  }
  }

  if (s_eq(name, (long)(intptr_t)"push")) {

  r = s_concat(({ long _t351 = s_concat(s_concat((long)(intptr_t)"l_push(", recvC), (long)(intptr_t)", "); long _t352 = u_emitExpr(l_atc(args, 0L, 4690), ctx); s_concat(_t351, _t352); }), (long)(intptr_t)")");
  }

  if ((((s_eq(name, (long)(intptr_t)"run") || s_eq(name, (long)(intptr_t)"status")) || s_eq(name, (long)(intptr_t)"start")) || s_eq(name, (long)(intptr_t)"try"))) {

  u_runCheck(l_atc(args, 0L, 4693), ctx);

  u_wantType(l_atc(args, 0L, 4694), (long)(intptr_t)"Str", name, ctx);

  u_wantType(l_atc(args, 1L, 4695), (long)(intptr_t)"List[Str]", name, ctx);

  mid = (long)(intptr_t)", ";

  if (s_eq(name, (long)(intptr_t)"run")) {

  mid = (long)(intptr_t)", 0, ";
  }

  r = s_concat(s_concat(s_concat(({ long _t363 = s_concat(({ long _t361 = s_concat(s_concat((long)(intptr_t)"simpler_", name), (long)(intptr_t)"("); long _t362 = u_emitExpr(l_atc(args, 0L, 4700), ctx); s_concat(_t361, _t362); }), (long)(intptr_t)", "); long _t364 = u_emitExpr(l_atc(args, 1L, 4700), ctx); s_concat(_t363, _t364); }), mid), ln), (long)(intptr_t)")");
  }

  if (s_eq(name, (long)(intptr_t)"feed")) {

  u_runCheck(l_atc(args, 0L, 4703), ctx);

  u_wantType(l_atc(args, 0L, 4704), (long)(intptr_t)"Str", name, ctx);

  u_wantType(l_atc(args, 1L, 4705), (long)(intptr_t)"List[Str]", name, ctx);

  u_wantType(l_atc(args, 2L, 4706), (long)(intptr_t)"Str", name, ctx);

  r = s_concat(s_concat(s_concat(({ long _t373 = s_concat(({ long _t371 = s_concat(s_concat((long)(intptr_t)"simpler_run(", u_emitExpr(l_atc(args, 0L, 4707), ctx)), (long)(intptr_t)", "); long _t372 = u_emitExpr(l_atc(args, 1L, 4707), ctx); s_concat(_t371, _t372); }), (long)(intptr_t)", "); long _t374 = u_emitExpr(l_atc(args, 2L, 4707), ctx); s_concat(_t373, _t374); }), (long)(intptr_t)", "), ln), (long)(intptr_t)")");
  }

  if (s_eq(name, (long)(intptr_t)"reach")) {

  u_fetchCheck(l_atc(args, 0L, 4710), ctx);

  u_wantType(l_atc(args, 0L, 4711), (long)(intptr_t)"Str", name, ctx);

  r = s_concat(s_concat(s_concat(s_concat((long)(intptr_t)"simpler_reach(", u_emitExpr(l_atc(args, 0L, 4712), ctx)), (long)(intptr_t)", "), ln), (long)(intptr_t)")");
  }

  if (s_eq(name, (long)(intptr_t)"send")) {

  u_mailCheck(l_atc(args, 0L, 4715), ctx);

  u_wantType(l_atc(args, 0L, 4716), (long)(intptr_t)"Str", name, ctx);

  u_wantType(l_atc(args, 1L, 4717), (long)(intptr_t)"Str", name, ctx);

  u_wantType(l_atc(args, 2L, 4718), (long)(intptr_t)"Str", name, ctx);

  r = s_concat(s_concat(s_concat(({ long _t387 = s_concat(({ long _t385 = s_concat(s_concat((long)(intptr_t)"simpler_send(", u_emitExpr(l_atc(args, 0L, 4719), ctx)), (long)(intptr_t)", "); long _t386 = u_emitExpr(l_atc(args, 1L, 4719), ctx); s_concat(_t385, _t386); }), (long)(intptr_t)", "); long _t388 = u_emitExpr(l_atc(args, 2L, 4719), ctx); s_concat(_t387, _t388); }), (long)(intptr_t)", "), ln), (long)(intptr_t)")");
  }

  if (s_eq(name, (long)(intptr_t)"fetch")) {

  u_fetchCheck(l_atc(args, 0L, 4722), ctx);

  u_wantType(l_atc(args, 0L, 4723), (long)(intptr_t)"Str", name, ctx);

  r = s_concat(s_concat(s_concat(s_concat((long)(intptr_t)"simpler_fetch(", u_emitExpr(l_atc(args, 0L, 4724), ctx)), (long)(intptr_t)", 0, 0, "), ln), (long)(intptr_t)")");
  }

  if (s_eq(name, (long)(intptr_t)"post")) {

  u_fetchCheck(l_atc(args, 0L, 4727), ctx);

  u_wantType(l_atc(args, 0L, 4728), (long)(intptr_t)"Str", name, ctx);

  u_wantType(l_atc(args, 1L, 4729), (long)(intptr_t)"Str", name, ctx);

  r = s_concat(s_concat(s_concat(({ long _t398 = s_concat(s_concat((long)(intptr_t)"simpler_fetch(", u_emitExpr(l_atc(args, 0L, 4730), ctx)), (long)(intptr_t)", "); long _t399 = u_emitExpr(l_atc(args, 1L, 4730), ctx); s_concat(_t398, _t399); }), (long)(intptr_t)", 1, "), ln), (long)(intptr_t)")");
  }

  if (s_eq(name, (long)(intptr_t)"date")) {

  u_wantType(l_atc(args, 0L, 4733), (long)(intptr_t)"Int", name, ctx);

  r = s_concat(s_concat((long)(intptr_t)"simpler_date(", u_emitExpr(l_atc(args, 0L, 4734), ctx)), (long)(intptr_t)")");
  }

  if (s_eq(name, (long)(intptr_t)"dateAs")) {

  u_wantType(l_atc(args, 0L, 4737), (long)(intptr_t)"Int", name, ctx);

  u_wantType(l_atc(args, 1L, 4738), (long)(intptr_t)"Str", name, ctx);

  r = s_concat(({ long _t407 = s_concat(s_concat((long)(intptr_t)"simpler_stamp(", u_emitExpr(l_atc(args, 0L, 4739), ctx)), (long)(intptr_t)", "); long _t408 = u_emitExpr(l_atc(args, 1L, 4739), ctx); s_concat(_t407, _t408); }), (long)(intptr_t)")");
  }

  if (s_eq(name, (long)(intptr_t)"secsOf")) {

  u_wantType(l_atc(args, 0L, 4742), (long)(intptr_t)"Str", name, ctx);

  r = s_concat(s_concat((long)(intptr_t)"simpler_secs(", u_emitExpr(l_atc(args, 0L, 4743), ctx)), (long)(intptr_t)")");
  }

  if (s_eq(name, (long)(intptr_t)"error")) {

  u_wantType(l_atc(args, 0L, 4746), (long)(intptr_t)"Str", name, ctx);

  r = s_concat(s_concat((long)(intptr_t)"(fflush(stdout), fputs((const char*)(intptr_t)", u_emitExpr(l_atc(args, 0L, 4747), ctx)), (long)(intptr_t)", stderr), fputc(10, stderr), 0L)");
  }

  if (s_eq(name, (long)(intptr_t)"part")) {

  u_wantType(l_atc(args, 0L, 4750), (long)(intptr_t)"Str", name, ctx);

  r = s_concat(s_concat((long)(intptr_t)"(fputs((const char*)(intptr_t)", u_emitExpr(l_atc(args, 0L, 4751), ctx)), (long)(intptr_t)", stdout), 0L)");
  }

  if (s_eq(name, (long)(intptr_t)"fetchWith")) {

  u_fetchCheck(l_atc(args, 0L, 4754), ctx);

  u_wantType(l_atc(args, 0L, 4755), (long)(intptr_t)"Str", name, ctx);

  u_wantType(l_atc(args, 1L, 4756), (long)(intptr_t)"List[Str]", name, ctx);

  r = s_concat(s_concat(s_concat(({ long _t423 = s_concat(s_concat((long)(intptr_t)"simpler_fetchwith(", u_emitExpr(l_atc(args, 0L, 4757), ctx)), (long)(intptr_t)", "); long _t424 = u_emitExpr(l_atc(args, 1L, 4757), ctx); s_concat(_t423, _t424); }), (long)(intptr_t)", "), ln), (long)(intptr_t)")");
  }

  if (s_eq(name, (long)(intptr_t)"var")) {

  u_wantType(l_atc(args, 0L, 4760), (long)(intptr_t)"Str", name, ctx);

  r = s_concat(s_concat((long)(intptr_t)"simpler_env(", u_emitExpr(l_atc(args, 0L, 4761), ctx)), (long)(intptr_t)")");
  }

  if (s_eq(name, (long)(intptr_t)"setVar")) {

  u_wantType(l_atc(args, 0L, 4764), (long)(intptr_t)"Str", name, ctx);

  u_wantType(l_atc(args, 1L, 4765), (long)(intptr_t)"Str", name, ctx);

  r = s_concat(({ long _t432 = s_concat(s_concat((long)(intptr_t)"simpler_setenv(", u_emitExpr(l_atc(args, 0L, 4766), ctx)), (long)(intptr_t)", "); long _t433 = u_emitExpr(l_atc(args, 1L, 4766), ctx); s_concat(_t432, _t433); }), (long)(intptr_t)")");
  }

  if (s_eq(name, (long)(intptr_t)"read")) {

  ({ long _t436 = l_atc(args, 0L, 4769); long _t437 = ((R_Sigs*)(intptr_t)((R_Ctx*)(intptr_t)ctx)->sigs)->reads; u_mayCheck(_t436, _t437, (long)(intptr_t)"read", (long)(intptr_t)"reads", ctx); });

  r = s_concat(s_concat(s_concat(s_concat((long)(intptr_t)"(long)(intptr_t)simpler_read((const char*)(intptr_t)simpler_may(", u_emitExpr(l_atc(args, 0L, 4770), ctx)), (long)(intptr_t)", simpler_rd, "), ln), (long)(intptr_t)", \"read\"))");
  }

  if (s_eq(name, (long)(intptr_t)"readBytes")) {

  ({ long _t441 = l_atc(args, 0L, 4773); long _t442 = ((R_Sigs*)(intptr_t)((R_Ctx*)(intptr_t)ctx)->sigs)->reads; u_mayCheck(_t441, _t442, (long)(intptr_t)"read", (long)(intptr_t)"reads", ctx); });

  r = s_concat(s_concat(s_concat(s_concat((long)(intptr_t)"simpler_readbytes((const char*)(intptr_t)simpler_may(", u_emitExpr(l_atc(args, 0L, 4774), ctx)), (long)(intptr_t)", simpler_rd, "), ln), (long)(intptr_t)", \"read\"))");
  }

  if (s_eq(name, (long)(intptr_t)"writeBytes")) {

  ({ long _t446 = l_atc(args, 0L, 4777); long _t447 = ((R_Sigs*)(intptr_t)((R_Ctx*)(intptr_t)ctx)->sigs)->writes; u_mayCheck(_t446, _t447, (long)(intptr_t)"write", (long)(intptr_t)"writes", ctx); });

  r = s_concat(s_concat(s_concat(({ long _t449 = s_concat(s_concat(s_concat(s_concat((long)(intptr_t)"simpler_writebytes(simpler_may(", u_emitExpr(l_atc(args, 0L, 4778), ctx)), (long)(intptr_t)", simpler_wr, "), ln), (long)(intptr_t)", \"write\"), "); long _t450 = u_emitExpr(l_atc(args, 1L, 4778), ctx); s_concat(_t449, _t450); }), (long)(intptr_t)", "), ln), (long)(intptr_t)")");
  }

  if (s_eq(name, (long)(intptr_t)"findByte")) {

  r = s_concat(({ long _t455 = s_concat(({ long _t453 = s_concat(s_concat((long)(intptr_t)"y_find(", recvC), (long)(intptr_t)", "); long _t454 = u_emitExpr(l_atc(args, 0L, 4781), ctx); s_concat(_t453, _t454); }), (long)(intptr_t)", "); long _t456 = u_emitExpr(l_atc(args, 1L, 4781), ctx); s_concat(_t455, _t456); }), (long)(intptr_t)")");
  }

  if (s_eq(name, (long)(intptr_t)"textAt")) {

  r = s_concat(s_concat(s_concat(({ long _t461 = s_concat(({ long _t459 = s_concat(s_concat((long)(intptr_t)"b_text(", recvC), (long)(intptr_t)", "); long _t460 = u_emitExpr(l_atc(args, 0L, 4784), ctx); s_concat(_t459, _t460); }), (long)(intptr_t)", "); long _t462 = u_emitExpr(l_atc(args, 1L, 4784), ctx); s_concat(_t461, _t462); }), (long)(intptr_t)", "), ln), (long)(intptr_t)")");

  if (s_eq(t, (long)(intptr_t)"Bytes")) {

  r = s_concat(s_concat(s_concat(({ long _t467 = s_concat(({ long _t465 = s_concat(s_concat((long)(intptr_t)"y_text(", recvC), (long)(intptr_t)", "); long _t466 = u_emitExpr(l_atc(args, 0L, 4786), ctx); s_concat(_t465, _t466); }), (long)(intptr_t)", "); long _t468 = u_emitExpr(l_atc(args, 1L, 4786), ctx); s_concat(_t467, _t468); }), (long)(intptr_t)", "), ln), (long)(intptr_t)")");
  }
  }

  if (s_eq(name, (long)(intptr_t)"intAt")) {

  r = s_concat(s_concat(s_concat(({ long _t473 = s_concat(({ long _t471 = s_concat(s_concat((long)(intptr_t)"b_int(", recvC), (long)(intptr_t)", "); long _t472 = u_emitExpr(l_atc(args, 0L, 4790), ctx); s_concat(_t471, _t472); }), (long)(intptr_t)", "); long _t474 = u_emitExpr(l_atc(args, 1L, 4790), ctx); s_concat(_t473, _t474); }), (long)(intptr_t)", "), ln), (long)(intptr_t)")");

  if (s_eq(t, (long)(intptr_t)"Bytes")) {

  r = s_concat(s_concat(s_concat(({ long _t479 = s_concat(({ long _t477 = s_concat(s_concat((long)(intptr_t)"y_int(", recvC), (long)(intptr_t)", "); long _t478 = u_emitExpr(l_atc(args, 0L, 4792), ctx); s_concat(_t477, _t478); }), (long)(intptr_t)", "); long _t480 = u_emitExpr(l_atc(args, 1L, 4792), ctx); s_concat(_t479, _t480); }), (long)(intptr_t)", "), ln), (long)(intptr_t)")");
  }
  }

  if ((((s_eq(name, (long)(intptr_t)"atan2") || s_eq(name, (long)(intptr_t)"pow")) || s_eq(name, (long)(intptr_t)"atan2_c")) || s_eq(name, (long)(intptr_t)"pow_c"))) {

  if (((!s_eq(t, (long)(intptr_t)"Float")) || (!s_eq(u_exprType(l_atc(args, 0L, 4796), ctx), (long)(intptr_t)"Float")))) {

  u_failAt((long)(intptr_t)"atan2 and pow work on two Float numbers: y.atan2(x), x.pow(y)", ctx);
  }

  cf = (long)(intptr_t)"pow";

  if (s_starts(name, (long)(intptr_t)"atan2")) {

  cf = (long)(intptr_t)"atan2";
  }

  r = s_concat(({ long _t488 = s_concat(s_concat(s_concat(s_concat((long)(intptr_t)"d2l(", cf), (long)(intptr_t)"(l2d("), recvC), (long)(intptr_t)"), l2d("); long _t489 = u_emitExpr(l_atc(args, 0L, 4803), ctx); s_concat(_t488, _t489); }), (long)(intptr_t)")))");
  }

  if (s_eq(name, (long)(intptr_t)"floatAt")) {

  r = s_concat(s_concat(s_concat(({ long _t492 = s_concat(s_concat((long)(intptr_t)"b_float(", recvC), (long)(intptr_t)", "); long _t493 = u_emitExpr(l_atc(args, 0L, 4806), ctx); s_concat(_t492, _t493); }), (long)(intptr_t)", "), ln), (long)(intptr_t)")");

  if (s_eq(t, (long)(intptr_t)"Bytes")) {

  r = s_concat(s_concat(s_concat(({ long _t496 = s_concat(s_concat((long)(intptr_t)"y_float(", recvC), (long)(intptr_t)", "); long _t497 = u_emitExpr(l_atc(args, 0L, 4808), ctx); s_concat(_t496, _t497); }), (long)(intptr_t)", "), ln), (long)(intptr_t)")");
  }
  }

  if (s_eq(name, (long)(intptr_t)"write")) {

  ({ long _t500 = l_atc(args, 0L, 4812); long _t501 = ((R_Sigs*)(intptr_t)((R_Ctx*)(intptr_t)ctx)->sigs)->writes; u_mayCheck(_t500, _t501, (long)(intptr_t)"write", (long)(intptr_t)"writes", ctx); });

  r = s_concat(({ long _t503 = s_concat(s_concat(s_concat(s_concat((long)(intptr_t)"simpler_write(simpler_may(", u_emitExpr(l_atc(args, 0L, 4813), ctx)), (long)(intptr_t)", simpler_wr, "), ln), (long)(intptr_t)", \"write\"), "); long _t504 = u_emitExpr(l_atc(args, 1L, 4813), ctx); s_concat(_t503, _t504); }), (long)(intptr_t)")");
  }

  if (s_eq(name, (long)(intptr_t)"append")) {

  ({ long _t507 = l_atc(args, 0L, 4816); long _t508 = ((R_Sigs*)(intptr_t)((R_Ctx*)(intptr_t)ctx)->sigs)->writes; u_mayCheck(_t507, _t508, (long)(intptr_t)"write", (long)(intptr_t)"writes", ctx); });

  r = s_concat(({ long _t510 = s_concat(s_concat(s_concat(s_concat((long)(intptr_t)"simpler_append(simpler_may(", u_emitExpr(l_atc(args, 0L, 4817), ctx)), (long)(intptr_t)", simpler_wr, "), ln), (long)(intptr_t)", \"write\"), "); long _t511 = u_emitExpr(l_atc(args, 1L, 4817), ctx); s_concat(_t510, _t511); }), (long)(intptr_t)")");
  }

  if (s_eq(name, (long)(intptr_t)"list")) {

  ({ long _t514 = l_atc(args, 0L, 4820); long _t515 = ((R_Sigs*)(intptr_t)((R_Ctx*)(intptr_t)ctx)->sigs)->reads; u_mayCheck(_t514, _t515, (long)(intptr_t)"list", (long)(intptr_t)"reads", ctx); });

  r = s_concat(s_concat(s_concat(s_concat((long)(intptr_t)"simpler_list(simpler_may(simpler_dir(", u_emitExpr(l_atc(args, 0L, 4821), ctx)), (long)(intptr_t)"), simpler_rd, "), ln), (long)(intptr_t)", \"list\"))");
  }

  if (s_eq(name, (long)(intptr_t)"exists")) {

  ({ long _t519 = l_atc(args, 0L, 4824); long _t520 = ((R_Sigs*)(intptr_t)((R_Ctx*)(intptr_t)ctx)->sigs)->reads; u_mayCheck(_t519, _t520, (long)(intptr_t)"read", (long)(intptr_t)"reads", ctx); });

  r = s_concat(s_concat(s_concat(s_concat((long)(intptr_t)"simpler_exists(simpler_may(", u_emitExpr(l_atc(args, 0L, 4825), ctx)), (long)(intptr_t)", simpler_rd, "), ln), (long)(intptr_t)", \"read\"))");
  }

  if (s_eq(name, (long)(intptr_t)"rename")) {

  ({ long _t524 = l_atc(args, 0L, 4828); long _t525 = ((R_Sigs*)(intptr_t)((R_Ctx*)(intptr_t)ctx)->sigs)->writes; u_mayCheck(_t524, _t525, (long)(intptr_t)"write", (long)(intptr_t)"writes", ctx); });

  ({ long _t527 = l_atc(args, 1L, 4829); long _t528 = ((R_Sigs*)(intptr_t)((R_Ctx*)(intptr_t)ctx)->sigs)->writes; u_mayCheck(_t527, _t528, (long)(intptr_t)"write", (long)(intptr_t)"writes", ctx); });

  r = s_concat(s_concat(s_concat(s_concat(s_concat(({ long _t530 = s_concat(s_concat(s_concat(s_concat((long)(intptr_t)"simpler_rename(simpler_may(", u_emitExpr(l_atc(args, 0L, 4830), ctx)), (long)(intptr_t)", simpler_wr, "), ln), (long)(intptr_t)", \"write\"), simpler_may("); long _t531 = u_emitExpr(l_atc(args, 1L, 4830), ctx); s_concat(_t530, _t531); }), (long)(intptr_t)", simpler_wr, "), ln), (long)(intptr_t)", \"write\"), "), ln), (long)(intptr_t)")");
  }

  if (s_eq(name, (long)(intptr_t)"delete")) {

  ({ long _t534 = l_atc(args, 0L, 4833); long _t535 = ((R_Sigs*)(intptr_t)((R_Ctx*)(intptr_t)ctx)->sigs)->writes; u_mayCheck(_t534, _t535, (long)(intptr_t)"write", (long)(intptr_t)"writes", ctx); });

  r = s_concat(s_concat(s_concat(s_concat(s_concat(s_concat((long)(intptr_t)"simpler_delete(simpler_may(", u_emitExpr(l_atc(args, 0L, 4834), ctx)), (long)(intptr_t)", simpler_wr, "), ln), (long)(intptr_t)", \"write\"), "), ln), (long)(intptr_t)")");
  }

  if (s_eq(name, (long)(intptr_t)"split")) {

  r = s_concat(({ long _t539 = s_concat(s_concat((long)(intptr_t)"s_split(", recvC), (long)(intptr_t)", "); long _t540 = u_emitExpr(l_atc(args, 0L, 4837), ctx); s_concat(_t539, _t540); }), (long)(intptr_t)")");
  }

  if (s_eq(name, (long)(intptr_t)"contains")) {

  r = s_concat(({ long _t543 = s_concat(s_concat((long)(intptr_t)"s_contains(", recvC), (long)(intptr_t)", "); long _t544 = u_emitExpr(l_atc(args, 0L, 4840), ctx); s_concat(_t543, _t544); }), (long)(intptr_t)")");
  }

  if (s_eq(name, (long)(intptr_t)"find")) {

  r = s_concat(({ long _t547 = s_concat(s_concat((long)(intptr_t)"s_find(", recvC), (long)(intptr_t)", "); long _t548 = u_emitExpr(l_atc(args, 0L, 4843), ctx); s_concat(_t547, _t548); }), (long)(intptr_t)")");
  }

  if ((((s_eq(name, (long)(intptr_t)"reTest") || s_eq(name, (long)(intptr_t)"reFind")) || s_eq(name, (long)(intptr_t)"reAll")) || s_eq(name, (long)(intptr_t)"reSplit"))) {

  r = s_concat(s_concat(s_concat(({ long _t551 = s_concat(s_concat(s_concat(s_concat((long)(intptr_t)"re_", s_case(s_slicec(name, 2L, s_len(name), 4846), 0)), (long)(intptr_t)"("), recvC), (long)(intptr_t)", "); long _t552 = u_emitExpr(l_atc(args, 0L, 4846), ctx); s_concat(_t551, _t552); }), (long)(intptr_t)", "), ln), (long)(intptr_t)")");
  }

  if (s_eq(name, (long)(intptr_t)"reReplace")) {

  r = s_concat(s_concat(s_concat(({ long _t557 = s_concat(({ long _t555 = s_concat(s_concat((long)(intptr_t)"re_replace(", recvC), (long)(intptr_t)", "); long _t556 = u_emitExpr(l_atc(args, 0L, 4849), ctx); s_concat(_t555, _t556); }), (long)(intptr_t)", "); long _t558 = u_emitExpr(l_atc(args, 1L, 4849), ctx); s_concat(_t557, _t558); }), (long)(intptr_t)", "), ln), (long)(intptr_t)")");
  }

  if (s_eq(name, (long)(intptr_t)"json")) {

  r = s_concat(({ long _t561 = s_concat(s_concat((long)(intptr_t)"s_json(", recvC), (long)(intptr_t)", "); long _t562 = u_emitExpr(l_atc(args, 0L, 4852), ctx); s_concat(_t561, _t562); }), (long)(intptr_t)")");
  }

  if (s_eq(name, (long)(intptr_t)"join")) {

  r = s_concat(({ long _t565 = s_concat(s_concat((long)(intptr_t)"l_join(", recvC), (long)(intptr_t)", "); long _t566 = u_emitExpr(l_atc(args, 0L, 4855), ctx); s_concat(_t565, _t566); }), (long)(intptr_t)")");
  }

  if (s_eq(name, (long)(intptr_t)"startsWith")) {

  r = s_concat(({ long _t569 = s_concat(s_concat((long)(intptr_t)"s_starts(", recvC), (long)(intptr_t)", "); long _t570 = u_emitExpr(l_atc(args, 0L, 4858), ctx); s_concat(_t569, _t570); }), (long)(intptr_t)")");
  }

  if (s_eq(name, (long)(intptr_t)"endsWith")) {

  r = s_concat(({ long _t573 = s_concat(s_concat((long)(intptr_t)"s_ends(", recvC), (long)(intptr_t)", "); long _t574 = u_emitExpr(l_atc(args, 0L, 4861), ctx); s_concat(_t573, _t574); }), (long)(intptr_t)")");
  }

  if (s_eq(name, (long)(intptr_t)"replace")) {

  r = s_concat(({ long _t579 = s_concat(({ long _t577 = s_concat(s_concat((long)(intptr_t)"s_replace(", recvC), (long)(intptr_t)", "); long _t578 = u_emitExpr(l_atc(args, 0L, 4864), ctx); s_concat(_t577, _t578); }), (long)(intptr_t)", "); long _t580 = u_emitExpr(l_atc(args, 1L, 4864), ctx); s_concat(_t579, _t580); }), (long)(intptr_t)")");
  }

  if (s_eq(name, (long)(intptr_t)"mod")) {

  r = s_concat(s_concat(s_concat(({ long _t583 = s_concat(s_concat((long)(intptr_t)"i_mod(", recvC), (long)(intptr_t)", "); long _t584 = u_emitExpr(l_atc(args, 0L, 4867), ctx); s_concat(_t583, _t584); }), (long)(intptr_t)", "), ln), (long)(intptr_t)")");
  }

  if (s_eq(name, (long)(intptr_t)"atan2")) {

  r = s_concat(({ long _t587 = s_concat(s_concat((long)(intptr_t)"d2l(atan2(l2d(", recvC), (long)(intptr_t)"), l2d("); long _t588 = u_emitExpr(l_atc(args, 0L, 4870), ctx); s_concat(_t587, _t588); }), (long)(intptr_t)")))");
  }

  if (s_eq(name, (long)(intptr_t)"padLeft")) {

  r = s_concat(({ long _t591 = s_concat(s_concat((long)(intptr_t)"s_padleft(", recvC), (long)(intptr_t)", "); long _t592 = u_emitExpr(l_atc(args, 0L, 4873), ctx); s_concat(_t591, _t592); }), (long)(intptr_t)")");
  }

  if (s_eq(name, (long)(intptr_t)"padRight")) {

  r = s_concat(({ long _t595 = s_concat(s_concat((long)(intptr_t)"s_padright(", recvC), (long)(intptr_t)", "); long _t596 = u_emitExpr(l_atc(args, 0L, 4876), ctx); s_concat(_t595, _t596); }), (long)(intptr_t)")");
  }

  if (s_eq(name, (long)(intptr_t)"set")) {

  r = s_concat(({ long _t601 = s_concat(({ long _t599 = s_concat(s_concat((long)(intptr_t)"m_set(", recvC), (long)(intptr_t)", "); long _t600 = u_emitExpr(l_atc(args, 0L, 4879), ctx); s_concat(_t599, _t600); }), (long)(intptr_t)", "); long _t602 = u_emitExpr(l_atc(args, 1L, 4879), ctx); s_concat(_t601, _t602); }), (long)(intptr_t)")");

  if (u_isListType(t)) {

  r = s_concat(s_concat(s_concat(({ long _t607 = s_concat(({ long _t605 = s_concat(s_concat((long)(intptr_t)"l_setc(", recvC), (long)(intptr_t)", "); long _t606 = u_emitExpr(l_atc(args, 0L, 4881), ctx); s_concat(_t605, _t606); }), (long)(intptr_t)", "); long _t608 = u_emitExpr(l_atc(args, 1L, 4881), ctx); s_concat(_t607, _t608); }), (long)(intptr_t)", "), ln), (long)(intptr_t)")");
  }
  }

  if (s_eq(name, (long)(intptr_t)"remove")) {

  r = s_concat(s_concat(s_concat(({ long _t611 = s_concat(s_concat((long)(intptr_t)"l_removec(", recvC), (long)(intptr_t)", "); long _t612 = u_emitExpr(l_atc(args, 0L, 4885), ctx); s_concat(_t611, _t612); }), (long)(intptr_t)", "), ln), (long)(intptr_t)")");
  }

  if (s_eq(name, (long)(intptr_t)"get")) {

  r = s_concat(({ long _t615 = s_concat(s_concat((long)(intptr_t)"m_get(", recvC), (long)(intptr_t)", "); long _t616 = u_emitExpr(l_atc(args, 0L, 4888), ctx); s_concat(_t615, _t616); }), (long)(intptr_t)")");

  mv = u_mapValueOf(t);

  if (s_eq(mv, (long)(intptr_t)"Str")) {

  r = s_concat(({ long _t620 = s_concat(s_concat((long)(intptr_t)"m_gets(", recvC), (long)(intptr_t)", "); long _t621 = u_emitExpr(l_atc(args, 0L, 4892), ctx); s_concat(_t620, _t621); }), (long)(intptr_t)")");
  }

  if ((u_isMapType(t) && (!u_plainType(mv)))) {

  r = s_concat(s_concat(s_concat(({ long _t624 = s_concat(s_concat((long)(intptr_t)"m_getp(", recvC), (long)(intptr_t)", "); long _t625 = u_emitExpr(l_atc(args, 0L, 4896), ctx); s_concat(_t624, _t625); }), (long)(intptr_t)", "), ln), (long)(intptr_t)")");
  }
  }

  if (s_eq(name, (long)(intptr_t)"has")) {

  r = s_concat(({ long _t628 = s_concat(s_concat((long)(intptr_t)"m_has(", recvC), (long)(intptr_t)", "); long _t629 = u_emitExpr(l_atc(args, 0L, 4900), ctx); s_concat(_t628, _t629); }), (long)(intptr_t)")");
  }

  if (s_eq(name, (long)(intptr_t)"ge")) {

  r = s_concat(({ long _t632 = s_concat(s_concat((long)(intptr_t)"(", recvC), (long)(intptr_t)" >= "); long _t633 = u_emitExpr(l_atc(args, 0L, 4903), ctx); s_concat(_t632, _t633); }), (long)(intptr_t)")");
  }

  if (s_eq(name, (long)(intptr_t)"le")) {

  r = s_concat(({ long _t636 = s_concat(s_concat((long)(intptr_t)"(", recvC), (long)(intptr_t)" <= "); long _t637 = u_emitExpr(l_atc(args, 0L, 4906), ctx); s_concat(_t636, _t637); }), (long)(intptr_t)")");
  }

  if (s_eq(name, (long)(intptr_t)"and")) {

  r = s_concat(({ long _t640 = s_concat(s_concat((long)(intptr_t)"(", recvC), (long)(intptr_t)" && "); long _t641 = u_emitExpr(l_atc(args, 0L, 4909), ctx); s_concat(_t640, _t641); }), (long)(intptr_t)")");
  }

  if (s_eq(name, (long)(intptr_t)"or")) {

  r = s_concat(({ long _t644 = s_concat(s_concat((long)(intptr_t)"(", recvC), (long)(intptr_t)" || "); long _t645 = u_emitExpr(l_atc(args, 0L, 4912), ctx); s_concat(_t644, _t645); }), (long)(intptr_t)")");
  }

  return r;
  return 0;
}
long u_cEscape(long s) {
  long out = 0;
  long k = 0;
  long n = 0;
  long c = 0;
  long piece = 0;

  out = (long)(intptr_t)"";

  k = 0L;

  n = s_len(s);

  while ((k < n)) {

  c = s_code(s_atc(s, k, 4924));

  piece = s_atc(s, k, 4925);

  if ((c == 34L)) {

  piece = (long)(intptr_t)"\\\"";
  }

  if ((c == 92L)) {

  piece = (long)(intptr_t)"\\\\";
  }

  if ((c == 10L)) {

  piece = (long)(intptr_t)"\\n";
  }

  if ((c == 9L)) {

  piece = (long)(intptr_t)"\\t";
  }

  if ((c == 13L)) {

  piece = (long)(intptr_t)"\\r";
  }

  out = s_concat(out, piece);

  k = (k + 1L);
  }

  return out;
  return 0;
}
long u_caseArity(long name, long ctx) {
  long r = 0;
  long a = 0;

  r = (0L - 1L);

  {
  long _lst = ((R_Sigs*)(intptr_t)((R_Ctx*)(intptr_t)ctx)->sigs)->types;
  long _n = l_len(_lst);
  for (long _i = 0; _i < _n; _i = _i + 1) {
  long t = l_at(_lst, _i);

  a = u_caseArityIn(t, name);

  if ((a > r)) {

  r = a;
  }
  }
  }

  return r;
  return 0;
}
long u_emitVar(long s, long ctx) {
  long vt = 0;
  long ar = 0;
  long r = 0;

  vt = u_variantOf(s, ctx);

  if (s_eq(vt, (long)(intptr_t)"")) {

  if (((s_eq(u_envOld(((R_Ctx*)(intptr_t)ctx)->env, s), (long)(intptr_t)"__gone__") && (!s_eq(s, (long)(intptr_t)"true"))) && (!s_eq(s, (long)(intptr_t)"false")))) {

  u_failAt(s_concat((long)(intptr_t)"no variable named ", s), ctx);
  }
  } else {

  ar = u_caseArity(s, ctx);

  if ((ar > 0L)) {

  u_failAt(s_concat(({ long _t31 = s_concat(s_concat((long)(intptr_t)"case ", s), (long)(intptr_t)" takes "); long _t32 = i_tostr(ar); s_concat(_t31, _t32); }), (long)(intptr_t)" values, got 0"), ctx);
  }
  }

  r = s;

  if (u_hasName(((R_Sigs*)(intptr_t)((R_Ctx*)(intptr_t)ctx)->sigs)->boxedNullary, s)) {

  r = s_concat(s, (long)(intptr_t)"()");
  }

  if ((s_eq(vt, (long)(intptr_t)"") && s_eq(u_envGet(((R_Ctx*)(intptr_t)ctx)->env, s), (long)(intptr_t)"Sys"))) {

  r = (long)(intptr_t)"0";
  }

  return r;
  return 0;
}
long u_emitArgs(long args, long ctx) {
  long out = 0;
  long k = 0;
  long m = 0;

  out = (long)(intptr_t)"";

  k = 0L;

  m = l_len(args);

  while ((k < m)) {

  if ((k > 0L)) {

  out = s_concat(out, (long)(intptr_t)", ");
  }

  out = s_concat(out, u_emitExpr(l_atc(args, k, 4991), ctx));

  k = (k + 1L);
  }

  return out;
  return 0;
}
long u_binType(long op, long a, long b, long ctx) {
  long r = 0;
  long ta = 0;

  r = (long)(intptr_t)"Int";

  if (((((((s_eq(op, (long)(intptr_t)"==") || s_eq(op, (long)(intptr_t)"<")) || s_eq(op, (long)(intptr_t)">")) || s_eq(op, (long)(intptr_t)"<=")) || s_eq(op, (long)(intptr_t)">=")) || s_eq(op, (long)(intptr_t)"&&")) || s_eq(op, (long)(intptr_t)"||"))) {

  r = (long)(intptr_t)"Bool";
  }

  if (s_eq(op, (long)(intptr_t)"u-")) {

  return u_exprType(b, ctx);
  }

  if (s_eq(r, (long)(intptr_t)"Bool")) {

  return r;
  }

  ta = u_exprType(a, ctx);

  if (s_eq(op, (long)(intptr_t)"+")) {

  if (s_eq(ta, (long)(intptr_t)"Str")) {

  r = (long)(intptr_t)"Str";
  }
  }

  if (s_eq(ta, (long)(intptr_t)"Float")) {

  if (u_isFloatArith(op)) {

  r = (long)(intptr_t)"Float";
  }
  }

  return r;
  return 0;
}
long u_exprType(long e, long ctx) {

  switch (((Obj*)(intptr_t)e)->tag) {
  case T_Num: { long v = ((Obj*)(intptr_t)e)->v0; return (long)(intptr_t)"Int"; }
  case T_FloatLit: { long s = ((Obj*)(intptr_t)e)->v0; return (long)(intptr_t)"Float"; }
  case T_Var: { long s = ((Obj*)(intptr_t)e)->v0; return u_varType(s, ctx); }
  case T_StrLit: { long s = ((Obj*)(intptr_t)e)->v0; return (long)(intptr_t)"Str"; }
  case T_ListLit: { long es = ((Obj*)(intptr_t)e)->v0; return u_listType(es, ctx); }
  case T_Bin: { long op = ((Obj*)(intptr_t)e)->v0; long a = ((Obj*)(intptr_t)e)->v1; long b = ((Obj*)(intptr_t)e)->v2; return u_binType(op, a, b, ctx); }
  case T_Call: { long name = ((Obj*)(intptr_t)e)->v0; long args = ((Obj*)(intptr_t)e)->v1; return u_callRet(name, ctx); }
  case T_Match: { long scrut = ((Obj*)(intptr_t)e)->v0; long arms = ((Obj*)(intptr_t)e)->v1; return u_matchType(scrut, arms, ctx); }
  case T_Field: { long recv = ((Obj*)(intptr_t)e)->v0; long fld = ((Obj*)(intptr_t)e)->v1; return u_fieldType(recv, fld, ctx); }
  case T_Method: { long recv = ((Obj*)(intptr_t)e)->v0; long name = ((Obj*)(intptr_t)e)->v1; long args = ((Obj*)(intptr_t)e)->v2; return u_methodRet(recv, name, ctx); }
  case T_Each: { long recv = ((Obj*)(intptr_t)e)->v0; long param = ((Obj*)(intptr_t)e)->v1; long body = ((Obj*)(intptr_t)e)->v2; return (long)(intptr_t)"Int"; }
  }
  return 0;
  return 0;
}
long u_matchType(long scrut, long arms, long ctx) {
  long r = 0;
  long a = 0;
  long styp = 0;
  long olds = 0;
  long bi = 0;

  r = (long)(intptr_t)"Int";

  if ((l_len(arms) > 0L)) {

  a = l_atc(arms, 0L, 5046);

  styp = u_exprType(scrut, ctx);

  olds = u_envOlds(((R_Ctx*)(intptr_t)ctx)->env, ((R_Arm*)(intptr_t)a)->binds);

  bi = 0L;

  while ((bi < l_len(((R_Arm*)(intptr_t)a)->binds))) {

  ({ long _t37 = l_atc(((R_Arm*)(intptr_t)a)->binds, bi, 5051); long _t38 = u_payloadType(styp, ((R_Arm*)(intptr_t)a)->tag, bi, ctx); u_envPut(((R_Ctx*)(intptr_t)ctx)->env, _t37, _t38); });

  bi = (bi + 1L);
  }

  r = u_exprType(((R_Arm*)(intptr_t)a)->body, ctx);

  u_envRestore(((R_Ctx*)(intptr_t)ctx)->env, ((R_Arm*)(intptr_t)a)->binds, olds);
  }

  return r;
  return 0;
}
long u_fieldType(long recv, long fld, long ctx) {
  long t = 0;
  long r = 0;

  t = u_exprType(recv, ctx);

  r = (long)(intptr_t)"Int";

  if ((((s_eq(fld, (long)(intptr_t)"toStr") || s_eq(fld, (long)(intptr_t)"char")) || s_eq(fld, (long)(intptr_t)"char_c")) || s_eq(fld, (long)(intptr_t)"self"))) {

  r = (long)(intptr_t)"Str";
  }

  if (s_eq(fld, (long)(intptr_t)"toFloat")) {

  r = (long)(intptr_t)"Float";
  }

  if (((s_eq(fld, (long)(intptr_t)"trim") || s_eq(fld, (long)(intptr_t)"upper")) || s_eq(fld, (long)(intptr_t)"lower"))) {

  r = (long)(intptr_t)"Str";
  }

  if (s_eq(fld, (long)(intptr_t)"not")) {

  r = (long)(intptr_t)"Bool";
  }

  if (s_eq(fld, (long)(intptr_t)"isInt")) {

  r = (long)(intptr_t)"Bool";
  }

  if (u_isMathField(fld)) {

  r = (long)(intptr_t)"Float";
  }

  if (s_eq(fld, (long)(intptr_t)"screen")) {

  r = (long)(intptr_t)"Screen";
  }

  if (s_eq(fld, (long)(intptr_t)"files")) {

  r = (long)(intptr_t)"Files";
  }

  if (s_eq(fld, (long)(intptr_t)"mail")) {

  r = (long)(intptr_t)"Mail";
  }

  if (s_eq(fld, (long)(intptr_t)"programs")) {

  r = (long)(intptr_t)"Programs";
  }

  if (s_eq(fld, (long)(intptr_t)"net")) {

  r = (long)(intptr_t)"Net";
  }

  if (s_eq(fld, (long)(intptr_t)"clock")) {

  r = (long)(intptr_t)"Clock";
  }

  if (s_eq(fld, (long)(intptr_t)"term")) {

  r = (long)(intptr_t)"Term";
  }

  if (s_eq(t, (long)(intptr_t)"Term")) {

  r = (long)(intptr_t)"Int";

  if (s_eq(fld, (long)(intptr_t)"key")) {

  r = (long)(intptr_t)"Str";
  }
  }

  if (s_eq(fld, (long)(intptr_t)"env")) {

  r = (long)(intptr_t)"Env";
  }

  if (s_eq(fld, (long)(intptr_t)"args")) {

  r = (long)(intptr_t)"List[Str]";
  }

  if (s_eq(fld, (long)(intptr_t)"stdin")) {

  r = (long)(intptr_t)"Str";
  }

  if (((s_eq(fld, (long)(intptr_t)"keys") || s_eq(fld, (long)(intptr_t)"jsonItems")) || s_eq(fld, (long)(intptr_t)"jsonKeys"))) {

  r = (long)(intptr_t)"List[Str]";
  }

  if (s_eq(fld, (long)(intptr_t)"bytes")) {

  r = (long)(intptr_t)"List[Int]";
  }

  if (s_eq(fld, (long)(intptr_t)"byValue")) {

  r = (long)(intptr_t)"List[Str]";
  }

  if (s_eq(fld, (long)(intptr_t)"sort")) {

  r = t;
  }

  if (u_isRec(t, ctx)) {

  r = u_recFieldType(t, fld, ctx);
  }

  if (u_useUFCS(t, fld, ctx)) {

  r = u_callRet(fld, ctx);
  }

  return r;
  return 0;
}
long u_recFieldType(long tyName, long fld, long ctx) {
  long r = 0;
  long recs = 0;
  long k = 0;
  long m = 0;
  long rec = 0;

  r = (long)(intptr_t)"Int";

  recs = ((R_Sigs*)(intptr_t)((R_Ctx*)(intptr_t)ctx)->sigs)->records;

  k = 0L;

  m = l_len(recs);

  while ((k < m)) {

  rec = l_atc(recs, k, 5146);

  if (s_eq(((R_RecDef*)(intptr_t)rec)->name, tyName)) {

  r = u_fieldTypeIn(rec, fld);
  }

  k = (k + 1L);
  }

  return r;
  return 0;
}
long u_fieldTypeIn(long rec, long fld) {
  long r = 0;
  long k = 0;
  long m = 0;

  r = (long)(intptr_t)"Int";

  k = 0L;

  m = l_len(((R_RecDef*)(intptr_t)rec)->fields);

  while ((k < m)) {

  if (s_eq(l_atc(((R_RecDef*)(intptr_t)rec)->fields, k, 5160), fld)) {

  r = l_atc(((R_RecDef*)(intptr_t)rec)->ftypes, k, 5161);
  }

  k = (k + 1L);
  }

  return r;
  return 0;
}
long u_methodRet(long recv, long name, long ctx) {
  long r = 0;
  long rt = 0;

  r = (long)(intptr_t)"Int";

  if (s_eq(name, (long)(intptr_t)"concat")) {

  r = (long)(intptr_t)"Str";
  }

  if (s_eq(name, (long)(intptr_t)"slice")) {

  r = (long)(intptr_t)"Str";
  }

  if ((s_eq(name, (long)(intptr_t)"read") || s_eq(name, (long)(intptr_t)"textAt"))) {

  r = (long)(intptr_t)"Str";
  }

  if (s_eq(name, (long)(intptr_t)"readBytes")) {

  r = (long)(intptr_t)"Bytes";
  }

  if ((s_eq(name, (long)(intptr_t)"intAt") || s_eq(name, (long)(intptr_t)"findByte"))) {

  r = (long)(intptr_t)"Int";
  }

  if (((((s_eq(name, (long)(intptr_t)"floatAt") || s_eq(name, (long)(intptr_t)"atan2")) || s_eq(name, (long)(intptr_t)"pow")) || s_eq(name, (long)(intptr_t)"atan2_c")) || s_eq(name, (long)(intptr_t)"pow_c"))) {

  r = (long)(intptr_t)"Float";
  }

  if (s_eq(name, (long)(intptr_t)"exists")) {

  r = (long)(intptr_t)"Bool";
  }

  if (((((((((s_eq(name, (long)(intptr_t)"run") || s_eq(name, (long)(intptr_t)"try")) || s_eq(name, (long)(intptr_t)"feed")) || s_eq(name, (long)(intptr_t)"fetch")) || s_eq(name, (long)(intptr_t)"post")) || s_eq(name, (long)(intptr_t)"fetchWith")) || s_eq(name, (long)(intptr_t)"dateAs")) || s_eq(name, (long)(intptr_t)"date")) || s_eq(name, (long)(intptr_t)"var"))) {

  r = (long)(intptr_t)"Str";
  }

  if ((s_eq(name, (long)(intptr_t)"split") || s_eq(name, (long)(intptr_t)"list"))) {

  r = (long)(intptr_t)"List[Str]";
  }

  if (((s_eq(name, (long)(intptr_t)"contains") || s_eq(name, (long)(intptr_t)"startsWith")) || s_eq(name, (long)(intptr_t)"endsWith"))) {

  r = (long)(intptr_t)"Bool";
  }

  if (((s_eq(name, (long)(intptr_t)"replace") || s_eq(name, (long)(intptr_t)"json")) || s_eq(name, (long)(intptr_t)"reReplace"))) {

  r = (long)(intptr_t)"Str";
  }

  if (s_eq(name, (long)(intptr_t)"reTest")) {

  r = (long)(intptr_t)"Bool";
  }

  if ((s_eq(name, (long)(intptr_t)"reFind") || s_eq(name, (long)(intptr_t)"reSplit"))) {

  r = (long)(intptr_t)"List[Str]";
  }

  if (s_eq(name, (long)(intptr_t)"reAll")) {

  r = (long)(intptr_t)"List[List[Str]]";
  }

  if (s_eq(name, (long)(intptr_t)"find")) {

  r = (long)(intptr_t)"Int";
  }

  if (s_eq(name, (long)(intptr_t)"join")) {

  r = (long)(intptr_t)"Str";
  }

  if (s_eq(name, (long)(intptr_t)"padLeft")) {

  r = (long)(intptr_t)"Str";
  }

  if (s_eq(name, (long)(intptr_t)"padRight")) {

  r = (long)(intptr_t)"Str";
  }

  if (s_eq(name, (long)(intptr_t)"atan2")) {

  r = (long)(intptr_t)"Float";
  }

  if (((((s_eq(name, (long)(intptr_t)"has") || s_eq(name, (long)(intptr_t)"and")) || s_eq(name, (long)(intptr_t)"or")) || s_eq(name, (long)(intptr_t)"ge")) || s_eq(name, (long)(intptr_t)"le"))) {

  r = (long)(intptr_t)"Bool";
  }

  if (s_eq(name, (long)(intptr_t)"get")) {

  r = u_mapValueOf(u_exprType(recv, ctx));
  }

  if (s_eq(name, (long)(intptr_t)"remove")) {

  r = u_elemOf(u_exprType(recv, ctx));
  }

  if (s_eq(name, (long)(intptr_t)"at")) {

  r = (long)(intptr_t)"Str";

  rt = u_exprType(recv, ctx);

  if (s_eq(rt, (long)(intptr_t)"Bytes")) {

  r = (long)(intptr_t)"Int";
  }

  if (u_isListType(rt)) {

  r = u_elemOf(rt);
  }
  }

  return r;
  return 0;
}
long u_isMain(long name) {

  return s_eq(name, (long)(intptr_t)"main");
  return 0;
}
long u_notEof(long toks, long i) {

  switch (((Obj*)(intptr_t)u_tokAt(toks, i))->tag) {
  case T_Eof: { return false; }
  case T_Ident: { long s = ((Obj*)(intptr_t)u_tokAt(toks, i))->v0; return true; }
  case T_Int: { long v = ((Obj*)(intptr_t)u_tokAt(toks, i))->v0; return true; }
  case T_Float: { long f = ((Obj*)(intptr_t)u_tokAt(toks, i))->v0; return true; }
  case T_Str: { long s = ((Obj*)(intptr_t)u_tokAt(toks, i))->v0; return true; }
  case T_Punct: { long p = ((Obj*)(intptr_t)u_tokAt(toks, i))->v0; return true; }
  }
  return 0;
  return 0;
}
long u_inBlock(long toks, long j) {

  return ((!u_isPunct(toks, j, (long)(intptr_t)"}")) && u_notEof(toks, j));
  return 0;
}
long u_inArgs(long toks, long j) {

  return ((!u_isPunct(toks, j, (long)(intptr_t)")")) && u_notEof(toks, j));
  return 0;
}
long u_isIdent(long toks, long i) {

  switch (((Obj*)(intptr_t)u_tokAt(toks, i))->tag) {
  case T_Ident: { long s = ((Obj*)(intptr_t)u_tokAt(toks, i))->v0; return true; }
  case T_Int: { long v = ((Obj*)(intptr_t)u_tokAt(toks, i))->v0; return false; }
  case T_Float: { long f = ((Obj*)(intptr_t)u_tokAt(toks, i))->v0; return false; }
  case T_Str: { long s = ((Obj*)(intptr_t)u_tokAt(toks, i))->v0; return false; }
  case T_Punct: { long p = ((Obj*)(intptr_t)u_tokAt(toks, i))->v0; return false; }
  case T_Eof: { return false; }
  }
  return 0;
  return 0;
}
long u_isWord(long toks, long i, long w) {

  switch (((Obj*)(intptr_t)u_tokAt(toks, i))->tag) {
  case T_Ident: { long s = ((Obj*)(intptr_t)u_tokAt(toks, i))->v0; return s_eq(s, w); }
  case T_Int: { long v = ((Obj*)(intptr_t)u_tokAt(toks, i))->v0; return false; }
  case T_Float: { long f = ((Obj*)(intptr_t)u_tokAt(toks, i))->v0; return false; }
  case T_Str: { long s = ((Obj*)(intptr_t)u_tokAt(toks, i))->v0; return false; }
  case T_Punct: { long p = ((Obj*)(intptr_t)u_tokAt(toks, i))->v0; return false; }
  case T_Eof: { return false; }
  }
  return 0;
  return 0;
}
long u_isAssign(long toks, long i) {

  return (u_isIdent(toks, i) && u_isPunct(toks, (i + 1L), (long)(intptr_t)"="));
  return 0;
}
long u_identAt(long toks, long i) {

  switch (((Obj*)(intptr_t)u_tokAt(toks, i))->tag) {
  case T_Ident: { long s = ((Obj*)(intptr_t)u_tokAt(toks, i))->v0; return s; }
  case T_Int: { long v = ((Obj*)(intptr_t)u_tokAt(toks, i))->v0; return (long)(intptr_t)"?"; }
  case T_Float: { long f = ((Obj*)(intptr_t)u_tokAt(toks, i))->v0; return (long)(intptr_t)"?"; }
  case T_Str: { long s = ((Obj*)(intptr_t)u_tokAt(toks, i))->v0; return (long)(intptr_t)"?"; }
  case T_Punct: { long p = ((Obj*)(intptr_t)u_tokAt(toks, i))->v0; return (long)(intptr_t)"?"; }
  case T_Eof: { return (long)(intptr_t)"?"; }
  }
  return 0;
  return 0;
}
long u_punctAt(long toks, long i) {

  switch (((Obj*)(intptr_t)u_tokAt(toks, i))->tag) {
  case T_Punct: { long p = ((Obj*)(intptr_t)u_tokAt(toks, i))->v0; return p; }
  case T_Ident: { long s = ((Obj*)(intptr_t)u_tokAt(toks, i))->v0; return (long)(intptr_t)"?"; }
  case T_Int: { long v = ((Obj*)(intptr_t)u_tokAt(toks, i))->v0; return (long)(intptr_t)"?"; }
  case T_Float: { long f = ((Obj*)(intptr_t)u_tokAt(toks, i))->v0; return (long)(intptr_t)"?"; }
  case T_Str: { long s = ((Obj*)(intptr_t)u_tokAt(toks, i))->v0; return (long)(intptr_t)"?"; }
  case T_Eof: { return (long)(intptr_t)"?"; }
  }
  return 0;
  return 0;
}
long u_isPunct(long toks, long i, long op) {

  switch (((Obj*)(intptr_t)u_tokAt(toks, i))->tag) {
  case T_Punct: { long p = ((Obj*)(intptr_t)u_tokAt(toks, i))->v0; return s_eq(p, op); }
  case T_Ident: { long s = ((Obj*)(intptr_t)u_tokAt(toks, i))->v0; return false; }
  case T_Int: { long v = ((Obj*)(intptr_t)u_tokAt(toks, i))->v0; return false; }
  case T_Float: { long f = ((Obj*)(intptr_t)u_tokAt(toks, i))->v0; return false; }
  case T_Str: { long s = ((Obj*)(intptr_t)u_tokAt(toks, i))->v0; return false; }
  case T_Eof: { return false; }
  }
  return 0;
  return 0;
}
long u_isCmpOp(long toks, long i) {

  return (((((u_isPunct(toks, i, (long)(intptr_t)"==") || u_isPunct(toks, i, (long)(intptr_t)"!=")) || u_isPunct(toks, i, (long)(intptr_t)"<")) || u_isPunct(toks, i, (long)(intptr_t)">")) || u_isPunct(toks, i, (long)(intptr_t)"<=")) || u_isPunct(toks, i, (long)(intptr_t)">="));
  return 0;
}
long u_isAddOp(long toks, long i) {

  return (u_isPunct(toks, i, (long)(intptr_t)"+") || u_isPunct(toks, i, (long)(intptr_t)"-"));
  return 0;
}
long u_isMulOp(long toks, long i) {

  return ((u_isPunct(toks, i, (long)(intptr_t)"*") || u_isPunct(toks, i, (long)(intptr_t)"/")) || u_isPunct(toks, i, (long)(intptr_t)"%"));
  return 0;
}
long u_lex(long src, long files) {
  long toks = 0;
  long lines = 0;
  long taken = 0;
  long n = 0;
  long i = 0;
  long line = 0;
  long c = 0;
  long j = 0;
  long s = 0;
  long start = 0;
  long num = 0;
  long fol = 0;

  toks = l_new();

  lines = l_new();

  taken = u_cTaken();

  n = s_len(src);

  i = 0L;

  line = (((l_len(files) - 1L) * 1000000L) + 1L);

  while ((i < n)) {

  c = s_code(s_atc(src, i, 5355));

  if (u_isSpace(c)) {

  if ((c == 10L)) {

  line = (line + 1L);
  }

  i = (i + 1L);
  } else {

  if (u_isComment(src, i, n)) {

  while (((i < n) && (!(s_code(s_atc(src, i, 5363)) == 10L)))) {

  i = (i + 1L);
  }
  } else {

  if (((c == 34L) || (c == 39L))) {

  j = (i + 1L);

  s = (long)(intptr_t)"";

  while (((j < n) && (!(s_code(s_atc(src, j, 5370)) == c)))) {

  if ((s_code(s_atc(src, j, 5371)) == 10L)) {

  fail(s_concat(u_spot(files, line), (long)(intptr_t)"a text is missing its closing quote"));
  }

  if ((s_code(s_atc(src, j, 5374)) == 92L)) {

  j = (j + 1L);

  if ((j == n)) {

  fail(s_concat(u_spot(files, line), (long)(intptr_t)"a text is missing its closing quote"));
  }

  s = s_concat(s, u_esc(src, j));
  } else {

  s = s_concat(s, s_atc(src, j, 5381));
  }

  j = (j + 1L);
  }

  if ((j == n)) {

  fail(s_concat(u_spot(files, line), (long)(intptr_t)"a text is missing its closing quote"));
  }

  l_push(toks, Str(s));

  l_push(lines, line);

  i = (j + 1L);
  } else {

  if (u_isDigit(c)) {

  start = i;

  num = 0L;

  while (((i < n) && u_isDigit(s_code(s_atc(src, i, 5395))))) {

  num = ((num * 10L) + (s_code(s_atc(src, i, 5396)) - 48L));

  i = (i + 1L);
  }

  if (((((i + 1L) < n) && (s_code(s_atc(src, i, 5401)) == 46L)) && u_isDigit(s_code(s_atc(src, (i + 1L), 5401))))) {

  i = (i + 1L);

  while (((i < n) && u_isDigit(s_code(s_atc(src, i, 5403))))) {

  i = (i + 1L);
  }

  if (((i < n) && ((s_code(s_atc(src, i, 5407)) == 101L) || (s_code(s_atc(src, i, 5407)) == 69L)))) {

  j = (i + 1L);

  if (((j < n) && ((s_code(s_atc(src, j, 5409)) == 43L) || (s_code(s_atc(src, j, 5409)) == 45L)))) {

  j = (j + 1L);
  }

  if (((j < n) && u_isDigit(s_code(s_atc(src, j, 5412))))) {

  i = j;

  while (((i < n) && u_isDigit(s_code(s_atc(src, i, 5414))))) {

  i = (i + 1L);
  }
  }
  }

  l_push(toks, Float(s_slicec(src, start, i, 5419)));

  l_push(lines, line);
  } else {

  l_push(toks, Int(num));

  l_push(lines, line);
  }
  } else {

  if (u_isAlpha(c)) {

  j = i;

  while (((j < n) && u_isAlnum(s_code(s_atc(src, j, 5428))))) {

  j = (j + 1L);
  }

  l_push(toks, Ident(u_safeName(s_slicec(src, i, j, 5431), taken)));

  l_push(lines, line);

  i = j;
  } else {

  if (u_isTwoCharOp(src, i, n)) {

  l_push(toks, Punct(s_slicec(src, i, (i + 2L), 5436)));

  l_push(lines, line);

  i = (i + 2L);
  } else {

  fol = ((l_len(lines) == 0L) || (!(l_atc(lines, (l_len(lines) - 1L), 5444) == line)));

  if (((s_code(s_atc(src, i, 5445)) == 40L) && fol)) {

  l_push(toks, Punct((long)(intptr_t)"s("));
  } else {

  l_push(toks, Punct(s_atc(src, i, 5448)));
  }

  l_push(lines, line);

  i = (i + 1L);
  }
  }
  }
  }
  }
  }
  }

  l_push(toks, Eof());

  l_push(lines, line);

  return Lexed(toks, lines, files);
  return 0;
}
long u_esc(long src, long j) {
  long c = 0;
  long r = 0;

  c = s_code(s_atc(src, j, 5465));

  r = s_atc(src, j, 5466);

  if ((c == 110L)) {

  r = (long)(intptr_t)"\n";
  }

  if ((c == 116L)) {

  r = (long)(intptr_t)"\t";
  }

  if ((c == 114L)) {

  r = (long)(intptr_t)"\r";
  }

  if (((((((!(c == 110L)) && (!(c == 116L))) && (!(c == 114L))) && (!(c == 92L))) && (!(c == 34L))) && (!(c == 39L)))) {

  r = s_concat((long)(intptr_t)"\\", r);
  }

  return r;
  return 0;
}
long u_isComment(long src, long i, long n) {

  return ((((i + 1L) < n) && (s_code(s_atc(src, i, 5477)) == 47L)) && (s_code(s_atc(src, (i + 1L), 5477)) == 47L));
  return 0;
}
long u_isTwoCharOp(long src, long i, long n) {

  return (((((((u_twoIs(src, i, n, 45L, 62L) || u_twoIs(src, i, n, 61L, 61L)) || u_twoIs(src, i, n, 60L, 61L)) || u_twoIs(src, i, n, 62L, 61L)) || u_twoIs(src, i, n, 38L, 38L)) || u_twoIs(src, i, n, 124L, 124L)) || u_twoIs(src, i, n, 33L, 61L)) || u_twoIs(src, i, n, 46L, 46L));
  return 0;
}
long u_twoIs(long src, long i, long n, long a, long b) {

  return ((((i + 1L) < n) && (s_code(s_atc(src, i, 5485)) == a)) && (s_code(s_atc(src, (i + 1L), 5485)) == b));
  return 0;
}
long u_isDigit(long c) {

  return ((c >= 48L) && (c <= 57L));
  return 0;
}
long u_cTaken() {

  return l_with(l_with(l_with(l_with(l_with(l_with(l_with(l_with(l_with(l_with(l_with(l_with(l_with(l_with(l_with(l_with(l_with(l_with(l_with(l_with(l_with(l_with(l_with(l_with(l_with(l_with(l_with(l_with(l_with(l_with(l_with(l_with(l_with(l_with(l_with(l_with(l_with(l_with(l_with(l_with(l_with(l_with(l_with(l_with(l_with(l_with(l_with(l_with(l_with(l_with(l_with(l_with(l_with(l_with(l_with(l_with(l_with(l_with(l_with(l_with(l_with(l_with(l_with(l_with(l_with(l_with(l_with(l_with(l_with(l_with(l_with(l_with(l_with(l_with(l_with(l_with(l_with(l_with(l_with(l_new(), (long)(intptr_t)"auto"), (long)(intptr_t)"case"), (long)(intptr_t)"char"), (long)(intptr_t)"const"), (long)(intptr_t)"default"), (long)(intptr_t)"do"), (long)(intptr_t)"double"), (long)(intptr_t)"enum"), (long)(intptr_t)"extern"), (long)(intptr_t)"float"), (long)(intptr_t)"goto"), (long)(intptr_t)"inline"), (long)(intptr_t)"int"), (long)(intptr_t)"long"), (long)(intptr_t)"register"), (long)(intptr_t)"restrict"), (long)(intptr_t)"short"), (long)(intptr_t)"signed"), (long)(intptr_t)"sizeof"), (long)(intptr_t)"static"), (long)(intptr_t)"struct"), (long)(intptr_t)"switch"), (long)(intptr_t)"typedef"), (long)(intptr_t)"union"), (long)(intptr_t)"unsigned"), (long)(intptr_t)"void"), (long)(intptr_t)"volatile"), (long)(intptr_t)"Obj"), (long)(intptr_t)"SList"), (long)(intptr_t)"SMap"), (long)(intptr_t)"FILE"), (long)(intptr_t)"NULL"), (long)(intptr_t)"EOF"), (long)(intptr_t)"NAN"), (long)(intptr_t)"INFINITY"), (long)(intptr_t)"BUFSIZ"), (long)(intptr_t)"DIR"), (long)(intptr_t)"errno"), (long)(intptr_t)"RAND_MAX"), (long)(intptr_t)"EXIT_SUCCESS"), (long)(intptr_t)"EXIT_FAILURE"), (long)(intptr_t)"SEEK_SET"), (long)(intptr_t)"SEEK_CUR"), (long)(intptr_t)"SEEK_END"), (long)(intptr_t)"HUGE_VAL"), (long)(intptr_t)"mk"), (long)(intptr_t)"l2d"), (long)(intptr_t)"d2l"), (long)(intptr_t)"argc"), (long)(intptr_t)"argv"), (long)(intptr_t)"printf"), (long)(intptr_t)"fprintf"), (long)(intptr_t)"sprintf"), (long)(intptr_t)"snprintf"), (long)(intptr_t)"malloc"), (long)(intptr_t)"realloc"), (long)(intptr_t)"free"), (long)(intptr_t)"exit"), (long)(intptr_t)"strlen"), (long)(intptr_t)"strcmp"), (long)(intptr_t)"strcpy"), (long)(intptr_t)"strcat"), (long)(intptr_t)"strstr"), (long)(intptr_t)"memcpy"), (long)(intptr_t)"atol"), (long)(intptr_t)"atof"), (long)(intptr_t)"qsort"), (long)(intptr_t)"getchar"), (long)(intptr_t)"fopen"), (long)(intptr_t)"fclose"), (long)(intptr_t)"fputs"), (long)(intptr_t)"fread"), (long)(intptr_t)"fseek"), (long)(intptr_t)"ftell"), (long)(intptr_t)"pow"), (long)(intptr_t)"fmod"), (long)(intptr_t)"ceil"), (long)(intptr_t)"fabs"), (long)(intptr_t)"round");
  return 0;
}
long u_safeName(long s, long taken) {
  long r = 0;
  long n = 0;
  long c0 = 0;
  long c1 = 0;

  r = s;

  n = s_len(s);

  if ((n > 1L)) {

  c0 = s_code(s_atc(s, 0L, 5508));

  c1 = s_code(s_atc(s, 1L, 5509));

  if (((c0 == 95L) || ((c1 == 95L) && (((((((c0 == 108L) || (c0 == 115L)) || (c0 == 109L)) || (c0 == 105L)) || (c0 == 102L)) || (c0 == 117L)) || (c0 == 84L))))) {

  r = s_concat(s, (long)(intptr_t)"_c");
  } else {

  if (u_hasName(taken, s)) {

  r = s_concat(s, (long)(intptr_t)"_c");
  }

  if ((n > 3L)) {

  if (s_eq(s_slicec(s, 0L, 4L, 5517), (long)(intptr_t)"cmp_")) {

  r = s_concat(s, (long)(intptr_t)"_c");
  }
  }

  if ((n > 7L)) {

  if (s_eq(s_slicec(s, 0L, 8L, 5522), (long)(intptr_t)"simpler_")) {

  r = s_concat(s, (long)(intptr_t)"_c");
  }
  }
  }
  }

  return r;
  return 0;
}
long u_isAlpha(long c) {

  return ((((c >= 65L) && (c <= 90L)) || ((c >= 97L) && (c <= 122L))) || (c == 95L));
  return 0;
}
long u_isAlnum(long c) {

  return (u_isAlpha(c) || u_isDigit(c));
  return 0;
}
long u_isSpace(long c) {

  return ((((c == 32L) || (c == 9L)) || (c == 10L)) || (c == 13L));
  return 0;
}
