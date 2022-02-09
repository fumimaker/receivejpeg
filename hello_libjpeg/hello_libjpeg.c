#include <stdio.h>
#include <jpeglib.h>
#include <stdlib.h>

int main (){
    // 構造体確保
    struct jpeg_decompress_struct cinfo;
    jpeg_create_decompress(&cinfo);
    struct jpeg_error_mgr jerr;
	cinfo.err = jpeg_std_error(&jerr);

    // 入力ファイル指定
    FILE *fp = fopen("./input.jpg", "rb");
    jpeg_stdio_src(&cinfo, fp);

    // ヘッダ情報取得
    jpeg_read_header(&cinfo, TRUE);
    int width = cinfo.image_width;
    int height = cinfo.image_height;
    int ch = cinfo.num_components;

    // データ読み込み
    jpeg_start_decompress(&cinfo);

    // 画像データ格納する配列を動的確保
    JSAMPARRAY *data =
        (JSAMPARRAY *)malloc(sizeof(JSAMPLE) * width * height * ch);
    JSAMPROW *row = data;

    // 列単位でデータを格納
    for (int y = 0; y < height; y++) {
        jpeg_read_scanlines(&cinfo, &row, 1);
        row += width * ch;
    }

    // 構造体破棄
    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    fclose(fp);

    return 0;
}