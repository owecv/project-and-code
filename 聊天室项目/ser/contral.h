#include "login.h"
#include <iostream>
#include <map>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define TYPE_LOGIN 0    //登录
#define TYPE_REG 1      //注册
#define TYPE_ONE 2      //一对一聊天
#define TYPE_GROUD 3    //群聊
#define TYPE_LIST 4     //显示在线列表

using namespace std;

class contral           //对传过来的指令进行解析，调用相应的函数进行处理
{
    private:
    map<int,view*> _model;
    public:
    contral();
    ~contral();
    void sysdata(char *buff,int fd);
};
