#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <errno.h>//打印错误信息的头文件
#include <sys/types.h>
#include <fcntl.h>
#include <string>

#include <opencv2/opencv.hpp>
#include <iostream>
using namespace std;
using namespace cv;

#include <json/json.h>
using namespace Json;

int main(int argc,char **argv)
{
	cout<<"argc:"<<argc<<endl;
  	for(int i=0;i<argc;++i)
  	{
  		cout<<"argv[]"<<argv[i]<<endl;
 	}

    int sockfd=socket(AF_INET,SOCK_STREAM,0);//调用socket函数创建套接字
    assert(sockfd!=-1);

    struct sockaddr_in saddr;
    memset(&saddr,0,sizeof(saddr));
    saddr.sin_family=AF_INET;
    
	saddr.sin_port=htons(7000);
 	saddr.sin_addr.s_addr=inet_addr(argv[1]);
	cout<<"success!0"<<endl;

    int res=connect(sockfd,(struct sockaddr*)&saddr,sizeof(saddr));
    assert(res!=-1);
	cout<<"success!1"<<endl;

	//打开摄像头
	//从摄像头读入视频
	Mat image;
    VideoCapture capture(0);
    vector<uchar> data_encode;

	int stdin_fd=0;
    fd_set fdset;

    struct timeval timeout={5,0};

    while(1)
    {
        //发送数据
		//循环显示每一帧
    	if (!capture.read(image)) 
            break;

		imshow("cli", image);
		if (waitKey(30) == 27) break;

        imencode(".jpg", image, data_encode);
        int len_encode = data_encode.size();
        string len = to_string(len_encode);
        int length = len.length();
        for (int i = 0; i < 16 - length; i++)
        {
            len = len + " ";
        }
        //发送数据
        send(sockfd, len.c_str(), strlen(len.c_str()), 0);
        char send_char[1];
        for (int i = 0; i < len_encode; i++)
        {
            send_char[0] = data_encode[i];
            send(sockfd, send_char, 1, 0);
        }

		FD_ZERO(&fdset);
        FD_SET(sockfd,&fdset);//循环添加所有描述符
        FD_SET(stdin_fd,&fdset);//将键盘也添加进来

		int n=select(sockfd+2,&fdset,NULL,NULL,&timeout);
        if(n==-1)
        {
        	printf("selection failure\n");
            break;
        }
        else if(n==0)
        {
        	//printf("time out\n");
        }
        if(FD_ISSET(sockfd,&fdset))
        {
			//接收返回信息
			char recvBuf[32];
			if(recv(sockfd, recvBuf, 32, 0));
       	}
 
        if(FD_ISSET(stdin_fd,&fdset))
        {
            char buff[20]={0};
            fgets(buff,20,stdin);
			if(strncmp(buff,"exit",4)==0)
        	{
            	break;
            	exit(0);
        	}
        }
	}
    close(sockfd);
	return 0;
}
