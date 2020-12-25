#include <bits/stdc++.h>
#define STB_IMAGE_IMPLEMENTATION
#include "lib/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "lib/stb_image_write.h"

#define CHANNEL_NUM 3

void ToGray(uint8_t* img, int width , int height , uint8_t* out_img ){
    uint8_t *pixel , r , g , b;
    int idx;
    for(int j = 0 ; j < height ; j++){
        for(int i = 0 ; i < width ; i++ ){
            idx = ( j * width + i );
            pixel = img + idx*3;
            r = pixel[0];
            g = pixel[1];
            b = pixel[2];
            out_img[idx] = (r*30 + g*59 + b*11 +50) / 100;
        }
    }
}

int main(int argc,char **argv){
    int width, height, bpp;
    uint8_t* rgb_image = stbi_load("image/im1.png", &width, &height, &bpp, 3);
    uint8_t* out_img = (uint8_t*)malloc(width*height);
    ToGray(rgb_image,width,height,out_img);
    stbi_write_png("image.png", width, height, 1, out_img, width);
    stbi_image_free(rgb_image);
    free(out_img);
    return 0;
}