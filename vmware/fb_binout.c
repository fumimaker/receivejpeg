// FBにねこ丸を表示するプログラム

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <stdint.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <linux/fb.h>
#include <linux/fs.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <jpeglib.h>
#include <sys/stat.h>

#define DEVICE_NAME "/dev/fb0"
#define HEIGHT 720
#define WIDTH 1280
#define DEPTH 3
#define in_file_name "0076.bin"
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

int OpenFrameBuffer(int fd){
    fd = open(DEVICE_NAME, O_RDWR);
    if(!fd){
        fprintf(stderr, "cannot open the FrameBuffer '%s'\n", DEVICE_NAME);
        exit(1);
    }
    return fd;
}

int main(int argc, char **argv){
    int fd = 0;
    int screensize;
    int x, y, col;
    int r, g, b;
    fd = OpenFrameBuffer(fd);
    struct fb_var_screeninfo vinfo;
    struct fb_fix_screeninfo finfo;

    struct jpeg_decompress_struct in_info;
    struct jpeg_error_mgr jpeg_error;
    JSAMPROW buffer = NULL;
    JSAMPROW row;
    struct stat sb;
    struct timeval start_time, end_time;

    in_info.err = jpeg_std_error(&jpeg_error);

    unsigned char binbuffer[0x100000]; //1MByte
    unsigned char headerbuffer[1024];
    unsigned char mem[0x100000];
    unsigned char eof[2] = {0xFF, 0xD9};
    unsigned char writebuffer[0x2A3000]; //2MByte

    int sizeofbin, sizeofheader;
    int address_counter = 0;

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
    fread(headerbuffer, sizeof(unsigned char), sizeofheader, fp_header);
    printf("sizeofheader:%d bytes\n", sizeofheader);
    gettimeofday(&start_time, NULL);

    memcpy(mem, headerbuffer, sizeofheader);


    for(int i=90; i<180; i++){
        char moji[32];
        sprintf(moji, "bininput/%04d.bin", i);

        FILE *fp = fopen(moji, "rb");
        if (fp == NULL) {
            printf("no file.\n");
            return -1;
        }
        if (stat(moji, &sb) == -1) {
            perror("stat");
            exit(EXIT_FAILURE);
        }
        sizeofbin = sb.st_size;
        fread(binbuffer, sizeof(unsigned char), sizeofbin, fp);

        memcpy(mem + sizeofheader, binbuffer, sizeofbin);
        memcpy(mem + sizeofheader + sizeofbin, eof, 2);



        jpeg_create_decompress(&in_info);
        jpeg_mem_src(&in_info, mem, sizeofheader + sizeofbin + 2);

        jpeg_read_header(&in_info, TRUE);
        jpeg_start_decompress(&in_info);

        // printf("in_info.output_height:%d in_info.output_components:%d\n", in_info.output_height, in_info.output_components);

        int stride =
            sizeof(JSAMPLE) * in_info.output_width * in_info.output_components;

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
        memcpy(writebuffer+address_counter, img, in_info.output_height*stride);
        address_counter += in_info.output_height * stride;
        printf("addr:%d\n", address_counter);
        fclose(fp);
    }
    gettimeofday(&end_time, NULL);
    print_diff_time(start_time, end_time);

    FILE *file = fopen("rawframe.raw", "wb");
    if (file == NULL) {
        printf("no file.\n");
        return -1;
    }
    fwrite(writebuffer, 1, 1280*720*3, file);
    fclose(file);



    if (ioctl(fd, FBIOGET_FSCREENINFO, &finfo))
    {
        fprintf(stderr, "cannot open fix info\n");
        exit(1);
    }
    if (ioctl(fd, FBIOGET_VSCREENINFO, &vinfo))
    {
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
    printf("RECVFRAM Atlys Ver0.1\n%d(pixel)x%d(line), %d(bit per pixel), %d(line length)\n", xres, yres, bpp, line_len);

    /* Handler if socket get a packet, it will be mapped on memory */

    uint32_t *buf;
    if ((buf = (uint32_t *)mmap(NULL, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) < 0){
        fprintf(stderr, "cannot get framebuffer");
        exit(1);
    }
    for(int y=0; y<720; y++){
        for(x=0; x<1176; x++){
            buf[y*xres + x] = writebuffer[ 1280*3*y + x*3 +0]<<16|
                            writebuffer[ 1280*3*y + x*3 +1]<<8|
                            writebuffer[ 1280*3*y + x*3 +2];
        }
    }


    msync(buf, screensize, 0);
    munmap(buf, screensize);
    close(fd);
    return 0;
}