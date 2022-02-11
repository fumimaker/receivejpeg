#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>

#define UDP_SIZE    1472
#define UDP_HEADER  8
#define UDP_DATASIZE    (UDP_SIZE - UDP_HEADER)


void print_diff_time(struct timeval start_time, struct timeval end_time) {
	struct timeval diff_time;
	if (end_time.tv_usec < start_time.tv_usec) {
		diff_time.tv_sec = end_time.tv_sec - start_time.tv_sec - 1;
		diff_time.tv_usec = end_time.tv_usec - start_time.tv_usec + 1000 * 1000;
	} else {
		diff_time.tv_sec = end_time.tv_sec - start_time.tv_sec;
		diff_time.tv_usec = end_time.tv_usec - start_time.tv_usec;
	}
	printf("time = %ld.%06ld sec\n", diff_time.tv_sec, diff_time.tv_usec);
}
int main()
{
	int sock;
	struct sockaddr_in addr;
    struct timeval start_time, end_time;
	char buf[2048];
	char framebuf[1000000];//1Mbyte
	sock = socket(AF_INET, SOCK_DGRAM, 0);

	addr.sin_family = AF_INET;
	addr.sin_port = htons(12345);
	addr.sin_addr.s_addr = INADDR_ANY;

	bind(sock, (struct sockaddr *)&addr, sizeof(addr));

	int flg = 1;
	uint8_t local_id = 0;
	uint32_t global_id = 0, size = 0;
    uint32_t global_counter = 0;
    memset(buf, 0, sizeof(buf));
    memset(framebuf, 0, sizeof(framebuf));
	uint32_t *buf_32 = (uint32_t *)buf;
	uint32_t *framebuf_32 = (uint32_t *)framebuf;
    // フレームループ
    int wari=0, amari=0;
    for(int k=0; k<5; k++){
        //1280*8*90ループ
        for(int j=0; j<90; j++){
            int bufcounter = 0;
            // 1280*8の画像ループ
            int broken = 0, internal_counter = 0;
            while (flg) {
                int received = recv(sock, buf, sizeof(buf), 0) - 8;
                global_id = buf_32[0];
                size = buf_32[1] & 0x00FFFFFF;
                local_id = (buf_32[1] >> 24) & 0xFF;
                printf("received:%d global_id:%u size:%u local:%u bufcounter:%d intcnt:%d\n", received, global_id, size, local_id, bufcounter, internal_counter);
                wari = size / UDP_DATASIZE;
                amari = size - (wari * UDP_DATASIZE);  // amari = size%UDP_DATA;
                if (local_id < wari) {  // 1464 //足りなかったら壊れてる
                    broken = received != UDP_DATASIZE ? 1 : 0;
                } else {
                    broken = received != amari ? 1 : 0;
                }

                if(local_id!=internal_counter){
                    // 内部カウンタとlocal_idが違ったらパケット不着なので壊れてる
                    broken = 1;
                    printf("broken: global_id=%d local_id=%d internal=%d\n", global_id, local_id, internal_counter);
                }

                for (int i = 0; i < received / 4; i++) {
                    framebuf_32[i + (bufcounter / 4)] = htonl(buf_32[i + 2]);
                }
                if(broken){
                    flg = 0;
                }
                if (internal_counter>=wari) {//amariまで完了
                    flg = 0;
                }
                if (amari==0 && (internal_counter==(wari-1))){
                    // amariが0だったとき、wari-1まで終わったらOK
                    flg = 0;
                }
                bufcounter += received;
                internal_counter++;
            }

            if (global_counter != global_id) {

            }

            if(!broken){
                char moji[32];
                sprintf(moji, "binout/%04d.bin", global_id);
                FILE *fp = fopen(moji, "wb");
                int n = fwrite(framebuf, sizeof(char), bufcounter, fp);
                fclose(fp);
            } else {

            }
            flg = 1;
            global_counter++;
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
