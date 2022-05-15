# receivejpeg
受信機側のコード
UDP受信してデコードしてFBに表示するのは `gcc ./vmware/fb_jpegrcv.c -ljpeg`

`fbset -i`
`fbset -g 1280 720 1280 720 24`
`fbset -g 1176 720 1176 720 32`