/*
** ����������ͯ 
** ѧ�ţ�2017202121 
*/

#include "cachelab.h"
#include <stdlib.h>
#include <stdio.h>
//����cachelab.ppt�����ݣ�ʹ��getopt���������������ָ���ַ���
#include <getopt.h>
#include <unistd.h>
#include <string.h>

typedef struct Block {
	char valid;         // �ж��Ƿ���Ч 
	unsigned long tag;  // tagλ 
	int data;           // ���ݣ�ʵ������LRU���Ե��ж� 
} Block;

int hits = 0; 
int misses = 0;
int evictions = 0;

typedef struct cache {
	int s;				//������ԭʼֵ����ָ����
	int b;				//���С��ԭʼֵ��ָ���� 
	int E;				//����
	int block_num;		//�����Ŀ 
	int size;			//ÿ��cache�Ĵ�С 
	Block *cacheBlock;  //ÿ��cache�Ŀ�ļ��� 
} Cache;

void help() {
	printf ("Usage: ./csim-ref [-hv] -s <s> -E <E> -b <b> -t <tracefile> \n");
	printf ("-h : Optional help flag that prints usage info\n");
	printf ("-v : Optional verbose flag that displays trace info \n");
	printf ("-s <s> : Number of set index bits (the number of sets is 2s)\n");
	printf ("-E <E> : Associativity (number of lines per set)\n");
	printf ("-b <b>: Number of block bits (the block size is 2b)\n");
	printf ("-t <tracefile> : Name of the valgrind trace to replay\n");
	return;
}

//addrΪ��trace�ļ��ж�ȡ�ġ���ַ���ֽ�����������
//�˺���Ŀ��Ϊ��ȡ���в������ֽ���
char get_size(char* addr) {
	char size;
	int i;
	
	for(i = 0; i <= strlen(addr) ; i++) {
		if(addr[i] == ',') {
			size = addr[i + 1]; //��','����ֽ�������size
			break;
		}
	}
	return size;
}

//addrΪ��trace�ļ��ж�ȡ�ġ���ַ���ֽ�����������
//�˺���Ŀ��Ϊ��ȥ��,������ȡ����ַ���ַ��� 
char* rid_comma(char* addr) {
	int length = strlen(addr);
	char* newaddr = malloc(length * sizeof(char));
	int i;
	
	for (i = 0; i <= length; i++) {
		if (addr[i] != ',') {
			newaddr[i] = addr[i];
		}
		else { //������,������ֹͣ������ 
			newaddr[i] = '\0';
			break;
		}
	}

	return newaddr;
}

//����ƥ��
void block_match(Cache cache, int tag2, int set_num,  char *optr, int* found, int* line_num) {
	int i;
	//��ÿ�鿪ʼ������set_num��¼��λ���ĸ��� 
	//E��ÿ��E�� 
	for (i = 0; i < cache.E; i++) { 
		if ( cache.cacheBlock[(set_num * cache.E) + i].tag == tag2 &&
			cache.cacheBlock[(set_num * cache.E) + i].valid == 1 ) {
			//���� 
			hits++;
			
			//���м����� 
			cache.cacheBlock[(set_num * cache.E) + i].data = *line_num;
			*found = 1;//�ҵ�֮������foundֵ 

			//��������ݽ��еĲ�����modify�����ֶ���д���� 
			if (!(strcmp(optr, "M"))) {
				hits++;
				printf("Hit ");
			}
			printf("Hit\n");
			break;
	    }
	}
}

//���ҿ��� 
void empty_lines(Cache cache, int index, int* evictionline, int* Full) {
	int i;
	//�����ڲ��� 
	  for (i = 0; i < cache.E; i++) {
	  	if ( cache.cacheBlock[((index * cache.E)+i)].valid != 1) {
	  		*Full = 0;
	  		*evictionline = i;
	  		break;
	  	}
	  }
}

//���ڿ��ÿռ� 
void cache_eviction(Cache cache, int* Full, int index, int* temp, int* eviction_index, int* evict) {
	int i;
	//���*Full==0,��˵�����ڿ��У�����Ҫ�����滻�� 
	if (*Full == 1) {
		for (i = 0; i < cache.E; i++) {
			// �����滻��ı�ţ�least-recently used			
				if (cache.cacheBlock[(index * cache.E) + i].data < *temp) {
				*temp = cache.cacheBlock[(index * cache.E) + i].data;
				*eviction_index = i;
			}
		}
		evictions++;
		*evict = 1;	
	}
}


int main(int argc, char *argv[]) {
 
	if (argc == 1) {
		help();
	}

	Cache cache;
	char option;		//ѡ� 
	int v = 0; 			//��Ч��Ч���ж� 
	char *file;			//��ȡ���ļ� 

	//����ѡ�� 
	while ((option = getopt(argc, argv, "hvs:E:b:t:")) != -1) {
		switch (option) {
			case ('h') :
				help();
				exit(0);
			case ('v') : 
				v = 1;		//׼����ӡ��ϸ��Ϣ 
				break;
			case ('s') :
				cache.s = atoi(optarg);
				break;
			case ('E') :
				cache.E = atoi(optarg);
				break;
			case ('b') :
				cache.b = atoi(optarg);
				break;
			case ('t') :
				file = optarg;  //��ȡ�ļ��� 
				break;
			default :
				message();
		}
	}

	FILE *trace = fopen(file, "r");
	
	//������� 
	cache.block_num = (1<<cache.s) * cache.E;

	//����cache�Ĵ�С 
	cache.size = cache.block_num * (1<<cache.b);
	cache.cacheBlock = malloc(cache.size * sizeof(Block));
	int i;
	
	//��cache��ʼ�� 
	for (i = 0; i < cache.block_num; ++i) {
		cache.cacheBlock[i].valid = 0;
		cache.cacheBlock[i].tag = 0;
		cache.cacheBlock[i].data = 0;
	}

	char operation[10]; 	//�洢���� 
	char addrs[20];  		//�洢��ַ 
	int lineCount = 1; 			//������
	char* ptr; 					//��ַת��ʱʹ�� 
	
	//����ɨ�裬addr���İ�����ַ���ֽ������ַ��� 
	while(fscanf(trace, "%s %s ", operation, addrs) != EOF) {

		char* addrNC = rid_comma(addrs); 
		char Size = get_size(addrs); 				//��ȡ�ֽ��� 
		int hexVersion = strtol(addrNC, &ptr, 16);  //�ѵ�ַת��Ϊʮ�������� 
		
		//��������ݽ��еĲ������ǡ�I�� 
		if (strcmp(operation, "I")) {

			if (v == 1) {
				printf(" %s %s, %c ", operation, addrNC, Size);
			}

			//����tagλ 
			int tag2 = hexVersion >> (cache.b + cache.s);
			
			//�������� 
			int setIndex = ( (hexVersion ^ (tag2 << (cache.b + cache.s))) >> cache.b);

			int match = 0; 
			int evict = 0; 
			
			block_match(cache, tag2, setIndex, operation, &match, &lineCount);

			//���û��ƥ�䵽��Ӧ�Ŀ飬��ô��һ������ 
			if (!match) {
				misses++;
				printf("miss ");
				int Full = 1;
				int eviction_index; 
				int temp = lineCount; //�ݴ��� 

				empty_lines(cache, setIndex, &eviction_index, &Full);

				cache_eviction(cache, &Full, setIndex, &temp, &eviction_index, &evict);
				//����������滻 
				cache.cacheBlock[(setIndex * cache.E) + eviction_index].valid = 1;
				cache.cacheBlock[(setIndex * cache.E) + eviction_index].tag = tag2;
				cache.cacheBlock[(setIndex * cache.E) + eviction_index].data = lineCount;

				if (evict) {
					printf("eviction ");
				}
				
				//��������ݽ��еĲ�����modify���ֶ���д�� 
				if (!(strcmp(operation, "M"))) {
					hits++;
					printf("hit");
				}

				printf("\n");
			}

			lineCount++;

		}
	}

    printSummary(hits, misses, evictions);
    return 0;
}
