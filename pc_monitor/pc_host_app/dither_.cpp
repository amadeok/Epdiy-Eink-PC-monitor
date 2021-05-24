#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>

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




extern "C" {

unsigned char makeDitherBayer16( unsigned char* pixels, int width, int height )noexcept
{
	int	col	= 0;
	int	row	= 0;

	for( int y = 0; y < height; y++ )
	{
		row	= y & 15;	//	y % 16
        
		for( int x = 0; x < width; x++ )
		{
			col	= x & 15;	//	x % 16

			const int	blue	= pixels[x * 3 + 0];
            const int	green	= pixels[x * 3 + 1];
            const int	red		= pixels[x * 3 + 2];

			int	color	= ((red + green + blue)/3 < BAYER_PATTERN_16X16[col][row] ? 0 : 255);
			
            pixels[x * 3 + 0]	= color;	//	blue
            pixels[x * 3 + 1]	= color;	//	green
            pixels[x * 3 + 2]	= color;	//	red
		}

		pixels	+= width * 3;
	}
    return pixels[0];
}}


extern "C" {

void	makeDitherBayer8( unsigned char* pixels, int width, int height )	noexcept
{
	int	col	= 0;
	int	row	= 0;

	for( int y = 0; y < height; y++ )
	{
		row	= y & 7;		//	% 8;
        
		for( int x = 0; x < width; x++ )
		{
			col	= x & 7;	//	% 8;

			const int	blue	= pixels[x * 3 + 0];
            const int	green	= pixels[x * 3 + 1];
            const int	red		= pixels[x * 3 + 2];

			int	color	= ((red + green + blue)/3 < BAYER_PATTERN_8X8[col][row] ? 0 : 255);
			
            pixels[x * 3 + 0]	= color;	//	blue
            pixels[x * 3 + 1]	= color;	//	green
            pixels[x * 3 + 2]	= color;	//	red
		}

		pixels	+= width * 3;
	}
}}
/////////////////////////////////////////////////////////////////////////////
extern "C" {

void	makeDitherBayer4( unsigned char * pixels, int width, int height )	noexcept
{
	int	col	= 0;
	int	row	= 0;

	for( int y = 0; y < height; y++ )
	{
		row	= y & 3;	//	% 4
        
		for( int x = 0; x < width; x++ )
		{
			col	= x & 3;	//	% 4

			const int	blue	= pixels[x * 3 + 0];
            const int	green	= pixels[x * 3 + 1];
            const int	red		= pixels[x * 3 + 2];

			int	color	= ((red + green + blue)/3 < BAYER_PATTERN_4X4[col][row] ? 0 : 255);
			
            pixels[x * 3 + 0]	= color;	//	blue
            pixels[x * 3 + 1]	= color;	//	green
            pixels[x * 3 + 2]	= color;	//	red
		}

		pixels	+= width * 3;
	}
}}

/////////////////////////////////////////////////////////////////////////////
extern "C" {
void	makeDitherBayer3( unsigned char* pixels, int width, int height )	noexcept
{
	int	col	= 0;
	int	row	= 0;

	for( int y = 0; y < height; y++ )
	{
		row	= y % 3;
        
		for( int x = 0; x < width; x++ )
		{
			col	= x % 3;

			const int	blue	= pixels[x * 3 + 0];
            const int	green	= pixels[x * 3 + 1];
            const int	red		= pixels[x * 3 + 2];

			int	color	= ((red + green + blue)/3 < BAYER_PATTERN_3X3[col][row] ? 0 : 255);
			
            pixels[x * 3 + 0]	= color;	//	blue
            pixels[x * 3 + 1]	= color;	//	green
            pixels[x * 3 + 2]	= color;	//	red
		}

		pixels	+= width * 3;
	}
}}
/////////////////////////////////////////////////////////////////////////////
extern "C" {
void	makeDitherBayer2( unsigned char* pixels, int width, int height )	noexcept
{
	int	col	= 0;
	int	row	= 0;

	for( int y = 0; y < height; y++ )
	{
		row	= y & 1;	//	y % 2
        
		for( int x = 0; x < width; x++ )
		{
			col	= x & 1;	//	x % 2

			const int	blue	= pixels[x * 3 + 0];
            const int	green	= pixels[x * 3 + 1];
            const int	red		= pixels[x * 3 + 2];

			int	color	= ((red + green + blue)/3 < BAYER_PATTERN_2X2[col][row] ? 0 : 255);
			
            pixels[x * 3 + 0]	= color;	//	blue
            pixels[x * 3 + 1]	= color;	//	green
            pixels[x * 3 + 2]	= color;	//	red
		}

		pixels	+= width * 3;
	}
}}

#define	f7_16	112		//const int	f7	= (7 << 8) / 16;
#define	f5_16	 80		//const int	f5	= (5 << 8) / 16;
#define	f3_16	 48		//const int	f3	= (3 << 8) / 16;
#define	f1_16	 16		//const int	f1	= (1 << 8) / 16;

#define	FS_COEF( v, err )	(((err) * ((v) << 8)) >> 12)

//	Back-white Floyd-Steinberg dither
extern "C" {

void	makeDitherFS( unsigned char* pixels, int width, int height )	noexcept
{
	const int	size	= width * height;

	int*	error	= (int*)malloc( size * sizeof(int) );

	//	Clear the errors buffer.
	memset( error, 0, size * sizeof(int) );

	//~~~~~~~~

	int	i	= 0;

	for( int y = 0; y < height; y++ )
	{
		unsigned char*   prow   = pixels + ( y * width * 3 );

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
}}


#define	SIERRA_LITE_COEF( v, err )	((( (err) * ((v) << 8)) >> 2) >> 8)

//	Black-white Sierra Lite dithering (variation of Floyd-Steinberg with less computational cost)
extern "C" {

void	makeDitherSierraLite( unsigned char* pixels, int width, int height )	noexcept
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
			const int	blue	= pixels[x * 3 + 0];
            const int	green	= pixels[x * 3 + 1];
            const int	red		= pixels[x * 3 + 2];

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
}}


#define	SIERRA_COEF( v, err )	((( (err) * ((v) << 8)) >> 5) >> 8)

extern "C" {

void	makeDitherSierra( unsigned char* pixels, int width, int height )	noexcept
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
			const int	blue	= pixels[x * 3 + 0];
            const int	green	= pixels[x * 3 + 1];
            const int	red		= pixels[x * 3 + 2];

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
}}
/////////////////////////////////////////////////////////////////////////////
extern "C" {

int main(){
	return 0;
}}