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
#include <time.h>

//json头文件
#include <iostream>
#include <json/json.h>
using namespace std;
using namespace Json;

//fork头文件
#include <sys/types.h>
#include <sys/wait.h>

#define IP "127.0.0.1"
#define PORT 6000
#define STDIN 0

//枚举出所有的操作
enum _TYPE
{
    TYPE_LOGIN,     //登陆                          0
    TYPE_REG,       //注册                          1
    TYPE_EXIT,      //退出                          2
    TYPE_GOAway,    //下线                          3
    TYPE_LIST,      //列出所有在线的用户            4
    TYPE_ONE,       //一对一聊天                    5
    TYPE_GROUP,     //群聊                          6

    TYPE_MESSAGE,   //服务器需要转发的信息类JSON包  7
    TYPE_tFILE,     //客户端请求文件传输服务        8
	TYPE_VIDEO		//客户端请求进行视频聊天		9
}TYPE;

char name_self[20]="";//保存该客户端上已登录用户的名字

void Login(int fd);//客户端请求进行登录操作
void Register(int fd);//客户端请求进行注册操作
void Exit(int fd);//客户端请求进行退出操作
void GetUserOnline(int fd);//客户端请求获取在线用户列表
void ChatToOne(int fd);//客户端请求进行一对一聊天操作
void ChatToGroup(int fd);//客户端请求进行群聊
void GoAway(int fd);//客户端请求进行下线操作
void FileTransmit(int fd);//客户端请求进行文件传输操作
void VideoChat(int fd);//客户端请求进行视频聊天

void cli_process(int fd);//当客户端和服务器建立上连接后，调用此函数进行处理

void Login_success(int fd);//当服务器端反馈登录成功时，调用此函数进行处理

int main()
{
    int sockfd=socket(AF_INET,SOCK_STREAM,0);//调用socket函数创建套接字
    assert(sockfd!=-1);

    struct sockaddr_in saddr;
    memset(&saddr,0,sizeof(saddr));
    saddr.sin_family=AF_INET;
    saddr.sin_port=htons(PORT);
    saddr.sin_addr.s_addr=inet_addr(IP);

    int res=connect(sockfd,(struct sockaddr*)&saddr,sizeof(saddr));
    assert(res!=-1);

    cout<<"连接服务器成功！"<<endl;

    cli_process(sockfd);//当服务器端反馈登录成功时，调用此函数进行处理

    //关闭连接
    close(sockfd);
    return 0;
}


void cli_process(int fd)//当服务器端反馈登录成功时，调用此函数进行处理
{
    cout<<"请选择要进行的操作："<<endl;
    cout<<"1.登录\n2.注册\n3.退出\n"<<endl;
    int choice;
    cin>>choice;
    switch(choice)
    {
        case 1:
        {
            Login(fd);//客户端请求进行登录操作
        }break;
        case 2:
        {
            Register(fd);//客户端请求进行注册操作
        }break;
        case 3:
        {
            Exit(fd);//客户端请求进行退出操作
        }break;
        default:
        {
            cout<<"您的输入有误！请重新选择："<<endl;
            cli_process(fd);
        }
    }
}

void Login(int fd)//客户端请求进行登录操作
{
    cout<<"请输入用户名：";
    cin>>name_self;

    char pw[20]="";
    cout<<"请输入密码：";
    cin>>pw;

    //制作请求登录的Json包
    TYPE=TYPE_LOGIN;
    Json::Value val;
    val["type"]=TYPE;
    val["name"]=name_self;
    val["passwd"]=pw;

    //cout<<"登录时发送给服务器的json包："<<val<<endl;
    if(-1==send(fd,val.toStyledString().c_str(),strlen(val.toStyledString().c_str()),0))
    {
        cout<<"给服务器发送登录请求的JSON包失败！"<<endl;
        return ;
    }
    else
    {
        cout<<"给服务器发送登录请求的JSON包成功！"<<endl;
    }

    //对从服务器返回的数据进行处理
    //返回“OK”表示存在此用户，登录成功
    char recvbuff[10]="";
    
    //接收到的数据有误
    if(recv(fd,recvbuff,9,0)<=0)
    {
        cout<<"server unlink or error!"<<endl;
    }

    cout<<"登录时，服务器端反馈的结果为："<<endl<<recvbuff<<endl;
    fflush(stdout);


    //对返回的数据进行判断
    if(strncmp(recvbuff,"OK",2)==0)
    {
        cout<<"Login success!"<<endl;
        //登录成功，调用处理登录成功的函数的进行操作
        Login_success(fd);
    }
    if(strncmp(recvbuff,"ok",2)==0)
    {
        cout<<"Login failed!"<<endl;
    }
}

void Register(int fd)//客户端注册操作
{
    char name[20]="";
    cout<<"请输入注册用户的名字：";

    cin>>name;
    char pw[20]="";
    cout<<"请输入注册用户的密码：";
    cin>>pw;

    //制作请求注册的Json包
    TYPE=TYPE_REG;//注册
    Json::Value val;
    val["type"]=TYPE;
    val["name"]=name;
    val["passwd"]=pw;

    if(-1==send(fd,val.toStyledString().c_str(),strlen(val.toStyledString().c_str()),0))
    {
        cout<<"发送注册新用户的Json包失败！"<<endl;
        return ;
    }
    else
    {
        cout<<"客户端给服务器发送注册请求的JSON包成功！"<<endl;
    }

    //对从服务器返回的数据进行处理
    //返回“OK”表示注册新用户成功
    char recvbuff[10]="";

    //接收到的数据有误
    if(recv(fd,recvbuff,9,0)<=0)
    {
        cout<<"server unlink or error!"<<endl;
    }

    cout<<"注册时，服务器端反馈的结果为："<<endl<<recvbuff<<endl;
    fflush(stdout);

    //对返回的数据进行判断
    if(strncmp(recvbuff,"OK",2)==0)
    {
        cout<<"Register success!"<<endl;
        cli_process(fd);
    }
    if(strncmp(recvbuff,"ok",2)==0)
    {
        cout<<"Register failed!"<<endl;
    }
}

void Exit(int fd)//客户端退出操作
{
    Json::Value val;
    TYPE=TYPE_EXIT;//用户下线
    val["type"]=TYPE;
    if(-1==send(fd,val.toStyledString().c_str(),strlen(val.toStyledString().c_str()),0))
    {
        cout<<"客户端发送退出请求的JSON包失败！"<<endl;
        return;
    }
    else
    {
        cout<<"客户端发送退出请求的JSON包成功！";
    }

    //对从服务器返回的确认信息进行处理
    //返回“OK”表示该用户下线成功
    char recvbuff[10]="";

    //接收到的数据有误
    if(recv(fd,recvbuff,9,0)<=0)
    {
        cout<<"server unlink or error!"<<endl;
    }

    cout<<"退出操作时，服务器端反馈的结果为："<<endl<<recvbuff<<endl;
    fflush(stdout);

    //对返回的数据进行判断
    if(strncmp(recvbuff,"OK",2)==0)
    {
        cout<<"EXIT success!"<<endl;
    }
    if(strncmp(recvbuff,"ok",2)==0)
    {
        cout<<"EXIT failed!"<<endl;
    }
    exit(0);
}

void GetUserOnline(int fd)//客户端请求获取在线用户列表
{
    TYPE=TYPE_LIST;
    cout<<"TYPE:"<<TYPE<<endl;

    Json::Value val;
    val["type"]=TYPE;

    send(fd,val.toStyledString().c_str(),strlen(val.toStyledString().c_str()),0);

    cout<<"等待接收服务器数据..."<<endl;
    char buff[1024*1024]="";
    if(recv(fd,buff,1024*1024,0)<=0)
    {
        cout<<"error!"<<endl;
    }
    else
    {
        printf("客户端收到的数据为：%s\n",buff);
        fflush(stdout);
    }

    cout<<"在线的用户有："<<endl;
    int i=0;
    while(buff[i]!='\0')
    {
        if(buff[i]=='$')
        {
            cout<<"\t";
        }
        else if(buff[i]=='#')
        {
            cout<<endl;
        }
        else
        {
            cout<<buff[i];
        }
        ++i;
    }
}

void GoAway(int fd)//客户端请求进行下线并退出的操作
{
    Json::Value val;
    TYPE=TYPE_GOAway;//用户下线
    val["type"]=TYPE;
    val["name"]=name_self;
    if(-1==send(fd,val.toStyledString().c_str(),strlen(val.toStyledString().c_str()),0))
    {
        cout<<"客户端发送“下线并退出”请求的JSON包失败！"<<endl;
        return;
    }
    else
    {
        cout<<"客户端发送“下线并退出”请求的JSON包成功！"<<endl;
    }

    //对从服务器返回的确认信息进行处理
    //返回“OK”表示该用户下线成功
    char recvbuff[10]="";

    //接收到的数据有误
    if(recv(fd,recvbuff,9,0)<=0)
    {
        cout<<"server unlink or error!"<<endl;
    }

    cout<<"下线操作时，服务器端反馈的结果为："<<endl<<recvbuff<<endl;
    fflush(stdout);

    //对返回的数据进行判断
    if(strncmp(recvbuff,"OK",2)==0)
    {
        cout<<"GoAway success!"<<endl;
        exit(0);
    }
    if(strncmp(recvbuff,"ok",2)==0)
    {
        cout<<"GoAway failed!"<<endl;
    }
}

void ChatToOne(int fd)//处理客户端“一对一”聊天请求
{
    cout<<"一对一聊天："<<endl;
    cout<<"请输入对方的姓名：";
    char B_name[20]="";
    cin>>B_name;

    //制作请求“一对一聊天”的Json包
    TYPE=TYPE_ONE;//一对一聊天
    Json::Value val;
    val["type"]=TYPE;
    val["A_name"]=name_self;//自己的名字
    val["B_name"]=B_name;//对方的姓名

    if(-1==send(fd,val.toStyledString().c_str(),strlen(val.toStyledString().c_str()),0))
    {
        cout<<"客户端发送“一对一聊天”请求的JSON包失败！"<<endl;
        return ;
    }
    else
    {
        cout<<"客户端发送“一对一聊天”请求的JSON包成功！"<<endl;
    }

    //对从服务器返回的数据进行处理
    //返回“OK”表示存在此用户且该用户在线，服务器准备成功
    char recvbuff[1024]="";

    //接收到的数据有误
    if(recv(fd,recvbuff,1023,0)<=0)
    {
        cout<<"error!"<<endl;
    }

    cout<<"一对一时，服务器端反馈的结果为："<<endl<<recvbuff<<endl;
    fflush(stdout);

    //对收到的服务器“一对一聊天”JSON反馈包进行解析
    Json::Value val_recv;
    Json::Reader read_recv;

    //Json包解析失败
    if(-1==read_recv.parse(recvbuff,val_recv))
    {
        cout<<"Json parse fail!"<<endl;
        return ;
    }

    //对返回的数据进行判断
    char ink[10]="";
    strcpy(ink,val_recv["OK"].toStyledString().c_str());

    if(ink[1]=='O'&&ink[2]=='K')
    {
        cout<<"“一对一聊天”请求成功！"<<endl;
        cout<<"Connect to "<<B_name<<" success!"<<endl;
        //连接成功
        cout<<"send to "<<B_name<<":";
        char send_message[1024]="";
		setbuf(stdin,NULL);//清空输入缓冲区
		fgets(send_message,1024,stdin);
       
        //制作信息类Json包，专门用于传递聊天的消息内容
        TYPE=TYPE_MESSAGE;//信息类JSON包
        Json::Value val_message;
        val_message["type"]=TYPE;
        val_message["A_name"]=val_recv["A_name"];//A（自己）的名字
        val_message["A_fd"]=val_recv["A_fd"];//A的fd
        val_message["B_name"]=val_recv["B_name"];//B(对方)的名字
        val_message["B_fd"]=val_recv["B_fd"];//B的fd
        val_message["message"]=send_message;//消息内容

        if(-1==send(fd,val_message.toStyledString().c_str(),strlen(val_message.toStyledString().c_str()),0))
        {
            cout<<"发送消息失败！"<<endl;
            return ;
        }
        else
        {
            cout<<"该消息已成功发送至服务器！"<<endl;
        }
    }
    if(ink[1]=='o'&&ink[2]=='k')
    {
        cout<<"该用户不在线！"<<endl;
        cout<<"Connect to “"<<B_name<<"” failed!"<<endl;
    }
}

void ChatToGroup(int fd)//客户端请求进行群聊
{
    cout<<"发起群聊，给所有在线用户发送消息..."<<endl;
    cout<<"请输入要群发的消息：";
    char message_buff[1024]="";
    cin>>message_buff;

    //制作请求“群聊”的Json包
    TYPE=TYPE_GROUP;//一对一聊天
    Json::Value val;
    val["type"]=TYPE;//JSON包的类型
    val["A_name"]=name_self;//自己的名字
    val["message"]=message_buff;//群发的消息
   
    if(-1==send(fd,val.toStyledString().c_str(),strlen(val.toStyledString().c_str()),0))
    {
        cout<<"客户端发送“群聊”的JSON包失败！"<<endl;
        return ;
    }
    else
    {
        cout<<"客户端发送“群聊”的JSON包成功！"<<endl;
    }
}

void FileTransmit(int fd)//客户端请求进行文件传输操作
{
    TYPE=TYPE_tFILE;
    cout<<"TYPE:"<<TYPE<<endl;

    Json::Value val;
    val["type"]=TYPE;

    if(-1==send(fd,val.toStyledString().c_str(),strlen(val.toStyledString().c_str()),0))
    {
        cout<<"客户端发送请求“文件传输服务”的JSON包失败！"<<endl;
        return ;
    }
    else
    {
        cout<<"客户端发送请求“文件传输服务”的JSON包成功！"<<endl;
    }

    //对从服务器返回的确认信息进行处理
    //返回“OK”表示服务器准备就绪，ftp_ser已启动
    char recvbuff[10]="";

    //接收到的数据有误
    if(recv(fd,recvbuff,9,0)<=0)
    {
        cout<<"server unlink or error!"<<endl;
    }

    cout<<"请求文件传输服务时，服务器端反馈的结果为："<<endl<<recvbuff<<endl;
    fflush(stdout);

    //对返回的数据进行判断
    if(strncmp(recvbuff,"OK",2)==0)
    {
        cout<<"“文件传输服务”请求成功，ftp_ser已启动！"<<endl;
            
        int i;
        int status;
        pid_t pid;
        pid=fork();
        printf("pid:%d\n",pid);
        if(pid==-1)
        {
            printf("复制进程出错！\n");
        }
        if(pid>0)//父进程
        {
            waitpid(pid,&status,0);
        }
        if(pid==0)
        {
            printf("正在启动ftp_cli程序...\n");
            execl("/home/wangpeng/代码/qq/小插件/ftp_cli","ftp_cli",NULL,NULL);
        }
    }
    if(strncmp(recvbuff,"ok",2)==0)
    {
        cout<<"“文件传输服务”请求失败！"<<endl;
    }
}

void VideoChat(int fd)//客户端请求进行视频聊天
{
	cout<<"正在准备视频聊天..."<<endl;
	cout<<"请输入对方的名字：";
	char B_name[20]="";
	cin>>B_name;

	//制作请求“视频聊天”的Json包
	TYPE=TYPE_VIDEO;//一对一聊天
	Json::Value val;
	val["type"]=TYPE;//JSON包的类型
	val["A_name"]=name_self;//自己的名字
	val["B_name"]=B_name;//对方的姓名

    if(-1==send(fd,val.toStyledString().c_str(),strlen(val.toStyledString().c_str()),0))
    {
        cout<<"客户端发送“视频聊天”请求的JSON包失败！"<<endl;
        return ;
    }
    else
    {
        cout<<"客户端发送“视频聊天”请求的JSON包成功！"<<endl;
    }

	//对从服务器返回的数据进行处理
    //返回“OK”表示存在此用户且该用户在线，服务器准备成功
    char recvbuff[1024]="";

    //接收到的数据有误
    if(recv(fd,recvbuff,1023,0)<=0)
    {
        cout<<"error!"<<endl;
    }

    cout<<"视频聊天时，服务器端反馈的结果为："<<endl<<recvbuff<<endl;
    fflush(stdout);

    //对收到的服务器“一对一聊天”JSON反馈包进行解析
    Json::Value val_recv;
    Json::Reader read_recv;

    //Json包解析失败
    if(-1==read_recv.parse(recvbuff,val_recv))
    {
        cout<<"Json parse fail!"<<endl;
        return ;
    }
	
	//对返回的数据进行判断
	char ink[10]="";
	strcpy(ink,val_recv["OK"].toStyledString().c_str());

	if(ink[1]=='O'&&ink[2]=='K')
	{
		cout<<"“一对一聊天”请求成功！"<<endl;

		int i;
    	int status;
    	pid_t pid;
    	pid=fork();
   		printf("pid:%d\n",pid);
    	if(pid==-1)
    	{
    		printf("复制进程出错！\n");
    	}
    	if(pid>0)//父进程
    	{
        	waitpid(pid,&status,0);
    	}
    	if(pid==0)
    	{
			cout<<"正在启动“video_cli程序“准备发送自己的视频！"<<endl;
			cout<<"参数:"<<val_recv["B_IP"].toStyledString().c_str()<<endl;
			char p[17]="";
			strcpy(p,val_recv["B_IP"].toStyledString().c_str());
			cout<<"p:"<<p<<endl;
			cout<<"p:";
			for(int i=0;i<17;++i)
			{
				cout<<p[i];
			}
			cout<<endl;

			char p0[16]={0};
			int j=1;
			for(int i=0;i<16;++i)
			{
				p0[i]=p[j];
				++j;
			}
			p0[16]='\0';
			cout<<"p0[10]="<<p0<<endl;
			execl("/home/wangpeng/代码/qq/小插件/video_cli","./video_cli","127.0.0.1",NULL);
    	}
	}
	if(ink[1]=='o'&&ink[2]=='k')
	{
		cout<<"视频聊天请求失败！"<<endl;
	}
}

void Login_success(int fd)//当服务器端反馈登录成功时，调用此函数进行处理
{
    int stdin_fd=STDIN;
    fd_set fdset;

    struct timeval timeout={5,0};

    cout<<"请选择要进行的操作："<<endl;
    cout<<"1：获取在线用户列表"<<endl;
    cout<<"2：一对一聊天"<<endl;
    cout<<"3：群聊"<<endl;
    cout<<"4:文件传输请求"<<endl;
	cout<<"5:视频聊天请求"<<endl;
    cout<<"6：下线并退出"<<endl;

    int  choice;

    while(1)
    {
        choice=0;

        FD_ZERO(&fdset);
        FD_SET(fd,&fdset);//将描述符添加进来
        FD_SET(stdin_fd,&fdset);//将键盘事件添加到监听队列中

        int n=select(fd+2,&fdset,NULL,NULL,&timeout);
        if(n==-1)
        {
            cout<<"selection failure"<<endl;
        }
        else if(n==0)
        {
        
        }
        if(FD_ISSET(stdin_fd,&fdset))
        {
            cin>>choice;
            cout<<"请选择要进行的操作："<<endl;
            cout<<"1：获取在线用户列表"<<endl;
            cout<<"2：一对一聊天"<<endl;
            cout<<"3：群聊"<<endl;
            cout<<"4:文件传输请求"<<endl;
			cout<<"5:视频聊天请求"<<endl;
            cout<<"6：下线并退出"<<endl;

            switch(choice)
            {
                case 1://获取在线用户列表
                {
                    GetUserOnline(fd);
                }break;
                case 2://一对一聊天
                {
                    ChatToOne(fd);
                }break;
                case 3://群聊
                {
                    ChatToGroup(fd);
                }break;
                case 4://文件传输请求
                {
                    FileTransmit(fd);//客户端请求进行文件传输操作
                }break;
				case 5://视频聊天请求
				{
					VideoChat(fd);//客户端请求进行视频聊天
				}break;
                case 6://下线并退出
                {
                    GoAway(fd);
                    exit(0);
                }break;
            }
        }
        if(FD_ISSET(fd,&fdset))
        {
            //接收服务器发送过来的消息
            char recv_message[1024]="";

            //接收消息有误
            if(recv(fd,recv_message,1023,0)<=0)
            {
                cout<<"error!"<<endl;
            }
            else
            {
                cout<<"收到的消息为："<<endl<<recv_message<<endl;

                //对接收到的数据进行解析
                Json::Value val;
                Json::Reader read;
                
                //Json包解析失败
                if(-1==read.parse(recv_message,val))
                {
                    cout<<"Json parse fail!"<<endl;
                    return ;
                }

                //判断判断收到的JSON包的类型，进行相应的处理
                if(val["type"]==6||val["type"]==7)//消息类JSON
                {
                    cout<<"Message from user:"<<val["A_name"].toStyledString().c_str()<<val["message"].toStyledString().c_str()<<endl;
                }

				if(val["type"]==9)//视频聊天类的JSON包
				{
					cout<<val["A_name"].toStyledString().c_str()<<"正在请求“视频聊天”！"<<endl;
					 
					int i;
        			int status;
        			pid_t pid;
        			pid=fork();
        			printf("pid:%d\n",pid);
        			if(pid==-1)
        			{
            			printf("复制进程出错！\n");
        			}
        			if(pid>0)//父进程
        			{
            			waitpid(pid,&status,0);
        			}
        			if(pid==0)
        			{
						cout<<"正在启动“video_server程序“准备接受对方的视频！"<<endl;
						cout<<"参数:"<<val["B_IP"].toStyledString().c_str()<<endl;
						char p[17]="";
						   
						strcpy(p,val["B_IP"].toStyledString().c_str());
						cout<<"p:"<<p<<endl;
						cout<<"p:";
						for(int i=0;i<17;++i)
						{
							cout<<p[i];
						}
						cout<<endl;

						char p0[16]="";
						int j=1;
						for(int i=0;i<15;++i)
						{   
							p0[i]=p[j];
							++j;
						}
						p0[16]='\0';
						cout<<"p0[16]="<<p0<<endl;

            			execl("/home/wangpeng/代码/qq/小插件/video_ser","./video_ser",p0,NULL);
        			}
				}
            }
        }
    }
}
