#ifndef _H_HLK_ALG_KMP_H_
#define _H_HLK_ALG_KMP_H_
extern void hlk_alg_KMPPre(char* p, int plen, int KMP_next[]);
extern char *hlk_alg_KmpSearch(char *s, int sLen, char* p, int pLen, int next[]);
#endif