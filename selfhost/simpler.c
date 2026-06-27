#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
typedef struct { int tag; long v0; long v1; long v2; } Obj;
long mk(int tag, long v0, long v1, long v2) { Obj* o = malloc(sizeof(Obj)); o->tag = tag; o->v0 = v0; o->v1 = v1; o->v2 = v2; return (long)(intptr_t)o; }
typedef struct { long* data; long len; long cap; } SList;
long l_new() { SList* l = (SList*)malloc(sizeof(SList)); l->cap = 8; l->len = 0; l->data = (long*)malloc(sizeof(long) * l->cap); return (long)(intptr_t)l; }
long l_push(long lp, long x) { SList* l = (SList*)(intptr_t)lp; if (l->len == l->cap) { l->cap = l->cap * 2; l->data = (long*)realloc(l->data, sizeof(long) * l->cap); } l->data[l->len] = x; l->len = l->len + 1; return 0; }
long l_at(long lp, long i) { return ((SList*)(intptr_t)lp)->data[i]; }
long l_len(long lp) { return ((SList*)(intptr_t)lp)->len; }
long s_len(long s) { return (long)strlen((const char*)(intptr_t)s); }
long s_at(long s, long i) { const char* p = (const char*)(intptr_t)s; char* r = (char*)malloc(2); r[0] = p[i]; r[1] = 0; return (long)(intptr_t)r; }
long s_code(long s) { return (long)(unsigned char)((const char*)(intptr_t)s)[0]; }
long s_slice(long s, long a, long b) { const char* p = (const char*)(intptr_t)s; if (b < a) b = a; long n = b - a; char* r = (char*)malloc(n + 1); for (long k = 0; k < n; k++) r[k] = p[a + k]; r[n] = 0; return (long)(intptr_t)r; }
long s_concat(long a, long b) { const char* x = (const char*)(intptr_t)a; const char* y = (const char*)(intptr_t)b; char* r = (char*)malloc(strlen(x) + strlen(y) + 1); strcpy(r, x); strcat(r, y); return (long)(intptr_t)r; }
long s_eq(long a, long b) { return strcmp((const char*)(intptr_t)a, (const char*)(intptr_t)b) == 0; }
long i_tostr(long n) { char* r = (char*)malloc(24); sprintf(r, "%ld", n); return (long)(intptr_t)r; }
double l2d(long x) { union { long l; double d; } u; u.l = x; return u.d; }
long d2l(double x) { union { long l; double d; } u; u.d = x; return u.l; }
long f_tostr(long x) { char* r = (char*)malloc(32); snprintf(r, 32, "%g", l2d(x)); return (long)(intptr_t)r; }
long s_tofloat(long s) { return d2l(atof((const char*)(intptr_t)s)); }
long i_tof(long n) { return d2l((double)n); }
long f_toi(long x) { return (long)l2d(x); }
long s_toint(long s) { return atol((const char*)(intptr_t)s); }
long s_split(long s, long d) { const char* p = (const char*)(intptr_t)s; char dc = ((const char*)(intptr_t)d)[0]; long l = l_new(); long start = 0; long i = 0; while (1) { char c = p[i]; if (c == 0 || c == dc) { long n = i - start; char* r = (char*)malloc(n + 1); for (long k = 0; k < n; k++) r[k] = p[start + k]; r[n] = 0; l_push(l, (long)(intptr_t)r); if (c == 0) break; start = i + 1; } i++; } return l; }
long s_contains(long h, long n) { return strstr((const char*)(intptr_t)h, (const char*)(intptr_t)n) != 0; }
long s_trim(long s) { const char* p = (const char*)(intptr_t)s; long n = strlen(p); long a = 0; while (a < n && (p[a] == 32 || p[a] == 9 || p[a] == 13 || p[a] == 10)) a = a + 1; long b = n; while (b > a && (p[b - 1] == 32 || p[b - 1] == 9 || p[b - 1] == 13 || p[b - 1] == 10)) b = b - 1; long len = b - a; char* r = (char*)malloc(len + 1); for (long k = 0; k < len; k++) r[k] = p[a + k]; r[len] = 0; return (long)(intptr_t)r; }
long s_replace(long s, long from, long to) { const char* p = (const char*)(intptr_t)s; const char* f = (const char*)(intptr_t)from; const char* t = (const char*)(intptr_t)to; long fl = strlen(f); if (fl == 0) return s; long tl = strlen(t); long pl = strlen(p); long count = 0; const char* q = p; while ((q = strstr(q, f)) != 0) { count++; q += fl; } char* r = (char*)malloc(pl + count * (tl - fl) + 1); char* w = r; const char* a = p; while (1) { const char* hit = strstr(a, f); if (hit == 0) { strcpy(w, a); break; } long pre = hit - a; memcpy(w, a, pre); w += pre; memcpy(w, t, tl); w += tl; a = hit + fl; } return (long)(intptr_t)r; }
typedef struct { long* keys; long* vals; long len; long cap; } SMap;
long m_find(long mp, long k) { SMap* m = (SMap*)(intptr_t)mp; const char* key = (const char*)(intptr_t)k; for (long i = 0; i < m->len; i++) { if (strcmp((const char*)(intptr_t)m->keys[i], key) == 0) return i; } return -1; }
long Map() { SMap* m = (SMap*)malloc(sizeof(SMap)); m->cap = 8; m->len = 0; m->keys = (long*)malloc(sizeof(long) * m->cap); m->vals = (long*)malloc(sizeof(long) * m->cap); return (long)(intptr_t)m; }
long m_set(long mp, long k, long v) { SMap* m = (SMap*)(intptr_t)mp; long i = m_find(mp, k); if (i >= 0) { m->vals[i] = v; return 0; } if (m->len == m->cap) { m->cap = m->cap * 2; m->keys = (long*)realloc(m->keys, sizeof(long) * m->cap); m->vals = (long*)realloc(m->vals, sizeof(long) * m->cap); } m->keys[m->len] = k; m->vals[m->len] = v; m->len = m->len + 1; return 0; }
long m_get(long mp, long k) { long i = m_find(mp, k); if (i >= 0) return ((SMap*)(intptr_t)mp)->vals[i]; return 0; }
long m_gets(long mp, long k) { long i = m_find(mp, k); if (i >= 0) return ((SMap*)(intptr_t)mp)->vals[i]; return (long)(intptr_t)""; }
long m_has(long mp, long k) { return m_find(mp, k) >= 0; }
long m_keys(long mp) { SMap* m = (SMap*)(intptr_t)mp; long l = l_new(); for (long i = 0; i < m->len; i++) l_push(l, m->keys[i]); return l; }
long m_byvalue(long mp) { SMap* m = (SMap*)(intptr_t)mp; long n = m->len; long c = n > 0 ? n : 1; long* idx = (long*)malloc(sizeof(long) * c); for (long i = 0; i < n; i++) idx[i] = i; for (long i = 1; i < n; i++) { long t = idx[i]; long j = i - 1; while (j >= 0 && m->vals[idx[j]] < m->vals[t]) { idx[j + 1] = idx[j]; j = j - 1; } idx[j + 1] = t; } long l = l_new(); for (long i = 0; i < n; i++) l_push(l, m->keys[idx[i]]); return l; }
int cmp_long(const void* a, const void* b) { long x = *(const long*)a; long y = *(const long*)b; if (x < y) return -1; if (x > y) return 1; return 0; }
int cmp_str(const void* a, const void* b) { return strcmp((const char*)(intptr_t)(*(const long*)a), (const char*)(intptr_t)(*(const long*)b)); }
long l_sort(long lp, long isStr) { SList* s = (SList*)(intptr_t)lp; long n = s->len; long c = n > 0 ? n : 1; long* d = (long*)malloc(sizeof(long) * c); for (long i = 0; i < n; i++) d[i] = s->data[i]; qsort(d, n, sizeof(long), isStr ? cmp_str : cmp_long); long r = l_new(); for (long i = 0; i < n; i++) l_push(r, d[i]); return r; }
const char* simpler_read(const char* path) { FILE* f = fopen(path, "rb"); if (!f) return ""; fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET); char* buf = (char*)malloc(n + 1); long got = fread(buf, 1, n, f); buf[got] = 0; fclose(f); return buf; }
long simpler_write(long path, long content) { FILE* f = fopen((const char*)(intptr_t)path, "w"); if (f) { fputs((const char*)(intptr_t)content, f); fclose(f); } return 0; }
long simpler_args(int argc, char** argv) { long l = l_new(); for (int k = 1; k < argc; k++) l_push(l, (long)(intptr_t)argv[k]); return l; }
long simpler_stdin() { long cap = 1024; long len = 0; char* buf = (char*)malloc(cap); int c; while ((c = getchar()) != EOF) { if (len + 1 >= cap) { cap = cap * 2; buf = (char*)realloc(buf, cap); } buf[len] = (char)c; len = len + 1; } buf[len] = 0; return (long)(intptr_t)buf; }
long fail(long msg) { fprintf(stderr, "%s\n", (const char*)(intptr_t)msg); exit(1); return 0; }
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
enum { T_Let, T_Bare, T_If, T_While };
long Let(long v0, long v1, long v2) { return mk(T_Let, v0, v1, v2); }
long Bare(long v0) { return mk(T_Bare, v0, 0, 0); }
long If(long v0, long v1, long v2) { return mk(T_If, v0, v1, v2); }
long While(long v0, long v1) { return mk(T_While, v0, v1, 0); }
enum { T_Ident, T_Str, T_Int, T_Float, T_Punct, T_Eof };
long Ident(long v0) { return mk(T_Ident, v0, 0, 0); }
long Str(long v0) { return mk(T_Str, v0, 0, 0); }
long Int(long v0) { return mk(T_Int, v0, 0, 0); }
long Float(long v0) { return mk(T_Float, v0, 0, 0); }
long Punct(long v0) { return mk(T_Punct, v0, 0, 0); }
long Eof() { return mk(T_Eof, 0, 0, 0); }
typedef struct { long tag; long binds; long body; } ArmT;
long Arm(long tag, long binds, long body) { ArmT* o = malloc(sizeof(ArmT)); o->tag = tag; o->binds = binds; o->body = body; return (long)(intptr_t)o; }
typedef struct { long name; long params; long ptypes; long ret; long effs; long body; long line; } FnT;
long Fn(long name, long params, long ptypes, long ret, long effs, long body, long line) { FnT* o = malloc(sizeof(FnT)); o->name = name; o->params = params; o->ptypes = ptypes; o->ret = ret; o->effs = effs; o->body = body; o->line = line; return (long)(intptr_t)o; }
typedef struct { long toks; long lines; } LexedT;
long Lexed(long toks, long lines) { LexedT* o = malloc(sizeof(LexedT)); o->toks = toks; o->lines = lines; return (long)(intptr_t)o; }
typedef struct { long cname; long arity; long ptypes; } CaseT;
long Case(long cname, long arity, long ptypes) { CaseT* o = malloc(sizeof(CaseT)); o->cname = cname; o->arity = arity; o->ptypes = ptypes; return (long)(intptr_t)o; }
typedef struct { long name; long cases; } TyDefT;
long TyDef(long name, long cases) { TyDefT* o = malloc(sizeof(TyDefT)); o->name = name; o->cases = cases; return (long)(intptr_t)o; }
typedef struct { long name; long fields; long ftypes; } RecDefT;
long RecDef(long name, long fields, long ftypes) { RecDefT* o = malloc(sizeof(RecDefT)); o->name = name; o->fields = fields; o->ftypes = ftypes; return (long)(intptr_t)o; }
typedef struct { long types; long records; long fns; } ProgT;
long Prog(long types, long records, long fns) { ProgT* o = malloc(sizeof(ProgT)); o->types = types; o->records = records; o->fns = fns; return (long)(intptr_t)o; }
typedef struct { long names; long tys; } TyEnvT;
long TyEnv(long names, long tys) { TyEnvT* o = malloc(sizeof(TyEnvT)); o->names = names; o->tys = tys; return (long)(intptr_t)o; }
typedef struct { long recNames; long records; long types; long fns; long fnNames; long fnRets; long boxedNullary; } SigsT;
long Sigs(long recNames, long records, long types, long fns, long fnNames, long fnRets, long boxedNullary) { SigsT* o = malloc(sizeof(SigsT)); o->recNames = recNames; o->records = records; o->types = types; o->fns = fns; o->fnNames = fnNames; o->fnRets = fnRets; o->boxedNullary = boxedNullary; return (long)(intptr_t)o; }
typedef struct { long env; long sigs; } CtxT;
long Ctx(long env, long sigs) { CtxT* o = malloc(sizeof(CtxT)); o->env = env; o->sigs = sigs; return (long)(intptr_t)o; }
typedef struct { long node; long next; } ParsedT;
long Parsed(long node, long next) { ParsedT* o = malloc(sizeof(ParsedT)); o->node = node; o->next = next; return (long)(intptr_t)o; }
typedef struct { long node; long next; } PStmtT;
long PStmt(long node, long next) { PStmtT* o = malloc(sizeof(PStmtT)); o->node = node; o->next = next; return (long)(intptr_t)o; }
typedef struct { long arm; long next; } PArmT;
long PArm(long arm, long next) { PArmT* o = malloc(sizeof(PArmT)); o->arm = arm; o->next = next; return (long)(intptr_t)o; }
typedef struct { long fn; long next; } PFnT;
long PFn(long fn, long next) { PFnT* o = malloc(sizeof(PFnT)); o->fn = fn; o->next = next; return (long)(intptr_t)o; }
typedef struct { long ty; long next; } PTyT;
long PTy(long ty, long next) { PTyT* o = malloc(sizeof(PTyT)); o->ty = ty; o->next = next; return (long)(intptr_t)o; }
typedef struct { long rec; long next; } PRecT;
long PRec(long rec, long next) { PRecT* o = malloc(sizeof(PRecT)); o->rec = rec; o->next = next; return (long)(intptr_t)o; }
typedef struct { long names; long types; long next; } PNamesT;
long PNames(long names, long types, long next) { PNamesT* o = malloc(sizeof(PNamesT)); o->names = names; o->types = types; o->next = next; return (long)(intptr_t)o; }
typedef struct { long ret; long effs; long next; } PRetT;
long PRet(long ret, long effs, long next) { PRetT* o = malloc(sizeof(PRetT)); o->ret = ret; o->effs = effs; o->next = next; return (long)(intptr_t)o; }
typedef struct { long body; long next; } PBodyT;
long PBody(long body, long next) { PBodyT* o = malloc(sizeof(PBodyT)); o->body = body; o->next = next; return (long)(intptr_t)o; }
typedef struct { long list; long next; } PArgsT;
long PArgs(long list, long next) { PArgsT* o = malloc(sizeof(PArgsT)); o->list = list; o->next = next; return (long)(intptr_t)o; }
long buildSigs(long prog);
long parse(long toks, long lines);
long isTypeDef(long toks, long i);
long isRecordDef(long toks, long i);
long parseRecord(long toks, long i);
long parseTypeDef(long toks, long i);
long parseFn(long toks, long lines, long i);
long parseParams(long toks, long i);
long parseRet(long toks, long j);
long parseBlock(long toks, long i);
long parseStmt(long toks, long i);
long isTypedAssign(long toks, long i);
long parseTypedLet(long toks, long i);
long parseIf(long toks, long i);
long parseWhile(long toks, long i);
long parseExpr(long toks, long pos);
long parseAdd(long toks, long pos);
long parseTerm(long toks, long pos);
long parsePostfix(long toks, long pos);
long parseEach(long toks, long recv, long i);
long parseFactor(long toks, long pos);
long parsePunctFactor(long toks, long pos);
long parseList(long toks, long pos);
long parseIdentFactor(long toks, long pos);
long parseParen(long toks, long pos);
long parseArgs(long toks, long i);
long parseMatch(long toks, long scrut, long i);
long parseArm(long toks, long j);
long emitType(long t);
long emitEnumType(long t);
long emitBoxedType(long t);
long hasPayload(long t);
long emitRecord(long rec);
long recParams(long rec);
long slotName(long bi);
long ctorParams(long arity);
long ctorArgs(long arity);
long emitProto(long f);
long emitFn(long f, long sigs);
long paramDecls(long f);
long newEnv();
long envPut(long env, long name, long ty);
long envGet(long env, long name);
long seedEnv(long body, long ctx);
long seedStmt(long s, long ctx);
long seedIf(long t, long el, long ctx);
long seedExpr(long e, long ctx);
long seedEach(long recv, long param, long body, long ctx);
long inferType(long e, long ctx);
long letType(long ty, long e, long ctx);
long callRet(long name, long ctx);
long variantOf(long name, long ctx);
long hasCase(long t, long name);
long isRec(long ty, long ctx);
long collectLets(long body);
long collectBody(long body, long names);
long collectStmt(long s, long names);
long collectExpr(long e, long names);
long collectIf(long t, long el, long names);
long pushUnique(long names, long name);
long hasName(long names, long name);
long emitStmt(long s, long asReturn, long ctx);
long emitIf(long c, long t, long el, long ctx);
long emitWhile(long c, long b, long ctx);
long emitBlock(long body, long ctx);
long emitBare(long e, long asReturn, long ctx);
long stmtExpr(long e, long ctx);
long exprStmt(long e, long ctx);
long emitCallStmt(long name, long args, long e, long ctx);
long emitMethodStmt(long name, long args, long e, long ctx);
long emitPrint(long arg, long ctx);
long emitEach(long recv, long param, long body, long ctx);
long isListType(long t);
long elemOf(long t);
long isMapType(long t);
long mapValueOf(long t);
long emitReturn(long e, long ctx);
long retVal(long e, long ctx);
long emitSwitch(long scrut, long arms, long ctx);
long emitArm(long a, long scrutC, long boxed, long styp, long ctx);
long payloadType(long tyName, long caseTag, long idx, long ctx);
long casePayloadType(long t, long caseTag, long idx);
long boxField(long scrutC, long field);
long checkExhaustive(long styp, long arms, long ctx);
long checkArities(long t, long arms, long ctx);
long failAt(long msg, long ctx);
long checkEffects(long f, long ctx);
long effBody(long body, long used, long ctx);
long effStmt(long s, long used, long ctx);
long effIf(long c, long t, long el, long used, long ctx);
long effWhile(long c, long b, long used, long ctx);
long effExpr(long e, long used, long ctx);
long effMethodE(long recv, long name, long args, long used, long ctx);
long effCallE(long name, long args, long used, long ctx);
long effBin(long a, long b, long used, long ctx);
long effMatchE(long scrut, long arms, long used, long ctx);
long effEachE(long recv, long body, long used, long ctx);
long effArgs(long args, long used, long ctx);
long addAll(long src, long dst);
long caseArityIn(long t, long tag);
long checkCases(long t, long arms, long ctx);
long armCovers(long arms, long cname);
long armHasBind(long arms);
long emitExpr(long e, long ctx);
long emitBin(long op, long a, long b, long ctx);
long isFloatArith(long op);
long checkBinOp(long op, long a, long b, long ctx);
long mixedFloat(long a, long b);
long isArithOp(long op);
long emitCall(long name, long args, long ctx);
long checkReturn(long f, long ctx);
long checkReturnStmt(long s, long ret, long ctx);
long checkRetExpr(long e, long ret, long ctx);
long isMatchExpr(long e);
long checkCall(long name, long args, long ctx);
long checkArgTypes(long name, long ptypes, long args, long ctx);
long compatible(long want, long got, long ctx);
long intLike(long t);
long notInt(long t, long ctx);
long varType(long s, long ctx);
long isVariant(long t, long ctx);
long emitField(long recv, long fld, long ctx);
long isCapField(long fld);
long emitMethod(long recv, long name, long args, long ctx);
long cEscape(long s);
long emitVar(long s, long ctx);
long emitArgs(long args, long ctx);
long binType(long op, long a, long b, long ctx);
long exprType(long e, long ctx);
long fieldType(long recv, long fld, long ctx);
long recFieldType(long tyName, long fld, long ctx);
long fieldTypeIn(long rec, long fld);
long methodRet(long recv, long name, long ctx);
long isMain(long name);
long notEof(long toks, long i);
long inBlock(long toks, long j);
long inArgs(long toks, long j);
long isIdent(long toks, long i);
long isWord(long toks, long i, long w);
long isAssign(long toks, long i);
long identAt(long toks, long i);
long punctAt(long toks, long i);
long isPunct(long toks, long i, long op);
long isCmpOp(long toks, long i);
long isAddOp(long toks, long i);
long isMulOp(long toks, long i);
long lex(long src);
long esc(long src, long j);
long isComment(long src, long i, long n);
long isTwoCharOp(long src, long i, long n);
long twoIs(long src, long i, long n, long a, long b);
long isDigit(long c);
long isAlpha(long c);
long isAlnum(long c);
long isSpace(long c);
int main(int argc, char** argv) {
  long src = 0;
  long lexed = 0;
  long toks = 0;
  long prog = 0;
  long sigs = 0;
  src = (long)(intptr_t)simpler_read((const char*)(intptr_t)(long)(intptr_t)"input.smplr");
  lexed = lex(src);
  toks = ((LexedT*)(intptr_t)lexed)->toks;
  prog = parse(toks, ((LexedT*)(intptr_t)lexed)->lines);
  sigs = buildSigs(prog);
  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"#include <stdio.h>");
  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"#include <stdlib.h>");
  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"#include <stdint.h>");
  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"#include <string.h>");
  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"typedef struct { int tag; long v0; long v1; long v2; } Obj;");
  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long mk(int tag, long v0, long v1, long v2) { Obj* o = malloc(sizeof(Obj)); o->tag = tag; o->v0 = v0; o->v1 = v1; o->v2 = v2; return (long)(intptr_t)o; }");
  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"typedef struct { long* data; long len; long cap; } SList;");
  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long l_new() { SList* l = (SList*)malloc(sizeof(SList)); l->cap = 8; l->len = 0; l->data = (long*)malloc(sizeof(long) * l->cap); return (long)(intptr_t)l; }");
  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long l_push(long lp, long x) { SList* l = (SList*)(intptr_t)lp; if (l->len == l->cap) { l->cap = l->cap * 2; l->data = (long*)realloc(l->data, sizeof(long) * l->cap); } l->data[l->len] = x; l->len = l->len + 1; return 0; }");
  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long l_at(long lp, long i) { return ((SList*)(intptr_t)lp)->data[i]; }");
  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long l_len(long lp) { return ((SList*)(intptr_t)lp)->len; }");
  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long s_len(long s) { return (long)strlen((const char*)(intptr_t)s); }");
  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long s_at(long s, long i) { const char* p = (const char*)(intptr_t)s; char* r = (char*)malloc(2); r[0] = p[i]; r[1] = 0; return (long)(intptr_t)r; }");
  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long s_code(long s) { return (long)(unsigned char)((const char*)(intptr_t)s)[0]; }");
  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long s_slice(long s, long a, long b) { const char* p = (const char*)(intptr_t)s; if (b < a) b = a; long n = b - a; char* r = (char*)malloc(n + 1); for (long k = 0; k < n; k++) r[k] = p[a + k]; r[n] = 0; return (long)(intptr_t)r; }");
  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long s_concat(long a, long b) { const char* x = (const char*)(intptr_t)a; const char* y = (const char*)(intptr_t)b; char* r = (char*)malloc(strlen(x) + strlen(y) + 1); strcpy(r, x); strcat(r, y); return (long)(intptr_t)r; }");
  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long s_eq(long a, long b) { return strcmp((const char*)(intptr_t)a, (const char*)(intptr_t)b) == 0; }");
  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long i_tostr(long n) { char* r = (char*)malloc(24); sprintf(r, \"%ld\", n); return (long)(intptr_t)r; }");
  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"double l2d(long x) { union { long l; double d; } u; u.l = x; return u.d; }");
  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long d2l(double x) { union { long l; double d; } u; u.d = x; return u.l; }");
  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long f_tostr(long x) { char* r = (char*)malloc(32); snprintf(r, 32, \"%g\", l2d(x)); return (long)(intptr_t)r; }");
  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long s_tofloat(long s) { return d2l(atof((const char*)(intptr_t)s)); }");
  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long i_tof(long n) { return d2l((double)n); }");
  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long f_toi(long x) { return (long)l2d(x); }");
  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long s_toint(long s) { return atol((const char*)(intptr_t)s); }");
  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long s_split(long s, long d) { const char* p = (const char*)(intptr_t)s; char dc = ((const char*)(intptr_t)d)[0]; long l = l_new(); long start = 0; long i = 0; while (1) { char c = p[i]; if (c == 0 || c == dc) { long n = i - start; char* r = (char*)malloc(n + 1); for (long k = 0; k < n; k++) r[k] = p[start + k]; r[n] = 0; l_push(l, (long)(intptr_t)r); if (c == 0) break; start = i + 1; } i++; } return l; }");
  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long s_contains(long h, long n) { return strstr((const char*)(intptr_t)h, (const char*)(intptr_t)n) != 0; }");
  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long s_trim(long s) { const char* p = (const char*)(intptr_t)s; long n = strlen(p); long a = 0; while (a < n && (p[a] == 32 || p[a] == 9 || p[a] == 13 || p[a] == 10)) a = a + 1; long b = n; while (b > a && (p[b - 1] == 32 || p[b - 1] == 9 || p[b - 1] == 13 || p[b - 1] == 10)) b = b - 1; long len = b - a; char* r = (char*)malloc(len + 1); for (long k = 0; k < len; k++) r[k] = p[a + k]; r[len] = 0; return (long)(intptr_t)r; }");
  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long s_replace(long s, long from, long to) { const char* p = (const char*)(intptr_t)s; const char* f = (const char*)(intptr_t)from; const char* t = (const char*)(intptr_t)to; long fl = strlen(f); if (fl == 0) return s; long tl = strlen(t); long pl = strlen(p); long count = 0; const char* q = p; while ((q = strstr(q, f)) != 0) { count++; q += fl; } char* r = (char*)malloc(pl + count * (tl - fl) + 1); char* w = r; const char* a = p; while (1) { const char* hit = strstr(a, f); if (hit == 0) { strcpy(w, a); break; } long pre = hit - a; memcpy(w, a, pre); w += pre; memcpy(w, t, tl); w += tl; a = hit + fl; } return (long)(intptr_t)r; }");
  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"typedef struct { long* keys; long* vals; long len; long cap; } SMap;");
  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long m_find(long mp, long k) { SMap* m = (SMap*)(intptr_t)mp; const char* key = (const char*)(intptr_t)k; for (long i = 0; i < m->len; i++) { if (strcmp((const char*)(intptr_t)m->keys[i], key) == 0) return i; } return -1; }");
  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long Map() { SMap* m = (SMap*)malloc(sizeof(SMap)); m->cap = 8; m->len = 0; m->keys = (long*)malloc(sizeof(long) * m->cap); m->vals = (long*)malloc(sizeof(long) * m->cap); return (long)(intptr_t)m; }");
  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long m_set(long mp, long k, long v) { SMap* m = (SMap*)(intptr_t)mp; long i = m_find(mp, k); if (i >= 0) { m->vals[i] = v; return 0; } if (m->len == m->cap) { m->cap = m->cap * 2; m->keys = (long*)realloc(m->keys, sizeof(long) * m->cap); m->vals = (long*)realloc(m->vals, sizeof(long) * m->cap); } m->keys[m->len] = k; m->vals[m->len] = v; m->len = m->len + 1; return 0; }");
  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long m_get(long mp, long k) { long i = m_find(mp, k); if (i >= 0) return ((SMap*)(intptr_t)mp)->vals[i]; return 0; }");
  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long m_gets(long mp, long k) { long i = m_find(mp, k); if (i >= 0) return ((SMap*)(intptr_t)mp)->vals[i]; return (long)(intptr_t)\"\"; }");
  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long m_has(long mp, long k) { return m_find(mp, k) >= 0; }");
  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long m_keys(long mp) { SMap* m = (SMap*)(intptr_t)mp; long l = l_new(); for (long i = 0; i < m->len; i++) l_push(l, m->keys[i]); return l; }");
  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long m_byvalue(long mp) { SMap* m = (SMap*)(intptr_t)mp; long n = m->len; long c = n > 0 ? n : 1; long* idx = (long*)malloc(sizeof(long) * c); for (long i = 0; i < n; i++) idx[i] = i; for (long i = 1; i < n; i++) { long t = idx[i]; long j = i - 1; while (j >= 0 && m->vals[idx[j]] < m->vals[t]) { idx[j + 1] = idx[j]; j = j - 1; } idx[j + 1] = t; } long l = l_new(); for (long i = 0; i < n; i++) l_push(l, m->keys[idx[i]]); return l; }");
  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"int cmp_long(const void* a, const void* b) { long x = *(const long*)a; long y = *(const long*)b; if (x < y) return -1; if (x > y) return 1; return 0; }");
  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"int cmp_str(const void* a, const void* b) { return strcmp((const char*)(intptr_t)(*(const long*)a), (const char*)(intptr_t)(*(const long*)b)); }");
  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long l_sort(long lp, long isStr) { SList* s = (SList*)(intptr_t)lp; long n = s->len; long c = n > 0 ? n : 1; long* d = (long*)malloc(sizeof(long) * c); for (long i = 0; i < n; i++) d[i] = s->data[i]; qsort(d, n, sizeof(long), isStr ? cmp_str : cmp_long); long r = l_new(); for (long i = 0; i < n; i++) l_push(r, d[i]); return r; }");
  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"const char* simpler_read(const char* path) { FILE* f = fopen(path, \"rb\"); if (!f) return \"\"; fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET); char* buf = (char*)malloc(n + 1); long got = fread(buf, 1, n, f); buf[got] = 0; fclose(f); return buf; }");
  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long simpler_write(long path, long content) { FILE* f = fopen((const char*)(intptr_t)path, \"w\"); if (f) { fputs((const char*)(intptr_t)content, f); fclose(f); } return 0; }");
  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long simpler_args(int argc, char** argv) { long l = l_new(); for (int k = 1; k < argc; k++) l_push(l, (long)(intptr_t)argv[k]); return l; }");
  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long simpler_stdin() { long cap = 1024; long len = 0; char* buf = (char*)malloc(cap); int c; while ((c = getchar()) != EOF) { if (len + 1 >= cap) { cap = cap * 2; buf = (char*)realloc(buf, cap); } buf[len] = (char)c; len = len + 1; } buf[len] = 0; return (long)(intptr_t)buf; }");
  printf("%s\n", (const char*)(intptr_t)(long)(intptr_t)"long fail(long msg) { fprintf(stderr, \"%s\\n\", (const char*)(intptr_t)msg); exit(1); return 0; }");
  {
  long _lst = ((ProgT*)(intptr_t)prog)->types;
  long _n = l_len(_lst);
  for (long _i = 0; _i < _n; _i = _i + 1) {
  long t = l_at(_lst, _i);
  printf("%s\n", (const char*)(intptr_t)emitType(t));
  }
  }
  {
  long _lst = ((ProgT*)(intptr_t)prog)->records;
  long _n = l_len(_lst);
  for (long _i = 0; _i < _n; _i = _i + 1) {
  long r = l_at(_lst, _i);
  printf("%s\n", (const char*)(intptr_t)emitRecord(r));
  }
  }
  {
  long _lst = ((ProgT*)(intptr_t)prog)->fns;
  long _n = l_len(_lst);
  for (long _i = 0; _i < _n; _i = _i + 1) {
  long f = l_at(_lst, _i);
  if ((!isMain(((FnT*)(intptr_t)f)->name))) {
  printf("%s\n", (const char*)(intptr_t)emitProto(f));
  }
  }
  }
  {
  long _lst = ((ProgT*)(intptr_t)prog)->fns;
  long _n = l_len(_lst);
  for (long _i = 0; _i < _n; _i = _i + 1) {
  long f = l_at(_lst, _i);
  printf("%s\n", (const char*)(intptr_t)emitFn(f, sigs));
  }
  }
  return 0;
}
long buildSigs(long prog) {
  long recNames = 0;
  long fnNames = 0;
  long fnRets = 0;
  long boxedNullary = 0;
  recNames = l_new();
  {
  long _lst = ((ProgT*)(intptr_t)prog)->records;
  long _n = l_len(_lst);
  for (long _i = 0; _i < _n; _i = _i + 1) {
  long r = l_at(_lst, _i);
  l_push(recNames, ((RecDefT*)(intptr_t)r)->name);
  }
  }
  fnNames = l_new();
  fnRets = l_new();
  {
  long _lst = ((ProgT*)(intptr_t)prog)->fns;
  long _n = l_len(_lst);
  for (long _i = 0; _i < _n; _i = _i + 1) {
  long f = l_at(_lst, _i);
  l_push(fnNames, ((FnT*)(intptr_t)f)->name);
  l_push(fnRets, ((FnT*)(intptr_t)f)->ret);
  }
  }
  boxedNullary = l_new();
  {
  long _lst = ((ProgT*)(intptr_t)prog)->types;
  long _n = l_len(_lst);
  for (long _i = 0; _i < _n; _i = _i + 1) {
  long t = l_at(_lst, _i);
  if (hasPayload(t)) {
  {
  long _lst = ((TyDefT*)(intptr_t)t)->cases;
  long _n = l_len(_lst);
  for (long _i = 0; _i < _n; _i = _i + 1) {
  long c = l_at(_lst, _i);
  if ((((CaseT*)(intptr_t)c)->arity == 0L)) {
  l_push(boxedNullary, ((CaseT*)(intptr_t)c)->cname);
  }
  }
  }
  }
  }
  }
  return Sigs(recNames, ((ProgT*)(intptr_t)prog)->records, ((ProgT*)(intptr_t)prog)->types, ((ProgT*)(intptr_t)prog)->fns, fnNames, fnRets, boxedNullary);
}
long parse(long toks, long lines) {
  long types = 0;
  long records = 0;
  long fns = 0;
  long i = 0;
  long pr = 0;
  long pt = 0;
  long pf = 0;
  types = l_new();
  records = l_new();
  fns = l_new();
  i = 0L;
  while (notEof(toks, i)) {
  if (isTypeDef(toks, i)) {
  if (isRecordDef(toks, i)) {
  pr = parseRecord(toks, i);
  l_push(records, ((PRecT*)(intptr_t)pr)->rec);
  i = ((PRecT*)(intptr_t)pr)->next;
  } else {
  pt = parseTypeDef(toks, i);
  l_push(types, ((PTyT*)(intptr_t)pt)->ty);
  i = ((PTyT*)(intptr_t)pt)->next;
  }
  } else {
  pf = parseFn(toks, lines, i);
  l_push(fns, ((PFnT*)(intptr_t)pf)->fn);
  i = ((PFnT*)(intptr_t)pf)->next;
  }
  }
  return Prog(types, records, fns);
}
long isTypeDef(long toks, long i) {
  return (isIdent(toks, i) && isPunct(toks, (i + 1L), (long)(intptr_t)"="));
}
long isRecordDef(long toks, long i) {
  return isPunct(toks, (i + 5L), (long)(intptr_t)":");
}
long parseRecord(long toks, long i) {
  long name = 0;
  long fields = 0;
  long ftypes = 0;
  long j = 0;
  long ty = 0;
  name = identAt(toks, i);
  fields = l_new();
  ftypes = l_new();
  j = (i + 4L);
  while (inBlock(toks, j)) {
  l_push(fields, identAt(toks, j));
  j = (j + 1L);
  ty = (long)(intptr_t)"Int";
  if (isPunct(toks, j, (long)(intptr_t)":")) {
  ty = identAt(toks, (j + 1L));
  j = (j + 2L);
  if (isPunct(toks, j, (long)(intptr_t)"[")) {
  ty = s_concat(s_concat(s_concat(ty, (long)(intptr_t)"["), identAt(toks, (j + 1L))), (long)(intptr_t)"]");
  while (((!isPunct(toks, j, (long)(intptr_t)"]")) && notEof(toks, j))) {
  j = (j + 1L);
  }
  j = (j + 1L);
  }
  }
  l_push(ftypes, ty);
  if (isPunct(toks, j, (long)(intptr_t)",")) {
  j = (j + 1L);
  }
  }
  return PRec(RecDef(name, fields, ftypes), (j + 1L));
}
long parseTypeDef(long toks, long i) {
  long name = 0;
  long cases = 0;
  long j = 0;
  long cn = 0;
  long arity = 0;
  long ptypes = 0;
  long pt = 0;
  name = identAt(toks, i);
  cases = l_new();
  j = (i + 4L);
  while (inBlock(toks, j)) {
  cn = identAt(toks, j);
  j = (j + 1L);
  arity = 0L;
  ptypes = l_new();
  if (isPunct(toks, j, (long)(intptr_t)"(")) {
  j = (j + 1L);
  while (isIdent(toks, j)) {
  pt = identAt(toks, j);
  arity = (arity + 1L);
  j = (j + 1L);
  if (isPunct(toks, j, (long)(intptr_t)"[")) {
  pt = s_concat(s_concat(s_concat(pt, (long)(intptr_t)"["), identAt(toks, (j + 1L))), (long)(intptr_t)"]");
  while (((!isPunct(toks, j, (long)(intptr_t)"]")) && notEof(toks, j))) {
  j = (j + 1L);
  }
  j = (j + 1L);
  }
  l_push(ptypes, pt);
  if (isPunct(toks, j, (long)(intptr_t)",")) {
  j = (j + 1L);
  }
  }
  j = (j + 1L);
  }
  l_push(cases, Case(cn, arity, ptypes));
  }
  return PTy(TyDef(name, cases), (j + 1L));
}
long parseFn(long toks, long lines, long i) {
  long name = 0;
  long pp = 0;
  long rt = 0;
  long pb = 0;
  name = identAt(toks, i);
  pp = parseParams(toks, (i + 1L));
  rt = parseRet(toks, ((PNamesT*)(intptr_t)pp)->next);
  pb = parseBlock(toks, ((PRetT*)(intptr_t)rt)->next);
  return PFn(Fn(name, ((PNamesT*)(intptr_t)pp)->names, ((PNamesT*)(intptr_t)pp)->types, ((PRetT*)(intptr_t)rt)->ret, ((PRetT*)(intptr_t)rt)->effs, ((PBodyT*)(intptr_t)pb)->body, l_at(lines, i)), ((PBodyT*)(intptr_t)pb)->next);
}
long parseParams(long toks, long i) {
  long names = 0;
  long types = 0;
  long j = 0;
  long ty = 0;
  names = l_new();
  types = l_new();
  j = (i + 1L);
  while (isIdent(toks, j)) {
  l_push(names, identAt(toks, j));
  j = (j + 1L);
  ty = (long)(intptr_t)"";
  if (isPunct(toks, j, (long)(intptr_t)":")) {
  ty = identAt(toks, (j + 1L));
  j = (j + 2L);
  if (isPunct(toks, j, (long)(intptr_t)"[")) {
  ty = s_concat(s_concat(s_concat(ty, (long)(intptr_t)"["), identAt(toks, (j + 1L))), (long)(intptr_t)"]");
  while (((!isPunct(toks, j, (long)(intptr_t)"]")) && notEof(toks, j))) {
  j = (j + 1L);
  }
  j = (j + 1L);
  }
  }
  l_push(types, ty);
  if (isPunct(toks, j, (long)(intptr_t)",")) {
  j = (j + 1L);
  }
  }
  return PNames(names, types, (j + 1L));
}
long parseRet(long toks, long j) {
  long ret = 0;
  long effs = 0;
  long k = 0;
  ret = (long)(intptr_t)"Int";
  effs = l_new();
  k = j;
  if (isPunct(toks, k, (long)(intptr_t)":")) {
  ret = identAt(toks, (k + 1L));
  k = (k + 2L);
  }
  while (((!isPunct(toks, k, (long)(intptr_t)"{")) && notEof(toks, k))) {
  if (isPunct(toks, k, (long)(intptr_t)"!")) {
  l_push(effs, identAt(toks, (k + 1L)));
  }
  k = (k + 1L);
  }
  return PRet(ret, effs, k);
}
long parseBlock(long toks, long i) {
  long body = 0;
  long j = 0;
  long p = 0;
  body = l_new();
  j = (i + 1L);
  while (inBlock(toks, j)) {
  p = parseStmt(toks, j);
  l_push(body, ((PStmtT*)(intptr_t)p)->node);
  j = ((PStmtT*)(intptr_t)p)->next;
  }
  return PBody(body, (j + 1L));
}
long parseStmt(long toks, long i) {
  long e = 0;
  long r = 0;
  long name = 0;
  long ae = 0;
  e = parseExpr(toks, i);
  r = PStmt(Bare(((ParsedT*)(intptr_t)e)->node), ((ParsedT*)(intptr_t)e)->next);
  if (isAssign(toks, i)) {
  name = identAt(toks, i);
  ae = parseExpr(toks, (i + 2L));
  r = PStmt(Let(name, (long)(intptr_t)"", ((ParsedT*)(intptr_t)ae)->node), ((ParsedT*)(intptr_t)ae)->next);
  }
  if (isTypedAssign(toks, i)) {
  r = parseTypedLet(toks, i);
  }
  if (isWord(toks, i, (long)(intptr_t)"if")) {
  r = parseIf(toks, i);
  }
  if (isWord(toks, i, (long)(intptr_t)"while")) {
  r = parseWhile(toks, i);
  }
  return r;
}
long isTypedAssign(long toks, long i) {
  return (isIdent(toks, i) && isPunct(toks, (i + 1L), (long)(intptr_t)":"));
}
long parseTypedLet(long toks, long i) {
  long name = 0;
  long ty = 0;
  long k = 0;
  long ae = 0;
  name = identAt(toks, i);
  ty = identAt(toks, (i + 2L));
  if (isPunct(toks, (i + 3L), (long)(intptr_t)"[")) {
  ty = s_concat(s_concat(s_concat(ty, (long)(intptr_t)"["), identAt(toks, (i + 4L))), (long)(intptr_t)"]");
  }
  k = (i + 1L);
  while (((!isPunct(toks, k, (long)(intptr_t)"=")) && notEof(toks, k))) {
  k = (k + 1L);
  }
  ae = parseExpr(toks, (k + 1L));
  return PStmt(Let(name, ty, ((ParsedT*)(intptr_t)ae)->node), ((ParsedT*)(intptr_t)ae)->next);
}
long parseIf(long toks, long i) {
  long c = 0;
  long tb = 0;
  long els = 0;
  long nxt = 0;
  long eb = 0;
  c = parseExpr(toks, (i + 1L));
  tb = parseBlock(toks, ((ParsedT*)(intptr_t)c)->next);
  els = l_new();
  nxt = ((PBodyT*)(intptr_t)tb)->next;
  if (isWord(toks, ((PBodyT*)(intptr_t)tb)->next, (long)(intptr_t)"else")) {
  eb = parseBlock(toks, (((PBodyT*)(intptr_t)tb)->next + 1L));
  els = ((PBodyT*)(intptr_t)eb)->body;
  nxt = ((PBodyT*)(intptr_t)eb)->next;
  }
  return PStmt(If(((ParsedT*)(intptr_t)c)->node, ((PBodyT*)(intptr_t)tb)->body, els), nxt);
}
long parseWhile(long toks, long i) {
  long c = 0;
  long b = 0;
  c = parseExpr(toks, (i + 1L));
  b = parseBlock(toks, ((ParsedT*)(intptr_t)c)->next);
  return PStmt(While(((ParsedT*)(intptr_t)c)->node, ((PBodyT*)(intptr_t)b)->body), ((PBodyT*)(intptr_t)b)->next);
}
long parseExpr(long toks, long pos) {
  long p = 0;
  long node = 0;
  long i = 0;
  long op = 0;
  long r = 0;
  p = parseAdd(toks, pos);
  node = ((ParsedT*)(intptr_t)p)->node;
  i = ((ParsedT*)(intptr_t)p)->next;
  while (isCmpOp(toks, i)) {
  op = punctAt(toks, i);
  r = parseAdd(toks, (i + 1L));
  node = Bin(op, node, ((ParsedT*)(intptr_t)r)->node);
  i = ((ParsedT*)(intptr_t)r)->next;
  }
  return Parsed(node, i);
}
long parseAdd(long toks, long pos) {
  long p = 0;
  long node = 0;
  long i = 0;
  long op = 0;
  long r = 0;
  p = parseTerm(toks, pos);
  node = ((ParsedT*)(intptr_t)p)->node;
  i = ((ParsedT*)(intptr_t)p)->next;
  while (isAddOp(toks, i)) {
  op = punctAt(toks, i);
  r = parseTerm(toks, (i + 1L));
  node = Bin(op, node, ((ParsedT*)(intptr_t)r)->node);
  i = ((ParsedT*)(intptr_t)r)->next;
  }
  return Parsed(node, i);
}
long parseTerm(long toks, long pos) {
  long p = 0;
  long node = 0;
  long i = 0;
  long op = 0;
  long r = 0;
  p = parsePostfix(toks, pos);
  node = ((ParsedT*)(intptr_t)p)->node;
  i = ((ParsedT*)(intptr_t)p)->next;
  while (isMulOp(toks, i)) {
  op = punctAt(toks, i);
  r = parsePostfix(toks, (i + 1L));
  node = Bin(op, node, ((ParsedT*)(intptr_t)r)->node);
  i = ((ParsedT*)(intptr_t)r)->next;
  }
  return Parsed(node, i);
}
long parsePostfix(long toks, long pos) {
  long p = 0;
  long node = 0;
  long i = 0;
  long pm = 0;
  long pe = 0;
  long name = 0;
  long pa = 0;
  p = parseFactor(toks, pos);
  node = ((ParsedT*)(intptr_t)p)->node;
  i = ((ParsedT*)(intptr_t)p)->next;
  while (isPunct(toks, i, (long)(intptr_t)".")) {
  if (isWord(toks, (i + 1L), (long)(intptr_t)"match")) {
  pm = parseMatch(toks, node, i);
  node = ((ParsedT*)(intptr_t)pm)->node;
  i = ((ParsedT*)(intptr_t)pm)->next;
  } else {
  if (isWord(toks, (i + 1L), (long)(intptr_t)"each")) {
  pe = parseEach(toks, node, i);
  node = ((ParsedT*)(intptr_t)pe)->node;
  i = ((ParsedT*)(intptr_t)pe)->next;
  } else {
  name = identAt(toks, (i + 1L));
  if (isPunct(toks, (i + 2L), (long)(intptr_t)"(")) {
  pa = parseArgs(toks, (i + 2L));
  node = Method(node, name, ((PArgsT*)(intptr_t)pa)->list);
  i = ((PArgsT*)(intptr_t)pa)->next;
  } else {
  node = Field(node, name);
  i = (i + 2L);
  }
  }
  }
  }
  if (isPunct(toks, i, (long)(intptr_t)"?")) {
  i = (i + 1L);
  }
  return Parsed(node, i);
}
long parseEach(long toks, long recv, long i) {
  long param = 0;
  long body = 0;
  long j = 0;
  long p = 0;
  param = identAt(toks, (i + 3L));
  body = l_new();
  j = (i + 5L);
  while (inBlock(toks, j)) {
  p = parseStmt(toks, j);
  l_push(body, ((PStmtT*)(intptr_t)p)->node);
  j = ((PStmtT*)(intptr_t)p)->next;
  }
  return Parsed(Each(recv, param, body), (j + 1L));
}
long parseFactor(long toks, long pos) {
  switch (((Obj*)(intptr_t)l_at(toks, pos))->tag) {
  case T_Int: { long v = ((Obj*)(intptr_t)l_at(toks, pos))->v0; return Parsed(Num(v), (pos + 1L)); }
  case T_Float: { long s = ((Obj*)(intptr_t)l_at(toks, pos))->v0; return Parsed(FloatLit(s), (pos + 1L)); }
  case T_Str: { long s = ((Obj*)(intptr_t)l_at(toks, pos))->v0; return Parsed(StrLit(s), (pos + 1L)); }
  case T_Ident: { long s = ((Obj*)(intptr_t)l_at(toks, pos))->v0; return parseIdentFactor(toks, pos); }
  case T_Punct: { long p = ((Obj*)(intptr_t)l_at(toks, pos))->v0; return parsePunctFactor(toks, pos); }
  case T_Eof: { return Parsed(Num(0L), pos); }
  }
  return 0;
}
long parsePunctFactor(long toks, long pos) {
  long r = 0;
  long f = 0;
  r = parseParen(toks, pos);
  if (isPunct(toks, pos, (long)(intptr_t)"[")) {
  r = parseList(toks, pos);
  }
  if (isPunct(toks, pos, (long)(intptr_t)"-")) {
  f = parseFactor(toks, (pos + 1L));
  r = Parsed(Bin((long)(intptr_t)"u-", Num(0L), ((ParsedT*)(intptr_t)f)->node), ((ParsedT*)(intptr_t)f)->next);
  }
  return r;
}
long parseList(long toks, long pos) {
  long elems = 0;
  long j = 0;
  long e = 0;
  elems = l_new();
  j = (pos + 1L);
  while (((!isPunct(toks, j, (long)(intptr_t)"]")) && notEof(toks, j))) {
  e = parseExpr(toks, j);
  l_push(elems, ((ParsedT*)(intptr_t)e)->node);
  j = ((ParsedT*)(intptr_t)e)->next;
  if (isPunct(toks, j, (long)(intptr_t)",")) {
  j = (j + 1L);
  }
  }
  return Parsed(ListLit(elems), (j + 1L));
}
long parseIdentFactor(long toks, long pos) {
  long name = 0;
  long r = 0;
  long pa = 0;
  name = identAt(toks, pos);
  r = Parsed(Var(name), (pos + 1L));
  if (isPunct(toks, (pos + 1L), (long)(intptr_t)"(")) {
  pa = parseArgs(toks, (pos + 1L));
  r = Parsed(Call(name, ((PArgsT*)(intptr_t)pa)->list), ((PArgsT*)(intptr_t)pa)->next);
  }
  return r;
}
long parseParen(long toks, long pos) {
  long inner = 0;
  inner = parseExpr(toks, (pos + 1L));
  return Parsed(((ParsedT*)(intptr_t)inner)->node, (((ParsedT*)(intptr_t)inner)->next + 1L));
}
long parseArgs(long toks, long i) {
  long list = 0;
  long j = 0;
  long e = 0;
  list = l_new();
  j = (i + 1L);
  while (inArgs(toks, j)) {
  if (isAssign(toks, j)) {
  j = (j + 2L);
  }
  e = parseExpr(toks, j);
  l_push(list, ((ParsedT*)(intptr_t)e)->node);
  j = ((ParsedT*)(intptr_t)e)->next;
  if (isPunct(toks, j, (long)(intptr_t)",")) {
  j = (j + 1L);
  }
  }
  return PArgs(list, (j + 1L));
}
long parseMatch(long toks, long scrut, long i) {
  long arms = 0;
  long j = 0;
  long pa = 0;
  arms = l_new();
  j = (i + 3L);
  while (inBlock(toks, j)) {
  pa = parseArm(toks, j);
  l_push(arms, ((PArmT*)(intptr_t)pa)->arm);
  j = ((PArmT*)(intptr_t)pa)->next;
  }
  return Parsed(Match(scrut, arms), (j + 1L));
}
long parseArm(long toks, long j) {
  long tag = 0;
  long binds = 0;
  long k = 0;
  long ae = 0;
  tag = identAt(toks, j);
  binds = l_new();
  k = (j + 1L);
  if (isPunct(toks, k, (long)(intptr_t)"(")) {
  k = (k + 1L);
  while (isIdent(toks, k)) {
  l_push(binds, identAt(toks, k));
  k = (k + 1L);
  if (isPunct(toks, k, (long)(intptr_t)",")) {
  k = (k + 1L);
  }
  }
  k = (k + 1L);
  }
  ae = parseExpr(toks, (k + 1L));
  return PArm(Arm(tag, binds, ((ParsedT*)(intptr_t)ae)->node), ((ParsedT*)(intptr_t)ae)->next);
}
long emitType(long t) {
  long r = 0;
  r = emitEnumType(t);
  if (hasPayload(t)) {
  r = emitBoxedType(t);
  }
  return r;
}
long emitEnumType(long t) {
  long out = 0;
  long k = 0;
  long m = 0;
  out = (long)(intptr_t)"enum {";
  k = 0L;
  m = l_len(((TyDefT*)(intptr_t)t)->cases);
  while ((k < m)) {
  if ((k > 0L)) {
  out = s_concat(out, (long)(intptr_t)",");
  }
  out = s_concat(s_concat(out, (long)(intptr_t)" "), ((CaseT*)(intptr_t)l_at(((TyDefT*)(intptr_t)t)->cases, k))->cname);
  k = (k + 1L);
  }
  return s_concat(out, (long)(intptr_t)" };");
}
long emitBoxedType(long t) {
  long out = 0;
  long k = 0;
  long m = 0;
  long cn = 0;
  long ar = 0;
  out = (long)(intptr_t)"enum {";
  k = 0L;
  m = l_len(((TyDefT*)(intptr_t)t)->cases);
  while ((k < m)) {
  if ((k > 0L)) {
  out = s_concat(out, (long)(intptr_t)",");
  }
  out = s_concat(s_concat(out, (long)(intptr_t)" T_"), ((CaseT*)(intptr_t)l_at(((TyDefT*)(intptr_t)t)->cases, k))->cname);
  k = (k + 1L);
  }
  out = s_concat(out, (long)(intptr_t)" };");
  k = 0L;
  while ((k < m)) {
  cn = ((CaseT*)(intptr_t)l_at(((TyDefT*)(intptr_t)t)->cases, k))->cname;
  ar = ((CaseT*)(intptr_t)l_at(((TyDefT*)(intptr_t)t)->cases, k))->arity;
  out = s_concat(s_concat(s_concat(s_concat(s_concat(s_concat(s_concat(s_concat(s_concat(out, (long)(intptr_t)"\nlong "), cn), (long)(intptr_t)"("), ctorParams(ar)), (long)(intptr_t)") { return mk(T_"), cn), (long)(intptr_t)", "), ctorArgs(ar)), (long)(intptr_t)"); }");
  k = (k + 1L);
  }
  return out;
}
long hasPayload(long t) {
  long has = 0;
  long k = 0;
  long m = 0;
  has = false;
  k = 0L;
  m = l_len(((TyDefT*)(intptr_t)t)->cases);
  while ((k < m)) {
  if ((((CaseT*)(intptr_t)l_at(((TyDefT*)(intptr_t)t)->cases, k))->arity > 0L)) {
  has = true;
  }
  k = (k + 1L);
  }
  return has;
}
long emitRecord(long rec) {
  long out = 0;
  long k = 0;
  long m = 0;
  long fn = 0;
  out = (long)(intptr_t)"typedef struct {";
  k = 0L;
  m = l_len(((RecDefT*)(intptr_t)rec)->fields);
  while ((k < m)) {
  out = s_concat(s_concat(s_concat(out, (long)(intptr_t)" long "), l_at(((RecDefT*)(intptr_t)rec)->fields, k)), (long)(intptr_t)";");
  k = (k + 1L);
  }
  out = s_concat(s_concat(s_concat(out, (long)(intptr_t)" } "), ((RecDefT*)(intptr_t)rec)->name), (long)(intptr_t)"T;");
  out = s_concat(s_concat(s_concat(s_concat(s_concat(s_concat(s_concat(s_concat(s_concat(out, (long)(intptr_t)"\nlong "), ((RecDefT*)(intptr_t)rec)->name), (long)(intptr_t)"("), recParams(rec)), (long)(intptr_t)") { "), ((RecDefT*)(intptr_t)rec)->name), (long)(intptr_t)"T* o = malloc(sizeof("), ((RecDefT*)(intptr_t)rec)->name), (long)(intptr_t)"T));");
  k = 0L;
  while ((k < m)) {
  fn = l_at(((RecDefT*)(intptr_t)rec)->fields, k);
  out = s_concat(s_concat(s_concat(s_concat(s_concat(out, (long)(intptr_t)" o->"), fn), (long)(intptr_t)" = "), fn), (long)(intptr_t)";");
  k = (k + 1L);
  }
  return s_concat(out, (long)(intptr_t)" return (long)(intptr_t)o; }");
}
long recParams(long rec) {
  long out = 0;
  long k = 0;
  long m = 0;
  out = (long)(intptr_t)"";
  k = 0L;
  m = l_len(((RecDefT*)(intptr_t)rec)->fields);
  while ((k < m)) {
  if ((k > 0L)) {
  out = s_concat(out, (long)(intptr_t)", ");
  }
  out = s_concat(s_concat(out, (long)(intptr_t)"long "), l_at(((RecDefT*)(intptr_t)rec)->fields, k));
  k = (k + 1L);
  }
  return out;
}
long slotName(long bi) {
  return s_concat((long)(intptr_t)"v", i_tostr(bi));
}
long ctorParams(long arity) {
  long out = 0;
  long k = 0;
  out = (long)(intptr_t)"";
  k = 0L;
  while ((k < arity)) {
  if ((k > 0L)) {
  out = s_concat(out, (long)(intptr_t)", ");
  }
  out = s_concat(s_concat(out, (long)(intptr_t)"long "), slotName(k));
  k = (k + 1L);
  }
  return out;
}
long ctorArgs(long arity) {
  long out = 0;
  long k = 0;
  long slot = 0;
  out = (long)(intptr_t)"";
  k = 0L;
  while ((k < 3L)) {
  if ((k > 0L)) {
  out = s_concat(out, (long)(intptr_t)", ");
  }
  slot = (long)(intptr_t)"0";
  if ((k < arity)) {
  slot = slotName(k);
  }
  out = s_concat(out, slot);
  k = (k + 1L);
  }
  return out;
}
long emitProto(long f) {
  return s_concat(s_concat(s_concat(s_concat((long)(intptr_t)"long ", ((FnT*)(intptr_t)f)->name), (long)(intptr_t)"("), paramDecls(f)), (long)(intptr_t)");");
}
long emitFn(long f, long sigs) {
  long rt = 0;
  long sig = 0;
  long out = 0;
  long env = 0;
  long pk = 0;
  long pn = 0;
  long ctx = 0;
  long decls = 0;
  long d = 0;
  long dn = 0;
  long k = 0;
  long m = 0;
  long last = 0;
  rt = (long)(intptr_t)"long";
  sig = paramDecls(f);
  if (isMain(((FnT*)(intptr_t)f)->name)) {
  rt = (long)(intptr_t)"int";
  sig = (long)(intptr_t)"int argc, char** argv";
  }
  out = s_concat(s_concat(s_concat(s_concat(s_concat(rt, (long)(intptr_t)" "), ((FnT*)(intptr_t)f)->name), (long)(intptr_t)"("), sig), (long)(intptr_t)") {");
  env = newEnv();
  pk = 0L;
  pn = l_len(((FnT*)(intptr_t)f)->params);
  while ((pk < pn)) {
  envPut(env, l_at(((FnT*)(intptr_t)f)->params, pk), l_at(((FnT*)(intptr_t)f)->ptypes, pk));
  pk = (pk + 1L);
  }
  ctx = Ctx(env, sigs);
  envPut(env, (long)(intptr_t)"__line__", i_tostr(((FnT*)(intptr_t)f)->line));
  seedEnv(((FnT*)(intptr_t)f)->body, ctx);
  checkReturn(f, ctx);
  checkEffects(f, ctx);
  decls = collectLets(((FnT*)(intptr_t)f)->body);
  d = 0L;
  dn = l_len(decls);
  while ((d < dn)) {
  out = s_concat(s_concat(s_concat(out, (long)(intptr_t)"\n  long "), l_at(decls, d)), (long)(intptr_t)" = 0;");
  d = (d + 1L);
  }
  k = 0L;
  m = l_len(((FnT*)(intptr_t)f)->body);
  while ((k < m)) {
  last = (((k + 1L) == m) && (!isMain(((FnT*)(intptr_t)f)->name)));
  out = s_concat(s_concat(out, (long)(intptr_t)"\n"), emitStmt(l_at(((FnT*)(intptr_t)f)->body, k), last, ctx));
  k = (k + 1L);
  }
  if (isMain(((FnT*)(intptr_t)f)->name)) {
  out = s_concat(out, (long)(intptr_t)"\n  return 0;");
  }
  return s_concat(out, (long)(intptr_t)"\n}");
}
long paramDecls(long f) {
  long out = 0;
  long k = 0;
  long m = 0;
  out = (long)(intptr_t)"";
  k = 0L;
  m = l_len(((FnT*)(intptr_t)f)->params);
  while ((k < m)) {
  if ((k > 0L)) {
  out = s_concat(out, (long)(intptr_t)", ");
  }
  out = s_concat(s_concat(out, (long)(intptr_t)"long "), l_at(((FnT*)(intptr_t)f)->params, k));
  k = (k + 1L);
  }
  return out;
}
long newEnv() {
  long ns = 0;
  long ts = 0;
  ns = l_new();
  ts = l_new();
  return TyEnv(ns, ts);
}
long envPut(long env, long name, long ty) {
  l_push(((TyEnvT*)(intptr_t)env)->names, name);
  l_push(((TyEnvT*)(intptr_t)env)->tys, ty);
  return 0L;
}
long envGet(long env, long name) {
  long r = 0;
  long k = 0;
  long m = 0;
  r = (long)(intptr_t)"Int";
  k = 0L;
  m = l_len(((TyEnvT*)(intptr_t)env)->names);
  while ((k < m)) {
  if (s_eq(l_at(((TyEnvT*)(intptr_t)env)->names, k), name)) {
  r = l_at(((TyEnvT*)(intptr_t)env)->tys, k);
  }
  k = (k + 1L);
  }
  return r;
}
long seedEnv(long body, long ctx) {
  long k = 0;
  long m = 0;
  k = 0L;
  m = l_len(body);
  while ((k < m)) {
  seedStmt(l_at(body, k), ctx);
  k = (k + 1L);
  }
  return 0L;
}
long seedStmt(long s, long ctx) {
  switch (((Obj*)(intptr_t)s)->tag) {
  case T_Let: { long name = ((Obj*)(intptr_t)s)->v0; long ty = ((Obj*)(intptr_t)s)->v1; long e = ((Obj*)(intptr_t)s)->v2; return envPut(((CtxT*)(intptr_t)ctx)->env, name, letType(ty, e, ctx)); }
  case T_Bare: { long e = ((Obj*)(intptr_t)s)->v0; return seedExpr(e, ctx); }
  case T_If: { long c = ((Obj*)(intptr_t)s)->v0; long t = ((Obj*)(intptr_t)s)->v1; long el = ((Obj*)(intptr_t)s)->v2; return seedIf(t, el, ctx); }
  case T_While: { long c = ((Obj*)(intptr_t)s)->v0; long b = ((Obj*)(intptr_t)s)->v1; return seedEnv(b, ctx); }
  }
  return 0;
}
long seedIf(long t, long el, long ctx) {
  seedEnv(t, ctx);
  seedEnv(el, ctx);
  return 0L;
}
long seedExpr(long e, long ctx) {
  switch (((Obj*)(intptr_t)e)->tag) {
  case T_Each: { long recv = ((Obj*)(intptr_t)e)->v0; long param = ((Obj*)(intptr_t)e)->v1; long body = ((Obj*)(intptr_t)e)->v2; return seedEach(recv, param, body, ctx); }
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
}
long seedEach(long recv, long param, long body, long ctx) {
  envPut(((CtxT*)(intptr_t)ctx)->env, param, elemOf(exprType(recv, ctx)));
  seedEnv(body, ctx);
  return 0L;
}
long inferType(long e, long ctx) {
  return exprType(e, ctx);
}
long letType(long ty, long e, long ctx) {
  long r = 0;
  r = inferType(e, ctx);
  if ((!s_eq(ty, (long)(intptr_t)""))) {
  r = ty;
  }
  return r;
}
long callRet(long name, long ctx) {
  long r = 0;
  long vt = 0;
  long k = 0;
  long m = 0;
  r = (long)(intptr_t)"Int";
  if (s_eq(name, (long)(intptr_t)"Map")) {
  r = (long)(intptr_t)"Map";
  }
  if (hasName(((SigsT*)(intptr_t)((CtxT*)(intptr_t)ctx)->sigs)->recNames, name)) {
  r = name;
  }
  vt = variantOf(name, ctx);
  if ((!s_eq(vt, (long)(intptr_t)""))) {
  r = vt;
  }
  k = 0L;
  m = l_len(((SigsT*)(intptr_t)((CtxT*)(intptr_t)ctx)->sigs)->fnNames);
  while ((k < m)) {
  if (s_eq(l_at(((SigsT*)(intptr_t)((CtxT*)(intptr_t)ctx)->sigs)->fnNames, k), name)) {
  r = l_at(((SigsT*)(intptr_t)((CtxT*)(intptr_t)ctx)->sigs)->fnRets, k);
  }
  k = (k + 1L);
  }
  return r;
}
long variantOf(long name, long ctx) {
  long r = 0;
  long types = 0;
  long k = 0;
  long m = 0;
  long t = 0;
  r = (long)(intptr_t)"";
  types = ((SigsT*)(intptr_t)((CtxT*)(intptr_t)ctx)->sigs)->types;
  k = 0L;
  m = l_len(types);
  while ((k < m)) {
  t = l_at(types, k);
  if (hasCase(t, name)) {
  r = ((TyDefT*)(intptr_t)t)->name;
  }
  k = (k + 1L);
  }
  return r;
}
long hasCase(long t, long name) {
  long found = 0;
  long k = 0;
  long m = 0;
  found = false;
  k = 0L;
  m = l_len(((TyDefT*)(intptr_t)t)->cases);
  while ((k < m)) {
  if (s_eq(((CaseT*)(intptr_t)l_at(((TyDefT*)(intptr_t)t)->cases, k))->cname, name)) {
  found = true;
  }
  k = (k + 1L);
  }
  return found;
}
long isRec(long ty, long ctx) {
  return hasName(((SigsT*)(intptr_t)((CtxT*)(intptr_t)ctx)->sigs)->recNames, ty);
}
long collectLets(long body) {
  long names = 0;
  names = l_new();
  collectBody(body, names);
  return names;
}
long collectBody(long body, long names) {
  long k = 0;
  long m = 0;
  k = 0L;
  m = l_len(body);
  while ((k < m)) {
  collectStmt(l_at(body, k), names);
  k = (k + 1L);
  }
  return 0L;
}
long collectStmt(long s, long names) {
  switch (((Obj*)(intptr_t)s)->tag) {
  case T_Let: { long name = ((Obj*)(intptr_t)s)->v0; long ty = ((Obj*)(intptr_t)s)->v1; long e = ((Obj*)(intptr_t)s)->v2; return pushUnique(names, name); }
  case T_Bare: { long e = ((Obj*)(intptr_t)s)->v0; return collectExpr(e, names); }
  case T_If: { long c = ((Obj*)(intptr_t)s)->v0; long t = ((Obj*)(intptr_t)s)->v1; long el = ((Obj*)(intptr_t)s)->v2; return collectIf(t, el, names); }
  case T_While: { long c = ((Obj*)(intptr_t)s)->v0; long b = ((Obj*)(intptr_t)s)->v1; return collectBody(b, names); }
  }
  return 0;
}
long collectExpr(long e, long names) {
  switch (((Obj*)(intptr_t)e)->tag) {
  case T_Each: { long recv = ((Obj*)(intptr_t)e)->v0; long param = ((Obj*)(intptr_t)e)->v1; long body = ((Obj*)(intptr_t)e)->v2; return collectBody(body, names); }
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
}
long collectIf(long t, long el, long names) {
  collectBody(t, names);
  collectBody(el, names);
  return 0L;
}
long pushUnique(long names, long name) {
  if ((!hasName(names, name))) {
  l_push(names, name);
  }
  return 0L;
}
long hasName(long names, long name) {
  long found = 0;
  long k = 0;
  long m = 0;
  found = false;
  k = 0L;
  m = l_len(names);
  while ((k < m)) {
  if (s_eq(l_at(names, k), name)) {
  found = true;
  }
  k = (k + 1L);
  }
  return found;
}
long emitStmt(long s, long asReturn, long ctx) {
  switch (((Obj*)(intptr_t)s)->tag) {
  case T_Let: { long name = ((Obj*)(intptr_t)s)->v0; long ty = ((Obj*)(intptr_t)s)->v1; long e = ((Obj*)(intptr_t)s)->v2; return s_concat(s_concat(s_concat(s_concat((long)(intptr_t)"  ", name), (long)(intptr_t)" = "), emitExpr(e, ctx)), (long)(intptr_t)";"); }
  case T_Bare: { long e = ((Obj*)(intptr_t)s)->v0; return emitBare(e, asReturn, ctx); }
  case T_If: { long c = ((Obj*)(intptr_t)s)->v0; long t = ((Obj*)(intptr_t)s)->v1; long el = ((Obj*)(intptr_t)s)->v2; return emitIf(c, t, el, ctx); }
  case T_While: { long c = ((Obj*)(intptr_t)s)->v0; long b = ((Obj*)(intptr_t)s)->v1; return emitWhile(c, b, ctx); }
  }
  return 0;
}
long emitIf(long c, long t, long el, long ctx) {
  long out = 0;
  out = s_concat(s_concat(s_concat(s_concat((long)(intptr_t)"  if (", emitExpr(c, ctx)), (long)(intptr_t)") {"), emitBlock(t, ctx)), (long)(intptr_t)"\n  }");
  if ((l_len(el) > 0L)) {
  out = s_concat(s_concat(s_concat(out, (long)(intptr_t)" else {"), emitBlock(el, ctx)), (long)(intptr_t)"\n  }");
  }
  return out;
}
long emitWhile(long c, long b, long ctx) {
  return s_concat(s_concat(s_concat(s_concat((long)(intptr_t)"  while (", emitExpr(c, ctx)), (long)(intptr_t)") {"), emitBlock(b, ctx)), (long)(intptr_t)"\n  }");
}
long emitBlock(long body, long ctx) {
  long out = 0;
  long k = 0;
  long m = 0;
  out = (long)(intptr_t)"";
  k = 0L;
  m = l_len(body);
  while ((k < m)) {
  out = s_concat(s_concat(out, (long)(intptr_t)"\n"), emitStmt(l_at(body, k), false, ctx));
  k = (k + 1L);
  }
  return out;
}
long emitBare(long e, long asReturn, long ctx) {
  long r = 0;
  r = stmtExpr(e, ctx);
  if (asReturn) {
  r = emitReturn(e, ctx);
  }
  return r;
}
long stmtExpr(long e, long ctx) {
  switch (((Obj*)(intptr_t)e)->tag) {
  case T_Each: { long recv = ((Obj*)(intptr_t)e)->v0; long param = ((Obj*)(intptr_t)e)->v1; long body = ((Obj*)(intptr_t)e)->v2; return emitEach(recv, param, body, ctx); }
  case T_Call: { long name = ((Obj*)(intptr_t)e)->v0; long args = ((Obj*)(intptr_t)e)->v1; return emitCallStmt(name, args, e, ctx); }
  case T_Num: { long v = ((Obj*)(intptr_t)e)->v0; return exprStmt(e, ctx); }
  case T_FloatLit: { long s = ((Obj*)(intptr_t)e)->v0; return exprStmt(e, ctx); }
  case T_Var: { long s = ((Obj*)(intptr_t)e)->v0; return exprStmt(e, ctx); }
  case T_StrLit: { long s = ((Obj*)(intptr_t)e)->v0; return exprStmt(e, ctx); }
  case T_ListLit: { long es = ((Obj*)(intptr_t)e)->v0; return exprStmt(e, ctx); }
  case T_Bin: { long op = ((Obj*)(intptr_t)e)->v0; long a = ((Obj*)(intptr_t)e)->v1; long b = ((Obj*)(intptr_t)e)->v2; return exprStmt(e, ctx); }
  case T_Match: { long scrut = ((Obj*)(intptr_t)e)->v0; long arms = ((Obj*)(intptr_t)e)->v1; return exprStmt(e, ctx); }
  case T_Field: { long recv = ((Obj*)(intptr_t)e)->v0; long fld = ((Obj*)(intptr_t)e)->v1; return exprStmt(e, ctx); }
  case T_Method: { long recv = ((Obj*)(intptr_t)e)->v0; long name = ((Obj*)(intptr_t)e)->v1; long args = ((Obj*)(intptr_t)e)->v2; return emitMethodStmt(name, args, e, ctx); }
  }
  return 0;
}
long exprStmt(long e, long ctx) {
  return s_concat(s_concat((long)(intptr_t)"  ", emitExpr(e, ctx)), (long)(intptr_t)";");
}
long emitCallStmt(long name, long args, long e, long ctx) {
  long r = 0;
  r = s_concat(s_concat((long)(intptr_t)"  ", emitExpr(e, ctx)), (long)(intptr_t)";");
  if (s_eq(name, (long)(intptr_t)"print")) {
  r = emitPrint(l_at(args, 0L), ctx);
  }
  return r;
}
long emitMethodStmt(long name, long args, long e, long ctx) {
  long r = 0;
  r = s_concat(s_concat((long)(intptr_t)"  ", emitExpr(e, ctx)), (long)(intptr_t)";");
  if (s_eq(name, (long)(intptr_t)"print")) {
  r = emitPrint(l_at(args, 0L), ctx);
  }
  return r;
}
long emitPrint(long arg, long ctx) {
  long fmt = 0;
  long cast = 0;
  long val = 0;
  long t = 0;
  fmt = (long)(intptr_t)"%ld";
  cast = (long)(intptr_t)"";
  val = emitExpr(arg, ctx);
  t = exprType(arg, ctx);
  if (s_eq(t, (long)(intptr_t)"Str")) {
  fmt = (long)(intptr_t)"%s";
  cast = (long)(intptr_t)"(const char*)(intptr_t)";
  }
  if (s_eq(t, (long)(intptr_t)"Float")) {
  fmt = (long)(intptr_t)"%s";
  cast = (long)(intptr_t)"(const char*)(intptr_t)";
  val = s_concat(s_concat((long)(intptr_t)"f_tostr(", val), (long)(intptr_t)")");
  }
  return s_concat(s_concat(s_concat(s_concat(s_concat((long)(intptr_t)"  printf(\"", fmt), (long)(intptr_t)"\\n\", "), cast), val), (long)(intptr_t)");");
}
long emitEach(long recv, long param, long body, long ctx) {
  long recvC = 0;
  long out = 0;
  recvC = emitExpr(recv, ctx);
  envPut(((CtxT*)(intptr_t)ctx)->env, param, elemOf(exprType(recv, ctx)));
  out = (long)(intptr_t)"  {";
  out = s_concat(s_concat(s_concat(out, (long)(intptr_t)"\n  long _lst = "), recvC), (long)(intptr_t)";");
  out = s_concat(out, (long)(intptr_t)"\n  long _n = l_len(_lst);");
  out = s_concat(out, (long)(intptr_t)"\n  for (long _i = 0; _i < _n; _i = _i + 1) {");
  out = s_concat(s_concat(s_concat(out, (long)(intptr_t)"\n  long "), param), (long)(intptr_t)" = l_at(_lst, _i);");
  out = s_concat(out, emitBlock(body, ctx));
  out = s_concat(out, (long)(intptr_t)"\n  }");
  out = s_concat(out, (long)(intptr_t)"\n  }");
  return out;
}
long isListType(long t) {
  long r = 0;
  r = false;
  if ((s_len(t) > 3L)) {
  if (s_eq(s_slice(t, 0L, 4L), (long)(intptr_t)"List")) {
  r = true;
  }
  }
  return r;
}
long elemOf(long t) {
  long r = 0;
  r = (long)(intptr_t)"Int";
  if (isListType(t)) {
  r = s_slice(t, 5L, (s_len(t) - 1L));
  }
  return r;
}
long isMapType(long t) {
  long r = 0;
  r = false;
  if ((s_len(t) > 4L)) {
  if (s_eq(s_slice(t, 0L, 4L), (long)(intptr_t)"Map[")) {
  r = true;
  }
  }
  return r;
}
long mapValueOf(long t) {
  long r = 0;
  r = (long)(intptr_t)"Int";
  if (isMapType(t)) {
  r = s_slice(t, 4L, (s_len(t) - 1L));
  }
  return r;
}
long emitReturn(long e, long ctx) {
  switch (((Obj*)(intptr_t)e)->tag) {
  case T_Match: { long scrut = ((Obj*)(intptr_t)e)->v0; long arms = ((Obj*)(intptr_t)e)->v1; return emitSwitch(scrut, arms, ctx); }
  case T_Num: { long v = ((Obj*)(intptr_t)e)->v0; return retVal(e, ctx); }
  case T_FloatLit: { long s = ((Obj*)(intptr_t)e)->v0; return retVal(e, ctx); }
  case T_Var: { long s = ((Obj*)(intptr_t)e)->v0; return retVal(e, ctx); }
  case T_StrLit: { long s = ((Obj*)(intptr_t)e)->v0; return retVal(e, ctx); }
  case T_ListLit: { long es = ((Obj*)(intptr_t)e)->v0; return retVal(e, ctx); }
  case T_Bin: { long op = ((Obj*)(intptr_t)e)->v0; long a = ((Obj*)(intptr_t)e)->v1; long b = ((Obj*)(intptr_t)e)->v2; return retVal(e, ctx); }
  case T_Call: { long name = ((Obj*)(intptr_t)e)->v0; long args = ((Obj*)(intptr_t)e)->v1; return retVal(e, ctx); }
  case T_Field: { long recv = ((Obj*)(intptr_t)e)->v0; long fld = ((Obj*)(intptr_t)e)->v1; return retVal(e, ctx); }
  case T_Method: { long recv = ((Obj*)(intptr_t)e)->v0; long name = ((Obj*)(intptr_t)e)->v1; long args = ((Obj*)(intptr_t)e)->v2; return retVal(e, ctx); }
  case T_Each: { long recv = ((Obj*)(intptr_t)e)->v0; long param = ((Obj*)(intptr_t)e)->v1; long body = ((Obj*)(intptr_t)e)->v2; return retVal(e, ctx); }
  }
  return 0;
}
long retVal(long e, long ctx) {
  return s_concat(s_concat((long)(intptr_t)"  return ", emitExpr(e, ctx)), (long)(intptr_t)";");
}
long emitSwitch(long scrut, long arms, long ctx) {
  long boxed = 0;
  long scrutC = 0;
  long styp = 0;
  long head = 0;
  long out = 0;
  long k = 0;
  long m = 0;
  boxed = armHasBind(arms);
  scrutC = emitExpr(scrut, ctx);
  styp = exprType(scrut, ctx);
  checkExhaustive(styp, arms, ctx);
  head = scrutC;
  if (boxed) {
  head = boxField(scrutC, (long)(intptr_t)"tag");
  }
  out = s_concat(s_concat((long)(intptr_t)"  switch (", head), (long)(intptr_t)") {");
  k = 0L;
  m = l_len(arms);
  while ((k < m)) {
  out = s_concat(s_concat(out, (long)(intptr_t)"\n"), emitArm(l_at(arms, k), scrutC, boxed, styp, ctx));
  k = (k + 1L);
  }
  out = s_concat(out, (long)(intptr_t)"\n  }");
  return s_concat(out, (long)(intptr_t)"\n  return 0;");
}
long emitArm(long a, long scrutC, long boxed, long styp, long ctx) {
  long r = 0;
  long decls = 0;
  long bi = 0;
  r = (long)(intptr_t)"";
  if (boxed) {
  decls = (long)(intptr_t)"";
  bi = 0L;
  while ((bi < l_len(((ArmT*)(intptr_t)a)->binds))) {
  envPut(((CtxT*)(intptr_t)ctx)->env, l_at(((ArmT*)(intptr_t)a)->binds, bi), payloadType(styp, ((ArmT*)(intptr_t)a)->tag, bi, ctx));
  decls = s_concat(s_concat(s_concat(s_concat(s_concat(decls, (long)(intptr_t)" long "), l_at(((ArmT*)(intptr_t)a)->binds, bi)), (long)(intptr_t)" = "), boxField(scrutC, slotName(bi))), (long)(intptr_t)";");
  bi = (bi + 1L);
  }
  r = s_concat(s_concat(s_concat(s_concat(s_concat(s_concat((long)(intptr_t)"  case T_", ((ArmT*)(intptr_t)a)->tag), (long)(intptr_t)": {"), decls), (long)(intptr_t)" return "), emitExpr(((ArmT*)(intptr_t)a)->body, ctx)), (long)(intptr_t)"; }");
  } else {
  r = s_concat(s_concat(s_concat(s_concat((long)(intptr_t)"  case ", ((ArmT*)(intptr_t)a)->tag), (long)(intptr_t)": return "), emitExpr(((ArmT*)(intptr_t)a)->body, ctx)), (long)(intptr_t)";");
  }
  return r;
}
long payloadType(long tyName, long caseTag, long idx, long ctx) {
  long r = 0;
  long types = 0;
  long k = 0;
  long m = 0;
  long t = 0;
  r = (long)(intptr_t)"Int";
  types = ((SigsT*)(intptr_t)((CtxT*)(intptr_t)ctx)->sigs)->types;
  k = 0L;
  m = l_len(types);
  while ((k < m)) {
  t = l_at(types, k);
  if (s_eq(((TyDefT*)(intptr_t)t)->name, tyName)) {
  r = casePayloadType(t, caseTag, idx);
  }
  k = (k + 1L);
  }
  return r;
}
long casePayloadType(long t, long caseTag, long idx) {
  long r = 0;
  long k = 0;
  long m = 0;
  long c = 0;
  r = (long)(intptr_t)"Int";
  k = 0L;
  m = l_len(((TyDefT*)(intptr_t)t)->cases);
  while ((k < m)) {
  c = l_at(((TyDefT*)(intptr_t)t)->cases, k);
  if (s_eq(((CaseT*)(intptr_t)c)->cname, caseTag)) {
  if ((idx < l_len(((CaseT*)(intptr_t)c)->ptypes))) {
  r = l_at(((CaseT*)(intptr_t)c)->ptypes, idx);
  }
  }
  k = (k + 1L);
  }
  return r;
}
long boxField(long scrutC, long field) {
  return s_concat(s_concat(s_concat((long)(intptr_t)"((Obj*)(intptr_t)", scrutC), (long)(intptr_t)")->"), field);
}
long checkExhaustive(long styp, long arms, long ctx) {
  long types = 0;
  long k = 0;
  long m = 0;
  long t = 0;
  types = ((SigsT*)(intptr_t)((CtxT*)(intptr_t)ctx)->sigs)->types;
  k = 0L;
  m = l_len(types);
  while ((k < m)) {
  t = l_at(types, k);
  if (s_eq(((TyDefT*)(intptr_t)t)->name, styp)) {
  checkCases(t, arms, ctx);
  checkArities(t, arms, ctx);
  }
  k = (k + 1L);
  }
  return 0L;
}
long checkArities(long t, long arms, long ctx) {
  long k = 0;
  long m = 0;
  long a = 0;
  long ar = 0;
  k = 0L;
  m = l_len(arms);
  while ((k < m)) {
  a = l_at(arms, k);
  ar = caseArityIn(t, ((ArmT*)(intptr_t)a)->tag);
  if ((!(ar < 0L))) {
  if ((!(l_len(((ArmT*)(intptr_t)a)->binds) == ar))) {
  failAt(s_concat(s_concat(s_concat(s_concat(s_concat((long)(intptr_t)"match case ", ((ArmT*)(intptr_t)a)->tag), (long)(intptr_t)" binds "), i_tostr(l_len(((ArmT*)(intptr_t)a)->binds))), (long)(intptr_t)", expected "), i_tostr(ar)), ctx);
  }
  }
  k = (k + 1L);
  }
  return 0L;
}
long failAt(long msg, long ctx) {
  return fail(s_concat(s_concat(s_concat((long)(intptr_t)"input.smplr:", envGet(((CtxT*)(intptr_t)ctx)->env, (long)(intptr_t)"__line__")), (long)(intptr_t)": "), msg));
}
long checkEffects(long f, long ctx) {
  long used = 0;
  long k = 0;
  long m = 0;
  long e = 0;
  if ((!isMain(((FnT*)(intptr_t)f)->name))) {
  used = l_new();
  effBody(((FnT*)(intptr_t)f)->body, used, ctx);
  k = 0L;
  m = l_len(used);
  while ((k < m)) {
  e = l_at(used, k);
  if ((!hasName(((FnT*)(intptr_t)f)->effs, e))) {
  failAt(s_concat(s_concat((long)(intptr_t)"uses !", e), (long)(intptr_t)" but does not declare it"), ctx);
  }
  k = (k + 1L);
  }
  }
  return 0L;
}
long effBody(long body, long used, long ctx) {
  long k = 0;
  long m = 0;
  k = 0L;
  m = l_len(body);
  while ((k < m)) {
  effStmt(l_at(body, k), used, ctx);
  k = (k + 1L);
  }
  return 0L;
}
long effStmt(long s, long used, long ctx) {
  switch (((Obj*)(intptr_t)s)->tag) {
  case T_Let: { long name = ((Obj*)(intptr_t)s)->v0; long ty = ((Obj*)(intptr_t)s)->v1; long e = ((Obj*)(intptr_t)s)->v2; return effExpr(e, used, ctx); }
  case T_Bare: { long e = ((Obj*)(intptr_t)s)->v0; return effExpr(e, used, ctx); }
  case T_If: { long c = ((Obj*)(intptr_t)s)->v0; long t = ((Obj*)(intptr_t)s)->v1; long el = ((Obj*)(intptr_t)s)->v2; return effIf(c, t, el, used, ctx); }
  case T_While: { long c = ((Obj*)(intptr_t)s)->v0; long b = ((Obj*)(intptr_t)s)->v1; return effWhile(c, b, used, ctx); }
  }
  return 0;
}
long effIf(long c, long t, long el, long used, long ctx) {
  effExpr(c, used, ctx);
  effBody(t, used, ctx);
  effBody(el, used, ctx);
  return 0L;
}
long effWhile(long c, long b, long used, long ctx) {
  effExpr(c, used, ctx);
  effBody(b, used, ctx);
  return 0L;
}
long effExpr(long e, long used, long ctx) {
  switch (((Obj*)(intptr_t)e)->tag) {
  case T_Method: { long recv = ((Obj*)(intptr_t)e)->v0; long name = ((Obj*)(intptr_t)e)->v1; long args = ((Obj*)(intptr_t)e)->v2; return effMethodE(recv, name, args, used, ctx); }
  case T_Call: { long name = ((Obj*)(intptr_t)e)->v0; long args = ((Obj*)(intptr_t)e)->v1; return effCallE(name, args, used, ctx); }
  case T_Bin: { long op = ((Obj*)(intptr_t)e)->v0; long a = ((Obj*)(intptr_t)e)->v1; long b = ((Obj*)(intptr_t)e)->v2; return effBin(a, b, used, ctx); }
  case T_Match: { long scrut = ((Obj*)(intptr_t)e)->v0; long arms = ((Obj*)(intptr_t)e)->v1; return effMatchE(scrut, arms, used, ctx); }
  case T_Field: { long recv = ((Obj*)(intptr_t)e)->v0; long fld = ((Obj*)(intptr_t)e)->v1; return effExpr(recv, used, ctx); }
  case T_Each: { long recv = ((Obj*)(intptr_t)e)->v0; long param = ((Obj*)(intptr_t)e)->v1; long body = ((Obj*)(intptr_t)e)->v2; return effEachE(recv, body, used, ctx); }
  case T_ListLit: { long es = ((Obj*)(intptr_t)e)->v0; return effArgs(es, used, ctx); }
  case T_Num: { long v = ((Obj*)(intptr_t)e)->v0; return 0L; }
  case T_FloatLit: { long s = ((Obj*)(intptr_t)e)->v0; return 0L; }
  case T_Var: { long s = ((Obj*)(intptr_t)e)->v0; return 0L; }
  case T_StrLit: { long s = ((Obj*)(intptr_t)e)->v0; return 0L; }
  }
  return 0;
}
long effMethodE(long recv, long name, long args, long used, long ctx) {
  if (s_eq(name, (long)(intptr_t)"print")) {
  l_push(used, (long)(intptr_t)"IO");
  }
  if (s_eq(name, (long)(intptr_t)"send")) {
  l_push(used, (long)(intptr_t)"IO");
  }
  if (s_eq(name, (long)(intptr_t)"read")) {
  l_push(used, (long)(intptr_t)"IO");
  l_push(used, (long)(intptr_t)"Fail");
  }
  if (s_eq(name, (long)(intptr_t)"write")) {
  l_push(used, (long)(intptr_t)"IO");
  }
  effExpr(recv, used, ctx);
  effArgs(args, used, ctx);
  return 0L;
}
long effCallE(long name, long args, long used, long ctx) {
  long fns = 0;
  long k = 0;
  long m = 0;
  long g = 0;
  fns = ((SigsT*)(intptr_t)((CtxT*)(intptr_t)ctx)->sigs)->fns;
  k = 0L;
  m = l_len(fns);
  while ((k < m)) {
  g = l_at(fns, k);
  if (s_eq(((FnT*)(intptr_t)g)->name, name)) {
  addAll(((FnT*)(intptr_t)g)->effs, used);
  }
  k = (k + 1L);
  }
  effArgs(args, used, ctx);
  return 0L;
}
long effBin(long a, long b, long used, long ctx) {
  effExpr(a, used, ctx);
  effExpr(b, used, ctx);
  return 0L;
}
long effMatchE(long scrut, long arms, long used, long ctx) {
  long k = 0;
  long m = 0;
  effExpr(scrut, used, ctx);
  k = 0L;
  m = l_len(arms);
  while ((k < m)) {
  effExpr(((ArmT*)(intptr_t)l_at(arms, k))->body, used, ctx);
  k = (k + 1L);
  }
  return 0L;
}
long effEachE(long recv, long body, long used, long ctx) {
  effExpr(recv, used, ctx);
  effBody(body, used, ctx);
  return 0L;
}
long effArgs(long args, long used, long ctx) {
  long k = 0;
  long m = 0;
  k = 0L;
  m = l_len(args);
  while ((k < m)) {
  effExpr(l_at(args, k), used, ctx);
  k = (k + 1L);
  }
  return 0L;
}
long addAll(long src, long dst) {
  long k = 0;
  long m = 0;
  k = 0L;
  m = l_len(src);
  while ((k < m)) {
  l_push(dst, l_at(src, k));
  k = (k + 1L);
  }
  return 0L;
}
long caseArityIn(long t, long tag) {
  long r = 0;
  long k = 0;
  long m = 0;
  long c = 0;
  r = (0L - 1L);
  k = 0L;
  m = l_len(((TyDefT*)(intptr_t)t)->cases);
  while ((k < m)) {
  c = l_at(((TyDefT*)(intptr_t)t)->cases, k);
  if (s_eq(((CaseT*)(intptr_t)c)->cname, tag)) {
  r = ((CaseT*)(intptr_t)c)->arity;
  }
  k = (k + 1L);
  }
  return r;
}
long checkCases(long t, long arms, long ctx) {
  long k = 0;
  long m = 0;
  long c = 0;
  k = 0L;
  m = l_len(((TyDefT*)(intptr_t)t)->cases);
  while ((k < m)) {
  c = l_at(((TyDefT*)(intptr_t)t)->cases, k);
  if ((!armCovers(arms, ((CaseT*)(intptr_t)c)->cname))) {
  failAt(s_concat(s_concat(s_concat((long)(intptr_t)"non-exhaustive match in ", ((TyDefT*)(intptr_t)t)->name), (long)(intptr_t)", missing case: "), ((CaseT*)(intptr_t)c)->cname), ctx);
  }
  k = (k + 1L);
  }
  return 0L;
}
long armCovers(long arms, long cname) {
  long found = 0;
  long k = 0;
  long m = 0;
  found = false;
  k = 0L;
  m = l_len(arms);
  while ((k < m)) {
  if (s_eq(((ArmT*)(intptr_t)l_at(arms, k))->tag, cname)) {
  found = true;
  }
  k = (k + 1L);
  }
  return found;
}
long armHasBind(long arms) {
  long has = 0;
  long k = 0;
  long m = 0;
  has = false;
  k = 0L;
  m = l_len(arms);
  while ((k < m)) {
  if ((l_len(((ArmT*)(intptr_t)l_at(arms, k))->binds) > 0L)) {
  has = true;
  }
  k = (k + 1L);
  }
  return has;
}
long emitExpr(long e, long ctx) {
  switch (((Obj*)(intptr_t)e)->tag) {
  case T_Num: { long v = ((Obj*)(intptr_t)e)->v0; return s_concat(i_tostr(v), (long)(intptr_t)"L"); }
  case T_FloatLit: { long s = ((Obj*)(intptr_t)e)->v0; return s_concat(s_concat((long)(intptr_t)"d2l(", s), (long)(intptr_t)")"); }
  case T_Var: { long s = ((Obj*)(intptr_t)e)->v0; return emitVar(s, ctx); }
  case T_StrLit: { long s = ((Obj*)(intptr_t)e)->v0; return s_concat(s_concat((long)(intptr_t)"(long)(intptr_t)\"", cEscape(s)), (long)(intptr_t)"\""); }
  case T_ListLit: { long es = ((Obj*)(intptr_t)e)->v0; return (long)(intptr_t)"l_new()"; }
  case T_Bin: { long op = ((Obj*)(intptr_t)e)->v0; long a = ((Obj*)(intptr_t)e)->v1; long b = ((Obj*)(intptr_t)e)->v2; return emitBin(op, a, b, ctx); }
  case T_Call: { long name = ((Obj*)(intptr_t)e)->v0; long args = ((Obj*)(intptr_t)e)->v1; return emitCall(name, args, ctx); }
  case T_Match: { long scrut = ((Obj*)(intptr_t)e)->v0; long arms = ((Obj*)(intptr_t)e)->v1; return (long)(intptr_t)"0"; }
  case T_Field: { long recv = ((Obj*)(intptr_t)e)->v0; long fld = ((Obj*)(intptr_t)e)->v1; return emitField(recv, fld, ctx); }
  case T_Method: { long recv = ((Obj*)(intptr_t)e)->v0; long name = ((Obj*)(intptr_t)e)->v1; long args = ((Obj*)(intptr_t)e)->v2; return emitMethod(recv, name, args, ctx); }
  case T_Each: { long recv = ((Obj*)(intptr_t)e)->v0; long param = ((Obj*)(intptr_t)e)->v1; long body = ((Obj*)(intptr_t)e)->v2; return (long)(intptr_t)"0"; }
  }
  return 0;
}
long emitBin(long op, long a, long b, long ctx) {
  long ca = 0;
  long cb = 0;
  long r = 0;
  long inner = 0;
  checkBinOp(op, a, b, ctx);
  ca = emitExpr(a, ctx);
  cb = emitExpr(b, ctx);
  r = s_concat(s_concat(s_concat(s_concat(s_concat(s_concat((long)(intptr_t)"(", ca), (long)(intptr_t)" "), op), (long)(intptr_t)" "), cb), (long)(intptr_t)")");
  if (s_eq(op, (long)(intptr_t)"==")) {
  if (s_eq(exprType(a, ctx), (long)(intptr_t)"Str")) {
  r = s_concat(s_concat(s_concat(s_concat((long)(intptr_t)"s_eq(", ca), (long)(intptr_t)", "), cb), (long)(intptr_t)")");
  }
  }
  if (s_eq(exprType(a, ctx), (long)(intptr_t)"Float")) {
  inner = s_concat(s_concat(s_concat(s_concat(s_concat(s_concat((long)(intptr_t)"(l2d(", ca), (long)(intptr_t)") "), op), (long)(intptr_t)" l2d("), cb), (long)(intptr_t)")) ");
  r = inner;
  if (isFloatArith(op)) {
  r = s_concat(s_concat((long)(intptr_t)"d2l(", inner), (long)(intptr_t)")");
  }
  }
  if (s_eq(op, (long)(intptr_t)"u-")) {
  r = s_concat(s_concat((long)(intptr_t)"(-", cb), (long)(intptr_t)")");
  if (s_eq(exprType(b, ctx), (long)(intptr_t)"Float")) {
  r = s_concat(s_concat((long)(intptr_t)"d2l(-l2d(", cb), (long)(intptr_t)"))");
  }
  }
  return r;
}
long isFloatArith(long op) {
  return (((s_eq(op, (long)(intptr_t)"+") || s_eq(op, (long)(intptr_t)"-")) || s_eq(op, (long)(intptr_t)"*")) || s_eq(op, (long)(intptr_t)"/"));
}
long checkBinOp(long op, long a, long b, long ctx) {
  long ta = 0;
  long tb = 0;
  if (isArithOp(op)) {
  ta = exprType(a, ctx);
  tb = exprType(b, ctx);
  if (notInt(ta, ctx)) {
  failAt(s_concat(s_concat((long)(intptr_t)"operator ", op), (long)(intptr_t)" needs Int operands"), ctx);
  }
  if (notInt(tb, ctx)) {
  failAt(s_concat(s_concat((long)(intptr_t)"operator ", op), (long)(intptr_t)" needs Int operands"), ctx);
  }
  if (mixedFloat(ta, tb)) {
  failAt(s_concat(s_concat((long)(intptr_t)"operator ", op), (long)(intptr_t)" mixes Int and Float; convert with .toFloat or .toInt"), ctx);
  }
  }
  return 0L;
}
long mixedFloat(long a, long b) {
  long r = 0;
  r = false;
  if ((s_eq(a, (long)(intptr_t)"Float") && (!s_eq(b, (long)(intptr_t)"Float")))) {
  r = true;
  }
  if ((s_eq(b, (long)(intptr_t)"Float") && (!s_eq(a, (long)(intptr_t)"Float")))) {
  r = true;
  }
  return r;
}
long isArithOp(long op) {
  return (((((s_eq(op, (long)(intptr_t)"+") || s_eq(op, (long)(intptr_t)"-")) || s_eq(op, (long)(intptr_t)"*")) || s_eq(op, (long)(intptr_t)"/")) || s_eq(op, (long)(intptr_t)"<")) || s_eq(op, (long)(intptr_t)">"));
}
long emitCall(long name, long args, long ctx) {
  checkCall(name, args, ctx);
  return s_concat(s_concat(s_concat(name, (long)(intptr_t)"("), emitArgs(args, ctx)), (long)(intptr_t)")");
}
long checkReturn(long f, long ctx) {
  long n = 0;
  if ((!isMain(((FnT*)(intptr_t)f)->name))) {
  n = l_len(((FnT*)(intptr_t)f)->body);
  if ((n > 0L)) {
  checkReturnStmt(l_at(((FnT*)(intptr_t)f)->body, (n - 1L)), ((FnT*)(intptr_t)f)->ret, ctx);
  }
  }
  return 0L;
}
long checkReturnStmt(long s, long ret, long ctx) {
  switch (((Obj*)(intptr_t)s)->tag) {
  case T_Bare: { long e = ((Obj*)(intptr_t)s)->v0; return checkRetExpr(e, ret, ctx); }
  case T_Let: { long name = ((Obj*)(intptr_t)s)->v0; long ty = ((Obj*)(intptr_t)s)->v1; long v = ((Obj*)(intptr_t)s)->v2; return 0L; }
  case T_If: { long c = ((Obj*)(intptr_t)s)->v0; long t = ((Obj*)(intptr_t)s)->v1; long el = ((Obj*)(intptr_t)s)->v2; return 0L; }
  case T_While: { long c = ((Obj*)(intptr_t)s)->v0; long b = ((Obj*)(intptr_t)s)->v1; return 0L; }
  }
  return 0;
}
long checkRetExpr(long e, long ret, long ctx) {
  long at = 0;
  if ((!isMatchExpr(e))) {
  at = exprType(e, ctx);
  if ((!compatible(ret, at, ctx))) {
  failAt(s_concat(s_concat(s_concat((long)(intptr_t)"return type mismatch: declared ", ret), (long)(intptr_t)", got "), at), ctx);
  }
  }
  return 0L;
}
long isMatchExpr(long e) {
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
}
long checkCall(long name, long args, long ctx) {
  long fns = 0;
  long k = 0;
  long m = 0;
  long f = 0;
  long recs = 0;
  long rec = 0;
  fns = ((SigsT*)(intptr_t)((CtxT*)(intptr_t)ctx)->sigs)->fns;
  k = 0L;
  m = l_len(fns);
  while ((k < m)) {
  f = l_at(fns, k);
  if (s_eq(((FnT*)(intptr_t)f)->name, name)) {
  checkArgTypes(name, ((FnT*)(intptr_t)f)->ptypes, args, ctx);
  }
  k = (k + 1L);
  }
  recs = ((SigsT*)(intptr_t)((CtxT*)(intptr_t)ctx)->sigs)->records;
  k = 0L;
  m = l_len(recs);
  while ((k < m)) {
  rec = l_at(recs, k);
  if (s_eq(((RecDefT*)(intptr_t)rec)->name, name)) {
  checkArgTypes(name, ((RecDefT*)(intptr_t)rec)->ftypes, args, ctx);
  }
  k = (k + 1L);
  }
  return 0L;
}
long checkArgTypes(long name, long ptypes, long args, long ctx) {
  long n = 0;
  long k = 0;
  long pt = 0;
  long at = 0;
  n = l_len(args);
  if ((n == l_len(ptypes))) {
  k = 0L;
  while ((k < n)) {
  pt = l_at(ptypes, k);
  at = exprType(l_at(args, k), ctx);
  if ((!compatible(pt, at, ctx))) {
  failAt(s_concat(s_concat(s_concat(s_concat(s_concat(s_concat(s_concat((long)(intptr_t)"argument ", i_tostr((k + 1L))), (long)(intptr_t)" of "), name), (long)(intptr_t)": expects "), pt), (long)(intptr_t)", got "), at), ctx);
  }
  k = (k + 1L);
  }
  }
  return 0L;
}
long compatible(long want, long got, long ctx) {
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
  if ((intLike(want) && intLike(got))) {
  r = true;
  }
  if ((isListType(want) && isListType(got))) {
  r = true;
  }
  return r;
}
long intLike(long t) {
  return (s_eq(t, (long)(intptr_t)"Int") || s_eq(t, (long)(intptr_t)"Bool"));
}
long notInt(long t, long ctx) {
  long r = 0;
  r = false;
  if (s_eq(t, (long)(intptr_t)"Str")) {
  r = true;
  }
  if (isListType(t)) {
  r = true;
  }
  if (isRec(t, ctx)) {
  r = true;
  }
  if (isVariant(t, ctx)) {
  r = true;
  }
  return r;
}
long varType(long s, long ctx) {
  long r = 0;
  long vt = 0;
  r = envGet(((CtxT*)(intptr_t)ctx)->env, s);
  vt = variantOf(s, ctx);
  if ((!s_eq(vt, (long)(intptr_t)""))) {
  r = vt;
  }
  return r;
}
long isVariant(long t, long ctx) {
  long found = 0;
  long types = 0;
  long k = 0;
  long m = 0;
  found = false;
  types = ((SigsT*)(intptr_t)((CtxT*)(intptr_t)ctx)->sigs)->types;
  k = 0L;
  m = l_len(types);
  while ((k < m)) {
  if (s_eq(((TyDefT*)(intptr_t)l_at(types, k))->name, t)) {
  found = true;
  }
  k = (k + 1L);
  }
  return found;
}
long emitField(long recv, long fld, long ctx) {
  long t = 0;
  long recvC = 0;
  long r = 0;
  long flag = 0;
  t = exprType(recv, ctx);
  recvC = emitExpr(recv, ctx);
  r = s_concat(s_concat(s_concat(s_concat(s_concat((long)(intptr_t)"((", t), (long)(intptr_t)"T*)(intptr_t)"), recvC), (long)(intptr_t)")->"), fld);
  if ((!isRec(t, ctx))) {
  if (s_eq(fld, (long)(intptr_t)"length")) {
  r = s_concat(s_concat((long)(intptr_t)"s_len(", recvC), (long)(intptr_t)")");
  if (isListType(t)) {
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
  if (s_eq(fld, (long)(intptr_t)"toInt")) {
  r = s_concat(s_concat((long)(intptr_t)"s_toint(", recvC), (long)(intptr_t)")");
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
  if (s_eq(fld, (long)(intptr_t)"not")) {
  r = s_concat(s_concat((long)(intptr_t)"(!", recvC), (long)(intptr_t)")");
  }
  if (isCapField(fld)) {
  r = (long)(intptr_t)"0";
  }
  if (s_eq(fld, (long)(intptr_t)"args")) {
  r = (long)(intptr_t)"simpler_args(argc, argv)";
  }
  if (s_eq(fld, (long)(intptr_t)"stdin")) {
  r = (long)(intptr_t)"simpler_stdin()";
  }
  if (s_eq(fld, (long)(intptr_t)"keys")) {
  r = s_concat(s_concat((long)(intptr_t)"m_keys(", recvC), (long)(intptr_t)")");
  }
  if (s_eq(fld, (long)(intptr_t)"byValue")) {
  r = s_concat(s_concat((long)(intptr_t)"m_byvalue(", recvC), (long)(intptr_t)")");
  }
  if (s_eq(fld, (long)(intptr_t)"sort")) {
  flag = (long)(intptr_t)"0";
  if (s_eq(elemOf(t), (long)(intptr_t)"Str")) {
  flag = (long)(intptr_t)"1";
  }
  r = s_concat(s_concat(s_concat(s_concat((long)(intptr_t)"l_sort(", recvC), (long)(intptr_t)", "), flag), (long)(intptr_t)")");
  }
  }
  return r;
}
long isCapField(long fld) {
  return ((s_eq(fld, (long)(intptr_t)"screen") || s_eq(fld, (long)(intptr_t)"files")) || s_eq(fld, (long)(intptr_t)"mail"));
}
long emitMethod(long recv, long name, long args, long ctx) {
  long t = 0;
  long recvC = 0;
  long r = 0;
  t = exprType(recv, ctx);
  recvC = emitExpr(recv, ctx);
  r = (long)(intptr_t)"0";
  if (s_eq(name, (long)(intptr_t)"concat")) {
  r = s_concat(s_concat(s_concat(s_concat((long)(intptr_t)"s_concat(", recvC), (long)(intptr_t)", "), emitExpr(l_at(args, 0L), ctx)), (long)(intptr_t)")");
  }
  if (s_eq(name, (long)(intptr_t)"slice")) {
  r = s_concat(s_concat(s_concat(s_concat(s_concat(s_concat((long)(intptr_t)"s_slice(", recvC), (long)(intptr_t)", "), emitExpr(l_at(args, 0L), ctx)), (long)(intptr_t)", "), emitExpr(l_at(args, 1L), ctx)), (long)(intptr_t)")");
  }
  if (s_eq(name, (long)(intptr_t)"at")) {
  r = s_concat(s_concat(s_concat(s_concat((long)(intptr_t)"s_at(", recvC), (long)(intptr_t)", "), emitExpr(l_at(args, 0L), ctx)), (long)(intptr_t)")");
  if (isListType(t)) {
  r = s_concat(s_concat(s_concat(s_concat((long)(intptr_t)"l_at(", recvC), (long)(intptr_t)", "), emitExpr(l_at(args, 0L), ctx)), (long)(intptr_t)")");
  }
  }
  if (s_eq(name, (long)(intptr_t)"push")) {
  r = s_concat(s_concat(s_concat(s_concat((long)(intptr_t)"l_push(", recvC), (long)(intptr_t)", "), emitExpr(l_at(args, 0L), ctx)), (long)(intptr_t)")");
  }
  if (s_eq(name, (long)(intptr_t)"read")) {
  r = s_concat(s_concat((long)(intptr_t)"(long)(intptr_t)simpler_read((const char*)(intptr_t)", emitExpr(l_at(args, 0L), ctx)), (long)(intptr_t)")");
  }
  if (s_eq(name, (long)(intptr_t)"write")) {
  r = s_concat(s_concat(s_concat(s_concat((long)(intptr_t)"simpler_write(", emitExpr(l_at(args, 0L), ctx)), (long)(intptr_t)", "), emitExpr(l_at(args, 1L), ctx)), (long)(intptr_t)")");
  }
  if (s_eq(name, (long)(intptr_t)"split")) {
  r = s_concat(s_concat(s_concat(s_concat((long)(intptr_t)"s_split(", recvC), (long)(intptr_t)", "), emitExpr(l_at(args, 0L), ctx)), (long)(intptr_t)")");
  }
  if (s_eq(name, (long)(intptr_t)"contains")) {
  r = s_concat(s_concat(s_concat(s_concat((long)(intptr_t)"s_contains(", recvC), (long)(intptr_t)", "), emitExpr(l_at(args, 0L), ctx)), (long)(intptr_t)")");
  }
  if (s_eq(name, (long)(intptr_t)"replace")) {
  r = s_concat(s_concat(s_concat(s_concat(s_concat(s_concat((long)(intptr_t)"s_replace(", recvC), (long)(intptr_t)", "), emitExpr(l_at(args, 0L), ctx)), (long)(intptr_t)", "), emitExpr(l_at(args, 1L), ctx)), (long)(intptr_t)")");
  }
  if (s_eq(name, (long)(intptr_t)"set")) {
  r = s_concat(s_concat(s_concat(s_concat(s_concat(s_concat((long)(intptr_t)"m_set(", recvC), (long)(intptr_t)", "), emitExpr(l_at(args, 0L), ctx)), (long)(intptr_t)", "), emitExpr(l_at(args, 1L), ctx)), (long)(intptr_t)")");
  }
  if (s_eq(name, (long)(intptr_t)"get")) {
  r = s_concat(s_concat(s_concat(s_concat((long)(intptr_t)"m_get(", recvC), (long)(intptr_t)", "), emitExpr(l_at(args, 0L), ctx)), (long)(intptr_t)")");
  if (s_eq(mapValueOf(t), (long)(intptr_t)"Str")) {
  r = s_concat(s_concat(s_concat(s_concat((long)(intptr_t)"m_gets(", recvC), (long)(intptr_t)", "), emitExpr(l_at(args, 0L), ctx)), (long)(intptr_t)")");
  }
  }
  if (s_eq(name, (long)(intptr_t)"has")) {
  r = s_concat(s_concat(s_concat(s_concat((long)(intptr_t)"m_has(", recvC), (long)(intptr_t)", "), emitExpr(l_at(args, 0L), ctx)), (long)(intptr_t)")");
  }
  if (s_eq(name, (long)(intptr_t)"ge")) {
  r = s_concat(s_concat(s_concat(s_concat((long)(intptr_t)"(", recvC), (long)(intptr_t)" >= "), emitExpr(l_at(args, 0L), ctx)), (long)(intptr_t)")");
  }
  if (s_eq(name, (long)(intptr_t)"le")) {
  r = s_concat(s_concat(s_concat(s_concat((long)(intptr_t)"(", recvC), (long)(intptr_t)" <= "), emitExpr(l_at(args, 0L), ctx)), (long)(intptr_t)")");
  }
  if (s_eq(name, (long)(intptr_t)"and")) {
  r = s_concat(s_concat(s_concat(s_concat((long)(intptr_t)"(", recvC), (long)(intptr_t)" && "), emitExpr(l_at(args, 0L), ctx)), (long)(intptr_t)")");
  }
  if (s_eq(name, (long)(intptr_t)"or")) {
  r = s_concat(s_concat(s_concat(s_concat((long)(intptr_t)"(", recvC), (long)(intptr_t)" || "), emitExpr(l_at(args, 0L), ctx)), (long)(intptr_t)")");
  }
  return r;
}
long cEscape(long s) {
  long out = 0;
  long k = 0;
  long n = 0;
  long c = 0;
  long piece = 0;
  out = (long)(intptr_t)"";
  k = 0L;
  n = s_len(s);
  while ((k < n)) {
  c = s_code(s_at(s, k));
  piece = s_at(s, k);
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
}
long emitVar(long s, long ctx) {
  long r = 0;
  r = s;
  if (hasName(((SigsT*)(intptr_t)((CtxT*)(intptr_t)ctx)->sigs)->boxedNullary, s)) {
  r = s_concat(s, (long)(intptr_t)"()");
  }
  return r;
}
long emitArgs(long args, long ctx) {
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
  out = s_concat(out, emitExpr(l_at(args, k), ctx));
  k = (k + 1L);
  }
  return out;
}
long binType(long op, long a, long b, long ctx) {
  long r = 0;
  r = (long)(intptr_t)"Int";
  if (s_eq(op, (long)(intptr_t)"u-")) {
  r = exprType(b, ctx);
  }
  if (s_eq(exprType(a, ctx), (long)(intptr_t)"Float")) {
  if (isFloatArith(op)) {
  r = (long)(intptr_t)"Float";
  }
  }
  return r;
}
long exprType(long e, long ctx) {
  switch (((Obj*)(intptr_t)e)->tag) {
  case T_Num: { long v = ((Obj*)(intptr_t)e)->v0; return (long)(intptr_t)"Int"; }
  case T_FloatLit: { long s = ((Obj*)(intptr_t)e)->v0; return (long)(intptr_t)"Float"; }
  case T_Var: { long s = ((Obj*)(intptr_t)e)->v0; return varType(s, ctx); }
  case T_StrLit: { long s = ((Obj*)(intptr_t)e)->v0; return (long)(intptr_t)"Str"; }
  case T_ListLit: { long es = ((Obj*)(intptr_t)e)->v0; return (long)(intptr_t)"List"; }
  case T_Bin: { long op = ((Obj*)(intptr_t)e)->v0; long a = ((Obj*)(intptr_t)e)->v1; long b = ((Obj*)(intptr_t)e)->v2; return binType(op, a, b, ctx); }
  case T_Call: { long name = ((Obj*)(intptr_t)e)->v0; long args = ((Obj*)(intptr_t)e)->v1; return callRet(name, ctx); }
  case T_Match: { long scrut = ((Obj*)(intptr_t)e)->v0; long arms = ((Obj*)(intptr_t)e)->v1; return (long)(intptr_t)"Int"; }
  case T_Field: { long recv = ((Obj*)(intptr_t)e)->v0; long fld = ((Obj*)(intptr_t)e)->v1; return fieldType(recv, fld, ctx); }
  case T_Method: { long recv = ((Obj*)(intptr_t)e)->v0; long name = ((Obj*)(intptr_t)e)->v1; long args = ((Obj*)(intptr_t)e)->v2; return methodRet(recv, name, ctx); }
  case T_Each: { long recv = ((Obj*)(intptr_t)e)->v0; long param = ((Obj*)(intptr_t)e)->v1; long body = ((Obj*)(intptr_t)e)->v2; return (long)(intptr_t)"Int"; }
  }
  return 0;
}
long fieldType(long recv, long fld, long ctx) {
  long t = 0;
  long r = 0;
  t = exprType(recv, ctx);
  r = (long)(intptr_t)"Int";
  if (s_eq(fld, (long)(intptr_t)"toStr")) {
  r = (long)(intptr_t)"Str";
  }
  if (s_eq(fld, (long)(intptr_t)"toFloat")) {
  r = (long)(intptr_t)"Float";
  }
  if (s_eq(fld, (long)(intptr_t)"trim")) {
  r = (long)(intptr_t)"Str";
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
  if (s_eq(fld, (long)(intptr_t)"args")) {
  r = (long)(intptr_t)"List[Str]";
  }
  if (s_eq(fld, (long)(intptr_t)"stdin")) {
  r = (long)(intptr_t)"Str";
  }
  if (s_eq(fld, (long)(intptr_t)"keys")) {
  r = (long)(intptr_t)"List[Str]";
  }
  if (s_eq(fld, (long)(intptr_t)"byValue")) {
  r = (long)(intptr_t)"List[Str]";
  }
  if (s_eq(fld, (long)(intptr_t)"sort")) {
  r = t;
  }
  if (isRec(t, ctx)) {
  r = recFieldType(t, fld, ctx);
  }
  return r;
}
long recFieldType(long tyName, long fld, long ctx) {
  long r = 0;
  long recs = 0;
  long k = 0;
  long m = 0;
  long rec = 0;
  r = (long)(intptr_t)"Int";
  recs = ((SigsT*)(intptr_t)((CtxT*)(intptr_t)ctx)->sigs)->records;
  k = 0L;
  m = l_len(recs);
  while ((k < m)) {
  rec = l_at(recs, k);
  if (s_eq(((RecDefT*)(intptr_t)rec)->name, tyName)) {
  r = fieldTypeIn(rec, fld);
  }
  k = (k + 1L);
  }
  return r;
}
long fieldTypeIn(long rec, long fld) {
  long r = 0;
  long k = 0;
  long m = 0;
  r = (long)(intptr_t)"Int";
  k = 0L;
  m = l_len(((RecDefT*)(intptr_t)rec)->fields);
  while ((k < m)) {
  if (s_eq(l_at(((RecDefT*)(intptr_t)rec)->fields, k), fld)) {
  r = l_at(((RecDefT*)(intptr_t)rec)->ftypes, k);
  }
  k = (k + 1L);
  }
  return r;
}
long methodRet(long recv, long name, long ctx) {
  long r = 0;
  long rt = 0;
  r = (long)(intptr_t)"Int";
  if (s_eq(name, (long)(intptr_t)"concat")) {
  r = (long)(intptr_t)"Str";
  }
  if (s_eq(name, (long)(intptr_t)"slice")) {
  r = (long)(intptr_t)"Str";
  }
  if (s_eq(name, (long)(intptr_t)"read")) {
  r = (long)(intptr_t)"Str";
  }
  if (s_eq(name, (long)(intptr_t)"split")) {
  r = (long)(intptr_t)"List[Str]";
  }
  if (s_eq(name, (long)(intptr_t)"contains")) {
  r = (long)(intptr_t)"Bool";
  }
  if (s_eq(name, (long)(intptr_t)"replace")) {
  r = (long)(intptr_t)"Str";
  }
  if (s_eq(name, (long)(intptr_t)"has")) {
  r = (long)(intptr_t)"Bool";
  }
  if (s_eq(name, (long)(intptr_t)"get")) {
  r = mapValueOf(exprType(recv, ctx));
  }
  if (s_eq(name, (long)(intptr_t)"at")) {
  r = (long)(intptr_t)"Str";
  rt = exprType(recv, ctx);
  if (isListType(rt)) {
  r = elemOf(rt);
  }
  }
  return r;
}
long isMain(long name) {
  return s_eq(name, (long)(intptr_t)"main");
}
long notEof(long toks, long i) {
  switch (((Obj*)(intptr_t)l_at(toks, i))->tag) {
  case T_Eof: { return false; }
  case T_Ident: { long s = ((Obj*)(intptr_t)l_at(toks, i))->v0; return true; }
  case T_Int: { long v = ((Obj*)(intptr_t)l_at(toks, i))->v0; return true; }
  case T_Float: { long f = ((Obj*)(intptr_t)l_at(toks, i))->v0; return true; }
  case T_Str: { long s = ((Obj*)(intptr_t)l_at(toks, i))->v0; return true; }
  case T_Punct: { long p = ((Obj*)(intptr_t)l_at(toks, i))->v0; return true; }
  }
  return 0;
}
long inBlock(long toks, long j) {
  return ((!isPunct(toks, j, (long)(intptr_t)"}")) && notEof(toks, j));
}
long inArgs(long toks, long j) {
  return ((!isPunct(toks, j, (long)(intptr_t)")")) && notEof(toks, j));
}
long isIdent(long toks, long i) {
  switch (((Obj*)(intptr_t)l_at(toks, i))->tag) {
  case T_Ident: { long s = ((Obj*)(intptr_t)l_at(toks, i))->v0; return true; }
  case T_Int: { long v = ((Obj*)(intptr_t)l_at(toks, i))->v0; return false; }
  case T_Float: { long f = ((Obj*)(intptr_t)l_at(toks, i))->v0; return false; }
  case T_Str: { long s = ((Obj*)(intptr_t)l_at(toks, i))->v0; return false; }
  case T_Punct: { long p = ((Obj*)(intptr_t)l_at(toks, i))->v0; return false; }
  case T_Eof: { return false; }
  }
  return 0;
}
long isWord(long toks, long i, long w) {
  switch (((Obj*)(intptr_t)l_at(toks, i))->tag) {
  case T_Ident: { long s = ((Obj*)(intptr_t)l_at(toks, i))->v0; return s_eq(s, w); }
  case T_Int: { long v = ((Obj*)(intptr_t)l_at(toks, i))->v0; return false; }
  case T_Float: { long f = ((Obj*)(intptr_t)l_at(toks, i))->v0; return false; }
  case T_Str: { long s = ((Obj*)(intptr_t)l_at(toks, i))->v0; return false; }
  case T_Punct: { long p = ((Obj*)(intptr_t)l_at(toks, i))->v0; return false; }
  case T_Eof: { return false; }
  }
  return 0;
}
long isAssign(long toks, long i) {
  return (isIdent(toks, i) && isPunct(toks, (i + 1L), (long)(intptr_t)"="));
}
long identAt(long toks, long i) {
  switch (((Obj*)(intptr_t)l_at(toks, i))->tag) {
  case T_Ident: { long s = ((Obj*)(intptr_t)l_at(toks, i))->v0; return s; }
  case T_Int: { long v = ((Obj*)(intptr_t)l_at(toks, i))->v0; return (long)(intptr_t)"?"; }
  case T_Float: { long f = ((Obj*)(intptr_t)l_at(toks, i))->v0; return (long)(intptr_t)"?"; }
  case T_Str: { long s = ((Obj*)(intptr_t)l_at(toks, i))->v0; return (long)(intptr_t)"?"; }
  case T_Punct: { long p = ((Obj*)(intptr_t)l_at(toks, i))->v0; return (long)(intptr_t)"?"; }
  case T_Eof: { return (long)(intptr_t)"?"; }
  }
  return 0;
}
long punctAt(long toks, long i) {
  switch (((Obj*)(intptr_t)l_at(toks, i))->tag) {
  case T_Punct: { long p = ((Obj*)(intptr_t)l_at(toks, i))->v0; return p; }
  case T_Ident: { long s = ((Obj*)(intptr_t)l_at(toks, i))->v0; return (long)(intptr_t)"?"; }
  case T_Int: { long v = ((Obj*)(intptr_t)l_at(toks, i))->v0; return (long)(intptr_t)"?"; }
  case T_Float: { long f = ((Obj*)(intptr_t)l_at(toks, i))->v0; return (long)(intptr_t)"?"; }
  case T_Str: { long s = ((Obj*)(intptr_t)l_at(toks, i))->v0; return (long)(intptr_t)"?"; }
  case T_Eof: { return (long)(intptr_t)"?"; }
  }
  return 0;
}
long isPunct(long toks, long i, long op) {
  switch (((Obj*)(intptr_t)l_at(toks, i))->tag) {
  case T_Punct: { long p = ((Obj*)(intptr_t)l_at(toks, i))->v0; return s_eq(p, op); }
  case T_Ident: { long s = ((Obj*)(intptr_t)l_at(toks, i))->v0; return false; }
  case T_Int: { long v = ((Obj*)(intptr_t)l_at(toks, i))->v0; return false; }
  case T_Float: { long f = ((Obj*)(intptr_t)l_at(toks, i))->v0; return false; }
  case T_Str: { long s = ((Obj*)(intptr_t)l_at(toks, i))->v0; return false; }
  case T_Eof: { return false; }
  }
  return 0;
}
long isCmpOp(long toks, long i) {
  return ((isPunct(toks, i, (long)(intptr_t)"==") || isPunct(toks, i, (long)(intptr_t)"<")) || isPunct(toks, i, (long)(intptr_t)">"));
}
long isAddOp(long toks, long i) {
  return (isPunct(toks, i, (long)(intptr_t)"+") || isPunct(toks, i, (long)(intptr_t)"-"));
}
long isMulOp(long toks, long i) {
  return (isPunct(toks, i, (long)(intptr_t)"*") || isPunct(toks, i, (long)(intptr_t)"/"));
}
long lex(long src) {
  long toks = 0;
  long lines = 0;
  long n = 0;
  long i = 0;
  long line = 0;
  long c = 0;
  long j = 0;
  long s = 0;
  long start = 0;
  long num = 0;
  toks = l_new();
  lines = l_new();
  n = s_len(src);
  i = 0L;
  line = 1L;
  while ((i < n)) {
  c = s_code(s_at(src, i));
  if (isSpace(c)) {
  if ((c == 10L)) {
  line = (line + 1L);
  }
  i = (i + 1L);
  } else {
  if (isComment(src, i, n)) {
  while (((i < n) && (!(s_code(s_at(src, i)) == 10L)))) {
  i = (i + 1L);
  }
  } else {
  if ((c == 34L)) {
  j = (i + 1L);
  s = (long)(intptr_t)"";
  while (((j < n) && (!(s_code(s_at(src, j)) == 34L)))) {
  if ((s_code(s_at(src, j)) == 92L)) {
  j = (j + 1L);
  s = s_concat(s, esc(src, j));
  } else {
  s = s_concat(s, s_at(src, j));
  }
  j = (j + 1L);
  }
  l_push(toks, Str(s));
  l_push(lines, line);
  i = (j + 1L);
  } else {
  if (isDigit(c)) {
  start = i;
  num = 0L;
  while (((i < n) && isDigit(s_code(s_at(src, i))))) {
  num = ((num * 10L) + (s_code(s_at(src, i)) - 48L));
  i = (i + 1L);
  }
  if (((((i + 1L) < n) && (s_code(s_at(src, i)) == 46L)) && isDigit(s_code(s_at(src, (i + 1L)))))) {
  i = (i + 1L);
  while (((i < n) && isDigit(s_code(s_at(src, i))))) {
  i = (i + 1L);
  }
  l_push(toks, Float(s_slice(src, start, i)));
  l_push(lines, line);
  } else {
  l_push(toks, Int(num));
  l_push(lines, line);
  }
  } else {
  if (isAlpha(c)) {
  j = i;
  while (((j < n) && isAlnum(s_code(s_at(src, j))))) {
  j = (j + 1L);
  }
  l_push(toks, Ident(s_slice(src, i, j)));
  l_push(lines, line);
  i = j;
  } else {
  if (isTwoCharOp(src, i, n)) {
  l_push(toks, Punct(s_slice(src, i, (i + 2L))));
  l_push(lines, line);
  i = (i + 2L);
  } else {
  l_push(toks, Punct(s_at(src, i)));
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
  return Lexed(toks, lines);
}
long esc(long src, long j) {
  long c = 0;
  long r = 0;
  c = s_code(s_at(src, j));
  r = s_at(src, j);
  if ((c == 110L)) {
  r = (long)(intptr_t)"\n";
  }
  if ((c == 116L)) {
  r = (long)(intptr_t)"\t";
  }
  if ((c == 114L)) {
  r = (long)(intptr_t)"\r";
  }
  return r;
}
long isComment(long src, long i, long n) {
  return ((((i + 1L) < n) && (s_code(s_at(src, i)) == 47L)) && (s_code(s_at(src, (i + 1L))) == 47L));
}
long isTwoCharOp(long src, long i, long n) {
  return (twoIs(src, i, n, 45L, 62L) || twoIs(src, i, n, 61L, 61L));
}
long twoIs(long src, long i, long n, long a, long b) {
  return ((((i + 1L) < n) && (s_code(s_at(src, i)) == a)) && (s_code(s_at(src, (i + 1L))) == b));
}
long isDigit(long c) {
  return ((c >= 48L) && (c <= 57L));
}
long isAlpha(long c) {
  return ((((c >= 65L) && (c <= 90L)) || ((c >= 97L) && (c <= 122L))) || (c == 95L));
}
long isAlnum(long c) {
  return (isAlpha(c) || isDigit(c));
}
long isSpace(long c) {
  return ((((c == 32L) || (c == 9L)) || (c == 10L)) || (c == 13L));
}
