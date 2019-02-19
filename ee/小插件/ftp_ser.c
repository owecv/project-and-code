#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>//linux系统调用有关的头文件
#include <string.h>
#include <assert.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <fcntl.h>//文件操作有关的头文件
#include <sys/wait.h>
#include <string>
#include <iostream>
#include <sys/sendfile.h>
#include <unistd.h>

//MySQL头文件
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <mysql/mysql.h> //包含MySQL所需要的头文件
#include <iostream>
#include <typeinfo>
#include <string.h>

using namespace std;

//MySQL
using namespace std;
#pragma comment(lib, "libmysql.lib")

#define SER_PORT 8000
#define SER_IP "127.0.0.1"

int socket_create();
void *thread_fun(void *arg);

int main()
{
    cout<<"ftp_ser启动成功！"<<endl;
    chdir("/home/wangpeng/代码/qq/文件传输服务");
    cout<<"更改ftp_ser运行目录成功！"<<endl;
    int sockfd=socket_create();
    struct sockaddr_in caddr;
    while(1)
    {
        int len=sizeof(caddr);
        int c=accept(sockfd,(struct sockaddr*)&caddr,(socklen_t*)&len);
        if(c<0)
        {
            continue;
        }

        printf("accept c=%d,ip=%s,port=%d\n",c,inet_ntoa(caddr.sin_addr),ntohs(caddr.sin_port));
        pthread_t id;
        pthread_create(&id,NULL,thread_fun,(void *)&c);
    }
}

void *thread_fun(void *arg)
{
    int c=*(int*)arg;
    
    while(1)
    {
        char buff[128]={0};
        buff[127]=0;

        int n=recv(c,buff,127,0);//n表示从buff中读取了几个数据
        if(n<=0)
        {
            printf("one client over\n");
            break;//当一个客户端连接断开时，n<=0，这个时候就没有必要一直循环等待接收客户端的消息了，直接break跳出while循环
        }
        else
        {
            printf("服务器端收到的信息：%s\n",buff);
            fflush(stdout);
        }
        
        char* ptr=NULL;//strtok_r会用到的一个参数
        char* s=strtok_r(buff," ",&ptr);
        
        if(NULL==s)//没有分割出命令
        {
            continue;
        }
        
        //将命令和参数分割出来
        char* myargv[10]={0};//这个数组里存了命令和参数
        myargv[0]=s;
       
        int i=1;
        while((s=strtok_r(NULL," ",&ptr))>0)
        {
            myargv[i++]=s;
        }

        if(strcmp(myargv[0],"ls")==0)
        {
            int pipefd[2];
            pipe(pipefd);//创建管道
        
            pid_t pid=fork();//先创建管道，再进行fork

            if(pid==-1)
            {
                printf("复制进程出错！\n");
            }
            if(pid==0)//0为子进程
            {
                dup2(pipefd[1],1);//1为读端
                dup2(pipefd[1],2);//2为写端
            
                printf("**************可以下载的文件有**************\n");
                execvp(myargv[0],myargv);
                perror("execvp error");
                exit(0);
            }
            
            close(pipefd[1]);
            wait(NULL);
            char readbuff[1024]={0};
            int pipe_num=read(pipefd[0],readbuff,1023);

            send(c,readbuff,pipe_num,0);
            close(pipefd[0]);
        }
        if(strcmp(myargv[0],"rm")==0)
        {
            char cmdl[64]="rm ";
            
            strcat(cmdl,myargv[1]);
            printf("cmdl=%s\n",cmdl);
            system(cmdl);
        }
        if(strcmp(myargv[0],"get")==0)
        {
            int fd=open(myargv[1],O_RDONLY);
            if(-1==fd)
            {
                printf("open file error!");
                exit(1);//如果代开文件失败，直接退出程序
            }
            else
            {
                cout<<"open file success!"<<endl;
            }

            int size=lseek(fd,0,SEEK_END);//计算要下载文件的大小
            lseek(fd,0,SEEK_SET);
            
            if(size==0)
            {
                cout<<"文件内容为0！"<<endl;
                continue;
            }
            else
            {
                cout<<"size:"<<size<<endl;
            }

            char file_size[10]={0};
            sprintf(file_size,"%d",size);// 把1234 传到了buffer 而buffer为char *
            fflush(stdout);

            cout<<"file_size:"<<file_size<<endl;

            send(c,file_size ,10,0);
            
            //此处接收从客户端发送过来的控制命令：ａｃｋ表示正常下载该文件，ｃａｎｃｅｌ表示进行断点续传
            char control_command[10]={0};//接收从客户端发送过来的控制命令
            if(recv(c,control_command,10,0)<=0)
            {
                cout<<"error"<<endl;
            }
            
            /*
            断点续传：
            １．先接收客户端断点续传文件的位置
            ２．在服务器端打开该请求文件，将文件指针移动到断点位置
            ３．继续正常下载文件
            */
            if(strncmp(control_command,"cancel",6)==0)//不需要完整下载该文件文件，进行断点续传
            {
                char file_size[10]={0};//保存断点位置
                if(recv(c,file_size,10,0)<=0)
                {
                    cout<<"error"<<endl;
                }
    
                int size_break=atoi(file_size);
                printf("断点续传文件的断点位置=%d\n",size_break);
                //将服务器端该文件的文件指针移动到断点位置
                lseek(fd,size_break,SEEK_SET);
                cout<<"正在进行断点续传..."<<endl;
                int num=0;//记录成功接收到的文件数据字节数
                int curr_size=size_break;
                char file_data[512]={0};
                while((num=read(fd,file_data,512))>0)
                {
                    send(c,file_data ,num,0);
                    curr_size+=num;
                    if(curr_size==size)
                    {
                        break;
                    }
                }
                cout<<"断点续传完成！"<<endl;
            }
            if(strncmp(control_command,"ack",3)==0)//正常下载该文件
            {
                sendfile(c,fd,NULL,size);
            }
        }
        if(strcmp(myargv[0],"put")==0)
        {
            /*
            处理文件秒传的代码：
            1.先接收客户端发送过来的待上传文件的MD5值信息
            2.根据待上传文件的ＭＤ５值，在数据库中查找是否存在此文件。如果存在此文件，则用硬链接实现秒传，如果不存在此文件则正常上传该文件
            */
            
            char buffer[128]={0};
            if(recv(c,buffer,128,0)<=0)//接收客户端发送过来的待上传文件的MD5值信息
            {
                return 0;
            }
            printf("待上传文件的ＭＤ５信息为：%s\n",buffer);
            
            char md5sum[33]={0};
            for(int i=0;i<32;++i)
            {
                md5sum[i]=buffer[i];
            }
            
            char filename[96]={0};
            int j=0;
            for(int i=34;buffer[i]!='\0';++i)
            {
                filename[j]=buffer[i];
                ++j;
            }
            
            cout<<"md5sum:"<<md5sum<<endl;
            cout<<"filename:"<<filename<<endl;

            MYSQL mysql; //声明MySQL的句柄
            const char * host = "127.0.0.1";//因为是作为本机测试，所以填写的是本>    地IP
            const char * user = "wangpeng";//这里改为你的用户名，即连接MySQL的用户名
            const char * passwd = "891256";//这里改为你的用户密码
            const char * db = "chat";//这里改为你要连接的数据库的名字,一个数据可>    能有几张表
            unsigned int port = 3306;//这是MySQL的服务器的端口，如果你没有修改过>    的话就是3306。
            const char * unix_socket = NULL;//unix_socket这是unix下的，我在Window    s下，所以就把它设置为NULL
            unsigned long client_flag = 0;//这个参数一般为0

            mysql_init(&mysql);//连接之前必须使用这个函数来初始化

            MYSQL* sock = mysql_real_connect(&mysql, host, user, passwd, db, port    , unix_socket, client_flag);
            if (sock == NULL) //连接MySQL 
            {
                printf("Failed to connect to MySQL!\n");
                fprintf(stderr, " %s\n", mysql_error(&mysql));
                exit(1);
            }
            else
            {
                fprintf(stderr, "Connected to MySQL successfully!\n");
            }

            MYSQL_RES * result;//保存结果集的
            
            if (mysql_set_character_set(&mysql, "gbk")) {   //将字符编码改为gbk
                fprintf(stderr, "错误,字符集更改失败！ %s\n", mysql_error(&mysql));
            }
            
            //拼接SQL语句
            string query = "select * from file_transfer where md5sum='";
            string md5_sum=md5sum;
            string temp="'";
            query=query+md5_sum+temp;
            temp.clear();
            cout<<"拼接完成的SQL查询语句为：" << query << endl;

            const char * i_query = query.c_str();

            if (mysql_query(&mysql, i_query) != 0)//如果连接成功，则开始查询，成功返回0
            {
                fprintf(stderr, "fail to query!\n");
                //打印错误原因
                fprintf(stderr, " %s\n", mysql_error(&mysql));
        
                cout<<"query error!"<<endl;
            }
            else
            {
                result=mysql_store_result(&mysql);
                int row_num = mysql_num_rows(result);//返回结果集中的行的数目
                cout<<"结果集中的记录行数为："<<row_num<<endl;
                if (row_num==0) //保存查询的结果
                {
                    fprintf(stderr, "fail to store result!\n");
                    cout<<"query error!"<<endl<<"数据库中不存在此ＭＤ５值！"<<endl;
                    cout<<"准备插入此条新ＭＤ５记录..."<<endl;
                    
                    //拼接SQL语句
                    string query = "insert into file_transfer (md5sum,file_name) values ('";
                    string md5_sum=md5sum;
                    string temp="','";
                    query=query+md5_sum+temp;
                    string file_name=filename;
                    query=query+file_name;
                    temp.clear();
                    temp="')";
                    query=query+temp;
                    cout<<"拼接完成的SQL查询语句为：" << query << endl;

                    const char * i_query = query.c_str();

                    /*插入，成功则返回0*/
                    int flag = mysql_real_query(&mysql, i_query, (unsigned int)strlen(i_query));
                    if (flag)
                    {
                        printf("Insert data failure!\n");
                        //打印错误原因
                        fprintf(stderr, " %s\n", mysql_error(&mysql));
                    }
                    else {
                        printf("Insert data success!\n");
                        cout<<"数据库中插入文件信息成功！"<<endl;
                    }
                    
                    cout<<"准备上传此文件..."<<endl;
                    
                    send(c,"ack",3,0);//服务器端不存在此文件，确认上传此文件
                    
                    char file_size[10]={0};
                    if(recv(c,file_size,10,0)<=0)
                    {
                        return 0;
                    }
                    printf("要上传的文件大小为：%s\n",file_size);
            
                    send(c,"OK",2,0);//服务器端收到文件的大小，并给客户端进行反馈确认
            
                    int size=atoi(file_size);
                    cout<<"要创建的文件名为："<<myargv[1]<<endl;
                    int fd=open(myargv[1],O_WRONLY | O_CREAT,0600);
                    if(fd==-1)
                    {
                        cout<<"在服务器端创建要接收的文件失败！"<<endl;
                        return 0;
                    }

                    char data_size[512]={0};
                    int num=0;

                    int curr_size=0;//当前文件的大小
                    while((num=recv(c,data_size,512,0))>0)
                    {
                        write(fd,data_size,num);//写入文件
                        curr_size+=num;
                        if(curr_size==size)//接收文件时，对文件的大小进行校验，当接收文件的大小等于文件大小时，说明文件已经接收完了，break退出循环即可
                        {
                            cout<<"文件上传成功！"<<endl;
                            break;
                        }
                    }
                    //send(c,"OK",2,0);//文件上传成功，并给客户端进行反馈确认
                }
                else
                {
                    MYSQL_ROW row;//代表的是结果集中的一行

                    char file_name[96]={0};//保存原文件名
                    
                    while ((row = mysql_fetch_row(result)) != NULL)
                    //读取结果集中的数据，返回的是下一行。因为保存结果集时，当前的游标在第一行【之前】 
                    {
                        cout<<"query succeed!"<<endl<<"服务器端该文件ＭＤ５的信息为："<<endl;
                
                        printf("ID： %s\t", row[0]);//打印当前行的第一列的数据
                        printf("md5： %s\t", row[1]);//打印当前行的第二列的数据
                        printf("file_name： %s\t", row[2]);//打印当前行的第三列的数据
                        strcpy(file_name,row[2]);
                        fflush(stdout);
                    }
                    
                    cout<<"服务器端已存在此文件，正在准备进行文件秒传操作..."<<endl;
                    send(c,"cancel",6,0);//服务器端已存在此文件，给客户端进行反馈，取消上传此文件
                    cout<<"原文件名为："<<file_name<<endl;
                    
                    //进行文件的硬链接
                    string cmd="ln ";
                    string temp=file_name;
                    temp.pop_back();
                    cout<<"file_name大小："<<temp.size()<<endl;
                    cmd=cmd+temp;
                    temp.clear();
                    temp=" ";
                    cmd=cmd+temp;
                    temp.clear();
                    temp=filename;//文件硬链接名
                    cmd=cmd+temp;
                    temp.clear();
            
                    cout<<"cmd:"<<cmd<<endl;
            
                    system(cmd.c_str());
                    //send(c,"OK",2,0);//服务器端文件硬链接创建成功，并给客户端进行反馈确认
                }
            }
        }
        if(strcmp(myargv[0],"pwd")==0)
        {
            int pipefd[2];
            pipe(pipefd);//创建管道
        
            pid_t pid=fork();//先创建管道，再进行fork

            if(pid==-1)
            {
                printf("复制进程出错！\n");
            }
            if(pid==0)//0为子进程
            {
                dup2(pipefd[1],1);//1为读端
                dup2(pipefd[1],2);//2为写端
            
                printf("**************ftp_ser运行目录**************\n");
                execvp(myargv[0],myargv);
                perror("execvp error");
                exit(0);
            }
            
            close(pipefd[1]);
            wait(NULL);
            char readbuff[1024]={0};
            int pipe_num=read(pipefd[0],readbuff,1023);

            send(c,readbuff,pipe_num,0);
            close(pipefd[0]);
        }
    }
    close(c);
}

int socket_create()
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

    int res=bind(sockfd,(struct sockaddr*)&saddr,sizeof(saddr));
    if(res==-1)
    {
        return -1;
    }

    listen(sockfd,5);
    return sockfd;
}


