#include <stdlib.h>
#include <inttypes.h>
u_int32_t getTick();

//Write an array to a file
void  array_to_file(void *array, int nb_bytes_to_write, const char *path, const char *filename, int k); 

//Read a file into an array in memory
void file_to_array(char array[], int array_size, int file_size, const char *path, const char *filename, int k); 

//Swaps the bytes in the framebuffer to get them in the order that the board needs
void swap_bytes(unsigned char *eink_framebuffer, unsigned char *eink_framebuffer_swapped, int eink_framebuffer_size, int source_image_bit_depth);

//Compare a framebuffer received from the board to the one sent (for debugging)
int extract_and_compare(unsigned char *eink_framebuffer_swapped, int g);
