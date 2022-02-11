#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>


void print_diff_time(struct timeval start_time, struct timeval end_time) {
	struct timeval diff_time;
	if (end_time.tv_usec < start_time.tv_usec) {
		diff_time.tv_sec = end_time.tv_sec - start_time.tv_sec - 1;
		diff_time.tv_usec = end_time.tv_usec - start_time.tv_usec + 1000 * 1000;
	} else {
		diff_time.tv_sec = end_time.tv_sec - start_time.tv_sec;
		diff_time.tv_usec = end_time.tv_usec - start_time.tv_usec;
	}
	printf("time = %ld.%06d sec\n", diff_time.tv_sec, diff_time.tv_usec);
}
int main()
{
	int sock;
	struct sockaddr_in addr;
    struct timeval start_time, end_time;
	char buf[2048];
	char framebuf[1000000];
	sock = socket(AF_INET, SOCK_DGRAM, 0);

	addr.sin_family = AF_INET;
	addr.sin_port = htons(12345);
	addr.sin_addr.s_addr = INADDR_ANY;

	bind(sock, (struct sockaddr *)&addr, sizeof(addr));

	int flg = 1;
	uint8_t local_id = 0;
	uint32_t global_id = 0, size = 0;

	memset(buf, 0, sizeof(buf));
	memset(framebuf, 0, sizeof(framebuf));
	uint32_t *buf_32 = (uint32_t *)buf;
	uint32_t *framebuf_32 = (uint32_t *)framebuf;
    // フレームループ
    for(int k=0; k<2; k++){
        //1280*8*90ループ
        for(int j=0; j<90; j++){
            int bufcounter = 0;
            // 1280*8の画像ループ
            while(flg){
                int received = recv(sock, buf, sizeof(buf), 0) - 8;
                global_id = buf_32[0];
                size = buf_32[1] & 0x00FFFFFF;
                local_id = (buf_32[1] >> 24) & 0xFF;
                printf("received:%d global_id:%u size:%u local:%u bufcounter:%d\n", received, global_id, size, local_id, bufcounter);
                gettimeofday(&start_time, NULL);
                for(int i=0; i<received/4; i++){
                    framebuf_32[i+(bufcounter/4)] = htonl(buf_32[i+2]);
                }
                gettimeofday(&end_time, NULL);
                //memcpy(framebuf, buf+bufcounter+8, received);
                bufcounter += received;
                if(local_id==(size/1464)){
                    flg = 0;
                    printf("nuke\n");
                }
            }

            char moji[32];
            sprintf(moji, "binout/%03d.bin", global_id);
            FILE* fp = fopen(moji, "wb");
            int n = fwrite(framebuf, sizeof(char), bufcounter, fp);
            fclose(fp);
            flg = 1;
        }
    }

    print_diff_time(start_time, end_time);



	// uint32_t dma_addr = 0;
	// int address=0;
	// for(int i=0; i<cnt; i++){
	// 	char moji[32];
	// 	sprintf(moji, "binout/%03d.bin", i);
	// 	FILE* fp = fopen(moji, "wb");
	// 	for (int j = 0; j < addrbuf[i]; j = j + 4) { // 4byteやってる
	// 		unsigned char buffer[4];
	// 		buffer[0] = outbuf.buf[3 + j + address];
	// 		buffer[1] = outbuf.buf[2 + j + address];
	// 		buffer[2] = outbuf.buf[1 + j + address];
	// 		buffer[3] = outbuf.buf[0 + j + address];
	// 		int n = fwrite(buffer, sizeof(unsigned char), 4, fp);
	// 	}
	// 	printf("write:%d addrbuf:%u\n", i, addrbuf[i]);
	// 	address = address + addrbuf[i];
	// 	fclose(fp);
	// }

	close(sock);

	return 0;
}
