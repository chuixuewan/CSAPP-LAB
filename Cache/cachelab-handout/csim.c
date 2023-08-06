#include "cachelab.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <stdbool.h>
#include <limits.h>
#define MININT -1
extern char *optarg;
extern int optind,opterr,optopt;
struct cache{
     int* Lps;        //Line per set
     int* count; //LRU time count
     int* valid;//flag
     
};
int hits,misses,evictions;

void update_stamp(int S,int E,struct cache * cache)
{
	for(int i = 0; i < S; ++i)
		for(int j = 0; j < E; ++j)
			if(cache[i].valid[j] == 1)
				++cache[i].count[j];
}
void Visit(int setag,int tag,struct cache* cach,int s,int E,int v)
{
       int j,k; int WhichSe;
       int no= -1,LRU=INT_MIN;
       WhichSe=setag;
      
  
       
        for(j=0;j<E;j++)
        {
         if(cach[WhichSe].valid[j]&&cach[WhichSe].Lps[j]==tag)
          {
          if(v==1)
          printf("hit ");
          hits++;
           cach[WhichSe].count[j]=0;
           return;
           }
          }  
         
		 for(j=0;j<E;j++)
		 {
		  if(cach[WhichSe].valid[j]==0)
		  {
		    if(v==1)
		    printf("miss ");
		    misses++;
		    cach[WhichSe].valid[j]=1;
		    cach[WhichSe].count[j]=0;
		    cach[WhichSe].Lps[j]=tag;
		    return;
		  }
		 }  
         
           
          
    
         for(k=0;k<E;k++)
         {
            if(LRU<cach[WhichSe].count[k])
            {
              LRU=cach[WhichSe].count[k];
              no=k;
            }
         }
        if(v==1)
        printf("miss ");
        misses++;  
        cach[WhichSe].Lps[no]=tag;
        cach[WhichSe].count[no]=0;
        if(v==1)
        printf("eviction ");
        evictions++;
        
       
        if(v==1)
       printf("\n");
     
}

int main(int argc, char *argv[])
{
   hits=misses=evictions=0;
      // 定义变量来存储选项和参数
    int opt; // 用于接收getopt的返回值
    int hflag = 0; // 用于判断是否有-h选项
    int vflag = 0; // 用于判断是否有-v选项
    int E, b; // 用于存储-s,-E,-b的参数
    int s,S;
    char *t; // 用于存储-t的参数
    char Mode;
   
    int size;
    
    int i,j;
    
    unsigned int num; // 用于存储转换后的长整数
   
    
    struct cache* Cach;
   
    int tag;
   
    int setag;
    
    if (argc < 9) {
        fprintf(stderr, "Arg Lack！！！\n");
        exit(1);
    }
    // 使用getopt函数解析命令行选项
    while ((opt = getopt(argc, argv, "hvs:E:b:t:")) != -1) {
        switch (opt) {
            case 'h':
                hflag = 1;
                break;
            case 'v':
                vflag = 1;
                break;
            case 's':
                s = atoi(optarg); // 将字符串转换为整数
                break;
            case 'E':
                E = atoi(optarg);
                break;
            case 'b':
                b = atoi(optarg);
                break;
            case 't':
                t = optarg; // 直接赋值字符串指针
                break;
            default: // 如果遇到无效的选项，打印错误信息并退出
                fprintf(stderr, "Invalid option.\n");
                exit(1);
        }
    }
     S = 1 << s;
    Cach=(struct cache*)malloc(S * sizeof(struct cache));
    if(Cach==NULL)
    {
    printf("内存分配失败！\n");
    exit(EXIT_FAILURE);
    }
    for(i=0;i<S;i++)// cache init
    {
        
        Cach[i].Lps=(int*)malloc(E*sizeof(int));
        Cach[i].count=(int*)malloc(E*sizeof(int));
        Cach[i].valid=(int*)malloc(E*sizeof(int));
        for(j=0;j<E;j++)
        {
        Cach[i].count[j]=-1;
        Cach[i].Lps[j]=-1;
        Cach[i].valid[j]=0;
        }
    }
    FILE *fp=fopen(t,"r");
     if (fp == NULL) { // 判断文件是否打开成功
        printf("无法打开文件\n");
        return -1;
    }
    while (fscanf(fp, " %c %xu,%d\n", &Mode, &num, &size) > 0) { // 循环读取文件中的每一行
      
        setag=(num >> b) & ((-1U) >> (64 - s));
        tag=num >> (b + s);
       
        
        switch(Mode)
		{
			//case 'I': continue;	   // 不用写关于 I 的判断也可以
			case 'L':
				Visit(setag,tag,Cach,s,E,vflag);
				break;
			case 'M':
				Visit(setag,tag,Cach,s,E,vflag); // miss的话还要进行一次storage
			case 'S':
				Visit(setag,tag,Cach,s,E,vflag);
		}
		update_stamp(S,E,Cach);	//更新时间戳
        
    }
    
  
    for(int i=0;i<S;i++)
    {
    free(Cach[i].Lps);
    free(Cach[i].count);
    free(Cach[i].valid);
    }
    free(Cach);
    
    fclose(fp); // 关闭文件
    printSummary(hits,  /* number of  hits */
				  misses, /* number of misses */
				  evictions);
    return 0;
}



