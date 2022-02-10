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

#include <linux/fb.h>
#include <linux/fs.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#define DEVICE_NAME "/dev/fb0"

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

    int xres, yres, bpp, line_len, stride;
    xres = vinfo.xres;
    yres = vinfo.yres;
    bpp = vinfo.bits_per_pixel;
    line_len = finfo.line_length;
    stride = xres*bpp/8;

    screensize = yres * xres * bpp / 8;
    printf("screen size: %d\n", screensize);
    printf("RECVFRAM Atlys Ver0.1\n%d(pixel)x%d(line), %d(bit per pixel), %d(line length)\n", xres, yres, bpp, line_len);

    /* Handler if socket get a packet, it will be mapped on memory */

    uint32_t *buf;
    if ((buf = (uint32_t *)mmap(NULL, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) < 0){
        fprintf(stderr, "cannot get framebuffer");
        exit(1);
    }
    int casenum = xres / 8;
    int color;
    for(int y=0; y<yres; y++){
        for(int x=0; x<xres; x++){
            // if(x<casenum) {
            //     color = 0x00FFFFFF;
            // } else if(x<casenum*2) {
            //     color = 0x00FFFF00;
            // } else if(x<casenum*3) {
            //     color = 0x0000FFFF;
            // } else if(x<casenum*4) {
            //     color = 0x00FF00FF;
            // } else if(x<casenum*5) {
            //     color = 0x00FF0000;
            // } else if(x<casenum*6) {
            //     color = 0x000000FF;
            // } else if(x<casenum*7) {
            //     color = 0x00000000;
            // }
            // switch(pattern){
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
            buf[y*xres + x] = 0xFFFFFFFF;
            //printf("x:%d y:%d\n", x, y);
        }
    }
    // while (1){
    //     for (y = 0; y < yres; ++y){
    //         for (x = 0; x < xres; ++x){
    //             r = (x * 256 / xres);
    //             g = (y * 256 / yres);
    //             *(buf + ((y * line_len / 4) + x)) = (r << 16) | (g << 8) | (b); // 00 RR GG BB
    //         }
    //     }
    //     if (++b > 255){
    //         b = 0;
    //     }
    // }
    //msync(buf, screensize, 0);
    munmap(buf, screensize);
    close(fd);
    return 0;
}