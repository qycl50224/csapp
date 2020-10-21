#include "cachelab.h"
#include <getopt.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// author:qycl50224
#define m 64

// address = 64 bits, so m = 64, t = m - (b + s)
// only consider implementation of L and M and S, no I
// use LRU(least recently used), so should track the last used time
// input:  L 10,4 , what we should consider is the first instruction and the address
// we should find the set which contains the address by find the block which contains the 
// adderss
// target:track hit, miss and eviction times 


// read input s,E,b
// initalize hit, miss, eviction
// calcS,B,tagBit	
// malloc C = S * E * (2^(b+tag+1)) byte
// read instruction 
//  if (instruction == 'I') jump
//	else if (instruction == 'L')
//		read address
// 		calc idxOfS,t
// 		for (E raw: S) 
//			if (isValid && sameTag) print(hit) hit++  update access time flag = 1
//		if (flag == 0) print(miss) miss++  
// iterate all row in that set and
// check valid bit and 
// check tag bit
// if (isValid && sameTag) 


int calcS(int s)
{
	int res = 1;
	int i;
	for (i = 1; i <= s; i++) {
		res *= 2;
	}
	return res;
}

// int calcB(int b)
// {
// 	int res = 1;
// 	int i;
// 	for (i = 1; i <= b; i++) {
// 		res *= 2;
// 	}
// 	return res;
// }

unsigned gett(unsigned long address, int b, int s)
{
	return address >> (b+s);
}

int getS(unsigned long address, int t, int b)
{
	return (((address << t) >> t) >> b) << b;
}
	

int isValid(int v) 
{
	return v? 1:0;
}

int sameTag(int realTag, int tag)
{
	return realTag == tag;
}

// void loadData(unsigned t, int S, struct cache_line[][] cache)
// {
	
// }


struct cache_line {
	int valid;
	unsigned tag;
	time_t time;
};

int main(int argc, char** argv) 
{
	int s,E,opt,S,b,t;
	char * file = NULL;
	while(-1 != (opt = getopt(argc, argv, "s:E:b:t:")))
	{
		switch(opt)
		{
			case 's':
				s = atoi(optarg);
				printf("s=%d\n", s);
				break;
			case 'E':
				E = atoi(optarg);
				printf("E=%d\n", E);
				break;
			case 'b':
				b = atoi(optarg);
				break;
			case 't':
				file = optarg;	
				printf("file is %s\n", file);
				break;
			default:
				printf("wrong argument\n");
				break;
		}
	}

	S = calcS(s);
	t = m - (s + b);


	struct cache_line cache[S][E];
	cache[0][0].valid = 1;
	printf("valid = %d\n", cache[0][0].valid);

	FILE * pFile;
	pFile = fopen(file, "r");

	char identifier;
	unsigned long address;
	int size;

	while(fscanf(pFile, " %c %lx,%d", &identifier, &address, &size)>0)
	{	
		int sbits = getS(address, t, b);
		unsigned tbits = gett(address, b, s);
		if(identifier == 'L')
		{

			printf("set = %d\n", sbits);
			printf("tag bit = %d\n", tbits);
			// printf("identifier = %c\n", identifier);
			// printf("address %lx\n", address);
			// printf("size %d\n", size);
			// loadData(address);
		} else if(identifier == 'S')
		{

			// storeData(address);
		} else if(identifier == 'M'){
			// modify(address);
		} else {
			continue;
		}
	}
	fclose(pFile);

	printSummary(0, 0, 0);
    return 0;
}