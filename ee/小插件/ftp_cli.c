#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <string>

#include <iostream>
using namespace std;

#define SER_PORT 8000
#define SER_IP "127.0.0.1"

// 判断文件或文件夹是否存在
//int access(const char *pathname, int mode);
/*
mode取值：
F_OK   测试文件是否存在
R_OK  测试读权限
W_OK 测试写权限
X_OK 测试执行权限
*/

int is_file_exist(const char *file_path)
{
	if(file_path==NULL)
		return -1;
	if(access(file_path,F_OK)==0)
		return 0;
	return -1;
}

//接收文件
void recv_file(int sockfd,char *name)
{
    char file_size[10]={0};
    if(recv(sockfd,file_size,10,0)<=0)
    {
        return;
    }
    
    int size=atoi(file_size);
    printf("待接收的文件大小=%d\n",size);
    
    cout<<"文件名为："<<name<<endl;
    
    /*
    此处实现文件的断点续传功能：
    １．首先根据输入的接收文件的文件名，判断本地是否存在一个该文件名对应的.temp文件。如果存在则说明这是一个断点续传文件，进行断点续传操作。否则进行正常的文件传输
    ２．给服务器发送控制命令：ａｃｋ表示正常下载文件，ｃａｎｃｅｌ表示进行文件的断点续传操作。
    */
    string file_path="./";
    file_path=file_path+name;
    string temp=".temp";
    file_path=file_path+temp;
    
    /*
    本地存在断点续传的缓存文件:
    １．首先打开本地的断点续传文件
    ２．计算断点位置
    ３．将断点位置反馈给服务器，服务器打开请求下载的文件，将文件指针移动到断点位置，继续传输文件直至文件传输完成
    */
    if(is_file_exist(file_path.c_str())==0)//本地存在断点续传的缓存文件
    {
        send(sockfd,"cancel",6,0);//给服务器发送断点续传的控制命令
        string file_name=name;
        string temp=".temp";
        file_name+=temp;
        temp.clear();
        int fd=open(file_name.c_str(),O_WRONLY);//以只写方式打开file_name.temp文件
        if(-1==fd)
        {
            printf("open temp file error!");
            exit(1);//如果代开文件失败，直接退出程序
        }
        else
        {
            cout<<"open temp file success!"<<endl;
        }

        int temp_size=lseek(fd,0,SEEK_END);//计算要下载文件的大小，将文件指针移动到文件末尾准备继续接收文件
        //lseek(fd,0,SEEK_SET);//
            
        if(temp_size==0)
        {
            cout<<"文件内容为0！"<<endl;
        }
        else
        {
            cout<<"size:"<<temp_size<<endl;
        }
        
        char file_size[10]={0};
        sprintf(file_size,"%d",temp_size);// 把1234 传到了buffer 而buffer为char *
        fflush(stdout);

        cout<<"temp file size:"<<temp_size<<endl;

        send(sockfd,file_size ,10,0);//通过文件指针计算出断点续传文件的断点位置，并发送给服务器
        
        char data_size[512]={0};//每次接收512字节的文件内容
    
        int num=0;//记录成功接收到的文件数据字节数
        int curr_size=temp_size;

        while((num=recv(sockfd,data_size,512,0))>0)
        {
            write(fd,data_size,num);
            curr_size+=num;

            float f=curr_size * 100/size;
            fflush(stdout); 
            printf("download:%2.f%%\r",f);
            if(curr_size==size)
            {
                break;
            }
        }
        printf("download successful!\n\n");
        
        //文件下载完成，将temp文件重命名为请求下载的文件
        string cmd="mv ";
        temp=file_name;
        cmd+=temp;
        temp.clear();
        temp=" ";
        cmd+=temp;
        temp.clear();
        temp=name;
        cmd+=temp;
        cout<<"cmd:"<<cmd<<endl;
        system(cmd.c_str());
    }
    //(is_file_exist(file_path.c_str())==-1)//本地不存在断点续传的缓存文件，则先创建缓存文件，等全部接收完毕，则将缓存文件重命名为待下载文件
    else
    {
        send(sockfd,"ack",3,0);//给服务器发送正常下载文件的控制命令
        string file_name=name;
        string temp=".temp";
        file_name+=temp;
        temp.clear();
        int fd=open(file_name.c_str(),O_WRONLY | O_CREAT,0600);
        if(fd==-1)
        {
            send(sockfd,"err",3,0);
            cout<<"在本地创建文件失败！"<<endl;
            return;
        }

        char data_size[512]={0};//每次接收512字节的文件内容
    
        int num=0;//记录成功接收到的文件数据字节数
        int curr_size=0;

        while((num=recv(sockfd,data_size,512,0))>0)
        {
            write(fd,data_size,num);
            curr_size+=num;

            float f=curr_size * 100/size;
            fflush(stdout); 
            printf("download:%2.f%%\r",f);
            if(curr_size==size)
            {
                break;
            }
        }
        printf("download successful!\n\n");
        
        //文件下载完成，将temp文件重命名为请求下载的文件
        string cmd="mv ";
        temp=file_name;
        cmd+=temp;
        temp.clear();
        temp=" ";
        cmd+=temp;
        temp.clear();
        temp=name;
        cmd+=temp;
        cout<<"cmd:"<<cmd<<endl;
        system(cmd.c_str());
    }
}

int main()
{
    
    int sockfd=socket(AF_INET,SOCK_STREAM,0);
    if(sockfd==-1)
    {
        return -1;
    }
    struct sockaddr_in saddr;
    memset(&saddr,0,sizeof(saddr));

    saddr.sin_family=AF_INET;
    saddr.sin_port=htons(SER_PORT);
    saddr.sin_addr.s_addr=inet_addr(SER_IP);
    
    int res=connect(sockfd,(struct sockaddr *)&saddr,sizeof(saddr));
    assert(res!=-1);

    while(1)
    {
        char buff[128]={0};
        setbuf(stdin,NULL);//清空输入缓冲区
        printf("connet sccess>> ");
        fflush(stdout);
        fgets(buff,128,stdin);//从键盘读取命令
        buff[strlen(buff)-1]=0;
        
        if(strncmp(buff,"end",3)==0)
        {
            break;
        }
        
        char cmd[128]={0};//存命令
        strcpy(cmd,buff);
        
        char* ptr=NULL;//strtok_r会用到的一个参数
        char* s=strtok_r(cmd," ",&ptr);

        if(s==NULL)
        {
            continue;
        }
        if(strcmp(s,"exit")==0)
        {
            break;
            exit(0);
        }
        if(strcmp(s,"ls")==0)
        {
            send(sockfd,buff,strlen(buff),0);
            cout<<"buff:"<<buff<<endl;

            char recvbuff[1024]={0};
            if(recv(sockfd,recvbuff,1023,0)<0)
            {
                continue;
            }
            else//打印出服务器端的ls结果
            {
                printf("%s\n",recvbuff);
            }
        }
        if(strcmp(s,"get")==0)
        {
            send(sockfd,buff,strlen(buff),0);
            
            cmd[128]={0};//存命令
            strcpy(cmd,buff);
            ptr=NULL;//strtok_r会用到的一个参数
            
            s=strtok_r(cmd," ",&ptr);
            if(s==NULL)
            {
                continue;
            }
            s=strtok_r(NULL," ",&ptr);//分割出文件名
            recv_file(sockfd,s);//接收文件
        }
        if(strcmp(s,"put")==0)//上传文件，实现文件的秒传功能
        {
            send(sockfd,buff,strlen(buff),0);
            
            cmd[128]={0};//存命令
            strcpy(cmd,buff);
            ptr=NULL;//strtok_r会用到的一个参数
            
            s=strtok_r(cmd," ",&ptr);//s为命令
            if(s==NULL)
            {
                continue;
            }
            s=strtok_r(NULL," ",&ptr);//分割出文件名
            
            /*
            此处实现文件的秒传功能：
            1.计算出该文件名（s）对应文件的MD5值，将计算结果重定向保存在file.md5文件中
            2.将file.md5文件中MD5值读取出来保存在char buffer[37]中
            3.将buffer中的内容发送给服务器
            4.服务器将buffer中的文件名和MD5值分割出来
            4.服务器对该文件的MD5值进行对比，如果发现该文件不存在，则正常上传该文件，并将该MD5值存入数据，否则发现该文件在服务器上已存在，则直接进行文件的硬链接，实现秒传
            */
            
            //计算文件的MD5值
            string cmd="md5sum ";
            string temp=s;
            cmd=cmd+temp;
            temp.clear();
            temp=" > file.md5";
            cmd=cmd+temp;
            temp.clear();
            
            cout<<"cmd:"<<cmd<<endl;
            
            system(cmd.c_str());
	
	        int file_md5_fd=open("./file.md5",O_RDONLY);//以只读方式打开file.md5文件
            if(file_md5_fd==-1)
            {
                printf("open file.md5 error！\n");
                return 1;
            }
            else
            {
                cout<<"open file.md5 success!"<<endl;
            }
            
            int file_md5_size=lseek(file_md5_fd,0,SEEK_END);//计算需要上传的文件的大小
            if(file_md5_size==0)
            {
                cout<<"size=0"<<endl;
                return 1;
            }
            else
            {
    	        lseek(file_md5_fd,0,SEEK_SET);//将文件指针回零
                cout<<"要计算MD5值的文件大小为："<<file_md5_size<<endl;
            }
	
	        char buffer[128]="";
	        read(file_md5_fd, buffer, file_md5_size);
            close(file_md5_fd);
            cout<<"md5sum:"<<buffer<<endl;
            
            send(sockfd,buffer,128,0);//将MD5计算结果发送给服务器
            
            /*
            此处对从服务器端反馈的结果进行判断，确定是否需要上传文件？
            服务器返回ack表示服务器端不存在此文件，需要上传
            返回cancel表示服务器已存在此文件，不需要再次上传
            */
            
            char recvbuff[20]={0};//记录服务器的反馈结果
            if(recv(sockfd,recvbuff,19,0)<0)
            {
                cout<<"未收到服务器对待上传的文件是否需要上传确认信息！"<<endl;
            }
            else//打印出服务器端的ls结果
            {
                cout<<"已收到服务器对待上传的文件是否需要上传确认信息！"<<endl;
                printf("%s\n",recvbuff);
            }
            
            if(strncmp(recvbuff,"ack",3)==0)//需要上传文件
            {
                int fd=open(s,O_RDONLY);//以只读方式打开需要上传的文件
                if(fd==-1)
                {
                    printf("open file error！\n");
                    continue;
                }
                else
                {
                    cout<<"open file success!"<<endl;
                }
            
                int size=lseek(fd,0,SEEK_END);//计算需要上传的文件的大小
                if(size==0)
                {
                    cout<<"size=0"<<endl;
                    continue;
                }
                else
                {
                    lseek(fd,0,SEEK_SET);//将文件指针回零
                    cout<<"要上传的文件大小为："<<size<<endl;
                }
            
                char file_size[10]={0};
                sprintf(file_size,"%d",size);// 把1234 传到了buffer 而buffer为char *
                cout<<"file_size:"<<file_size<<endl;
                fflush(stdout);

                send(sockfd,file_size ,10,0);
            
                char recvbuff[20]={0};//记录服务器的反馈结果
                if(recv(sockfd,recvbuff,19,0)<0)
                {
                    cout<<"未接收到服务器对要上传的文件大小信息的确认信息！"<<endl;
                }
                else//打印出服务器端的ls结果
                {
                    cout<<"已收到服务器对要上传的文件大小信息的确认信息！"<<endl;
                    printf("%s\n",recvbuff);
                }

                sendfile(sockfd,fd,NULL,size);//系统提供的函数
            }
            if(strncmp(recvbuff,"cancel",6)==0)//不需要上传文件
            {
                cout<<"服务器已存在此文件！执行秒传操作..."<<endl;
            }
            
        }
        if(strcmp(s,"pwd")==0)
        {
            send(sockfd,buff,strlen(buff),0);
            cout<<"buff:"<<buff<<endl;

            char recvbuff[1024]={0};
            if(recv(sockfd,recvbuff,1023,0)<0)
            {
                continue;
            }
            else//打印出服务器端的ls结果
            {
                printf("%s\n",recvbuff);
            }
        }
        if(strcmp(s,"lsl")==0)
        {
            printf("*************本地文件可以操作的文件*********\n");
            system("ls");
            printf("********************************************\n");
        }
        //打印目前所处的路径（客户端）
        if(strcmp(s,"pwdl")==0)
        {
            char cmdl[64]="pwd";
            printf("本地路径为：");
            fflush(stdout);
            system(cmdl);
        }
    }
    close(sockfd);
    return 0;
}
