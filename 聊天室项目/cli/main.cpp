#include "cli.h"
#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>

const int PORT=6000;
const char *IP="127.0.0.1";

int main()
{
    int sockfd=socket(AF_INET,SOCK_STREAM,0);
    assert(sockfd!=-1);

    struct sockaddr_in saddr;
    saddr.sin_family=AF_INET;
    saddr.sin_port=htons(PORT);
    saddr.sin_addr.s_addr=inet_addr(IP);
    
    int con=connect(sockfd,(struct sockaddr*)&saddr,sizeof(saddr));
    assert(con!=-1);
    cout<<sockfd<<endl;

    while(1)
    {
        run(sockfd);
    }

    return 0;
}
