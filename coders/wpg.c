/*
% Copyright (C) 2003 GraphicsMagick Group
% Copyright (C) 2002 ImageMagick Studio
%
% This program is covered by multiple licenses, which are described in
% Copyright.txt. You should have received a copy of Copyright.txt with this
% package; otherwise see http://www.graphicsmagick.org/www/Copyright.html.
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                            W   W  PPPP    GGGG                              %
%                            W   W  P   P  G                                  %
%                            W W W  PPPP   G GGG                              %
%                            WW WW  P      G   G                              %
%                            W   W  P       GGG                               %
%                                                                             %
%                                                                             %
%                    Read/Write ImageMagick Image Format.                     %
%                                                                             %
%                                                                             %
%                              Software Design                                %
%                              Jaroslav Fojtik                                %
%                                 June 2000                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%
*/

/*
  Include declarations.
*/
#include "magick/studio.h"
#include "magick/blob.h"
#include "magick/cache.h"
#include "magick/color.h"
#include "magick/constitute.h"
#include "magick/magick.h"
#include "magick/tempfile.h"
#include "magick/utility.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I s W P G                                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsWPG() returns True if the image format type, identified by the magick
%  string, is WPG.
%
%  The format of the IsWPG method is:
%
%      unsigned int IsWPG(const unsigned char *magick,const size_t length)
%
%  A description of each parameter follows:
%
%    o status:  Method IsWPG returns True if the image format type is WPG.
%
%    o magick: This string is generally the first few bytes of an image file
%      or blob.
%
%    o length: Specifies the length of the magick string.
%
%
*/
static unsigned int IsWPG(const unsigned char *magick,const size_t length)
{
  if (length < 4)
    return(False);
  if (memcmp(magick,"\377WPC",4) == 0)
    return(True);
  return(False);
}


static void Rd_WP_DWORD(Image *image,unsigned long *d)
{
  unsigned char
    b;

  b=ReadBlobByte(image);
  *d=b;
  if (b < 0xFF)
    return;
  b=ReadBlobByte(image);
  *d=(unsigned long) b;
  b=ReadBlobByte(image);
  *d+=(unsigned long) b*256l;
  if (*d < 0x8000)
    return;
  *d=(*d & 0x7FFF) << 16;
  b=ReadBlobByte(image);
  *d+=(unsigned long) b;
  b=ReadBlobByte(image);
  *d+=(unsigned long) b*256l;
  return;
}

static void InsertRow(unsigned char *p,long y,Image *image, int bpp)
{
  int
    bit;

  long
    x;

  register PixelPacket
    *q;

  IndexPacket
    index;

  register IndexPacket
    *indexes;

  switch (bpp)
    {
    case 1:  /* Convert bitmap scanline. */
      {
        q=SetImagePixels(image,0,y,image->columns,1);
        if (q == (PixelPacket *) NULL)
          break;
        indexes=GetIndexes(image);
        for (x=0; x < ((long) image->columns-7); x+=8)
          {
            for (bit=0; bit < 8; bit++)
              {
                index=((*p) & (0x80 >> bit) ? 0x01 : 0x00);
                indexes[x+bit]=index;
                *q++=image->colormap[index];
              }
            p++;
          }
        if ((image->columns % 8) != 0)
          {
            for (bit=0; bit < (long) (image->columns % 8); bit++)
              {
                index=((*p) & (0x80 >> bit) ? 0x01 : 0x00);
                indexes[x+bit]=index;
                *q++=image->colormap[index];
              }
            p++;
          }
        if (!SyncImagePixels(image))
          break;
        break;
      }
    case 2:  /* Convert PseudoColor scanline. */
      {
        q=SetImagePixels(image,0,y,image->columns,1);
        if (q == (PixelPacket *) NULL)
          break;
        indexes=GetIndexes(image);
        for (x=0; x < ((long) image->columns-1); x+=2)
          {
            index=(IndexPacket) ((*p >> 6) & 0x3);
            VerifyColormapIndex(image,index);
            indexes[x]=index;
            *q++=image->colormap[index];
            index=(IndexPacket) ((*p >> 4) & 0x3);
            VerifyColormapIndex(image,index);
            indexes[x]=index;
            *q++=image->colormap[index];
            index=(IndexPacket) ((*p >> 2) & 0x3);
            VerifyColormapIndex(image,index);
            indexes[x]=index;
            *q++=image->colormap[index];
            index=(IndexPacket) ((*p) & 0x3);
            VerifyColormapIndex(image,index);
            indexes[x+1]=index;
            *q++=image->colormap[index];
            p++;
          }
        if ((image->columns % 4) != 0)
          {
            index=(IndexPacket) ((*p >> 6) & 0x3);
            VerifyColormapIndex(image,index);
            indexes[x]=index;
            *q++=image->colormap[index];
            if ((image->columns % 4) >= 1)

              {
                index=(IndexPacket) ((*p >> 4) & 0x3);
                VerifyColormapIndex(image,index);
                indexes[x]=index;
                *q++=image->colormap[index];
                if ((image->columns % 4) >= 2)

                  {
                    index=(IndexPacket) ((*p >> 2) & 0x3);
                    VerifyColormapIndex(image,index);
                    indexes[x]=index;
                    *q++=image->colormap[index];
                  }
              }
            p++;
          }
        if (!SyncImagePixels(image))
          break;
        break;
      }
      
    case 4:  /* Convert PseudoColor scanline. */
      {
        q=SetImagePixels(image,0,y,image->columns,1);
        if (q == (PixelPacket *) NULL)
          break;
        indexes=GetIndexes(image);
        for (x=0; x < ((long) image->columns-1); x+=2)
          {
            index=(IndexPacket) ((*p >> 4) & 0xf);
            VerifyColormapIndex(image,index);
            indexes[x]=index;
            *q++=image->colormap[index];
            index=(IndexPacket) ((*p) & 0xf);
            VerifyColormapIndex(image,index);
            indexes[x+1]=index;
            *q++=image->colormap[index];
            p++;
          }
        if ((image->columns % 2) != 0)
          {
            index=(IndexPacket) ((*p >> 4) & 0xf);
            VerifyColormapIndex(image,index);
            indexes[x]=index;
            *q++=image->colormap[index];
            p++;
          }
        if (!SyncImagePixels(image))
          break;
        break;
      }
    case 8: /* Convert PseudoColor scanline. */
      {
        q=SetImagePixels(image,0,y,image->columns,1);
        if (q == (PixelPacket *) NULL) break;
        indexes=GetIndexes(image);

        for (x=0; x < (long) image->columns; x++)
          {
            index=(IndexPacket) (*p++);
            VerifyColormapIndex(image,index);
            indexes[x]=index;
            *q++=image->colormap[index];
          }
        if (!SyncImagePixels(image))
          break;
      }
      break;
     
    case 24:     /*  Convert DirectColor scanline.  */
      q=SetImagePixels(image,0,y,image->columns,1);
      if (q == (PixelPacket *) NULL)
        break;
      for (x=0; x < (long) image->columns; x++)
        {
          q->red=ScaleCharToQuantum(*p++);
          q->green=ScaleCharToQuantum(*p++);
          q->blue=ScaleCharToQuantum(*p++);
          q++;
        }
      if (!SyncImagePixels(image))
        break;
      break;     
    }
}

#define InsertByte(b) \
{ \
  BImgBuff[x]=b; \
  x++; \
  if((long) x>=ldblk) \
  { \
    InsertRow(BImgBuff,(long) y,image,bpp); \
    x=0; \
    y++; \
    } \
}
static int UnpackWPGRaster(Image *image,int bpp)
{
  int
    x,
    y,
    i;

  unsigned char
    bbuf,
    *BImgBuff,
    RunCount;

  long
    ldblk;

  x=0;
  y=0;

  ldblk=(long) ((bpp*image->columns+7)/8);
  BImgBuff=(unsigned char *) AcquireMemory(ldblk);
  if(BImgBuff==NULL) return(-2);

  while(y<(long) image->rows)
    {
      bbuf=ReadBlobByte(image);

      /*
        if not readed byte ??????
        {
          delete Raster;
          Raster=NULL;
          return(-2);
        }
      */

      RunCount=bbuf & 0x7F;
      if(bbuf & 0x80)
        {
          if(RunCount)  /* repeat next byte runcount * */
            {
              bbuf=ReadBlobByte(image);
              for(i=0;i<(int) RunCount;i++) InsertByte(bbuf);
            }
          else {  /* read next byte as RunCount; repeat 0xFF runcount* */
            RunCount=ReadBlobByte(image);
            for(i=0;i<(int) RunCount;i++) InsertByte(0xFF);
          }
        }
      else {
        if(RunCount)   /* next runcount byte are readed directly */
          {
            for(i=0;i < (int) RunCount;i++)
              {
                bbuf=ReadBlobByte(image);
                InsertByte(bbuf);
              }
          }
        else {  /* repeat previous line runcount* */
          RunCount=ReadBlobByte(image);
          if(x) {    /* attempt to duplicate row from x position: */
            /* I do not know what to do here */
            LiberateMemory((void **) &BImgBuff);
            return(-3);
          }
          for(i=0;i < (int) RunCount;i++)
            {
              x=0;
              y++;    /* Here I need to duplicate previous row RUNCOUNT* */
              if(y<2) continue;
              if(y>(long) image->rows)
                {
                  LiberateMemory((void **) &BImgBuff);
                  return(-4);
                }
              InsertRow(BImgBuff,y-1,image,bpp);
            }
        }
      }
    }
  LiberateMemory((void **) &BImgBuff);
  return(0);
}

#define InsertRByte(b) \
{ \
  BImgBuff[x]=b; \
  x++; \
  if((long) x >= ldblk) \
  { \
    InsertRow(BImgBuff,(long) (image->rows-y-1),image,bpp); \
    x=0; \
    y++; \
   } \
}
static int UnpackWPG2Raster(Image *image,int bpp)
{
  char
    SampleSize=1;

  unsigned char
    bbuf,
    *BImgBuff,
    RunCount,
    SampleBuffer[8];

  int
    x,
    y,
    i;

  long
    ldblk;


  x=0;
  y=0;
  ldblk=(long) ((bpp*image->columns+7)/8);
  BImgBuff=(unsigned char *) AcquireMemory(ldblk);
  if(BImgBuff==NULL)
    return(-2);

  while(y<(long) image->rows)
    {
      bbuf=ReadBlobByte(image);

      switch(bbuf)
        {
        case 0x7D:
          SampleSize=ReadBlobByte(image);  /* DSZ */
          if(SampleSize>8) return(-2);
          if(SampleSize<1) return(-2);
          break;
        case 0x7E:
          fprintf(stderr,"\nUnsupported WPG token XOR, please report!");  
          break;
        case 0x7F:
          RunCount=ReadBlobByte(image);   /* BLK */
          for(i=0;i<(int) SampleSize*((int)RunCount+1);i++)
            {
              InsertRByte(0);
            }
          break;
        case 0xFD:
          fprintf(stderr,"\nUnsupported WPG2 token EXT, please report!");  
          break;
        case 0xFE:
          RunCount=ReadBlobByte(image);  /* RST */
          if(x!=0)
            {
              fprintf(stderr,
                "\nUnsupported WPG2 unaligned token RST x=%d, please report!\n"
                  ,x);
              return(-3);
            }
          {
            /* duplicate the previous row RunCount x */
            for(i=0;i<=RunCount;i++)
              {      
                InsertRow(BImgBuff,(long) (image->rows>y?image->rows-y-1:0),
                  image,bpp);
                y++;
              }    
          }
          break;
        case 0xFF:
          RunCount=ReadBlobByte(image);          /* WHT */
          for(i=0;i<(int) SampleSize*((int)RunCount+1);i++)
            {
              InsertRByte(0xFF);
            }
          break;
        default:
          RunCount=bbuf & 0x7F;

          if(bbuf & 0x80)
            {  
              for(i=0;i<(int) SampleSize;i++)
                SampleBuffer[i]=ReadBlobByte(image);
              for(i=0;i<=(int)RunCount;i++)
                for(bbuf=0;bbuf<SampleSize;bbuf++)
                  InsertRByte(SampleBuffer[bbuf]);
            }
          else {
            for(i=0;i<(int) SampleSize*((int)RunCount+1);i++)
              {
                bbuf=ReadBlobByte(image);
                InsertRByte(bbuf);
              }
          }
        }

    }
  LiberateMemory((void **) &BImgBuff);
  return(0);
}


static Image *ExtractPostscript(Image *image,const ImageInfo *image_info,
  ExtendedSignedIntegralType PS_Offset,long PS_Size)
{
  FILE
    *f;

  ImageInfo
    *clone_info;

  ExceptionInfo
    exception;

  Image
    *image2;

  if ((clone_info=CloneImageInfo(image_info)) == NULL)
    return(image);
  clone_info->blob=(void *) NULL;
  clone_info->length=0;
  if( (f=AcquireTemporaryFileStream(clone_info->filename,BinaryFileIOMode))
      == NULL)
    goto FINISH;
  (void) SeekBlob(image,PS_Offset,SEEK_SET);
  while(PS_Size-- > 0)
    {
      (void) fputc(ReadBlobByte(image),f);
    }
  (void) fclose(f);    

  if((clone_info->file=fopen(clone_info->filename,"r"))==NULL)
    goto FINISH_UNL;
  image2=ReadImage(clone_info,&exception);

  if(image2!=NULL)   /* Allocate next image structure. */
    {
      while(image->previous!=NULL)
        image=image->previous;
       
      image->previous=image2;
      image2->next=image;
    
      while(image->next!=NULL)
        image=SyncNextImageInList(image);
    }
    
 FINISH_UNL:    
  LiberateTemporaryFile(clone_info->filename);
 FINISH:
  DestroyImageInfo(clone_info);
  return(image);
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e a d W P G I m a g e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method ReadWPGImage reads an WPG X image file and returns it.  It
%  allocates the memory necessary for the new Image structure and returns a
%  pointer to the new image.
%
%  The format of the ReadWPGImage method is:
%
%    Image *ReadWPGImage(const ImageInfo *image_info,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image:  Method ReadWPGImage returns a pointer to the image after
%      reading. A null image is returned if there is a memory shortage or if
%      the image cannot be read.
%
%    o image_info: Specifies a pointer to a ImageInfo structure.
%
%    o exception: return any errors or warnings in this structure.
%
%
*/
static Image *ReadWPGImage(const ImageInfo *image_info,
  ExceptionInfo *exception)
{
  typedef struct
  {
    unsigned long FileId;
    ExtendedSignedIntegralType DataOffset;
    unsigned int ProductType;
    unsigned int FileType;
    unsigned char MajorVersion;
    unsigned char MinorVersion;
    unsigned int EncryptKey;
    unsigned int Reserved;
  }WPGHeader;

  typedef struct
  {
    unsigned char  RecType;
    unsigned long   RecordLength;
  }WPGRecord;

  typedef struct
  {
    unsigned char  Class;
    unsigned char    RecType;
    unsigned long   Extension;
    unsigned long   RecordLength;
  }WPG2Record;

  typedef struct
  {
    unsigned int Width;
    unsigned int Heigth;
    unsigned int Depth;
    unsigned int HorzRes;
    unsigned int VertRes;
  }WPGBitmapType1;

  typedef struct
  {
    unsigned int Width;
    unsigned int Heigth;
    unsigned char Depth;
    unsigned char Compression;
  }WPG2BitmapType1;

  typedef struct
  {
    unsigned int RotAngle;
    unsigned int LowLeftX;
    unsigned int LowLeftY;
    unsigned int UpRightX;
    unsigned int UpRightY;
    unsigned int Width;
    unsigned int Heigth;
    unsigned int Depth;
    unsigned int HorzRes;
    unsigned int VertRes;
  }WPGBitmapType2;

  typedef struct
  {
    unsigned int StartIndex;
    unsigned int NumOfEntries;
  }WPGColorMapRec;

  typedef struct {
    unsigned long PS_unknown1;
    unsigned int PS_unknown2;
    unsigned int PS_unknown3;
  } WPGPSl1Record;  

  Image
    *image;

  unsigned int
    status;

  WPGHeader
    Header;

  WPGRecord
    Rec;

  WPG2Record
    Rec2;

  WPGBitmapType1
    BitmapHeader1;

  WPG2BitmapType1
    Bitmap2Header1;

  WPGBitmapType2
    BitmapHeader2;

  WPGColorMapRec
    WPG_Palette;

  int
    i,
    bpp;

  long
    ldblk;

  unsigned char
    *BImgBuff;

  /*
    Open image file.
  */
  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  image=AllocateImage(image_info);
  image->depth=8;
  status=OpenBlob(image_info,image,ReadBinaryBlobMode,exception);
  if (status == False)
    ThrowReaderException(FileOpenError,"UnableToOpenFile",image);
  /*
    Read WPG image.
  */
  Header.FileId=ReadBlobLSBLong(image);
  Header.DataOffset=(ExtendedSignedIntegralType) ReadBlobLSBLong(image);
  Header.ProductType=ReadBlobLSBShort(image);
  Header.FileType=ReadBlobLSBShort(image);
  Header.MajorVersion=ReadBlobByte(image);
  Header.MinorVersion=ReadBlobByte(image);
  Header.EncryptKey=ReadBlobLSBShort(image);
  Header.Reserved=ReadBlobLSBShort(image);

  if (Header.FileId!=0x435057FF || (Header.ProductType>>8)!=0x16)
    ThrowReaderException(CorruptImageError,"NotAWPGImageFile",image);
  if (Header.EncryptKey!=0)
    ThrowReaderException(CoderError,"EncryptedWPGImageFileNotSupported",image);

  image->colors = 0;
  bpp=0;

  switch(Header.FileType)
    {
    case 1:     /*WPG level 1*/
      while(!EOFBlob(image)) /* object parser loop */
        {
          (void) SeekBlob(image,Header.DataOffset,SEEK_SET);
          if(EOFBlob(image))
            break;

          Rec.RecType=(i=ReadBlobByte(image));
          if(i==EOF)
            break;
          Rd_WP_DWORD(image,&Rec.RecordLength);
          if(EOFBlob(image))
            break;

          Header.DataOffset=TellBlob(image)+Rec.RecordLength;

          switch(Rec.RecType)
            {
            case 0x0B: /* bitmap type 1 */
              BitmapHeader1.Width=ReadBlobLSBShort(image);
              BitmapHeader1.Heigth=ReadBlobLSBShort(image);
              BitmapHeader1.Depth=ReadBlobLSBShort(image);
              BitmapHeader1.HorzRes=ReadBlobLSBShort(image);
              BitmapHeader1.VertRes=ReadBlobLSBShort(image);

              if(BitmapHeader1.HorzRes && BitmapHeader1.VertRes)
                {
                  image->units=PixelsPerCentimeterResolution;
                  image->x_resolution=BitmapHeader1.HorzRes/470.0;
                  image->y_resolution=BitmapHeader1.VertRes/470.0;
                }
              image->columns=BitmapHeader1.Width;
              image->rows=BitmapHeader1.Heigth;
              bpp=BitmapHeader1.Depth;

              goto UnpackRaster;

            case 0x0E:  /*Color palette */
              WPG_Palette.StartIndex=ReadBlobLSBShort(image);
              WPG_Palette.NumOfEntries=ReadBlobLSBShort(image);

              image->colors=WPG_Palette.NumOfEntries;
              if (!AllocateImageColormap(image,image->colors))
                goto NoMemory;
              for (i=WPG_Palette.StartIndex;
                   i < (int)WPG_Palette.NumOfEntries; i++)
                {
                  image->colormap[i].red=
                    ScaleCharToQuantum(ReadBlobByte(image));
                  image->colormap[i].green=
                    ScaleCharToQuantum(ReadBlobByte(image));
                  image->colormap[i].blue=
                    ScaleCharToQuantum(ReadBlobByte(image));
                }
              break;
     
            case 0x11:  /* Start PS l1 */
              if(Rec.RecordLength>8)
                image=ExtractPostscript(image,image_info,
                        TellBlob(image)+8,   /*skip PS header in the wpg*/
                        (long) Rec.RecordLength-8);
              break;     

            case 0x14:  /* bitmap type 2 */
              BitmapHeader2.RotAngle=ReadBlobLSBShort(image);
              BitmapHeader2.LowLeftX=ReadBlobLSBShort(image);
              BitmapHeader2.LowLeftY=ReadBlobLSBShort(image);
              BitmapHeader2.UpRightX=ReadBlobLSBShort(image);
              BitmapHeader2.UpRightY=ReadBlobLSBShort(image);
              BitmapHeader2.Width=ReadBlobLSBShort(image);
              BitmapHeader2.Heigth=ReadBlobLSBShort(image);
              BitmapHeader2.Depth=ReadBlobLSBShort(image);
              BitmapHeader2.HorzRes=ReadBlobLSBShort(image);
              BitmapHeader2.VertRes=ReadBlobLSBShort(image);

              image->units=PixelsPerCentimeterResolution;
              image->page.width=(unsigned int)
                ((BitmapHeader2.LowLeftX-BitmapHeader2.UpRightX)/470.0);
              image->page.height=(unsigned int)
                ((BitmapHeader2.LowLeftX-BitmapHeader2.UpRightY)/470.0);
              image->page.x=(int) (BitmapHeader2.LowLeftX/470.0);
              image->page.y=(int) (BitmapHeader2.LowLeftX/470.0);
              if(BitmapHeader2.HorzRes && BitmapHeader2.VertRes)
                {
                  image->x_resolution=BitmapHeader2.HorzRes/470.0;
                  image->y_resolution=BitmapHeader2.VertRes/470.0;
                }
              image->columns=BitmapHeader2.Width;
              image->rows=BitmapHeader2.Heigth;
              bpp=BitmapHeader2.Depth;

            UnpackRaster:
              if (image->colors == 0 && bpp!=24)
                {
                  image->colors=1 << bpp;
                  if (!AllocateImageColormap(image,image->colors))
                    {
                    NoMemory:
                      ThrowReaderException(ResourceLimitError,
                        "MemoryAllocationFailed",
                         image)
                    }
                }
              else {
                if(bpp<24)
                  if( image->colors<(1UL<<bpp) && bpp!=24 )
                    ReacquireMemory((void **) &image->colormap,
                      (1<<bpp)*sizeof(PixelPacket));
              }
          
              if(bpp==1)
                {
                  if(image->colormap[0].red==0 &&
                     image->colormap[0].green==0 &&
                     image->colormap[0].blue==0 &&
                     image->colormap[1].red==0 &&
                     image->colormap[1].green==0 &&
                     image->colormap[1].blue==0)
                    {  /*fix crippled monochrome palette*/
                      image->colormap[1].red =
                        image->colormap[1].green =
                        image->colormap[1].blue = MaxRGB;
                    }
                }      

              if(UnpackWPGRaster(image,bpp) < 0)
                /* The raster cannot be unpacked */
                {
                DecompressionFailed:
                  ThrowReaderException(CoderError,"UnableToDecompressImage",
                     image)
                }

              /* Allocate next image structure. */
              AllocateNextImage(image_info,image);
              image->depth=8;
              if (image->next == (Image *) NULL)
                goto Finish;
              image=SyncNextImageInList(image);
              image->columns=image->rows=0;
              image->colors=0;
              break;

            case 0x1B:  /*Postscript l2*/
              if(Rec.RecordLength>0x3C)
                image=ExtractPostscript(image,image_info,
                    TellBlob(image)+0x3C,   /*skip PS l2 header in the wpg*/
                    (long) Rec.RecordLength-0x3C);
              break;
            }
        }
      break;

    case 2:  /*WPG level 2*/
      while(!EOFBlob(image)) /* object parser loop */

        {
          (void) SeekBlob(image,Header.DataOffset,SEEK_SET);
          if(EOFBlob(image))
            break;

          Rec2.Class=(i=ReadBlobByte(image));
          if(i==EOF)
            break;
          Rec2.RecType=(i=ReadBlobByte(image));
          if(i==EOF)
            break;
          Rd_WP_DWORD(image,&Rec2.Extension);
          Rd_WP_DWORD(image,&Rec2.RecordLength);
          if(EOFBlob(image))
            break;

          Header.DataOffset=TellBlob(image)+Rec2.RecordLength;

          switch(Rec2.RecType)
            {
            case 0x0C:    /*Color palette */
              WPG_Palette.StartIndex=ReadBlobLSBShort(image);
              WPG_Palette.NumOfEntries=ReadBlobLSBShort(image);

              image->colors=WPG_Palette.NumOfEntries;
              if (!AllocateImageColormap(image,image->colors))
                ThrowReaderException(ResourceLimitError,
                  "MemoryAllocationFailed",image);
              for (i=WPG_Palette.StartIndex;
                   i < (int)WPG_Palette.NumOfEntries; i++)
                {
                  image->colormap[i].red=
                    ScaleCharToQuantum(ReadBlobByte(image));
                  image->colormap[i].green=
                    ScaleCharToQuantum(ReadBlobByte(image));
                  image->colormap[i].blue=
                    ScaleCharToQuantum(ReadBlobByte(image));
                  (void) ReadBlobByte(image);   /*Opacity??*/
                }
              break;
            case 0x0E:
              Bitmap2Header1.Width=ReadBlobLSBShort(image);
              Bitmap2Header1.Heigth=ReadBlobLSBShort(image);
              Bitmap2Header1.Depth=ReadBlobByte(image);
              Bitmap2Header1.Compression=ReadBlobByte(image);

              if(Bitmap2Header1.Compression>1)
                continue; /*Unknown compression method */
              switch(Bitmap2Header1.Depth)
                {
                case 1:
                  bpp=1;
                  break;
                case 2:
                  bpp=2;
                  break;
                case 3:
                  bpp=4;
                  break;
                case 4:
                  bpp=8;
                  break;
                case 8:
                  bpp=24;
                  break;
                default:
                  continue;  /*Ignore raster with unknown depth*/
                }
              image->columns=Bitmap2Header1.Width;
              image->rows=Bitmap2Header1.Heigth;  

              if (image->colors == 0 && bpp!=24)
                {
                  image->colors=1 << bpp;
                  if (!AllocateImageColormap(image,image->colors))
                    goto NoMemory;
                }
              else {
                if(bpp<24)
                  if( image->colors<(1UL<<bpp) && bpp!=24 )
                    ReacquireMemory((void **) &image->colormap,
                      (1<<bpp)*sizeof(PixelPacket));
              }


              switch(Bitmap2Header1.Compression)
                {
                case 0:    /*Uncompressed raster*/
                  {
                    ldblk=(long) ((bpp*image->columns+7)/8);
                    if((BImgBuff=(unsigned char *)
                        AcquireMemory(ldblk))==NULL)
                      goto NoMemory;

                    for(i=0;i<(long) image->rows;i++)
                      {
                        (void) ReadBlob(image,ldblk,(char *)BImgBuff);
                        InsertRow(BImgBuff,i,image,bpp);
                      }
                    if(BImgBuff)
                      LiberateMemory((void **) &BImgBuff);
                    break;
                  }
                case 1:    /*RLE for WPG2 */
                  {
                    if( UnpackWPG2Raster(image,bpp) < 0)
                      goto DecompressionFailed;
                    break;
                  }   
                }

              /* Allocate next image structure. */
              AllocateNextImage(image_info,image);
              image->depth=8;
              if (image->next == (Image *) NULL)
                goto Finish;
              image=SyncNextImageInList(image);
              image->columns=image->rows=0;
              image->colors=0;
              break;

            case 0x12:  /* Postscript WPG2*/
              if(Rec2.RecordLength>0x12)
                image=ExtractPostscript(image,image_info,
                  TellBlob(image)+0x12,   /*skip PS header in the wpg2*/
                  (long) (Rec2.RecordLength-0x12));
              break;
            }
        }

      break;

    default:
      ThrowReaderException(CoderError,"Unsupportedlevel",image)
  }

 Finish:
  CloseBlob(image);

  /*
    Rewind list, removing any empty images while rewinding.
  */
  while ((image != (Image *) NULL) && (image->previous != (Image *) NULL))
    {
      if ((image->rows == 0) || (image->columns == 0))
        DeleteImageFromList(&image);
      image=image->previous;
    }
  return(image);       
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e g i s t e r W P G I m a g e                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method RegisterWPGImage adds attributes for the WPG image format to
%  the list of supported formats.  The attributes include the image format
%  tag, a method to read and/or write the format, whether the format
%  supports the saving of more than one frame to the same file or blob,
%  whether the format supports native in-memory I/O, and a brief
%  description of the format.
%
%  The format of the RegisterWPGImage method is:
%
%      RegisterWPGImage(void)
%
*/
ModuleExport void RegisterWPGImage(void)
{
  MagickInfo
    *entry;

  entry=SetMagickInfo("WPG");
  entry->decoder=(DecoderHandler) ReadWPGImage;
  entry->magick=(MagickHandler) IsWPG;
  entry->description=AcquireString("Word Perfect Graphics");
  entry->module=AcquireString("WPG");
  entry->seekable_stream=True;
  (void) RegisterMagickInfo(entry);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   U n r e g i s t e r W P G I m a g e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method UnregisterWPGImage removes format registrations made by the
%  WPG module from the list of supported formats.
%
%  The format of the UnregisterWPGImage method is:
%
%      UnregisterWPGImage(void)
%
*/
ModuleExport void UnregisterWPGImage(void)
{
  (void) UnregisterMagickInfo("WPG");
}
