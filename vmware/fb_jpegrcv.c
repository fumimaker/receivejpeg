
#include <fcntl.h>
#include <linux/fb.h>
#include <linux/fs.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <jpeglib.h>
#include <setjmp.h>

#define UDP_SIZE 1472
#define UDP_HEADER 8
#define UDP_DATASIZE (UDP_SIZE - UDP_HEADER)
#define HEIGHT 720
#define WIDTH 1280
#define DEPTH 3
#define headername "headerout.bin"
#define DEVICE_NAME "/dev/fb0"

typedef struct my_error_mgr {
    struct jpeg_error_mgr jerr;
    jmp_buf jmpbuf;
} my_error_mgr;

static void error_exit(j_common_ptr cinfo) {
    my_error_mgr *err = (my_error_mgr *)cinfo->err;
    (*cinfo->err->output_message)(cinfo);
    longjmp(err->jmpbuf, 1);
}

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
    char udpbuffer[2048];
    sock = socket(AF_INET, SOCK_DGRAM, 0);

    struct jpeg_decompress_struct in_info;
    // struct jpeg_error_mgr jpeg_error;
    JSAMPROW rowbuffer = NULL;
    JSAMPROW row;
    struct stat sb;
    my_error_mgr myerr;
    in_info.err = jpeg_std_error(&myerr.jerr);
    myerr.jerr.error_exit = error_exit;
    if (setjmp(myerr.jmpbuf)) {
        goto error;
    }
    int address_counter = 0;
    int sizeofbin, sizeofheader;
    unsigned char binbuffer[0x100000];  // 1MByte
    unsigned char headerbuffer[1024];
    unsigned char mem[0x100000];
    unsigned char eof[2] = {0xFF, 0xD9};
    unsigned char writebuffer[0x2A3000];  // 2MByte

    int fd = 0;
    int screensize;
    int x, y, col;
    int r, g, b;
    fd = OpenFrameBuffer(fd);
    struct fb_var_screeninfo vinfo;
    struct fb_fix_screeninfo finfo;

    if (ioctl(fd, FBIOGET_FSCREENINFO, &finfo)) {
        fprintf(stderr, "cannot open fix info\n");
        exit(1);
    }
    if (ioctl(fd, FBIOGET_VSCREENINFO, &vinfo)) {
        fprintf(stderr, "cannot open variable info\n");
        exit(1);
    }

    int xres, yres, bpp, line_len;
    xres = vinfo.xres;
    yres = vinfo.yres;
    bpp = vinfo.bits_per_pixel;
    line_len = finfo.line_length;

    screensize = yres * xres * bpp / 8;
    printf("screen size: %d\n", screensize);
    printf(
        "RECVFRAM Atlys Ver0.1\n%d(pixel)x%d(line), %d(bit per pixel), %d(line "
        "length)\n", xres, yres, bpp, line_len);

    uint32_t *fb_buf;
    if ((fb_buf = (uint32_t *)mmap(NULL, screensize, PROT_READ | PROT_WRITE,
                                MAP_SHARED, fd, 0)) < 0) {
        fprintf(stderr, "cannot get framebuffer");
        exit(1);
    }

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

    memset(udpbuffer, 0, sizeof(udpbuffer));
    memset(binbuffer, 0, sizeof(binbuffer));

    // headerを書く
    memcpy(mem, headerbuffer, sizeofheader);

    uint32_t *buf_32 = (uint32_t *)udpbuffer;
    uint32_t *framebuf_32 = (uint32_t *)binbuffer;
    int wari = 0, amari = 0;
    // printf("received:%d global_id:%u size:%u local:%u
    // bufcounter:%d\n", received, global_id, size, local_id,
    // bufcounter);
    // フレームループ

    unsigned int framecounter = 0;
    for (unsigned int k = 0; k < 0xFFFFFFFF; k++) {
        // 1280*8*90 Loop

        for (int j = 0; j < 90; j++) {//もはや意味ないけど
            int bufcounter = 0;
            int broken = 0;
            // 1280*8の画像ループ
            while (flg) {
                int received = recv(sock, udpbuffer, sizeof(udpbuffer), 0) - 8;
                global_id = buf_32[0];
                size = buf_32[1] & 0x00FFFFFF;
                local_id = (buf_32[1] >> 24) & 0xFF;

                if(local_id==0){
                    wari = size / UDP_DATASIZE;
                    amari = size - (wari * UDP_DATASIZE);
                    // amari = size%UDP_DATA;
                    framecounter = global_id;
                    bufcounter = 0;
                }

                printf("received:%d global_id:%u size:%u local:%u bufcounter:%d\n", received, global_id, size, local_id, bufcounter);
                for (int i = 0; i < received / 4; i++) {
                    framebuf_32[i + (bufcounter / 4)] = htonl(buf_32[i + 2]);
                }
                bufcounter += received;

                if(framecounter != global_id) {
                    printf("broken: framecnt:%d global:%d\n", framecounter, global_id);
                    broken = 1;
                }

                if (local_id == wari) {
                    flg = 0;
                    if (size != bufcounter) {
                        broken = 1;
                    }
                }
                if ((amari == 0) && (local_id == (wari - 1))) {
                    flg = 0;
                    if (size != bufcounter) {
                        broken = 1;
                    }
                }
            }
            framecounter++;
            // gettimeofday(&start_time, NULL);
            flg = 1;

            if(broken==0){
                //内容を書く
                memcpy(mem + sizeofheader, binbuffer, bufcounter);
                // EOFマーカーを書く
                memcpy(mem + sizeofheader + bufcounter, eof, 2);

                jpeg_create_decompress(&in_info);
                jpeg_mem_src(&in_info, mem, sizeofheader + bufcounter + 2);

                jpeg_read_header(&in_info, TRUE);
                jpeg_start_decompress(&in_info);

                // printf("in_info.output_height:%d
                // in_info.output_components:%d\n", in_info.output_height,
                // in_info.output_components);

                int stride = sizeof(JSAMPLE) * in_info.output_width *
                             in_info.output_components;

                if ((rowbuffer = (unsigned char *)calloc(stride, 1)) == NULL) {
                    perror("calloc error");
                }

                unsigned char img[WIDTH * 8 * DEPTH];  // 8lineしかないので

                for (int i = 0; i < in_info.output_height; i++) {
                    jpeg_read_scanlines(&in_info, &rowbuffer, 1);
                    row = rowbuffer;
                    for (int k = 0; k < stride; k++) {
                        img[k + i * stride] = *row++;
                    }
                }
                jpeg_finish_decompress(&in_info);
                jpeg_destroy_decompress(&in_info);
                free(rowbuffer);
                int position = global_id % 90;
                for (int y = 0; y < 8; y++) {
                    for (int x = 0; x < 1176; x++) {
                        fb_buf[y * xres + x + 1176 * 8 * position] =
                            img[1280 * 3 * y + x * 3 + 0] << 16 |
                            img[1280 * 3 * y + x * 3 + 1] << 8 |
                            img[1280 * 3 * y + x * 3 + 2];
                    }
                }
                msync(fb_buf, screensize, 0);
            }
            //gettimeofday(&end_time, NULL);
            //print_diff_time(start_time, end_time);
        }
    }


    close(sock);
    munmap(fb_buf, screensize);
    return 0;

    error:
        jpeg_destroy_decompress(&in_info);
        free(rowbuffer);
        printf("error occured\n");
    }
