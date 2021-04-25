#include <string.h>
#include <stdlib.h>
#include <utils.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
extern char *compressed_eink_framebuffer_ptrs[8];
extern unsigned char *array_with_zeros, *draw_white_bytes, *draw_black_bytes;
extern int compressed_chunk_lengths[8];
extern int eink_framebuffer_size, chunk_size;
extern char working_dir[256];

void rle_extract1(char *decompressed, int nb_chunks, char *eink_framebuffer_swapped, const int eink_framebuffer_size, int compressed_size)
{
    int counter = 0, counter2 = 0, id = 0, i = 0, j = 0, offset = 0, k;
    //  int plus10 = 0, plus50 = 0, plus30 = 0, plus70 = 0, plus90 = 0, minus = 0; //for testing
    for (k = 0; k < nb_chunks; k++)
    {
        compressed_size = compressed_chunk_lengths[k];
        counter2 = 0, counter = 0;
        while (counter < compressed_size)
        {

            i = (char)compressed_eink_framebuffer_ptrs[k][counter++];
            j = compressed_eink_framebuffer_ptrs[k][counter++] & 0xff;
            if (i < 0)
            {
                offset = (i + 130);
                switch (j)
                {
                case 0:
                    memcpy(decompressed + counter2, array_with_zeros, offset);
                    counter2 += offset;
                    break;
                case 85:
                    memcpy(decompressed + counter2, draw_black_bytes, offset);
                    counter2 += offset;
                    break;
                case 170:
                    memcpy(decompressed + counter2, draw_white_bytes, offset);
                    counter2 += offset;
                    break;
                default:
                    for (int f = 0; f < i + 130; f++)
                    {
                        decompressed[counter2] = j;
                        counter2++;
                    }
                    break;
                }
            }
            else if (i >= 0)
            {
                for (int f = 0; f < i; f++)
                {
                    decompressed[counter2] = j;
                    j = compressed_eink_framebuffer_ptrs[k][counter++] & 0xff;
                    counter2++;
                }
                decompressed[counter2] = j;
                counter2++;
            }
        }
        //for testing and debugging:
        // if (i < 0)
        //     minus++;
        // if (i > 10)
        // {
        //     plus10++;
        //     if (i > 30)
        //     {
        //         plus30++;
        //         if (i > 50)
        //         {
        //             plus50++;
        //             if (i > 70)
        //             {
        //                 plus70++;
        //                 if (i > 90)
        //                     plus90++;
        //             }
        //         }
        //     }
        // }
        //   array_to_file(decompressed, counter2, working_dir, "decompressed", 0);
        for (int h = 0; h < chunk_size; h++) // for debugging
        {
            if (decompressed[h] != eink_framebuffer_swapped[k * chunk_size + h])
                printf(" d ");
        }
    }
    //printf("10, 30, 50, 70, 90,  is %d, %d, %d, %d, %d, minus: %d \n", plus10, plus30, plus50, plus70, plus90, minus);
}

int rle_compress(char *array_to_compress, char *tmp_array, int nb_chunks, char *compressed_eink_framebuffer, const int total_nb_pixels, int chunk_size)
{

    uint8_t t[256];
    int source_size = total_nb_pixels;

    int i = 0, keep, end_of_file = 0, counter = 0, counter2;
    long t1 = getTick();
    // array_to_file(array_to_compress, total_nb_pixels, working_dir, "array_to_compress", switcher);
    //  array_to_file(eink_framebuffer_swapped, total_nb_pixels / 2, working_dir, "eink_framebuffer_swapped", 0);
    int k;
    for (k = 0; k < nb_chunks; k++)
    {
        memcpy(tmp_array, array_to_compress + (k * chunk_size), chunk_size);
        // array_to_file(source, chunk_size, working_dir, "source", k);
       // array_to_file(array_to_compress + (k * chunk_size), chunk_size, working_dir, "atc", k);
        end_of_file = 0;
        counter = 0, counter2 = 0;
        if (nb_chunks == 2)
        {
            if (k == 0)
                chunk_size = 123600;
            else
                chunk_size = 123900;
        }
        t[0] = tmp_array[counter++];
        while (counter < chunk_size)
        {
            if (end_of_file == 1)
                break;
            t[1] = tmp_array[counter++];
            if (t[0] != t[1]) // uncompressible sequence
            {
                i = 1;
                if (counter < chunk_size + 1)
                    do
                    {
                        t[++i] = tmp_array[counter++];
                        if (counter >= chunk_size)
                        {
                            end_of_file = 1;
                            break;
                        }
                    } while (counter < chunk_size && i < 128 && t[i] != t[i - 1]);
                if ((keep = t[i] == t[i - 1]))
                    --i;

                compressed_eink_framebuffer_ptrs[k][counter2++] = i - 1;
                memcpy(compressed_eink_framebuffer_ptrs[k] + counter2, t, i);
                counter2 += i;

                t[0] = t[i];
                if (!keep)
                    continue; // size too large or EOF
            }
            // compressible sequence
            i = 2;
            do
            {
                t[1] = tmp_array[counter++];
                if (counter >= chunk_size)
                {
                    end_of_file = 1;
                    break;
                }
            } while (++i < 130 && t[0] == t[1]);

            compressed_eink_framebuffer_ptrs[k][counter2++] = i + 125;
            compressed_eink_framebuffer_ptrs[k][counter2++] = t[0];

            t[0] = t[1];
        }
        compressed_chunk_lengths[k] = counter2;
      //  array_to_file(compressed_eink_framebuffer_ptrs[k], counter2, working_dir, "cefv1", k);

        //  printf("counter2 is %d \n", counter2);
    }

    //printf("compressing took %d \n", getTick() - t1);
    return counter2;
}

int rle_compress_v2(unsigned char *array_to_compress, unsigned char tmp_array[], int nb_chunks, uint16_t **added_compression_arr, const int chunk_size)
{

    uint8_t t[256];
    int i = 0, keep, end_of_file = 0, counter = 0, counter2, counter3 = 0;
    long t1 = getTick();
    array_to_file(array_to_compress, chunk_size / 100, working_dir, "array_to_compress", 0);
    //array_to_file(eink_framebuffer_swapped, total_nb_pixels / 2, working_dir, "eink_framebuffer_swapped", 0);
    int k;
    for (k = 0; k < nb_chunks; k++)
    {
        memcpy(tmp_array, array_to_compress + (k * chunk_size), chunk_size);
        // array_to_file(source, chunk_size, working_dir, "source", k);
        end_of_file = 0;
        counter = 0, counter2 = 0;

        t[0] = tmp_array[counter++];
        while (counter < chunk_size)
        {
            if (end_of_file == 1)
                break;
            t[1] = tmp_array[counter++];
            if (t[0] != t[1]) // uncompressible sequence
            {
                i = 1;
                if (counter < chunk_size + 1)
                    do
                    {
                        t[++i] = tmp_array[counter++];
                        if (counter >= chunk_size)
                        {
                            end_of_file = 1;
                            break;
                        }
                    } while (counter < chunk_size && i < 128 && t[i] != t[i - 1]);

                if ((keep = t[i] == t[i - 1]))
                    --i;
                int i2 = i;

                if (i % 2 != 0)
                {
                    t[++i] = tmp_array[counter++];
                    // i2--;
                }
                compressed_eink_framebuffer_ptrs[k][counter2++] = i - 1;

                memcpy(added_compression_arr[k] + counter3 / 2, t, i);
                memcpy(compressed_eink_framebuffer_ptrs[k] + counter2, array_with_zeros, i2);

                counter3 += i;
                counter2 += i2;

                t[0] = t[i];
                if (!keep)
                    continue; // size too large or EOF
            }
            // compressible sequence
            i = 2;
            do
            {
                t[1] = tmp_array[counter++];
                if (counter >= chunk_size)
                {
                    end_of_file = 1;
                    break;
                }
            } while (++i < 130 && t[0] == t[1]);

            compressed_eink_framebuffer_ptrs[k][counter2++] = i + 125;
            compressed_eink_framebuffer_ptrs[k][counter2++] = t[0];

            t[0] = t[1];
            array_to_file(compressed_eink_framebuffer_ptrs[k], counter2, working_dir, "cef", k);
            array_to_file(added_compression_arr[k], counter3, working_dir, "aca", k);
        }
        compressed_chunk_lengths[k] = counter2;
        // array_to_file(compressed_eink_framebuffer_ptrs[k], counter2, working_dir, "cef", k);

        //  printf("counter2 is %d \n", counter2);
    }

    //printf("compressing took %d \n", getTick() - t1);
    return counter2;
}

void optimize_rle(char *eink_framebuffer)
{
    for (int y = 0; y < eink_framebuffer_size; y++)
    {
        int currentent_nb = eink_framebuffer[y];
        switch (currentent_nb)
        {
        case 255:
            eink_framebuffer[y] = 0;
            break;
        case 192:
            eink_framebuffer[y] = 0;
            break;
        case 48:
            eink_framebuffer[y] = 0;
            break;
        case 12:
            eink_framebuffer[y] = 0;
            break;
        case 3:
            eink_framebuffer[y] = 0;
            break;
        case 240:
            eink_framebuffer[y] = 0;
            break;
        case 15:
            eink_framebuffer[y] = 0;
            break;
        case 252:
            eink_framebuffer[y] = 0;
            break;
        case 63:
            eink_framebuffer[y] = 0;
            break;
        case 60:
            eink_framebuffer[y] = 0;
            break;
        case 195:
            eink_framebuffer[y] = 0;
            break;
        case 204:
            eink_framebuffer[y] = 0;
            break;
        case 51:
            eink_framebuffer[y] = 0;
            break;
        case 207:
            eink_framebuffer[y] = 0;
            break;
        case 243:
            eink_framebuffer[y] = 0;
            break;
        }
    }
}

void rle_extract2(int compressed_size, unsigned char *decompressed_p, unsigned char *compressed, int k)
{
    int counter, counter2, j = 0;

    counter2 = 0, counter = 0;
    int offset = 0;
    int8_t i = 0;
    while (counter < compressed_size)
    {
        i = compressed[counter];
        counter++;
        j = compressed[counter] & 0xff;
        counter++;

        if (i < 0)
        {
            offset = (i + 130);
            switch (j)
            {
            case 0:
                memcpy(decompressed_p + counter2, array_with_zeros, offset);
                counter2 += offset;

                break;
            default:
                for (int f = 0; f < i + 130; f++)
                {
                    decompressed_p[counter2] = j;
                    counter2++;
                }
                break;
            }
        }
        else if (i >= 0)
        {
            for (int f = 0; f < i; f++)
            {
                decompressed_p[counter2] = j;

                j = compressed[counter++] & 0xff;
                counter2++;
            }
            decompressed_p[counter2] = j;
            counter2++;
        }
    }
}