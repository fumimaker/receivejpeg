#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>

#define HEIGHT 8
#define WIDTH 1280
#define DEPTH 3
#define in_file_name "0076.bin"
#define headername "headerout.bin"

#include <jpeglib.h>
// #include <turbojpeg.h>

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

// gettimeofday(&start_time, NULL);
// gettimeofday(&end_time, NULL);
// print_diff_time(start_time, end_time);

int main() {
    struct jpeg_decompress_struct in_info;
    struct jpeg_error_mgr jpeg_error;
    JSAMPROW buffer = NULL;
    JSAMPROW row;
    struct stat sb;
    struct timeval start_time, end_time;
    int address_counter = 0;
    int sizeofbin, sizeofheader;

    unsigned char binbuffer[0x100000]; //1MByte
    unsigned char headerbuffer[1024];
    unsigned char mem[0x100000];
    unsigned char eof[2] = {0xFF, 0xD9};
    unsigned char writebuffer[0x2A3000]; //2MByte

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
        // printf("stride:%d \n", stride);

        if ((buffer = (unsigned char *)calloc(stride, 1)) == NULL) {
            perror("calloc error");
        }

        unsigned char img[WIDTH * HEIGHT * DEPTH];

        for (int i = 0; i < in_info.output_height; i++) {
            jpeg_read_scanlines(&in_info, &buffer, 1);
            row = buffer;

            for (int k = 0; k < stride; k++) {
                img[k + i * stride] = *row++;
                // printf("height:%d width:%d addr:%d data:0x%06x\n", i, k, k +
                // i * stride, img[k + i*stride]);
            }
        }
        jpeg_finish_decompress(&in_info);
        jpeg_destroy_decompress(&in_info);
        free(buffer);
        memcpy(writebuffer+address_counter, img, in_info.output_height*stride);
        address_counter += in_info.output_height * stride;
        // sprintf(moji, "rawout/%04d.raw", i);
        // FILE *file = fopen(moji, "wb");
        // if (fp == NULL) {
        //     printf("no file.\n");
        //     return -1;
        // }
        // fwrite(img, 1, WIDTH * HEIGHT * DEPTH, file);
        // fclose(file);
        printf("addr:%d\n", address_counter);
        fclose(fp);
    }
    gettimeofday(&end_time, NULL);
    print_diff_time(start_time, end_time);

    // sprintf(moji, "rawout/%04d.raw", i);
    FILE *file = fopen("rawframe.raw", "wb");
    if (file == NULL) {
        printf("no file.\n");
        return -1;
    }
    fwrite(writebuffer, 1, 1280*720*3, file);
    fclose(file);

    return 0;
}