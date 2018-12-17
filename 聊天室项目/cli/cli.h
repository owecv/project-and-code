#include <iostream>
#include <json/json.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>//将十进制的字符串IP地址转换为二进制的数
#include <pthread.h>
#include <unistd.h>
using namespace std;

void Login(int fd);//登录
void login_success(int fd);
void run(int fd);
void Register(int fd);//注册
void Goaway(int fd);


