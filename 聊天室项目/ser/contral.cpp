#include "head.h"
#include <map>
#include "contral.h"
using namespace std;

contral::~contral()
{

}

contral::contral()
{
    _model.insert(make_pair(TYPE_LOGIN,new Login_view()));
    
    _model.insert(make_pair(TYPE_REG,new Register()));
    
    _model.insert(make_pair(TYPE_GROUD,new Onetogroud()));
    
    _model.insert(make_pair(TYPE_ONE,new Onetoone()));
    
    _model.insert(make_pair(TYPE_LIST,new Getlist()));
}

void contral::sysdata(char *buff,int fd)
{
    assert(buff!=NULL);

    char recvbuff[128]="";
    strcpy(recvbuff,buff);
    Json::Value val;
    Json::Reader read;
    if(-1==read.parse(recvbuff,val))
    {
        cout<<"json parse error"<<endl;
        return;
    }
    
    char  *p=(char *)(val["type"].toStyledString().c_str());
    int TYPE=atoi(p);
    cout<<"p="<<p<<endl;

    if(!_model[TYPE])
    {
        send(fd,"no server",9,0);
        return;
    }

   switch(TYPE)
   {
       case TYPE_LOGIN:
       {
           cout<<"login ok"<<endl;
           _model[TYPE_LOGIN]->process(buff,fd); 
       }break;
       case TYPE_REG:
       {
            cout<<"reg ok"<<endl;
           _model[TYPE_REG]->process(buff,fd);
       }break;
       case TYPE_ONE:
       {
            cout<<"one to one"<<endl;
            _model[TYPE_ONE]->process(buff,fd);
       }break;
       case TYPE_GROUD:
       {
            cout<<"one to groud"<<endl;
           _model[TYPE_GROUD]->process(buff,fd);

       }break;
       case TYPE_LIST:
       {
            cout<<"get list"<<endl;
           _model[TYPE_LIST]->process(buff,fd);
       }break;
   }
}

