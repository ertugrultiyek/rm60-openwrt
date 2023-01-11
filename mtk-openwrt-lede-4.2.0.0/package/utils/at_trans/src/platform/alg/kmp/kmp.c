////////////////////////////////KMP//////////////////////////////////////////////
//优化过后的next 数组求法
/*
function:
        KMP的next数组求解（预处理）
Param:
@p 需要匹配的字符串
@next 需要匹配的字符串对应的next数组
*/
void hlk_alg_KMPPre(char* p, int pLen, int KMP_next[])
{
    KMP_next[0] = -1;
    int k = -1;
    int j = 0;
    while (j < pLen-1) {
        //p[k]表示前缀，p[j]表示后缀  
        if (k == -1 || p[j] == p[k]) {
            ++j;
            ++k;
            if (p[j] != p[k]) {
                KMP_next[j]=k;
            } else if (KMP_next[k] != -1) {//若是k是0的话，它的next[0]是-1，
                KMP_next[j] = KMP_next[k];
            } else {
                KMP_next[j] = 0;
            }
        } else {
            k = KMP_next[k];
        }
    }

    return;
}

/*
function:
        KMP字符匹配算法
Param:
@s 文本内容
@sLen 文本内容长度
@p 需要匹配的字符串
@pLen 需要匹配的字符串长度
@next[] 辅助数组
*/
char *hlk_alg_KmpSearch(char *s, int sLen, char* p, int pLen, int next[])
{
    int i = 0;
    int j = 0;
    while(i < sLen && j < pLen) {
        //①如果j=-1（代表又回到了P串的开头第一个字符，因为next[0]=-1），或者当前字符匹配成功（即S[i]==P[j]），都令i++，j++    
        if (j == -1 || s[i] == p[j]) {
            i++;
            j++;
        } else {
            //②如果j!=-1，且当前字符匹配失败，则令i不变(当前s串失配的地方)，j=next[j]    
            //next[j]即为j所对应的next值（其实就是和它含有相同前缀的地方，比如P为"AFHKAFOIU"，则next[6]=2,即第7个字符“O”的前两个字符"AF"(第五、六个字符)有相同前缀"AF"(第一、二个字符))
            j = next[j];
        }
    }

    if (j == pLen) {
        return &s[i-j];
    } else {
        return NULL;
    }
}
