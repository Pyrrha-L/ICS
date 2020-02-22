/*
** 姓名：李梓童 
** 学号：2017202121 
*/

#include "cachelab.h"
#include <stdlib.h>
#include <stdio.h>
//根据cachelab.ppt的内容，使用getopt函数来解析输入的指令字符串
#include <getopt.h>
#include <unistd.h>
#include <string.h>

typedef struct Block {
	char valid;         // 判断是否有效 
	unsigned long tag;  // tag位 
	int data;           // 数据，实际用于LRU策略的判断 
} Block;

int hits = 0; 
int misses = 0;
int evictions = 0;

typedef struct cache {
	int s;				//组数（原始值，即指数）
	int b;				//块大小（原始值即指数） 
	int E;				//行数
	int block_num;		//块的数目 
	int size;			//每个cache的大小 
	Block *cacheBlock;  //每个cache的块的集合 
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

//addr为从trace文件中读取的“地址，字节数”的内容
//此函数目的为获取进行操作的字节数
char get_size(char* addr) {
	char size;
	int i;
	
	for(i = 0; i <= strlen(addr) ; i++) {
		if(addr[i] == ',') {
			size = addr[i + 1]; //把','后的字节数赋给size
			break;
		}
	}
	return size;
}

//addr为从trace文件中读取的“地址，字节数”的内容
//此函数目的为除去“,”，获取纯地址的字符串 
char* rid_comma(char* addr) {
	int length = strlen(addr);
	char* newaddr = malloc(length * sizeof(char));
	int i;
	
	for (i = 0; i <= length; i++) {
		if (addr[i] != ',') {
			newaddr[i] = addr[i];
		}
		else { //遇到“,”，则停止读数据 
			newaddr[i] = '\0';
			break;
		}
	}

	return newaddr;
}

//检查块匹配
void block_match(Cache cache, int tag2, int set_num,  char *optr, int* found, int* line_num) {
	int i;
	//从每组开始搜索，set_num记录定位到哪个组 
	//E：每组E行 
	for (i = 0; i < cache.E; i++) { 
		if ( cache.cacheBlock[(set_num * cache.E) + i].tag == tag2 &&
			cache.cacheBlock[(set_num * cache.E) + i].valid == 1 ) {
			//命中 
			hits++;
			
			//命中计数器 
			cache.cacheBlock[(set_num * cache.E) + i].data = *line_num;
			*found = 1;//找到之后，修正found值 

			//如果对数据进行的操作是modify（“又读又写”） 
			if (!(strcmp(optr, "M"))) {
				hits++;
				printf("Hit ");
			}
			printf("Hit\n");
			break;
	    }
	}
}

//查找空行 
void empty_lines(Cache cache, int index, int* evictionline, int* Full) {
	int i;
	//在组内查找 
	  for (i = 0; i < cache.E; i++) {
	  	if ( cache.cacheBlock[((index * cache.E)+i)].valid != 1) {
	  		*Full = 0;
	  		*evictionline = i;
	  		break;
	  	}
	  }
}

//存在可用空间 
void cache_eviction(Cache cache, int* Full, int index, int* temp, int* eviction_index, int* evict) {
	int i;
	//如果*Full==0,则说明存在空行，不需要进行替换。 
	if (*Full == 1) {
		for (i = 0; i < cache.E; i++) {
			// 决定替换块的编号：least-recently used			
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
	char option;		//选项卡 
	int v = 0; 			//有效无效的判断 
	char *file;			//读取的文件 

	//解析选项 
	while ((option = getopt(argc, argv, "hvs:E:b:t:")) != -1) {
		switch (option) {
			case ('h') :
				help();
				exit(0);
			case ('v') : 
				v = 1;		//准备打印详细信息 
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
				file = optarg;  //读取文件名 
				break;
			default :
				message();
		}
	}

	FILE *trace = fopen(file, "r");
	
	//计算块数 
	cache.block_num = (1<<cache.s) * cache.E;

	//计算cache的大小 
	cache.size = cache.block_num * (1<<cache.b);
	cache.cacheBlock = malloc(cache.size * sizeof(Block));
	int i;
	
	//对cache初始化 
	for (i = 0; i < cache.block_num; ++i) {
		cache.cacheBlock[i].valid = 0;
		cache.cacheBlock[i].tag = 0;
		cache.cacheBlock[i].data = 0;
	}

	char operation[10]; 	//存储操作 
	char addrs[20];  		//存储地址 
	int lineCount = 1; 			//计数器
	char* ptr; 					//地址转化时使用 
	
	//按行扫描，addr里存的包括地址和字节数的字符串 
	while(fscanf(trace, "%s %s ", operation, addrs) != EOF) {

		char* addrNC = rid_comma(addrs); 
		char Size = get_size(addrs); 				//获取字节数 
		int hexVersion = strtol(addrNC, &ptr, 16);  //把地址转化为十六进制数 
		
		//如果对数据进行的操作不是“I” 
		if (strcmp(operation, "I")) {

			if (v == 1) {
				printf(" %s %s, %c ", operation, addrNC, Size);
			}

			//计算tag位 
			int tag2 = hexVersion >> (cache.b + cache.s);
			
			//计算组数 
			int setIndex = ( (hexVersion ^ (tag2 << (cache.b + cache.s))) >> cache.b);

			int match = 0; 
			int evict = 0; 
			
			block_match(cache, tag2, setIndex, operation, &match, &lineCount);

			//如果没有匹配到相应的块，那么进一步操作 
			if (!match) {
				misses++;
				printf("miss ");
				int Full = 1;
				int eviction_index; 
				int temp = lineCount; //暂存编号 

				empty_lines(cache, setIndex, &eviction_index, &Full);

				cache_eviction(cache, &Full, setIndex, &temp, &eviction_index, &evict);
				//进行驱逐和替换 
				cache.cacheBlock[(setIndex * cache.E) + eviction_index].valid = 1;
				cache.cacheBlock[(setIndex * cache.E) + eviction_index].tag = tag2;
				cache.cacheBlock[(setIndex * cache.E) + eviction_index].data = lineCount;

				if (evict) {
					printf("eviction ");
				}
				
				//如果对数据进行的操作是modify（又读又写） 
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
