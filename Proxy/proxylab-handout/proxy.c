#include <stdio.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
#define MAX_CACHE 10
#define SBUFSIZE 16
#define NTHREADS 4

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char *conn_hdr = "Connection: close\r\n";
static const char *proxy_hdr = "Proxy-Connection: close\r\n";

struct Uri{
  char hostname[MAXLINE];
  char portname[MAXLINE];
  char pathname[MAXLINE];
};
typedef struct {
    int *buf;          /* Buffer array */         
    int n;             /* Maximum number of slots */
    int front;         /* buf[(front+1)%n] is first item */
    int rear;          /* buf[rear%n] is last item */
    sem_t mutex;       /* Protects accesses to buf */
    sem_t slots;       /* Counts available slots */
    sem_t items;       /* Counts available items */
} sbuf_t;

typedef struct
{
    char obj[MAX_OBJECT_SIZE];
    char uri[MAXLINE];
    int LRU;
    int isEmpty;

    int read_cnt; //读者数量
    sem_t w;      //保护Cache
    sem_t mutex;  //保护 read_cnt

} block;

typedef struct
{
    block data[MAX_CACHE];
    int num;
} Cache;

Cache cache;

void init_Cache();
int get_Cache(char *url);
int get_Index();
void update_LRU(int index);
void write_Cache(char *uri, char *buf);

/* $end sbuft */
sbuf_t sbuf;
void sbuf_init(sbuf_t *sp, int n);
void sbuf_deinit(sbuf_t *sp);
void sbuf_insert(sbuf_t *sp, int item);
int sbuf_remove(sbuf_t *sp);

void build_new_request(rio_t *real_client, char *new_request, struct Uri *u);
void Severdoit(int fd);
void parseUri(struct Uri *u, char* uri);
void clienterror(int fd, char *cause, char *errnum, 
		 char *shortmsg, char *longmsg);

void *thread(void *vargp)
{
   Pthread_detach(pthread_self());
   while(1)
   {
      int connfd=sbuf_remove(&sbuf);
      Severdoit(connfd);
      Close(connfd);
   
   }

}		 
		 

int main(int argc, char **argv)
{
    //printf("%s", user_agent_hdr);
    int listenfd, connfd;
    char hostname[MAXLINE],port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    int i;
    pthread_t tid;
    init_Cache();
    if(argc !=2)
    {
     fprintf(stderr,"(%s) arguments'nums wrong", argv[0]);
     exit(1);
    }
    
    listenfd = Open_listenfd(argv[1]);
    
    sbuf_init(&sbuf,16);
    
    for(i=0;i<4;i++)
    Pthread_create(&tid, NULL, thread, NULL);
    
    while(1)
    {
      clientlen=sizeof(clientaddr);
      connfd =Accept(listenfd, (SA *)&clientaddr, &clientlen);
      Getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
      printf("Accept connection from (%s, %s)\n", hostname, port);
      sbuf_insert(&sbuf, connfd);
    }
    
    return 0;
}

void Severdoit(int fd) 
{
    struct stat sbuf;
    struct Uri u;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char hosthead[10],  host[MAXLINE];
    char filename[MAXLINE], cgiargs[MAXLINE];
    char hostname[MAXLINE], port[MAXLINE];
    char* ptr;
    rio_t rios, rioc;
    int proxyfd;
    char* cache_tag[MAXLINE];
    /* Read request line and headers */
    Rio_readinitb(&rios, fd);
    if (!Rio_readlineb(&rios, buf, MAXLINE))  //line:netp:doit:readrequest
        return;
    printf("From client:\n");    
    printf("%s", buf);
    sscanf(buf, "%s %s %s", method, uri, version);       //line:netp:doit:parserequest
    if (strcasecmp(method, "GET")) {                     //line:netp:doit:beginrequesterr
        clienterror(fd, method, "501", "Not Implemented",
                    "Proxy only implement GET method");
        return;
    }
    int i;
    strcpy(cache_tag, uri);
    if((i=get_Cache(uri))!=-1)
    {
     P(&cache.data[i].mutex);
     cache.data[i].read_cnt++;
     if(cache.data[i].read_cnt==1)
      P(&cache.data[i].w);
     V(&cache.data[i].mutex); 
      
      Rio_writen(fd, cache.data[i].obj, strlen(cache.data[i].obj));
      
      P(&cache.data[i].mutex);
     cache.data[i].read_cnt--;
     if(cache.data[i].read_cnt==0)
      V(&cache.data[i].w);
     V(&cache.data[i].mutex); 
     
     return;
    }
    
    parseUri(&u,uri);
    proxyfd=open_clientfd(u.hostname,u.portname);
    if(!proxyfd)
    {
    printf("connection failed\n");
    return;
    }
    
    char new_header[MAXLINE];
    sprintf(new_header, "GET %s HTTP/1.0\r\n", u.pathname);
    
    Rio_readinitb(&rioc, proxyfd);
    build_new_request(&rios, new_header, &u);
    
    Rio_writen(proxyfd, new_header, strlen(new_header)); 
    int char_nums;
    char cache_buf[MAX_OBJECT_SIZE];
    int size_buf = 0;
    while((char_nums = Rio_readlineb(&rioc, buf, MAXLINE)))
        {
          printf("From server: %d bytes return \n", (int)strlen(buf));
          size_buf += char_nums;
        if(size_buf < MAX_OBJECT_SIZE)
            strcat(cache_buf, buf);
          Rio_writen(fd, buf, char_nums); 
        } 
        Close(proxyfd);
         if(size_buf < MAX_OBJECT_SIZE){
        write_Cache(cache_tag, cache_buf);
    }                               
}

void parseUri(struct Uri *u, char* uri){

   char* thostname=strstr(uri,"//");
   char* tportname;
   char* tpathname;
   if(thostname)
   {
      thostname+=2;
       
   }
   else
     thostname=uri;
     
     tportname=strstr(thostname,":");
     
     if(tportname)
     {
        *tportname='\0';
        strncpy(u->hostname,thostname,MAXLINE);
        
        if(tpathname=strstr(tportname+1,"/"))
        {
           *tpathname='\0';
        strncpy(u->portname,tportname+1,MAXLINE);
           *tpathname='/';
        strncpy(u->pathname,tpathname,MAXLINE);
           return;
        }
        else
        {
          strncpy(u->portname,tportname+1,MAXLINE);
          return;
        }
        
     }
     
     else
     {
        strcpy(u->portname,"80");
       if(tpathname=strstr(tportname+1,"/"))
        {
        *tpathname='\0';
        strncpy(u->hostname,thostname,MAXLINE);
           *tpathname='/';
        strncpy(u->pathname,tpathname,MAXLINE);
           return;
        }
        strncpy(u->hostname,thostname,MAXLINE);
        strcpy(u->pathname,"/");
        
     }
     
     return;
     

}
void build_new_request(rio_t *real_client, char *new_request, struct Uri *u)
{
char temp_buf[MAXLINE];

    /* get old request_hdr */
    while(Rio_readlineb(real_client, temp_buf, MAXLINE) > 0){
        if (strstr(temp_buf, "\r\n")) break; /* read to end */

        /* if all old request_hdr had been read, we get it */
        if (strstr(temp_buf, "Host:")) continue;
        if (strstr(temp_buf, "User-Agent:")) continue;
        if (strstr(temp_buf, "Connection:")) continue;
        if (strstr(temp_buf, "Proxy Connection:")) continue;

        sprintf(new_request, "%s%s", new_request, temp_buf);
    }

    /* build new request_hdr */
    sprintf(new_request, "%sHost: %s:%s\r\n", new_request, u->hostname, u->portname);
    sprintf(new_request, "%s%s%s%s", new_request, user_agent_hdr, conn_hdr, proxy_hdr);
    sprintf(new_request,"%s\r\n", new_request);

}

void clienterror(int fd, char *cause, char *errnum, 
		 char *shortmsg, char *longmsg) 
{
    char buf[MAXLINE];

    /* Print the HTTP response headers */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n\r\n");
    Rio_writen(fd, buf, strlen(buf));

    /* Print the HTTP response body */
    sprintf(buf, "<html><title>Tiny Error</title>");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<body bgcolor=""ffffff"">\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "%s: %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<p>%s: %s\r\n", longmsg, cause);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<hr><em>The Tiny Web server</em>\r\n");
    Rio_writen(fd, buf, strlen(buf));
}
/* Create an empty, bounded, shared FIFO buffer with n slots */
/* $begin sbuf_init */
void sbuf_init(sbuf_t *sp, int n)
{
    sp->buf = Calloc(n, sizeof(int)); 
    sp->n = n;                       /* Buffer holds max of n items */
    sp->front = sp->rear = 0;        /* Empty buffer iff front == rear */
    Sem_init(&sp->mutex, 0, 1);      /* Binary semaphore for locking */
    Sem_init(&sp->slots, 0, n);      /* Initially, buf has n empty slots */
    Sem_init(&sp->items, 0, 0);      /* Initially, buf has zero data items */
}
/* $end sbuf_init */

/* Clean up buffer sp */
/* $begin sbuf_deinit */
void sbuf_deinit(sbuf_t *sp)
{
    Free(sp->buf);
}
/* $end sbuf_deinit */

/* Insert item onto the rear of shared buffer sp */
/* $begin sbuf_insert */
void sbuf_insert(sbuf_t *sp, int item)
{
    P(&sp->slots);                          /* Wait for available slot */
    P(&sp->mutex);                          /* Lock the buffer */
    sp->buf[(++sp->rear)%(sp->n)] = item;   /* Insert the item */
    V(&sp->mutex);                          /* Unlock the buffer */
    V(&sp->items);                          /* Announce available item */
}
/* $end sbuf_insert */

/* Remove and return the first item from buffer sp */
/* $begin sbuf_remove */
int sbuf_remove(sbuf_t *sp)
{
    int item;
    P(&sp->items);                          /* Wait for available item */
    P(&sp->mutex);                          /* Lock the buffer */
    item = sp->buf[(++sp->front)%(sp->n)];  /* Remove the item */
    V(&sp->mutex);                          /* Unlock the buffer */
    V(&sp->slots);                          /* Announce available slot */
    return item;
}
/* $end sbuf_remove */
/* $end sbufc */

  void init_Cache()
{
    cache.num = 0;
    int i;
    for (i = 0; i < MAX_CACHE; i++)
    {
        cache.data[i].LRU = 0;
        cache.data[i].isEmpty = 1;
        // w, mutex均初始化为1
        Sem_init(&cache.data[i].w, 0, 1);
        Sem_init(&cache.data[i].mutex, 0, 1);
        cache.data[i].read_cnt = 0;
    }
}

//从Cache中找到内容
int get_Cache(char *url)
{
    int i;
    for (i = 0; i < MAX_CACHE; i++)
    {
        //读者加锁
        P(&cache.data[i].mutex);
        cache.data[i].read_cnt++;
        if (cache.data[i].read_cnt == 1)
            P(&cache.data[i].w);
        V(&cache.data[i].mutex);

        if ((cache.data[i].isEmpty == 0) && (strcmp(url, cache.data[i].uri) == 0))
            {
                P(&cache.data[i].mutex);
                cache.data[i].read_cnt--;
                if (cache.data[i].read_cnt == 0)
                V(&cache.data[i].w);
                V(&cache.data[i].mutex);
                break;
            }

        P(&cache.data[i].mutex);
        cache.data[i].read_cnt--;
        if (cache.data[i].read_cnt == 0)
            V(&cache.data[i].w);
        V(&cache.data[i].mutex);
    }
    if (i >= MAX_CACHE)
        return -1;
    return i;
}

//找到可以存放的缓存行
int get_Index()
{
    int min = __INT_MAX__;
    int minindex = 0;
    int i;
    for (i = 0; i < MAX_CACHE; i++)
    {
        //读锁
        P(&cache.data[i].mutex);
        cache.data[i].read_cnt++;
        if (cache.data[i].read_cnt == 1)
            P(&cache.data[i].w);
        V(&cache.data[i].mutex);

        if (cache.data[i].isEmpty == 1)
        {
            minindex = i;
            P(&cache.data[i].mutex);
            cache.data[i].read_cnt--;
            if (cache.data[i].read_cnt == 0)
                V(&cache.data[i].w);
            V(&cache.data[i].mutex);
            break;
        }
        if (cache.data[i].LRU < min)
        {
            minindex = i;
            P(&cache.data[i].mutex);
        cache.data[i].read_cnt--;
        if (cache.data[i].read_cnt == 0)
            V(&cache.data[i].w);
        V(&cache.data[i].mutex);
        continue;
        }

        P(&cache.data[i].mutex);
        cache.data[i].read_cnt--;
        if (cache.data[i].read_cnt == 0)
            V(&cache.data[i].w);
        V(&cache.data[i].mutex);
    }

    return minindex;
}
//更新LRU
void update_LRU(int index)
{
    for (int i = 0; i < MAX_CACHE; i++)
    {

        if (cache.data[i].isEmpty == 0 && i != index)
        {
            P(&cache.data[i].w);
            cache.data[i].LRU--;
            V(&cache.data[i].w);
        }
    }
}
//写缓存
void write_Cache(char *uri, char *buf)
{

    int i = get_Index();
    //加写锁
    P(&cache.data[i].w);
    //写入内容
    strcpy(cache.data[i].obj, buf);
    strcpy(cache.data[i].uri, uri);
    cache.data[i].isEmpty = 0;
    cache.data[i].LRU = __INT_MAX__;
    update_LRU(i);

    V(&cache.data[i].w);
}
