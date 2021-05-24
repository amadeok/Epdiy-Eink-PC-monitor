#include <2bitLUTforC_inv.h>
#include <string.h>
#include <utils.h>
#include <cstdlib>
#include <stdio.h>
#include <unistd.h>

extern int total_nb_pixels, refres_every_x_frames, nb_draws;
extern char working_dir[256];
;
int loop_counter0 = 1, loop_counter1 = 0;

void *generate_eink_framebuffer_v1(unsigned char *source_1bpp, char *padded_2bpp_framebuffer_current, char *padded_2bpp_framebuffer_previous, char *eink_framebuffer)
{ //generate eink framebuffer from 1bpp monochrome capture
    int counter = 0;

    // long t = getTick();
    for (int g = 0; g < total_nb_pixels / 4; g++)
    {
        memcpy(padded_2bpp_framebuffer_current + counter, two_bit_LUT + (source_1bpp[g] * 2), 2);
        counter += 2;
        eink_framebuffer[g] = padded_2bpp_framebuffer_previous[g] ^= (padded_2bpp_framebuffer_current[g] << 1);
    }
    //array_to_file(eink_framebuffer, total_nb_pixels / 8, working_dir, "eink_framebuffer", 0);
    //   printf("generate_eink_framebuffer_v1 took: %d\n", getTick() - t);
    // array_to_file(padded_2bpp_framebuffer_current, 10000, working_dir, "padded_2bpp_framebuffer_current", 0);
}

void generate_eink_framebuffer_v2(char *source_8bpp_current, char *source_8bpp_previous, char *source_8bpp_modified_previous, char **eink_framebuffer, int mode)
{ //generate eink framebuffer from 8bpp monochrome capture, slower than v1
    if (mode != 10)
    {
        char *eink_framebuffer_ = eink_framebuffer[0];
        memset(eink_framebuffer, 0, total_nb_pixels / 4);
        unsigned char temp_mask = 0;
        int dif_counter = 0, delta_counter = 0, delta_nb;
        for (int counter = 0; counter < total_nb_pixels; counter += 4)
        {
            temp_mask = 0;

            for (int y = 0; y < 4; y++)
            {
                if (source_8bpp_previous[counter + y] == 1 && source_8bpp_current[counter + y] == 0)
                {
                    temp_mask |= 1 << y * 2; // make pixel blacker
                }
                else if (source_8bpp_previous[counter + y] == 0 && source_8bpp_current[counter + y] == 1)
                {
                    temp_mask |= 2 << y * 2; // make pixel whiter
                }
            }
            eink_framebuffer_[delta_counter] |= temp_mask;
            delta_counter++;
        }
    }
    else
    {

        for (int x = 0; x < nb_draws; x++)
            memset(eink_framebuffer[x], 0, total_nb_pixels / 4);
        memset(source_8bpp_previous, 255, total_nb_pixels);
        array_to_file(source_8bpp_current, total_nb_pixels, working_dir, "source_8bpp_current", 0);
        array_to_file(source_8bpp_previous, total_nb_pixels, working_dir, "source_8bpp_previous", 0);
        unsigned char temp_mask = 0;
        unsigned char temp_masks[4];
        int cur = 0, prev = 0;
        int dif_counter = 0, delta_counter = 0, delta_nb;
        int n = 0;
        for (int counter = 0; counter < total_nb_pixels; counter += 4)
        {
            temp_mask = 0;
            for (int k = 0; k < nb_draws; k++)
                temp_masks[k] = 0;
            for (int y = 0; y < 4; y++)
            {

                cur = (uint8_t)source_8bpp_current[counter + y];
                prev = (uint8_t)source_8bpp_previous[counter + y];
                if (counter +y == 50)
                    n = 0;
                //n = (uint8_t)source_8bpp_previous[counter + y] - (uint8_t)source_8bpp_current[counter + y];
                switch (cur)
                {
                case 0:
                    switch (prev)
                    {
                    case 0:
                        break;
                    case 85:
                        temp_masks[0] |= 1 << y * 2; // make pixel blacker
                        break;
                    case 170:
                        for (int q = 0; q < 2; q++) // make pixel blacker
                            temp_masks[q] |= 1 << y * 2;
                        break;

                    case 255:
                        for (int q = 0; q < 3; q++) // make pixel blacker
                            temp_masks[q] |= 1 << y * 2;
                        break;
                    }
                    break;

                case 85: //cur
                    switch (prev)
                    {
                    case 0:
                        temp_masks[0] |= 2 << y * 2; // make pixel whiter
                        break;
                    case 85:
                        break;
                    case 170:
                        temp_masks[0] |= 1 << y * 2; // make pixel blacker
                        break;

                    case 255:
                        for (int q = 0; q < 2; q++)
                            temp_masks[q] |= 1 << y * 2; // make pixel blacker
                        break;
                    }
                    break;

                case 170: //cur
                    switch (prev)
                    {
                    case 0:
                        for (int q = 0; q < 2; q++)
                            temp_masks[q] |= 2 << y * 2; // make pixel whiter
                        break;
                    case 85:
                        temp_masks[0] |= 2 << y * 2; // make pixel whiter
                        break;
                    case 170:
                        break;

                    case 255:
                        temp_masks[0] |= 1 << y * 2; // make pixel blacker
                        break;
                    }
                    break;

                case 255: //cur
                    switch (prev)
                    {
                    case 0:
                        for (int q = 0; q < 3; q++)
                            temp_masks[q] |= 2 << y * 2; // make pixel blacker
                        break;
                    case 85:
                        for (int q = 0; q < 2; q++)
                            temp_masks[q] |= 2 << y * 2; // make pixel blacker
                        break;
                    case 170:
                        temp_masks[0] |= 2 << y * 2; // make pixel blacker
                        break;
                    case 255:
                        break;
                    }
                    break;
                }
            }
            delta_counter++;

            for (int k = 0; k < nb_draws; k++)
            {
                eink_framebuffer[k][delta_counter] |= temp_masks[k];
            }
        }
        for (int k = 0; k < nb_draws; k++)
        {
            array_to_file(eink_framebuffer[k], 230400, working_dir, "eink_framebuffer", k);
        }
    }
    //     array_to_file(eink_framebuffer, 230400, working_dir, "eink_framebuffer", 0);
}

void quantize(char *source_8bpp_current, char *source_8bpp_modified_current, int size)
{
    int val;

    for (int h = 0; h < size; h++)
    { // int v = source_8bpp_current[h];
        val = (uint8_t)source_8bpp_current[h] / 64;
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

void generate_eink_framebuffer_v2_with_ghost(char *source_8bpp_current, char *source_8bpp_previous, char *source_8bpp_modified_previous, char **eink_framebuffer, int nb_pixels_to_change)
{ // generate eink framebuffer and attempt to reduce ghosting (experimental)
    memset(eink_framebuffer, 0, total_nb_pixels / 4);
    unsigned char temp_mask = 0;
    int dif_counter = 0, delta_counter = 1, delta_nb;
    memcpy(source_8bpp_modified_previous, source_8bpp_previous, total_nb_pixels);

    for (int counter = nb_pixels_to_change; counter < total_nb_pixels; counter += 1)
        if (source_8bpp_previous[counter] == 1 && source_8bpp_current[counter] == 0)
        {
            for (int u = 1; u < nb_pixels_to_change; u++)
            {
                if (source_8bpp_current[counter - u] == 0)
                    source_8bpp_modified_previous[counter - u] = 1;
                if (source_8bpp_current[counter + u] == 0)
                    source_8bpp_modified_previous[counter + u] = 1;
            }
            counter += nb_pixels_to_change * 2;
        }

    if (loop_counter1 == 1)
    {
        loop_counter1 = 0;
        memset(source_8bpp_modified_previous, 1, total_nb_pixels);
        memset(source_8bpp_current, 0, total_nb_pixels);
    }
    if (loop_counter0 % 81 == 0)
    {
        memset(source_8bpp_modified_previous, 0, total_nb_pixels);
        memset(source_8bpp_current, 1, total_nb_pixels);

        loop_counter1 = 1;
    }
    for (int counter = 4; counter < total_nb_pixels; counter += 4)
    {

        temp_mask = 0;
        for (int y = 0; y < 4; y++)
        {

            if (source_8bpp_modified_previous[counter + y] == 1 && source_8bpp_current[counter + y] == 0)
            {

                temp_mask |= 1 << y * 2; // make blacker
            }
            else if (source_8bpp_modified_previous[counter + y] == 0 && source_8bpp_current[counter + y] == 1)
            {

                temp_mask |= 2 << y * 2; // make whiter
            }
        }
        eink_framebuffer[0][delta_counter] |= temp_mask;
        delta_counter++;
        //     array_to_file(eink_framebuffer, 230400, working_dir, "eink_framebuffer", 0);
    }
}