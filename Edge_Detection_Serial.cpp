#include <bits/stdc++.h>
#include <sys/time.h>
#define STB_IMAGE_IMPLEMENTATION
#include "lib/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "lib/stb_image_write.h"

#define CHANNEL_NUM 3

void ToGray(uint8_t* img, int width , int height , uint8_t* out_img){
    uint8_t *pixel , r , g , b;
    int idx;
    for(int j = 0 ; j < height ; j++){
        for(int i = 0 ; i < width ; i++ ){
            idx = ( j * width + i );
            pixel = img + idx * CHANNEL_NUM;
            r = pixel[0];
            g = pixel[1];
            b = pixel[2];
            out_img[idx] = (r*30 + g*59 + b*11 +50) / 100;
        }
    }
}

void Gaussian_blur(uint8_t* img, int width , int height , uint8_t* out_img){
    int idx;
    int8_t kernel[9] = {
        1 , 2 , 1,
        2 , 4 , 2,
        1 , 2 , 1
    };
    for(int j = 0 ; j < height ; j++){
        for(int i = 0 ; i < width ; i++ ){
            idx = ( j * width + i );
            // set 0 to boundary
            if(i == 0 || i == width - 1){
                img[idx] = 0;
                continue;
            }
            if(j == 0 || j == height - 1){
                img[idx] = 0;
                continue;
            }
            // Gaussian blur
            float sum = 0;
            for(int k = -4 ; k < 5 ; k++)
                sum += ( kernel[k + 4] * img[idx + k] );
            out_img[idx] = sum / 16.0;
        }
    }
}

void Sobel_serial(uint8_t* img, int width , int height , uint8_t* out_img){
    int idx;
    int8_t x_kernel[9] = {
        -1 , 0 , 1,
        -2 , 0 , 2,
        -1 , 0 , 1
    };
    int8_t y_kernel[9] = {
        -1 , -2 , -1,
        0 , 0 , 0,
        1 , 2 , 1
    };
    for(int j = 0 ; j < height ; j++){
        for(int i = 0 ; i < width ; i++ ){
            idx = ( j * width + i );
            // set 0 to boundary
            if(i == 0 || i == width - 1){
                img[idx] = 0;
                continue;
            }
            if(j == 0 || j == height - 1){
                img[idx] = 0;
                continue;
            }
            // Sobel edge detection
            int sum_x = 0,sum_y = 0;
            // x direction differential
            for(int k = -4 ; k < 5 ; k++)
                sum_x += ( x_kernel[k + 4] * img[idx + k] );
            // y direction differential
            for(int k = -4 ; k < 5 ; k++)
                sum_y += ( y_kernel[k + 4] * img[idx + k] );
            int sum = abs(sum_x) + abs(sum_y);
            // thresholding 
            if(sum >= 127)
                out_img[idx] = 255;
            else
                out_img[idx] = 0;
        }
    }
}

int main(int argc,char **argv){
    int width, height, bpp;
    struct timeval start,end;
    // load image & allocate memory
    uint8_t* rgb_image = stbi_load("image/im1.png", &width, &height, &bpp, CHANNEL_NUM);
    uint8_t* gray_img = (uint8_t*)malloc(width*height);
    uint8_t* blur_img = (uint8_t*)malloc(width*height);
    uint8_t* out_img = (uint8_t*)malloc(width*height);
    // doing computation
    gettimeofday(&start,NULL);
    ToGray(rgb_image,width,height,gray_img);
    Gaussian_blur(gray_img,width,height,blur_img);
    Sobel_serial(blur_img,width,height,out_img);
    gettimeofday(&end,NULL);
    stbi_write_png("result/image.png", width, height, 1, out_img, width);
    double timeuse = (end.tv_sec - start.tv_sec) + (double)(end.tv_usec - start.tv_usec)/1000000.0;
    printf("Serial Time : %.6lf s\n", timeuse);
    // free memory
    stbi_image_free(rgb_image);
    free(gray_img);
    free(out_img);
    return 0;
}