#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <main.h>
extern char *dir;
const void fill_adj(unsigned char *delta_array, unsigned char *delta_array_modded)
{
    unsigned char ori_val = 0, modded_val = 0;
    int counter = 0;
    long t = getTick();
    bool overlaps = false;
    memcpy(delta_array_modded, delta_array, 990000 / 4);
    for (int g = 0; g < 990000 / 4; g++)
    {
        modded_val = 0;
        ori_val = delta_array[g];
        if (overlaps == true && ori_val << 6 == 0)
        {
            delta_array_modded[g] += 1;
        }
        overlaps = false;
        switch (ori_val)
        {
        case 1:                         // 00000001
            delta_array_modded[g] |= 5; // 00000101

            break;
        case 4:                          // 00000100
            delta_array_modded[g] |= 21; // 00010101

            break;
        case 5:                          // 00000101
            delta_array_modded[g] |= 21; // 00010101

            break;
        case 16:                         // 00010000
            delta_array_modded[g] |= 84; // 01010100

            break;
        case 17:                         // 00010001
            delta_array_modded[g] |= 85; // 01010101

            break;
        case 18:                         // 00010010
            delta_array_modded[g] |= 86; // 01010110

            break;
        case 19:                         // 00010011
            delta_array_modded[g] |= 87; // 01010111

            break;
        case 21:                         // 00010101
            delta_array_modded[g] |= 85; // 01010101

            break;
        case 25:                         // 00011001
            delta_array_modded[g] |= 89; // 01011001

            break;
        case 29:                         // 00011101
            delta_array_modded[g] |= 93; // 01011101

            break;
        case 33:                         // 00100001
            delta_array_modded[g] |= 37; // 00100101

            break;
        case 49:                         // 00110001
            delta_array_modded[g] |= 53; // 00110101

            break;
        case 64:                         // 01000000
            delta_array_modded[g] |= 80; // 01010000
            overlaps = true;
            break;
        case 65:                         // 01000001
            delta_array_modded[g] |= 85; // 01010101
            overlaps = true;
            break;
        case 66:                         // 01000010
            delta_array_modded[g] |= 82; // 01010010
            overlaps = true;
            break;
        case 67:                         // 01000011
            delta_array_modded[g] |= 83; // 01010011
            overlaps = true;
            break;
        case 68:                         // 01000100
            delta_array_modded[g] |= 85; // 01010101
            overlaps = true;
            break;
        case 69:                         // 01000101
            delta_array_modded[g] |= 85; // 01010101
            overlaps = true;
            break;
        case 70:                         // 01000110
            delta_array_modded[g] |= 86; // 01010110
            overlaps = true;
            break;
        case 71:                         // 01000111
            delta_array_modded[g] |= 87; // 01010111
            overlaps = true;
            break;
        case 72:                         // 01001000
            delta_array_modded[g] |= 88; // 01011000
            overlaps = true;
            break;
        case 73:                         // 01001001
            delta_array_modded[g] |= 89; // 01011001
            overlaps = true;
            break;
        case 74:                         // 01001010
            delta_array_modded[g] |= 90; // 01011010
            overlaps = true;
            break;
        case 75:                         // 01001011
            delta_array_modded[g] |= 91; // 01011011
            overlaps = true;
            break;
        case 76:                         // 01001100
            delta_array_modded[g] |= 92; // 01011100
            overlaps = true;
            break;
        case 77:                         // 01001101
            delta_array_modded[g] |= 93; // 01011101
            overlaps = true;
            break;
        case 78:                         // 01001110
            delta_array_modded[g] |= 94; // 01011110
            overlaps = true;
            break;
        case 79:                         // 01001111
            delta_array_modded[g] |= 95; // 01011111
            overlaps = true;
            break;
        case 80:                         // 01010000
            delta_array_modded[g] |= 84; // 01010100
            overlaps = true;
            break;
        case 81:                         // 01010001
            delta_array_modded[g] |= 85; // 01010101
            overlaps = true;
            break;
        case 82:                         // 01010010
            delta_array_modded[g] |= 86; // 01010110
            overlaps = true;
            break;
        case 83:                         // 01010011
            delta_array_modded[g] |= 87; // 01010111
            overlaps = true;
            break;
        case 84:                         // 01010100
            delta_array_modded[g] |= 85; // 01010101
            overlaps = true;
            break;
        case 97:                          // 01100001
            delta_array_modded[g] |= 101; // 01100101
            overlaps = true;
            break;
        case 100:                         // 01100100
            delta_array_modded[g] |= 101; // 01100101
            overlaps = true;
            break;
        case 113:                         // 01110001
            delta_array_modded[g] |= 117; // 01110101
            overlaps = true;
            break;
        case 116:                         // 01110100
            delta_array_modded[g] |= 117; // 01110101
            overlaps = true;
            break;
        case 129:                         // 10000001
            delta_array_modded[g] |= 133; // 10000101

            break;
        case 132:                         // 10000100
            delta_array_modded[g] |= 149; // 10010101

            break;
        case 133:                         // 10000101
            delta_array_modded[g] |= 149; // 10010101

            break;
        case 145:                         // 10010001
            delta_array_modded[g] |= 149; // 10010101

            break;
        case 161:                         // 10100001
            delta_array_modded[g] |= 165; // 10100101

            break;
        case 177:                         // 10110001
            delta_array_modded[g] |= 181; // 10110101

            break;
        case 193:                         // 11000001
            delta_array_modded[g] |= 197; // 11000101

            break;
        case 196:                         // 11000100
            delta_array_modded[g] |= 213; // 11010101

            break;
        case 197:                         // 11000101
            delta_array_modded[g] |= 213; // 11010101

            break;
        case 209:                         // 11010001
            delta_array_modded[g] |= 213; // 11010101

            break;
        case 225:                         // 11100001
            delta_array_modded[g] |= 229; // 11100101

            break;
        case 96: // 01100000
                 //modded_val |= delta_array[g];
            overlaps = true;
            break;
        case 112: // 01110000
                  //modded_val |= delta_array[g];
            overlaps = true;
            break;
        case 88: // 01011000
                 //modded_val |= delta_array[g];
            overlaps = true;
            break;
        case 104: // 01101000
                  //modded_val |= delta_array[g];
            overlaps = true;
            break;
        case 120: // 01111000
                  //modded_val |= delta_array[g];
            overlaps = true;
            break;
        case 92: // 01011100
                 //modded_val |= delta_array[g];
            overlaps = true;
            break;
        case 108: // 01101100
                  //modded_val |= delta_array[g];
            overlaps = true;
            break;
        case 124: // 01111100
                  //modded_val |= delta_array[g];
            overlaps = true;
            break;
        case 85: // 01010101
                 //modded_val |= delta_array[g];
            overlaps = true;
            break;
        case 101: // 01100101
                  //modded_val |= delta_array[g];
            overlaps = true;
            break;
        case 117: // 01110101
                  //modded_val |= delta_array[g];
            overlaps = true;
            break;
        case 89: // 01011001
                 //modded_val |= delta_array[g];
            overlaps = true;
            break;
        case 105: // 01101001
                  //modded_val |= delta_array[g];
            overlaps = true;
            break;
        case 121: // 01111001
                  //modded_val |= delta_array[g];
            overlaps = true;
            break;
        case 93: // 01011101
                 //modded_val |= delta_array[g];
            overlaps = true;
            break;
        case 109: // 01101101
                  //modded_val |= delta_array[g];
            overlaps = true;
            break;
        case 125: // 01111101
                  //modded_val |= delta_array[g];
            overlaps = true;
            break;
        case 98: // 01100010
                 //modded_val |= delta_array[g];
            overlaps = true;
            break;
        case 114: // 01110010
                  //modded_val |= delta_array[g];
            overlaps = true;
            break;
        case 86: // 01010110
                 //modded_val |= delta_array[g];
            overlaps = true;
            break;
        case 102: // 01100110
                  //modded_val |= delta_array[g];
            overlaps = true;
            break;
        case 118: // 01110110
                  //modded_val |= delta_array[g];
            overlaps = true;
            break;
        case 90: // 01011010
                 //modded_val |= delta_array[g];
            overlaps = true;
            break;
        case 106: // 01101010
                  //modded_val |= delta_array[g];
            overlaps = true;
            break;
        case 122: // 01111010
                  //modded_val |= delta_array[g];
            overlaps = true;
            break;
        case 94: // 01011110
                 //modded_val |= delta_array[g];
            overlaps = true;
            break;
        case 110: // 01101110
                  //modded_val |= delta_array[g];
            overlaps = true;
            break;
        case 126: // 01111110
                  //modded_val |= delta_array[g];
            overlaps = true;
            break;
        case 99: // 01100011
                 //modded_val |= delta_array[g];
            overlaps = true;
            break;
        case 87: // 01010111
                 //modded_val |= delta_array[g];
            overlaps = true;
            break;
        case 103: // 01100111
                  //modded_val |= delta_array[g];
            overlaps = true;
            break;
        case 119: // 01110111
                  //modded_val |= delta_array[g];
            overlaps = true;
            break;
        case 91: // 01011011
                 //modded_val |= delta_array[g];
            overlaps = true;
            break;
        case 107: // 01101011
                  //modded_val |= delta_array[g];
            overlaps = true;
            break;
        case 123: // 01111011
                  //modded_val |= delta_array[g];
            overlaps = true;
            break;
        case 95: // 01011111
                 //modded_val |= delta_array[g];
            overlaps = true;
            break;
        case 111: // 01101111
                  //modded_val |= delta_array[g];
            overlaps = true;
            break;
        case 127: // 01111111
                  //modded_val |= delta_array[g];
            overlaps = true;
            break;
            //default:
            //modded_val |= delta_array[g];
            //break; }
            //delta_array_modded[g] = modded_val;
        }
        //   array_to_file(delta_array_modded, g, dir, "delta_array_modded", 0);
    }
}
