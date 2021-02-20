#include <cstdlib>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>
#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <fstream>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr
#include <64bitLUTc.h>
//#include <fill_adj_v2_inv.h>
#include <netinet/tcp.h>
#include <utils.h>
#include <rle_compression.h>
#include <generate_eink_framebuffer.h>

char *esp32_ip_address = "192.168.43.109";
int width_resolution = 1200, height_resolution = 825;

unsigned char compressed_chunk_lengths_in_bytes[16];

char *fifo0 = "/tmp/epdiy_pc_monitor0";
char *fifo1 = "/tmp/epdiy_pc_monitor1";

char working_dir[256];
unsigned char *array_with_zeros;

int socket_desc;

int compressed_chunk_lengths[8];

unsigned char *decompressed;          // for testing or debugging
unsigned char *decompressed_received; // for testing or debugging
unsigned char *compressed_received;   // for testing or debugging
unsigned char *received;  // for testing or debugging
unsigned char *compression_temporary_array;

unsigned char *compressed_eink_framebuffer_pointer_array[8]; //array of pointers pointing to chunks of framebuffer

int total_nb_pixels, eink_framebuffer_size, chunk_size, nb_chunks;
int source_image_bit_depth = 8;

  uint8_t ready0[6];
  uint8_t ready1[6];

int fd0, fd1;




void wifi_transfer(unsigned char compressed_eink_framebuffer[], int compressed_framebuffer_size)
{

    int ret = 0, buf_size = 4096, tot = 0, tot2 = 0, len = 0;
    send(socket_desc, "ready1", 6, 0);
    send(socket_desc, compressed_chunk_lengths_in_bytes, 16 * sizeof(unsigned char), 0);
    //   array_to_file(eink_framebuffer_swapped, eink_framebuffer_size, working_dir, "eink_framebuffer_swapped", 0);
    unsigned char ready[5];
    int g;
    for (g = 0; g < nb_chunks; g++)
    {
        buf_size = 4096;
        int len2;
        tot = 0;
        ret = 0;
        int converted_number = 0;
        int compressed_chunk_size = compressed_chunk_lengths[g];
        if (compressed_chunk_size < buf_size)
            buf_size = compressed_chunk_size;
        do
        {
            ret = send(socket_desc, compressed_eink_framebuffer_pointer_array[g] + tot, buf_size, 0);
            tot += ret;
            if (ret == -1)
                printf("error transfer wifi returneed -1\n");
            if (compressed_chunk_size - tot < buf_size)
                buf_size = compressed_chunk_size - tot;
        } while (tot < compressed_chunk_size);

        if (tot != compressed_chunk_size)
        {
            printf("warning tot != compress size\n");
        }
    }

    //puts("Data Send to esp\n");
    //   array_to_file(received, eink_framebuffer_size, working_dir, "received", 0);
}




static int mirroring_task()
{

    fd0 = open(fifo0, O_RDONLY);
    fd1 = open(fifo1, O_WRONLY);
    int change_pipe_size = true;
    if (change_pipe_size == true)
    {
        long pipe_size = (long)fcntl(fd0, F_GETPIPE_SZ);
        int ret = fcntl(fd0, F_SETPIPE_SZ, 1048576);
        std::string the_ret = "";
        pipe_size = (long)fcntl(fd0, F_GETPIPE_SZ);
        std::string string_buf = "";
        printf("new pipe size: %ld\n", pipe_size);
    }
    int width_resolution = 1200, height_resolution = 825;
    nb_chunks = 5; // number of pieces into which divide the framebuffer (for multiprocessing)

    total_nb_pixels = width_resolution * height_resolution;
    eink_framebuffer_size = total_nb_pixels / 4;
    chunk_size = (eink_framebuffer_size / nb_chunks) + 10000;
    int loop_counter = 0;
    unsigned char ack[1] = {246};
    unsigned char ack2[1];
    unsigned char ready0[6];
    unsigned char ready1[6];


    int tot = 0, pos = 0, ret2 = 0;

    int compressed_framebuffer_size = 0;

    unsigned char line_changed[height_resolution]; //array containing 1 or 0 depending on whether the corresponding line on the screen has changed

    unsigned char *source_8bpp_current;           // array containing the current monochrome 8bpp screen capture
    unsigned char *source_8bpp_previous;          //array containing the previous monochrome 8bpp screen capture
    unsigned char *source_8bpp_modified_current;  //same as 'source_8bpp_current' but has been modified
    unsigned char *source_8bpp_modified_previous; //same as 'source_8bpp_previous' but has been modified

    unsigned char *eink_framebuffer; // the 2bpp array that will be fed directly to the display

    unsigned char *eink_framebuffer_modified; // an 'eink_framebuffer' that has been modified

    unsigned char *eink_framebuffer_swapped; // an 'eink_framebuffer' with bytes swapped

    unsigned char compressed_eink_framebuffer[eink_framebuffer_size]; // an 'eink_framebuffer' that has been compressed with RLE compression

    unsigned char *padded_2bpp_framebuffer_current;  //array containing the current monochrome 2bpp screen capture
    unsigned char *padded_2bpp_framebuffer_previous; //array containing the previous monochrome 2bpp screen capture


    unsigned char *source_1bpp; //array containing the current monochrome 1bpp screen capture

    source_1bpp = (unsigned char *)calloc(total_nb_pixels, sizeof(unsigned char));
    compression_temporary_array = (unsigned char *)calloc(eink_framebuffer_size / 4, sizeof(unsigned char));

    padded_2bpp_framebuffer_current = (unsigned char *)calloc(eink_framebuffer_size, sizeof(unsigned char));
    padded_2bpp_framebuffer_previous = (unsigned char *)calloc(eink_framebuffer_size, sizeof(unsigned char));

    source_8bpp_modified_current = (unsigned char *)calloc(total_nb_pixels, sizeof(unsigned char));
    source_8bpp_modified_previous = (unsigned char *)calloc(total_nb_pixels, sizeof(unsigned char));
    source_8bpp_current = (unsigned char *)calloc(total_nb_pixels, sizeof(unsigned char));
    source_8bpp_previous = (unsigned char *)calloc(total_nb_pixels, sizeof(unsigned char));

    eink_framebuffer = (unsigned char *)calloc(eink_framebuffer_size, sizeof(unsigned char));
    eink_framebuffer_modified = (unsigned char *)calloc(eink_framebuffer_size, sizeof(unsigned char));
    eink_framebuffer_swapped = (unsigned char *)calloc(eink_framebuffer_size, sizeof(unsigned char));
    decompressed = (unsigned char *)calloc(eink_framebuffer_size, sizeof(unsigned char));

    for (int h = 0; h < 5; h++)
        compressed_eink_framebuffer_pointer_array[h] = (unsigned char *)calloc(chunk_size + 10000, sizeof(unsigned char));

    if (source_image_bit_depth == 1)
    { //assume the first screen capture to be completely white
        memset(padded_2bpp_framebuffer_previous, 85, eink_framebuffer_size * sizeof(unsigned char));
        memset(padded_2bpp_framebuffer_current, 85, eink_framebuffer_size * sizeof(unsigned char));
    }
    else if (source_image_bit_depth == 8)
    { //assume the first screen capture to be completely white
        memset(source_8bpp_current, 1, total_nb_pixels * sizeof(unsigned char));
        memset(source_8bpp_previous, 1, total_nb_pixels * sizeof(unsigned char));
    }
    else
    {
        printf("unsupported bit depth\n");
        return -1;
    }
        int total = 0;

    while (1)
    {   if (total != 0) // if screen didn't change don't wait for ack from board
        recv(socket_desc, ready0, 6, 0);
        write(fd1, ack, 1);
        read(fd0, ack2, 1);
        long t0 = getTick();
        read(fd0, line_changed, height_resolution);

        if (source_image_bit_depth == 1)
            ret2 = read(fd0, source_1bpp, total_nb_pixels / 8 * sizeof(unsigned char));
        else if (source_image_bit_depth == 8)
        {
            memcpy(source_8bpp_previous, source_8bpp_current, total_nb_pixels * sizeof(unsigned char));
            ret2 = read(fd0, source_8bpp_current, total_nb_pixels * sizeof(unsigned char));
        }

        loop_counter++;

        if (source_image_bit_depth == 1)
        {
            memcpy(padded_2bpp_framebuffer_previous, padded_2bpp_framebuffer_current, eink_framebuffer_size);
            generate_eink_framebuffer_v1(source_1bpp, padded_2bpp_framebuffer_current, padded_2bpp_framebuffer_previous, eink_framebuffer);
            optimize_rle(eink_framebuffer);
        }
        else if (source_image_bit_depth == 8)
        {
            generate_eink_framebuffer_v2(source_8bpp_current, source_8bpp_previous, source_8bpp_modified_previous, eink_framebuffer);
        }
        swap_bytes(eink_framebuffer, eink_framebuffer_swapped, eink_framebuffer_size, source_image_bit_depth);
        //array_to_file(eink_framebuffer_swapped, eink_framebuffer_size / 2, working_dir, "eink_framebuffer_swapped", 0);
        //compressed_framebuffer_size = rle_compress(eink_framebuffer_swapped, source, 1, switcher, compressed_eink_framebuffer, eink_framebuffer_size, eink_framebuffer_size);

        rle_compress(eink_framebuffer_swapped, compression_temporary_array, 5, compressed_eink_framebuffer, eink_framebuffer_size, eink_framebuffer_size / 5);

        for (int g = 0; g < 5 * 2; g += 2)
        {
            int foo = compressed_chunk_lengths[g / 2];
            compressed_chunk_lengths_in_bytes[g] = (unsigned)foo & 0xff;   // LSB mask the lower 8 bits
            compressed_chunk_lengths_in_bytes[g + 1] = (unsigned)foo >> 8; // MSB shift the higher 8 bits
            tot += foo;
        }
        // rle_extract1(decompressed, 1, compressed_eink_framebuffer, total_nb_pixels, total_nb_pixels); //for testing

        //  send(socket_desc, dif_list, height_resolution, 0);

       // for (int h = 0; h < 5; h++) // for debugging
        //    printf(" %5d ", compressed_chunk_lengths[h]);
        //printf("\n");

        total = 0;
        for (int h = 0; h < height_resolution; h++)
            total += line_changed[h];
        
        if (total != 0) //send framebuffer only if current capture is different than previous
            wifi_transfer(compressed_eink_framebuffer, compressed_framebuffer_size);

          printf("processing time %dms\n", getTick() - t0);
    }

    return 0;
}

int main(int argc, char *argv[])
{
    mkfifo(fifo0, 0666);
    getcwd(working_dir, sizeof(working_dir));
    //printf("current working directory is: %s\n", working_dir);

    array_with_zeros = (unsigned char *)calloc(129, sizeof(unsigned char));

    //Create socket
    struct sockaddr_in server;
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_desc == -1)
    {
        printf("Could not create socket");
    }
    int yes = 0; // 1 - on, 0 - off
    int result = setsockopt(socket_desc,
                            IPPROTO_TCP,
                            TCP_NODELAY,
                            (char *)&yes,
                            sizeof(int));
    if (result < 0)
        printf("error setting socket options\n");
    server.sin_addr.s_addr = inet_addr(esp32_ip_address);
    //server.sin_addr.s_addr = inet_addr("192.168.1.27");
    server.sin_family = AF_INET;
    server.sin_port = htons(3333);
    int enable_wifi = 1;
    //Connect to remote server
    if (enable_wifi == 1)
    {
        if (connect(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0)
        {
            puts("PC-host application Error: \n Error connecting to esp, wrong ESP32 ip address or wifi network?\n");
            return 1;
        }
        puts("Connected to esp wifi\n");
    }

    mirroring_task();   //Start the mirroring process
}
