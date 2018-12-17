#include "cli.h"//包含自己的头文件
#include <json/json.h>
#include <iostream>
#include <sys/socket.h>//socket就是一个可读可写的文件描述符、linux中一切皆文件
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>//十进制的点分IP字符串到二进制数的转换
#include <pthread.h>
#include <unistd.h>
using namespace std;

enum _TYPE
{
    TYPE_LOGIN,
    TYPE_REG,
    TYPE_ONE,
    TYPE_GROUD,
    TYPE_LIST,
    TYPE_GO 
}TYPE;

void run(int fd)
{
    cout<<"1.login\n2.register\n3.exit\n"<<endl;
    int choice;
    cin>>choice;
    switch(choice)
    {
    case 1:
        {
            Login(fd);    
        }break;
    case 2:
        {
            Register(fd);
        }break;
    case 3:
        {
            Goaway(fd);
        }break;
    default:
        {
          cout<<"cin error"<<endl;

        }
    }
}
void Login(int fd)
{
    char name[20]="";
    cout<<"please cin name"<<endl;

    cin>>name;
    char pw[20]="";
    cout<<"please cin passwd"<<endl;
    cin>>pw;

    TYPE=TYPE_LOGIN;
    Json::Value val;
    val["type"]=TYPE;
    val["name"]=name;
    val["passwd"]=pw;
    
    if(-1==send(fd,val.toStyledString().c_str(),strlen(val.toStyledString().c_str()),0))
    {
        cout<<"send reason fail"<<endl;
        return ;
    }
    char recvbuff[10]="";

    if(recv(fd,recvbuff,9,0)<=0)
    {
        cout<<"server unlink or error"<<endl;
    }
    if(strncmp(recvbuff,"OK",2)==0)
    {
        cout<<"login success"<<endl;
        login_success(fd);
    }
    cout<<"no success"<<endl;
}
void login_success(int fd)
{
    while(1)
    {
    cout<<"1:get list"<<endl;
    cout<<"2:talk one"<<endl;
    cout<<"3:talk groud"<<endl;

    int  choice;
    cin>>choice;
    switch(choice)
    {
       case 1:
        {
            TYPE=TYPE_LIST;
            cout<<"TYPE"<<TYPE<<endl;
            
            Json::Value val;
            val["type"]=TYPE;
            
            send(fd,val.toStyledString().c_str(),strlen(val.toStyledString().c_str()),0);    
            char buff[128]="";
            if(0<recv(fd,buff,127,0))
            {
                cout<<"data is "<<buff<<endl;
            }
        }break;
       case 2:
        {
            TYPE=TYPE_LIST; 
            Json::Value val;
            val["type"]=TYPE;
            send(fd,val.toStyledString().c_str(),strlen(val.toStyledString().c_str()),0);    
            
            char buff[128]="";
            if(0<recv(fd,buff,127,0))
            {
                cout<<buff<<endl;
            }
            TYPE=TYPE_ONE;
            Json::Value raw;
            raw["type"]=TYPE;
            string data;
            string name;
            cout<<"please put data"<<endl;
            cin>>data;
            raw["data"]=data;
            cout<<"please put name"<<endl;
            cin>>name;
            raw["name"]=name;
            int fd;
            cin>>fd;
            raw["fd"]=fd;
            send(fd,val.toStyledString().c_str(),strlen(val.toStyledString(
            ).c_str()),0);    
        
        }break;
       case 3:
        {
            TYPE=TYPE_GROUD;
            Json::Value val;
            val["type"]=TYPE;
            string data;
            cin>>data;
            val["data"]=data;
            
            send(fd,val.toStyledString().c_str(),strlen(val.toStyledString(
            ).c_str()),0);    
        }break;
    }
  }  
}
void Register(int fd)
{
    char name[20]="";
    cout<<"please cin name"<<endl;

    cin>>name;
    char pw[20]="";
    cout<<"please cin passwd"<<endl;
    cin>>pw;
    
    TYPE=TYPE_REG;
    Json::Value val;
    val["type"]=TYPE;
    val["name"]=name;
    val["passwd"]=pw;
    
    if(-1==send(fd,val.toStyledString().c_str(),strlen(val.toStyledString().c_str()),0))
    {
        cout<<"send reason fail"<<endl;
        return ;
    }
    char recvbuff[10]="";
    if(0<recv(fd,recvbuff,9,0))
    {
        if(strncmp(recvbuff,"already",7)==0)
        {
            cout<<"already have data"<<endl;
            return ;
        }
    }
    memset(recvbuff,0,10);
    if(0<recv(fd,recvbuff,9,0))
    {
        if(strncmp(recvbuff,"OK",2)==0)
        {
            cout<<"login success"<<endl;
            login_success(fd);
        }
    }
}

void Goaway(int fd)
{
    Json::Value val;
    TYPE=TYPE_GO;
    val["type"]=TYPE;
    if(-1==send(fd,val.toStyledString().c_str(),strlen(val.toStyledString().c_str()),0))
    {
        cout<<"send json error"<<endl;
        return;
    }
}
