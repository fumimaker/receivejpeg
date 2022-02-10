
#include <stdio.h>
#include <jpeglib.h>
#include <stdlib.h>
#include <string.h>

#define HEIGHT 720
#define WIDTH 1280

#define in_file_name "input.jpg"


////////////// NOT WORKING ////////////



int main (){
    struct jpeg_decompress_struct in_info;
    struct jpeg_error_mgr jpeg_error;
    in_info.err = jpeg_std_error(&jpeg_error);

    FILE *infile;
    if ((infile = fopen(in_file_name, "rb")) == NULL) {
        fprintf(stderr, "ファイルが開けません: %s\n", in_file_name);
        return -1;
    }

    jpeg_create_decompress(&in_info);
    jpeg_stdio_src(&in_info, infile);

    jpeg_read_header(&in_info, TRUE);
    jpeg_start_decompress(&in_info);


    printf("in_info.output_height:%d in_info.output_components:%d\n",
           in_info.output_height, in_info.output_components);
    printf("JSAMPARRAY:%d JSAMPROW:%d JSAMPLE:%d\n", sizeof(JSAMPARRAY), sizeof(JSAMPROW), sizeof(JSAMPLE));

        // 展開用メモリの確保
        JSAMPARRAY buffer =
            (JSAMPARRAY)malloc(sizeof(JSAMPROW) * in_info.output_height);
    for (int i = 0; i < in_info.output_height; ++i) {
        buffer[i] = (JSAMPROW)calloc(
            sizeof(JSAMPLE), in_info.output_width * in_info.output_components);
    }

    // 画像のスキャン
    while (in_info.output_scanline < in_info.output_height) {
        jpeg_read_scanlines(&in_info, buffer + in_info.output_scanline,
                            in_info.output_height - in_info.output_scanline);
    }

    FILE *file;
    file = fopen("test.raw", "wb");
    JSAMPARRAY img = (JSAMPARRAY)malloc(sizeof(JSAMPROW) * HEIGHT);
    for (int i = 0; i < HEIGHT; i++) {
        img[i] = (JSAMPROW)calloc(sizeof(JSAMPLE),
                                  WIDTH * in_info.output_components);
        memcpy(img[i], buffer[i], WIDTH * in_info.output_components);

        fwrite(img, 1, sizeof(img), file);
        fclose(file);

        jpeg_finish_decompress(&in_info);
        jpeg_destroy_decompress(&in_info);
        fclose(infile);
	}
	return 0;
}