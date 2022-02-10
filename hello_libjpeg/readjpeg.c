#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <jpeglib.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>

#define HEIGHT 720
#define WIDTH 1280
#define DEPTH 3
#define in_file_name "input.jpg"

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

int main() {
    struct jpeg_decompress_struct in_info;
    struct jpeg_error_mgr jpeg_error;
    JSAMPROW buffer = NULL;
    JSAMPROW row;
    struct stat sb;
    struct timeval start_time, end_time;
    unsigned char jpegbuffer[0x100000];//1MByte


    in_info.err = jpeg_std_error(&jpeg_error);

    FILE *infile;
    if ((infile = fopen(in_file_name, "rb")) == NULL) {
        fprintf(stderr, "ファイルが開けません: %s\n", in_file_name);
        return -1;
    }
    gettimeofday(&start_time, NULL);
    jpeg_create_decompress(&in_info);
    jpeg_stdio_src(&in_info, infile);

    jpeg_read_header(&in_info, TRUE);
    jpeg_start_decompress(&in_info);

    printf("in_info.output_height:%d in_info.output_components:%d\n",
           in_info.output_height, in_info.output_components);

    int stride =
        sizeof(JSAMPLE) * in_info.output_width * in_info.output_components;
    printf("stride:%d \n", stride);

    if ((buffer = (unsigned char *)calloc(stride, 1)) == NULL) {
        perror("calloc error");
    }

    unsigned char img[WIDTH * HEIGHT * DEPTH];

    for (int i = 0; i < in_info.output_height; i++) {
        jpeg_read_scanlines(&in_info, &buffer, 1);
        row = buffer;

        for (int k = 0; k < stride; k++) {
            img[k + i * stride] = *row++;
            // printf("height:%d width:%d addr:%d data:0x%06x\n", i, k, k + i * stride, img[k + i*stride]);
        }
    }
    jpeg_finish_decompress(&in_info);
    jpeg_destroy_decompress(&in_info);
    gettimeofday(&end_time, NULL);
    print_diff_time(start_time, end_time);
    free(buffer);
    FILE *file;
    file = fopen("test.raw", "wb");
    fwrite(img, 1, WIDTH * HEIGHT * DEPTH, file);
    fclose(file);
    fclose(infile);
    return 0;
}