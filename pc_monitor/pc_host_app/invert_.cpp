#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>

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
            val = (u_int8_t) source_8bpp_current[h] / 64;
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