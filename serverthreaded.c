#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>
#include <netinet/in.h>
#include <ctype.h>

//#define dev = 0
#define SERVERNAME "Server: felixserver\r\n"

void *connection_handler(void *);
int get_line(int, char *, int);
char *file;
pthread_mutex_t lock;

int main(int argc, char *argv[]) {
    //char client_message[2000];

    int portn = 8088;
    //create socket
    int socket_desc, new_socket;
    struct sockaddr_in server;

    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_desc == -1) {
        printf("Could not create socket");
    }
    //prepare sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons( portn );

    //bind
    if ( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0) {
        perror("Bind failed");
        return 1;
    }
    //bool cont = true;
    int c, *new_sock;
    struct sockaddr_in client;
    puts("Waiting for connections\n");
    listen(socket_desc, 3);
    c = sizeof(struct sockaddr_in);

    while ( (new_socket = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c))) {
        puts("Connected\n");
        pthread_t t;
        new_sock = malloc(1);
        *new_sock = new_socket;

        if( pthread_create( &t, NULL, connection_handler, (void*)new_sock) < 0) {
            perror("could not create thread");
        }

        puts("assigned thread");
    }
    if (new_socket<0) {
        perror("Accept failed");
    }
    puts("Closing file\n");
    //free(socket_desc);
    return 0;
}


//Takes in socket, a character pointer and a byte size
//Then reads from the socket into the buffer
//Returns the size of the read
int get_line(int socket, char *buffer, int size) {
    int l = 0;
    char p = '\0';
    int m;

    while ( (l < size - 1) && (p != '\n') ) {
        m = recv(socket,&p, 1,0);
        if ( m > 0) {
            if (p == '\r') {
                m = recv(socket, &p,1, MSG_PEEK);

                if ( (m > 0) && (p == '\n') ) {
                    recv(socket, &p, 1, 0);
                } else {
                    p = '\n';
                }
            }
            buffer[l] = p;
            l++;
        } else {
            p = '\n';
        }
    }
    buffer[l] = '\0';

    return(l);
}

void *connection_handler(void *socket_desc) {
    //Get the socket
    int sock = *(int*)socket_desc;
    char buf[1024]; //Create a buffer for the request
    //Now read the first line
    get_line(sock, buf, sizeof(buf));
	size_t k = 0, l = 0;
	char method[255];
	while (!isspace((int)buf[k]) && (l < sizeof(method) -1) ) {
        method[l] = buf[k];
        k++;
        l++;
	}
	//Make sure the last character is a null
	method[l] = '\0';

	//Now check what method we've recieved
    //If it's not a valid method then return and error
	if (strcasecmp(method, "GET") && strcasecmp(method, "POST") ) {
        perror("Method not GET or POST");
        return 0;
	} else {

	//If it's a valid method get the url from the message
	//First clear any whitespace in the buffer
	while (isspace((int) buf[k]) && (k < sizeof(buf))) {
        k++;
	}
	//Now we read from the buffer into the url buffer
	l = 0;
	char url[255];
	while (!isspace((int)buf[k]) && (l < sizeof(url) - 1) && (k < sizeof(buf)) ) {
        url[l] = buf[k];
        l++;k++;
	}
	//Make sure the last character is a null
	url[l] = '\0';

	//If the url begins with a / then remove it
	if (url[0] == '/')
        memmove(url, url+1, strlen(url));
	printf("url: %s\n", url);
	char content[256];
    //If the url ends with a / add index.html to the end
	if (url[strlen(url) - 1] == '/') {
        strcat(url, "index.html");
        sprintf(content, "Content-Type: text/html\r\n");
	} else {
        //Check what kind of file our path is to
        char copy[256];
        const char *o = ".";
        strcpy(copy, url);
        char *token = strtok(copy, o);
        token = strtok(NULL, o);
        if (strcasecmp(token, "jpg") == 0 || strcasecmp(token, "jpeg") == 0) {
            sprintf(content, "Content-Type: image/jpeg\r\n");
        } else
        if (strcasecmp(token, "gif") == 0) {
            sprintf(content, "Content-Type: image/gif\r\n");
        } else
        if (strcasecmp(token, "html") == 0 ) {
            sprintf(content, "Content-Type: text/html\r\n");
        } else
        if (strcasecmp(token, "txt") == 0 ) {
            sprintf(content, "Content-Type: text/enriched\r\n");
        } else {
            sprintf(content, "Content-Type: text/enriched\r\n");
        }
    }

    //Aquire the lock and open the file
    pthread_mutex_lock(&lock);
    FILE *f = fopen(url,"r");
    char headers[1024];
    if (f == NULL) {
        perror("Error opening file");
        sprintf(headers, "HTTP/1.1 404 NOT FOUND\r\n");
        send(sock, &headers, strlen(headers), 0);
        sprintf(headers, SERVERNAME);
        send(sock, &headers, strlen(headers), 0);
        sprintf(headers, "Content-Type: text/html\r\n");
        send(sock, &headers, strlen(headers), 0);
        char *htmlnotfound = "<HTML><TITLE>Not Found<TITLE><BODY> <P> The server could not fulfill your request because the resource specified does not exist</P></BODY></HTML>\r\n";
        sprintf(headers, "Content-Length: %d\r\n", (int)sizeof(htmlnotfound));
        send(sock, &headers, strlen(headers), 0);
        sprintf(headers, "\r\n");
        send(sock, &headers, strlen(headers), 0);
        sprintf(headers, "\r\n");
        send(sock, &headers, strlen(headers), 0);
        sprintf(headers, "%s", htmlnotfound);
        send(sock, &headers, strlen(headers), 0);
    } else {
        //Send the headers
        sprintf(headers, "HTTP/1.1 200 OK\r\n");
        send(sock, &headers, strlen(headers), 0);
        sprintf(headers, SERVERNAME);
        send(sock, &headers, strlen(headers), 0);
		sprintf(headers, "%s", content);
        send(sock, &headers, strlen(headers), 0);
	//Get the size of the file
	fseek(f, 0L, SEEK_END);
	size_t sz = ftell(f);
	fseek(f, 0L, SEEK_SET);
		printf("Size of file: %d\r\n", (int)sz);
		sprintf(headers, "Content-Length: %d\r\n", (int)sz);
		send(sock, &headers, strlen(headers), 0);
		sprintf(headers, "Connection: close\r\n");
        send(sock, &headers, strlen(headers), 0);
        sprintf(headers, "\r\n");
        send(sock, &headers, strlen(headers), 0);
        char buf[sz];
        //printf("%d\n", (int)sizeof(buf));
        //Send the file
        fread(buf, sz, 1, f);
        ssize_t n;
        const char *p = buf;
        printf("sending file\n");
        while (sz > 0) {
        	n = send(sock, p, sz, 0);
        	printf("Sent: %d\n",(int)n);
        	if (n <= 0) break;
        	p += n;
        	sz -= n;
        }
        //send(sock, buf, sizeof(buf), 0);
        
    }
    fclose(f);
    pthread_mutex_unlock(&lock);
    free(socket_desc);
    return 0;
    }
}
