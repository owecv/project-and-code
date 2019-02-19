#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

//opencv的头文件
#include <opencv2/opencv.hpp>
using namespace cv;

#include <iostream>
using namespace std;

#include <json/json.h>
using namespace Json;

int main(int argc,char **argv)
{
	cout<<"argc:"<<argc<<endl;
 	for(int i=0;i<argc;++i)
 	{
 		cout<<"argv[]"<<argv[i]<<endl;
 	}

    int sockfd=socket(AF_INET,SOCK_STREAM,0);
    assert(sockfd!=-1);
    struct sockaddr_in saddr,caddr;
    memset(&saddr,0,sizeof(saddr));
    saddr.sin_family=AF_INET;
	saddr.sin_port=htons(7000);//1024 知名端口，1025-4096，临时端口
 	saddr.sin_addr.s_addr=inet_addr(argv[1]);

	cout<<"success!0"<<endl;
    int res=bind(sockfd,(struct sockaddr*)&saddr,sizeof(saddr));//命名套接字
    assert(res!=-1);
    listen(sockfd,5);//创建监听队列
	cout<<"success!1"<<endl;

	int len=sizeof(caddr);
	int c=accept(sockfd,(struct sockaddr*)&caddr,(socklen_t*)&len);//    接收客户端链接
 	if(c<0)
	{   
 		cout<<"连接失败！"<<endl;
 	}
 	printf("accept c=%d,ip=%s,port:%d\n",c,inet_ntoa(caddr.sin_addr),ntohs(caddr.sin_port));

	char recvBuf[16];
 	char recvBuf_1[1];
	Mat img_decode;
	vector<uchar> data;

    while(1)
    {
		if(recv(c,recvBuf,16,0))
		{
			for(int i=0;i<16;i++)
			{
				if(recvBuf[i]<'0'||recvBuf[i]>'9')
				{
					recvBuf[i]=' ';
				}
			}
			data.resize(atoi(recvBuf));
            for (int i = 0; i < atoi(recvBuf); i++)
            {
                recv(c, recvBuf_1, 1, 0);
                data[i] = recvBuf_1[0];
            }
            send(c, "Server has recieved messages!", 29, 0);
            img_decode = imdecode(data, CV_LOAD_IMAGE_COLOR);
            imshow("server", img_decode);
            if (waitKey(30) == 27) break;		
		}
    }
}
