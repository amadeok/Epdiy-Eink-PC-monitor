#include <stdlib.h>

 //Generate eink framebuffer from 1bpp monochrome captured image
void *generate_eink_framebuffer_v1(unsigned char *source_1bpp, unsigned char *padded_2bpp_framebuffer_current, unsigned char *padded_2bpp_framebuffer_previous, unsigned char *eink_framebuffer);

 //Generate eink framebuffer from 8bpp monochrome captured image
void generate_eink_framebuffer_v2(unsigned char *source_8bpp_current, unsigned char *source_8bpp_previous, unsigned char *source_8bpp_modified_previous, unsigned char *eink_framebuffer);

 // Generate eink framebuffer and attempt to reduce ghosting (experimental)
void generate_eink_framebuffer_v2_with_ghost(unsigned char *source_8bpp_current, unsigned char *source_8bpp_previous, unsigned char *source_8bpp_modified_previous, unsigned char *eink_framebuffer, int nb_pixels_to_change);
