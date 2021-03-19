#include <2bitLUTforC_inv.h>
#include <string.h>

extern int total_nb_pixels, refres_every_x_frames;

int loop_counter0 = 1, loop_counter1 = 0;

void *generate_eink_framebuffer_v1(unsigned char *source_1bpp, unsigned char *padded_2bpp_framebuffer_current, unsigned char *padded_2bpp_framebuffer_previous, unsigned char *eink_framebuffer)
{   //generate eink framebuffer from 1bpp monochrome capture
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

void generate_eink_framebuffer_v2(unsigned char *source_8bpp_current, unsigned char *source_8bpp_previous, unsigned char *source_8bpp_modified_previous, unsigned char *eink_framebuffer)
{   //generate eink framebuffer from 8bpp monochrome capture, slower than v1
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
        eink_framebuffer[delta_counter] |= temp_mask;
        delta_counter++;
    }
    //     array_to_file(eink_framebuffer, 230400, working_dir, "eink_framebuffer", 0);
}


void generate_eink_framebuffer_v2_with_ghost(unsigned char *source_8bpp_current, unsigned char *source_8bpp_previous, unsigned char *source_8bpp_modified_previous, unsigned char *eink_framebuffer, int nb_pixels_to_change)
{   // generate eink framebuffer and attempt to reduce ghosting (experimental)
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
        eink_framebuffer[delta_counter] |= temp_mask;
        delta_counter++;
        //     array_to_file(eink_framebuffer, 230400, working_dir, "eink_framebuffer", 0);
    }
}