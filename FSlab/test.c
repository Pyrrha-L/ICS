#include <stdio.h>
#include <time.h>

int main()
{
	int * b=malloc(sizeof(int)*8);
	free(b);
	printf("ddd.");
		
	return 0;
}

/*
	//������ʾ��Ŀ¼�ġ�/�� 
	p++;
	//���·���У���һ�γ��֡�/��
	q = strchr(p, '/');
	
	
	q = strchr(p, '.');
	if (q != NULL) {
		*q = '\0';
		q++;     //q���ļ���׺ 
	}
 
	if (*tmp_path != '/') {
		//����ͬһ������ 
		while (offset < blk_info->size) {
			//Ѱ��ƥ�� 
			if (strcmp(file_dir->fname, tmp_path) == 0 && file_dir->flag == 2) {
				start_blk = file_dir->nStartBlock;
				break;
			}
			//������һ���ṹ�� 
			file_dir++;
			offset += sizeof(struct u_fs_file_directory);
		}
		//��Ŀ¼��û���ҵ�
		if (offset == blk_info->size) {
			free(blk_info);
			return -1;
		}
		//���뵽������Ŀ¼�ļ��� 
		if (disk_read(start_blk, blk_info)) {
			free(blk_info);
			return -1;
		}
		file_dir = (struct u_fs_file_directory*) blk_info->data;
		
		
	}
	
	offset = 0;
	
	while (offset < blk_info->size) {
		//���ļ���ȡ��attr�� 
		if (file_dir->flag != 0 && strcmp(file_dir->fname, p) == 0
				&& (q == NULL || strcmp(file_dir->fext, q) == 0)) {
			start_blk = file_dir->nStartBlock;
			read_file_dir(attr, file_dir);
			free(blk_info);
			return 0;
		}
		file_dir++;
		offset += sizeof(struct u_fs_file_directory);
	}

*/
