#include <bits/stdc++.h>
#include <sys/time.h>
#define STB_IMAGE_IMPLEMENTATION
#include "lib/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "lib/stb_image_write.h"

#define CHANNEL_NUM 3

int8_t Blur_kernel[9] = {
    1 , 2 , 1,
    2 , 4 , 2,
    1 , 2 , 1
};

int8_t x_edge_kernel[9] = {
    -1 , 0 , 1,
    -2 , 0 , 2,
    -1 , 0 , 1
};
int8_t y_edge_kernel[9] = {
    -1 , -2 , -1,
    0 , 0 , 0,
    1 , 2 , 1
};

void print_image(int width , int height , int x , int y , int num , uint8_t* img){
    for(int j = x ; j < width && j - x < num  ; j++){
        for(int i = y ; i < height && i - y < num ; i++){
            printf("%u ",img[i * width + j]);
        }
        printf("\n");
    }
}

void print_fmatrix(int width , int height , int x , int y , int num , float* matrix){
    for(int j = x ; j < width && j - x < num  ; j++){
        for(int i = y ; i < height && i - y < num ; i++){
            printf("%f ",matrix[i * width + j]);
        }
        printf("\n");
    }
}

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
                sum += ( Blur_kernel[k + 4] * img[idx + k] );
            out_img[idx] = sum / 16.0;
        }
    }
}

void Sobel_serial(uint8_t* img, int width , int height , float* angle , uint8_t* out_img){
    int idx;
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
            float sum_x = 0,sum_y = 0;
            // x direction differential
            for(int k = -4 ; k < 5 ; k++)
                sum_x += ( x_edge_kernel[k + 4] * img[idx + k] );
            // y direction differential
            for(int k = -4 ; k < 5 ; k++)
                sum_y += ( y_edge_kernel[k + 4] * img[idx + k] );
            // the angle of gradient
            angle[idx] = atan2f(sum_y,sum_x);
            int sum = abs(sum_x) + abs(sum_y);
            // thresholding 
            out_img[idx] = sum;
            // if(sum >= 127)
            //     out_img[idx] = 255;
            // else
            //     out_img[idx] = 0;
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
    float* angle = (float*)malloc(width*height * sizeof(float));
    // doing computation
    gettimeofday(&start,NULL);
    ToGray(rgb_image,width,height,gray_img);
    Gaussian_blur(gray_img,width,height,blur_img);
    Sobel_serial(blur_img,width,height,angle,out_img);
    gettimeofday(&end,NULL);
    // print_image(width,height,350,270,10,out_img);
    // print_fmatrix(width,height,350,270,10,angle);
    stbi_write_png("result/image.png", width, height, 1, out_img, width);
    double timeuse = (end.tv_sec - start.tv_sec) + (double)(end.tv_usec - start.tv_usec)/1000000.0;
    printf("Serial Time : %.6lf s\n", timeuse);
    // free memory
    stbi_image_free(rgb_image);
    free(gray_img);
    free(out_img);
    return 0;
}