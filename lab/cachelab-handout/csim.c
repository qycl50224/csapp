#include "cachelab.h"
#include <getopt.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// author:qycl50224
#define m 64

int calcS(int s)
{
    int res = 1;
    int i;
    for (i = 1; i <= s; i++) {
        res *= 2;
    }
    return res;
}

unsigned gett(unsigned long address, int b, int s)
{
    return address >> (b+s);
}

int getS(unsigned long address, int t, int b)
{
    return ((address << t) >> t) >> b;
}
    
struct cache_line {
    int valid;
    unsigned tag;
    int time;
};

void loadOrStore(int E, unsigned t, int S, struct cache_line cache[S][E], unsigned long address,int * hits, int * misses, int * eviction)
{
    int e;
    int flag = 0; // check if hit
    int haveEmpty = 0; // check is there any line with valid=0
    for (e = 0; e < E; e++) {
        cache[S][e].time++;     // update every line's time
        if(cache[S][e].valid == 0)
        {
            haveEmpty = 1;
        }
        if(cache[S][e].valid == 1 && cache[S][e].tag == t)
        {
            cache[S][e].time = 0;
            flag = 1;
            ++*hits;
            printf("hit\n");
        }
    }
    if(flag == 0)
    {
        ++*misses;
        printf("miss\n");
        if (haveEmpty == 1)
        {
            for (e = 0; e < E; e++)
            {
                if (cache[S][e].valid == 0)
                {
                    cache[S][e].valid = 1;
                    cache[S][e].tag = t;
                    cache[S][e].time = 0;   
                    return;
                }
            }
        } else {
            ++*eviction;
            printf("eviction\n");
            double max = 0;
            int target = 0;
            for (e = 0; e < E; e++)
            {
                int time = cache[S][e].time;
                if (time > max)
                {
                    max = time;
                    printf("%f\n", max);
                    target = e;
                }
            }
            cache[S][target].tag = t;
            cache[S][target].time = 0;
        }   
    }
}



void modifyData(int E, unsigned t, int S, struct cache_line cache[S][E], unsigned long address,int * hits, int * misses, int * eviction)
{
    loadOrStore(E,t,S,cache, address, hits, misses, eviction);
    loadOrStore(E,t,S,cache, address, hits, misses, eviction);
    return;
}

int main(int argc, char** argv) 
{
    int * hits;
    int * misses;
    int * eviction;
    int ini1 = 0;
    int ini2 = 0;
    int ini3 = 0;
    hits = &ini1;
    misses = &ini2;
    eviction = &ini3; 

    int s,E,opt,S,b,t;
    char * file = NULL;
    while(-1 != (opt = getopt(argc, argv, "s:E:b:t:")))
    {
        sleep(0.01);
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
    // printf("t=%d\n", t);
    // printf("S=%d\n", S);
    struct cache_line cache[S][E];
    int setCount;
    int eCount;
    // initialize array
    for (setCount = 0; setCount < S; setCount++) {
        for (eCount = 0; eCount < E; eCount++) {
            cache[setCount][eCount].tag = 0;
            cache[setCount][eCount].valid = 0;
            cache[setCount][eCount].time = 0;
        }
    }

    FILE * pFile;
    pFile = fopen(file, "r");

    char identifier;
    unsigned long address;
    int size;

    while(fscanf(pFile, " %c %lx,%d", &identifier, &address, &size)>0)
    {   

        // printf("sbits former  %lx\n", address );
        int sbits = getS(address, t, b);
        // printf("sbits  %x\n", sbits );
        unsigned tbits = gett(address, b, s);
        // printf("tagbits  %d\n", tbits);
        // printf("Instruction %c address %lx  sbits %d\n", identifier, address, sbits);
        if(identifier == 'L')
        {

            // printf("set = %d\n", sbits);
            // printf("tag bit = %d\n", tbits);
            // printf("identifier = %c\n", identifier);
            // printf("size %d\n", size);
            loadOrStore(E, tbits, sbits, cache, address, hits, misses, eviction);
        } else if(identifier == 'S')
        {

            loadOrStore(E, tbits, sbits, cache, address, hits, misses, eviction);
        } else if(identifier == 'M'){
            modifyData(E, tbits, sbits, cache, address, hits, misses, eviction);
        } else {
            continue;
        }
    }
    fclose(pFile);
    printSummary(*hits, *misses, *eviction);
    return 0;
}