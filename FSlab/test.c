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
	//跳过表示根目录的‘/’ 
	p++;
	//相对路径中，第一次出现‘/’
	q = strchr(p, '/');
	
	
	q = strchr(p, '.');
	if (q != NULL) {
		*q = '\0';
		q++;     //q：文件后缀 
	}
 
	if (*tmp_path != '/') {
		//仍在同一个块中 
		while (offset < blk_info->size) {
			//寻找匹配 
			if (strcmp(file_dir->fname, tmp_path) == 0 && file_dir->flag == 2) {
				start_blk = file_dir->nStartBlock;
				break;
			}
			//跳到下一个结构体 
			file_dir++;
			offset += sizeof(struct u_fs_file_directory);
		}
		//根目录下没有找到
		if (offset == blk_info->size) {
			free(blk_info);
			return -1;
		}
		//进入到读出的目录文件里 
		if (disk_read(start_blk, blk_info)) {
			free(blk_info);
			return -1;
		}
		file_dir = (struct u_fs_file_directory*) blk_info->data;
		
		
	}
	
	offset = 0;
	
	while (offset < blk_info->size) {
		//把文件读取到attr里 
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
