#include <sys/socket.h>
#include <netinet/ip.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#define KOTYA_PORT 8080
#define BUF_SIZE 1024
#define MY_SOCK_PATH "/somepath"
#define LISTEN_BACKLOG 50
#define handle_error(msg) \
	do { perror(msg); exit(EXIT_FAILURE); } while (0)
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
	
};
int parse_header(char *buf, struct header *header)
{
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
	return 0;
}
int main(int argc, char *argv[])
{
    int sfd, cfd;
    struct sockaddr_in my_addr, peer_addr;
    socklen_t peer_addr_size;
    char buf[BUF_SIZE];
    ssize_t r;
    ssize_t w;
        
    sfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sfd == -1)
		handle_error("socket");
    memset(&my_addr, 0, sizeof(struct sockaddr_in));    
    /* Очистка структуры */
    
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(KOTYA_PORT);
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
			struct header header1 = { .address = NULL, .version = NULL };
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
			printf("metod %d, address '%s', version '%s' \n", header1.method, header1.address, header1.version);
			if(header1.address) {
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
	}	
	if (shutdown(sfd, SHUT_RDWR) == -1)
		handle_error("shutdown_sfd");	
	if (close(sfd) == -1)
		handle_error("close_sfd");
}               

