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

/* ���� */
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

/* ������������ָ��ǰ����ָ�� */ 
#define PRED_PTR(ptr) ((char *)(ptr))
#define SUCC_PTR(ptr) ((char *)(ptr) + WSIZE)

#define PRED(ptr) (GET(PRED_PTR(ptr)))
#define SUCC(ptr) (GET(SUCC_PTR(ptr)))

/* ��չ�ѵĴ�С */
#define INITCHUNKSIZE (1<<6)
#define CHUNKSIZE (1<<12)

/* ������� */
#define READ_LIST_HEAD(x)  ((mem_heap_lo())+(x)*DSIZE)
#define SET_LIST_HEAD(x,val) (SET_PTR((mem_heap_lo())+(x)*DSIZE, val))

/* ��չ�� */
static void *extend_heap(size_t size);
/* �ϲ�free block */
static void *coalesce(void *ptr);
static void *place(void *ptr, size_t size);
/* ��ʼ�����������ͷ */
static void init_list_head();
/* ��ȡ���������ͷ */ 
/*static void *READ_LIST_HEAD(int x); */
/* ���ÿ��������ͷ 
static void SET_LIST_HEAD(int x, void *val);*/
/* ��ptr��ָ���free block���뵽������б��� */
static void add_node(void *ptr, size_t size);
/* ��ptr��ָ��Ŀ�ӷ�����б���ɾ�� */
static void delete_node(void *ptr);

static void *extend_heap(size_t size)
{
    void *ptr;
    
    size = ALIGN(size);
    
    if ((ptr = mem_sbrk(size)) == (void *)-1)
        return NULL;

    /* ������չ�ѵ�ͷ��β */
    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));
    /* ��β�� */
    PUT(HDRP(NEXT_BLKP(ptr)), PACK(0, 1));
    
    add_node(ptr, size);
    
    return coalesce(ptr);
}

static void init_list_head()
{ 
	int k;  /* ѭ������ */ 
    char *heap;
	
    /* ��ʼ���������� */
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

/* ��������ͷ�ĵ�ַ  
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
    /* cursor1���ڲ�������ڵ� */ 
    void *cursor1 = NULL;
    /* cursor2���ڲ�������ڵ�*/ 
    void *cursor2 = NULL;

    /* �Ҷ�Ӧ����ͷ */
    while ((i < LISTSIZE - 1) && (size > 1))
    {
        size >>= 1;
        i++;
    }

    /* �ҵ���Ӧ���������Ѱ�Ҷ�Ӧ�Ĳ���λ�� */
    void *find_head = READ_LIST_HEAD(i);
    cursor1 = GET(find_head);
    
    while ((cursor1 != NULL) && (size > GET_SIZE(HDRP(cursor1))))
    {
        cursor2 = cursor1;
        cursor1 = PRED(cursor1);
    }

    /* ѭ������������� */
    if (cursor1 != NULL)
    {
        /* ��ͷ���� */
        if (cursor2 == NULL)
        {            
            SET_PTR(PRED_PTR(ptr), cursor1);
            SET_PTR(SUCC_PTR(cursor1), ptr);
            SET_PTR(SUCC_PTR(ptr), NULL);
            SET_LIST_HEAD(i, ptr);
        }
        /* �м���� */
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
        { /* ������ */
        	SET_PTR(PRED_PTR(ptr), NULL);
            SET_PTR(SUCC_PTR(ptr), NULL);
            SET_LIST_HEAD(i, ptr);            
        }
        else
        { /* ��β���� */
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

    /* ͨ����Ĵ�С�ҵ���Ӧ���� */
    while ((i < LISTSIZE - 1) && (size > 1))
    {
        size >>= 1;
        i++;
    }

    /* ������������������ֿ����� */
    if (PRED(ptr) != NULL)
    {
        /* ��ͷɾ�� */
        if (SUCC(ptr) == NULL)
        {            
            SET_PTR(SUCC_PTR(PRED(ptr)), NULL);
            SET_LIST_HEAD(i, PRED(ptr));
        }
        /* �м�ɾ�� */
        else
        {
            SET_PTR(SUCC_PTR(PRED(ptr)), SUCC(ptr));
            SET_PTR(PRED_PTR(SUCC(ptr)), PRED(ptr));
        }
    }
    else
    {
        /* ĩβɾ�� */
        if (SUCC(ptr) == NULL)
        {	
			SET_LIST_HEAD(i, NULL);
        }
        /* �ձ�ɾ�� */
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
    
    /* ǰ���Ϊallocated�� */
    if (prev_alloc && next_alloc)
    {
        return ptr;
    }
    /* ǰ��allocated������free */
    else if (prev_alloc && !next_alloc)
    {
        delete_node(ptr);
        delete_node(NEXT_BLKP(ptr));
        size += GET_SIZE(HDRP(NEXT_BLKP(ptr)));
        PUT(HDRP(ptr), PACK(size, 0));
        PUT(FTRP(ptr), PACK(size, 0));
    }
    /* ����allocated��ǰ��free */
    else if (!prev_alloc && next_alloc)
    {
        delete_node(ptr);
        delete_node(PREV_BLKP(ptr));
        size += GET_SIZE(HDRP(PREV_BLKP(ptr)));
        PUT(FTRP(ptr), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(ptr)), PACK(size, 0));
        ptr = PREV_BLKP(ptr);
    }
    /* ǰ��free */
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
    /* ����ռ��ʣ��Ŀռ��С */
    size_t left = ptr_size - size;

    delete_node(ptr);

    /* ʣ��Ĵ�СС����С�飬�򲻷���ԭ�� */
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
    
    /* ��ʼ���� */
    if ((long)(heap = mem_sbrk(4 * WSIZE)) == -1)
        return -1;

    PUT(heap, 0);
    PUT(heap + (1 * WSIZE), PACK(DSIZE, 1));
    PUT(heap + (2 * WSIZE), PACK(DSIZE, 1));
    PUT(heap + (3 * WSIZE), PACK(0, 1));

    /* ��չ�� */
    if (extend_heap(INITCHUNKSIZE) == NULL)
        return -1;

    return 0;
}

void *mm_malloc(size_t size)
{
    if (size == 0)
        return NULL;
    /* ���� */
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
        /* Ѱ�Ҷ�Ӧ�� */
        void * tmp = READ_LIST_HEAD( i );
        
        if (((tmpsize <= 1) && ( GET(tmp) != NULL)))
        {
            ptr = GET(tmp) ;
            /* Ѱ�Ҵ�С���ʵ�free�� */
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

    /* û���ҵ� */
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

    /* ���sizeС��ԭ����Ĵ�С������ԭ���� */
    if ((left = GET_SIZE(HDRP(ptr)) - size) >= 0)
    {
        return ptr;
    }
    /* �����һ������free�������� */
    else if (!GET_ALLOC(HDRP(NEXT_BLKP(ptr))) || !GET_SIZE(HDRP(NEXT_BLKP(ptr))))
    {
        /* ������Ϻ����free��Ҳ��������չ */
        if ((left = GET_SIZE(HDRP(ptr)) + GET_SIZE(HDRP(NEXT_BLKP(ptr))) - size) < 0)
        {
            if (extend_heap(MAX(-left, CHUNKSIZE)) == NULL)
                return NULL;
            left += MAX(-left, CHUNKSIZE);
        }

        /* ɾ��ʹ�õ�free�飬�����¿��ͷβ */
        delete_node(NEXT_BLKP(ptr));
        PUT(HDRP(ptr), PACK(size + left, 1));
        PUT(FTRP(ptr), PACK(size + left, 1));
    }
    /* û�п������õ�����free�� */
    else
    {
        new_block = mm_malloc(size);
        memcpy(new_block, ptr, GET_SIZE(HDRP(ptr)));
        mm_free(ptr);
    }

    return new_block;
}
