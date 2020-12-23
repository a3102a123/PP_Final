#include <bits/stdc++.h>
#define STB_IMAGE_IMPLEMENTATION
#include "lib/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "lib/stb_image_write.h"

#define CHANNEL_NUM 3

int main(int argc,char **argv){
    int width, height, bpp;
    uint8_t* rgb_image = stbi_load("image/im1.png", &width, &height, &bpp, 3);
    uint8_t* out_img = (uint8_t*)malloc(width*height*CHANNEL_NUM);
    memcpy(out_img,rgb_image,width*height*CHANNEL_NUM);
    stbi_write_png("image.png", width, height, CHANNEL_NUM, out_img, width*CHANNEL_NUM);
    stbi_image_free(rgb_image);
    free(out_img);
    return 0;
}