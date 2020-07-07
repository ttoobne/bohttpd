/**
 * @author ttoobne
 * @date 2020/7/3
 */

#include "debug.h"
#include "rio.h"

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUF_SIZE 8192
void error_handling(char *message);

int main(int argc, char *argv[])
{
	int sock;
	char message[BUF_SIZE];
	int str_len;
    size_t len;
	struct sockaddr_in serv_adr;

	if(argc!=3) {
		printf("Usage : %s <IP> <port>\n", argv[0]);
		exit(1);
	}
	
	sock=socket(PF_INET, SOCK_STREAM, 0);   
	if(sock==-1)
		error_handling("socket() error");
	
	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family=AF_INET;
	serv_adr.sin_addr.s_addr=inet_addr(argv[1]);
	serv_adr.sin_port=htons(atoi(argv[2]));
	
	if(connect(sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr))==-1)
		error_handling("connect() error!");
	else
		puts("Connected...........");
	
    sprintf(message, "GET /index.html HTTP/1.1\r\n");
    sprintf(message, "%sHost: ttoobne.com\r\n", message);
    sprintf(message, "%sConnection: keep-alive\r\n", message);
    sprintf(message, "%sCache-Control: max-age=0\r\n", message);
    sprintf(message, "%sUpgrade-Insecure-Requests: 1\r\n", message);
    sprintf(message, "%sUser-Agent: Mozilla/5.0 (Linux; Android 6.0; Nexus 5 Build/MRA58N) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/83.0.4103.116 Mobile Safari/537.36\r\n", message);
    sprintf(message, "%sAccept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.9\r\n", message);
    sprintf(message, "%sAccept-Encoding: gzip, deflate\r\n", message);
    sprintf(message, "%sAccept-Language: zh-CN,zh;q=0.9\r\n", message);
    sprintf(message, "%sIf-None-Match: \"5ed21299-1c6e\"\r\n", message);
    len = sprintf(message, "%sIf-Modified-Since: Sat, 30 May 2020 08:00:25 GMT\r\n\r\n", message);

	//while(1) 
	//{
		str_len = rio_writen(sock, message, len);
        if (str_len < 0) {
            ERROR("write error.");
            return 1;
        }
        DBG("write len: %d\n", len);

        len = BUF_SIZE - 1;
		str_len = rio_readn(sock, &message[0], &len);
        if (str_len < 0) {
            ERROR("read error");
            return 1;
        }

        DBG("read len: %d\n", str_len);

		message[str_len] = 0;
		printf("Message from server:\n%s", message);
	//}
	
	close(sock);
	return 0;
}

void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}