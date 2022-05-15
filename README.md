# receivejpeg
受信機側のコード
UDP受信してデコードしてFBに表示するのは `gcc ./vmware/fb_jpegrcv.c -ljpeg`

`fbset -i`
`fbset -g 1280 720 1280 720 24`
`fbset -g 1176 720 1176 720 32`

# Ark PC ゲーミングモニタで動く状態
 `fbset`

 ```
 mode "1920x1080"
    geometry 1920 1080 1920 1080 32
    timings 0 0 0 0 0 0 0
    accel true
    rgba 8/16,8/8,8/0,0/0
endmode

Frame buffer device information:
    Name        : nouveaudrmfb
    Address     : 0
    Size        : 8388608
    Type        : PACKED PIXELS
    Visual      : TRUECOLOR
    XPanStep    : 1
    YPanStep    : 1
    YWrapStep   : 0
    LineLength  : 7680
    Accelerator : No
```
`sudo apt install libjpeg-dev`が必要。
`sudo fbset -g 1920 1080 1920 1080 32`
`gcc fb_jpegrcv.c -ljpeg`
