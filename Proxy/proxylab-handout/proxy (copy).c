#include <stdio.h>
#include "csapp.h"
/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char *conn_hdr = "Connection: close\r\n";
static const char *proxy_hdr = "Proxy-Connection: close\r\n";

struct Uri{
  char hostname[MAXLINE];
  char portname[MAXLINE];
  char pathname[MAXLINE];
};

void build_new_request(rio_t *real_client, char *new_request, struct Uri *u);
void Severdoit(int fd);
void parseUri(struct Uri *u, char* uri);
void clienterror(int fd, char *cause, char *errnum, 
		 char *shortmsg, char *longmsg);

int main(int argc, char **argv)
{
    //printf("%s", user_agent_hdr);
    int listenfd, connfd;
    char hostname[MAXLINE],port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    
    if(argc !=2)
    {
     fprintf(stderr,"(%s) arguments'nums wrong", argv[0]);
     exit(1);
    }
    
    listenfd = Open_listenfd(argv[1]);
    while(1)
    {
      clientlen=sizeof(clientaddr);
      connfd =Accept(listenfd, (SA *)&clientaddr, &clientlen);
      Getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
      printf("Accept connection from (%s, %s)\n", hostname, port);
      Severdoit(connfd);
      Close(connfd);
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
    while((char_nums = Rio_readlineb(&rioc, buf, MAXLINE)))
        {
          printf("From server: %d bytes return \n", (int)strlen(buf));
          Rio_writen(fd, buf, char_nums); 
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


