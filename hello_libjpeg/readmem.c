#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <jpeglib.h>
#include <sys/stat.h>

#define HEIGHT 720
#define WIDTH 1280
#define DEPTH 3
#define in_file_name "input.jpg"

int main() {
    struct jpeg_decompress_struct in_info;
    struct jpeg_error_mgr jpeg_error;
    JSAMPROW buffer = NULL;
    JSAMPROW row;
    struct stat sb;
    int sizeofjpeg;

    unsigned char jpegbuffer[0x100000];//1MByte
    in_info.err = jpeg_std_error(&jpeg_error);

    FILE *fp = fopen(in_file_name, "rb");
    if (fp == NULL) {
        printf("no file.\n");
        return -1;
    }

    if (stat(in_file_name, &sb) == -1) {
        perror("stat");
        exit(EXIT_FAILURE);
    }
    sizeofjpeg = sb.st_size;
    fread(jpegbuffer, sizeof(unsigned char), sizeofjpeg, fp);

    jpeg_create_decompress(&in_info);
    jpeg_mem_src(&in_info, jpegbuffer, sizeofjpeg);

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
    free(buffer);
    FILE *file;
    file = fopen("test.raw", "wb");
    fwrite(img, 1, WIDTH * HEIGHT * DEPTH, file);
    fclose(file);
    fclose(fp);
    return 0;
}