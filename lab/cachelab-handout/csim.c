#include "cachelab.h"
// author:qycl50224


// address = 64 bits, so m = 64, t = m - (b + s)
// only consider implementation of L and M and S, no I
// use LRU(least recently used), so should track the last used time
// input:  L 10,4 , what we should consider is the first instruction and the address
// we should find the set which contains the address by find the block which contains the 
// adderss
// target:track hit, miss and eviction times 

int main()
{
    printSummary(0, 0, 0);
    return 0;
}

void read()
{
	int s;
	int E;
	int b;	
}

int calcSetSize(int s)
{
	int res = 1;
	int i;
	for (i = 1; i <= s; i++)
	{
		res *= 2;
	}
	return res;
}

int calcBlockSize(int b)
{
	int res = 1;
	int i;
	for (i = 1; i <= b; i++)
	{
		res *= 2;
	}
	return res;
}

int calcTag(int s, int b, int m)
{
	return m-(b+s);
}

int calct(int setSize, int address)
{
	return address / setSize;
}

int getIndexOfBlock(int address, int blockSize)
{
	return address / blockSize;
}
	
int getIndexOfSet(int blockIdx)
{
	return blockIdx % 4;
}

// int getDecimalAddress()


Boolean isValid(int v) 
{
	return v == 1? true:false;
}

Boolean sameTag(int realTag, int tag)
{
	return realTag == tag;
}


