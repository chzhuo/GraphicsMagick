/*
% Copyright (C) 2003 GraphicsMagick Group
% Copyright (C) 2002 ImageMagick Studio
% Copyright 1991-1999 E. I. du Pont de Nemours and Company
%
% This program is covered by multiple licenses, which are described in
% Copyright.txt. You should have received a copy of Copyright.txt with this
% package; otherwise see http://www.graphicsmagick.org/www/Copyright.html.
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                            TTTTT   GGGG   AAA                               %
%                              T    G      A   A                              %
%                              T    G  GG  AAAAA                              %
%                              T    G   G  A   A                              %
%                              T     GGG   A   A                              %
%                                                                             %
%                                                                             %
%                   Read/Write GraphicsMagick Image Format.                   %
%                                                                             %
%                                                                             %
%                              Software Design                                %
%                                John Cristy                                  %
%                                 July 1992                                   %
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
#include "magick/attribute.h"
#include "magick/blob.h"
#include "magick/cache.h"
#include "magick/color.h"
#include "magick/magick.h"
#include "magick/monitor.h"
#include "magick/utility.h"

/*
  Forward declarations.
*/
static unsigned int
  WriteTGAImage(const ImageInfo *,Image *);

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e a d T G A I m a g e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method ReadTGAImage reads a Truevision TGA image file and returns it.
%  It allocates the memory necessary for the new Image structure and returns
%  a pointer to the new image.
%
%  The format of the ReadTGAImage method is:
%
%      Image *ReadTGAImage(const ImageInfo *image_info,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image:  Method ReadTGAImage returns a pointer to the image after
%      reading.  A null image is returned if there is a memory shortage or
%      if the image cannot be read.
%
%    o image_info: Specifies a pointer to a ImageInfo structure.
%
%    o exception: return any errors or warnings in this structure.
%
%
*/
static Image *ReadTGAImage(const ImageInfo *image_info,ExceptionInfo *exception)
{
#define TGAColormap 1
#define TGARGB 2
#define TGAMonochrome 3
#define TGARLEColormap  9
#define TGARLERGB  10
#define TGARLEMonochrome  11

  typedef struct _TGAInfo
  {
    unsigned char
      id_length,
      colormap_type,
      image_type;

    unsigned short
      colormap_index,
      colormap_length;

    unsigned char
      colormap_size;

    unsigned short
      x_origin,
      y_origin,
      width,
      height;

    unsigned char
      bits_per_pixel,
      attributes;
  } TGAInfo;

  Image
    *image;

  IndexPacket
    index;

  long
    y;

  PixelPacket
    pixel;

  register IndexPacket
    *indexes;

  register long
    i,
    x;

  register PixelPacket
    *q;

  size_t
    count;

  TGAInfo
    tga_info;

  unsigned char
    j,
    k,
    runlength;

  unsigned int
    status;

  unsigned long
    base,
    flag,
    offset,
    real,
    skip;

  /*
    Open image file.
  */
  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  image=AllocateImage(image_info);
  status=OpenBlob(image_info,image,ReadBinaryBlobMode,exception);
  if (status == False)
    ThrowReaderException(FileOpenError,"UnableToOpenFile",image);
  /*
    Read TGA header information.
  */
  count=ReadBlob(image,1,(char *) &tga_info.id_length);
  tga_info.colormap_type=ReadBlobByte(image);
  tga_info.image_type=ReadBlobByte(image);
  do
  {
    if ((count == 0) || (tga_info.image_type == 0) ||
        (tga_info.image_type > 11))
      ThrowReaderException(CorruptImageError,"NotATGAImageFile",image);
    tga_info.colormap_index=ReadBlobLSBShort(image);
    tga_info.colormap_length=ReadBlobLSBShort(image);
    tga_info.colormap_size=ReadBlobByte(image);
    tga_info.x_origin=ReadBlobLSBShort(image);
    tga_info.y_origin=ReadBlobLSBShort(image);
    tga_info.width=ReadBlobLSBShort(image);
    tga_info.height=ReadBlobLSBShort(image);
    tga_info.bits_per_pixel=ReadBlobByte(image);
    tga_info.attributes=ReadBlobByte(image);
    /*
      Initialize image structure.
    */
    image->matte=tga_info.bits_per_pixel == 32;
    image->columns=tga_info.width;
    image->rows=tga_info.height;
    image->depth=tga_info.bits_per_pixel <= 8 ? 8 : QuantumDepth;
    if (tga_info.colormap_type != 0)
      {
        if ((tga_info.image_type == TGARLEColormap) ||
            (tga_info.image_type == TGARLERGB))
          image->storage_class=PseudoClass;
        image->colors=tga_info.colormap_length;
      }
    if (tga_info.id_length != 0)
      {
        char
          *comment;

        /*
          TGA image comment.
        */
        comment=(char *) AcquireMemory(tga_info.id_length+1);
        if (comment == (char *) NULL)
          ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed",
            image);
        (void) ReadBlob(image,tga_info.id_length,comment);
        comment[tga_info.id_length]='\0';
        (void) SetImageAttribute(image,"comment",comment);
        LiberateMemory((void **) &comment);
      }
    (void) memset(&pixel,0,sizeof(PixelPacket));
    pixel.opacity=TransparentOpacity;
    if (tga_info.colormap_type != 0)
      {
        /*
          Read TGA raster colormap.
        */
        if (!AllocateImageColormap(image,image->colors))
          ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed",
            image);
        for (i=0; i < (long) image->colors; i++)
        {
          switch (tga_info.colormap_size)
          {
            case 8:
            default:
            {
              /*
                Gray scale.
              */
              pixel.red=ScaleCharToQuantum(ReadBlobByte(image));
              pixel.green=pixel.red;
              pixel.blue=pixel.red;
              break;
            }
            case 15:
            case 16:
            {
              /*
                5 bits each of red green and blue.
              */
              j=ReadBlobByte(image);
              k=ReadBlobByte(image);
              pixel.red=(Quantum)
                (((double) MaxRGB*((int) (k & 0x7c) >> 2))/31+0.5);
              pixel.green=(Quantum) ((MaxRGB*
                (((int) (k & 0x03) << 3)+((int) (j & 0xe0) >> 5)))/31+0.5);
              pixel.blue=(Quantum)
                (((double) MaxRGB*((int) (j & 0x1f)))/31+0.5);
              break;
            }
            case 24:
            case 32:
            {
              /*
                8 bits each of blue, green and red.
              */
              pixel.blue=ScaleCharToQuantum(ReadBlobByte(image));
              pixel.green=ScaleCharToQuantum(ReadBlobByte(image));
              pixel.red=ScaleCharToQuantum(ReadBlobByte(image));
              break;
            }
          }
          image->colormap[i]=pixel;
        }
      }
    if (image_info->ping && (image_info->subrange != 0))
      if (image->scene >= (image_info->subimage+image_info->subrange-1))
        break;
    /*
      Convert TGA pixels to pixel packets.
    */
    base=0;
    flag=0;
    skip=False;
    real=0;
    index=0;
    runlength=0;
    offset=0;
    for (y=0; y < (long) image->rows; y++)
    {
      real=offset;
      if (((unsigned char) (tga_info.attributes & 0x20) >> 5) == 0)
        real=image->rows-real-1;
      q=SetImagePixels(image,0,(long) real,image->columns,1);
      if (q == (PixelPacket *) NULL)
        break;
      indexes=GetIndexes(image);
      for (x=0; x < (long) image->columns; x++)
      {
        if ((tga_info.image_type == TGARLEColormap) ||
            (tga_info.image_type == TGARLERGB) ||
            (tga_info.image_type == TGARLEMonochrome))
          {
            if (runlength != 0)
              {
                runlength--;
                skip=flag != 0;
              }
            else
              {
                count=ReadBlob(image,1,(char *) &runlength);
                if (count == 0)
                  ThrowReaderException(CorruptImageError,
                    "UnableToReadImageData",image);
                flag=runlength & 0x80;
                if (flag != 0)
                  runlength-=128;
                skip=False;
              }
          }
        if (!skip)
          switch (tga_info.bits_per_pixel)
          {
            case 8:
            default:
            {
              /*
                Gray scale.
              */
              index=ReadBlobByte(image);
              if (tga_info.colormap_type != 0)
                {
                  VerifyColormapIndex(image,index);
                  pixel=image->colormap[index];
                }
              else
                {
                  pixel.red=ScaleCharToQuantum(index);
                  pixel.green=ScaleCharToQuantum(index);
                  pixel.blue=ScaleCharToQuantum(index);
                }
              break;
            }
            case 15:
            case 16:
            {
              /*
                5 bits each of red green and blue.
              */
              j=ReadBlobByte(image);
              k=ReadBlobByte(image);
              pixel.red=(Quantum)
                ((double) (MaxRGB*((int) (k & 0x7c) >> 2))/31+0.5);
              pixel.green=(Quantum) (((double) MaxRGB*
                (((int) (k & 0x03) << 3)+((int) (j & 0xe0) >> 5)))/31+0.5);
              pixel.blue=(Quantum)
                (((double) MaxRGB*((int) (j & 0x1f)))/31+0.5);
              if (image->storage_class == PseudoClass)
                {
                  index=(IndexPacket) (((unsigned short) k << 8)+j);
                  VerifyColormapIndex(image,index);
                }
              break;
            }
            case 24:
            case 32:
            {
              /*
                8 bits each of blue green and red.
              */
              pixel.blue=ScaleCharToQuantum(ReadBlobByte(image));
              pixel.green=ScaleCharToQuantum(ReadBlobByte(image));
              pixel.red=ScaleCharToQuantum(ReadBlobByte(image));
              if (tga_info.bits_per_pixel == 32)
                pixel.opacity=ScaleCharToQuantum(MaxRGB-ReadBlobByte(image));
              break;
            }
          }
        if (status == False)
          ThrowReaderException(CorruptImageError,"UnableToReadImageData",image);
        if (image->storage_class == PseudoClass)
          indexes[x]=index;
        *q++=pixel;
      }
      if (((unsigned char) (tga_info.attributes & 0xc0) >> 6) == 4)
        offset+=4;
      else
        if (((unsigned char) (tga_info.attributes & 0xc0) >> 6) == 2)
          offset+=2;
        else
          offset++;
      if (offset >= image->rows)
        {
          base++;
          offset=base;
        }
      if (!SyncImagePixels(image))
        break;
      if (image->previous == (Image *) NULL)
        if (QuantumTick(y,image->rows))
          if (!MagickMonitor(LoadImageText,y,image->rows,exception))
            break;
    }
    if (EOFBlob(image))
      {
        ThrowException(exception,CorruptImageError,"UnexpectedEndOfFile",
          image->filename);
        break;
      }
    /*
      Proceed to next image.
    */
    if (image_info->subrange != 0)
      if (image->scene >= (image_info->subimage+image_info->subrange-1))
        break;
    count=ReadBlob(image,1,(char *) &tga_info.id_length);
    tga_info.colormap_type=ReadBlobByte(image);
    tga_info.image_type=ReadBlobByte(image);
    status=((tga_info.image_type != 0) && (tga_info.image_type <= 11));
    if (status == True)
      {
        /*
          Allocate next image structure.
        */
        AllocateNextImage(image_info,image);
        if (image->next == (Image *) NULL)
          {
            DestroyImageList(image);
            return((Image *) NULL);
          }
        image=SyncNextImageInList(image);
        if (!MagickMonitor(LoadImagesText,TellBlob(image),GetBlobSize(image),exception))
          break;
      }
  } while (status == True);
  while (image->previous != (Image *) NULL)
    image=image->previous;
  CloseBlob(image);
  return(image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e g i s t e r T G A I m a g e                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method RegisterTGAImage adds attributes for the TGA image format to
%  the list of supported formats.  The attributes include the image format
%  tag, a method to read and/or write the format, whether the format
%  supports the saving of more than one frame to the same file or blob,
%  whether the format supports native in-memory I/O, and a brief
%  description of the format.
%
%  The format of the RegisterTGAImage method is:
%
%      RegisterTGAImage(void)
%
*/
ModuleExport void RegisterTGAImage(void)
{
  MagickInfo
    *entry;

  entry=SetMagickInfo("ICB");
  entry->decoder=(DecoderHandler) ReadTGAImage;
  entry->encoder=(EncoderHandler) WriteTGAImage;
  entry->description=AcquireString("Truevision Targa image");
  entry->module=AcquireString("TGA");
  (void) RegisterMagickInfo(entry);
  entry=SetMagickInfo("TGA");
  entry->decoder=(DecoderHandler) ReadTGAImage;
  entry->encoder=(EncoderHandler) WriteTGAImage;
  entry->description=AcquireString("Truevision Targa image");
  entry->module=AcquireString("TGA");
  (void) RegisterMagickInfo(entry);
  entry=SetMagickInfo("VDA");
  entry->decoder=(DecoderHandler) ReadTGAImage;
  entry->encoder=(EncoderHandler) WriteTGAImage;
  entry->description=AcquireString("Truevision Targa image");
  entry->module=AcquireString("TGA");
  (void) RegisterMagickInfo(entry);
  entry=SetMagickInfo("VST");
  entry->decoder=(DecoderHandler) ReadTGAImage;
  entry->encoder=(EncoderHandler) WriteTGAImage;
  entry->description=AcquireString("Truevision Targa image");
  entry->module=AcquireString("TGA");
  (void) RegisterMagickInfo(entry);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   U n r e g i s t e r T G A I m a g e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method UnregisterTGAImage removes format registrations made by the
%  TGA module from the list of supported formats.
%
%  The format of the UnregisterTGAImage method is:
%
%      UnregisterTGAImage(void)
%
*/
ModuleExport void UnregisterTGAImage(void)
{
  (void) UnregisterMagickInfo("ICB");
  (void) UnregisterMagickInfo("TGA");
  (void) UnregisterMagickInfo("VDA");
  (void) UnregisterMagickInfo("VST");
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   W r i t e T G A I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method WriteTGAImage writes a image in the Truevision Targa rasterfile
%  format.
%
%  The format of the WriteTGAImage method is:
%
%      unsigned int WriteTGAImage(const ImageInfo *image_info,Image *image)
%
%  A description of each parameter follows.
%
%    o status: Method WriteTGAImage return True if the image is written.
%      False is returned is there is a memory shortage or if the image file
%      fails to write.
%
%    o image_info: Specifies a pointer to a ImageInfo structure.
%
%    o image:  A pointer to an Image structure.
%
%
*/
static unsigned int WriteTGAImage(const ImageInfo *image_info,Image *image)
{
#define TargaColormap 1
#define TargaRGB 2
#define TargaMonochrome 3
#define TargaRLEColormap  9
#define TargaRLERGB  10
#define TargaRLEMonochrome  11

  typedef struct _TargaInfo
  {
    unsigned char
      id_length,
      colormap_type,
      image_type;

    unsigned short
      colormap_index,
      colormap_length;

    unsigned char
      colormap_size;

    unsigned short
      x_origin,
      y_origin,
      width,
      height;

    unsigned char
      bits_per_pixel,
      attributes;
  } TargaInfo;

  const ImageAttribute
    *attribute;

  long
    count,
    y;

  register const PixelPacket
    *p;

  register IndexPacket
    *indexes;

  register long
    x;

  register long
    i;

  register unsigned char
    *q;

  TargaInfo
    targa_info;

  unsigned char
    *targa_pixels;

  unsigned int
    status;

  unsigned long
    scene;

  /*
    Open output image file.
  */
  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  status=OpenBlob(image_info,image,WriteBinaryBlobMode,&image->exception);
  if (status == False)
    ThrowWriterException(FileOpenError,"UnableToOpenFile",image);
  scene=0;
  do
  {
    /*
      Initialize TGA raster file header.
    */
    TransformColorspace(image,RGBColorspace);
    targa_info.id_length=0;
    attribute=GetImageAttribute(image,"comment");
    if (attribute != (const ImageAttribute *) NULL)
      targa_info.id_length=Min(strlen(attribute->value),255);
    targa_info.colormap_type=0;
    targa_info.colormap_index=0;
    targa_info.colormap_length=0;
    targa_info.colormap_size=0;
    targa_info.x_origin=0;
    targa_info.y_origin=0;
    targa_info.width=(unsigned short) image->columns;
    targa_info.height=(unsigned short) image->rows;
    targa_info.bits_per_pixel=8;
    targa_info.attributes=0;
    if ((image->storage_class == DirectClass) || (image->colors > 256))
      {
        /*
          Full color TGA raster.
        */
        targa_info.image_type=TargaRGB;
        targa_info.bits_per_pixel=24;
        if (image->matte)
          {
            targa_info.bits_per_pixel=32;
            targa_info.attributes=8;  /* # of alpha bits */
          }
      }
    else
      {
        /*
          Colormapped TGA raster.
        */
        targa_info.image_type=TargaColormap;
        targa_info.colormap_type=1;
        targa_info.colormap_index=0;
        targa_info.colormap_length=(unsigned short) image->colors;
        targa_info.colormap_size=24;
      }
    /*
      Write TGA header.
    */
    (void) WriteBlobByte(image,targa_info.id_length);
    (void) WriteBlobByte(image,targa_info.colormap_type);
    (void) WriteBlobByte(image,targa_info.image_type);
    (void) WriteBlobLSBShort(image,targa_info.colormap_index);
    (void) WriteBlobLSBShort(image,targa_info.colormap_length);
    (void) WriteBlobByte(image,targa_info.colormap_size);
    (void) WriteBlobLSBShort(image,targa_info.x_origin);
    (void) WriteBlobLSBShort(image,targa_info.y_origin);
    (void) WriteBlobLSBShort(image,targa_info.width);
    (void) WriteBlobLSBShort(image,targa_info.height);
    (void) WriteBlobByte(image,targa_info.bits_per_pixel);
    (void) WriteBlobByte(image,targa_info.attributes);
    if (targa_info.id_length != 0)
      (void) WriteBlob(image,targa_info.id_length,attribute->value);
    if (targa_info.image_type == TargaColormap)
      {
        unsigned char
          *targa_colormap;

        /*
          Dump colormap to file (blue, green, red byte order).
        */
        targa_colormap=(unsigned char *)
          AcquireMemory(3*targa_info.colormap_length);
        if (targa_colormap == (unsigned char *) NULL)
          ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed",
            image);
        q=targa_colormap;
        for (i=0; i < (long) image->colors; i++)
        {
          *q++=ScaleQuantumToChar(image->colormap[i].blue);
          *q++=ScaleQuantumToChar(image->colormap[i].green);
          *q++=ScaleQuantumToChar(image->colormap[i].red);
        }
        (void) WriteBlob(image,3*targa_info.colormap_length,
          (char *) targa_colormap);
        LiberateMemory((void **) &targa_colormap);
      }
    /*
      Convert MIFF to TGA raster pixels.
    */
    count=(long) ((targa_info.bits_per_pixel*targa_info.width) >> 3);
    targa_pixels=(unsigned char *) AcquireMemory(count);
    if (targa_pixels == (unsigned char *) NULL)
      ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed",image);
    for (y=(long) (image->rows-1); y >= 0; y--)
    {
      p=AcquireImagePixels(image,0,y,image->columns,1,&image->exception);
      if (p == (const PixelPacket *) NULL)
        break;
      q=targa_pixels;
      indexes=GetIndexes(image);
      for (x=0; x < (long) image->columns; x++)
      {
        if (targa_info.image_type == TargaColormap)
          *q++=indexes[x];
        else
          {
            *q++=ScaleQuantumToChar(p->blue);
            *q++=ScaleQuantumToChar(p->green);
            *q++=ScaleQuantumToChar(p->red);
            if (image->colorspace == CMYKColorspace)
              *q++=ScaleQuantumToChar(p->opacity);
            else
              if (image->matte)
                *q++=ScaleQuantumToChar(MaxRGB-p->opacity);
          }
        p++;
      }
      (void) WriteBlob(image,q-targa_pixels,(char *) targa_pixels);
      if (image->previous == (Image *) NULL)
        if (QuantumTick(y,image->rows))
          if (!MagickMonitor(SaveImageText,y,image->rows,&image->exception))
            break;
    }
    LiberateMemory((void **) &targa_pixels);
    if (image->next == (Image *) NULL)
      break;
    image=SyncNextImageInList(image);
    if (!MagickMonitor(SaveImagesText,scene++,GetImageListLength(image),&image->exception))
      break;
  } while (image_info->adjoin);
  if (image_info->adjoin)
    while (image->previous != (Image *) NULL)
      image=image->previous;
  CloseBlob(image);
  return(True);
}
