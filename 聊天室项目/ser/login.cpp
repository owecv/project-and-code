#include "login.h"
#include <map>
#include <iterator>
#include "head.h"
#include <mysql/mysql.h>
using namespace std;

void Login_view::process(char *buff,int fd)
{
    cout<<"buff "<<buff;
    
    Json::Value val;
    Json::Reader read;
    
    if(-1==read.parse(buff,val))
    {
        cout<<"json parse fail"<<endl;
        return ;
    }

    MYSQL *mpcon=mysql_init((MYSQL *)0);
    MYSQL_RES *mp_res;
    MYSQL_ROW mp_row;

    //if(!mysql_real_connect(mpcon,"127.0.0.1","root","1234","chat",3306,NULL,0))
    if(!mysql_real_connect(mpcon,"127.0.0.1","root","891256","chat",3306,NULL,0))
    {
        cout<<"sql link error"<<endl;
        return ;
    }
    
    char cmd2[100]="";
    sprintf(cmd2,"select * from user where name='%s' and passwd='%s'",val["name"].asString().c_str(),val["passwd"].asString().c_str());

    if(mysql_real_query(mpcon,cmd2,strlen(cmd2)))
    {
        cout<<"3 query fail error"<<endl;
        send(fd,"ok",2,0);
        return ;
    }
    mp_res=mysql_store_result(mpcon);
    if((mp_row=mysql_fetch_row(mp_res))!=0)
    {
        send(fd,"OK",2,0);
    }
    send(fd,"fail",4,0);   
}
void Register::process(char *buff,int fd)
{
    cout<<"!!!buff"<<buff<<endl;
    Json::Value val;
    Json::Reader read;
           
    if(-1==read.parse(buff,val))
    {
        cout<<"json parse fail"<<endl;
        return ;
    }
    MYSQL *mpcon=mysql_init((MYSQL *)0);
    MYSQL_RES *mp_res;
    MYSQL_ROW mp_row;
    
    //if(!mysql_real_connect(mpcon,"127.0.0.1","root","1234","chat",NULL,NULL,0))
    if(!mysql_real_connect(mpcon,"127.0.0.1","root","891256","chat",NULL,NULL,0))
    {
        cout<<"sql link error"<<endl;
        return ;
    }
    cout<<"link success"<<endl;
    
    char cmd2[100]="";
    sprintf(cmd2,"select * from user where name='%s' and passwd='%s';",val["name"].asString().c_str(),val["passwd"].asString().c_str());
    
    if(mysql_real_query(mpcon,cmd2,strlen(cmd2)))
    {
        cout<<"already have it 1"<<endl;
        send(fd,"already",7,0);
        return ;
     }
     
    mp_res=mysql_store_result(mpcon);
   if((mp_row=mysql_fetch_row(mp_res))!=0)
   {
       cout<<"already have it 2"<<endl;
       send(fd,"already",7,0);
       return ;
   }
    send(fd,"CAN",3,0);
    
    char cmd[128]="";

    sprintf(cmd,"insert into user values('%s','%s');",val["name"].asString().c_str(),val["passwd"].asString().c_str());
    cout<<"cmd "<<cmd<<endl;
    if(mysql_real_query(mpcon,cmd,strlen(cmd)))
    {
        cout<<"0 query fail error"<<endl;
        send(fd,"faily",5,0);
        return;
    }
    cout<<"insert success"<<endl;
    send(fd,"OK",2,0);
    
    cout<<"success"<<endl;
}

void Onetoone::process(char *buff,int fd)
{
    Json::Value val;
    Json::Reader read;
    
    cout<<"Onetoone buff  "<<buff<<endl;
    char recvbuffs[128]="";
    strcpy(recvbuffs,buff);

    if(-1==read.parse(recvbuffs,val))
    {
        cout<<"json parse fail"<<endl;
        return ;
    }

    cout<<"one bu"<<recvbuffs<<endl; 
    MYSQL *mpcon=mysql_init((MYSQL *)0);
    MYSQL_RES *mp_res;
    MYSQL_ROW mp_row;

    //if(!mysql_real_connect(mpcon,"127.0.0.1","root","1234","chat",3306,NULL,0))
    if(!mysql_real_connect(mpcon,"127.0.0.1","root","891256","chat",3306,NULL,0))
    {
        cout<<"sql link error"<<endl;
        return ;
    }
    
    map<char*,int> map1;
    char cmd3[100]="select * from online;";
    if(mysql_real_query(mpcon,cmd3,strlen(cmd3)))
    {
        cout<<"3 query fail,error"<<endl;
        return ;
    }
    mp_res=mysql_store_result(mpcon);

    while(mp_row=mysql_fetch_row(mp_res)) 
    {
        cout<<"name:"<<mp_row[0]<<" "<<"num"<<mp_row[1]<<endl;
    }

    char recvbuff[512]="";
    char *p=recvbuff;
    map<char *,int>::iterator it=map1.begin();    
    for(;it!=map1.end();++it)
    {
        strcpy(p,it->first);
        p=p+strlen(p);
        *p=it->second;
        strcpy(p+1,"#");
        p=p+2;
    }
    send(fd,recvbuff,511,0);
    
    memset(recvbuff,0,512);
    if(recv(fd,recvbuff,511,0)<=0)
    {
        cout<<"client error or recv error"<<endl;
    }
    
    Json::Value raw;
    Json::Reader reads;
    if(-1==reads.parse(recvbuff,raw))
    {
        cout<<"json parse fail"<<endl;
        return ;
    }
     
    int fds=atoi(raw["fd"].toStyledString().c_str());

    send(fds,raw["data"].toStyledString().c_str(),strlen(raw["data"].toStyledString().c_str()),0) ;  
}
void Onetogroud::process(char *buff,int fd)
{
    Json::Value val;
    Json::Reader read;
    if(-1==read.parse(buff,val));
    {
        cout<<"json parse fail"<<endl;
        return ;
    }

    MYSQL *mpcon=mysql_init((MYSQL *)0);
    MYSQL_RES *mp_res;
    MYSQL_ROW mp_row;
    
    //if(!mysql_real_connect(mpcon,"127.0.0.1","root","123456",NULL,3306,NULL,0))
    if(!mysql_real_connect(mpcon,"127.0.0.1","root","891256",NULL,3306,NULL,0))
    {
        cout<<"sql link error"<<endl;
        return ;
    }
    
    if(mysql_select_db(mpcon,"chat"))
    {
        cout<<"select error"<<endl;
        return ;
    }
    
    map<char*,int> map1;
    map<char*,int>::iterator it=map1.begin();
    
    char cmd3[100]="select * from online;";
    if(mysql_real_query(mpcon,cmd3,strlen(cmd3)))
    {
        cout<<"3 query fail,error"<<endl;
        return ;
    }
    mp_res=mysql_store_result(mpcon);

    while(mp_row=mysql_fetch_row(mp_res))
    {
        cout<<"name:"<<mp_row[0]<<" "<<"num"<<mp_row[1]<<endl;
        char *p=mp_row[0];
        char *p1=mp_row[1];
        int i=atoi(p1);
        map1.insert(make_pair(p,i));
    }

    int k=0;  
    vector<int> vec;
    
    for(;it!=map1.end();++it)
    {
        int h=it->second;
        vec.push_back(h);
    }

    for(int i=0;i<vec.size();++i)
    {
        send(vec[i],val["data"].toStyledString().c_str(),strlen(val["data"].toStyledString().c_str()),0) ;   
    }
}

void Getlist::process(char *buff,int fd)
{
    cout<<"get list"<<endl;

    MYSQL *mpcon=mysql_init((MYSQL *)0);
    MYSQL_RES *mp_res;
    MYSQL_ROW mp_row;

    //if(!mysql_real_connect(mpcon,"127.0.0.1","root","1234","chat",3306,NULL,0))
    if(!mysql_real_connect(mpcon,"127.0.0.1","root","891256","chat",3306,NULL,0))
    {
        cout<<"sql link error"<<endl;
        return ;
    }
    
    map<char*,int> map1;

    char cmd3[100]="select * from online;";
    if(mysql_real_query(mpcon,cmd3,strlen(cmd3)))
    {
        cout<<"3 query fail,error"<<endl;
        return ;
    }
    mp_res=mysql_store_result(mpcon);
    
    while(mp_row=mysql_fetch_row(mp_res))
    {
        cout<<"name:"<<mp_row[0]<<" "<<"num "<<mp_row[1]<<endl;
        char *p=mp_row[0];
        char *p1=mp_row[1];
        int i=atoi(p1);
        map1.insert(make_pair(p,i));
    }

    char buffs[256]="";
    char *p=buffs;
    map<char*,int>::iterator it=map1.begin();
    
    for(;it!=map1.end();++it)
    {
        strcpy(p,it->first);
        p=p+strlen(it->first);
        cout<<"it->second "<<it->second<<endl;
        *p='it->second';
    }
    send(fd,buffs,strlen(buffs),0);
   }


