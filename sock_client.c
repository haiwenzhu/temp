#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

int get_socket_addr(const char *addr, const uint16_t port, struct sockaddr *sa)
{
	struct sockaddr_in *sin = (struct sockaddr_in*)sa;
	sin->sin_family = AF_INET;
	sin->sin_port = htons(port);
	if (inet_pton(AF_INET, addr, &sin->sin_addr) > 0) {
		return 1;
	}
	
	return 0;
}

void conn(const char *addr, const uint16_t port)
{
	struct sockaddr_in sin;
	int sockfd;
	char buf[1204];

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		perror("call socket failed");
	}

	if (get_socket_addr(addr, port, (struct sockaddr*)&sin) < 0) {
		perror("call get_socket_addr failed");
	}

	if (connect(sockfd, (struct sockaddr*)&sin, sizeof(sin)) < 0) {
		perror("call connect failed");
	}

	while (1) {
		if (fgets(buf, 1024, stdin) == NULL) {
			perror("call fgets error");
			break;
		}   
		else {
			size_t sended = send(sockfd, buf, strlen(buf) + 1, 0); 
			if (sended < 0) {
				perror("call send failed");
				break;
			}   
		}   

		size_t recved = recv(sockfd, buf, 1024, 0); 
		if (recved < 0) {
			perror("call recv failed");
			break;
		}   
		else {
			if (fputs(buf, stdout) == EOF) {
				perror("call fputs failed");
				break;
			}   
		}   
	}   

	close(sockfd);
}

int main()
{
	const char *addr = "127.0.0.1";
	const uint16_t port = 12345;
	conn(addr, port);
}
