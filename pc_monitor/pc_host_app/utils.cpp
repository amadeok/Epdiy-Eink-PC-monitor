#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>
#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <fstream>
#include <rle_compression.h>

#ifdef _WIN32
#include <windows.h>
#endif

extern unsigned char *compressed_eink_framebuffer_ptrs[8];
extern unsigned char *decompressed_received;

extern int compressed_chunk_lengths[8];
extern int chunk_size;

uint32_t getTick()
{
    struct timespec ts;
    unsigned theTick = 0U;
    clock_gettime(CLOCK_REALTIME, &ts);
    theTick = ts.tv_nsec / 1000000;
    theTick += ts.tv_sec * 1000;
    return theTick;
}
void array_to_file(void *array, int nb_bytes_to_write, const char *path, const char *filename, int k)
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

void swap_bytes(char *eink_framebuffer, char *eink_framebuffer_swapped, int eink_framebuffer_size, int source_image_bit_depth)
{ //swapping bytes is necessary to get them in the order that the board needs
    //   long t = getTick();

    for (int h = 0; h < eink_framebuffer_size; h += 4)
    {
        // memcpy(temp_arr, eink_framebuffer + h, 4);
        eink_framebuffer_swapped[h] = eink_framebuffer[h + 2];
        eink_framebuffer_swapped[h + 1] = eink_framebuffer[h + 3];
        eink_framebuffer_swapped[h + 2] = eink_framebuffer[h + 0];
        eink_framebuffer_swapped[h + 3] = eink_framebuffer[h + 1];
    }

    //  printf("swapping took: %d\n", getTick() - t);
}

void improve_dither_compression(unsigned char *eink_framebuffer, int eink_framebuffer_size, unsigned char *line_changed, int width, int height)
{
    int line_size = width / 4;
    for (int h = 0; h < height; h++)
    {
        if (line_changed[h] == 0)
            memset(eink_framebuffer + h * line_size, 0, line_size);
    }
}

int extract_and_compare(unsigned char *eink_framebuffer_swapped, int g)
{
    rle_extract2(compressed_chunk_lengths[g], decompressed_received, compressed_eink_framebuffer_ptrs[g], g);

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

DWORD pipe_read(HANDLE handle, void *buffer, DWORD nNumberOfBytesToRead, DWORD lpNumberOfBytesRead)
{
    DWORD ret;
#ifdef __linux__
    lpNumberOfBytesRead = read(HANDLE, buffer, nNumberOfBytesToRead * sizeof(unsigned char));
#elif _WIN32
    ReadFile(handle, buffer, sizeof(unsigned char) * nNumberOfBytesToRead, &lpNumberOfBytesRead, NULL);
#endif
    return lpNumberOfBytesRead;
}

DWORD pipe_write(HANDLE handle, void *buffer, DWORD nNumberOfBytesToWrite, DWORD lpNumberOfBytesWritten)

{
#ifdef __linux__
    lpNumberOfBytesWritten = write(HANDLE, buffer, nNumberOfBytesToWrite);
#elif _WIN32
    WriteFile(handle, buffer, nNumberOfBytesToWrite, &lpNumberOfBytesWritten, NULL);
#endif
    return lpNumberOfBytesWritten;
}
