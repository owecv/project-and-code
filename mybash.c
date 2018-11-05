#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#define CIN_BUFFER_SIZE 128

int main(int argc,char *argv[],char *envp[])
{
    while(1)
    {
        char cin_buffer[CIN_BUFFER_SIZE]={'0'};
        printf("[stu@localhost ~]$");
        fflush(stdout);
        fgets(cin_buffer,128,stdin);
        int n=strncmp(cin_buffer,"exit",4);
        if(0==strncmp(cin_buffer,"exit",4))
        {
            exit(0);
        }
        if(0==strncmp(strtok(cin_buffer," "),"ls",2))
        {
            pid_t pid=fork();
            if(pid==0)
            {
                execl("/bin/ls","ls","-l",NULL);
            }
            if (pid>0)
            {
                wait(NULL);
            }
        }
        if(0==strncmp(strtok(cin_buffer," "),"pwd",2))
        {
            pid_t pid=fork();
            if(pid==0)
            {
                execl("/bin/pwd","pwd",NULL,NULL);
            }
            if (pid>0)
            {
                wait(NULL);
            }
        }
        if(0==strncmp(strtok(cin_buffer," "),"ps",2))
        {
            pid_t pid=fork();
            if(pid==0)
            {
                execl("/bin/ps","ps",NULL,NULL);
            }
            if (pid>0)
            {
                wait(NULL);
            }
        }
        if(0==strncmp(strtok(cin_buffer," "),"mkdir",2))
        {
            pid_t pid=fork();
            char buffer[128]={0};
            strcpy(buffer,strtok(cin_buffer," "));
            if(pid==0)
            {
                execl("/bin/mkdir","mkdir",buffer,NULL);
            }
            if (pid>0)
            {
                wait(NULL);
            }
        }
    }
    return 0;
}
