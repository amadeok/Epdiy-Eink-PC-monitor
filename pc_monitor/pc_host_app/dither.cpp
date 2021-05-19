
#include <Dither.h>

#include <stdlib.h>
#include <string.h>


#ifndef MIN
#define MIN(a,b)            (((a) < (b)) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef	CLAMP
//	This produces faster code without jumps
#define		CLAMP( x, xmin, xmax )		(x)	= MAX( (xmin), (x) );	\
										(x)	= MIN( (xmax), (x) )
#define		CLAMPED( x, xmin, xmax )	MAX( (xmin), MIN( (xmax), (x) ) )
#endif

#define	GRAY( r,g,b )	(((r) + (g) + (b)) / 3)

typedef	int	pixel;

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
//	Color discretization arrays
/////////////////////////////////////////////////////////////////////////////

const BYTE	VALUES_6BPP[]	= {	0,  85, 170, 255	};
const BYTE	VALUES_9BPP[]	= {	0,  36,  72, 108, 144, 180, 216, 255	};
const BYTE	VALUES_12BPP[]	= {	0,  17,  34,  51,  68,  85, 102, 119, 136, 153, 170, 187, 204, 221, 238, 255	};
const BYTE	VALUES_15BPP[]	= {	0,   8,  16,  24,  32,  40,  48,  56,  64,  80,  88,  96, 104, 112, 120, 128, 136, 144, 152, 160, 168, 176, 184, 192, 200, 208, 216, 224, 232, 240, 248, 255	};
const BYTE	VALUES_18BPP[]	= {	0,   4,   8,  12,  16,  20,  24,  28,  32,  36,  40,  44,  48,  52,  56,  60,  64,  68,  72,  76,  80,  84,  88,  92,  96, 100, 104, 108, 112, 116, 120, 126, 130, 136, 140, 144, 148, 152, 156, 160, 164, 168, 172, 176, 180, 184, 188, 192, 196, 200, 204, 208, 212, 216, 220, 224, 228, 232, 236, 240, 244, 248, 252, 255	};


/////////////////////////////////////////////////////////////////////////////
//	Ordered dither matrices
/////////////////////////////////////////////////////////////////////////////

const	int BAYER_PATTERN_2X2[2][2]		=	{	//	2x2 Bayer Dithering Matrix. Color levels: 5
												{	 51, 206	},
												{	153, 102	}
											};

const	int BAYER_PATTERN_3X3[3][3]		=	{	//	3x3 Bayer Dithering Matrix. Color levels: 10
												{	 181, 231, 131	},
												{	  50,  25, 100	},
												{	 156,  75, 206	}
											};

const	int BAYER_PATTERN_4X4[4][4]		=	{	//	4x4 Bayer Dithering Matrix. Color levels: 17
												{	 15, 195,  60, 240	},
												{	135,  75, 180, 120	},
												{	 45, 225,  30, 210	},
												{	165, 105, 150,  90	}

											};



const	int BAYER_PATTERN_8X8[8][8]		=	{	//	8x8 Bayer Dithering Matrix. Color levels: 65
												{	  0, 128,  32, 160,   8, 136,  40, 168	},
												{	192,  64, 224,  96, 200,  72, 232, 104	},
												{	 48, 176,  16, 144,  56, 184,  24, 152	},
												{	240, 112, 208,  80, 248, 120, 216,  88	},
												{	 12, 140,  44, 172,   4, 132,  36, 164	},
												{	204,  76, 236, 108, 196,  68, 228, 100	},
												{	 60, 188,  28, 156,  52, 180,  20, 148	},
												{	252, 124, 220,  92, 244, 116, 212,  84	}
											};

const	int	BAYER_PATTERN_16X16[16][16]	=	{	//	16x16 Bayer Dithering Matrix.  Color levels: 256
												{	  0, 191,  48, 239,  12, 203,  60, 251,   3, 194,  51, 242,  15, 206,  63, 254	}, 
												{	127,  64, 175, 112, 139,  76, 187, 124, 130,  67, 178, 115, 142,  79, 190, 127	},
												{	 32, 223,  16, 207,  44, 235,  28, 219,  35, 226,  19, 210,  47, 238,  31, 222	},
												{	159,  96, 143,  80, 171, 108, 155,  92, 162,  99, 146,  83, 174, 111, 158,  95	},
												{	  8, 199,  56, 247,   4, 195,  52, 243,  11, 202,  59, 250,   7, 198,  55, 246	},
												{	135,  72, 183, 120, 131,  68, 179, 116, 138,  75, 186, 123, 134,  71, 182, 119	},
												{	 40, 231,  24, 215,  36, 227,  20, 211,  43, 234,  27, 218,  39, 230,  23, 214	},
												{	167, 104, 151,  88, 163, 100, 147,  84, 170, 107, 154,  91, 166, 103, 150,  87	},
												{	  2, 193,  50, 241,  14, 205,  62, 253,   1, 192,  49, 240,  13, 204,  61, 252	},
												{	129,  66, 177, 114, 141,  78, 189, 126, 128,  65, 176, 113, 140,  77, 188, 125	},
												{	 34, 225,  18, 209,  46, 237,  30, 221,  33, 224,  17, 208,  45, 236,  29, 220	},
												{	161,  98, 145,  82, 173, 110, 157,  94, 160,  97, 144,  81, 172, 109, 156,  93	},
												{	 10, 201,  58, 249,   6, 197,  54, 245,   9, 200,  57, 248,   5, 196,  53, 244	},
												{	137,  74, 185, 122, 133,  70, 181, 118, 136,  73, 184, 121, 132,  69, 180, 117	},
												{	 42, 233,  26, 217,  38, 229,  22, 213,  41, 232,  25, 216,  37, 228,  21, 212	},
												{	169, 106, 153,  90, 165, 102, 149,  86, 168, 105, 152,  89, 164, 101, 148,  85	}
											};

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
//	Ordered dither using matrix
/////////////////////////////////////////////////////////////////////////////

void	makeDitherBayer16( BYTE* pixels, int width, int height )noexcept
{
	int	col	= 0;
	int	row	= 0;

	for( int y = 0; y < height; y++ )
	{
		row	= y & 15;	//	y % 16
        
		for( int x = 0; x < width; x++ )
		{
			col	= x & 15;	//	x % 16

			const pixel	blue	= pixels[x * 3 + 0];
            const pixel	green	= pixels[x * 3 + 1];
            const pixel	red		= pixels[x * 3 + 2];

			pixel	color	= ((red + green + blue)/3 < BAYER_PATTERN_16X16[col][row] ? 0 : 255);
			
            pixels[x * 3 + 0]	= color;	//	blue
            pixels[x * 3 + 1]	= color;	//	green
            pixels[x * 3 + 2]	= color;	//	red
		}

		pixels	+= width * 3;
	}
}
/////////////////////////////////////////////////////////////////////////////

void	makeDitherBayer8( BYTE* pixels, int width, int height )	noexcept
{
	int	col	= 0;
	int	row	= 0;

	for( int y = 0; y < height; y++ )
	{
		row	= y & 7;		//	% 8;
        
		for( int x = 0; x < width; x++ )
		{
			col	= x & 7;	//	% 8;

			const pixel	blue	= pixels[x * 3 + 0];
            const pixel	green	= pixels[x * 3 + 1];
            const pixel	red		= pixels[x * 3 + 2];

			pixel	color	= ((red + green + blue)/3 < BAYER_PATTERN_8X8[col][row] ? 0 : 255);
			
            pixels[x * 3 + 0]	= color;	//	blue
            pixels[x * 3 + 1]	= color;	//	green
            pixels[x * 3 + 2]	= color;	//	red
		}

		pixels	+= width * 3;
	}
}
/////////////////////////////////////////////////////////////////////////////

void	makeDitherBayer4( BYTE* pixels, int width, int height )	noexcept
{
	int	col	= 0;
	int	row	= 0;

	for( int y = 0; y < height; y++ )
	{
		row	= y & 3;	//	% 4
        
		for( int x = 0; x < width; x++ )
		{
			col	= x & 3;	//	% 4

			const pixel	blue	= pixels[x * 3 + 0];
            const pixel	green	= pixels[x * 3 + 1];
            const pixel	red		= pixels[x * 3 + 2];

			pixel	color	= ((red + green + blue)/3 < BAYER_PATTERN_4X4[col][row] ? 0 : 255);
			
            pixels[x * 3 + 0]	= color;	//	blue
            pixels[x * 3 + 1]	= color;	//	green
            pixels[x * 3 + 2]	= color;	//	red
		}

		pixels	+= width * 3;
	}
}
/////////////////////////////////////////////////////////////////////////////

void	makeDitherBayer3( BYTE* pixels, int width, int height )	noexcept
{
	int	col	= 0;
	int	row	= 0;

	for( int y = 0; y < height; y++ )
	{
		row	= y % 3;
        
		for( int x = 0; x < width; x++ )
		{
			col	= x % 3;

			const pixel	blue	= pixels[x * 3 + 0];
            const pixel	green	= pixels[x * 3 + 1];
            const pixel	red		= pixels[x * 3 + 2];

			pixel	color	= ((red + green + blue)/3 < BAYER_PATTERN_3X3[col][row] ? 0 : 255);
			
            pixels[x * 3 + 0]	= color;	//	blue
            pixels[x * 3 + 1]	= color;	//	green
            pixels[x * 3 + 2]	= color;	//	red
		}

		pixels	+= width * 3;
	}
}
/////////////////////////////////////////////////////////////////////////////

void	makeDitherBayer2( BYTE* pixels, int width, int height )	noexcept
{
	int	col	= 0;
	int	row	= 0;

	for( int y = 0; y < height; y++ )
	{
		row	= y & 1;	//	y % 2
        
		for( int x = 0; x < width; x++ )
		{
			col	= x & 1;	//	x % 2

			const pixel	blue	= pixels[x * 3 + 0];
            const pixel	green	= pixels[x * 3 + 1];
            const pixel	red		= pixels[x * 3 + 2];

			pixel	color	= ((red + green + blue)/3 < BAYER_PATTERN_2X2[col][row] ? 0 : 255);
			
            pixels[x * 3 + 0]	= color;	//	blue
            pixels[x * 3 + 1]	= color;	//	green
            pixels[x * 3 + 2]	= color;	//	red
		}

		pixels	+= width * 3;
	}
}
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
//	Colored Ordered dither, using 16x16 matrix (dither is applied on all color planes (r,g,b)
/////////////////////////////////////////////////////////////////////////////

//	This is the ultimate method for Bayer Ordered Diher with 16x16 matrix
//	ncolors - number of colors diapazons to use. Valid values 0..255, but interesed are 0..40
//	1		- color (1 bit per color plane,  3 bits per pixel)
//	3		- color (2 bit per color plane,  6 bits per pixel)
//	7		- color (3 bit per color plane,  9 bits per pixel)
//	15		- color (4 bit per color plane, 12 bits per pixel)
//	31		- color (5 bit per color plane, 15 bits per pixel)
void	makeDitherBayerRgbNbpp( BYTE* pixels, int width, int height, int ncolors )	noexcept
{
	int	divider	= 256 / ncolors;

	for( int y = 0; y < height; y++ )
	{
		const int	row	= y & 15;	//	y % 16
        
		for( int x = 0; x < width; x++ )
		{
			const int	col	= x & 15;	//	x % 16

			const int	t		= BAYER_PATTERN_16X16[col][row];
			const int	corr	= (t / ncolors);

			const int	blue	= pixels[x * 3 + 0];
			const int	green	= pixels[x * 3 + 1];
			const int	red		= pixels[x * 3 + 2];
	
			int	i1	= (blue  + corr) / divider;	CLAMP( i1, 0, ncolors );
			int	i2	= (green + corr) / divider;	CLAMP( i2, 0, ncolors );
			int	i3	= (red   + corr) / divider;	CLAMP( i3, 0, ncolors );

			//	If you want to compress the image, use the values of i1,i2,i3
			//	they have values in the range 0..ncolors
			//	So if the ncolors is 4 - you have values: 0,1,2,3 which is encoded in 2 bits
			//	2 bits for 3 planes == 6 bits per pixel

			pixels[x * 3 + 0]	= CLAMPED( i1 * divider, 0, 255 );	//VALUES_6BPP[i1];	//	blue
			pixels[x * 3 + 1]	= CLAMPED( i2 * divider, 0, 255 );	//VALUES_6BPP[i2];	//	green
			pixels[x * 3 + 2]	= CLAMPED( i3 * divider, 0, 255 );	//VALUES_6BPP[i3];	//	red
		}

		pixels	+= width * 3;
	}
}
/////////////////////////////////////////////////////////////////////////////

//	Color ordered dither using 3 bits per pixel (1 bit per color plane)
void	makeDitherBayerRgb3bpp( BYTE* pixels, int width, int height )	noexcept
{
	int	col	= 0;
	int	row	= 0;

	for( int y = 0; y < height; y++ )
	{
		row	= y & 15;	//	y % 16
        
		for( int x = 0; x < width; x++ )
		{
			col	= x & 15;	//	x % 16

            pixels[x * 3 + 0]	= (pixels[x * 3 + 0] > BAYER_PATTERN_16X16[col][row] ? 255 : 0);
            pixels[x * 3 + 1]	= (pixels[x * 3 + 1] > BAYER_PATTERN_16X16[col][row] ? 255 : 0);
            pixels[x * 3 + 2]	= (pixels[x * 3 + 2] > BAYER_PATTERN_16X16[col][row] ? 255 : 0);
		}

		pixels	+= width * 3;
	}
}
/////////////////////////////////////////////////////////////////////////////

//	Color ordered dither using 6 bits per pixel (2 bit per color plane)
void	makeDitherBayerRgb6bpp( BYTE* pixels, int width, int height )	noexcept
{
	for( int y = 0; y < height; y++ )
	{
		const int	row	= y & 15;	//	y % 16
        
		for( int x = 0; x < width; x++ )
		{
			const int	col	= x & 15;	//	x % 16

			const int	t		= BAYER_PATTERN_16X16[col][row];
			const int	corr	= (t / 3);

			const int	blue	= pixels[x * 3 + 0];
			const int	green	= pixels[x * 3 + 1];
			const int	red		= pixels[x * 3 + 2];
	
			int	i1	= (blue  + corr) / 85;	CLAMP( i1, 0, 3 );
			int	i2	= (green + corr) / 85;	CLAMP( i2, 0, 3 );
			int	i3	= (red   + corr) / 85;	CLAMP( i3, 0, 3 );

			//	If you want to compress the image, use the values of i1,i2,i3
			//	they have values in the range 0..3 which is encoded in 2 bits
			//	2 bits for 3 planes == 6 bits per pixel
			//	out.writeBit( i1 & 0x01 );
			//	out.writeBit( i1 & 0x02 );
			//	out.writeBit( i2 & 0x01 );
			//	out.writeBit( i2 & 0x02 );
			//	out.writeBit( i3 & 0x01 );
			//	out.writeBit( i3 & 0x02 );

			pixels[x * 3 + 0]	= i1 * 85;	//VALUES_6BPP[i1];	//	blue
			pixels[x * 3 + 1]	= i2 * 85;	//VALUES_6BPP[i2];	//	green
			pixels[x * 3 + 2]	= i3 * 85;	//VALUES_6BPP[i3];	//	red
		}

		pixels	+= width * 3;
	}
}

/*Faster but slightly inaccurate version
void	makeDitherBayerRgb6bpp( BYTE* pixels, int width, int height )	noexcept
{
	for( int y = 0; y < height; y++ )
	{
		const int	row	= y & 15;	//	y % 16
        
		for( int x = 0; x < width; x++ )
		{
			const int	col	= x & 15;	//	x % 16

			const int	t		= BAYER_PATTERN_16X16[col][row];
			const int	corr	= (t >> 2) - 32;

			const int	blue	= pixels[x * 3 + 0];
			const int	green	= pixels[x * 3 + 1];
			const int	red		= pixels[x * 3 + 2];
		
			int	i1	= (blue  + corr) >> 6;	CLAMP( i1, 0, 3 );
			int	i2	= (green + corr) >> 6;	CLAMP( i2, 0, 3 );
			int	i3	= (red   + corr) >> 6;	CLAMP( i3, 0, 3 );

			pixels[x * 3 + 0]	= VALUES_6BPP[i1];	//	blue
			pixels[x * 3 + 1]	= VALUES_6BPP[i2];	//	green
			pixels[x * 3 + 2]	= VALUES_6BPP[i3];	//	red
		}

		pixels	+= width * 3;
	}
}
*/
/////////////////////////////////////////////////////////////////////////////

//	Color ordered dither using 9 bits per pixel (3 bit per color plane)
void	makeDitherBayerRgb9bpp( BYTE* pixels, int width, int height )	noexcept
{
	for( int y = 0; y < height; y++ )
	{
		int	row	= y & 15;	//	y % 16
        
		for( int x = 0; x < width; x++ )
		{
			int	col	= x & 15;	//	x % 16

			const int	t		= BAYER_PATTERN_16X16[col][row];
			const int	corr	= (t / 7) - 2;	//	 -2 because: 256/7=36  36*7=252  256-252=4   4/2=2 - correction -2

			const int	blue	= pixels[x * 3 + 0];
            const int	green	= pixels[x * 3 + 1];
            const int	red		= pixels[x * 3 + 2];

			int	i1	= (blue  + corr) / 36;		CLAMP( i1, 0, 7 );
			int	i2	= (green + corr) / 36;		CLAMP( i2, 0, 7 );
			int	i3	= (red   + corr) / 36;		CLAMP( i3, 0, 7 );

            pixels[x * 3 + 0]	= i1 * 36;	//VALUES_9BPP[i1];
            pixels[x * 3 + 1]	= i2 * 36;	//VALUES_9BPP[i2];
            pixels[x * 3 + 2]	= i3 * 36;	//VALUES_9BPP[i3];
		}

		pixels	+= width * 3;
	}
}

/*Faster but slightly inaccurate version
void	makeDitherBayerRgb9bpp( BYTE* pixels, int width, int height )	noexcept
{
	for( int y = 0; y < height; y++ )
	{
		int	row	= y & 15;	//	y % 16
        
		BYTE*   prow   = pixels + ( y * width * 3 );

		for( int x = 0; x < width; x++ )
		{
			int	col	= x & 15;	//	x % 16

			int	t		= BAYER_PATTERN_16X16[col][row];
			int	corr	= (t >> 3) - 16;

			const int	blue	= prow[x * 3 + 0];
            const int	green	= prow[x * 3 + 1];
            const int	red		= prow[x * 3 + 2];

			int	i1	= (blue  + corr) >> 5;		CLAMP( i1, 0, 7 );
			int	i2	= (green + corr) >> 5;		CLAMP( i2, 0, 7 );
			int	i3	= (red   + corr) >> 5;		CLAMP( i3, 0, 7 );

            prow[x * 3 + 0]	= VALUES_9BPP[i1];
            prow[x * 3 + 1]	= VALUES_9BPP[i2];
            prow[x * 3 + 2]	= VALUES_9BPP[i3];
		}
	}
}
*/
/////////////////////////////////////////////////////////////////////////////

//	Color ordered dither using 12 bits per pixel (4 bit per color plane)
void	makeDitherBayerRgb12bpp( BYTE* pixels, int width, int height )	noexcept
{
	for( int y = 0; y < height; y++ )
	{
		int	row	= y & 15;	//	y % 16
        
		for( int x = 0; x < width; x++ )
		{
			int	col	= x & 15;	//	x % 16

			const int	t		= BAYER_PATTERN_16X16[col][row];
			const int	corr	= t / 15;

			const int	blue	= pixels[x * 3 + 0];
            const int	green	= pixels[x * 3 + 1];
            const int	red		= pixels[x * 3 + 2];

			int	i1	= (blue  + corr) / 17;		CLAMP( i1, 0, 15 );
			int	i2	= (green + corr) / 17;		CLAMP( i2, 0, 15 );
			int	i3	= (red   + corr) / 17;		CLAMP( i3, 0, 15 );

            pixels[x * 3 + 0]	= i1 * 17;
            pixels[x * 3 + 1]	= i2 * 17;
            pixels[x * 3 + 2]	= i3 * 17;
		}

		pixels	+= width * 3;
	}
}

/*Faster but slightly inaccurate version
void	makeDitherBayerRgb12bpp( BYTE* pixels, int width, int height )	noexcept
{
	for( int y = 0; y < height; y++ )
	{
		int	row	= y & 15;	//	y % 16
        
		BYTE*   prow   = pixels + ( y * width * 3 );

		for( int x = 0; x < width; x++ )
		{
			int	col	= x & 15;	//	x % 16

			int	t		= BAYER_PATTERN_16X16[col][row];
			int	corr	= (t >> 4) - 8;

			const int	blue	= prow[x * 3 + 0];
            const int	green	= prow[x * 3 + 1];
            const int	red		= prow[x * 3 + 2];

			int	i1	= (blue  + corr) >> 4;		CLAMP( i1, 0, 15 );
			int	i2	= (green + corr) >> 4;		CLAMP( i2, 0, 15 );
			int	i3	= (red   + corr) >> 4;		CLAMP( i3, 0, 15 );

            prow[x * 3 + 0]	= VALUES_12BPP[i1];
            prow[x * 3 + 1]	= VALUES_12BPP[i2];
            prow[x * 3 + 2]	= VALUES_12BPP[i3];
		}
	}
}
*/
/////////////////////////////////////////////////////////////////////////////

//	Color ordered dither using 15 bits per pixel (5 bit per color plane)
void	makeDitherBayerRgb15bpp( BYTE* pixels, int width, int height )	noexcept
{
	for( int y = 0; y < height; y++ )
	{
		int	row	= y & 15;	//	y % 16
        
		for( int x = 0; x < width; x++ )
		{
			int	col	= x & 15;	//	x % 16

			const int	t		= BAYER_PATTERN_16X16[col][row];
			const int	corr	= (t / 31);

			const int	blue	= pixels[x * 3 + 0];
            const int	green	= pixels[x * 3 + 1];
            const int	red		= pixels[x * 3 + 2];

			int	i1	= (blue  + corr) / 8;		CLAMP( i1, 0, 31 );
			int	i2	= (green + corr) / 8;		CLAMP( i2, 0, 31 );
			int	i3	= (red   + corr) / 8;		CLAMP( i3, 0, 31 );

            pixels[x * 3 + 0]	= i1 * 8;
            pixels[x * 3 + 1]	= i2 * 8;
            pixels[x * 3 + 2]	= i3 * 8;
		}

		pixels	+= width * 3;
	}
}

/*Faster but slightly inaccurate version
void	makeDitherBayerRgb15bpp( BYTE* pixels, int width, int height )	noexcept
{
	for( int y = 0; y < height; y++ )
	{
		int	row	= y & 15;	//	y % 16
        
		BYTE*   prow   = pixels + ( y * width * 3 );

		for( int x = 0; x < width; x++ )
		{
			int	col	= x & 15;	//	x % 16

			int	t		= BAYER_PATTERN_16X16[col][row];
			int	corr	= (t >> 5) - 4;

			const int	blue	= prow[x * 3 + 0];
            const int	green	= prow[x * 3 + 1];
            const int	red		= prow[x * 3 + 2];

			int	i1	= (blue  + corr) >> 3;		CLAMP( i1, 0, 31 );
			int	i2	= (green + corr) >> 3;		CLAMP( i2, 0, 31 );
			int	i3	= (red   + corr) >> 3;		CLAMP( i3, 0, 31 );

            prow[x * 3 + 0]	= VALUES_15BPP[i1];
            prow[x * 3 + 1]	= VALUES_15BPP[i2];
            prow[x * 3 + 2]	= VALUES_15BPP[i3];
		}
	}
}
*/
/////////////////////////////////////////////////////////////////////////////

//	Color ordered dither using 18 bits per pixel (6 bit per color plane)
void	makeDitherBayerRgb18bpp( BYTE* pixels, int width, int height )	noexcept
{
	for( int y = 0; y < height; y++ )
	{
		int	row	= y & 15;	//	y % 16
        
		for( int x = 0; x < width; x++ )
		{
			int	col	= x & 15;	//	x % 16

			const int	t		= BAYER_PATTERN_16X16[col][row];
			const int	corr	= t / 63;

			const int	blue	= pixels[x * 3 + 0];
            const int	green	= pixels[x * 3 + 1];
            const int	red		= pixels[x * 3 + 2];

			int	i1	= (blue  + corr) / 4;		CLAMP( i1, 0, 63 );
			int	i2	= (green + corr) / 4;		CLAMP( i2, 0, 63 );
			int	i3	= (red   + corr) / 4;		CLAMP( i3, 0, 63 );

            pixels[x * 3 + 0]	= i1 * 4;//VALUES_18BPP[i1];
            pixels[x * 3 + 1]	= i2 * 4;//VALUES_18BPP[i2];
            pixels[x * 3 + 2]	= i3 * 4;//VALUES_18BPP[i3];
		}

		pixels	+= width * 3;
	}
}

/*Faster but slightly inaccurate version
void	makeDitherBayerRgb18bpp( BYTE* pixels, int width, int height )	noexcept
{
	for( int y = 0; y < height; y++ )
	{
		int	row	= y & 15;	//	y % 16
        
		BYTE*   prow   = pixels + ( y * width * 3 );

		for( int x = 0; x < width; x++ )
		{
			int	col	= x & 15;	//	x % 16

			int	t		= BAYER_PATTERN_16X16[col][row];
			int	corr	= (t >> 6) - 2;

			const int	blue	= prow[x * 3 + 0];
            const int	green	= prow[x * 3 + 1];
            const int	red		= prow[x * 3 + 2];

			int	i1	= (blue  + corr) >> 2;		CLAMP( i1, 0, 63 );
			int	i2	= (green + corr) >> 2;		CLAMP( i2, 0, 63 );
			int	i3	= (red   + corr) >> 2;		CLAMP( i3, 0, 63 );

            prow[x * 3 + 0]	= VALUES_18BPP[i1];
            prow[x * 3 + 1]	= VALUES_18BPP[i2];
            prow[x * 3 + 2]	= VALUES_18BPP[i3];
		}
	}
}*/
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
//	Floyd-Steinberg dither
/////////////////////////////////////////////////////////////////////////////

//	Floyd-Steinberg dither uses constants 7/16 5/16 3/16 and 1/16
//	But instead of using real arythmetic, I will use integer on by
//	applying shifting ( << 8 )
//	When use the constants, don't foget to shift back the result ( >> 8 )
#define	f7_16	112		//const int	f7	= (7 << 8) / 16;
#define	f5_16	 80		//const int	f5	= (5 << 8) / 16;
#define	f3_16	 48		//const int	f3	= (3 << 8) / 16;
#define	f1_16	 16		//const int	f1	= (1 << 8) / 16;

/////////////////////////////////////////////////////////////////////////////

//#define	FS_COEF( v, err )	(( ((err) * ((v) * 100)) / 16) / 100)
//#define	FS_COEF( v, err )	(( ((err) * ((v) << 8)) >> 4) >> 8)
#define	FS_COEF( v, err )	(((err) * ((v) << 8)) >> 12)


//	This is the ultimate method for Floyd-Steinberg colored Diher
//	ncolors - number of colors diapazons to use. Valid values 0..255, but interesed are 0..40
//	1		- color (1 bit per color plane,  3 bits per pixel)
//	3		- color (2 bit per color plane,  6 bits per pixel)
//	7		- color (3 bit per color plane,  9 bits per pixel)
//	15		- color (4 bit per color plane, 12 bits per pixel)
//	31		- color (5 bit per color plane, 15 bits per pixel)
void	makeDitherFSRgbNbpp( BYTE* pixels, int width, int height, int ncolors )	noexcept
{
	int	divider	= 256 / ncolors;

	const int	size	= width * height;

	int*	errorB	= (int*)malloc( size * sizeof(int) );
	int*	errorG	= (int*)malloc( size * sizeof(int) );
	int*	errorR	= (int*)malloc( size * sizeof(int) );

	//	Clear the errors buffer.
	memset( errorB, 0, size * sizeof(int) );
	memset( errorG, 0, size * sizeof(int) );
	memset( errorR, 0, size * sizeof(int) );

	//~~~~~~~~

	int	i	= 0;

	for( int y = 0; y < height; y++ )
	{
		BYTE*   prow   = pixels + ( y * width * 3 );

		for( int x = 0; x < width; x++,i++ )
		{
			const int	blue	= prow[x * 3 + 0];
            const int	green	= prow[x * 3 + 1];
            const int	red		= prow[x * 3 + 2];

			const int	newValB	= blue	+ errorB[i];
			const int	newValG	= green + errorG[i];
			const int	newValR	= red	+ errorR[i];

			int	i1	= newValB / divider;	CLAMP( i1, 0, ncolors );
			int	i2	= newValG / divider;	CLAMP( i2, 0, ncolors );
			int	i3	= newValR / divider;	CLAMP( i3, 0, ncolors );

			//	If you want to compress the image, use the values of i1,i2,i3
			//	they have values in the range 0..ncolors
			//	So if the ncolors is 4 - you have values: 0,1,2,3 which is encoded in 2 bits
			//	2 bits for 3 planes == 6 bits per pixel

			const int	newcB	= CLAMPED( i1 * divider, 0, 255 );	//	blue
			const int	newcG	= CLAMPED( i2 * divider, 0, 255 );	//	green
			const int	newcR	= CLAMPED( i3 * divider, 0, 255 );	//	red

            prow[x * 3 + 0]	= newcB;
            prow[x * 3 + 1]	= newcG;
            prow[x * 3 + 2]	= newcR;

			const int cerrorB	= newValB - newcB;
			const int cerrorG	= newValG - newcG;
			const int cerrorR	= newValR - newcR;

			int	idx = i+1;
			if( x+1 < width )
			{
				errorR[idx] += FS_COEF( 7, cerrorR );
				errorG[idx] += FS_COEF( 7, cerrorG );
				errorB[idx] += FS_COEF( 7, cerrorB );
			}
			
			idx += width - 2;
			if( x-1 > 0 && y+1 < height )
			{
				errorR[idx] += FS_COEF( 3, cerrorR );
				errorG[idx] += FS_COEF( 3, cerrorG );
				errorB[idx] += FS_COEF( 3, cerrorB );
			}
			
			idx++;
			if( y+1 < height )
			{
				errorR[idx] += FS_COEF( 5, cerrorR );
				errorG[idx] += FS_COEF( 5, cerrorG );
				errorB[idx] += FS_COEF( 5, cerrorB );
			}
			
			idx++;
			if( x+1 < width && y+1 < height )
			{
				errorR[idx] += FS_COEF( 1, cerrorR );
				errorG[idx] += FS_COEF( 1, cerrorG );
				errorB[idx] += FS_COEF( 1, cerrorB );
			}
		}
	}

	free( errorB );
	free( errorG );
	free( errorR );
}
/////////////////////////////////////////////////////////////////////////////

//	Back-white Floyd-Steinberg dither
void	makeDitherFS( BYTE* pixels, int width, int height )	noexcept
{
	const int	size	= width * height;

	int*	error	= (int*)malloc( size * sizeof(int) );

	//	Clear the errors buffer.
	memset( error, 0, size * sizeof(int) );

	//~~~~~~~~

	int	i	= 0;

	for( int y = 0; y < height; y++ )
	{
		BYTE*   prow   = pixels + ( y * width * 3 );

		for( int x = 0; x < width; x++,i++ )
		{
			const int	blue	= prow[x * 3 + 0];
            const int	green	= prow[x * 3 + 1];
            const int	red		= prow[x * 3 + 2];

			//	Get the pixel gray value.
			int	newVal	= (red+green+blue)/3 + (error[i] >> 8);	//	PixelGray + error correction

			int	newc	= (newVal < 128 ? 0 : 255);
            prow[x * 3 + 0]	= newc;	//	blue
            prow[x * 3 + 1]	= newc;	//	green
            prow[x * 3 + 2]	= newc;	//	red

			//	Correction - the new error
			const int	cerror	= newVal - newc;

			int idx = i+1;
			if( x+1 < width )
				error[idx] += (cerror * f7_16);

			idx += width - 2;
			if( x-1 > 0 && y+1 < height )
				error[idx] += (cerror * f3_16);

			idx++;
			if( y+1 < height )
				error[idx] += (cerror * f5_16);

			idx++;
			if( x+1 < width && y+1 < height )
				error[idx] += (cerror * f1_16);
		}
	}

	free( error );
}
/////////////////////////////////////////////////////////////////////////////

//	Color Floyd-Steinberg dither using 3 bits per pixel (1 bit per color plane)
void	makeDitherFSRgb3bpp( BYTE* pixels, int width, int height )	noexcept
{
	const int	size	= width * height;

	int*	errorB	= (int*)malloc( size * sizeof(int) );
	int*	errorG	= (int*)malloc( size * sizeof(int) );
	int*	errorR	= (int*)malloc( size * sizeof(int) );

	//	Clear the errors buffer.
	memset( errorB, 0, size * sizeof(int) );
	memset( errorG, 0, size * sizeof(int) );
	memset( errorR, 0, size * sizeof(int) );

	//~~~~~~~~

	int	i	= 0;

	for( int y = 0; y < height; y++ )
	{
		BYTE*   prow   = pixels + ( y * width * 3 );

		for( int x = 0; x < width; x++,i++ )
		{
			const int	blue	= prow[x * 3 + 0];
            const int	green	= prow[x * 3 + 1];
            const int	red		= prow[x * 3 + 2];

			int	newValB	= blue  + (errorB[i] >> 8);	//	PixelRed   + error correctionB
			int	newValG	= green + (errorG[i] >> 8);	//	PixelGreen + error correctionG
			int	newValR	= red   + (errorR[i] >> 8);	//	PixelBlue  + error correctionR

			int	newcb	= (newValB < 128 ? 0 : 255);
			int	newcg	= (newValG < 128 ? 0 : 255);
			int	newcr	= (newValR < 128 ? 0 : 255);

            prow[x * 3 + 0]	= newcb;
            prow[x * 3 + 1]	= newcg;
            prow[x * 3 + 2]	= newcr;

			//	Correction - the new error
			int	cerrorR	= newValR - newcr;
			int	cerrorG	= newValG - newcg;
			int	cerrorB	= newValB - newcb;

			int	idx	= i+1;
			if( x+1 < width )
			{
				errorR[idx] += (cerrorR * f7_16);
				errorG[idx] += (cerrorG * f7_16);
				errorB[idx] += (cerrorB * f7_16);
			}

			idx += width - 2;
			if( x-1 > 0 && y+1 < height )
			{
				errorR[idx] += (cerrorR * f3_16);
				errorG[idx] += (cerrorG * f3_16);
				errorB[idx] += (cerrorB * f3_16);
			}

			idx++;
			if( y+1 < height )
			{
				errorR[idx] += (cerrorR * f5_16);
				errorG[idx] += (cerrorG * f5_16);
				errorB[idx] += (cerrorB * f5_16);
			}
			
			idx++;
			if( x+1 < width && y+1 < height )
			{
				errorR[idx] += (cerrorR * f1_16);
				errorG[idx] += (cerrorG * f1_16);
				errorB[idx] += (cerrorB * f1_16);
			}
		}
	}

	free( errorB );
	free( errorG );
	free( errorR );
}
/////////////////////////////////////////////////////////////////////////////

//	Color Floyd-Steinberg dither using 6 bits per pixel (2 bit per color plane)
void	makeDitherFSRgb6bpp( BYTE* pixels, int width, int height )	noexcept
{
	const int	size	= width * height;

	int*	errorB	= (int*)malloc( size * sizeof(int) );
	int*	errorG	= (int*)malloc( size * sizeof(int) );
	int*	errorR	= (int*)malloc( size * sizeof(int) );

	//	Clear the errors buffer.
	memset( errorB, 0, size * sizeof(int) );
	memset( errorG, 0, size * sizeof(int) );
	memset( errorR, 0, size * sizeof(int) );

	//~~~~~~~~

	int	i	= 0;

	for( int y = 0; y < height; y++ )
	{
		BYTE*   prow   = pixels + ( y * width * 3 );

		for( int x = 0; x < width; x++,i++ )
		{
			const int	blue	= prow[x * 3 + 0];
            const int	green	= prow[x * 3 + 1];
            const int	red		= prow[x * 3 + 2];

			int	newValB	= (int)blue	 + (errorB[i] >> 8);	//	PixelBlue  + error correctionB
			int	newValG	= (int)green + (errorG[i] >> 8);	//	PixelGreen + error correctionG
			int	newValR	= (int)red	 + (errorR[i] >> 8);	//	PixelRed   + error correctionR

			//	The error could produce values beyond the borders, so need to keep the color in range
			int	idxR	= CLAMPED( newValR, 0, 255 );
			int	idxG	= CLAMPED( newValG, 0, 255 );
			int	idxB	= CLAMPED( newValB, 0, 255 );

			int	newcR	= VALUES_6BPP[idxR >> 6];	//	x >> 6 is the same as x / 64
			int	newcG	= VALUES_6BPP[idxG >> 6];	//	x >> 6 is the same as x / 64
			int	newcB	= VALUES_6BPP[idxB >> 6];	//	x >> 6 is the same as x / 64

            prow[x * 3 + 0]	= newcB;
            prow[x * 3 + 1]	= newcG;
            prow[x * 3 + 2]	= newcR;

			int cerrorB	= newValB - newcB;
			int cerrorG	= newValG - newcG;
			int cerrorR	= newValR - newcR;

			int	idx = i+1;
			if( x+1 < width )
			{
				errorR[idx] += (cerrorR * f7_16);
				errorG[idx] += (cerrorG * f7_16);
				errorB[idx] += (cerrorB * f7_16);
			}
			
			idx += width - 2;
			if( x-1 > 0 && y+1 < height )
			{
				errorR[idx] += (cerrorR * f3_16);
				errorG[idx] += (cerrorG * f3_16);
				errorB[idx] += (cerrorB * f3_16);
			}
			
			idx++;
			if( y+1 < height )
			{
				errorR[idx] += (cerrorR * f5_16);
				errorG[idx] += (cerrorG * f5_16);
				errorB[idx] += (cerrorB * f5_16);
			}
			
			idx++;
			if( x+1 < width && y+1 < height )
			{
				errorR[idx] += (cerrorR * f1_16);
				errorG[idx] += (cerrorG * f1_16);
				errorB[idx] += (cerrorB * f1_16);
			}
		}
	}

	free( errorB );
	free( errorG );
	free( errorR );
}
/////////////////////////////////////////////////////////////////////////////

//	Color Floyd-Steinberg dither using 9 bits per pixel (3 bit per color plane)
void	makeDitherFSRgb9bpp( BYTE* pixels, int width, int height )	noexcept
{
	const int	size	= width * height;

	int*	errorB	= (int*)malloc( size * sizeof(int) );
	int*	errorG	= (int*)malloc( size * sizeof(int) );
	int*	errorR	= (int*)malloc( size * sizeof(int) );

	//	Clear the errors buffer.
	memset( errorB, 0, size * sizeof(int) );
	memset( errorG, 0, size * sizeof(int) );
	memset( errorR, 0, size * sizeof(int) );

	//~~~~~~~~

	int	i	= 0;

	for( int y = 0; y < height; y++ )
	{
		BYTE*   prow   = pixels + ( y * width * 3 );

		for( int x = 0; x < width; x++,i++ )
		{
			const int	blue	= prow[x * 3 + 0];
            const int	green	= prow[x * 3 + 1];
            const int	red		= prow[x * 3 + 2];

			int	newValB	= (int)blue	 + (errorB[i] >> 8);	//	PixelBlue  + error correctionB
			int	newValG	= (int)green + (errorG[i] >> 8);	//	PixelGreen + error correctionG
			int	newValR	= (int)red	 + (errorR[i] >> 8);	//	PixelRed   + error correctionR

			//	The error could produce values beyond the borders, so need to keep the color in range
			int	idxR	= CLAMPED( newValR, 0, 255 );
			int	idxG	= CLAMPED( newValG, 0, 255 );
			int	idxB	= CLAMPED( newValB, 0, 255 );

			int	newcR	= VALUES_9BPP[idxR >> 5];	//	x >> 5 is the same as x / 32
			int	newcG	= VALUES_9BPP[idxG >> 5];	//	x >> 5 is the same as x / 32
			int	newcB	= VALUES_9BPP[idxB >> 5];	//	x >> 5 is the same as x / 32

            prow[x * 3 + 0]	= newcB;
            prow[x * 3 + 1]	= newcG;
            prow[x * 3 + 2]	= newcR;

			int cerrorB	= newValB - newcB;
			int cerrorG	= newValG - newcG;
			int cerrorR	= newValR - newcR;

			int	idx = i+1;
			if( x+1 < width )
			{
				errorR[idx] += (cerrorR * f7_16);
				errorG[idx] += (cerrorG * f7_16);
				errorB[idx] += (cerrorB * f7_16);
			}
			
			idx += width - 2;
			if( x-1 > 0 && y+1 < height )
			{
				errorR[idx] += (cerrorR * f3_16);
				errorG[idx] += (cerrorG * f3_16);
				errorB[idx] += (cerrorB * f3_16);
			}
			
			idx++;
			if( y+1 < height )
			{
				errorR[idx] += (cerrorR * f5_16);
				errorG[idx] += (cerrorG * f5_16);
				errorB[idx] += (cerrorB * f5_16);
			}
			
			idx++;
			if( x+1 < width && y+1 < height )
			{
				errorR[idx] += (cerrorR * f1_16);
				errorG[idx] += (cerrorG * f1_16);
				errorB[idx] += (cerrorB * f1_16);
			}
		}
	}

	free( errorB );
	free( errorG );
	free( errorR );
}
/////////////////////////////////////////////////////////////////////////////

//	Color Floyd-Steinberg dither using 12 bits per pixel (4 bit per color plane)
void	makeDitherFSRgb12bpp( BYTE* pixels, int width, int height )	noexcept
{
	const int	size	= width * height;

	int*	errorB	= (int*)malloc( size * sizeof(int) );
	int*	errorG	= (int*)malloc( size * sizeof(int) );
	int*	errorR	= (int*)malloc( size * sizeof(int) );

	//	Clear the errors buffer.
	memset( errorB, 0, size * sizeof(int) );
	memset( errorG, 0, size * sizeof(int) );
	memset( errorR, 0, size * sizeof(int) );

	//~~~~~~~~

	int	i	= 0;

	for( int y = 0; y < height; y++ )
	{
		BYTE*   prow   = pixels + ( y * width * 3 );

		for( int x = 0; x < width; x++,i++ )
		{
			const int	blue	= prow[x * 3 + 0];
            const int	green	= prow[x * 3 + 1];
            const int	red		= prow[x * 3 + 2];

			int	newValB	= (int)blue	 + (errorB[i] >> 8);	//	PixelBlue  + error correctionB
			int	newValG	= (int)green + (errorG[i] >> 8);	//	PixelGreen + error correctionG
			int	newValR	= (int)red	 + (errorR[i] >> 8);	//	PixelRed   + error correctionR

			//	The error could produce values beyond the borders, so need to keep the color in range
			int	idxR	= CLAMPED( newValR, 0, 255 );
			int	idxG	= CLAMPED( newValG, 0, 255 );
			int	idxB	= CLAMPED( newValB, 0, 255 );

			int	newcR	= VALUES_12BPP[idxR >> 4];	//	x >> 4 is the same as x / 16
			int	newcG	= VALUES_12BPP[idxG >> 4];	//	x >> 4 is the same as x / 16
			int	newcB	= VALUES_12BPP[idxB >> 4];	//	x >> 4 is the same as x / 16

            prow[x * 3 + 0]	= newcB;
            prow[x * 3 + 1]	= newcG;
            prow[x * 3 + 2]	= newcR;

			int cerrorB	= newValB - newcB;
			int cerrorG	= newValG - newcG;
			int cerrorR	= newValR - newcR;

			int	idx = i+1;
			if( x+1 < width )
			{
				errorR[idx] += (cerrorR * f7_16);
				errorG[idx] += (cerrorG * f7_16);
				errorB[idx] += (cerrorB * f7_16);
			}
			
			idx += width - 2;
			if( x-1 > 0 && y+1 < height )
			{
				errorR[idx] += (cerrorR * f3_16);
				errorG[idx] += (cerrorG * f3_16);
				errorB[idx] += (cerrorB * f3_16);
			}
			
			idx++;
			if( y+1 < height )
			{
				errorR[idx] += (cerrorR * f5_16);
				errorG[idx] += (cerrorG * f5_16);
				errorB[idx] += (cerrorB * f5_16);
			}
			
			idx++;
			if( x+1 < width && y+1 < height )
			{
				errorR[idx] += (cerrorR * f1_16);
				errorG[idx] += (cerrorG * f1_16);
				errorB[idx] += (cerrorB * f1_16);
			}
		}
	}

	free( errorB );
	free( errorG );
	free( errorR );
}
/////////////////////////////////////////////////////////////////////////////

//	Color Floyd-Steinberg dither using 15 bits per pixel (5 bit per color plane)
void	makeDitherFSRgb15bpp( BYTE* pixels, int width, int height )	noexcept
{
	const int	size	= width * height;

	int*	errorB	= (int*)malloc( size * sizeof(int) );
	int*	errorG	= (int*)malloc( size * sizeof(int) );
	int*	errorR	= (int*)malloc( size * sizeof(int) );

	//	Clear the errors buffer.
	memset( errorB, 0, size * sizeof(int) );
	memset( errorG, 0, size * sizeof(int) );
	memset( errorR, 0, size * sizeof(int) );

	//~~~~~~~~

	int	i	= 0;

	for( int y = 0; y < height; y++ )
	{
		BYTE*   prow   = pixels + ( y * width * 3 );

		for( int x = 0; x < width; x++,i++ )
		{
			const int	blue	= prow[x * 3 + 0];
            const int	green	= prow[x * 3 + 1];
            const int	red		= prow[x * 3 + 2];

			int	newValB	= (int)blue	 + (errorB[i] >> 8);	//	PixelBlue  + error correctionB
			int	newValG	= (int)green + (errorG[i] >> 8);	//	PixelGreen + error correctionG
			int	newValR	= (int)red	 + (errorR[i] >> 8);	//	PixelRed   + error correctionR

			//	The error could produce values beyond the borders, so need to keep the color in range
			int	idxR	= CLAMPED( newValR, 0, 255 );
			int	idxG	= CLAMPED( newValG, 0, 255 );
			int	idxB	= CLAMPED( newValB, 0, 255 );

			int	newcR	= VALUES_15BPP[idxR >> 3];	//	x >> 3 is the same as x / 8
			int	newcG	= VALUES_15BPP[idxG >> 3];	//	x >> 3 is the same as x / 8
			int	newcB	= VALUES_15BPP[idxB >> 3];	//	x >> 3 is the same as x / 8

            prow[x * 3 + 0]	= newcB;
            prow[x * 3 + 1]	= newcG;
            prow[x * 3 + 2]	= newcR;

			int cerrorB	= newValB - newcB;
			int cerrorG	= newValG - newcG;
			int cerrorR	= newValR - newcR;

			int	idx = i+1;
			if( x+1 < width )
			{
				errorR[idx] += (cerrorR * f7_16);
				errorG[idx] += (cerrorG * f7_16);
				errorB[idx] += (cerrorB * f7_16);
			}
			
			idx += width - 2;
			if( x-1 > 0 && y+1 < height )
			{
				errorR[idx] += (cerrorR * f3_16);
				errorG[idx] += (cerrorG * f3_16);
				errorB[idx] += (cerrorB * f3_16);
			}
			
			idx++;
			if( y+1 < height )
			{
				errorR[idx] += (cerrorR * f5_16);
				errorG[idx] += (cerrorG * f5_16);
				errorB[idx] += (cerrorB * f5_16);
			}
			
			idx++;
			if( x+1 < width && y+1 < height )
			{
				errorR[idx] += (cerrorR * f1_16);
				errorG[idx] += (cerrorG * f1_16);
				errorB[idx] += (cerrorB * f1_16);
			}
		}
	}

	free( errorB );
	free( errorG );
	free( errorR );
}
/////////////////////////////////////////////////////////////////////////////

//	Color Floyd-Steinberg dither using 18 bits per pixel (6 bit per color plane)
void	makeDitherFSRgb18bpp( BYTE* pixels, int width, int height )	noexcept
{
	const int	size	= width * height;

	int*	errorB	= (int*)malloc( size * sizeof(int) );
	int*	errorG	= (int*)malloc( size * sizeof(int) );
	int*	errorR	= (int*)malloc( size * sizeof(int) );

	//	Clear the errors buffer.
	memset( errorB, 0, size * sizeof(int) );
	memset( errorG, 0, size * sizeof(int) );
	memset( errorR, 0, size * sizeof(int) );

	//~~~~~~~~

	int	i	= 0;

	for( int y = 0; y < height; y++ )
	{
		BYTE*   prow   = pixels + ( y * width * 3 );

		for( int x = 0; x < width; x++,i++ )
		{
			const int	blue	= prow[x * 3 + 0];
            const int	green	= prow[x * 3 + 1];
            const int	red		= prow[x * 3 + 2];

			int	newValB	= (int)blue	 + (errorB[i] >> 8);	//	PixelBlue  + error correctionB
			int	newValG	= (int)green + (errorG[i] >> 8);	//	PixelGreen + error correctionG
			int	newValR	= (int)red	 + (errorR[i] >> 8);	//	PixelRed   + error correctionR

			//	The error could produce values beyond the borders, so need to keep the color in range
			int	idxR	= CLAMPED( newValR, 0, 255 );
			int	idxG	= CLAMPED( newValG, 0, 255 );
			int	idxB	= CLAMPED( newValB, 0, 255 );

			int	newcR	= VALUES_18BPP[idxR >> 2];	//	x >> 2 is the same as x / 4
			int	newcG	= VALUES_18BPP[idxG >> 2];	//	x >> 2 is the same as x / 4
			int	newcB	= VALUES_18BPP[idxB >> 2];	//	x >> 2 is the same as x / 4

            prow[x * 3 + 0]	= newcB;
            prow[x * 3 + 1]	= newcG;
            prow[x * 3 + 2]	= newcR;

			int cerrorB	= newValB - newcB;
			int cerrorG	= newValG - newcG;
			int cerrorR	= newValR - newcR;

			int	idx = i+1;
			if( x+1 < width )
			{
				errorR[idx] += (cerrorR * f7_16);
				errorG[idx] += (cerrorG * f7_16);
				errorB[idx] += (cerrorB * f7_16);
			}
			
			idx += width - 2;
			if( x-1 > 0 && y+1 < height )
			{
				errorR[idx] += (cerrorR * f3_16);
				errorG[idx] += (cerrorG * f3_16);
				errorB[idx] += (cerrorB * f3_16);
			}
			
			idx++;
			if( y+1 < height )
			{
				errorR[idx] += (cerrorR * f5_16);
				errorG[idx] += (cerrorG * f5_16);
				errorB[idx] += (cerrorB * f5_16);
			}
			
			idx++;
			if( x+1 < width && y+1 < height )
			{
				errorR[idx] += (cerrorR * f1_16);
				errorG[idx] += (cerrorG * f1_16);
				errorB[idx] += (cerrorB * f1_16);
			}
		}
	}

	free( errorB );
	free( errorG );
	free( errorR );
}
/////////////////////////////////////////////////////////////////////////////

//#define	SIERRA_LITE_COEF( v, err )	((( (err) * ((v) * 100)) / 4) / 100)
#define	SIERRA_LITE_COEF( v, err )	((( (err) * ((v) << 8)) >> 2) >> 8)

//	This is the ultimate method for SierraLite colored Diher
//	ncolors - number of colors diapazons to use. Valid values 0..255, but interesed are 0..40
//	1		- color (1 bit per color plane,  3 bits per pixel)
//	3		- color (2 bit per color plane,  6 bits per pixel)
//	7		- color (3 bit per color plane,  9 bits per pixel)
//	15		- color (4 bit per color plane, 12 bits per pixel)
//	31		- color (5 bit per color plane, 15 bits per pixel)
void	makeDitherSierraLiteRgbNbpp( BYTE* pixels, int width, int height, int ncolors )	noexcept
{
	int	divider	= 256 / ncolors;

	const int	size	= width * height;

	int*	errorB	= (int*)malloc( size * sizeof(int) );
	int*	errorG	= (int*)malloc( size * sizeof(int) );
	int*	errorR	= (int*)malloc( size * sizeof(int) );

	//	Clear the errors buffer.
	memset( errorB, 0, size * sizeof(int) );
	memset( errorG, 0, size * sizeof(int) );
	memset( errorR, 0, size * sizeof(int) );

	//~~~~~~~~

	int	i	= 0;

	for( int y = 0; y < height; y++ )
	{
		BYTE*   prow   = pixels + ( y * width * 3 );

		for( int x = 0; x < width; x++,i++ )
		{
			const int	blue	= prow[x * 3 + 0];
            const int	green	= prow[x * 3 + 1];
            const int	red		= prow[x * 3 + 2];

			const int	newValB	= blue	+ errorB[i];
			const int	newValG	= green + errorG[i];
			const int	newValR	= red	+ errorR[i];

			int	i1	= newValB / divider;	CLAMP( i1, 0, ncolors );
			int	i2	= newValG / divider;	CLAMP( i2, 0, ncolors );
			int	i3	= newValR / divider;	CLAMP( i3, 0, ncolors );

			//	If you want to compress the image, use the values of i1,i2,i3
			//	they have values in the range 0..ncolors
			//	So if the ncolors is 4 - you have values: 0,1,2,3 which is encoded in 2 bits
			//	2 bits for 3 planes == 6 bits per pixel

			const int	newcB	= CLAMPED( i1 * divider, 0, 255 );	//	blue
			const int	newcG	= CLAMPED( i2 * divider, 0, 255 );	//	green
			const int	newcR	= CLAMPED( i3 * divider, 0, 255 );	//	red

            prow[x * 3 + 0]	= newcB;
            prow[x * 3 + 1]	= newcG;
            prow[x * 3 + 2]	= newcR;

			const int cerrorB	= (newValB - newcB);
			const int cerrorG	= (newValG - newcG);
			const int cerrorR	= (newValR - newcR);

			int	idx = i;
			if( x+1 < width )
			{
				errorR[idx+1] += SIERRA_LITE_COEF( 2, cerrorR );
				errorG[idx+1] += SIERRA_LITE_COEF( 2, cerrorG );
				errorB[idx+1] += SIERRA_LITE_COEF( 2, cerrorB );
			}
			
			idx += width;
			if( y + 1 < height )
			{
				if( x-1 > 0 && y+1 < height )
				{
					errorR[idx-1] += SIERRA_LITE_COEF( 1, cerrorR );
					errorG[idx-1] += SIERRA_LITE_COEF( 1, cerrorG );
					errorB[idx-1] += SIERRA_LITE_COEF( 1, cerrorB );
				}

				errorR[idx] += SIERRA_LITE_COEF( 1, cerrorR );
				errorG[idx] += SIERRA_LITE_COEF( 1, cerrorG );
				errorB[idx] += SIERRA_LITE_COEF( 1, cerrorB );
			}
		}
	}

	free( errorB );
	free( errorG );
	free( errorR );
}
/////////////////////////////////////////////////////////////////////////////

//	Black-white Sierra Lite dithering (variation of Floyd-Steinberg with less computational cost)
void	makeDitherSierraLite( BYTE* pixels, int width, int height )	noexcept
{
	//	To avoid real number calculations, I will raise the level of INT arythmetics by shifting with 8 bits to the left ( << 8 )
	//	Later, when it is necessary will return to te normal level by shifting back 8 bits to the right ( >> 8 )
	//	    X   2
    //	1   1
    //	  (1/4)

	//~~~~~~~~

	const int	size	= width * height;

	int*	error	= (int*)malloc( size * sizeof(int) );

	//	Clear the errors buffer.
	memset( error, 0, size * sizeof(int) );

	//~~~~~~~~

	int	i	= 0;

	for( int y = 0; y < height; y++ )
	{
		for( int x = 0; x < width; x++,i++ )
		{
			const pixel	blue	= pixels[x * 3 + 0];
            const pixel	green	= pixels[x * 3 + 1];
            const pixel	red		= pixels[x * 3 + 2];

			//	Get the pixel gray value.
			int	newVal	= (red + green + blue) / 3 + error[i];		//	PixelGray + error correction
			int	newc	= (newVal < 128 ? 0 : 255);

			pixels[x * 3 + 0]	= newc;
			pixels[x * 3 + 1]	= newc;
			pixels[x * 3 + 2]	= newc;

			//	Correction - the new error
			const int	cerror	= newVal - newc;

			int idx = i;
			if( x + 1 < width )
				error[idx+1] += SIERRA_LITE_COEF( 2, cerror );

			idx += width;
			if( y + 1 < height )
			{
				if( x-1 >= 0 )
					error[idx-1] += SIERRA_LITE_COEF( 1, cerror );

				error[idx] += SIERRA_LITE_COEF( 1, cerror );
			}
		}
		
		pixels	+= width*3;
	}

	free( error );
}
/////////////////////////////////////////////////////////////////////////////


//#define	SIERRA_COEF( v, err )	((( (err) * ((v) * 100)) / 32) / 100)
#define	SIERRA_COEF( v, err )	((( (err) * ((v) << 8)) >> 5) >> 8)

//	This is the ultimate method for SierraLite colored Diher
//	ncolors - number of colors diapazons to use. Valid values 0..255, but interesed are 0..40
//	1		- color (1 bit per color plane,  3 bits per pixel)
//	3		- color (2 bit per color plane,  6 bits per pixel)
//	7		- color (3 bit per color plane,  9 bits per pixel)
//	15		- color (4 bit per color plane, 12 bits per pixel)
//	31		- color (5 bit per color plane, 15 bits per pixel)
void	makeDitherSierraRgbNbpp( BYTE* pixels, int width, int height, int ncolors )	noexcept
{
	int	divider	= 256 / ncolors;

	const int	size	= width * height;

	int*	errorB	= (int*)malloc( size * sizeof(int) );
	int*	errorG	= (int*)malloc( size * sizeof(int) );
	int*	errorR	= (int*)malloc( size * sizeof(int) );

	//	Clear the errors buffer.
	memset( errorB, 0, size * sizeof(int) );
	memset( errorG, 0, size * sizeof(int) );
	memset( errorR, 0, size * sizeof(int) );

	//~~~~~~~~

	int	i	= 0;

	for( int y = 0; y < height; y++ )
	{
		BYTE*   prow   = pixels + ( y * width * 3 );

		for( int x = 0; x < width; x++,i++ )
		{
			const int	blue	= prow[x * 3 + 0];
            const int	green	= prow[x * 3 + 1];
            const int	red		= prow[x * 3 + 2];

			const int	newValB	= blue	+ errorB[i];
			const int	newValG	= green + errorG[i];
			const int	newValR	= red	+ errorR[i];

			int	i1	= newValB / divider;	CLAMP( i1, 0, ncolors );
			int	i2	= newValG / divider;	CLAMP( i2, 0, ncolors );
			int	i3	= newValR / divider;	CLAMP( i3, 0, ncolors );

			//	If you want to compress the image, use the values of i1,i2,i3
			//	they have values in the range 0..ncolors
			//	So if the ncolors is 4 - you have values: 0,1,2,3 which is encoded in 2 bits
			//	2 bits for 3 planes == 6 bits per pixel

			const int	newcB	= CLAMPED( i1 * divider, 0, 255 );	//	blue
			const int	newcG	= CLAMPED( i2 * divider, 0, 255 );	//	green
			const int	newcR	= CLAMPED( i3 * divider, 0, 255 );	//	red

            prow[x * 3 + 0]	= newcB;
            prow[x * 3 + 1]	= newcG;
            prow[x * 3 + 2]	= newcR;

			const int cerrorB	= (newValB - newcB);
			const int cerrorG	= (newValG - newcG);
			const int cerrorR	= (newValR - newcR);

			int idx = i;
			if( x + 1 < width )
			{
				errorR[idx+1] += SIERRA_COEF( 5, cerrorR );
				errorG[idx+1] += SIERRA_COEF( 5, cerrorG );
				errorB[idx+1] += SIERRA_COEF( 5, cerrorB );
			}

			if( x + 2 < width )
			{
				errorR[idx+2] += SIERRA_COEF( 3, cerrorR );
				errorG[idx+2] += SIERRA_COEF( 3, cerrorG );
				errorB[idx+2] += SIERRA_COEF( 3, cerrorB );
			}

			if( y + 1 < height )
			{
				idx += width;
				if( x-2 >= 0 )
				{
					errorR[idx-2] += SIERRA_COEF( 2, cerrorR );
					errorG[idx-2] += SIERRA_COEF( 2, cerrorG );
					errorB[idx-2] += SIERRA_COEF( 2, cerrorB );
				}


				if( x-1 >= 0 )
				{
					errorR[idx-1] += SIERRA_COEF( 4, cerrorR );
					errorG[idx-1] += SIERRA_COEF( 4, cerrorG );
					errorB[idx-1] += SIERRA_COEF( 4, cerrorB );
				}

				errorR[idx] += SIERRA_COEF( 5, cerrorR );
				errorG[idx] += SIERRA_COEF( 5, cerrorG );
				errorB[idx] += SIERRA_COEF( 5, cerrorB );

				if( x+1 < width )
				{
					errorR[idx+1] += SIERRA_COEF( 4, cerrorR );
					errorG[idx+1] += SIERRA_COEF( 4, cerrorG );
					errorB[idx+1] += SIERRA_COEF( 4, cerrorB );
				}

				if( x+2 < width )
				{
					errorR[idx+2] += SIERRA_COEF( 2, cerrorR );
					errorG[idx+2] += SIERRA_COEF( 2, cerrorG );
					errorB[idx+2] += SIERRA_COEF( 2, cerrorB );
				}
			}

			if( y + 2 < height )
			{
				idx	+= width;
				if( x-1 >= 0 )
				{
					errorR[idx-1] += SIERRA_COEF( 2, cerrorR );
					errorG[idx-1] += SIERRA_COEF( 2, cerrorG );
					errorB[idx-1] += SIERRA_COEF( 2, cerrorB );
				}

				errorR[idx] += SIERRA_COEF( 3, cerrorR );
				errorG[idx] += SIERRA_COEF( 3, cerrorG );
				errorB[idx] += SIERRA_COEF( 3, cerrorB );

				if( x+1 < width )
				{
					errorR[idx+1] += SIERRA_COEF( 2, cerrorR );
					errorG[idx+1] += SIERRA_COEF( 2, cerrorG );
					errorB[idx+1] += SIERRA_COEF( 2, cerrorB );
				}
			}

		}
	}

	free( errorB );
	free( errorG );
	free( errorR );
}
/////////////////////////////////////////////////////////////////////////////

//	Black-white Sierra Lite dithering (variation of Floyd-Steinberg with less computational cost)
void	makeDitherSierra( BYTE* pixels, int width, int height )	noexcept
{
	//	To avoid real number calculations, I will raise the level of INT arythmetics by shifting with 8 bits to the left ( << 8 )
	//	Later, when it is necessary will return to te normal level by shifting back 8 bits to the right ( >> 8 )
	//	       X   5   3
    //	2   4  5   4   2
    //	    2  3   2
    //	    (1/32)

	//~~~~~~~~

	const int	size	= width * height;

	int*	error	= (int*)malloc( size * sizeof(int) );

	//	Clear the errors buffer.
	memset( error, 0, size * sizeof(int) );

	//~~~~~~~~

	int	i	= 0;

	for( int y = 0; y < height; y++ )
	{
		for( int x = 0; x < width; x++,i++ )
		{
			const pixel	blue	= pixels[x * 3 + 0];
            const pixel	green	= pixels[x * 3 + 1];
            const pixel	red		= pixels[x * 3 + 2];

			//	Get the pixel gray value.
			int	newVal	= (red + green + blue) / 3 + error[i];		//	PixelGray + error correction
			int	newc	= (newVal < 128 ? 0 : 255);

			pixels[x * 3 + 0]	= newc;
			pixels[x * 3 + 1]	= newc;
			pixels[x * 3 + 2]	= newc;

			//	Correction - the new error
			const int	cerror	= newVal - newc;

			int idx = i;
			if( x + 1 < width )
				error[idx+1] += SIERRA_COEF( 5, cerror );

			if( x + 2 < width )
				error[idx+2] += SIERRA_COEF( 3, cerror );

			if( y + 1 < height )
			{
				idx += width;
				if( x-2 >= 0 )
					error[idx-2] += SIERRA_COEF( 2, cerror );
				
				if( x-1 >= 0 )
					error[idx-1] += SIERRA_COEF( 4, cerror );

				error[idx] += SIERRA_COEF( 5, cerror );

				if( x+1 < width )
					error[idx+1] += SIERRA_COEF( 4, cerror );

				if( x+2 < width )
					error[idx+2] += SIERRA_COEF( 2, cerror );
			}

			if( y + 2 < height )
			{
				idx	+= width;
				if( x-1 >= 0 )
					error[idx-1] += SIERRA_COEF( 2, cerror );

				error[idx] += SIERRA_COEF( 3, cerror );

				if( x+1 < width )
					error[idx+1] += SIERRA_COEF( 2, cerror );
			}
		}
		
		pixels	+= width*3;
	}

	free( error );
}
/////////////////////////////////////////////////////////////////////////////
