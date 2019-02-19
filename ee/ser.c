#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

//MySQL头文件
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <mysql/mysql.h> //包含MySQL所需要的头文件
#include <iostream>
#include <typeinfo>
#include <string.h>

//Json头文件
#include <json/json.h>

//多线程处理的头文件
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>

#include <stack>

//fork的头文件
#include <sys/types.h>
#include <sys/wait.h>

const char *ip="127.0.0.1";
unsigned short port=6000;

//MySQL
using namespace std;
#pragma comment(lib, "libmysql.lib")

//Json
using namespace std;
using namespace Json;

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

void function0(MYSQL mysql);//查询数据库的全部信息
void function1(MYSQL mysql);//查询数据库的第一列
void function2(MYSQL mysql);//一些API函数的测试
void function3(MYSQL mysql);//查询指定信息的用户信息
void function4(MYSQL mysql);//测试删除数据库中的记录
void function5();//在数据库中插入记录

int create_socket(const char *ip,unsigned short port);//创建socket套接字

void user_login(MYSQL mysql,char *buff,int fd,string ip,int port);//处理客户端的登录请求
void user_register(MYSQL mysql,char *buff,int fd);//处理客户端的用户注册请求
void user_exit(int fd);//处理客户端用户退出请求
void user_goaway(MYSQL mysql,char *buff,int fd);//处理客户端的下线请求
void user_dispaly(MYSQL mysql,int fd);//查询出所有在线用户，并发送给客户端
void user_chat_one_to_one(MYSQL mysql,char *buff,int fd);//处理客户端“一对一”聊天请求
void user_chat_to_group(MYSQL mysql,char *buff);//处理客户端请求的“群聊”事件
void user_file_transmit(int fd);//处理客户端的文件传输请求
void user_video_chat(MYSQL mysql,int fd,char *buff);//处理客户端的视频聊天请求

void user_message_transmit(char *buff);//解析并转发服务器收到的消息类JSON包

void *user_process(void * arg);//服务器一直循环等待接收客户端的链接，如果接收到客户端的链接就把链接交给这个函数处理。然后继续循环等>     待接收其他客户端的链接

int main()
{
    int sockfd=create_socket(ip,port);
    struct sockaddr_in caddr;

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

    //用结构体封装要传递给process函数的参数
    typedef struct data
    {
        MYSQL mysql;//MySQL的句柄
        int fd;//获得到的fd
		string ip;//连接到的客户端的IP
		int port;//连接到的客户端的port
    }data;

    while(1)
    {
        cout<<"等待客户端连接..."<<endl;
        int len=sizeof(caddr);

        int c=accept(sockfd,(struct sockaddr*)&caddr,(socklen_t*)&len);//接收客户端链接
        
        printf("accept c=%d,ip=%s,port:%d\n",c,inet_ntoa(caddr.sin_addr),ntohs(caddr.sin_port));

        pthread_t id;

        if(c<0)
        {
            continue;
        }
        else//如果服务器接收到来自客户端的连接，就创建一个线程，调用线程函数user_process()去处理它
        {
            //将要传递给porcess函数的参数封装到parm结构体中
            data parm;
            parm.mysql=mysql;
            parm.fd=c;
			parm.ip=inet_ntoa(caddr.sin_addr);
			parm.port=ntohs(caddr.sin_port);
            pthread_create(&id,NULL,user_process,(void *)&parm);
        }
        //pthread_join(id,NULL);
    }
    //关闭MySQL连接
    mysql_close(&mysql);
    return 0;
}

int create_socket(const char *ip,unsigned short port)
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

//处理客户端的登录事件
void user_login(MYSQL mysql,char *buff,int fd,string ip,int port)
{
    //cout<<"buff "<<buff;

    Json::Value val;
    Json::Reader read;

    //Json包解析失败
    if(-1==read.parse(buff,val))
    {
        cout<<"Json parse fail!"<<endl;
        return ;
    }

    MYSQL_RES * result;//保存结果集的

    if (mysql_set_character_set(&mysql, "gbk")) {   //将字符编码改为gbk
    fprintf(stderr, "错误,字符集更改失败！ %s\n", mysql_error(&mysql));
    }

    //拼接SQL语句
    string query = "select * from user where name='";
    string user_name=val["name"].asString().c_str();
    string temp="' and passwd='";
    query=query+user_name+temp;
    string user_passwd=val["passwd"].asString().c_str();
    query=query+user_passwd;
    temp.clear();
    temp="'";
    query=query+temp;
    cout<<"拼接完成的SQL查询语句为：" << query << endl;

    const char * i_query = query.c_str();

    if (mysql_query(&mysql, i_query) != 0)//如果连接成功，则开始查询，成功返回0
    {
        fprintf(stderr, "fail to query!\n");
        //打印错误原因
        fprintf(stderr, " %s\n", mysql_error(&mysql));
        
        cout<<"query error!"<<endl<<"不存在此用户！"<<endl;
        send(fd,"ok",2,0);

        return;
    }
    else
    {
        result=mysql_store_result(&mysql);
        int row_num = mysql_num_rows(result);//返回结果集中的行的数目
        cout<<"结果集中的记录行数为："<<row_num<<endl;
        if (row_num==0) //保存查询的结果
        {
            fprintf(stderr, "fail to store result!\n");
            cout<<"query error!"<<endl<<"不存在此用户！登录失败。。"<<endl;
            send(fd,"ok",2,0);
            return ;
        }
        else
        {
            MYSQL_ROW row;//代表的是结果集中的一行

            while ((row = mysql_fetch_row(result)) != NULL)
            //读取结果集中的数据，返回的是下一行。因为保存结果集时，当前的游标在第一行【之前】 
            {
                cout<<"query succeed!"<<endl<<"服务器端该用户的信息为："<<endl;
                
                printf("ID： %s\t", row[0]);//打印当前行的第一列的数据        
                printf("姓名： %s\t", row[1]);//打印当前行的第二列的数据
                printf("密码： %s\t", row[2]);//打印当前行的第三列的数据
                printf("用户状态： %s\n", row[3]);//打印当前行的第一列的数据
                fflush(stdout);
            }

            //数据库查询，存在此用户且密码正确，更改服务器端用户在线状态
            //拼接SQL语句
            string query = "update user set status='on' where name='";
            string user_name=val["name"].asString().c_str();
            string temp="'";
            query=query+user_name+temp;
            cout<<"拼接完成的SQL查询语句为：" << query << endl;

            const char * i_query = query.c_str();
            if (mysql_query(&mysql, i_query) != 0)//如果连接成功，则开始查询，成功返回0
            {
                fprintf(stderr, "fail to query!\n");
                //打印错误原因
                fprintf(stderr, " %s\n", mysql_error(&mysql));

                cout<<"query error!"<<endl;
                cout<<"服务器更新用户在线状态失败！"<<endl;
                send(fd,"ok",2,0);

                return;
            }
            else
            {
                //该用户上线成功，将该用户的sock_fd添加到MySQL数据库中
                char cmd[100]="";
                sprintf(cmd,"update user set sock_fd=%d where name='%s'",fd,val["name"].asString().c_str());

                cout<<"在MySQL中插入该用户和服务器链接的sock_fd的SQL语句为："<<endl;
                cout<<cmd<<endl;

                if (mysql_real_query(&mysql, cmd,strlen(cmd)) != 0)//如果连接成功，则开始查询，成功返回0
                {
                    fprintf(stderr, "fail to query!\n");
                    //打印错误原因
                    fprintf(stderr, " %s\n", mysql_error(&mysql));

                    cout<<"query error!"<<endl;
                    cout<<"服务器插入sock_fd失败！"<<endl;

                    return;
                }
                else
                {
                    cout<<"在MySQL中插入该用户与服务器连接的sock_fd成功！"<<endl;

                    //给客户点进行确认登录并反馈登录结果
                    send(fd,"OK",2,0);
                    //cout<<"该用户上线成功！等待下一步操作..."<<endl;
                }
            }

			//数据库查询，存在此用户且密码正确，更改服务器端用户IP
            //拼接SQL语句
			query.clear();
			user_name.clear();
			temp.clear();

            query = "update user set ip= '";
			query=query+ip;
			temp="' ";
			query=query+temp;
			temp.clear();
			temp="where name='";
			query=query+temp;

            user_name=val["name"].asString().c_str();
			temp.clear();
            temp="'";
            query=query+user_name+temp;
            cout<<"拼接完成的SQL查询语句为：" << query << endl;

            const char * i_query0 = query.c_str();
            if (mysql_query(&mysql, i_query0) != 0)//如果连接成功，则开始查询，成功返回0
            {
                fprintf(stderr, "fail to query!\n");
                //打印错误原因
                fprintf(stderr, " %s\n", mysql_error(&mysql));

                cout<<"query error!"<<endl;
                cout<<"服务器更新用户IP失败！"<<endl;
                send(fd,"ok",2,0);

                return;
            }
            else
            {
                //该用户上线成功，将该用户的sock_fd添加到MySQL数据库中
                char cmd[100]="";
                sprintf(cmd,"update user set port=%d where name='%s'",port,val["name"].asString().c_str());

                cout<<"在MySQL中插入该用户和服务器链接的port的SQL语句为："<<endl;
                cout<<cmd<<endl;

                if (mysql_real_query(&mysql, cmd,strlen(cmd)) != 0)//如果连接成功，则开始查询，成功返回0
                {
                    fprintf(stderr, "fail to query!\n");
                    //打印错误原因
                    fprintf(stderr, " %s\n", mysql_error(&mysql));

                    cout<<"query error!"<<endl;
                    cout<<"服务器插入连接端口号（port）失败！"<<endl;

                    return;
                }
                else
                {
                    cout<<"在MySQL中插入该用户与服务器连接的port成功！"<<endl;

                    //给客户点进行确认登录并反馈登录结果
                    send(fd,"OK",2,0);
                    cout<<"该用户上线成功！等待下一步操作..."<<endl;
                }
            }
        }
    }
    mysql_free_result(result);//释放结果集result
}

void user_register(MYSQL mysql,char *buff,int fd)//处理客户端的用户注册请求
{
    Json::Value val;
    Json::Reader read;

    //Json包解析失败
    if(-1==read.parse(buff,val))
    {
        cout<<"Json parse fail!"<<endl;
        return ;
    }

    MYSQL_RES * result;//保存结果集的

    if (mysql_set_character_set(&mysql, "gbk")) {   //将字符编码改为gbk
    fprintf(stderr, "错误,字符集更改失败！ %s\n", mysql_error(&mysql));
    }
    
    //拼接SQL语句
    string query = "insert into user (name,passwd) values('";
    string user_name=val["name"].asString().c_str();
    string temp="',";
    query=query+user_name+temp;
    string user_passwd=val["passwd"].asString().c_str();
    query=query+user_passwd;
    temp.clear();
    temp=")";
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
        //注册失败，并给客户端进行反馈
        send(fd,"ok",2,0);
    }
    else {
        printf("Insert data success!\n");
        //数据库插入新用户信息，并给客户反馈注册成功的结果
        send(fd,"OK",2,0);
        cout<<"该用户注册成功！等待下一步操作（登录或退出）"<<endl;
    }
}

void user_exit(int fd)//处理客户端用户退出请求
{
    //服务器处理客户端下线请求
    send(fd,"OK",2,0);

    //关闭socket套接字
    close(fd);
}

void user_goaway(MYSQL mysql,char *buff,int fd)//处理客户端的下线请求
{
    cout<<"服务器正在进行用户下线操作...";

    Json::Value val;
    Json::Reader read;

    //Json包解析失败
    if(-1==read.parse(buff,val))
    {
        cout<<"Json parse fail!"<<endl;
        return ;
    }

    MYSQL_RES * result;//保存结果集的

    if (mysql_set_character_set(&mysql, "gbk")) {   //将字符编码改为gbk
    fprintf(stderr, "错误,字符集更改失败！ %s\n", mysql_error(&mysql));
    }


    //拼接SQL语句
    string query = "update user set status='off' where name='";
    string user_name=val["name"].asString().c_str();
    string temp="'";
    query=query+user_name+temp;
    cout<<"拼接完成的SQL查询语句为：" << query << endl;

    const char * i_query = query.c_str();
    if (mysql_query(&mysql, i_query) != 0)//如果连接成功，则开始查询，成功返回0
    {
        fprintf(stderr, "fail to query!\n");
        //打印错误原因
        fprintf(stderr, " %s\n", mysql_error(&mysql));

        cout<<"query error!"<<endl;
        cout<<"服务器更新用户离线状态失败！"<<endl;
        send(fd,"ok",2,0);

        return;
    }
    else
    {
        //给客户点进行确认登录并反馈登录结果
        send(fd,"OK",2,0);
        cout<<"该用户下线成功！"<<endl;
    }

    //关闭socket套接字
    int c=fd;
    close(c);
}

void *user_process(void *arg)//服务器一直循环等待接收客户端的链接，如果接收到客户端的    链接就把链接交给这个函数处理。然后继续循环等待接收其他客户端的链接
{
    //将主函数传递给process函数的参数，进行解析
    typedef struct data
    {
		MYSQL mysql;//MySQL的句柄
        int fd;//客户端的fd
		string ip;//客户端的IP
		int port;//连接到的客户端的port
    }data;

    //将参数传进来
    data * parm=(data *)arg;
    MYSQL mysql=parm->mysql;
    int fd=parm->fd;
	string ip=parm->ip;
	int port=parm->port;

    cout<<"服务器端注册的所有用户，及各用户在线状态："<<endl;
    function0(mysql);//查询数据库的全部信息
    
    while(1)
    {
        char recv_buff[128]={0};

        void *info;

        if(recv(fd,recv_buff,127,0)<=0)
        {
            //关闭该socket连接
            close(fd);

            cout<<"error!"<<endl;
            cout<<"客户端意外退出！该连接已关闭！"<<endl;
        }
        else
        {
            printf("服务器(user_process)端收到的Json包为：\n%s",recv_buff);
            fflush(stdout);
        }

        //对接收缓冲区中的Json包进行解析
        Json::Value val;
        Json::Reader read;

        //Json包解析失败
        if(-1==read.parse(recv_buff,val))
        {
            cout<<"Json parse failed!"<<endl;
            return 0;
        }
        //判断客户端请求的操作类型
        if(val["type"]==0)//客户端请求登录
        {
            user_login(mysql,recv_buff,fd,ip,port);
        }
        if(val["type"]==1)//客户端请求注册
        {
            user_register(mysql,recv_buff,fd);
        }
        if(val["type"]==2)//客户端请求退出
        {
            user_exit(fd);
            break;
        }
        if(val["type"]==3)//客户端请求下线并退出
        {
            user_goaway(mysql,recv_buff,fd);
            break;
        }
        if(val["type"]==4)//客户端请求列出所有在线用户
        {   
            user_dispaly(mysql,fd);//查询出所有在线用户，并发送给客户端
        }
        if(val["type"]==5)//客户端请求进行一对一聊天
        {
            user_chat_one_to_one(mysql,recv_buff,fd);
        }
        if(val["type"]==6)//客户端请求进行群聊
        {
            user_chat_to_group(mysql,recv_buff);//处理客户端请求的“群聊”事件
        }
        if(val["type"]==7)//服务器收到一个消息类的JSON包
        {
            user_message_transmit(recv_buff);//解析并转发服务器收到的消息类JSON包
        }
        if(val["type"]==8)//客户端请求启动“文件传输服务”
        {
            user_file_transmit(fd);//处理客户端的文件传输请求
        }
		if(val["type"]==9)//客户端请求启动“文件传输服务”
        {
 			user_video_chat(mysql,fd,recv_buff);//处理客户端的视频聊天请求
 		}

    }
    //关闭socket套接字
    close(fd);
    return 0;
}

void user_dispaly(MYSQL mysql,int fd)//查询出所有在线用户，并发送给客户端
{
    MYSQL_RES * result;//保存结果集的

    if (mysql_set_character_set(&mysql, "gbk")) {   //将字符编码改为gbk
    fprintf(stderr, "错误,字符集更改失败！ %s\n", mysql_error(&mysql));
    }

    //拼接SQL语句
    string query = "select id,name from user where status='on'";

    cout<<"拼接完成的SQL查询语句为：" << query << endl;

    const char * i_query = query.c_str();

    if (mysql_query(&mysql, i_query) != 0)//如果连接成功，则开始查询 .成功返回0
    {
        fprintf(stderr, "fail to query!\n");
        exit(1);
    }
    else
    {
        if ((result = mysql_store_result(&mysql)) == NULL) //保存查询的结果
        {
            fprintf(stderr, "fail to store result!\n");
            exit(1);
        }
        else
        {
            MYSQL_ROW row;//代表的是结果集中的一行 
            //my_ulonglong row;
            int row_num = mysql_num_rows(result);//返回结果集中的行的数目
            cout<<"结果集中的记录行数为："<<row_num<<endl;

            cout<<"所有在线用户如下："<<endl;

            string user_online;//保存从结果集中分理出的在线用户信息

            while ((row = mysql_fetch_row(result)) != NULL)
            //读取结果集中的数据，返回的是下一行。因为保存结果集时，当前的游标在第一行【之前】 
            {
                printf("ID： %s\t", row[0]);//打印当前行的第一列的数据        
                printf("姓名： %s\t", row[1]);//打印当前行的第二列的数据
                
                fflush(stdout);
                cout << endl;
                string temp0="ID：";
                string temp1="$Name：";
                user_online=user_online+temp0+row[0]+temp1+row[1];
                string temp="#";
                user_online=user_online+temp;
            }
            cout<<"发送给客户端的数据为："<<user_online.c_str()<<endl;
            if(-1==send(fd,user_online.c_str(),strlen(user_online.c_str()),0))
            {
                cout<<"发送信息失败！"<<endl;
                return ;
            }   
        }
    }
    mysql_free_result(result);//释放结果集result
}

void user_chat_one_to_one(MYSQL mysql,char *buff,int fd)//处理客户端“一对一”聊天请求
{
    cout<<"正在进行“一对一”聊天处理..."<<endl;

    Json::Value val;
    Json::Reader read;

    //Json包解析失败
    if(-1==read.parse(buff,val))
    {
        cout<<"Json parse fail!"<<endl;
        return ;
    }
   
    //一对一聊天时A的sock_fd
    int A_fd;
    //一对一聊天时B的sock_fd
    int B_fd;

    //查询客户请求“一对一聊天的对方（B）是否在线”
    MYSQL_RES * result;//保存结果集的

    if (mysql_set_character_set(&mysql, "gbk")) {   //将字符编码改为gbk
    fprintf(stderr, "错误,字符集更改失败！ %s\n", mysql_error(&mysql));
    }

    //拼接SQL语句
    //查询B用户是否在线,并且查询出B_fd
    string query = "select * from user where name='";
    string user_name=val["B_name"].asString().c_str();
    string temp="' and status='on'";
    query=query+user_name+temp;
    
    cout<<"拼接完成的SQL查询语句为：" << query << endl;

    const char * i_query = query.c_str();

    if (mysql_query(&mysql, i_query) != 0)//如果连接成功，则开始查询，成功返回0
    {
        fprintf(stderr, "fail to query!\n");
        //打印错误原因
        fprintf(stderr, " %s\n", mysql_error(&mysql));

        cout<<"query error!"<<endl<<"不存在此用户！"<<endl;
        send(fd,"ok",2,0);

        return;
    }
    else
    {
        result=mysql_store_result(&mysql);
        int row_num = mysql_num_rows(result);//返回结果集中的行的数目
        cout<<"结果集中的记录行数为："<<row_num<<endl;
        if (row_num==0) //保存查询的结果
        {
            fprintf(stderr, "fail to store result!\n");
            cout<<"query error!"<<endl<<"B用户不在线！连接失败。。"<<endl;
            send(fd,"ok",2,0);
            return ;
        }
        else
        {
            MYSQL_ROW row;//代表的是结果集中的一行

            while ((row = mysql_fetch_row(result)) != NULL)
            //读取结果集中的数据，返回的是下一行。因为保存结果集时，当前的游标在第一行【之前】 
            {
                cout<<"query succeed!"<<endl<<"服务器端该用户的信息为："<<endl;

                printf("ID： %s\t", row[0]);//打印当前行的第一列的数据        
                printf("姓名： %s\t", row[1]);//打印当前行的第二列的数据
                printf("密码： %s\t", row[2]);//打印当前行的第三列的数据
                printf("用户状态： %s\t", row[3]);//打印当前行的第一列的数据
                printf("fd： %s\n", row[4]);//打印当前行的第一列的数据
                B_fd=atoi(row[4]);
                fflush(stdout);
            }
            printf("B的fd为：%d\n",B_fd);

            //send(fd,"OK",2,0);

            //拼接SQL语句，查询A自己的fd
            string query = "select * from user where name='";
            string user_name=val["A_name"].asString().c_str();
            string temp="'";
            query=query+user_name+temp;
            
            cout<<"拼接完成的SQL查询语句为：" << query << endl;
            
            const char * i_query = query.c_str();
            
            if (mysql_query(&mysql, i_query) != 0)//如果连接成功，则开始查询，成功返回0
            {
                fprintf(stderr, "fail to query!\n");
                //打印错误原因
                fprintf(stderr, " %s\n", mysql_error(&mysql));
            
                cout<<"query error!"<<endl<<"不存在此用户！"<<endl;
                send(fd,"ok",2,0);
                
                return;
            }
            else
            {
                result=mysql_store_result(&mysql);
                int row_num = mysql_num_rows(result);//返回结果集中的行的数目
                cout<<"结果集中的记录行数为："<<row_num<<endl;
                if (row_num==0) //保存查询的结果
                {
                    fprintf(stderr, "fail to store result!\n");
                    cout<<"query error!"<<endl<<"该用户（A）不在线！连接失败。。"<<endl;
                    return ;
                }
                else
                {
                    MYSQL_ROW row;//代表的是结果集中的一行
                
                    while ((row = mysql_fetch_row(result)) != NULL)
                    //读取结果集中的数据，返回的是下一行。因为保存结果集时，当前的游标在第一行【之前】 
                    {
                            cout<<"query succeed!"<<endl<<"服务器端该用户的信息为："<<endl;
                    
                        printf("ID： %s\t", row[0]);//打印当前行的第一列的数据        
                        printf("姓名： %s\t", row[1]);//打印当前行的第二列的数据
                        printf("密码： %s\t", row[2]);//打印当前行的第三列的数据
                        printf("用户状态： %s\t", row[3]);//打印当前行的第四列的数据
                        printf("sock_fd：%s\n",row[4]);//打印当前行的第五列的数据
                        A_fd=atoi(row[4]);
                        fflush(stdout);
                    }
                    printf("A的fd为：%d\n",A_fd);
                }
            }

            //制作反馈客户端请求“一对一聊天”的Json包
            //1.对客户端请求的“一对一聊天”B用户身份和在线状态进行核对
            //2.查询出A用户的fd，和B用户的fd
            //3.对客户端的“一对一聊天”请求进行确认
          
            Json::Value val_OK;
            val_OK["OK"]="OK";
            val_OK["A_name"]=val["A_name"];//Ay 用户的名字
            val_OK["A_fd"]=A_fd;//A用户的fd
            val_OK["B_name"]=val["B_name"];//B用户的名字
            val_OK["B_fd"]=B_fd;//B用户的fd
            
            if(-1==send(fd,val_OK.toStyledString().c_str(),strlen(val_OK.toStyledString().c_str()),0))
            {
                cout<<"服务器端发送“一对一聊天”请求的确认JSON包失败！"<<endl;
                return ;
            }
            else
            {
                cout<<"服务器端发送“一对一聊天”请求的确认JSON包成功！"<<endl;
            }
        }
    }
}

void user_message_transmit(char *buff)//解析并转发服务器收到的消息类JSON包
{
    cout<<"正在处理信息类JSON包..."<<endl;

    Json::Value val;
    Json::Reader read;

    //Json包解析失败
    if(-1==read.parse(buff,val))
    {
        cout<<"Json parse fail!"<<endl;
        return ;
    }
    
    int B_fd=atoi(val["B_fd"].toStyledString().c_str());//信息的目的fd
    
    if(-1==send(B_fd,val.toStyledString().c_str(),strlen(val.toStyledString().c_str()),0))
    {
        cout<<"服务器转发信息类JSON包失败！"<<endl;
        return ;
    }
    else
    {
        cout<<"服务器转发信息类JSON包成功！"<<endl;
    }
}

void user_chat_to_group(MYSQL mysql,char *buff)//处理客户端请求的“群聊”事件
{
    cout<<"正在处理群发信息类JSON包..."<<endl;
    
    //查询出所有在线用户的fd
    MYSQL_RES * result;//保存结果集的
    
    if (mysql_set_character_set(&mysql, "gbk"))//将字符编码改为gbk
    {
        fprintf(stderr, "错误,字符集更改失败！ %s\n", mysql_error(&mysql));
    }
    
    //拼接SQL语句
    string query = "select *from user where status='on'";
    
    cout<<"拼接完成的SQL查询语句为：" << query << endl;
    
    const char * i_query = query.c_str();
    
    if (mysql_query(&mysql, i_query) != 0)//如果连接成功，则开始查询 .成功返回0
    {
        fprintf(stderr, "fail to query!\n");
        exit(1);
    }
    else
    {
        if ((result = mysql_store_result(&mysql)) == NULL) //保存查询的结果
        {
            fprintf(stderr, "fail to store result!\n");
            exit(1);
        }
        else
        {
            MYSQL_ROW row;//代表的是结果集中的一行 
            //my_ulonglong row;
            int row_num = mysql_num_rows(result);//返回结果集中的行的数目
            cout<<"结果集中的记录行数为："<<row_num<<endl;
            
            cout<<"所有在线用户如下："<<endl;
            
            stack<int> user_online_fd;//保存从结果集中分离出的在线用户的fd
            int fd_temp;
            while ((row = mysql_fetch_row(result)) != NULL)
            //读取结果集中的数据，返回的是下一行。因为保存结果集时，当前的游标在第一行【之前】 
            {
                printf("ID： %s\t", row[0]);//打印当前行的第一列的数据        
                printf("姓名： %s\t", row[1]);//打印当前行的第二列的数据
                printf("密码： %s\t", row[2]);//打印当前行的第三列的数据
                printf("用户状态： %s\t", row[3]);//打印当前行的第四列的数据
                printf("fd： %s\t",row[4]);//打印当前行的第五列的数据
                fd_temp=atoi(row[4]);
                user_online_fd.push(fd_temp);
                cout << endl;
                cout<<"在user_online_fd中添加fd成功！"<<fd_temp<<endl;
                fflush(stdout);
            }
            while(!user_online_fd.empty())
            {
                int fd=user_online_fd.top();
                user_online_fd.pop();
                if(-1==send(fd,buff,strlen(buff),0))
                {
                    cout<<"服务器转发信息类JSON包失败！"<<endl;
                    return ;
                }
                else
                {
                    cout<<"服务器转发信息类JSON包成功！"<<endl;
                }

            }
        }
    }
    mysql_free_result(result);//释放结果集result
}

void user_file_transmit(int fd)//处理客户端的文件传输请求
{
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
        send(fd,"OK",2,0);
        //waitpid(pid,&status,0);
    }
    if(pid==0)
    {
        printf("正在启动ftp_ser程序...\n");
        execl("/home/wangpeng/代码/qq/小插件/ftp_ser","ftp_ser",NULL,NULL);
    }
}

void user_video_chat(MYSQL mysql,int fd,char *buff)//处理客户端的视频聊天请求
{
	cout<<"正在处理信息类JSON包..."<<endl;
  
 	Json::Value val;
 	Json::Reader read;
 
	//Json包解析失败
 	if(-1==read.parse(buff,val))
 	{
 		cout<<"Json parse fail!"<<endl;
 		return ;
 	}

	string A_name=val["A_name"].asString().c_str();
	string B_name=val["B_name"].asString().c_str();
	string A_IP;
	string B_IP;
	int A_port;
	int B_port;
	int A_fd;
	int B_fd;
	//查询客户请求“视频聊天的对方（B）是否在线”
    MYSQL_RES * result;//保存结果集的

    if (mysql_set_character_set(&mysql, "gbk")) {   //将字符编码改为gbk
    fprintf(stderr, "错误,字符集更改失败！ %s\n", mysql_error(&mysql));
    }

	//拼接SQL语句
    //查询B用户是否在线,并且查询出B_fd
    string query = "select * from user where name='";
    string user_name=val["B_name"].asString().c_str();
    string temp="' and status='on'";
    query=query+user_name+temp;
    
    cout<<"拼接完成的SQL查询语句为：" << query << endl;

    const char * i_query = query.c_str();

    if (mysql_query(&mysql, i_query) != 0)//如果连接成功，则开始查询，成功返回0
    {
        fprintf(stderr, "fail to query!\n");
        //打印错误原因
        fprintf(stderr, " %s\n", mysql_error(&mysql));

        cout<<"query error!"<<endl<<"不存在此用户！"<<endl;
        send(fd,"ok",2,0);

        return;
    }
    else
    {
        result=mysql_store_result(&mysql);
        int row_num = mysql_num_rows(result);//返回结果集中的行的数目
        cout<<"结果集中的记录行数为："<<row_num<<endl;
        if (row_num==0) //保存查询的结果
        {
            fprintf(stderr, "fail to store result!\n");
            cout<<"query error!"<<endl<<"B用户不在线！连接失败。。"<<endl;
            send(fd,"ok",2,0);
            return ;
        }
        else
        {
            MYSQL_ROW row;//代表的是结果集中的一行

            while ((row = mysql_fetch_row(result)) != NULL)
            //读取结果集中的数据，返回的是下一行。因为保存结果集时，当前的游标在第一行【之前】 
            {
                cout<<"query succeed!"<<endl<<"服务器端该用户的信息为："<<endl;

                printf("ID： %s\t", row[0]);//打印当前行的第一列的数据        
                printf("姓名： %s\t", row[1]);//打印当前行的第二列的数据
                printf("密码： %s\t", row[2]);//打印当前行的第三列的数据
                printf("用户状态： %s\t", row[3]);//打印当前行的第四列的数据
                printf("fd： %s\t", row[4]);//打印当前行的第五列列的数据
				printf("IP： %s\t", row[5]);//打印当前行的第六列的数据
				printf("端口号： %s\t", row[6]);//打印当前行的第七列的数据
                fflush(stdout);

                cout << endl;
                B_fd=atoi(row[4]);
				B_IP=row[5];
				B_port=atoi(row[6]);
                fflush(stdout);
            }
            printf("B的fd为：%d\n",B_fd);
			cout<<"B的IP为："<<B_IP<<endl;
			cout<<"B的port为："<<B_port<<endl;

			//拼接SQL语句，查询A自己的fd
            string query = "select * from user where name='";
            string user_name=val["A_name"].asString().c_str();
            string temp="'";
            query=query+user_name+temp;
            
            cout<<"拼接完成的SQL查询语句为：" << query << endl;
            
            const char * i_query = query.c_str();
            
            if (mysql_query(&mysql, i_query) != 0)//如果连接成功，则开始查询，成功返回0
            {
                fprintf(stderr, "fail to query!\n");
                //打印错误原因
                fprintf(stderr, " %s\n", mysql_error(&mysql));
            
                cout<<"query error!"<<endl<<"不存在此用户！"<<endl;
                send(fd,"ok",2,0);
                
                return;
            }
            else
            {
                result=mysql_store_result(&mysql);
                int row_num = mysql_num_rows(result);//返回结果集中的行的数目
                cout<<"结果集中的记录行数为："<<row_num<<endl;
                if (row_num==0) //保存查询的结果
                {
                    fprintf(stderr, "fail to store result!\n");
                    cout<<"query error!"<<endl<<"该用户（A）不在线！连接失败。。"<<endl;
                    return ;
                }
                else
                {
                    MYSQL_ROW row;//代表的是结果集中的一行
                
                    while ((row = mysql_fetch_row(result)) != NULL)
                    //读取结果集中的数据，返回的是下一行。因为保存结果集时，当前的游标在第一行【之前】 
                    {
                            cout<<"query succeed!"<<endl<<"服务器端该用户的信息为："<<endl;
                    
                        printf("ID： %s\t", row[0]);//打印当前行的第一列的数据        
                        printf("姓名： %s\t", row[1]);//打印当前行的第二列的数据
                        printf("密码： %s\t", row[2]);//打印当前行的第三列的数据
                        printf("用户状态： %s\t", row[3]);//打印当前行的第四列的数据
                        printf("sock_fd：%s\n",row[4]);//打印当前行的第五列的数据
						printf("IP： %s\t", row[5]);//打印当前行的第六列的数据
 						printf("端口号： %s\t", row[6]);//打印当前行的第七列的数据
 						fflush(stdout);
 
						cout << endl;
 						A_fd=atoi(row[4]);
 						A_IP=row[5];
 						A_port=atoi(row[6]);
						fflush(stdout);
					}
 					printf("A的fd为：%d\n",B_fd);
 					cout<<"A的IP为："<<A_IP<<endl;
 					cout<<"A的port为："<<A_port<<endl;
                }
            }

            //制作反馈客户端请求“视频聊天”的Json包
            //1.对客户端请求的“一对一聊天”B用户身份和在线状态进行核对
            //2.查询出A用户的fd，和B用户的fd
            //3.对客户端的“一对一聊天”请求进行确认
          
			TYPE=TYPE_VIDEO;
            Json::Value val_OK;
			val_OK["type"]=TYPE;
            val_OK["OK"]="OK";
            val_OK["A_name"]=val["A_name"];//Ay 用户的名字
            val_OK["A_fd"]=A_fd;//A用户的fd
			val_OK["A_IP"]=A_IP;//A用户的IP
			val_OK["A_port"]=A_port;//A用户的port
            
			val_OK["B_name"]=val["B_name"];//B用户的名字
            val_OK["B_fd"]=B_fd;//B用户的fd
			val_OK["B_IP"]=B_IP;//B用户的IP
			val_OK["B_port"]=B_port;//B用户的port
            
			if(-1==send(B_fd,val_OK.toStyledString().c_str(),strlen(val_OK.toStyledString().c_str()),0))
			{
		     	cout<<"服务器向B发送“视频聊天”请求的JSON包失败！"<<endl;
				return ;
			}
			else
			{
				cout<<"服务器向B发送“视频聊天”请求的JSON包成功！"<<endl;
			}
			sleep(1);

            if(-1==send(fd,val_OK.toStyledString().c_str(),strlen(val_OK.toStyledString().c_str()),0))
            {
                cout<<"服务器端发送“视频聊天”请求的确认JSON包失败！"<<endl;
                return ;
            }
            else
            {
                cout<<"服务器端发送“视频聊天”请求的确认JSON包成功！"<<endl;
            }
		}
	}
}

void function0(MYSQL mysql)//查询数据库的全部信息
{
	MYSQL_RES * result;//保存结果集的

	if (mysql_set_character_set(&mysql, "gbk")) {	//将字符编码改为gbk
		fprintf(stderr, "错误, %s\n", mysql_error(&mysql));
	}

	const char * i_query = "select * from user";//查询语句，从那个表中查询,这里后面没有;
	if (mysql_query(&mysql, i_query) != 0)//如果连接成功，则开始查询 .成功返回0
	{
		fprintf(stderr, "fail to query!\n");
		exit(1);
	}
	else
	{
		if ((result = mysql_store_result(&mysql)) == NULL) //保存查询的结果
		{
			fprintf(stderr, "fail to store result!\n");
			exit(1);
		}
		else
		{
			MYSQL_ROW row;//代表的是结果集中的一行 
						  //my_ulonglong row;
			while ((row = mysql_fetch_row(result)) != NULL)
				//读取结果集中的数据，返回的是下一行。因为保存结果集时，当前的游标在第一行【之前】 
			{
                printf("ID： %s\t", row[0]);//打印当前行的第一列的数据        
                printf("姓名： %s\t", row[1]);//打印当前行的第二列的数据
                printf("密码： %s\t", row[2]);//打印当前行的第三列的数据
                printf("用户状态： %s\t", row[3]);//打印当前行的第四列的数据
				printf("sock_fd： %s\t", row[4]);//打印当前行的第四列的数据
				printf("IP： %s\t", row[5]);//打印当前行的第五列的数据
				printf("端口号： %s\t", row[6]);//打印当前行的第六列的数据
                fflush(stdout);

                cout << endl;
			}
		}
	}
	mysql_free_result(result);//释放结果集result
}

void function1(MYSQL mysql)//查询数据库的第一列
{
	MYSQL_RES * result;//保存结果集的
	if (mysql_set_character_set(&mysql, "gbk")) {	//将字符编码改为gbk
		fprintf(stderr, "错误, %s\n", mysql_error(&mysql));
	}

	const char * i_query0 = "select 姓名 from info";//查询语句，从那个表中查询,这里后面没有;

	if (mysql_query(&mysql, i_query0) != 0)//如果连接成功，则开始查询 .成功返回0
	{
		fprintf(stderr, "fail to query!\n");
		exit(1);
	}
	else
	{
		if ((result = mysql_store_result(&mysql)) == NULL) //保存查询的结果
		{
			fprintf(stderr, "fail to store result!\n");
			exit(1);
		}
		else
		{
			MYSQL_ROW row;//代表的是结果集中的一行
			while ((row = mysql_fetch_row(result)) != NULL)
			//读取结果集中的数据，返回的是下一行。因为保存结果集时，当前的游标在第一行【之前】 
			{
				printf("姓名： %s\n", row[0]);//打印当前行的第一列的数据
			}
		}
	}
	mysql_free_result(result);//释放结果集result
}

void function2(MYSQL mysql)//一些API函数的测试
{
	MYSQL_RES * result;//保存结果集的
	if (mysql_set_character_set(&mysql, "gbk")) {	//将字符编码改为gbk
		fprintf(stderr, "错误, %s\n", mysql_error(&mysql));
	}

	const char * i_query = "select * from info";//查询语句，从那个表中查询,这里后面没有;
	if (mysql_query(&mysql, i_query) != 0)//如果连接成功，则开始查询 .成功返回0
	{
		fprintf(stderr, "fail to query!\n");
		exit(1);
	}
	else
	{
		if ((result = mysql_store_result(&mysql)) == NULL) //保存查询的结果
		{
			fprintf(stderr, "fail to store result!\n");
			exit(1);
		}
		else
		{
			int num = mysql_num_fields(result);//计算结果集中的列数
			int row_num = mysql_num_rows(result);//返回结果集中的行的数目
			cout << "/******************/" << endl;
			cout << "结果集中的列数为：" << num << endl;
			cout << "结果集中的行数为：" << row_num << endl;
			/*
			typedef struct st_mysql_res
			{
			my_ulonglong row_count;	                     // 结果集的行数
			unsigned int	field_count, current_field;	     // 结果集的列数，当前列
			MYSQL_FIELD	*fields;	                     // 结果集的列信息
			MYSQL_DATA	*data;	                         // 结果集的数据
			MYSQL_ROWS	*data_cursor;	                 // 结果集的光标
			MEM_ROOT	field_alloc;                         // 内存结构
			MYSQL_ROW	row;	                             // 非缓冲的时候用到
			MYSQL_ROW	current_row;                         // mysql_store_result时会用到。当前行
			unsigned long *lengths;	                     // 每列的长度
			MYSQL	*handle;	                             // mysql_use_result会用。
			my_bool	eof;	                             // 是否为行尾
			} MYSQL_RES;
			*/
			cout << "/*******************/" << endl;
			cout << "结构体指针的方法" << endl;
			cout << "结果集中的列数：" << result->field_count << endl;
			cout << "结果集中的行数：" << result->row_count << endl;
			cout << "当前行数：" << result->current_field << endl;

			cout << "/*******************/" << endl;
			cout << "返回上次UPDATE、DELETE或INSERT查询更改／删除／插入的行数" << endl;
			cout << mysql_affected_rows(&mysql) << endl;
			cout << "返回下一个表字段的类型" << endl;
			cout << mysql_fetch_field(result) << endl;
			cout << "返回全部字段结构的数组。" << endl;
			cout << mysql_fetch_fields(result) << endl;
			cout << "以字符串形式返回client版本号信息。" << endl;
			cout << mysql_get_client_info() << endl;
			cout << "以整数形式返回client版本号信息。" << endl;
			cout << mysql_get_client_version() << endl;
			cout << "返回描写叙述连接的字符串。" << endl;
			cout << mysql_get_host_info(&mysql) << endl;
			cout << "获取或初始化MYSQL结构。" << endl;
			cout << mysql_init(&mysql) << endl;
			cout << "检查与server的连接是否工作，如有必要又一次连接。" << endl;
			//返回1表示server的连接正常工作
			cout << mysql_ping(&mysql) << endl;
			cout << "选择数据库。" << endl;
			cout << mysql_select_db(&mysql, "test0") << endl;
			cout << "以字符串形式返回server状态。" << endl;
			cout << mysql_stat(&mysql) << endl;
			cout << "返回当前线程ID。" << endl;
			cout << mysql_thread_id(&mysql) << endl;
			cout << "初始化MySQL库" << endl;
			mysql_library_init;
			cout << "结束MySQL库的使用。" << endl;
			mysql_library_end();
			cout << "对于每一个非SELECT查询（比如INSERT、UPDATE、DELETE）。通过调用mysql_affected_rows()。可发现有多少行已被改变（影响）。" << endl;
			cout << mysql_affected_rows(&mysql) << endl;
}
	}
	mysql_free_result(result);//释放结果集result
}

void function3(MYSQL mysql)//查询指定信息的用户信息
{
	MYSQL_RES * result;//保存结果集的

	if (mysql_set_character_set(&mysql, "gbk")) {	//将字符编码改为gbk
		fprintf(stderr, "错误, %s\n", mysql_error(&mysql));
	}

	/*cout << "请输入用户名：";
	char *name ;
	cin >> *name;*/

	//const char * i_query = "select * from info where 性别='女'";//查询语句，从那个表中查询,这里后面没有;
	while (1) {
		string query = "select * from info where 姓名='";
		cout << "请输入用户名：";
		string name;
		cin >> name;
		query = query + name;
		string temp = "'";
		query = query + temp;
		cout << query << endl;
		const char * i_query = query.c_str();
		if (mysql_query(&mysql, i_query) != 0)//如果连接成功，则开始查询 .成功返回0
		{
			fprintf(stderr, "fail to query!\n");
			//打印错误原因
			fprintf(stderr, " %s\n", mysql_error(&mysql));
			exit(1);
		}
		else
		{
			if ((result = mysql_store_result(&mysql)) == NULL) //保存查询的结果
			{
				fprintf(stderr, "fail to store result!\n");
				exit(1);
			}
			else
			{
				MYSQL_ROW row;//代表的是结果集中的一行 
							  //my_ulonglong row;
				int n = 1;
				while ((row = mysql_fetch_row(result)) != NULL)
					//读取结果集中的数据，返回的是下一行。因为保存结果集时，当前的游标在第一行【之前】 
				{
					cout << "序号：" << n++ << "  ";
					//cout << mysql_fetch_field(result) << endl;
					printf("姓名： %s\t", row[0]);//打印当前行的第一列的数据
											   //cout <<"name is " << (char *)row[0] << "\t";
											   //cout << typeid(row[0]).name() << endl;
					printf("性别： %s\t", row[1]);//打印当前行的第二列的数据
											   //row = mysql_num_row(result);
											   //printf("%lu\n", mysql_num_row(result));
					printf("准考证号： %s\t", row[2]);//打印当前行的第一列的数据
					printf("证件编号： %s\t", row[3]);//打印当前行的第二列的数据
												 //cout << typeid(row[4]).name() << endl;
					printf("民族： %s\t\n", row[4]);//打印当前行的第一列的数据 
												 /*cout << row[0] << "\t";
												 cout << row[1] << "\t";*/
					cout << endl;
				}
			}
		}
	}
	mysql_free_result(result);//释放结果集result
}

void function4(MYSQL mysql)//删除数据库中的记录
{
	const char * query = "delete from info where 姓名='王子裕'";
	/*删除，成功则返回0*/
	int flag = mysql_real_query(&mysql, query, (unsigned int)
		strlen(query));
	if (flag) {
		printf("Delete data failure!\n");
	}
	else {
		printf("Delete data success!\n");
	}
	mysql_close(&mysql);
}

void function5()//在数据库中插入记录
{
	MYSQL mysql; //声明MySQL的句柄
	const char * host = "127.0.0.1";//因为是作为本机测试，所以填写的是本地IP
	const char * user = "root";//这里改为你的用户名，即连接MySQL的用户名
	const char * passwd = "891256";//这里改为你的用户密码
	const char * db = "mydatabase";//这里改为你要连接的数据库的名字,一个数据可能有几张表
	unsigned int port = 3306;//这是MySQL的服务器的端口，如果你没有修改过的话就是3306。
	const char * unix_socket = NULL;//unix_socket这是unix下的，我在Windows下，所以就把它设置为NULL
	unsigned long client_flag = 0;//这个参数一般为0

	mysql_init(&mysql);//连接之前必须使用这个函数来初始化

	MYSQL* sock = mysql_real_connect(&mysql, host, user, passwd, db, port, unix_socket, client_flag);
	if (sock == NULL) //连接MySQL 
	{
		printf("fail to connect mysql \n");
		fprintf(stderr, " %s\n", mysql_error(&mysql));
		exit(1);
	}
	else
	{
		fprintf(stderr, "connect ok!\n");
	}

	if (mysql_set_character_set(&mysql, "gbk")) {	//将字符编码改为gbk
		fprintf(stderr, "错误, %s\n", mysql_error(&mysql));
	}

	const char * query = "insert into info(姓名,证件编号) values('华成龙',101)";
	/*插入，成功则返回0*/
	int flag = mysql_real_query(&mysql, query, (unsigned int)
		strlen(query));
	if (flag) {
		printf("Insert data failure!\n");
		//打印错误原因
		fprintf(stderr, " %s\n", mysql_error(&mysql));
	}
	else {
		printf("Insert data success!\n");
	}
	mysql_close(&mysql);
}
