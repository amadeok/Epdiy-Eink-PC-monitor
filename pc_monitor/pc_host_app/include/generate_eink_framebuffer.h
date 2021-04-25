#include <stdlib.h>

 //Generate eink framebuffer from 1bpp monochrome captured image
void *generate_eink_framebuffer_v1(unsigned char *source_1bpp, char *padded_2bpp_framebuffer_current, char *padded_2bpp_framebuffer_previous,  char *eink_framebuffer);

 //Generate eink framebuffer from 8bpp monochrome captured image
void generate_eink_framebuffer_v2(unsigned char *source_8bpp_current, unsigned char *source_8bpp_previous, unsigned char *source_8bpp_modified_previous,  char *eink_framebuffer);

void generate_filter_framebuffer(unsigned char *source_8bpp_current, unsigned char *source_8bpp_previous,  char *filter_framebuffer);

void filter_unwanted_dither( char * eink_framebuffer,  char * filter_framebuffer,  char * eink_framebuffer_modified, int eink_framebuffer_size);


 // Generate eink framebuffer and attempt to reduce ghosting (experimental)
void generate_eink_framebuffer_v2_with_ghost(unsigned char *source_8bpp_current, unsigned char *source_8bpp_previous, unsigned char *source_8bpp_modified_previous,  char *eink_framebuffer, int nb_pixels_to_change);
