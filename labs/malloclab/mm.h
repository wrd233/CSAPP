#include <stdio.h>

extern int mm_init (void);
extern void *mm_malloc (size_t size);
extern void mm_free (void *ptr);
extern void *mm_realloc(void *ptr, size_t size);

// 调试代码
int mm_check(void);
// 扩展堆并返回指向新空闲块的指针
static void *extend_heap(size_t words);
/* 合并空闲块的函数 */
static void *coalesce(void *bp);
// 实现块的放置策略
static void *find_fit(size_t asize);
// 实现块的分割策略
static void place(void *bp, size_t asize);

/* 
 * Students work in teams of one or two.  Teams enter their team name, 
 * personal names and login IDs in a struct of this
 * type in their bits.c file.
 */
typedef struct {
    char *teamname; /* ID1+ID2 or ID1 */
    char *name1;    /* full name of first member */
    char *id1;      /* login ID of first member */
    char *name2;    /* full name of second member (if any) */
    char *id2;      /* login ID of second member */
} team_t;

extern team_t team;

