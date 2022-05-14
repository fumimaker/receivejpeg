#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>

#define SERIAL_PORT "/dev/ttyACM0"

int main(int argc, char *argv[]) {
    unsigned char msg[] = "serial port open...\n";
    unsigned char buf[255];  // バッファ
    int fd;                  // ファイルディスクリプタ
    struct termios tio;      // シリアル通信設定
    int baudRate = B115200;
    int i;
    int len;
    int ret;
    int size;

    fd = open(SERIAL_PORT, O_RDWR | O_SYNC);  // デバイスをオープンする
    if (fd < 0) {
        printf("open error\n");
        return -1;
    }

    tio.c_cflag += CREAD;   // 受信有効
    tio.c_cflag += CLOCAL;  // ローカルライン（モデム制御なし）
    tio.c_cflag += CS8;     // データビット:8bit
    tio.c_cflag += 0;       // ストップビット:1bit
    tio.c_cflag += 0;       // パリティ:None

    cfsetispeed(&tio, baudRate);
    cfsetospeed(&tio, baudRate);

    cfmakeraw(&tio);  // RAWモード

    tcsetattr(fd, TCSANOW, &tio);  // デバイスに設定を行う

    ioctl(fd, TCSETS, &tio);  // ポートの設定を有効にする

    // 送受信処理ループ
    char a[1] = {0x41};
    char b[1] = {0x42};
    for (int i = 0; i < 101; i++) {
        write(fd, a, 1);
        tcflush(fd, TCIOFLUSH);
        usleep(200);
        write(fd, b, 1);
        tcflush(fd, TCIOFLUSH);
        usleep(200);
        //printf("%d\n", i);
    }

    close(fd);  // デバイスのクローズ
    return 0;
}