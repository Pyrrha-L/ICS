/*
 * Filesystem Lab disigned and implemented by Liang Junkai,RUC
 * StudentId: 2017202121
 * Name: Li Zitong
 * Linked-based approach
 * Most functions: return 0 means normal state,
 * non-zero for exceptions
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <fuse.h>
#include <errno.h>
#include "disk.h"

#define DIRMODE S_IFDIR|0755
#define REGMODE S_IFREG|0644

#define SUPER_BLOCK 1
#define BITMAP_BLOCK 2
#define ROOT_BLOCK 1
 
#define MAX_DATA_IN_BLOCK 4084
#define MAX_FILENAME 24 
#define MAX_EXTENSION 3
#define INODE_SIZE 90

long used_node_count = 0;

struct super_block {
	long fs_size; //һ���м����� 
	long fs_inode_num; //�����Կ��������ڵ�
	long first_blk; //��һ���飨��Ŀ¼�飩�ı�� 
	long bitmap; //bitmap��� 
};

struct inode_file_directory { // Լ 90 bytes
	int uid;
	int gid;
	int link_count; //�м�������ָ�����inode 
	char fname[MAX_FILENAME + 1]; //�ļ����� 
	char fext[MAX_EXTENSION + 1]; //��չ�� 
	long fsize;                 
	long StartBlock; //��ʼ���
	time_t atime;
	time_t mtime;
	time_t ctime;
	int flag;  //0:δʹ��; 1:�����ļ�; 2:Ŀ¼�ļ� 
};

struct disk_block {
	int size; // ��ǰ�����ÿռ� 
	long NextBlock; //��һ����� 
	char data[MAX_DATA_IN_BLOCK];
};

int mkfs()
{
	if(disk_init()){
		printf("disk_init failure.\n");
		exit(-1);
	}
	
	/* 
		��ʼ�� superblock
		�ƻ���2�������洢bitmap
	*/
	
	struct super_block *super_block_record = malloc(sizeof(struct super_block));
	super_block_record->fs_size=BLOCK_NUM;
	super_block_record->fs_inode_num=(MAX_DATA_IN_BLOCK/INODE_SIZE)*BLOCK_NUM;
	super_block_record->first_blk=3;
	super_block_record->bitmap=2;
	disk_write(0, super_block_record);
	free(super_block_record);
	
	printf("super_block initialize success.\n");
	
	/*initial Bitmap_block*/
	int a[1024];
	memset(a, 0, 1024*sizeof(int));
	disk_write(2, a);  /* �ڶ���bitmap�� */
	 
	a[0] = 15; /* ������λΪ1111������ǰ�ĸ��飨SuperBlk,Bitmap,Root����ռ�� */ 
	disk_write(1, a);  /* ��һ��bitmap�� */
	
	printf("bitmap_block initialize success.\n");

	//��ʼ����Ŀ¼��  
	struct disk_block *root = malloc(sizeof(struct disk_block));
	root->size = 0;
	root->NextBlock = -1;
	root->data[0] = '\0';
	disk_write(3,root);
	free(root);
	
	printf("root block initialize success.\n");
	printf("initialize successfully.\n");
	
	return 0;
}

void read_file_dir(struct inode_file_directory *dest_, struct inode_file_directory *src_) {
	strcpy(dest_->fname, src_->fname);
	strcpy(dest_->fext, src_->fext);
	dest_->fsize = src_->fsize;
	dest_->StartBlock = src_->StartBlock;
	dest_->flag = src_->flag;
	return;
}

/*
 *��bitmap��Ϊ���Ϊstart_blk�Ŀ�����flag��־λ
 *
 */
int set_blk(long start_blk, int flag, int * bitmap, int imi) {

	if (start_blk == -1)
		return -1;
		
	int start = start_blk / 8;
	int left = start_blk % 8;
	int f;
	
	// ��left+1�Ķ� 
	int mask = 1;
	mask <<= left;
	
	int tmp;
	tmp = bitmap[start];
	f = tmp;
	
	if (flag){
		f |= mask;
	}
	else{
		f &= ~mask;
	}

	tmp = f;
	
	bitmap[start] = tmp;
	
	return 0;
}
 
int copy_bitmap(int * src, int * dest){
	int i=0;
	for( i ; i<2048 ; i++){
		dest[i] = src [i];
	}
	return 0;
}

int read_all_bitmap(int * bitmap){
	if(disk_read(1,bitmap) || disk_read(2,bitmap+1024)){
		printf("read 2 blocks bitmap error.\n");
	}
	return 0;
}

int write_all_bitmap(int * bitmap){
	if(disk_write(1,bitmap) || disk_write(2,bitmap+1024)){
		printf("write 2 blocks bitmap error.\n");
	}
	return 0;
}

void set_file_dir(struct inode_file_directory *src_, struct inode_file_directory *dest_) {
	strcpy(dest_->fname, src_->fname);
	strcpy(dest_->fext, src_->fext);
	dest_->fsize = src_->fsize;
	dest_->StartBlock = src_->StartBlock;
	dest_->flag = src_->flag;
	return;
}

/*
 * ��ʼ���µ�inode 
 */ 
void init_inode(struct inode_file_directory *file_dir, char*name, char*ext, int flag) 
{	
	strcpy(file_dir->fname, name);
	if (flag == 1 && *ext != '\0')
		strcpy(file_dir->fext, ext);
	file_dir->fsize = 0;
	file_dir->flag = flag;
	file_dir->gid = getgid();
	file_dir->uid = getuid(); 
	file_dir->link_count = 1;
	return;
}

/*
 * ��ʼ���µĿ� 
 */
void init_new_blk(struct disk_block *blk_info) {
	blk_info->size = 0;
	blk_info->NextBlock = -1;
	strcpy(blk_info->data, "\0");
	return;
}

void info_inode(struct inode_file_directory *attr) {
	printf("-------inode-------\n");
	printf("flag: %d\n",attr->flag);
	printf("name: %s\n",attr->fname);
	printf("extent: %s\n",attr->fext);
	printf("size: %ld\n",attr->fsize);
	printf("StartBlock: %ld\n",attr->StartBlock);
	return;
}

/*
 * ��next_blk�����blk_info������
 */
void nextClear(long next_blk, struct disk_block* blk_info) {
	
	int *tmp_bitmap=malloc(sizeof(int)*2048);
	read_all_bitmap(tmp_bitmap);
	
	while (next_blk != -1) {
		set_blk(next_blk, 0, tmp_bitmap, 1);
		disk_read(next_blk, blk_info);
		next_blk = blk_info->NextBlock;
	}
	
	write_all_bitmap(tmp_bitmap);
	return;
}

/*
 *��·����ȡinode��attr
 *�ɹ�����0�����򷵻�-1 
 */
int get_attr_from_path(const char * path,	struct inode_file_directory *attr) {
	struct disk_block *blk_info;
	blk_info = malloc(sizeof(struct disk_block));
	
	if (disk_read(0, blk_info)) {
		printf("read super block error.\n");
		free(blk_info);
		return -1;
	}
	//��superblock�л�ȡ��һ�������Ϣ 
	struct super_block* sb_record;
	sb_record = (struct super_block*) blk_info;
	long start_blk;
	start_blk = sb_record->first_blk;
	
	//��ȡ��һ����Ч��
	if (disk_read(start_blk, blk_info)) {
		printf("can not find root.\n");
		free(blk_info);
		return -1;
	}
	 
	struct inode_file_directory *file_dir=(struct inode_file_directory*) blk_info->data; 
	 
	int offset = 0;
	char *p, *q, *tmp_path;
	tmp_path = strdup(path); //����·�� 
	p = tmp_path;
	
	//����Ǹ�Ŀ¼ 
	if (strcmp(path, "/") == 0) {
		attr->flag = 2;
		attr->StartBlock = start_blk;
		free(blk_info);
		return 0;
	}
	
	//���·��Ϊ��
	if (!p) {
		free(blk_info);
		return -1;
	}
	
	p++;	
	while( p!=NULL && tmp_path!=NULL ){
		
		q = strchr(p, '/');
		
		if (q != NULL) {
			if(tmp_path != p){
				tmp_path = p;
			}
			*q = '\0'; //�Ѹ�λ�û�����ֹ 
			q++;
			p = q; //p����ǰ���ļ��� 
		}
		else if( q == NULL ){ //�Ѿ��������ļ��� 
			break;
		}
		//tmp_path Ҫ���ҵ���һ��Ŀ¼��p Ŀ�� 
		//�ϼ�Ŀ¼���Ǹ�Ŀ¼��Ŀ¼ 
		if (*tmp_path != '/') {
			//����ͬһ������ 
			while (offset < blk_info->size) {
			//Ѱ��ƥ�� 
				if (strcmp(file_dir->fname, tmp_path) == 0 && file_dir->flag == 2) {
					start_blk = file_dir->StartBlock;
					break;
				}
				//������һ���ṹ�� 
				file_dir++;
				offset += sizeof(struct inode_file_directory);
			}
			//����Ŀ¼��û���ҵ�
			if (offset == blk_info->size) {
				free(blk_info);
				printf("can not find directory.\n");
				return -1;
			}
			//���뵽������Ŀ¼�ļ��� 
			if (disk_read(start_blk, blk_info)) {
				free(blk_info);
				return -1;
			}
			file_dir = (struct inode_file_directory*) blk_info->data;
			
			tmp_path = p;
			offset = 0;
		}
	}
	
	q = strchr(p, '.');
	if(q != NULL){
		*q = '\0';
		q++;
	}
	
	offset = 0; 
	while (offset < blk_info->size) {
			//���ļ���ȡ��attr�� 
		if (file_dir->flag != 0 && strcmp(file_dir->fname, p) == 0
			&& (q == NULL || strcmp(file_dir->fext, q) == 0)) {
			start_blk = file_dir->StartBlock;
			read_file_dir(attr, file_dir);
			free(blk_info);
			return 0;
		}
		file_dir++;
		offset += sizeof(struct inode_file_directory);
	}
	free(blk_info);
	
	printf("get_attr_from_path:%s\n",path);
	
	return -1;
}

/*
 * �ж�·��ָ���ļ��Ƿ�ǿ�
 * ������-1����Ϊ������״̬��������Ч·������̶�ȡ������ѯ���ǳ����ļ��� 
 * ������0����Ŀ¼�����ҷǿ�
 * ������1����Ŀ¼Ϊ�� 
 */ 
int is_empty(const char* path) {
	struct disk_block *blk_info = malloc(sizeof(struct disk_block));
	struct inode_file_directory *attr = malloc(sizeof(struct inode_file_directory));

	if (get_attr_from_path(path, attr) == -1) {
		printf("get_attr_from_path(%s, attr) failure.\n",path);
		free(blk_info);
		free(attr);
		return 0;
	}
	
	long start_blk;
	start_blk = attr->StartBlock;
	
	// ���inodeָ���start_blkΪ-1����û�����ݿ� 
	if(start_blk==-1){
		free(blk_info);
		free(attr);
		return 1;
	}
	
	if (attr->flag == 1) {
		free(blk_info);
		free(attr);
		return 0;
	}
	
	if (disk_read(start_blk, blk_info)) {
		free(blk_info);
		free(attr);
		return 0;
	}

	struct inode_file_directory *file_dir =
			(struct inode_file_directory*) blk_info->data;
	int pos = 0;
	while (pos < blk_info->size) {
		if (file_dir->flag != 0) {
			free(blk_info);
			free(attr);
			return 0;
		}
		file_dir++;
		pos += sizeof(struct inode_file_directory);
	}

	free(blk_info);
	free(attr);
	return 1;
}

/*
 *�������Ŀ��п飬maxΪ������num���������п��������start_blkΪ��Ӧ�Ŀ��п����ڿ�ʼ��ı�ţ��޸�bitmap 
 *����max���������п������ 
 */
int find_free_blocks(int num, long* start_blk, int *bitmap, int imi) {
	*start_blk = 1 + BITMAP_BLOCK + 1;
	int tmp = 0;
	int k=0;

	int start, left;
	unsigned int mask, f;
	int *flag;

	int max = 0;
	long max_start = -1;
	
	//�����ҿ� 
	while (*start_blk < BLOCK_NUM - 1) {
		start = *start_blk / 8;
		
		left = *start_blk % 8;
		k = start;
		
		mask = 1;
		mask <<= left ;
		flag = malloc(sizeof(int));
		*flag = bitmap[k];
		f = *flag;
		for (tmp = 0; tmp < num; tmp++) {
			if ((f & mask) == mask)
				break;
			if ((mask & 0x80000000) == 0x80000000) {
				k += 1;
				*flag = bitmap[k];
				f = *flag;
				mask = 1;
			} else
				mask <<= 1;
		}
		
		if (tmp > max) {
			max_start = *start_blk;
			max = tmp;
		}
		
		if (tmp == num)
			break;
		
		*start_blk = (tmp + 1) + *start_blk;
		tmp = 0;
	}
	*start_blk = max_start;

	//���ҵ��Ŀտ�λ����bitmap����Ϊ1 
	int j = max_start;
	int i;
	for (i = 0; i < max; ++i) {
		if (set_blk(j++, 1, bitmap, imi) == -1) {
			printf("set_blk error.\n");
			return -1;
		}
	}
	
	return max;

}

/*
 * ��attrָ���inodeд��pathָ��ĸ�Ŀ¼��
 * �ɹ�����0 
 */
int set_attr(const char* path, struct inode_file_directory* attr) {
	int res;
	struct disk_block* blk_info = malloc(sizeof(struct disk_block));

	char *p = malloc(24 * sizeof(char)), *q = malloc(24 * sizeof(char));
	long start_blk;

	if ((res = anal_path(p, q, path, &start_blk, 1))) {
		free(p);
		free(q);
		return res;
	}

	if (disk_read(start_blk, blk_info) == -1) {
		res = -1;
		free(blk_info);
		return res;
	}
	struct inode_file_directory *file_dir =
			(struct inode_file_directory*) blk_info->data;
	int offset = 0;
	while (offset < blk_info->size) {
		if (file_dir->flag != 0 && strcmp(p, file_dir->fname) == 0
				&& (*q == '\0' || strcmp(q, file_dir->fext) == 0)) {
			set_file_dir(attr, file_dir);
			res = 0;
			break;
		}
		file_dir++;
		offset += sizeof(struct inode_file_directory);
	}
	free(p);
	free(q);
	
	if (disk_write(start_blk, blk_info) == -1) {
		res = -1;
	}
	
	free(blk_info);
	return res;
}

/*
 * ����·����nameΪ�ļ����ƣ�extΪ�ļ���չ����pathΪ·�� 
 * p_dir_blkΪ��Ŀ¼��Ӧ�Ŀ�ţ�flagΪ�ļ���𡣡� 
 *
 */
int anal_path(char*name, char*ext, const char*path, long*p_dir_blk, int flag) {
	char*tmp_path, *p, *q;
	tmp_path = strdup(path);
	struct inode_file_directory *file_dir = malloc(sizeof(struct inode_file_directory));
	struct inode_file_directory *file_dir_2 = malloc(sizeof(struct inode_file_directory));
	p = tmp_path;
	
	if ((q = strchr(p, '/')) == NULL) {
		if (get_attr_from_path("/", file_dir) == -1) {
			free(file_dir);
			return -ENOENT;
		}
	}

	struct disk_block *blk_info;
	blk_info = malloc(sizeof(struct disk_block));
	
	if (disk_read(0, blk_info)) {
		free(blk_info);
		return -1;
	}
	//��superblock�л�ȡ��һ�������Ϣ 
	struct super_block* sb_record;
	sb_record = (struct super_block*) blk_info;
	long start_blk;
	start_blk = sb_record->first_blk;
	
	//��ȡ��һ����Ч��
	if (disk_read(start_blk, blk_info)) {
		printf("can not find root.\n");
		free(blk_info);
		return -1;
	}
	 
	file_dir=(struct inode_file_directory*) blk_info->data; 
	file_dir_2 = file_dir;
	
	int offset = 0;
	
	p++;
	
	while( p!=NULL && tmp_path!=NULL ){
		
		q = strchr(p, '/');
		
		if (q != NULL) {
			if(tmp_path != p){
				tmp_path = p;
			}
			*q = '\0'; //�Ѹ�λ�û�����ֹ 
			q++;
			p = q; //p����ǰ���ļ��� 
		}
		else if( q == NULL ){ //�Ѿ��������ļ��� 
			break;
		}
		//tmp_path Ҫ���ҵ���һ��Ŀ¼��p Ŀ�� 
		//�ϼ�Ŀ¼���Ǹ�Ŀ¼��Ŀ¼ 
		if (*tmp_path != '/') {
			//����ͬһ������ 
			while (offset < blk_info->size) {
			//Ѱ��ƥ�� 
				if (strcmp(file_dir->fname, tmp_path) == 0 && file_dir->flag == 2) {
					
					file_dir_2 = file_dir; //���游Ŀ¼��Ϣ 	
					start_blk = file_dir->StartBlock;
					break;
				}
				//������һ���ṹ�� 
				file_dir++;
				offset += sizeof(struct inode_file_directory);
			}
			//����Ŀ¼��û���ҵ�
			if (offset == blk_info->size) {
				free(blk_info);
				printf("can not find directory.\n");
				return -1;
			}
			//���뵽������Ŀ¼�ļ��� 
			if (disk_read(start_blk, blk_info)) {
				printf("error when reading start_blk.\n");
				free(blk_info);
				return -1;
			}
			file_dir = (struct inode_file_directory*) blk_info->data;
			
			tmp_path = p;
			offset = 0;
		}
	}
	if (flag == 1) {
		q = strchr(p, '.');
		if (q != NULL) {
			*q = '\0';
			q++;
		}
	}

	*name = *ext = '\0';
	if (p != NULL)
		strcpy(name, p);
	if (q != NULL)
		strcpy(ext, q);
	
	//���¸���Ŀ¼��ʱ��
	time_t timenow=time(NULL); 
	file_dir_2->ctime = timenow;
	file_dir_2->mtime = timenow;
	tmp_path = strdup(path);
	
	if(strcmp(tmp_path,"/")){
		p=strchr(tmp_path,'/');
		if(p!=NULL){
			*p='\0';
			set_attr(tmp_path,file_dir_2);
		}
	}
	
	//���ļ����ϲ�Ŀ¼ 
	*p_dir_blk = start_blk;
	
	
	if (*p_dir_blk == -1) {
		return -ENOENT;
	}
	
	return 0;
}

/*
 * ʹposͬ����offset����һ��������ĩβ���п�д��λ�� 
 * �ɹ�����0. 
 *
 */
int move_to_offset(struct inode_file_directory *file_dir, char *p, char *q,int* offset, int* pos, int size, int flag) {
	while (*offset < size) {
		if (flag == 0)
			*pos = *offset;
		file_dir++;
		*offset += sizeof(struct inode_file_directory);
	}
	return 0;
}

/*
 * Ϊ p_dir_blk ��ָ��ĸ�Ŀ¼�������ݿ���ӽ�һ���¿����Դ洢file_dir�� 
 * p\q\flagΪ�µ�file_dir����Ϣ
 * �ɹ�����0 
 */
int enlarge_blk(long p_dir_blk, struct inode_file_directory *file_dir,
	struct disk_block *blk_info, long *tmp, char*p, char*q, int flag) {
	
	int * tmp_bitmap = malloc(sizeof(int)*2048);
	read_all_bitmap(tmp_bitmap);
	
	long blk;
	tmp = malloc(sizeof(long)); //�ҵ����п����ʼ���� 
	
	if (find_free_blocks(1, tmp, tmp_bitmap,0) == 1){
		if((find_free_blocks(1, tmp, tmp_bitmap,0) == 0)){
			free(p);
			free(q);
			free(blk_info);
			return -ENOSPC;
		}
	}
	else {
		free(p);
		free(q);
		free(blk_info);
		return -ENOSPC; //�Ҳ������п飬û�п��ÿռ� 
	}
	
	read_all_bitmap(tmp_bitmap);
	
	if(find_free_blocks(1, tmp, tmp_bitmap,1)==1){
		blk = *tmp;
	}
	
	free(tmp);
	blk_info->NextBlock = blk;
	disk_write(p_dir_blk, blk_info); //���¸�Ŀ¼��С 

	blk_info->size = sizeof(struct inode_file_directory);
	blk_info->NextBlock = -1;
	file_dir = (struct inode_file_directory*) blk_info->data; //���¸�Ŀ¼�����ݿ� 
	init_inode(file_dir, p, q, flag);

	tmp = malloc(sizeof(long));
	if (find_free_blocks(1, tmp, tmp_bitmap,1) == 1){
		file_dir->StartBlock = *tmp; //Ϊ��д����ļ��������ݿ�
		//ȷ�����ݿ�͸�Ŀ¼�鶼�пռ����д�룬�ٽ��޸ĺ��bitmapд�ش��̡�
		write_all_bitmap(tmp_bitmap);
	}
	else {
		return -ENOSPC;
	}
	free(tmp);
	free(tmp_bitmap);
	disk_write(blk, blk_info); //������д����ļ��������ݿ� 

	init_new_blk(blk_info);
	disk_write(file_dir->StartBlock, blk_info);
	return 0;
}

/*
 * ����path����flag��Ӧ���ļ�
 * 1���ļ� 2��Ŀ¼ 
 *
 * �ɹ�����0 
 */
int create_inode(const char* path, int flag) {
	int res;
	
	int * tmp_bitmap = malloc(sizeof(int)*2048);
	read_all_bitmap(tmp_bitmap);
	
	long p_dir_blk;
	char *p = malloc(24 * sizeof(char)), *q = malloc(24 * sizeof(char));

	//����·���� p_dir_blkΪ��Ŀ¼��Ӧ�����ݿ��ţ�p��qΪ�ļ����ͺ�׺ 
	if ((res = anal_path(p, q, path, &p_dir_blk, flag))) {
		free(p);
		free(q);
		return res;
	}
	struct disk_block *blk_info = malloc(sizeof(struct disk_block));
	
	if (disk_read(p_dir_blk, blk_info)) {
		free(blk_info);
		free(p);
		free(q);
		return -ENOENT;
	}
	struct inode_file_directory *file_dir =(struct inode_file_directory*) blk_info->data;

	int offset = 0;
	int pos = blk_info->size;

	/*  �����������ļ����ʿɲ��ã�����Ҫ�޸�ƫ����(offset)ʹ֮����ĩβ��file_dirҲָ��blk_info��ĩβ */
	if ((res = move_to_offset(file_dir, p, q, &offset, &pos, blk_info->size, flag))) {
		free(blk_info);
		free(p);
		free(q);		
		return res;
	}
	
	file_dir += offset / sizeof(struct inode_file_directory);

	long *tmp = malloc(sizeof(long));
	
	if (pos == blk_info->size) {
		if (blk_info->size > MAX_DATA_IN_BLOCK) {
			if ((res = enlarge_blk(p_dir_blk, file_dir, blk_info, tmp, p, q,
					flag))) {
				printf("enlarge_blk %ld failure.\n",p_dir_blk);
				free(blk_info);
				free(p);
				free(q);
				return -ENOSPC;
			}
			free(blk_info);
			free(p);
			free(q);
			return 0;
		} else {
			blk_info->size += sizeof(struct inode_file_directory);
		}
	} else {
		offset = 0;
		file_dir = (struct inode_file_directory*) blk_info->data;

		while (offset < pos)
			file_dir++;
	}
	init_inode(file_dir, p, q, flag);

	if ((res = find_free_blocks(1, tmp, tmp_bitmap, 1)) == 1) {
		file_dir->StartBlock = *tmp;
		write_all_bitmap(tmp_bitmap); //д��bitmap 
	} else {
		free(blk_info);
		free(p);
		free(q);
		return -1;
	}
	free(tmp);
	
	//�����������ļ�
	time_t timenow = time(NULL); 
	file_dir->atime = timenow;
	file_dir->ctime = timenow;
	file_dir->mtime = timenow;
	
	disk_write(p_dir_blk, blk_info); //���¸�Ŀ¼�� 
	init_new_blk(blk_info);
	
	//printf("blk_info:%s %d %ld\n",blk_info->data,blk_info->size, blk_info->NextBlock);

	disk_write(file_dir->StartBlock, blk_info); //�����µ����ݿ� 
	
	free(blk_info);
	free(p);
	free(q);
	
	used_node_count++;
	
	return 0;
}

int rm_file_dir(const char *path, int flag) {
	struct inode_file_directory *attr = malloc(
			sizeof(struct inode_file_directory));
	
	if (get_attr_from_path(path, attr) == -1) {
		free(attr);
		return -ENOENT;
	}
	
	if (flag == 1 && attr->flag == 2) {
		free(attr);
		return -EISDIR;
	} else if (flag == 2 && attr->flag == 1) {
		free(attr);
		return -ENOTDIR;
	}
	
	//printf("debug: rm_file_dir : %s \n", attr->fname);
	struct disk_block* blk_info = malloc(sizeof(struct disk_block));
	if (flag == 1) {
		long next_blk = attr->StartBlock;
		nextClear(next_blk, blk_info);
	} else if (!is_empty(path)) {
		free(blk_info);
		free(attr);
		return -ENOTEMPTY;
	}

	attr->flag = 0;
	if (set_attr(path, attr) == -1) {
		free(blk_info);
		free(attr);
		return -1;
	}
	
	used_node_count--;
	free(blk_info);
	free(attr);
	return 0;
}

//Filesystem operations that you need to implement
int fs_getattr (const char *path, struct stat *attr)
{
	//printf("into fs_getattr\n");
	struct inode_file_directory *attr2 = malloc(sizeof(struct inode_file_directory));

	if (get_attr_from_path(path, attr2) == -1) {
		//printf("error at getattr\n");
		free(attr2);
		return -ENOENT;
	}
	
	memset(attr, 0, sizeof(struct stat));
	
	attr->st_nlink = 1;
	attr->st_gid = getgid();
	attr->st_uid = getuid(); 
	
	if (attr2->flag == 2) {   //Ŀ¼
		attr->st_mode = DIRMODE;
		attr->st_size = sizeof (struct inode_file_directory);
	} else if (attr2->flag == 1) {   //�����ļ� 
		attr->st_mode = REGMODE;
		attr->st_size = attr2->fsize;
	}
	
	free(attr2);
	
	printf("Getattr is called:%s\n",path);
	return 0;
}

/*
 * ������ʼ���ƫ����Ѱ��λ��
 * �ɹ�����0�����򷵻�-1 
 *
 * offset�� ƫ����
 * start_blk: ��ʼ��
 */
int find_offset_blk(long*start_blk, long*offset, struct disk_block *blk_info) {
	while (1) {
		if (disk_read(*start_blk, blk_info)) {
			return -1;
		}
		if (*offset <= blk_info->size)
			break;
		*offset -= blk_info->size;
		*start_blk = blk_info->NextBlock;
	}
	return 0;
}

int fs_readdir(const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
{
	struct disk_block *blk_info;
	struct inode_file_directory *attr;
	blk_info = malloc(sizeof(struct disk_block));
	attr = malloc(sizeof(struct inode_file_directory));

	if (get_attr_from_path(path, attr) == -1) {
		free(attr);
		free(blk_info);
		return -ENOENT;
	}

	long start_blk = attr->StartBlock;
	if (attr->flag == 1) {
		free(attr);
		free(blk_info);
		return -ENOTDIR;
	}
	
	if (disk_read(start_blk, blk_info)) {   //��ȡ���ݿ�
		free(attr);
		free(blk_info);
		return -ENOENT;
	}

	struct inode_file_directory *file_dir =
			(struct inode_file_directory*) blk_info->data;
	int pos = 0;
	char name[MAX_FILENAME + MAX_EXTENSION + 2];
	
	//��ȡ�ļ� 
	while (pos < blk_info->size) {
		strcpy(name, file_dir->fname);
		//�����ļ���չ�� 
		if (strlen(file_dir->fext) != 0) {
			strcat(name, ".");
			strcat(name, file_dir->fext);
		}
		if (file_dir->flag != 0 
				&& filler(buffer, name, NULL, 0))  //add filename into buffer
			break;
		file_dir++;
		pos += sizeof(struct inode_file_directory);
	}
	
	// �޸�Ŀ¼�ļ���atime
	attr->atime = time(NULL);
	set_attr(path,attr);
	
	free(attr);
	free(blk_info);

	printf("Readdir is called:%s\n", path);
	return 0;
}

int fs_read(const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi)
{
	struct inode_file_directory *attr = malloc(sizeof(struct inode_file_directory));
			
	//����·��Ѱַ
	if (get_attr_from_path(path, attr)) {
		printf("get_attr_from_path(%s, attr) failure.\n",path); 
		free(attr);
		return -ENOENT;
	}
	
	//Ŀ¼���ɶ�ȡ 
	if (attr->flag == 2) {
		free(attr);
		return -EISDIR;
	}

	struct disk_block *blk_info; // �ļ������������ݿ� 
		blk_info = malloc(sizeof(struct disk_block));

	if (disk_read(attr->StartBlock, blk_info)) {
		printf("read content from disk %ld failure.\n",attr->StartBlock);
		free(attr);
		free(blk_info);
		return -1;
	}
	
	if (offset < attr->fsize) {
		if (offset + size > attr->fsize)  //����������ֻ��ȡ�����ڵĲ��� 
			size = attr->fsize - offset; 
	} else
		size = 0; //���ƫ���������ļ���С���򲻶�ȡ 

	int real_offset, blk_num, i, ret = 0;
	long start_blk = blk_info->NextBlock;

	blk_num = offset / MAX_DATA_IN_BLOCK;
	real_offset = offset % MAX_DATA_IN_BLOCK;
	
	// ����offset������block��,�ҵ�offset��ָ�Ŀ�ʼλ�õĿ�
	// �����ζ����̣��������ܽϲ 
	for (i = 0; i < blk_num; i++) {
		if (disk_read(blk_info->NextBlock, blk_info)|| start_blk == -1) {
			printf("read_block_from_pos failed!\n");
			free(attr);
			free(blk_info);
			return -1;
		}
	}

	int temp = size;
	char *pt = blk_info->data;
	pt += real_offset;
	
	//�����ȡ��size��ʣ�µĴ���ֻ��ȡʣ�µĲ���
	ret = (MAX_DATA_IN_BLOCK - real_offset < size ?
		MAX_DATA_IN_BLOCK - real_offset : size);
	
	memcpy(buffer, pt, ret);
	temp -= ret;

	//���Ҫ����size��û�ж��� 
	while (temp > 0) {
		if (disk_read(blk_info->NextBlock, blk_info)) {
			printf("read_block_from_pos failed!\n");
			break;
		}
		if (temp > MAX_DATA_IN_BLOCK) {
			memcpy(buffer + size - temp, blk_info->data, MAX_DATA_IN_BLOCK);
			temp -= MAX_DATA_IN_BLOCK;
		} else {
			memcpy(buffer + size - temp, blk_info->data, temp);
			break;
		}
	}
	
	attr->atime = time(NULL);
	set_attr(path,attr);
	 
	free(attr);
	free(blk_info);

	printf("Read is called:%s\n",path);
	return ret;
}

int fs_mknod (const char *path, mode_t mode, dev_t dev)
{
	int res = create_inode(path, 1);
	
	printf("Mknod is called:%s\n",path);
	return res;
}

int fs_mkdir (const char *path, mode_t mode)
{
	int res = create_inode(path, 2);
	
	printf("Mkdir is called:%s\n",path);
	return res;
}

int fs_rmdir (const char *path)
{
	int res = rm_file_dir(path, 2);
	printf("Rmdir is called:%s  and res is %d \n",path, res);
	return res;
}

int fs_unlink (const char *path)
{
	int res = rm_file_dir(path, 1);
	printf("Unlink is callded:%s\n",path);
	return res;
}

int fs_rename (const char *oldpath, const char *newname)
{	
	if  (!(strcmp(oldpath, newname))) {
		printf("oldpath  and  newpath are the same.\n");
		return 0;
	}
	
	int rt;
	long new_dir_blk;

	char *p_dest = malloc(24 * sizeof(char)); //Ŀ������ 
	char *rd = malloc(4 * sizeof(char)); //Ŀ�����չ 
	
	struct inode_file_directory * src_inode = malloc(sizeof(struct inode_file_directory));
	struct inode_file_directory * dest_inode = malloc(sizeof(struct inode_file_directory));
	struct inode_file_directory * d_dest_inode = malloc(sizeof(struct inode_file_directory)); //Ŀ����ϼ�Ŀ¼ 
	
	if((rt = get_attr_from_path(oldpath,src_inode))){
		printf("get oldpath %s nodeinfo failed.\n", oldpath);
		return -1;
	}
	
	if((rt = get_attr_from_path(newname,dest_inode))){
		printf("get newpath nodeinfo failed. and create newpath.\n");
		if((rt=create_inode(newname, src_inode->flag))==0){
			get_attr_from_path(newname,dest_inode);
		}
	}
	
	rt = anal_path(p_dest, rd, newname, &new_dir_blk, dest_inode->flag);
	if (rt != 0) {
		printf("anal_path(%s); failed", newname);
		return rt;
	}

	//info_inode(src_inode);
	//info_inode(dest_inode);
	if (rt == 0) {
		if (dest_inode->flag==2) {
			/* EISDIR:
			 *   newpath  is  an  existing directory, but oldpath is not a directory.
			 */
			if (src_inode->flag==1) {
				printf("newpath is dir && oldpath is not a dir -> EISDIR");
				rt = -EISDIR;
				free(p_dest);
				free(rd);
				free(src_inode);
				free(dest_inode);
				free(d_dest_inode);
				return rt;
			}
			/* ENOTEMPTY:
			 *   newpath is a non-empty  directory
			 */
			rt = is_empty(newname);  //rt : 0 �ǿ� 1 �� 
			if (rt == 0) {
				printf("new path is not empty.\n ");
				free(p_dest);
				free(rd);
				free(src_inode);
				free(dest_inode);
				free(d_dest_inode);
				return -ENOTEMPTY;
			}
		}
		/* ENOTDIR:
		 *   oldpath  is a directory, and newpath exists but is not a directory
		 */
		if (src_inode->flag==2&&dest_inode->flag!=2) {
			printf("oldpath is dir && newpath is not a dir -> ENOTDIR \n");
			rt = -ENOTDIR;
			free(p_dest);
			free(rd);
			free(src_inode);
			free(dest_inode);
			free(d_dest_inode);
			return rt;
		}

	}
		
	dest_inode->fsize = src_inode->fsize;
	dest_inode->StartBlock = src_inode->StartBlock;
	src_inode->StartBlock = -1;
	dest_inode->flag = src_inode->flag;

	/* utimes and inodes update */
	time_t newtime = time(NULL);
	dest_inode->mtime = newtime;
	dest_inode->ctime = newtime;
	src_inode->ctime = newtime;
	
	rt = set_attr(newname, dest_inode);
	if (rt != 0) {
		printf("set_attr; new ; failed\n");
		return -1;
	}
	rt = set_attr(oldpath, src_inode);
	if (rt != 0) {
		printf("set_attr; old ; failed\n");
		return -1;
	}
	
	/* Step 3: delete the source */
	if (src_inode->flag==2) {
		rt = fs_rmdir(oldpath);
	} else {
		rt = fs_unlink(oldpath);
	}
	
	printf("Rename is called:%s\n",oldpath);
	return rt;
		
}

int fs_write (const char *path, const char *buffer, size_t size, off_t offset, struct fuse_file_info *fi)
{
	//��·������inode 
	struct inode_file_directory *attr = malloc(sizeof(struct inode_file_directory));
	get_attr_from_path(path, attr);
	
	size_t size_all=size;
	
	int * tmp_bitmap=malloc(sizeof(int)*2048); 
	read_all_bitmap(tmp_bitmap);
	
	//ƫ���������ļ��ߴ� 
	if (offset > attr->fsize) {
		free(attr);
		return -EFBIG;
	}

	long start_blk = attr->StartBlock;
	if (start_blk == -1) {
		printf("no data block assigned to the inode.\n");
		free(attr);
		return -1;
	}

	int res;
	struct disk_block *blk_info = malloc(sizeof(struct disk_block));
	int para_offset = offset;

	int num;
	
	//blk_infoΪ����ƫ����֮��Ŀ� 
	if ((res = (find_offset_blk(&start_blk, &offset, blk_info)))) {
		return res;
	}

	char* pt = blk_info->data;
	
	//ptָ��Ӧ��д���λ�� 
	pt += offset;

	int towrite = 0;
	int writen = 0;

	towrite = (MAX_DATA_IN_BLOCK - offset < size ?
		MAX_DATA_IN_BLOCK - offset :size);
	size -= towrite;

	long* next_blk = malloc(sizeof(long));

	if (size > 0) {
		//Ѱ���µĿ��п飬�ҵ��Ŀ��п������� 
		num = find_free_blocks(size / MAX_DATA_IN_BLOCK + 1, next_blk,tmp_bitmap, 0);
		if (num == 0) { //���û���µĿ��п飬��bitmap��д�ء� 
			printf("there is no free block.\n");
			free(pt);
			free(attr);
			free(blk_info);
			free(next_blk);
			return -ENOSPC;
		}
		
		int i;
		while (1) {
			for (i = 0; i < num; ++i) {
				towrite = (MAX_DATA_IN_BLOCK < size ? MAX_DATA_IN_BLOCK : size);
				size -= towrite;
			}
			if (size == 0)
				break;
			
			//�ҿ��п�
			num = find_free_blocks(size / MAX_DATA_IN_BLOCK + 1, next_blk, tmp_bitmap, 0);
			if (num == 0) {
				free(attr);
				free(blk_info);
				free(next_blk);
				return -ENOSPC;
			}
		}
	}

	//����ִ�е���һ������˵�����㹻�Ŀռ䡣
	writen = 0;
	towrite = 0;
	
	size=size_all; //�ָ�ԭ��ֵ
	towrite = (MAX_DATA_IN_BLOCK - offset < size ?
		MAX_DATA_IN_BLOCK - offset :size);
	strncpy(pt, buffer, towrite);
	buffer += towrite;
	blk_info->size += towrite;
	writen += towrite;
	size -= towrite;
	
	read_all_bitmap(tmp_bitmap);
	
	if (size > 0) {
		//Ѱ���µĿ��п飬�ҵ��Ŀ��п������� 
		num = find_free_blocks(size / MAX_DATA_IN_BLOCK + 1, next_blk,tmp_bitmap,1);
		if (num == 0) { //���û���µĿ��п飬��bitmap��д�ء� 
			printf("there is no free block.\n");
			free(pt);
			free(attr);
			free(blk_info);
			free(next_blk);
			return -ENOSPC;
		}
		
		blk_info->NextBlock = *next_blk;
		disk_write(start_blk, blk_info);
		
		int i;
		while (1) {
			for (i = 0; i < num; ++i) {
				towrite = (MAX_DATA_IN_BLOCK < size ? MAX_DATA_IN_BLOCK : size);
				blk_info->size = towrite;
				strncpy(blk_info->data, buffer, towrite);
				buffer += towrite;
				size -= towrite;
				writen += towrite;

				if (size == 0)
					blk_info->NextBlock = -1;
				else //��ǰ��֤���ҵ��Ŀ��п����������ʿ���ֱ�Ӽ�1 
					blk_info->NextBlock = *next_blk + 1;
					
				disk_write(*next_blk, blk_info);
				*next_blk = *next_blk + 1;
			}
			if (size == 0)
				break;
			
			//�ҵ��Ŀ��п鲻����������
			num = find_free_blocks(size / MAX_DATA_IN_BLOCK + 1, next_blk, tmp_bitmap,1);
			if (num == 0) {
				free(attr);
				free(blk_info);
				free(next_blk);
				return -ENOSPC;
			}
		}
	} else if (size == 0) {
		long next_blk;
		next_blk = blk_info->NextBlock;
		blk_info->NextBlock = -1; //��ǰд���Ϊ��ֹ�� 
		disk_write(start_blk, blk_info);
		nextClear(next_blk, blk_info); //����һ���鿪ʼ��� 
	}
	
	//����attr 
	size = writen;
	attr->ctime = time(NULL);
	attr->mtime = time(NULL);
	attr->fsize = para_offset + size;
	if (set_attr(path, attr) == -1) {
		size = -1;
	}
	
	free(attr);
	free(blk_info);
	free(next_blk);
	printf("Write is called:%s\n",path);
	
	return size;
}

int fs_truncate (const char *path, off_t size)
{
	struct inode_file_directory *attr = malloc(sizeof(struct inode_file_directory));
	get_attr_from_path(path, attr);
	long start_blk = attr->StartBlock;
	struct disk_block *blk_info = malloc(sizeof(struct disk_block));
	
	if (start_blk == -1) {
		printf("no data block assigned to the inode.\n");
		free(attr);
		return -1;
	}
	disk_read(start_blk, blk_info);
		
	//sizeС��ԭ���ļ��ߴ磬��ȥ����Ĳ��� 
	if (size <= attr->fsize) {
		attr->fsize=size;
		long i;
		
		int num = size / MAX_DATA_IN_BLOCK + 1;
		
		for(i=0;i<num-1;i++){
			disk_read(blk_info->NextBlock, blk_info);
			size -= MAX_DATA_IN_BLOCK;
		}
		
		//ͬһ�����ж���Ĵ�С��0 
		memset(blk_info->data+size, '\0', MAX_DATA_IN_BLOCK-size);
		//���ʣ��� 
		nextClear(blk_info->NextBlock, blk_info);
	}
	else if( size / MAX_DATA_IN_BLOCK == attr->fsize / MAX_DATA_IN_BLOCK ){
		attr->fsize=size;
	}
	else if( size / MAX_DATA_IN_BLOCK > attr->fsize / MAX_DATA_IN_BLOCK ){
		//��Ҫ����µĿ�
		int * tmp_bitmap=malloc(sizeof(int)*2048);
		read_all_bitmap(tmp_bitmap);
		
		long size_record=attr->fsize;
		find_offset_blk(&start_blk, &size_record, blk_info);
		
		long* next_blk = malloc(sizeof(long));
		int add_blk = size / MAX_DATA_IN_BLOCK - attr->fsize / MAX_DATA_IN_BLOCK;
		
		while(1){ //ģ����� 
			int num = find_free_blocks( add_blk , next_blk, tmp_bitmap, 0);
			if(num == add_blk){
				break;
			}
			else if( num < add_blk ){
				add_blk -= num;
			}
			else if( add_blk <= 1 && find_free_blocks( add_blk , next_blk, tmp_bitmap, 0)==0 ){
				//�ռ䲻�����޷������µĿ� 
				return -ENOSPC;
			}
		}
		
		//����ִ�е���һ������ռ��㹻
		//�ָ�bitmap
		read_all_bitmap(tmp_bitmap); 
		while(1){
			int num = find_free_blocks( add_blk , next_blk, tmp_bitmap,1);
			if(num == add_blk){
				blk_info->NextBlock = *next_blk;
				break;
			}
			else if( num < add_blk ){
				blk_info->NextBlock = *next_blk;
				disk_read(*next_blk, blk_info);
				add_blk -= num;
			}
			else if( add_blk <= 1 && find_free_blocks( add_blk , next_blk, tmp_bitmap,1)==0 ){
				//�ռ䲻�����޷������µĿ� 
				return -ENOSPC;
			}
		}
		write_all_bitmap(tmp_bitmap); 
	}
	
	attr->fsize = size;
	attr->ctime = time(NULL);
	set_attr(path,attr);
	
	printf("Truncate is called:%s\n",path);
	return 0;
}

int fs_utime (const char *path, struct utimbuf *buffer)
{
	struct inode_file_directory *file_dir =malloc(sizeof(struct inode_file_directory));
	
	get_attr_from_path(path, file_dir);

	//�����������ļ� 
	file_dir->atime=buffer->actime;
	file_dir->ctime=time(NULL);
	file_dir->mtime=buffer->modtime;
	
	set_attr(path, file_dir);
	printf("Utime is called:%s\n",path);
	return 0;
}

int getbitmapused(){
	int * tmp_bitmap = malloc(sizeof(int)*2048);
	read_all_bitmap(tmp_bitmap);
	
	int rt = 0;
	int i;
	for(i=0; i<2048; i++){
		int tmp_val=*(tmp_bitmap+i);
		while(tmp_val){
			rt+=tmp_val&1;
			tmp_val>>=1;
		}
	}
	
	free(tmp_bitmap);
    return rt;
}

int fs_statfs (const char *path, struct statvfs *stat)
{
	memset(stat, 0, sizeof(struct statvfs));
	int used_block_count = getbitmapused();
	
	stat->f_bsize = BLOCK_SIZE;
	stat->f_blocks = BLOCK_NUM; 
	stat->f_bfree = BLOCK_NUM-used_block_count;
	stat->f_bavail = stat->f_bfree;
	stat->f_files = (BLOCK_NUM-3)*(MAX_DATA_IN_BLOCK / sizeof(struct inode_file_directory));
	stat->f_ffree = stat->f_files - used_node_count; 
	stat->f_favail = stat->f_ffree;
	stat->f_namemax = MAX_FILENAME;
	printf("Statfs is called:%s\n",path);
	return 0;
}

int fs_open (const char *path, struct fuse_file_info *fi)
{
	//���������O_APPEND 
    if ((fi->flags & O_APPEND) != 0) {
    	fi->fh = 1;
	}
	else{
		fi->fh = 0;
	}
	printf("Open is called:%s\n",path);
	return 0;
}

//Functions you don't actually need to modify
int fs_release (const char *path, struct fuse_file_info *fi)
{
	printf("Release is called:%s\n",path);
	return 0;
}

int fs_opendir (const char *path, struct fuse_file_info *fi)
{
	printf("Opendir is called:%s\n",path);
	return 0;
}

int fs_releasedir (const char * path, struct fuse_file_info *fi)
{
	printf("Releasedir is called:%s\n",path);
	return 0;
}

static struct fuse_operations fs_operations = {
	.getattr    = fs_getattr,
	.readdir    = fs_readdir,
	.read       = fs_read,
	.mkdir      = fs_mkdir,
	.rmdir      = fs_rmdir,
	.unlink     = fs_unlink,
	.rename     = fs_rename,
	.truncate   = fs_truncate,
	.utime      = fs_utime,
	.mknod      = fs_mknod,
	.write      = fs_write,
	.statfs     = fs_statfs,
	.open       = fs_open,
	.release    = fs_release,
	.opendir    = fs_opendir,
	.releasedir = fs_releasedir
};

int main(int argc, char *argv[])
{
	if(disk_init())
		{
		printf("Can't open virtual disk!\n");
		return -1;
		}
	if(mkfs())
		{
		printf("Mkfs failed!\n");
		return -2;
		}
    return fuse_main(argc, argv, &fs_operations, NULL);
}
