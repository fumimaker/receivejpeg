#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
#include <arpa/inet.h>

#define num 1472

int main() {
	int sock;
	struct sockaddr_in addr;
	sock = socket(AF_INET, SOCK_DGRAM, 0);

	addr.sin_family = AF_INET;
	addr.sin_port = htons(12345);
	addr.sin_addr.s_addr = inet_addr("203.178.128.70");
	char buf[num];
	for (int i = 0; i<num; i++){
		buf[i] = 0xFF&i;
	}
	sendto(sock, buf, num, 0, (struct sockaddr *)&addr, sizeof(addr));

	close(sock);

	return 0;
}