#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>
#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <fstream>
#include <rle_compression.h>

extern unsigned char *compressed_eink_framebuffer_pointer_array[8]; 
extern unsigned char * decompressed_received;

extern int compressed_chunk_lengths[8];
extern int chunk_size;

u_int32_t getTick()
{
    struct timespec ts;
    unsigned theTick = 0U;
    clock_gettime(CLOCK_REALTIME, &ts);
    theTick = ts.tv_nsec / 1000000;
    theTick += ts.tv_sec * 1000;
    return theTick;
}
void static array_to_file(void *array, int nb_bytes_to_write, const char *path, const char *filename, int k) 
{
    char buffer[250];
    sprintf(buffer, "%s/%s%d", path, filename, k);
    FILE *f3 = fopen(buffer, "wb");
    fwrite(array, sizeof(char), nb_bytes_to_write, f3);
    fflush(f3);
    fclose(f3);
}
void file_to_array(char array[], int array_size, int file_size, const char *path, const char *filename, int k) 
{
    char buffer[250];
    sprintf(buffer, "%s%s", path, filename);
    //array =  (char*)malloc( array_size * sizeof(char));
    std::string inFileName = buffer;
    std::ifstream inFile(inFileName, std::ios::binary);
    int begin = inFile.tellg();
    inFile.seekg(0, std::ios::end);
    int end = inFile.tellg();
    inFile.seekg(0, std::ios::beg);
    inFile.read(array, file_size);
    inFile.close();
}

void swap_bytes(unsigned char *eink_framebuffer, unsigned char *eink_framebuffer_swapped, int eink_framebuffer_size, int source_image_bit_depth)
{   //swapping bytes is necessary to get them in the order that the board needs
    //   long t = getTick();
    if (source_image_bit_depth == 1)
    {
        for (int h = 0; h < eink_framebuffer_size; h += 4)
        {
            // memcpy(temp_arr, eink_framebuffer + h, 4);
            eink_framebuffer_swapped[h] = eink_framebuffer[h + 3];
            eink_framebuffer_swapped[h + 1] = eink_framebuffer[h + 2];
            eink_framebuffer_swapped[h + 2] = eink_framebuffer[h + 1];
            eink_framebuffer_swapped[h + 3] = eink_framebuffer[h + 0];
        }
    }
    else if (source_image_bit_depth == 8)
    {
        for (int h = 0; h < eink_framebuffer_size; h += 4)
        {
            // memcpy(temp_arr, eink_framebuffer + h, 4);
            eink_framebuffer_swapped[h] = eink_framebuffer[h + 2];
            eink_framebuffer_swapped[h + 1] = eink_framebuffer[h + 3];
            eink_framebuffer_swapped[h + 2] = eink_framebuffer[h + 0];
            eink_framebuffer_swapped[h + 3] = eink_framebuffer[h + 1];
        }
    }
    //  printf("swapping took: %d\n", getTick() - t);
}

int extract_and_compare(unsigned char *eink_framebuffer_swapped, int g)
{
    rle_extract2(compressed_chunk_lengths[g], decompressed_received, compressed_eink_framebuffer_pointer_array[g], g);

    for (int h = 0; h < chunk_size - 2; h++)
    {
        if (decompressed_received[h] != eink_framebuffer_swapped[(g * chunk_size) + h])
        {
            printf("arrays are different %d %d \n", decompressed_received[h], eink_framebuffer_swapped[(g * chunk_size) + h]);
            sleep(10);
            return h;
        }
    }
}