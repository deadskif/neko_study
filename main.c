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
	char * address;
	char * version;
	
};
int parse_header(char * header_str, struct header * header)
{
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
		{ 	struct header header1;
			int i, j = 0, fst_space = 0, snd_space = 0, eol = 0;
			int method_parsed = 0;
			
			for (i = 0; buf[i] != '\n'; i++ )
			{	//printf("buf i %c, k= %d, i= %d, j=%d\n", buf[i], k, i,j);
				if ((buf[i] == ' ') && (!method_parsed))
				{
					if (strncmp(buf,"GET", i) == 0)
						header1.method = METHOD_GET;
					else if (strncmp(buf, "POST", i) == 0)
						header1.method = METHOD_POST;
					else
						header1.method = METHOD_UNKNOWN;
					fst_space = i;
					method_parsed++;
					//printf( "metod %d\n", header1.method);
				}	
				if ((buf[i] == ' ') && (j != i))
				{
					header1.address = (char *)malloc(i - j);
					header1.address[i - j] = '\0';
					for (j = fst_space+1; j<i; j++)
					{
						header1.address[j - fst_space - 1] = buf[j];
						//printf("%c", header1.address[n-j+1]);
					}
					printf("\n");
					snd_space = i;
				}
				eol = i;			
					
			//w = write(cfd, buf, r);
			//if (w == -1)
				//handle_error("write");
			//if (w < r)
				//printf("something strange\n");
			}
			header1.version = (char *)malloc(eol - snd_space + 1);
			header1.version[eol - snd_space +1] = '\0';
			//printf("[%d] = '\\0'\n", n - m +1);
			//printf("n=%d, m=%d\n", n, m);
			for (i = snd_space + 1; i <= eol; i++)
			{
				header1.version[i - snd_space - 1] = buf[i];
				//printf("[%d - %d + 1 = %d] = %c", i, m, i - m - 1, buf[i]);
			}
#if 0
			printf("[0]=%#x,", header1.version[0]);
			for (i = 0; header1.version[i] != '\0'; i++)
				printf("[%d]=%#x,", i, header1.version[i]);
			printf("\n");
#endif
			printf("metod %d, address '%s', version '%s' \n", header1.method, header1.address, header1.version);
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

