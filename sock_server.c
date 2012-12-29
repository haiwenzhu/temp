#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <string.h>
#include <event2/event.h>

#define BUFSIZE 1024
#define DEBUG(msg) printf("[DEBUG]%s\n", (msg))

static int err = 0;

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

/*event to monitor stdin*/
void event_ready_stdin(evutil_socket_t fd, short what, void *arg)
{
	DEBUG("stdin is ready for read");
	char buf[BUFSIZE];
	if (fgets(buf, BUFSIZE, stdin) != NULL) {
		send(*(int*)arg, buf, strlen(buf) + 1, 0);
	}
	else {
		err = -1;
	}
}

/*event to monitor socket*/
void event_ready_socket(evutil_socket_t fd, short what, void *arg)
{
	DEBUG("socket is ready for read");
	char buf[BUFSIZE];
	if (recv(fd, buf, BUFSIZE, 0) > 0) {
		fputs(buf, stdout);
	}
	else {
		err = -1;
	}
}

/*event to monitor if error occure*/
void event_check_err(evutil_socket_t fd, short what, void *arg)
{
	DEBUG("check error...");
	if (err < 0) {
		DEBUG("error occur!");
		event_base_loopexit((struct event_base*)arg, NULL);
	}
}

void run(const char *addr, const uint16_t port)
{
	struct sockaddr_in sin;
	int sockfd;
	int clientfd;
	char buf[1024];

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		perror("call socket failed");
	}

	if (get_socket_addr(addr, port, (struct sockaddr*)&sin) < 0) {
		perror("call get_socket_addr failed");
	}

	if (bind(sockfd, (struct sockaddr*)&sin, sizeof(sin)) < 0) {
		perror("call bind failed");
	}

	if (listen(sockfd, 5) < 0) {
		perror("call listen failed");
	}

	if ((clientfd = accept(sockfd, NULL, NULL)) < 0) {
		perror("call accept failed");
	}
	
	/*
	while (1) {
		if (fgets(buf, 1024, stdin) == NULL) {
			perror("call fgets error");
			break;
		}
		else {
			size_t sended = send(clientfd, buf, strlen(buf) + 1, 0);
			if (sended < 0) {
				perror("call send failed");
				break;
			}
		}

		size_t recved = recv(clientfd, buf, 1024, 0);
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
	*/
	struct event *ev_stdin, *ev_socket, *ev_check;
	struct event_base *base = event_base_new();
	struct timeval sec5 = {10, 0};

	ev_stdin = event_new(base, STDIN_FILENO, EV_READ|EV_PERSIST, event_ready_stdin, (int*)&clientfd);
	ev_socket = event_new(base, clientfd, EV_READ|EV_PERSIST, event_ready_socket, NULL);
	ev_check = event_new(base, -1, EV_TIMEOUT|EV_PERSIST, event_check_err, (void*)base);
	
	event_add(ev_stdin, NULL);
	event_add(ev_socket, NULL);
	event_add(ev_check, &sec5);
	event_base_dispatch(base);

	close(clientfd);
	close(sockfd);
}

int main()
{
	const char *addr = "127.0.0.1";
	const uint16_t port = 12345;
	run(addr, port);
}
