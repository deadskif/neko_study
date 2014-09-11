#include <sys/socket.h>
#include <netinet/ip.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <syslog.h>

#include "kotya_server.h"

int sfd, cfd, ffd;
struct sockaddr_in my_addr, peer_addr;
socklen_t peer_addr_size;
char buf[BUF_SIZE];
ssize_t r;
ssize_t w;



int is_suf(const char *str, const char *suf)
{
    int str_len = strlen(str);
    int str_suf = strlen(suf);
    return strcmp(str + str_len - str_suf, suf) == 0;
}    


struct list * list_add_end (struct list * ptr, const char * name, const char * value)
{
    if (ptr == NULL)
    {
        ptr = (struct list *)malloc(sizeof(struct list));
        if (ptr == NULL)
        {
            handle_error("malloc");
        }
        ptr->name = strdup(name);
        ptr->value = strdup(value);
        if ((ptr->name == NULL) || (ptr->value == NULL))
            handle_error("strdup");
        ptr->next = NULL;
    }
    else 
        ptr->next = list_add_end(ptr->next, name, value);
    return ptr;
}
void list_free (struct list * ptr)
{    
    if (ptr == NULL)
        return;    
    list_free (ptr->next);
    free(ptr->name);
    free(ptr->value);
    free(ptr);
}
void write_http_response (int cfd, struct http_response *response, struct header *header1)
{
    char buf[BUF_SIZE];
    int buf_len;
    int w, r;
    
    buf_len = sprintf(buf, "%s %s\r\nContent_Type: %s\r\nServer: %s\r\n", header1->version, response->status_str, response->content_type, response->server_info);
    buf_len += sprintf(buf + buf_len, "\r\n");
    if(response->body_str) {
        buf_len += sprintf(buf + buf_len, "%s", response->body_str);
    }
    if((w = write(cfd, buf, buf_len)) < buf_len)
        handle_error("write");
    if(response->filed >= 0) {
        int ffd = response->filed;                
        while((r = read(ffd, buf, BUF_SIZE)) > 0)
        { 
            if ((w = write(cfd, buf, r)) == -1)
                handle_error("write_file");
        }
        close(ffd);
    }
}
char *get_mimetype_by_name(const char *name)
{
    char *mime_type;
    char *ptr;
    char buf[BUF_SIZE];
    sprintf(buf, "mimetype '%s'",name);
    FILE *fp = popen(buf,"re");
    if (fp == NULL)
        handle_error("popen");
    fgets(buf, BUF_SIZE, fp);
    buf[strlen(buf)-1] = '\0';
    //printf("fgets(): '%s'\n", buf);
    //ssize_t r = getline(&buf, 
    if ((ptr = strchr(buf,':')) == NULL)
        handle_error("mimetype_strchr");
    if ((mime_type = strdup(ptr + 2)) == NULL)
        handle_error("strdup_mimetype");
    pclose(fp);
    return mime_type;
   
}

int parse_header(char *buf, struct header *header)
{
    char *method;
    char *address;
    char *version;
    method = strtok(buf, " ");
    if (strcmp(method,"GET") == 0)
        header->method = METHOD_GET; 
    else if (strcmp(method, "POST") == 0)
        header->method = METHOD_POST;
    else
        header->method = METHOD_UNKNOWN;
    
    address = strtok(NULL, " ");
    if ((header->address = strdup(address)) == NULL)
        handle_error("strdup");
    version = strtok(NULL, " \r\n");
    if ((header->version = strdup(version)) == NULL)
        handle_error("strdup");
    for(;;)
    {
        char *name;
        char *value;
        name = strtok(NULL, ": \r\n");
        value = strtok(NULL, "\r\n");
        if (value && (*value == ' '))
            value++;
        //printf("name '%s', value '%s'\n", name, value);
        if ((name == NULL) || (value == NULL))
            break;
        header->headers = list_add_end( header->headers, name, value);
    }
    return 0;
        
}

int kotya_init(int port)
{
    sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd == -1)
        handle_error("socket");
    memset(&my_addr, 0, sizeof(struct sockaddr_in));    
    /* Очистка структуры */
    
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(port);
    my_addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(sfd, (struct sockaddr *) &my_addr,
            sizeof(struct sockaddr_in)) == -1)
        handle_error("bind");
    if (listen(sfd, LISTEN_BACKLOG) == -1)
        handle_error("listen");

}
int kotya_main_loop()
{
    for (;;)
    {
        peer_addr_size = sizeof(struct sockaddr_in);
        cfd = accept(sfd, (struct sockaddr *) &peer_addr,
                        &peer_addr_size);
        if (cfd == -1)
            handle_error("accept");

           /* Код обработки входящего соединения(й)...*/
        r = read(cfd, buf, BUF_SIZE);
        if (r == -1)
            handle_error("read");
        if (r > 0) 
        { 
            struct http_response response = RESPONSE_INIT;
            struct header header1 = HEADER_INIT;
            struct list *i;
            parse_header(buf, &header1);
            syslog(LOG_DEBUG,"metod %d, address '%s', version '%s' \n", header1.method, header1.address, header1.version);
            for (i = header1.headers; i != NULL; i = i->next)
                syslog(LOG_DEBUG,"name '%s' , value '%s' \n", i->name, i->value);
            if (header1.method == METHOD_GET)
            {
                char* address;    
                int header_len;
                response.file_name = strtok(header1.address,"?/");
                if(response.file_name == NULL)
                    response.file_name = "index.html"; 
                syslog(LOG_DEBUG,"'%s'\n",response.file_name );
                response.status_str = "200 OK";
                response.content_type = get_mimetype_by_name(response.file_name);
                if ((ffd = open(response.file_name, O_RDONLY)) < 0)
                {
                    response.status_str = "404 Not Found";
                    response.body_str = "<html><head><title>Error 404</title></head><body><p>404 Not Found</p></body></html>";
                }
                response.filed = ffd;
            }
            else 
            {
                response.status_str = "400 Bad Request";
                response.body_str = "<html><head><title>Error 400</title></head><body><p>400 Bad Request</p></body></html>";
            }
            write_http_response(cfd, &response, &header1);
            if(header1.address)
            {
                free(header1.address);
                header1.address = NULL;
            }
            if(header1.version) {
                free(header1.version);
                header1.version = NULL;
            }
        }    
        if (shutdown(cfd, SHUT_RDWR) == -1)
            handle_error("shutdown_cfd");
        if (close(cfd) == -1)
            handle_error("close_cfd");
    } /* for(;;) */
    if (shutdown(sfd, SHUT_RDWR) == -1)
        handle_error("shutdown_sfd");    
    if (close(sfd) == -1)
        handle_error("close_sfd");

}
