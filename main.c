#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "kotya_server.h"
#if 0
    char *method_start = NULL;
    char *address_start = NULL;
    char *version_start = NULL;
    
    while (*buf++ == ' ')
        ;
    method_start = buf - 1;
    while (*buf++ != ' ')
        ;
    *(buf - 1) = '\0';
    printf("start '%s' \n", method_start);
    if (strcmp(method_start,"GET") == 0)
        header->method = METHOD_GET; 
    else if (strcmp(method_start, "POST") == 0)
        header->method = METHOD_POST;
    else
        header->method = METHOD_UNKNOWN;
    
    while ( *buf++ == ' ')
        ;
    address_start = buf - 1;
    while (*buf++ != ' ')
        ;
    *(buf - 1) = '\0';
    header->address = strdup(address_start);
    while (*buf++ == ' ')
        ;
    version_start = buf - 1;
    while (*buf++ != '\n')
        ;
    *(buf - 1) = '\0';
    header->version = strdup(version_start);
#endif    

int main(int argc, char *argv[])
{
    int flag_n = 0, opt;
    int port = KOTYA_PORT;
    pid_t pid;
   
    while ((opt = getopt(argc, argv, "p:n")) != -1)
    {
        switch (opt)
        {
            case 'p':
                port = atoi(optarg);
                break;
            case 'n':
                flag_n = 1;
                break;
            default: /* '?' */
                fprintf(stderr, "Usage: %s [-p port]\n",
                       argv[0]);
                exit(EXIT_FAILURE);
        }
    }
    if (flag_n == 0)
    {
        pid = fork();
        if (pid == -1)
            handle_error("fork");
        else if (pid > 0)
            exit(0);
    }
    kotya_init(port); 
    kotya_main_loop();
    return 0;     
    /* Теперь мы можем принимать входящие соединения по одному
              с помощью accept(2) */
    
#if 0
            {
                printf("header1.version = %p, header1.address = %p\n", header1.version, header1.address);
                printf("address_start = %d, [] = %#x\n", address_start, buf[address_start]);
                printf("address_end = %d, [] = %#x\n", address_end, buf[address_end]);
                printf("address_diff = %d\n", address_end - address_start);
                int __i;
                for (__i = 0; header1.address[__i] != '\0'; __i++)
                    printf("[%d]=%#x('%c'),", __i, header1.address[__i], header1.address[__i]);
                printf("\n");
                
                printf("version_start = %d, [] = %#x\n", version_start, buf[version_start]);
                printf("version_end = %d, [] = %#x\n", version_end, buf[version_end]);
                printf("version_diff = %d\n", version_end - version_start);
                
                for (__i = 0; header1.version[__i] != '\0'; __i++)
                    printf("[%d]=%#x('%c'),", __i, header1.version[__i], header1.version[__i]);
                printf("\n");
            }
#endif

#if 0
            printf("[0]=%#x,", header1.version[0]);
            for (i = 0; header1.version[i] != '\0'; i++)
                printf("[%d]=%#x,", i, header1.version[i]);
            printf("\n");
#endif
}               

