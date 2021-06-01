#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <inttypes.h>
#include <iostream>

uint8_t *pixels_ori;
uint8_t *pixels_inv_b;
uint8_t *pixels_inv_g;
uint8_t *chunk_arr_b;
uint8_t *chunk_arr_g;
uint8_t *regions[10000];
int counter = 0;
int counter2_ = 0;
int nb_regions = 0;
int width_, height_;
int chunk_w_, chunk_h_;
int width_rows_, height_rows_;
int first_time = 1;
int n = 0;

extern "C"
{
    void selective_invert(unsigned char *pixels, unsigned char *pixels_inv, int width, int height, int chunk_w, int chunk_h, int thres)
    {
        //  int chunk = 30;
        //  int thres = 15; //chunk /2;  good chunk 30 thres 15
        int size = width * height;
        int nb_times_chunk_h = height / chunk_h;
        int width_rows = width / chunk_w;
        int height_rows = height / chunk_h;

        for (int y = 0; y < height_rows; y++)
        {
            for (int x = 0; x < width_rows; x++)
            {
                int b_counter = 0;
                int w_counter = 0;
                for (int n = 0; n < chunk_h; n++)
                {
                    int start = (x * chunk_w) + (y * width * chunk_h) + (n * width);
                    int end = start + chunk_w;

                    for (int j = start; j < end; j++)
                        switch (pixels[j])
                        {
                        case 0:
                            pixels_inv[j] = 255;
                            b_counter += 1;
                            break;
                        case 255:
                            pixels_inv[j] = 0;
                            //w_counter += 1;
                            break;
                        }
                }

                if (b_counter > thres)
                    for (int n = 0; n < chunk_h; n++)
                        memcpy(pixels + (x * chunk_w) + (y * width * chunk_h) + (n * width), pixels_inv + (x * chunk_w) + (y * width * chunk_h) + (n * width), chunk_w);
            }
        }
    }
}
extern "C"
{
    void quantize(char *source_8bpp_current, char *source_8bpp_modified_current, int size)
    {
        int val;

        for (int h = 0; h < size; h++)
        { // int v = source_8bpp_current[h];
            val = (u_int8_t)source_8bpp_current[h] / 64;
            switch (val)
            {
            case 0:
                source_8bpp_modified_current[h] = 0;
                break;
            case 1:
                source_8bpp_modified_current[h] = 85;
                break;
            case 2:
                source_8bpp_modified_current[h] = 170;
                break;
            case 3:
                source_8bpp_modified_current[h] = 255;
                break;
            }
        }
    }
}

extern "C"
{
    void fill_regions(uint8_t *dest, uint8_t *from, uint8_t *matrix, int thres)
    {
        //  thres = 5;
        int x, y;
        for (int b = 0; b < counter2_; b++)
        {
            int nn = regions[b][width_rows_ * 2 + 1];
            if (nn < thres * 2)
                for (int a = 0; a < nn; a += 2)
                {
                    x = regions[b][a];
                    y = regions[b][a + 1];
                    for (int n = 0; n < chunk_h_; n++)
                        memcpy(dest + (x * chunk_w_) + (y * width_ * chunk_h_) + (n * width_), from + (x * chunk_w_) + (y * width_ * chunk_h_) + (n * width_), chunk_w_);
                    //  matrix[x + y * width_rows_] = n;
                }
        }
    }
}
extern "C"
{
    void find_regions(int to_find, uint8_t *matrix)
    {
        counter = 0;
        int y = 0, x = 0;
        int curr = 0, prev = 0;
        counter2_ = 0;
        for (int y = 0; y < height_rows_; y++)
        {
            for (int x = 0; x < width_rows_; x++)
            {
                prev = curr;
                curr = matrix[x + y * width_rows_];
                if (curr != prev || x % width_rows_ == 0)
                {
                    if (counter > width_rows_ * 2 + 1)
                        printf("Warning counter overflow \n");
                    regions[counter2_][width_rows_ * 2 + 1] = counter; //store counter number
                    counter = 0;

                    counter2_++;
                }
                if (matrix[x + y * width_rows_] == to_find)
                {
                    regions[counter2_][counter] = x;
                    regions[counter2_][counter + 1] = y;
                    counter += 2;
                }

                //printf(" [%3d %3d %3d] ", x, y, matrix[x + y * width]);
                //    printf("%d", matrix[x + y * width_rows_]);
            }
            //  printf(" \n");
        }
        if (counter2_ > 10000)
            printf("Warning counter 2 overflow \n");
    }
}
extern "C"
{
    void polarize(uint8_t *pixels, float factor, int pivot, int size)
    {
        int ori, val, fin;
        for (int a = 0; a < size; a++)
        {
            //   pixels[a] = pixels[a];
            if (pixels[a] >= pivot)
            {
                //  val = ;
                pixels[a] = pixels[a] + (255 - pixels[a]) / factor;
            }
            else
            {
                pixels[a] = pixels[a] - pixels[a] / factor;
            }
        }
    }
}
extern "C"
{
    void polarize_24bit(uint8_t *pixels_, uint8_t *pixels_24, float factor_, int pivot_, int size_)
    {
        int ori, val, fin;
        // factor_ = 2;
        // pivot_ = 130;
        // size_ = 1200*825;
        printf("info %f  \n", factor_);
        printf("info %d \n", pivot_);
        printf("info %d \n", size_);

        for (int a = 0; a < size_; a++)
        {
            //   if (a < 500)
            if (pixels_[a] >= pivot_)
            {
                val = pixels_[a] + (255 - pixels_[a]) / factor_;
                //  val = 170;
                pixels_24[a * 3] = val;
                pixels_24[a * 3 + 1] = val;
                pixels_24[a * 3 + 2] = val;
                // if (a < 20)
                //        printf("%d %d %d \n", pixels_24[a], pixels_24[a + 1], pixels_24[a + 2]);
            }
            else
            {
                val = pixels_[a] - pixels_[a] / factor_;
                //    val = 170;
                pixels_24[a * 3] = val;
                pixels_24[a * 3 + 1] = val;
                pixels_24[a * 3 + 2] = val;
                //    if (a < 20)
                //       printf("%d %d %d \n", pixels_24[a], pixels_24[a + 1], pixels_24[a + 2]);
            }
        }
        printf("%d \n", pixels_24[24 * 3]);
    }
}
extern "C"
{
    int polarize_int(int n, int pivot, int factor)
    {
        if (n >= pivot)
        {
            //  val = ;
            return n + (255 - n) / factor;
        }
        else
        {
            return n - n / factor;
        }
    }
}
extern "C"
{
    void alloc_memory(int size)
    {
        pixels_inv_b = (uint8_t *)calloc(size * 2, sizeof(unsigned char));
        pixels_inv_g = (uint8_t *)calloc(size * 2, sizeof(unsigned char));

        chunk_arr_b = (uint8_t *)calloc(size * 2, sizeof(unsigned char));
        chunk_arr_g = (uint8_t *)calloc(size * 2, sizeof(unsigned char));

        pixels_ori = (uint8_t *)calloc(size * 2, sizeof(unsigned char));
    }
}
extern "C"
{
    void selective_invert_v2(uint8_t *pixels, int width, int height, int chunk_w, int chunk_h, int thres_perc, int b_thres, int fill_thres)
    {

        n = 0;
        //  int chunk = 30;
        //  int thres = 15; //chunk /2;  good chunk 30 thres 15
        int area = chunk_w * chunk_h;
        float quotient = (float)thres_perc / 100;
        int thres = int(quotient * area);

        int prev_width_rows = width_rows_;

        if (prev_width_rows != width / chunk_w)
            for (int x = 0; x < 10000; x++)
            {
                free(regions[x]);
                regions[x] = (uint8_t *)calloc((width / chunk_w) * 2 + 5, 1);
            }

        int size = width * height;
        int nb_times_chunk_h = height / chunk_h;
        int width_rows = width / chunk_w;
        int height_rows = height / chunk_h;
        width_ = width;
        height_ = height;
        width_rows_ = width / chunk_w;
        height_rows_ = height / chunk_h;
        chunk_w_ = chunk_w, chunk_h_ = chunk_h;
        // if (first_time == 1){
        //     alloc_memory(size);
        //     first_time = 0;
        // }
        // printf("%d   \n", n++);
        memset(chunk_arr_b, 0, width_rows * height_rows * 2);
        memset(chunk_arr_g, 0, width_rows * height_rows * 2);
        memset(pixels_inv_b, 0, size);
        memset(pixels_inv_g, 0, size);
        memcpy(pixels_ori, pixels, size);

        // memcpy(pixels_inv_b, pixels, size);
        int primary_thres = 127;
        for (int y = 0; y < height_rows; y++)
        {
            for (int x = 0; x < width_rows; x++)
            {
                int b_counter = 0;
                int w_counter = 0;
                int g_counter = 0;
                for (int n = 0; n < chunk_h; n++)
                {
                    int start = (x * chunk_w) + (y * width * chunk_h) + (n * width);
                    int end = start + chunk_w;

                    for (int j = start; j < end; j++)
                    {
                        //  pixels[j] = polarize_int(pixels[j], 160, 2);

                        if (pixels[j] < primary_thres)
                        {
                            if (pixels[j] < 80)

                                pixels_inv_b[j] = 255;

                            b_counter += 1;
                            if (pixels[j] > b_thres)
                            {
                                pixels_inv_g[j] = 0;
                            }
                        }
                        else if (pixels[j] >= primary_thres)
                        {
                            g_counter += 1;
                            pixels_inv_b[j] = 0;

                            pixels_inv_g[j] = 255;
                        }
                    }
                }

                if (b_counter > thres)
                {
                    chunk_arr_b[x + y * width_rows_] = 1;
                    for (int n = 0; n < chunk_h; n++)
                        memcpy(pixels + (x * chunk_w) + (y * width * chunk_h) + (n * width), pixels_inv_b + (x * chunk_w) + (y * width * chunk_h) + (n * width), chunk_w);
                }
                if (g_counter > thres)
                {
                    chunk_arr_g[x + y * width_rows_] = 1;
                    for (int n = 0; n < chunk_h; n++)
                        memcpy(pixels + (x * chunk_w) + (y * width * chunk_h) + (n * width), pixels_inv_g + (x * chunk_w) + (y * width * chunk_h) + (n * width), chunk_w);
                }
            }
        }
        find_regions(1, chunk_arr_b);
        fill_regions(pixels, pixels_ori, chunk_arr_b, fill_thres);
        find_regions(0, chunk_arr_g);
        fill_regions(pixels, pixels_inv_g, chunk_arr_g, fill_thres);

        //     printf("nb_regions %d \n\n", nb_regions);

        // for (int y = 0; y < height_rows; y++)
        // {
        //     for (int x = 0; x < width_rows; x++)
        //     {

        //         //printf(" [%3d %3d %3d] ", x, y, chunk_arr_b[x + y * width]);
        //         printf("%d", chunk_arr_b[x + y * width_rows_]);
        //     }
        //     printf(" \n");
        // }
    }
}

extern "C"
{
    void invert_task(uint8_t *pixels, int width, int height, int chunk_w, int chunk_h, int thres_perc, int b_thres, int fill_thres, float pole_factor, int pivot)
    {
        int size = width * height;
        if (pole_factor != 0)
            polarize(pixels, pole_factor, pivot, size);
        selective_invert_v2(pixels, width, height, chunk_w, chunk_h, thres_perc, b_thres, fill_thres);
    }
}