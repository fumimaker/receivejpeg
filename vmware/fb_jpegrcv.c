#include <jpeglib.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define UDP_SIZE 1472
#define UDP_HEADER 8
#define UDP_DATASIZE (UDP_SIZE - UDP_HEADER)
#define HEIGHT 720
#define WIDTH 1280
#define DEPTH 3
#define headername "headerout.bin"
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
int OpenFrameBuffer(int fd) {
    fd = open(DEVICE_NAME, O_RDWR);
    if (!fd) {
        fprintf(stderr, "cannot open the FrameBuffer '%s'\n", DEVICE_NAME);
        exit(1);
    }
    return fd;
}
int main() {
    int sock;
    struct sockaddr_in addr;
    struct timeval start_time, end_time;
    char buf[2048];
    char framebuf[1000000];  // 1Mbyte
    sock = socket(AF_INET, SOCK_DGRAM, 0);

    struct jpeg_decompress_struct in_info;
    struct jpeg_error_mgr jpeg_error;
    JSAMPROW buffer = NULL;
    JSAMPROW row;
    struct stat sb;

    int address_counter = 0;
    int sizeofbin, sizeofheader;
    unsigned char binbuffer[0x100000];  // 1MByte
    unsigned char headerbuffer[1024];
    unsigned char mem[0x100000];
    unsigned char eof[2] = {0xFF, 0xD9};
    unsigned char writebuffer[0x2A3000];  // 2MByte

    in_info.err = jpeg_std_error(&jpeg_error);

    FILE *fp_header = fopen(headername, "rb");
    if (fp_header == NULL) {
        printf("no header file.\n");
        return -1;
    }
    if (stat(headername, &sb) == -1) {
        perror("stat");
        exit(EXIT_FAILURE);
    }
    sizeofheader = sb.st_size;
    int temp =
        fread(headerbuffer, sizeof(unsigned char), sizeofheader, fp_header);
    printf("sizeofheader:%d bytes\n", sizeofheader);
    fclose(fp_header);

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
    int wari = 0, amari = 0;
    // フレームループ
    for (int k = 0; k < 30; k++) {
        // 1280*8*90 Loop
        address_counter = 0;
        for (int j = 0; j < 90; j++) {
            int bufcounter = 0;
            // 1280*8の画像ループ
            while (flg) {
                int received = recv(sock, buf, sizeof(buf), 0) - 8;
                global_id = buf_32[0];
                size = buf_32[1] & 0x00FFFFFF;
                local_id = (buf_32[1] >> 24) & 0xFF;
                // printf("received:%d global_id:%u size:%u local:%u
                // bufcounter:%d\n", received, global_id, size, local_id,
                // bufcounter);
                wari = size / UDP_DATASIZE;
                amari = size - (wari * UDP_DATASIZE);  // amari = size%UDP_DATA;

                for (int i = 0; i < received / 4; i++) {
                    framebuf_32[i + (bufcounter / 4)] = htonl(buf_32[i + 2]);
                }
                // memcpy(framebuf, buf+bufcounter+8, received);
                bufcounter += received;
                if (local_id == wari) {
                    flg = 0;
                    // printf("nuke\n");
                }
                if ((amari == 0) && (local_id == (wari - 1))) {
                    flg = 0;
                }
            }
            gettimeofday(&start_time, NULL);
            flg = 1;
            // headerを書く
            memcpy(mem, headerbuffer, sizeofheader);
            //内容を書く
            memcpy(mem + sizeofheader, framebuf, bufcounter);
            // EOFマーカーを書く
            memcpy(mem + sizeofheader + bufcounter, eof, 2);

            jpeg_create_decompress(&in_info);
            jpeg_mem_src(&in_info, mem, sizeofheader + bufcounter + 2);

            jpeg_read_header(&in_info, TRUE);
            jpeg_start_decompress(&in_info);

            // printf("in_info.output_height:%d in_info.output_components:%d\n",
            // in_info.output_height, in_info.output_components);

            int stride = sizeof(JSAMPLE) * in_info.output_width *
                         in_info.output_components;

            if ((buffer = (unsigned char *)calloc(stride, 1)) == NULL) {
                perror("calloc error");
            }

            unsigned char img[WIDTH * HEIGHT * DEPTH];

            for (int i = 0; i < in_info.output_height; i++) {
                jpeg_read_scanlines(&in_info, &buffer, 1);
                row = buffer;
                for (int k = 0; k < stride; k++) {
                    img[k + i * stride] = *row++;
                }
            }
            jpeg_finish_decompress(&in_info);
            jpeg_destroy_decompress(&in_info);
            free(buffer);
            memcpy(writebuffer + address_counter, img,
                   in_info.output_height * stride);
            address_counter += in_info.output_height * stride;
            gettimeofday(&end_time, NULL);
            printf("addr:%d\n", address_counter);
        }
        char moji[32];
        sprintf(moji, "rawout/udp%04d.raw", k);
        FILE *file = fopen(moji, "wb");
        if (file == NULL) {
            printf("no file.\n");
            return -1;
        }
        fwrite(writebuffer, 1, 1280 * 720 * 3, file);
        fclose(file);
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
