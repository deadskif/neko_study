#ifndef KOTYA_SERVER_H_
#define KOTYA_SERVER_H_

#define KOTYA_PORT 8080;
#define BUF_SIZE 1024
#define MY_SOCK_PATH "/somepath"
#define LISTEN_BACKLOG 50

#define handle_error(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)

#define RESPONSE_INIT { \
    .status_str = NULL, \
    .content_type = NULL, \
    .server_info = "kotya_server", \
    .head = NULL, \
    .file_name = NULL, \
    .filed = -1, \
    .body_str = NULL \
}
#define HEADER_INIT { \
    .method = METHOD_UNKNOWN, \
    .address = NULL, \
    .version = NULL, \
    .headers = NULL \
}

enum http_method 
{
    METHOD_GET,
    METHOD_POST,
    METHOD_UNKNOWN = -1
};
struct list 
{
    char * name;
    char * value;
    struct list * next;
};
struct header 
{
    enum http_method method;
    char *address;
    char *version;
    struct list *headers;
    
};
struct http_response
{
    /*Header*/
    char *status_str;
    char *content_type;
    char *server_info;
    struct list *head;
    /*Body*/
    char *file_name;
    int filed;
    char *body_str;
};

int kotya_init(int port);
int kotya_main_loop();
#endif /* KOTYA_SERVER_H_ */

