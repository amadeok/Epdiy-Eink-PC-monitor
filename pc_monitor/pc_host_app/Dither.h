
#ifndef __DITHER_H__
#define	__DITHER_H__

/*****************************************************************************
FILE       : Dither.h
Author     : Svetoslav Chekanov
Description: Collection of dithering algorithms

Copyright (c) 2014 Brosix
*****************************************************************************/
/* #    Revisions    # */

/////////////////////////////////////////////////////////////////////////////
//	Ordered dither using matrix
/////////////////////////////////////////////////////////////////////////////

void	makeDitherBayer16( unsigned char* pixels, int width, int height )    noexcept;
void	makeDitherBayer8 ( unsigned char* pixels, int width, int height )	noexcept;
void	makeDitherBayer4 ( unsigned char* pixels, int width, int height )	noexcept;
void	makeDitherBayer3 ( unsigned char* pixels, int width, int height )	noexcept;
void	makeDitherBayer2 ( unsigned char* pixels, int width, int height )	noexcept;

/////////////////////////////////////////////////////////////////////////////
//	Floyd-Steinberg dither
/////////////////////////////////////////////////////////////////////////////

void	makeDitherFS		( unsigned char* pixels, int width, int height )	noexcept;

void	makeDitherSierraLite	   ( unsigned char* pixels, int width, int height )	noexcept;
void	makeDitherSierra		   ( unsigned char* pixels, int width, int height )	noexcept;

/////////////////////////////////////////////////////////////////////////////

#endif // !__DITHER_H__
