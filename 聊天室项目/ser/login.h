#include<iostream>
using namespace std;

class view
{ public:
    virtual void process(char *buff,int fd)=0;
};

class Login_view:public view
{
    public:
    void process(char *buff,int fd);
};

class Register:public view
{
    public:
    void process(char *buff,int fd);
};

class Onetogroud:public view
{
    public:
    void process(char *buff,int fd);
};

class Onetoone:public view
{
    public:
    void process(char *buff,int fd);
};

class Getlist:public view
{
    public:
    void process(char *buff,int fd);
};
