#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
using namespace std;

void run(char *ip,unsigned short port);
void cli_cb(int fd,short event,void *arg);
void listen_cb(int fd,short event,void *arg);
