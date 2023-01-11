#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "actrie.h"

/*
//半角转全角
void half_to_full(char *sHalfStr, char *sFullStr)
{
	int i = 0, j = 0, len;
	len = strlen(sHalfStr);
	if (len == 0)
		return;
	for (i, j; i < len; i++, j++)
	{
		if (sHalfStr[i] < 0)
		{
			sFullStr[j++] = sHalfStr[i++];
			sFullStr[j] = sHalfStr[i];
		}
		else
		{
			sFullStr[j++] = 163;
			sFullStr[j] = 128 + sHalfStr[i];
		}
	}
	*(sFullStr + j) = '\0';
}
*/

/*
* ac_alloc
*
* Creates a new AC_STRUCT structure and initializes its fields.
*
* Parameters:    none.
*
* Returns:  A dynamically allocated AC_STRUCT structure.
*/
AC_STRUCT *ac_alloc(void)
{
    AC_STRUCT *node;

    if ((node = malloc(sizeof(AC_STRUCT))) == NULL) {
        return NULL;
    }
    memset(node, 0, sizeof(AC_STRUCT));

    if ((node->root = malloc(sizeof(TSNode))) == NULL) {
        free(node);
        return NULL;
    }
    memset(node->root, 0, sizeof(TSNode));

    return node;
}

/*必须先插短的再插长的，例如先插AB再插ABC，反过来插还没处理
* ac_add_string
*
* Adds a string to the AC_STRUCT structure's keyword tree.
*
* NOTE:  The `id' value given must be unique to any of the strings
*        added to the tree, and must be a small integer greater than
*        0 (since it is used to index an array holding information
*        about each of the strings).
*
*        The best id's to use are to number the strings from 1 to K.
*
* Parameters:   node      -  an AC_STRUCT structure
*               P         -  the sequence
*               M         -  the sequence length
*               id        -  the sequence identifier
*
* Returns:  non-zero on success, zero on error.
*/
int ac_add_string(AC_STRUCT *node, char *P, int M, int id)
{
    int i = 0, j, flag;
    TSTree currentNode, preNode, newNode;

    if (id == 0 || id <= node->pNum) {
        return 0;
    }

    if (node->pNum == 0) { //还未添加模式串
        currentNode = node->root;

        for (i; i < M; i++) {
            if ((newNode = malloc(sizeof(TSNode))) == NULL) {
                return -1;
            }
            memset(newNode, 0, sizeof(TSNode));
            newNode->data = P[i];
            if (i == M - 1) {
                newNode->ID = id;   //记录 模式串的 下标
            } else {
                newNode->ID = 0;
            }
            currentNode->next = newNode;
            currentNode = currentNode->next;
            
        }

        node->pNum++;
        return 1;
    }

    currentNode = preNode = node->root->next;
    while (currentNode != NULL && i < M) {
        if (P[i] == currentNode->data) {
            i++;
            preNode = currentNode;
            currentNode = currentNode->next;
            flag = 0;
        } else if (P[i] < currentNode->data) {
            preNode = currentNode;
            currentNode = currentNode->lchild;
            flag = 1;
        } else {
            preNode = currentNode;
            currentNode = currentNode->rchild;
            flag = 2;
        }
    }

    //先插入长串，再插入短串，比如先插AABAB，再插入AAB
    if (i == M) {
        preNode->ID = id;   //记录 模式串的 下标
    }

    if ((newNode = malloc(sizeof(TSNode))) == NULL) {
        return -1;
    }

    memset(newNode, 0, sizeof(TSNode));
    newNode->data = P[i];

    if (i == M - 1) {
        newNode->ID = id;   //记录 模式串的 下标
    } else {
        newNode->ID = 0;
    }

    currentNode = preNode;
    if (flag == 0) {
        currentNode->next = newNode;
        currentNode = currentNode->next;
        i++;
    } else if (flag == 1) {
        currentNode->lchild = newNode;
        currentNode = currentNode->lchild;
        i++;
    } else {
        currentNode->rchild = newNode;
        currentNode = currentNode->rchild;
        i++;
    }

    //插入完毕则返回
    if (i == M) {
        node->pNum++;
        return 1;
    }

    //将剩余字符插入
    for (i; i < M; i++) {
        if ((newNode = malloc(sizeof(TSNode))) == NULL) {
            return -1;
        }
        memset(newNode, 0, sizeof(TSNode));
        newNode->data = P[i];
        if (i == M - 1) {
            newNode->ID = id;
        } else {
            newNode->ID = 0;
        }
        currentNode->next = newNode;
        currentNode = currentNode->next;
    }

    node->pNum++;

    return 1;
}

//对trie树添加faillink，完成outputlink，实现自动机
int ac_implement(AC_STRUCT *node)
{
    TSTree root, currentNode, preNode, lchild, rchild, state, tempNode;
    root = node->root;
    int flag;
    Queue *q1, *q2, *q3;

    q1 = que_init(); //主要队列
    q2 = que_init(); //辅助队列
    q3 = que_init(); //辅助队列
    
    if (root->next != NULL) {
        currentNode = root->next;
        if (!enqueue(q2, currentNode)) {
            return 0;
        }

        while (!empty(q2)) {
            currentNode = dequeue(q2);
            currentNode->faillink = root; //将所有的深度为1的节点的失效函数指向root
            enqueue(q1, currentNode);
            lchild = currentNode->lchild;
            rchild = currentNode->rchild;
            if (lchild != NULL)
                enqueue(q2, lchild);
            if (rchild != NULL)
                enqueue(q2, rchild);
        } // 此时q1中是所有的深度为1的结点，q2为空

        //q1中每取出一个深度为n（n>=1）的点，就会将该点对应的深度为n+1的点入列，并计算其失效节点
        while (!empty(q1)) {
            //取出一个深度为n的点
            preNode = dequeue(q1);
            if (preNode->next != NULL) {
                //依次将所有深度为n+1的点入列，计算其faillink
                enqueue(q2, preNode->next);

                while (!empty(q2)) {
                    currentNode = dequeue(q2);
                    lchild = currentNode->lchild;
                    rchild = currentNode->rchild;
                    if (lchild != NULL) {
                        enqueue(q2, lchild);
                    }

                    if (rchild != NULL) {
                        enqueue(q2, rchild);
                    }

                    enqueue(q1, currentNode);

                    //计算currentNode的faillink
                    flag = 1;
                    state = preNode; //取出的深度为n的点
                    //如果在n层的失效节点的下一层没发现，则寻找n层的失效节点的失效节点的下一层，直到失效节点为根节点
                    while (flag) {
                        //寻找最近层的相等节点
                        state = state->faillink; //第一次循环是深度为n的点的失效节点
                        if (state->next != NULL) {
                            enqueue(q3, state->next);
                            while (!empty(q3)) {
                                tempNode = dequeue(q3);
                                lchild = tempNode->lchild;
                                rchild = tempNode->rchild;
                                if (lchild != NULL) {
                                    enqueue(q3, lchild);
                                }
                                if (rchild != NULL) {
                                    enqueue(q3, rchild);
                                }
                                if (tempNode->data == currentNode->data) {
                                    currentNode->faillink = tempNode;
                                    flag = 0;
                                }
                            }
                        }

                        if (state == root && flag == 1) {
                            currentNode->faillink = root;
                            break;
                        }
                    } //end while(flag)
                }     //end q2
            }
        } //end q1
    }

    return 1;
}

//搜索之前对AC自动机初始化
void search_init(AC_STRUCT *node, long strNum, char *searchStr)
{
    int i,j;
    if (node->match_table != NULL) {
        free(node->match_table);
    }

    node->startPoint = node->startPoint + node->strNum;
    node->strNum = strNum;
    node->searchStr = searchStr;
    node->currentPoint = 0;

    node->match_table = malloc(node->pNum*sizeof(Ac_match_info));
    for (i = 0; i < node->pNum; i++) {
        for (j = 0; j < POS_RECORD_NUM; j++) {
            node->match_table[i].pos[j] = -1;
        }
    }

    return;
}

//AC搜索
int ac_search(AC_STRUCT *node)
{
    long i, position;
    long j = 0;
    int flag;
    char *searchStr = node->searchStr;
    TSTree currentNode, child, faillinkout, currentNode1;

    if (node->startPoint == 0) {
        node->currentNode = node->root;
    }
    currentNode = node->currentNode;


    for (i = 0; i < node->strNum; i++) {
        child = currentNode->next;

        while (currentNode != node->root || child != NULL) {
            if (child == NULL) {
                currentNode = currentNode->faillink;
                child = currentNode->next;
                continue;
            }

            if (child->data == searchStr[i]) {
                currentNode = child;
                node->currentNode = child;
                node->currentPoint = i + 1;

                {
                    currentNode1 = node->currentNode;
                    //不管该点的ID是否为零，都应检查其失效节点以及失效节点的失效节点...的ID是否为零，直到root
                    while (currentNode1 != node->root) {
                        if (currentNode1->ID != 0) {

                            for (j = 0; j < POS_RECORD_NUM; j++) {
                                if (node->match_table[node->currentNode->ID-1].pos[j] != 0) {
                                    node->match_table[node->currentNode->ID-1].pos[j] = node->currentPoint;
                                }
                            }

                        }
                        currentNode1 = currentNode1->faillink;
                    }
                }

                break;
            } else if (searchStr[i] > child->data) {
                child = child->rchild;
            } else {
                child = child->lchild;
            }
        }
    }
}

//存储树节点的队列，辅助实现faillink
Queue *que_init()
{
    Queue *q;
    if ((q = malloc(sizeof(Queue))) == NULL) {
        return NULL;
    }

    memset(q, 0, sizeof(Queue));
    q->head = 0;
    q->tail = 0;

    return q;
}

//队列判空
int empty(Queue *q)
{
    if (q->head == q->tail) {
        return 1;
    }

    return 0;
}

//入队，成功返回1，否则返回0
int enqueue(Queue *q, TSTree node)
{
    if ((q->head - q->tail + maxq) % maxq == 1) {
        return 0;
    } else {
        q->queue[q->tail] = node;
        q->tail = (q->tail + 1) % maxq;
        return 1;
    }
}

//出队，返回出队节点或NULL（队空）
TSTree dequeue(Queue *q)
{
    if (q->head == q->tail) {
        return NULL;
    } else {
        int head = q->head;
        q->head = (head + 1) % maxq;
        return q->queue[head];
    }
}

//先根序遍历 ，调试用
void preorder(TSTree node)
{
    TSTree currentNode;
    currentNode = node;
    if (currentNode != NULL) {
        printf("%c %d...", currentNode->data, currentNode->ID);

        if (currentNode->faillink != NULL) {
            printf("%c %d\n", currentNode->faillink->data, currentNode->faillink->ID);
        } else {
            printf("\n");
        }

        preorder(currentNode->lchild);
        preorder(currentNode->next);
        preorder(currentNode->rchild);
    }
}