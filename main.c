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
#define KOTYA_PORT 8080
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
enum http_method 
{
	METHOD_GET,
	METHOD_POST,
	METHOD_UNKNOWN = -1
};
int is_suf(const char *str, const char *suf)
{
	int str_len = strlen(str);
	int str_suf = strlen(suf);
	return strcmp(str + str_len - str_suf, suf) == 0;
}	
struct list 
{
	char * name;
	char * value;
	struct list * next;
};

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

#define HEADER_INIT { \
	.method = METHOD_UNKNOWN, \
	.address = NULL, \
	.version = NULL, \
	.headers = NULL \
}
int parse_header(char *buf, struct header *header)
{
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
int main(int argc, char *argv[])
{
    int sfd, cfd, ffd;
    struct sockaddr_in my_addr, peer_addr;
    socklen_t peer_addr_size;
    char buf[BUF_SIZE];
    ssize_t r;
    ssize_t w;
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
		
	/* Теперь мы можем принимать входящие соединения по одному
              с помощью accept(2) */
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

