
// Compress a Eink framebuffer or a chunk of a Eink framebuffer
int rle_compress(unsigned char *array_to_compress, unsigned char compression_temporary_array[], int nb_chunks, unsigned char compressed_eink_framebuffer[], const int total_nb_pixels, const int chunk_size);

// Replaces unnecessary bytes with 0s to improve rle compression, not needed if using generate_eink_framebuffer_v2()
void optimize_rle(unsigned char *eink_framebuffer);

// Simpler extraction for debugging
void rle_extract2(int compressed_size, unsigned char *decompressed_p, unsigned char *compressed, int k);

// Extract a compressed framebuffer (for debugging)
void rle_extract1(unsigned char decompressed[], int nb_chunks, unsigned char compressed_eink_framebuffer[], const int eink_framebuffer_size, const int chunk_size);
