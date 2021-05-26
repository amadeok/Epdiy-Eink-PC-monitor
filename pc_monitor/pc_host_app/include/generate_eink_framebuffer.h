#include <stdlib.h>

 //Generate eink framebuffer from 1bpp monochrome captured image
void *generate_eink_framebuffer_v1(unsigned char *source_1bpp, char *padded_2bpp_framebuffer_current, char *padded_2bpp_framebuffer_previous,  char *eink_framebuffer);

 //Generate eink framebuffer from 8bpp monochrome captured image
void generate_eink_framebuffer_v2(char *source_8bpp_current, char *source_8bpp_previous, char *source_8bpp_modified_previous,  char **eink_framebuffer, int mode);


void quantize(char *source_8bpp_current, char *source_8bpp_modified_current, int size);


 // Generate eink framebuffer and attempt to reduce ghosting (experimental)
void generate_eink_framebuffer_v2_with_ghost(char *source_8bpp_current, char *source_8bpp_previous, char *source_8bpp_modified_previous,  char **eink_framebuffer, int nb_pixels_to_change);

