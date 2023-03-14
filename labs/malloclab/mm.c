/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"
#include "macro.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "ateam",
    /* First member's full name */
    "Harry Bovik",
    /* First member's email address */
    "bovik@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

// 定义常量
#define WSIZE 4         // 字长和头部/尾部大小（字节）
#define DSIZE 8         // 双字大小（字节）
#define CHUNKSIZE (1<<12)// 初始空闲块的大小（字节）

// 定义宏函数，获取较大值
#define MAX(x, y) ((x) > (y)? (x) : (y))

// 定义宏函数，将大小和分配位打包成一个字(方便对空闲块的头部/尾部操作)
#define PACK(size, alloc) ((size) | (alloc))

// 定义宏函数，读取和写入地址p处的字
#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

// 定义宏函数，从地址p读取块的大小和已分配标志(注：此处的p理应指向块的起始地址，而非块的有效载荷)
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

// 定义宏函数，给定块指针bp，计算其头和尾的地址
#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

// 定义宏函数，给定块指针bp，计算其后继和前驱块的地址
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))


static char *heap_listp; //指向分配器维护的链表的起始块的指针

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    // 创建初始的空堆
    if ((heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1)
        return -1;

    // 为堆分配空间并设置头部
    PUT(heap_listp, 0);                                 // 堆的起始位置
    PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1));        // prologue header
    PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1));        // prologue footer
    PUT(heap_listp + (3*WSIZE), PACK(0, 1));            // epilogue header
    heap_listp += (2*WSIZE);                            // 堆指针指向 prologue block 的地址(注意，不是指向prologue的header)

    // 扩展空堆的大小，增加一个 CHUNKSIZE 大小的初始空闲块
    if (extend_heap(CHUNKSIZE/WSIZE) == NULL)
        return -1;

    return 0;
}

// 扩展堆并返回指向新空闲块的指针
static void *extend_heap(size_t words)
{
    char *bp;
    size_t size;

    // 为了保持内存对齐，分配偶数个字
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;

    // 初始化新空闲块的头部和尾部
    PUT(HDRP(bp), PACK(size, 0));             // 空闲块头部
    PUT(FTRP(bp), PACK(size, 0));             // 空闲块尾部
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));     // 新的 epilogue 块头部

    // 如果新生成的空闲块的前一个块是空闲的，则合并前一个块和新块
    return coalesce(bp);
}

/* 合并空闲块的函数 */
static void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp))); // 获取前一个块的分配状态
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp))); // 获取后一个块的分配状态
    size_t size = GET_SIZE(HDRP(bp)); // 获取当前块的大小

    if (prev_alloc && next_alloc) { // 前后均为已分配状态
        return bp; // 直接返回当前块的地址
    }
    else if (prev_alloc && !next_alloc) { // 后一个块为未分配状态
        size += GET_SIZE(HDRP(NEXT_BLKP(bp))); // 更新当前块大小
        PUT(HDRP(bp), PACK(size, 0)); // 设置新的头部
        PUT(FTRP(bp), PACK(size, 0)); // 设置新的尾部
    }
    else if (!prev_alloc && next_alloc) { // 前一个块为未分配状态
        size += GET_SIZE(HDRP(PREV_BLKP(bp))); // 更新当前块大小
        PUT(FTRP(bp), PACK(size, 0)); // 设置新的尾部
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0)); // 设置新的头部
        bp = PREV_BLKP(bp); // 更新块指针
    }
    else { // 前后均为未分配状态
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp))); // 更新当前块大小
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0)); // 设置新的头部
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0)); // 设置新的尾部
        bp = PREV_BLKP(bp); // 更新块指针
    }

    return bp; // 返回合并后的块指针
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    /*原始代码*/
    // int newsize = ALIGN(size + SIZE_T_SIZE);   // 调整请求的内存大小为 ALIGNMENT 的倍数，并加上一个字节的头部大小
    // void *p = mem_sbrk(newsize);    // 通过调用 mem_sbrk 函数分配内存
    // if (p == (void *)-1)   // 如果 mem_sbrk 分配失败，返回 NULL
    //     return NULL;
    // else {
    //     *(size_t *)p = size;   // 写入块的大小信息，存放在第一个字节中
    //     return (void *)((char *)p + SIZE_T_SIZE);  // 返回内存块的地址，跳过头部字节，因为返回的指针是指向有效载荷的
    // }

  	size_t asize; // 调整后的块大小  
  	size_t extendsize; // 需要扩展堆的大小  
  	char *bp;

  	// 忽略无意义的请求
	if (size == 0)
    	return NULL;

  	/* 调整块大小以包括开销和对齐要求 */
  	if (size <= DSIZE)
    	asize = 2*DSIZE ;  
  	else  
    	asize = DSIZE * ((size+ (DSIZE) + (DSIZE-1)) / DSIZE) ;  

  	// 在空闲块中查找合适的块  
  	if ( (bp = find_fit (asize)) != NULL) {  
    	place(bp, asize);  
    	return bp;  
  	}  

  	// 如果找不到合适的块，则需要扩展堆  
  	extendsize = MAX(asize,CHUNKSIZE);  
  	if ( (bp = extend_heap (extendsize/WSIZE)) == NULL)  
    	return NULL;  
  	place(bp, asize);  
  	return bp;  
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    size_t size = GET_SIZE(HDRP(ptr)); // 获取当前块的大小

    PUT(HDRP(ptr), PACK(size, 0)); // 设置头部为未分配状态
    PUT(FTRP(ptr), PACK(size, 0)); // 设置尾部为未分配状态

    coalesce(ptr); // 合并空闲块
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    
    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}


/*
 * mm_check - 检查堆的正确性
 */
int mm_check(void){
    // 输出 heap_listp 的位置
    INFO("heap_listp地址: %p",heap_listp)
    return 0;
}

// 实现块的放置策略
static void *find_fit(size_t asize){
    // 策略:遍历隐式空闲链表，找到第一个能够放下这个的块
    for(void* tmp=heap_listp; GET_SIZE(tmp-WSIZE)>0; tmp=NEXT_BLKP(tmp)){
        // 如果已分配，那么跳过
        if(GET_ALLOC(tmp-WSIZE)){
            continue;
        }
        // 如果未分配，但有效荷载不足，那么跳过
        if(GET_SIZE(tmp-WSIZE)<asize){
            continue;
        }
        // 确定第一个可用的空闲块，将alloc置位
        PUT(HDRP(tmp), PACK(GET_SIZE(tmp-WSIZE), 1));             // 空闲块头部
        PUT(FTRP(tmp), PACK(GET_SIZE(tmp-WSIZE), 1));             // 空闲块尾部
        // 返回第一个可用的空闲块
        return tmp;
    }

    return NULL;
}

// 实现块的分割策略
static void place(void *bp, size_t asize){
    // 策略:不分割，使用整个空闲块
    return bp;
}













