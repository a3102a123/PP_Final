#include <bits/stdc++.h>
#include <sys/time.h>
#define STB_IMAGE_IMPLEMENTATION
#include "lib/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "lib/stb_image_write.h"
#include <pthread.h>
#include <unistd.h>

#define CHANNEL_NUM 3
#define WEEK 25
#define STRONG 180

using namespace std;

typedef struct
{
  int thread_id;
  int height;
} Arg;

// Global use
int width, height, bpp;
int number_of_thread;
int chunk_height;
int chunk_width;
uint8_t *rgb_image;
uint8_t *gray_img;
uint8_t *blur_img;
uint8_t *gradient_img;
uint8_t *out_img;
float *angle;

stack<int> s;
pthread_mutex_t lock2;

int8_t Blur_kernel[9] = {
    1, 2, 1,
    2, 4, 2,
    1, 2, 1};

int8_t x_edge_kernel[9] = {
    -1, 0, 1,
    -2, 0, 2,
    -1, 0, 1};
int8_t y_edge_kernel[9] = {
    -1, -2, -1,
    0, 0, 0,
    1, 2, 1};

void print_image(int width, int height, int x, int y, int num, uint8_t *img)
{
  for (int j = x; j < width && j - x < num; j++)
  {
    for (int i = y; i < height && i - y < num; i++)
    {
      printf("%u ", img[i * width + j]);
    }
    printf("\n");
  }
}

void print_fmatrix(int width, int height, int x, int y, int num, float *matrix)
{
  for (int j = x; j < width && j - x < num; j++)
  {
    for (int i = y; i < height && i - y < num; i++)
    {
      printf("%f ", matrix[i * width + j]);
    }
    printf("\n");
  }
}

void *ToGrayThread(void *arg)
{
  Arg *data = (Arg *)arg;
  //pthread_t tid = pthread_self();
  int tid = data->thread_id;
  //printf("thread id=%d\n", tid);
  uint8_t *pixel, r, g, b;
  int idx;
  for (int j = chunk_height * tid; j < data->height; j += 1)
  {
    //printf("height num=%d\n", j);
    for (int i = 0; i < chunk_width; i++)
    {
      idx = (j * chunk_width + i);
      pixel = rgb_image + idx * CHANNEL_NUM;
      r = pixel[0];
      g = pixel[1];
      b = pixel[2];
      gray_img[idx] = (r * 30 + g * 59 + b * 11 + 50) / 100;
    }
  }
  pthread_exit((void *)0);
}

void *GaussianThread(void *arg)
{
  Arg *data = (Arg *)arg;
  int tid = data->thread_id;
  int idx;
  for (int j = chunk_height * tid; j < data->height; j++)
  {
    for (int i = 0; i < width; i++)
    {
      idx = (j * width + i);
      // set 0 to boundary
      if (i == 0 || i == width - 1)
      {
        blur_img[idx] = 0;
        continue;
      }
      if (j == 0 || j == height - 1)
      {
        blur_img[idx] = 0;
        continue;
      }
      // Gaussian blur
      float sum = 0;
      for (int k = -4; k < 5; k++)
        sum += (Blur_kernel[k + 4] * gray_img[idx + k]);
      blur_img[idx] = sum / 16.0;
    }
  }
  pthread_exit((void *)0);
}

void *SobelThread(void *arg)
{
  Arg *data = (Arg *)arg;
  int tid = data->thread_id;

  int idx;
  for (int j = chunk_height * tid; j < data->height; j++)
  {
    for (int i = 0; i < width; i++)
    {
      idx = (j * width + i);
      // set 0 to boundary
      if (i == 0 || i == width - 1)
      {
        blur_img[idx] = 0;
        continue;
      }
      if (j == 0 || j == height - 1)
      {
        blur_img[idx] = 0;
        continue;
      }
      // Sobel edge detection
      float sum_x = 0, sum_y = 0;
      // x direction differential
      for (int k = -4; k < 5; k++)
        sum_x += (x_edge_kernel[k + 4] * blur_img[idx + k]);
      // y direction differential
      for (int k = -4; k < 5; k++)
        sum_y += (y_edge_kernel[k + 4] * blur_img[idx + k]);
      // the angle of gradient
      angle[idx] = atan2f(sum_y, sum_x) * 180 / M_PI;
      int sum = abs(sum_x) + abs(sum_y);
      gradient_img[idx] = sum;
    }
  }
  pthread_exit((void *)0);
}

void *SuppressionThread(void *arg)
{
  Arg *data = (Arg *)arg;
  int tid = data->thread_id;

  int idx;
  for (int j = chunk_height * tid; j < data->height; j++)
  {
    for (int i = 0; i < width; i++)
    {
      // skip the boundary
      if (i == 0 || i == width - 1)
      {
        continue;
      }
      if (j == 0 || j == height - 1)
      {
        continue;
      }
      idx = (j * width + i);
      float dir = angle[idx];
      if (dir < 0)
        dir += 180;
      int q = 255, r = 255;
      // determine the adjacent gradient
      if ((dir >= 0 && dir < 22.5) || (dir >= 157.5 && dir <= 180))
      {
        q = gradient_img[idx + width];
        r = gradient_img[idx - width];
      }
      else if ((dir >= 22.5 && dir < 67.5))
      {
        q = gradient_img[idx - width + 1];
        r = gradient_img[idx + width - 1];
      }
      else if ((dir >= 67.5 && dir < 112.5))
      {
        q = gradient_img[idx + 1];
        r = gradient_img[idx - 1];
      }
      else if ((dir >= 112.5 && dir < 157.5))
      {
        q = gradient_img[idx - width - 1];
        r = gradient_img[idx + width + 1];
      }
      // supression
      if (gradient_img[idx] >= q && gradient_img[idx] >= r)
        out_img[idx] = gradient_img[idx];
      else
        out_img[idx] = 0;
    }
  }
  pthread_exit((void *)0);
}

// operate on same array
void *double_thresholdThread(void *arg)
{
  Arg *data = (Arg *)arg;
  int tid = data->thread_id;

  int idx;
  for (int j = chunk_height * tid; j < data->height; j++)
  {
    for (int i = 0; i < width; i++)
    {
      idx = (j * width + i);
      if (out_img[idx] >= STRONG)
        out_img[idx] = 255;
      else if (out_img[idx] >= WEEK)
        out_img[idx] = WEEK;
      else
        out_img[idx] = 0;
    }
  }
  pthread_exit((void *)0);
}

void bfsFunc()
{
  // BFS
  int size;
  while (size = s.size())
  {
    for (int i = 0; i < size; i++)
    {
      int idx = s.top();
      s.pop();
      // printf("%d\n",idx);
      for (int k = -1; k <= 1; k++)
      {
        for (int l = -1; l <= 1; l++)
        {
          int temp_idx = idx + k * width + l;
          if (out_img[temp_idx] == WEEK)
          {
            out_img[temp_idx] = 255;
            s.push(temp_idx);
          }
        }
      }
    }
  }
}

void *HysteresisThread(void *arg)
{
  // put the strong edge to stack
  Arg *data = (Arg *)arg;
  int tid = data->thread_id;

  for (int j = chunk_height * tid; j < data->height; j++)
  {
    for (int i = 0; i < width; i++)
    {
      int idx = (j * width + i);
      // skip the boundary
      if (i == 0 || i == width - 1)
      {
        continue;
      }
      if (j == 0 || j == height - 1)
      {
        continue;
      }
      if (out_img[idx] == 255)
      {
        pthread_mutex_lock(&lock2);
        s.push(idx);
        pthread_mutex_unlock(&lock2);
      }
    }
  }
}

void *HysteresisThread2(void *arg)
{
  Arg *data = (Arg *)arg;
  int tid = data->thread_id;
  int idx;
  for (int j = chunk_height * tid; j < data->height; j++)
  {
    for (int i = 0; i < width; i++)
    {
      // skip the boundary
      if (i == 0 || i == width - 1)
      {
        continue;
      }
      if (j == 0 || j == height - 1)
      {
        continue;
      }
      idx = (j * width + i);
      if (out_img[idx] == WEEK)
      {
        for (int k = -1; k <= 1; k++)
        {
          for (int l = -1; l <= 1; l++)
          {
            if (out_img[idx + k * width + l] == 255)
            {
              out_img[idx] = 255;
              break;
            }
          }
        }
      }
      // if (out_img[idx] != 255)
      //   out_img[idx] = 0;
    }
  }
  // bottom up
  for (int j = data->height - 1; j >= chunk_height * tid; j--)
  {
    for (int i = width - 1; i >= 0; i--)
    {
      // skip the boundary
      if (i == 0 || i == width - 1)
      {
        continue;
      }
      if (j == 0 || j == height - 1)
      {
        continue;
      }
      idx = (j * width + i);
      if (out_img[idx] == WEEK)
      {
        for (int k = -1; k <= 1; k++)
        {
          for (int l = -1; l <= 1; l++)
          {
            if (out_img[idx + k * width + l] == 255)
            {
              out_img[idx] = 255;
              break;
            }
          }
        }
      }
      if (out_img[idx] != 255)
        out_img[idx] = 0;
    }
  }
  pthread_exit((void *)0);
}

int main(int argc, char **argv)
{
  // int width, height, bpp;
  struct timeval start[6], end[6];
  const char *function_name[6] = {"ToGray", "Gaussian_blur", "Sobel_serial", "non_max_Suppression", "double_threshold", "Hysteresis"};

  // ---Added
  number_of_thread = atoi(argv[1]);
  
  // thread init
  long long int thread;
  //pthread_t *thread_handles;
  //thread_handles = (pthread_t *)malloc(number_of_thread * sizeof(pthread_t));
  pthread_t thread_handles[number_of_thread];
  
  // 設定 pthread 性質是要能 join
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
  
  // lock
  pthread_mutex_init(&lock2, NULL);


  Arg arg[number_of_thread]; // 每個 thread 傳入的參數
  // ---
  char img_name_temp[] = "image/";
  int len = strlen(img_name_temp) + strlen(argv[2]) + 1;
  char img_name[len];
  memset(img_name, '\0', len);
  strcat(img_name, img_name_temp);
  strcat(img_name, argv[2]);
  printf("%s\n", img_name);
  // load image & allocate memory
  rgb_image = stbi_load(img_name, &width, &height, &bpp, CHANNEL_NUM);
  gray_img = (uint8_t *)malloc(width * height);
  blur_img = (uint8_t *)malloc(width * height);
  gradient_img = (uint8_t *)malloc(width * height);
  out_img = (uint8_t *)malloc(width * height);
  angle = (float *)malloc(width * height * sizeof(float));

  // calc height,width per thread
  chunk_height = ceil(height / (float)number_of_thread);
  chunk_width = width;
  

  //--- ToGrayThread
  // printf("%s","GGstart\n");
  gettimeofday(&start[0], NULL);
  for (thread = 0; thread < number_of_thread; thread++)
  {
    arg[thread].thread_id = thread;
    arg[thread].height = min((int)(chunk_height*(thread+1)),height);
    pthread_create(&thread_handles[thread], &attr, ToGrayThread, (void *)&arg[thread]);
  }
  for (thread = 0; thread < number_of_thread; thread++)
  {
    pthread_join(thread_handles[thread], NULL);
  }
  gettimeofday(&end[0], NULL);
  // Gaussian Thread
  gettimeofday(&start[1], NULL);
  for (thread = 0; thread < number_of_thread; thread++)
  {
    pthread_create(&thread_handles[thread], &attr, GaussianThread, (void *)&arg[thread]);
  }
  for (thread = 0; thread < number_of_thread; thread++)
  {
    pthread_join(thread_handles[thread], NULL);
  }
  gettimeofday(&end[1], NULL);

  // Sobel Thread
  gettimeofday(&start[2], NULL);
  for (thread = 0; thread < number_of_thread; thread++)
  {
    pthread_create(&thread_handles[thread], &attr, SobelThread, (void *)&arg[thread]);
  }
  for (thread = 0; thread < number_of_thread; thread++)
  {
    pthread_join(thread_handles[thread], NULL);
  }
  gettimeofday(&end[2], NULL);
  
  gettimeofday(&start[3], NULL);
  for (thread = 0; thread < number_of_thread; thread++)
  {
    pthread_create(&thread_handles[thread], &attr, SuppressionThread, (void *)&arg[thread]);
  }
  for (thread = 0; thread < number_of_thread; thread++)
  {
    pthread_join(thread_handles[thread], NULL);
  }
  gettimeofday(&end[3], NULL);

  gettimeofday(&start[4], NULL);
  for (thread = 0; thread < number_of_thread; thread++)
  {
    pthread_create(&thread_handles[thread], &attr, double_thresholdThread, (void *)&arg[thread]);
  }
  for (thread = 0; thread < number_of_thread; thread++)
  {
    pthread_join(thread_handles[thread], NULL);
  }
  gettimeofday(&end[4], NULL);

  gettimeofday(&start[5], NULL);
  for (thread = 0; thread < number_of_thread; thread++)
  {
    pthread_create(&thread_handles[thread], &attr, HysteresisThread, (void *)&arg[thread]);
  }
  
  for (thread = 0; thread < number_of_thread; thread++)
  {
    pthread_join(thread_handles[thread], NULL);
  }
  /*printf("Free size: %d", s.size());
  for (int i = 0; i < s.size(); i++)
  {
    int t = s.top();
    s.pop();
    printf("%d ", t);
  }
  printf("Third Check\n");*/
  bfsFunc();
  gettimeofday(&end[5], NULL);
  // print_image(width,height,0,0,max(width,height) + 1,out_img);
  // print_fmatrix(width,height,0,0,max(width,height) + 1,angle);
  stbi_write_png("result/Pthread_image.png", width, height, 1, out_img, width);
  double total_time = 0.0;
  for (int index = 0; index < 6; index++)
  {
    double timeuse = (end[index].tv_sec - start[index].tv_sec) + (double)(end[index].tv_usec - start[index].tv_usec) / 1000000.0;
    printf("%s Time : %.6lf s\n", function_name[index], timeuse);
    total_time += timeuse;
  }
  printf("total Time : %.6lf s\n", total_time);
  // free memory
  stbi_image_free(rgb_image);
  free(gray_img);
  free(blur_img);
  free(gradient_img);
  free(out_img);
  free(angle);
  pthread_mutex_destroy(&lock2);
  return 0;
}