#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Input the team name */
    "Malloc Lab",
    /* Input the name  of member1 */
    "Li Zitong",
    /* Input the student-ID of member1  */
    "2017202121",
    /* Input the name of member2 if any */
    "None",
    /* Input the student-ID of member2 if any */
    "None"
};

/* 对齐 */
#define ALIGNMENT 8
#define ALIGN(size) ((((size) + (ALIGNMENT-1)) / (ALIGNMENT)) * (ALIGNMENT))

#define WSIZE     4
#define DSIZE     8

#define LISTSIZE     16

#define MAX(x, y) ((x) > (y) ? (x) : (y))

#define PACK(size, alloc) ((size) | (alloc))

#define GET(p)            (*(unsigned int *)(p))
#define PUT(p, val)       (*(unsigned int *)(p) = (val))

#define SET_PTR(p, ptr) (*(unsigned int *)(p) = (unsigned int)(ptr))

#define GET_SIZE(p)  (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

#define HDRP(ptr) ((char *)(ptr) - WSIZE)
#define FTRP(ptr) ((char *)(ptr) + GET_SIZE(HDRP(ptr)) - DSIZE)

#define NEXT_BLKP(ptr) ((char *)(ptr) + GET_SIZE((char *)(ptr) - WSIZE))
#define PREV_BLKP(ptr) ((char *)(ptr) - GET_SIZE((char *)(ptr) - DSIZE))

/* 新增：链表中指向前后块的指针 */ 
#define PRED_PTR(ptr) ((char *)(ptr))
#define SUCC_PTR(ptr) ((char *)(ptr) + WSIZE)

#define PRED(ptr) (GET(PRED_PTR(ptr)))
#define SUCC(ptr) (GET(SUCC_PTR(ptr)))

/* 扩展堆的大小 */
#define INITCHUNKSIZE (1<<6)
#define CHUNKSIZE (1<<12)

/* 链表操作 */
#define READ_LIST_HEAD(x)  ((mem_heap_lo())+(x)*DSIZE)
#define SET_LIST_HEAD(x,val) (SET_PTR((mem_heap_lo())+(x)*DSIZE, val))

/* 扩展推 */
static void *extend_heap(size_t size);
/* 合并free block */
static void *coalesce(void *ptr);
static void *place(void *ptr, size_t size);
/* 初始化空闲链表表头 */
static void init_list_head();
/* 读取空闲链表表头 */ 
/*static void *READ_LIST_HEAD(int x); */
/* 设置空闲链表表头 
static void SET_LIST_HEAD(int x, void *val);*/
/* 将ptr所指向的free block插入到分离空闲表中 */
static void add_node(void *ptr, size_t size);
/* 将ptr所指向的块从分离空闲表中删除 */
static void delete_node(void *ptr);

static void *extend_heap(size_t size)
{
    void *ptr;
    
    size = ALIGN(size);
    
    if ((ptr = mem_sbrk(size)) == (void *)-1)
        return NULL;

    /* 设置扩展堆的头和尾 */
    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));
    /* 结尾块 */
    PUT(HDRP(NEXT_BLKP(ptr)), PACK(0, 1));
    
    add_node(ptr, size);
    
    return coalesce(ptr);
}

static void init_list_head()
{ 
	int k;  /* 循环变量 */ 
    char *heap;
	
    /* 初始化空闲链表 */
    if ((long)(heap = mem_sbrk(LISTSIZE * DSIZE)) == -1){
    	printf( "Initialize list failure.\n" );
    	return;
	}
	
	for (k = 0; k < LISTSIZE; k++)
    {
        SET_PTR(heap + (k * DSIZE), NULL);
    }
    
    return;
}

/* 返回链表头的地址  
static void *READ_LIST_HEAD(int x)
{
	void *ptr;
	ptr = mem_heap_lo()+x*DSIZE;
	return ptr;
} 
*/

/*
static void SET_LIST_HEAD(int x, void *val)
{
	void *ptr;
	ptr = mem_heap_lo()+x*DSIZE;
	SET_PTR(ptr, val);
	return;
}
*/

static void add_node(void *ptr, size_t size)
{
    int i = 0;
    /* cursor1用于查找链表节点 */ 
    void *cursor1 = NULL;
    /* cursor2用于插入链表节点*/ 
    void *cursor2 = NULL;

    /* 找对应的链头 */
    while ((i < LISTSIZE - 1) && (size > 1))
    {
        size >>= 1;
        i++;
    }

    /* 找到对应的链后继续寻找对应的插入位置 */
    void *find_head = READ_LIST_HEAD(i);
    cursor1 = GET(find_head);
    
    while ((cursor1 != NULL) && (size > GET_SIZE(HDRP(cursor1))))
    {
        cursor2 = cursor1;
        cursor1 = PRED(cursor1);
    }

    /* 循环后有四种情况 */
    if (cursor1 != NULL)
    {
        /* 开头插入 */
        if (cursor2 == NULL)
        {            
            SET_PTR(PRED_PTR(ptr), cursor1);
            SET_PTR(SUCC_PTR(cursor1), ptr);
            SET_PTR(SUCC_PTR(ptr), NULL);
            SET_LIST_HEAD(i, ptr);
        }
        /* 中间插入 */
        else
        {
            SET_PTR(PRED_PTR(ptr), cursor1);
            SET_PTR(SUCC_PTR(cursor1), ptr);
            SET_PTR(SUCC_PTR(ptr), cursor2);
            SET_PTR(PRED_PTR(cursor2), ptr);
        }
    }
    else
    {
        if (cursor2 == NULL)
        { /* 空链表 */
        	SET_PTR(PRED_PTR(ptr), NULL);
            SET_PTR(SUCC_PTR(ptr), NULL);
            SET_LIST_HEAD(i, ptr);            
        }
        else
        { /* 结尾插入 */
            SET_PTR(PRED_PTR(ptr), NULL);
            SET_PTR(SUCC_PTR(ptr), cursor2);
            SET_PTR(PRED_PTR(cursor2), ptr);
        }
    }
}

static void delete_node(void *ptr)
{
    int i = 0;
    size_t size = GET_SIZE(HDRP(ptr));

    /* 通过块的大小找到对应的链 */
    while ((i < LISTSIZE - 1) && (size > 1))
    {
        size >>= 1;
        i++;
    }

    /* 根据这个块的情况分四种可能性 */
    if (PRED(ptr) != NULL)
    {
        /* 表头删除 */
        if (SUCC(ptr) == NULL)
        {            
            SET_PTR(SUCC_PTR(PRED(ptr)), NULL);
            SET_LIST_HEAD(i, PRED(ptr));
        }
        /* 中间删除 */
        else
        {
            SET_PTR(SUCC_PTR(PRED(ptr)), SUCC(ptr));
            SET_PTR(PRED_PTR(SUCC(ptr)), PRED(ptr));
        }
    }
    else
    {
        /* 末尾删除 */
        if (SUCC(ptr) == NULL)
        {	
			SET_LIST_HEAD(i, NULL);
        }
        /* 空表删除 */
        else
        {
            SET_PTR(PRED_PTR(SUCC(ptr)), NULL);
        }
    }
}

static void *coalesce(void *ptr)
{
    size_t prev_alloc = GET_ALLOC(HDRP(PREV_BLKP(ptr)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(ptr)));
    size_t size = GET_SIZE(HDRP(ptr));
    
    /* 前后均为allocated块 */
    if (prev_alloc && next_alloc)
    {
        return ptr;
    }
    /* 前面allocated，后面free */
    else if (prev_alloc && !next_alloc)
    {
        delete_node(ptr);
        delete_node(NEXT_BLKP(ptr));
        size += GET_SIZE(HDRP(NEXT_BLKP(ptr)));
        PUT(HDRP(ptr), PACK(size, 0));
        PUT(FTRP(ptr), PACK(size, 0));
    }
    /* 后面allocated，前面free */
    else if (!prev_alloc && next_alloc)
    {
        delete_node(ptr);
        delete_node(PREV_BLKP(ptr));
        size += GET_SIZE(HDRP(PREV_BLKP(ptr)));
        PUT(FTRP(ptr), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(ptr)), PACK(size, 0));
        ptr = PREV_BLKP(ptr);
    }
    /* 前后都free */
    else
    {
        delete_node(ptr);
        delete_node(PREV_BLKP(ptr));
        delete_node(NEXT_BLKP(ptr));
        size += GET_SIZE(HDRP(PREV_BLKP(ptr))) + GET_SIZE(HDRP(NEXT_BLKP(ptr)));
        PUT(HDRP(PREV_BLKP(ptr)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(ptr)), PACK(size, 0));
        ptr = PREV_BLKP(ptr);
    }

    add_node(ptr, size);

    return ptr;
}

static void *place(void *ptr, size_t size)
{
    size_t ptr_size = GET_SIZE(HDRP(ptr));
    /* 分配空间后剩余的空间大小 */
    size_t left = ptr_size - size;

    delete_node(ptr);

    /* 剩余的大小小于最小块，则不分离原块 */
    if (left < DSIZE * 2)
    {
        PUT(HDRP(ptr), PACK(ptr_size, 1));
        PUT(FTRP(ptr), PACK(ptr_size, 1));
    }
    else if (size >= 110)
    {
        PUT(HDRP(ptr), PACK(left, 0));
        PUT(FTRP(ptr), PACK(left, 0));
        
		PUT(HDRP(NEXT_BLKP(ptr)), PACK(size, 1));
        PUT(FTRP(NEXT_BLKP(ptr)), PACK(size, 1));
        
		add_node(ptr, left);
       
	    return NEXT_BLKP(ptr);
    }
    else
    {
        PUT(HDRP(ptr), PACK(size, 1));
        PUT(FTRP(ptr), PACK(size, 1));
        
		PUT(HDRP(NEXT_BLKP(ptr)), PACK(left, 0));
        PUT(FTRP(NEXT_BLKP(ptr)), PACK(left, 0));
        
		add_node(NEXT_BLKP(ptr), left);
    }
    
	return ptr;
}

int mm_init(void)
{
	char *heap; 
    init_list_head();
    
    /* 初始化堆 */
    if ((long)(heap = mem_sbrk(4 * WSIZE)) == -1)
        return -1;

    PUT(heap, 0);
    PUT(heap + (1 * WSIZE), PACK(DSIZE, 1));
    PUT(heap + (2 * WSIZE), PACK(DSIZE, 1));
    PUT(heap + (3 * WSIZE), PACK(0, 1));

    /* 扩展堆 */
    if (extend_heap(INITCHUNKSIZE) == NULL)
        return -1;

    return 0;
}

void *mm_malloc(size_t size)
{
    if (size == 0)
        return NULL;
    /* 对齐 */
    if (size <= DSIZE)
    {
        size = 2 * DSIZE;
    }
    else
    {
        size = ALIGN(size + DSIZE);
    }


    int i = 0;
    size_t tmpsize = size;
    void *ptr = NULL;

    while (i < LISTSIZE)
    {
        /* 寻找对应链 */
        void * tmp = READ_LIST_HEAD( i );
        
        if (((tmpsize <= 1) && ( GET(tmp) != NULL)))
        {
            ptr = GET(tmp) ;
            /* 寻找大小合适的free块 */
            while ((ptr != NULL) && ((size > GET_SIZE(HDRP(ptr)))))
            {
                ptr = PRED(ptr);
            }
            
            if (ptr != NULL)
                break;
        }

        tmpsize >>= 1;
        i++;
    }

    /* 没有找到 */
    if (ptr == NULL)
    {
        if ((ptr = extend_heap(MAX(size, CHUNKSIZE))) == NULL)
            return NULL;
    }

    ptr = place(ptr, size);

    return ptr;
}

void mm_free(void *ptr)
{
    size_t size = GET_SIZE(HDRP(ptr));

    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));

    add_node(ptr, size);
    
    coalesce(ptr);
}

void *mm_realloc(void *ptr, size_t size)
{
    void *new_block = ptr;
    int left;

    if (size == 0)
        return NULL;
	
    if (size <= DSIZE)
    {
        size = 2 * DSIZE;
    }
    else
    {
        size = ALIGN(size + DSIZE);
    }

    /* 如果size小于原来块的大小，返回原来块 */
    if ((left = GET_SIZE(HDRP(ptr)) - size) >= 0)
    {
        return ptr;
    }
    /* 检查下一个块是free块或结束块 */
    else if (!GET_ALLOC(HDRP(NEXT_BLKP(ptr))) || !GET_SIZE(HDRP(NEXT_BLKP(ptr))))
    {
        /* 如果加上后面的free块也不够，扩展 */
        if ((left = GET_SIZE(HDRP(ptr)) + GET_SIZE(HDRP(NEXT_BLKP(ptr))) - size) < 0)
        {
            if (extend_heap(MAX(-left, CHUNKSIZE)) == NULL)
                return NULL;
            left += MAX(-left, CHUNKSIZE);
        }

        /* 删除使用的free块，设置新块的头尾 */
        delete_node(NEXT_BLKP(ptr));
        PUT(HDRP(ptr), PACK(size + left, 1));
        PUT(FTRP(ptr), PACK(size + left, 1));
    }
    /* 没有可以利用的连续free块 */
    else
    {
        new_block = mm_malloc(size);
        memcpy(new_block, ptr, GET_SIZE(HDRP(ptr)));
        mm_free(ptr);
    }

    return new_block;
}
