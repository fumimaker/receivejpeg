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
#include <sys/types.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#include <time.h>
#define DEVICE_NAME "/dev/fb0"

int OpenFrameBuffer(int fd) {
    fd = open(DEVICE_NAME, O_RDWR);
    if (!fd) {
        fprintf(stderr, "cannot open the FrameBuffer '%s'\n", DEVICE_NAME);
        exit(1);
    }
    return fd;
}

int main(int argc, char **argv) {
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

    int xres, yres, bpp, line_len, stride;
    xres = vinfo.xres;
    yres = vinfo.yres;
    bpp = vinfo.bits_per_pixel;
    line_len = finfo.line_length;
    stride = xres * bpp / 8;

    screensize = yres * xres * bpp / 8;
    printf("screen size: %d\n", screensize);
    printf(
        "RECVFRAM Atlys Ver0.1\n%d(pixel)x%d(line), %d(bit per pixel), %d(line "
        "length)\n",
        xres, yres, bpp, line_len);

    /* Handler if socket get a packet, it will be mapped on memory */

    uint32_t *buf;
    if ((buf = (uint32_t *)mmap(NULL, screensize, PROT_READ | PROT_WRITE,
                                MAP_SHARED, fd, 0)) < 0) {
        fprintf(stderr, "cannot get framebuffer");
        exit(1);
    }

    struct termios tio;  // シリアル通信設定
    int baudRate = B115200;
#define SERIAL_PORT "/dev/ttyACM0"
    int fd_serial = open(SERIAL_PORT, O_RDWR | O_SYNC);
    if (fd_serial < 0) {
        printf("serial open error\n");
        // return -1;
    }
    tio.c_cflag += CREAD;   // 受信有効
    tio.c_cflag += CLOCAL;  // ローカルライン（モデム制御なし）
    tio.c_cflag += CS8;     // データビット:8bit
    tio.c_cflag += 0;       // ストップビット:1bit
    tio.c_cflag += 0;       // パリティ:None

    cfsetispeed(&tio, baudRate);
    cfsetospeed(&tio, baudRate);
    cfmakeraw(&tio);                      // RAWモード
    tcsetattr(fd_serial, TCSANOW, &tio);  // デバイスに設定を行う
    ioctl(fd_serial, TCSETS, &tio);  // ポートの設定を有効にする
    srand((unsigned int)time(NULL));
    char a[1] = {0x41};

    for (int y = 0; y < yres; y++) {
        for (int x = 0; x < xres; x++) {
            buf[y * xres + x] = 0x0;
        }
    }
    char charbuf[255];
    while (1) {
        //tcflush(fd_serial, TCIOFLUSH);
        int len = read(fd_serial, charbuf, sizeof(charbuf));
        if (len>0){
            if(charbuf[0]=='A'){
                int color;
                for (int y = 0; y < 64; y++) {
                    for (int x = 0; x < xres; x++) {
                        // int pattern = x / (xres / 8);
                        // switch (pattern) {
                        //     case 0:
                        //         color = 0x00FFFFFF;
                        //         break;
                        //     case 1:
                        //         color = 0x00FFFF00;
                        //         break;
                        //     case 2:
                        //         color = 0x0000FFFF;
                        //         break;
                        //     case 3:
                        //         color = 0x0000FF00;
                        //         break;
                        //     case 4:
                        //         color = 0x00FF00FF;
                        //         break;
                        //     case 5:
                        //         color = 0x00FF0000;
                        //         break;
                        //     case 6:
                        //         color = 0x000000FF;
                        //         break;
                        //     case 7:
                        //         color = 0x00000000;
                        //         break;
                        // }
                        buf[y * xres + x] = 0xFFFFFF;
                        // printf("x:%d y:%d\n", x, y);
                    }
                }
                msync(buf, screensize, 0);
            }
            else if(charbuf[0]=='B'){
                for (int y = 0; y < 64; y++) {
                    for (int x = 0; x < xres; x++) {
                        buf[y * xres + x] = 0x0;
                    }
                }
                msync(buf, screensize, 0);
            }
        }
    }

    munmap(buf, screensize);
    close(fd);
    close(fd_serial);
    return 0;
}