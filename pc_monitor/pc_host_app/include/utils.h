#include <stdlib.h>
#include <inttypes.h>
uint32_t getTick();

#if !defined(_WIN32)
#define HANDLE int
#define DWORD unsigned long
#define LPDWORD *DWORD.
#define SOCKET int32_t
#endif
#ifdef _WIN32
#include <windows.h>
#endif

//Write an array to a file
void array_to_file(void *array, int nb_bytes_to_write, const char *path, const char *filename, int k);

//Read a file into an array in memory
void file_to_array(char array[], int array_size, int file_size, const char *path, const char *filename, int k);

//Swaps the bytes in the framebuffer to get them in the order that the board needs
void swap_bytes(char *eink_framebuffer, char *eink_framebuffer_swapped, int eink_framebuffer_size, int source_image_bit_depth);

// if a line didn't actually change in the original 8bpp capture, set the corresponding line to 0s
// so that the dithering doesn't spoil the rest of the image
void improve_dither_compression(unsigned char *eink_framebuffer, int eink_framebuffer_size, unsigned char *line_changed, int width, int height);

//Compare a framebuffer received from the board to the one sent (for debugging)
int extract_and_compare(unsigned char *eink_framebuffer_swapped, int g);

DWORD pipe_read(HANDLE handle, void *buffer, DWORD nNumberOfBytesToRead, DWORD lpNumberOfBytesRead);
DWORD pipe_write(HANDLE handle, void *buffer, DWORD nNumberOfBytesToWrite, DWORD lpNumberOfBytesWritten);
