#include "sprpck.h"
#include "bmp.h"

extern BYTE rgb[];
extern BYTE CollRedirect[];

BYTE *original = NULL;		// ptr to the complete converted file
int org_w,org_h;					// original size

//
// prototypes
//

  void  error(int line,char *w, ...);
  void  SaveRGB(char *filename,char *outfile,BYTE *data,int type,int size,int line);
  void  SaveSprite(char *filename,BYTE *data,int len,int line,int type);
  unsigned long  LoadFile(char *filename,BYTE **adr);
  long  ConvertFile(BYTE *in,long in_size,int type,int *in_h,int *in_w,int line);

/*******************************************************************/

long ConvertBMP(BYTE * in,long in_size,int *in_w, int *in_h,int line)
    {
    int x,y,pad;

    long new_size = 0;
    int bpp,bpl;
    BYTE r,g,b;

    BYTE *pByte, *pByte2, *help;
    PBITMAPFILEHEADER inbmpfile;
    PBITMAPINFO inbmpinfo;

    short nColor[16*16*16];	// Used for counting the colors
    int nActColIndex;
    BYTE bActColor;         // actual color (4 or 8 bit)
	  int nActColor;          // actual color (24 bit)
    BYTE bOrgColIndex[256];  // used for adjusting a 256 color palette
    short nOrgColIndex[16];  // used for adjusting a 24bit palette

//
// BS42 07/08/98
// cleaned up structure-loading !
//

    inbmpfile = (PBITMAPFILEHEADER) malloc (sizeof(BITMAPFILEHEADER));
    
    inbmpfile->bfType    = *(WORD *) (in);
    inbmpfile->bfSize    = *(DWORD *)(in + 2);
    inbmpfile->bfOffBits = *(DWORD *)(in + 10); 
       
//    inbmpfile = (PBITMAPFILEHEADER) in; 


    inbmpinfo = (PBITMAPINFO) malloc (sizeof(BITMAPINFO));
    
    inbmpinfo->bmiHeader.biSize         = *(DWORD *) (in + 14);
    inbmpinfo->bmiHeader.biWidth        = *(LONG *)  (in + 18);
    inbmpinfo->bmiHeader.biHeight       = *(LONG *)  (in + 22);
    inbmpinfo->bmiHeader.biPlanes       = *(WORD *)  (in + 26);
    inbmpinfo->bmiHeader.biBitCount     = *(WORD *)  (in + 28);
    inbmpinfo->bmiHeader.biCompression  = *(DWORD *) (in + 30);
    inbmpinfo->bmiHeader.biSizeImage    = *(DWORD *) (in + 34);
    inbmpinfo->bmiHeader.biXPelsPerMeter= *(LONG *)  (in + 38);
    inbmpinfo->bmiHeader.biYPelsPerMeter= *(LONG *)  (in + 42);
    inbmpinfo->bmiHeader.biClrUsed	= *(DWORD *) (in + 46);
    inbmpinfo->bmiHeader.biClrImportant = *(DWORD *) (in + 50);
    
//    inbmpinfo = (PBITMAPINFO) (in + sizeof(BITMAPFILEHEADER));

/*
    printf("WORD %d, DWORD %d, BYTE %d, UINT %d, LONG %d\n",
     sizeof(WORD), sizeof(DWORD), sizeof(BYTE), sizeof(UINT), sizeof(LONG));
    printf("sizeof(BITMAPFILEHEADER)= %d\n", sizeof(BITMAPFILEHEADER));
    printf("sizeof(BITMAPINFO)= %d\n", sizeof(BITMAPINFO));
   	printf("bfType = %X, bfSize = %lu !\n", inbmpfile->bfType, inbmpfile->bfSize);
*/
    if (inbmpfile->bfType == 0x4d42	// 0x4d42 --> 'BM' in Intel
        && inbmpfile->bfSize == (DWORD) in_size)
        {
        if (verbose)
           {
           printf("BMP recognized!\n");

           printf("Bitmap file header:\n");
           printf("filesize: %d\n", inbmpfile->bfSize);
           printf("offset to pixels: %d\n", inbmpfile->bfOffBits);

           printf("Bitmap info header:\n");
           printf("linesize in pixels: %d\n", inbmpinfo->bmiHeader.biSize);
           printf("Width in pixels: %d\n", inbmpinfo->bmiHeader.biWidth);
           printf("Height in pixels: %d\n", inbmpinfo->bmiHeader.biHeight);
           printf("Planes: %u\n", inbmpinfo->bmiHeader.biPlanes);
           printf("BitCount: %u\n", inbmpinfo->bmiHeader.biBitCount);
           printf("Compression: %d (0 = BI_RGB, 1 = BI_RLE8, 2 = BI_RLE4)\n", inbmpinfo->bmiHeader.biCompression);
           printf("SizeImage: %d\n", inbmpinfo->bmiHeader.biSizeImage);
           printf("XPelsPerMeter: %d\n", inbmpinfo->bmiHeader.biXPelsPerMeter);
           printf("YPelsPerMeter: %d\n", inbmpinfo->bmiHeader.biYPelsPerMeter);
           printf("ColorsUsed: %d\n", inbmpinfo->bmiHeader.biClrUsed);
           printf("ColorsImportant: %d\n", inbmpinfo->bmiHeader.biClrImportant);
           } // if (verbose)

        if (inbmpinfo->bmiHeader.biCompression != BI_RGB)
           error(line, "RLE-bitmaps are not supported!");

        if (inbmpinfo->bmiHeader.biBitCount != 4 &&
            inbmpinfo->bmiHeader.biBitCount != 8 &&
            inbmpinfo->bmiHeader.biBitCount != 24)
           error(line, "Only 4-, 8- or 24-bit-per-pixel bitmaps are supported!");

        *in_w = org_w = inbmpinfo->bmiHeader.biWidth;
        *in_h = org_h = inbmpinfo->bmiHeader.biHeight;
        bpl = inbmpinfo->bmiHeader.biBitCount;
        bpp = inbmpinfo->bmiHeader.biPlanes;

        new_size = (long)( org_w * org_h );

        if ( (original = malloc(new_size)) == NULL)
           error(line,"Not enough memory for original bitmap!");


        if (verbose)
           printf("BMP-size : w = %d h = %d (%d planes) \n",org_w,org_h,bpp);


        // Scanlines are stored bottom up!
        help = original;

// old patch by nop90 
//		pByte2 = in + inbmpfile->bfOffBits + inbmpinfo->bmiHeader.biSizeImage;  // Here end the pixels :)

// new patch by kezax. This works with files exported by GIMP and files generated via devIL library
		pByte2 = in + inbmpfile->bfSize; // we start from the end of the file to read lines from top to bottom

        if (bpl == 24)
		{
           nActColIndex = 0;
           for (x = 0; x < 16; x++)	        // up to 16 colors from a 24bit-BMP
              nOrgColIndex[x] = nActColIndex;

			nActColIndex = -1;
           for (x = 0; x < 16*16*16; x++)	// Lynx colour palette range: RGB=16*16*16
              nColor[x] = nActColIndex;


			// A scanline ends always on a 32 bit boundary !
           pad = (4 - ((org_w*3) % 4)) % 4;

           // First count the used colours
           // if up to 16, the colour indexes are adjusted
		   // nActColIndex should be -1 now !!!!!!!
           for (y = org_h; y; y--)
		   {
				pByte2 -= (org_w*3 + pad); // Scanlines are stored left to right!
				pByte = pByte2;
				for (x = org_w; x; x--)
				{
					// RGBTriple
					b = *pByte++ >> 4; /* rgbBlue */
					g = *pByte++ >> 4; /* rgbGreen */
					r = *pByte++ >> 4; /* rgbRed */

					nActColor = g*16*16 + b*16 + r;  // 
					if (nColor[nActColor] == -1) // A new Color encountered
					{
						nActColIndex++;
						nColor[nActColor] = nActColIndex;
						if (nActColIndex <16)
							nOrgColIndex[nActColIndex] = nActColor;// Palette adjusting
					}

					*help++ = nColor[nActColor];
				}  // x
			} // y

           if (verbose)
              printf("Colours used: %d \n", nActColIndex+1);

		   if (nActColIndex >= 16)
	           error(line, "Only 24-bit-per-pixel bitmaps with up to 16 colours are supported!");

           // Now adjust the color palette
           help = rgb;
           for (x = 0; x < 16; x++)
              {
              *help++ = nOrgColIndex[x] >> 8;	// green
              *help++ = nOrgColIndex[x] & 0xff;	// blue, red
              }

		}	

        if (bpl == 8)
           {
           nActColIndex = -1;
           for (x = 0; x < 256; x++)
              nColor[x] = nActColIndex;

           // A scanline ends always on a 32 bit boundary !
           pad = (4 - (org_w % 4)) % 4;

           // First count the used colours
           // if up to 16, the colour indexes are adjusted
           // else the colour indexes are truncated
           for (y = org_h; y; y--)
              {
              pByte2 -= (org_w + pad); // Scanlines are stored left to right!
              pByte = pByte2;

              for (x = org_w; x; x--)
                  {
                  bActColor = *pByte++;
                  if (nColor[bActColor] == -1) // A new Color encountered
                     {
                     nActColIndex++;
                     nColor[bActColor] = nActColIndex;
                     bOrgColIndex[nActColIndex] = bActColor;// Palette adjusting
                     }
                  }  // x
              } // y

           if (verbose)
              printf("Colours used: %d \n", nActColIndex+1);

           // Now transfer the colour indexes from in to original
           pByte2 = in + inbmpfile->bfOffBits + inbmpinfo->bmiHeader.biSizeImage;  // Here end the pixels :)

           for (y = org_h; y; y--)
              {
              pByte2 -= (org_w + pad); // Scanlines are stored left to right!
              pByte = pByte2;

              if (nActColIndex < 16)
                 {
                 for (x = org_w; x; x--)
                    *help++ = nColor[*pByte++];
                 }
              else  // > 16 colors: truncate the indexes
                 {
                 for (x = org_w; x; x--)
                    *help++ = *pByte++ & 0x0f;
                 }
              } // y

           // Now adjust the color palette
// 10/20/2018: nop90 - there are different bmp types with different BITMAPINFOHEADER sizes. Better use size field           
           pByte = in + sizeof(BITMAPFILEHEADER) + inbmpinfo->bmiHeader.biSize;//sizeof(BITMAPINFOHEADER);
           help = rgb;
           for (x = 0; x < 16; x++)
              {
              // sizeof(RGBQUAD) = 4
              b = (*(pByte + bOrgColIndex[x]*4 + 0)) >> 4; /* rgbBlue */
              g = (*(pByte + bOrgColIndex[x]*4 + 1)) >> 4; /* rgbGreen */
              r = (*(pByte + bOrgColIndex[x]*4 + 2)) >> 4; /* rgbRed */
              *help++ = g;
              *help++ = ( b<<4 ) | r;
              }
           } // 8 bits

        if (bpl == 4)
           {
           // A scanline ends always on a 32 bit boundary !
           pad = (8 - (org_w % 8)) % 8;
           if (verbose)
              printf("org_w %d, pad %d\n", org_w, pad);
           for (y = org_h; y; y--)
              {
              pByte2 -= (org_w + pad)/2; // Scanlines are stored left to right!
              pByte = pByte2;

              for (x = org_w; x > 0;)
                 {
                 *help++ = (*pByte)>>4;	// Erst die oberen 4 bit nehmen!
                 x--;
                 if (x) // No more nybble-pixels?
                    {
                    *help++ = (*pByte++) & 0x0f;	// Dann die unteren 4 bit!
                    x--;
                    }
                 }
              }
           // Erstellung der Lynx-Farbpalette
           // Anpassung an den Wertebereich des Lynx: 4 statt 8 bit pro Farbe
// 10/20/2018: nop90 - there are different bmp types with different BITMAPINFOHEADER sizes. Better use size field           
           pByte = in + sizeof(BITMAPFILEHEADER) + inbmpinfo->bmiHeader.biSize-2;//sizeof(BITMAPINFOHEADER);
           help = rgb;
           for (x = 16 ; x ; --x)
              {
              // RGBQUAD !!!
              b = (*pByte++) >> 4; /* rgbBlue */
              g = (*pByte++) >> 4; /* rgbGreen */
              r = (*pByte++) >> 4; /* rgbRed */
              pByte++;	// skipping rgbReserved
              *help++ = g;
              *help++ = ( b<<4 ) | r;
              }

           } // 4 bits

       }
   else
       {
       printf("BMP not detected!\nSize %ld, %d\n", in_size, inbmpfile->bfSize);
       }
   free (inbmpfile);
   free (inbmpinfo);
   free(in);
   return ( new_size );

}


long ConvertPCX(BYTE * in,long in_size,int *in_w, int *in_h,int line)
    {
    int count,count_bpl,x,y,count_planes;
    BYTE shifter,i;
		int bpp,bpl,planes;
    BYTE r,g,b;
    BYTE byte,*pByte,*help;
    long new_size;
    long save_insize = in_size;

//    if ( *in != 10 || *(in+PCX_VERSION) != 5 || *(in+PCX_ENC) != 1)
//      error(line,"No or unsupported PCX-file !\n");

    *in_w = org_w = BLINTEL(in,PCX_XMAX)-BLINTEL(in,PCX_XMIN) +1 ;
    *in_h = org_h = BLINTEL(in,PCX_YMAX)-BLINTEL(in,PCX_YMIN) +1 ;

    planes = *(in+PCX_PLANES);

    bpp = (int) *(in+PCX_BPP) ;
    bpl = BLINTEL(in,PCX_BPL) ;

    if ( !((planes == 1 && bpp == 8) || (planes == 4 && bpp == 1) ))
      error(line,"No or unsupported PCX-file !\n");

    new_size = (long) ( org_w * org_h)  ;

    if ( (original = malloc(new_size)) == NULL)
      error(line,"Not enough memory for original pcx!");


    if (verbose)
      printf("PXC-size : w = %d h = %d (%d) \n",org_w,org_h,bpp);

    pByte = in+PCX_RGB;
    help = rgb;

    for (x = 16 ; x ; --x)
    {
      r = (*pByte++) >> 4; /* red */
      g = (*pByte++) >> 4; /* green */
      b = (*pByte++) >> 4; /* blue */
      *help++ = g;
      *help++ = ( b<<4 ) | r;
    }

    help  = original;
    pByte = in+PCX_DATA;

    if ( bpp == 8)
    {
      x = org_w+1;
      count_bpl = bpl;
      in_size = new_size;
      while (in_size > 0)
      {
        if ( ((byte = *pByte++) & 0xc0 )== 0xC0 )
        {
          count = byte & 0x3f;
          byte = *pByte++;
          do{
            if ( x-- ) { *help++ = byte & 15; --in_size; }
            --count_bpl;
            if ( ! count_bpl ) { x = org_w+1;  count_bpl = bpl; }
          }while (--count);
        }
        else
        {
          if ( x-- ) { *help++ = byte & 15; --in_size; }
          --count_bpl;
          if ( !count_bpl ) { x = org_w+1; count_bpl = bpl; }
        }
      } /*while*/
      if ( pByte != in+save_insize )
      {
        pByte = in+save_insize-16*3;
        help = rgb;
    
        for (x = 16 ; x ; --x)
        {
          r = (*pByte++) >> 4; /* red */
          g = (*pByte++) >> 4; /* green */
          b = (*pByte++) >> 4; /* blue */
          *help++ = g;
          *help++ = ( b<<4 ) | r;
        }
        if (verbose) printf("PCX:Using palette at the end of file !\n");
      }      
    }
    else  // 1bit / 8 planes
    {
      for ( y = org_h; y ; --y, help += org_w)
      {
        for (i = org_w ; i ; --i)
          *(help+i) = 0;

        for (count_planes = planes, shifter = 1;
             count_planes;
             --count_planes, help -= org_w, shifter <<=1)

          for ( x = org_w ; x ; )
            if ( ( (byte = *pByte++) & 0xc0 ) == 0xc0 )
              { 
                count = byte & 0x3f;
                byte = *pByte++;
                do
                {
                  for (i = 0x80; x && i; i>>=1, --x)
									{
                    if  (byte & (BYTE)i)
                      *help |= shifter;

                    help++;
									}

                  --count;
                }while ( count );
              }
              else
              { 
								// Note : bit 7 and 6 are used to as flag
								//        therefore can't be used here !!!
                for (i = 0x20; x && i  ; i>>=1, --x) 
                {  
									if  (byte & (BYTE)i)
                    *help |= shifter;
                 
                  help++;
								}
              } 
           
      }/*for (y ..*/
    }
    
   help = original;

   for (in_size = new_size ; in_size ; --in_size)
     *help++ &=15;

   free(in);
   return ( new_size );
}
/************************/
/* main conversion-loop */
/************************/
long ConvertFile(BYTE *in,
								 long in_size,
								 int type,
                 int *in_w,int *in_h,
                 int line)
{
  BYTE *pByte = NULL,*pByte2;
  long new_size = 0;


  switch (type)
  {
  case TYPE_RAW1:
    {
    int bit;
    signed char byte;

    new_size = in_size << 3;
    
    if ( (original = malloc(new_size)) == NULL)
      error(line,"Not enough memory for raw data!");
      
    pByte = original;
    pByte2 = in;
    while (in_size)
    {
      byte = *pByte2++;
      for (bit = 8 ; bit ; byte <<= 1 , --bit )
        *pByte++ = (byte < 0) ? 1 : 0;
        
      --in_size;
    }
    }
    org_w = *in_w;
    org_h = *in_h;
    free(in);
    break;

  case TYPE_RAW4:
    new_size = in_size<<1;
    if ( (original = malloc(new_size)) == NULL)
      error(line,"Not enough memory for raw data!");
      
    pByte  = original;
    pByte2 = in;
    
    while (in_size)
    {
      *pByte++ = (*pByte2)>>4;
      *pByte++ = (*pByte2++) & 0xf;
      --in_size;
    }
    org_w = *in_w;
    org_h = *in_h;
    free(in);
    break;
    
  case TYPE_RAW8:
    original = in;
    new_size = in_size;
    org_w = *in_w;
    org_h = *in_h;
    break;
    
  case TYPE_SPS:
    pByte = original = in;
    
    while (in_size)
    {
      BYTE b = *in++;
      --in_size;
      
      if ( b < ' ' ) continue; // skip LF and CR
      if ( ' ' == b )
      {
        b = 0;      }
      else if (('0' <= b) && (b <= '9'))
      {
        b = b - '0';
      }
      else if (('A' <= b) && (b <= 'F'))
      {        
      	b = b - 'A' + 10;
      }
      else if (('a' <= b) && (b <= 'f'))
      {
        b = b - 'a' + 10;
      }
        
      *pByte++ = b;
      ++new_size;
    }
    org_w = *in_w;
    org_h = *in_h;
    break;
    
  case TYPE_PCX:
    new_size = ConvertPCX(in,in_size,in_w,in_h,line);
    break;

  case TYPE_BMP:
    new_size = ConvertBMP(in,in_size,in_w,in_h,line);
    break;

	case TYPE_PI1:
    {
        short i,o;
        //      USHORT *puShort;

        *in_w = org_w = 320;
        *in_h = org_h = 200;

        original = malloc(new_size = 64000l);

        pByte2= in + 2;  // skip first 2 bytes

        pByte = rgb;	    

        for ( i = 15; i >= 0 ; --i)
        {
            register BYTE r,g,b;

            if ( (r = ((*pByte2++) & 0x07) << 1) ) ++r;
            if ( (g = (*pByte2     & 0x70) >> 3) ) ++g;
            if ( (b = ((*pByte2++) & 0x07) << 1) ) ++b;

            // printf("r %x g %x b %x \n",r,g,b);

            *pByte++ = g;
            *pByte++ = (b<<4) | r;
        }

        //	  puShort = (USHORT *)pByte2;    

        pByte = original-16;

        for ( i = 3999; i >= 0 ; --i)
        {
            register USHORT w0,w1,w2,w3;

            w3 = *((USHORT *)pByte2);
            pByte2++;
            w2 = *((USHORT *)pByte2);
            pByte2++;
            w1 = *((USHORT *)pByte2);
            pByte2++;
            w0 = *((USHORT *)pByte2);
            pByte2++;
#ifdef i386
            w3 = (w3<<8)|(w3>>8); // swap byte-order
            w2 = (w2<<8)|(w2>>8); // hey, gcc knows rorw !!!
            w1 = (w1<<8)|(w1>>8);
            w0 = (w0<<8)|(w0>>8);
#endif
            pByte += 32;

            for ( o = 15 ; o >= 0 ; --o )
            {
                *--pByte = ((w0&1)<<3) | ((w1&1)<<2) | ((w2&1)<<1) | (w3&1);
                w0 >>= 1;
                w1 >>= 1;
                w2 >>= 1;
                w3 >>= 1;
            }
        } 
    }
    break;

 } /*switch*/
 

 return ( new_size );
}
 
BYTE* HandleOffset(BYTE * original,
           			 int *in_w,int *in_h,
                 int off_x,int off_y,
                 int line)
{
	BYTE *help,*help1,*modified;
	int x,y;
	
	if ( (modified = malloc( (*in_w)*(*in_h) ) ) == 0)
	  error(line,"Out of memory (HandleOffset) !");
	

  help  = modified;
    
  help1 = original + off_y * (*in_w);
 
  *in_h -= off_y;
  *in_w -= off_x;

  for ( y = *in_h ; y ; --y)
  {
    help1 += off_x;
    for ( x = *in_w ; x ; --x)
      *help++ = *help1++;
  }

  return (modified);
}

unsigned long LoadFile(char fn[],BYTE **ptr)
{
  unsigned long len;
  int f;

  if ((f = open(fn,O_RDONLY | O_BINARY)) >= 0)
  {
    len = lseek(f,0L,SEEK_END);
    lseek(f,0L,SEEK_SET);
    if ( ( *ptr=malloc(len) ) == NULL)
      return 0;
#ifdef DEBUG
  printf("filesize: %lu\n", len);
#endif
    len  = read(f,*ptr,len);
#ifdef DEBUG
      //printf("sizeof(int): %u\n", sizeof(int));
      printf("bytes read: %lu\n", len);
#endif
    close(f);
    if (verbose) printf("Read: %s \n",fn);

    return (len);
  }else
    return 0;
}
//10/20/2018:nop90 aded parameter for redir and palette array names 
void SaveRGB(char *filename,char *outfile, BYTE *pal,int type,int size,int line)
{
  int i;
  FILE *f;
  BYTE *pal2 = pal+1,g,br;
  BYTE *pCollRedirect = CollRedirect;
  
  if ( (f = fopen(filename,"w") ) == NULL)
    error(line,"Couldn't write color-table !\n");  
  switch (type){
  case C_HEADER:
  
    fprintf(f,"char ");
    fprintf(f,outfile);
    fprintf(f,"_redir[]={");
    for (i = 1<<(size-1) ; i ; --i)
    { 
      g = *pCollRedirect++;
      br = *pCollRedirect++;
      fprintf(f,"0x%02X",(g<<4)|br);
      if (i > 1)
        putc(',',f);
    }

    fprintf(f,"};\nchar ");
    fprintf(f,outfile);
    fprintf(f,"_pal[]={\n\t");
    
    for (i = 16 ; i ; --i , pal += 2)
      fprintf(f,"0x%02X,",(int)*pal);
    fprintf(f,"\n\t");
    for (i = 16 ; i ; --i , pal2 += 2)
      if ( i > 1 )
        fprintf(f,"0x%02X,",(int)*pal2);
      else
        fprintf(f,"0x%02X};\n",(int)*pal2);

    break;
    
  case ASM_SRC:
    fprintf(f,"redir:\tdc.b ");
    for (i = 1<<(size-1) ; i ; --i)
    { 
      g = *pCollRedirect++;
      br = *pCollRedirect++;
      fprintf(f,"$%02X",(g<<4)|br);
      if (i > 1)
        putc(',',f);
    }
    fprintf(f,"\npal:\tdc.b ");
    for (i = 16; i ; --i , pal += 2)
      if ( i > 1 ) fprintf(f,"$%02X,",(int)*pal);
    else
      fprintf(f,"$%02X\n\tdc.b ",(int)*pal);
      
    for (i = 16; i ; --i , pal2 += 2)
      if ( i > 1 ) fprintf(f,"$%02X,",(int)*pal2);
    else
      fprintf(f,"$%02X\n",(int)*pal2);
      
    break;
  case LYXASS_SRC:
    fprintf(f,"redir:\tdc.b ");
    for (i = 1<<(size-1) ; i ; --i)
    { 
      g = *pCollRedirect++;
      br = *pCollRedirect++;
      fprintf(f,"$%02X",(g<<4)|br);
      if (i > 1)
        putc(',',f);
    }

    fprintf(f,"\npal:\tDP ");
    for (i = 16; i ; --i)
    { g = *pal++; br = *pal++;
      if ( i > 1 )
        fprintf(f,"%X%02X,",g,br);
      else
        fprintf(f,"%X%02X\n",g,br);
    }
    break;
  }/*switch*/
      
  fclose(f);
  if (verbose)
    printf("%s written.\n",filename);
}

void SaveSprite(char *filename,BYTE *ptr,int size,int line,int type)
{
  if ( type != C_HEADER )
  {
    int handle;
     
    if ( (handle = open(filename,O_CREAT | O_TRUNC | O_BINARY | O_RDWR, 0644)) <0 )
    {
      error(line,"Couldn't open %s for writing !\n",filename);
    }

    if ( write(handle,ptr,size) != size )
      error(line,"Couldn't write %s !\n",filename);
    close(handle);
  }
  else
  {
    FILE * out;
    char label[34] = "_";
    int i,o,segdata;
    char * dot;
    
    if ( (out = fopen(filename,"wb")) == NULL )
	error(line,"Couldn't open %s !\n",filename);

    dot = strrchr(filename,'.');
    *dot = 0;
    strcat(label,filename);
    *dot = '.';
    
    segdata = size + ((size+0x1f) >> 5 );
    
    putc(0xfd,out); putc(0xfd,out);  /*magic*/
    putc(7+(char)strlen(label),out); /*symbol-struct-len low*/
    putc(0,out);                     /*symbol-struct-len high*/
    putc(size & 0xff,out);            /*data-len low*/
    putc(size>>8,out);                /*data-len high*/
    putc(segdata & 0xff,out);        /*seg-len low*/
    putc(segdata>>8,out);            /*seg-len high*/
    putc(1,out);                     /*symbols low*/
    putc(0,out);                     /*symbols high*/
    putc(strlen(label),out);
    fprintf(out,"%s",label);
    putc(0,out); putc(0,out);        /*symbol pos 0 rel to seg-start*/
    putc(7,out); putc(0,out);        /*symbol flag = rel to seg-start*/

    for (i = size>>5; i ; --i)
    {
      putc(0,out);
      for (o = 32; o ; --o )
        putc(*ptr++,out);
    }
    if ( (i = size & 0x1f) )
    {
      putc(i,out);
      for ( ; i ; --i)
        putc(*ptr++,out);
    }
    fclose(out);
  }

   if (verbose)
     printf("Written: %s \n"
            "--------------------------\n",filename);
}

void error(int line,char *f,...)
{
  va_list argp;
  
  va_start(argp,f);
  printf("Error (%d):",line);
  printf(f,va_arg(argp,char *) );
  va_end(argp);
#ifdef ATARI
  while (getc(stdin) != ' ');
#endif
  exit(-1);
}



int ReadRGB(char *filename,BYTE *palinput)
{
	int i;
    char string[256];
	char *pdest;
	int col[32];	// Thge read Lynx-GBR-palette

	FILE *stream;

	if( (stream = fopen( filename, "r" )) == NULL )
	{
		printf("Can't open pal-input file: %s\n", filename);
		return 0;
	}

	while (fgets( string, 255, stream ))
	{
		if ((pdest = strstr( string, "pal[]={" )) != NULL)
		{
			fgets( string, 255, stream );
			//printf( "%s", string);
			sscanf(string, "%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x",
			&col[ 0],&col[ 1],&col[ 2],&col[ 3],&col[ 4],&col[ 5],&col[ 6],&col[ 7],
			&col[ 8],&col[ 9],&col[10],&col[11],&col[12],&col[13],&col[14],&col[15]);
			//for (i=0; i<16; i++)
			//	printf("%x,", col[i]);
			printf("\n");


			fgets( string, 255, stream );
			//printf( "%s", string);
			sscanf(string, "%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x",
			&col[16+ 0],&col[16+ 1],&col[16+ 2],&col[16+ 3],&col[16+ 4],&col[16+ 5],&col[16+ 6],&col[16+ 7],
			&col[16+ 8],&col[16+ 9],&col[16+10],&col[16+11],&col[16+12],&col[16+13],&col[16+14],&col[16+15]);
			//for (i=0; i<16; i++)
			//	printf("%x,", col[16+i]);
			printf("\n");

		}

	}
	fclose( stream );

	for (i=0; i<16; i++)
	{
		*palinput++ = col[i];	// green
		*palinput++	= col[16+i];	// blue + red
	}
	
	
	return 1;
}
