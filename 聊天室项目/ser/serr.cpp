#include "head.h"
#include "contral.h"
using namespace std;

int create_socket(char *ip,unsigned short port)
{
    int sockfd=socket(AF_INET,SOCK_STREAM,0);
    assert(sockfd!=-1);

    struct sockaddr_in saddr;
    saddr.sin_family=AF_INET;
    saddr.sin_port=htons(port);
    saddr.sin_addr.s_addr=inet_addr(ip);
    int bin=bind(sockfd,(struct sockaddr*)&saddr,sizeof(saddr));
    assert(bin!=-1);
    listen(sockfd,5);
    return sockfd;
}
//client's call back funcation
void cli_cb(int fd,short event,void *arg)
{
    char buff[128]="";
    while(1)
    {
        if(0<recv(fd,buff,127,0))
        {
            cout<<"first buff"<<buff<<endl;
            contral temp;
            temp.sysdata(buff,fd);
            memset(buff,0,127);
        }
    }
}
    //sockfd's goback funation
void listen_cb(int fd,short event,void *arg)
{
    struct event_base *lib_base=(struct event_base*)arg;
    struct sockaddr_in caddr;
    socklen_t len=sizeof(caddr);
    int cli_fd=accept(fd,(struct sockaddr*)&caddr,&len);
    cout<<"cli_fd  "<<cli_fd<<endl;
    
    if(-1==cli_fd)
    {
        cout<<"accept error"<<endl;
        return;
    }
    
    struct event* cli_event=event_new(lib_base,cli_fd,EV_READ,cli_cb,lib_base);
    if(NULL==cli_event)
    {
       cout<<"cli event create fail:error" <<endl;
       return ;
    }
    event_add(cli_event,NULL);
}

void run(char *ip,unsigned short port)
{
    int sockfd=create_socket(ip,port);
    struct event_base *lib_base=event_base_new();
   
    struct event* listen_event=event_new(lib_base,sockfd,EV_READ,listen_cb,lib_base);
    
    if(NULL==listen_event)
    {
        cout<<"cli event create fail"<<endl;
    }
    
    event_add(listen_event,NULL);

    while(1)
    event_base_dispatch(lib_base);  //value is epoll_wait
} 
