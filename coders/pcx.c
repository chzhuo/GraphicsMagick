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
%                            PPPP    CCCC  X   X                              %
%                            P   P  C       X X                               %
%                            PPPP   C        X                                %
%                            P      C       X X                               %
%                            P       CCCC  X   X                              %
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
#include "magick/blob.h"
#include "magick/cache.h"
#include "magick/color.h"
#include "magick/magick.h"
#include "magick/monitor.h"
#include "magick/utility.h"

/*
  Typedef declarations.
*/
typedef struct _PCXInfo
{
  unsigned char
    identifier,
    version,
    encoding,
    bits_per_pixel;

  short int
    left,
    top,
    right,
    bottom,
    horizontal_resolution,
    vertical_resolution;

  unsigned char
    reserved,
    planes;

  short int
    bytes_per_line,
    palette_info;

  unsigned char
    colormap_signature;
} PCXInfo;

/*
  Forward declarations.
*/
static unsigned int
  WritePCXImage(const ImageInfo *,Image *);

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I s D C X                                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method IsDCX returns True if the image format type, identified by the
%  magick string, is DCX.
%
%  The format of the IsDCX method is:
%
%      unsigned int IsDCX(const unsigned char *magick,const size_t length)
%
%  A description of each parameter follows:
%
%    o status:  Method IsDCX returns True if the image format type is DCX.
%
%    o magick: This string is generally the first few bytes of an image file
%      or blob.
%
%    o length: Specifies the length of the magick string.
%
%
*/
static unsigned int IsDCX(const unsigned char *magick,const size_t length)
{
  if (length < 4)
    return(False);
  if (memcmp(magick,"\261\150\336\72",4) == 0)
    return(True);
  return(False);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I s P C X                                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method IsPCX returns True if the image format type, identified by the
%  magick string, is PCX.
%
%  The format of the IsPCX method is:
%
%      unsigned int IsPCX(const unsigned char *magick,const size_t length)
%
%  A description of each parameter follows:
%
%    o status:  Method IsPCX returns True if the image format type is PCX.
%
%    o magick: This string is generally the first few bytes of an image file
%      or blob.
%
%    o length: Specifies the length of the magick string.
%
%
*/
static unsigned int IsPCX(const unsigned char *magick,const size_t length)
{
  if (length < 2)
    return(False);
  if (memcmp(magick,"\012\002",2) == 0)
    return(True);
  if (memcmp(magick,"\012\005",2) == 0)
    return(True);
  return(False);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e a d P C X I m a g e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method ReadPCXImage reads a ZSoft IBM PC Paintbrush file and returns it.
%  It allocates the memory necessary for the new Image structure and returns
%  a pointer to the new image.
%
%  The format of the ReadPCXImage method is:
%
%      Image *ReadPCXImage(const ImageInfo *image_info,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image:  Method ReadPCXImage returns a pointer to the image after
%      reading.  A null image is returned if there is a memory shortage or
%      if the image cannot be read.
%
%    o image_info: Specifies a pointer to a ImageInfo structure.
%
%    o exception: return any errors or warnings in this structure.
%
%
*/
static Image *ReadPCXImage(const ImageInfo *image_info,ExceptionInfo *exception)
{
  Image
    *image;

  ExtendedSignedIntegralType
    *page_table;

  int
    bits,
    id,
    mask;

  long
    y;

  PCXInfo
    pcx_info;

  register IndexPacket
    *indexes;

  register long
    x;

  register PixelPacket
    *q;

  register long
    i;

  register unsigned char
    *p,
    *r;

  size_t
    count;

  unsigned char
    packet,
    *pcx_colormap,
    *pcx_pixels,
    *scanline;

  unsigned int
    status;

  unsigned long
    pcx_packets;

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
    Determine if this is a PCX file.
  */
  page_table=(ExtendedSignedIntegralType *) NULL;
  if (LocaleCompare(image_info->magick,"DCX") == 0)
    {
      unsigned long
        magic;

      /*
        Read the DCX page table.
      */
      magic=ReadBlobLSBLong(image);
      if (magic != 987654321)
        ThrowReaderException(CorruptImageError,"NotADCXImageFile",image);
      page_table=(ExtendedSignedIntegralType *)
         AcquireMemory(1024*sizeof(ExtendedSignedIntegralType));
      if (page_table == (ExtendedSignedIntegralType *) NULL)
        ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed",image);
      for (id=0; id < 1024; id++)
      {
        page_table[id]=(ExtendedSignedIntegralType) ReadBlobLSBLong(image);
        if (page_table[id] == 0)
          break;
      }
    }
  if (page_table != (ExtendedSignedIntegralType *) NULL)
    (void) SeekBlob(image,(ExtendedSignedIntegralType) page_table[0],SEEK_SET);
  count=ReadBlob(image,1,(char *) &pcx_info.identifier);
  for (id=1; id < 1024; id++)
  {
    /*
      Verify PCX identifier.
    */
    pcx_info.version=ReadBlobByte(image);
    if ((count == 0) || (pcx_info.identifier != 0x0a))
      ThrowReaderException(CorruptImageError,"NotAPCXImageFile",image);
    pcx_info.encoding=ReadBlobByte(image);
    pcx_info.bits_per_pixel=ReadBlobByte(image);
    pcx_info.left=ReadBlobLSBShort(image);
    pcx_info.top=ReadBlobLSBShort(image);
    pcx_info.right=ReadBlobLSBShort(image);
    pcx_info.bottom=ReadBlobLSBShort(image);
    pcx_info.horizontal_resolution=ReadBlobLSBShort(image);
    pcx_info.vertical_resolution=ReadBlobLSBShort(image);
    /*
      Read PCX raster colormap.
    */
    image->columns=(pcx_info.right-pcx_info.left)+1;
    image->rows=(pcx_info.bottom-pcx_info.top)+1;
    image->depth=pcx_info.bits_per_pixel <= 8 ? 8 : QuantumDepth;
    image->units=PixelsPerInchResolution;
    image->x_resolution=pcx_info.horizontal_resolution;
    image->y_resolution=pcx_info.vertical_resolution;
    image->colors=16;
    pcx_colormap=(unsigned char *) AcquireMemory(3*256);
    if (pcx_colormap == (unsigned char *) NULL)
      ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed",image);
    (void) ReadBlob(image,3*image->colors,(char *) pcx_colormap);
    pcx_info.reserved=ReadBlobByte(image);
    pcx_info.planes=ReadBlobByte(image);
    if ((pcx_info.bits_per_pixel != 8) || (pcx_info.planes == 1))
      if ((pcx_info.version == 3) || (pcx_info.version == 5) ||
          ((pcx_info.bits_per_pixel*pcx_info.planes) == 1))
        image->colors=1 << (pcx_info.bits_per_pixel*pcx_info.planes);
    if (!AllocateImageColormap(image,image->colors))
      ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed",image);
    if ((pcx_info.bits_per_pixel >= 8) && (pcx_info.planes != 1))
      image->storage_class=DirectClass;
    p=pcx_colormap;
    for (i=0; i < (long) image->colors; i++)
    {
      image->colormap[i].red=ScaleCharToQuantum(*p++);
      image->colormap[i].green=ScaleCharToQuantum(*p++);
      image->colormap[i].blue=ScaleCharToQuantum(*p++);
    }
    pcx_info.bytes_per_line=ReadBlobLSBShort(image);
    pcx_info.palette_info=ReadBlobLSBShort(image);
    for (i=0; i < 58; i++)
      (void) ReadBlobByte(image);
    if (image_info->ping && (image_info->subrange != 0))
      if (image->scene >= (image_info->subimage+image_info->subrange-1))
        break;
    /*
      Read image data.
    */
    pcx_packets=image->rows*pcx_info.bytes_per_line*pcx_info.planes;
    pcx_pixels=(unsigned char *) AcquireMemory(pcx_packets);
    scanline=(unsigned char *) AcquireMemory(Max(image->columns,
      pcx_info.bytes_per_line)*pcx_info.planes);
    if ((pcx_pixels == (unsigned char *) NULL) ||
        (scanline == (unsigned char *) NULL))
      ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed",image);
    if (pcx_info.encoding == 0)
      {
        /*
          Data is not compressed
        */
        p=pcx_pixels;
        while(pcx_packets != 0)
          {
            packet=ReadBlobByte(image);
            *p++=packet;
            pcx_packets--;
            continue;
          }
      }
    else
      {
        /*
          Uncompress image data.
        */
        p=pcx_pixels;
        while (pcx_packets != 0)
          {
            packet=ReadBlobByte(image);
            if ((packet & 0xc0) != 0xc0)
              {
                *p++=packet;
                pcx_packets--;
                continue;
              }
            count=packet & 0x3f;
            for (packet=ReadBlobByte(image); count != 0; count--)
              {
                *p++=packet;
                pcx_packets--;
                if (pcx_packets == 0)
                  break;
              }
          }
      }
    if (image->storage_class == DirectClass)
      image->matte=pcx_info.planes > 3;
    else
      if ((pcx_info.version == 5) ||
          ((pcx_info.bits_per_pixel*pcx_info.planes) == 1))
        {
          /*
            Initialize image colormap.
          */
          if (image->colors > 256)
            ThrowReaderException(CorruptImageError,"ColormapExceeds256Colors",
              image);
          if ((pcx_info.bits_per_pixel*pcx_info.planes) == 1)
            {
              /*
                Monochrome colormap.
              */
              image->colormap[0].red=0;
              image->colormap[0].green=0;
              image->colormap[0].blue=0;
              image->colormap[1].red=MaxRGB;
              image->colormap[1].green=MaxRGB;
              image->colormap[1].blue=MaxRGB;
            }
          else
            if (image->colors > 16)
              {
                /*
                  256 color images have their color map at the end of the file.
                */
                pcx_info.colormap_signature=ReadBlobByte(image);
                (void) ReadBlob(image,3*image->colors,(char *) pcx_colormap);
                p=pcx_colormap;
                for (i=0; i < (long) image->colors; i++)
                {
                  image->colormap[i].red=ScaleCharToQuantum(*p++);
                  image->colormap[i].green=ScaleCharToQuantum(*p++);
                  image->colormap[i].blue=ScaleCharToQuantum(*p++);
                }
            }
          LiberateMemory((void **) &pcx_colormap);
        }
    /*
      Convert PCX raster image to pixel packets.
    */
    for (y=0; y < (long) image->rows; y++)
    {
      p=pcx_pixels+(y*pcx_info.bytes_per_line*pcx_info.planes);
      q=SetImagePixels(image,0,y,image->columns,1);
      if (q == (PixelPacket *) NULL)
        break;
      indexes=GetIndexes(image);
      r=scanline;
      if (image->storage_class == DirectClass)
        for (i=0; i < pcx_info.planes; i++)
        {
          r=scanline+i;
          for (x=0; x < pcx_info.bytes_per_line; x++)
          {
            switch (i)
            {
              case 0:
              {
                *r=ScaleCharToQuantum(*p++);
                break;
              }
              case 1:
              {
                *r=ScaleCharToQuantum(*p++);
                break;
              }
              case 2:
              {
                *r=ScaleCharToQuantum(*p++);
                break;
              }
              case 3:
              default:
              {
                *r=ScaleCharToQuantum(*p++);
                break;
              }
            }
            r+=pcx_info.planes;
          }
        }
      else
        if (pcx_info.planes > 1)
          {
            for (x=0; x < (long) image->columns; x++)
              *r++=0;
            for (i=0; i < pcx_info.planes; i++)
            {
              r=scanline;
              for (x=0; x < pcx_info.bytes_per_line; x++)
              {
                 bits=(*p++);
                 for (mask=0x80; mask != 0; mask>>=1)
                 {
                   if (bits & mask)
                     *r|=1 << i;
                   r++;
                 }
               }
            }
          }
        else
          switch (pcx_info.bits_per_pixel)
          {
            case 1:
            {
              register long
                bit;

              for (x=0; x < ((long) image->columns-7); x+=8)
              {
                for (bit=7; bit >= 0; bit--)
                  *r++=((*p) & (0x01 << bit) ? 0x01 : 0x00);
                p++;
              }
              if ((image->columns % 8) != 0)
                {
                  for (bit=7; bit >= (long) (8-(image->columns % 8)); bit--)
                    *r++=((*p) & (0x01 << bit) ? 0x01 : 0x00);
                  p++;
                }
              break;
            }
            case 2:
            {
              for (x=0; x < ((long) image->columns-3); x+=4)
              {
                *r++=(*p >> 6) & 0x3;
                *r++=(*p >> 4) & 0x3;
                *r++=(*p >> 2) & 0x3;
                *r++=(*p) & 0x3;
                p++;
              }
              if ((image->columns % 4) != 0)
                {
                  for (i=3; i >= (long) (4-(image->columns % 4)); i--)
                    *r++=(*p >> (i*2)) & 0x03;
                  p++;
                }
              break;
            }
            case 4:
            {
              for (x=0; x < ((long) image->columns-1); x+=2)
              {
                *r++=(*p >> 4) & 0xf;
                *r++=(*p) & 0xf;
                p++;
              }
              if ((image->columns % 2) != 0)
                *r++=(*p++ >> 4) & 0xf;
              break;
            }
            case 8:
            {
              (void) memcpy(r,p,image->columns);
              break;
            }
            default:
              break;
          }
      /*
        Transfer image scanline.
      */
      r=scanline;
      for (x=0; x < (long) image->columns; x++)
      {
        if (image->storage_class == PseudoClass)
          indexes[x]=(*r++);
        else
          {
            q->red=ScaleCharToQuantum(*r++);
            q->green=ScaleCharToQuantum(*r++);
            q->blue=ScaleCharToQuantum(*r++);
            if (image->matte)
              q->opacity=(Quantum) (MaxRGB-ScaleCharToQuantum(*r++));
          }
        q++;
      }
      if (!SyncImagePixels(image))
        break;
      if (image->previous == (Image *) NULL)
        if (QuantumTick(y,image->rows))
          if (!MagickMonitor(LoadImageText,y,image->rows,exception))
            break;
    }
    if (image->storage_class == PseudoClass)
      SyncImage(image);
    LiberateMemory((void **) &scanline);
    LiberateMemory((void **) &pcx_pixels);
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
    if (page_table == (ExtendedSignedIntegralType *) NULL)
      break;
    if (page_table[id] == 0)
      break;
    (void) SeekBlob(image,(ExtendedSignedIntegralType) page_table[id],SEEK_SET);
    count=ReadBlob(image,1,(char *) &pcx_info.identifier);
    if ((count != 0) && (pcx_info.identifier == 0x0a))
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
  }
  if (page_table != (ExtendedSignedIntegralType *) NULL)
    LiberateMemory((void **) &page_table);
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
%   R e g i s t e r P C X I m a g e                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method RegisterPCXImage adds attributes for the PCX image format to
%  the list of supported formats.  The attributes include the image format
%  tag, a method to read and/or write the format, whether the format
%  supports the saving of more than one frame to the same file or blob,
%  whether the format supports native in-memory I/O, and a brief
%  description of the format.
%
%  The format of the RegisterPCXImage method is:
%
%      RegisterPCXImage(void)
%
*/
ModuleExport void RegisterPCXImage(void)
{
  MagickInfo
    *entry;

  entry=SetMagickInfo("DCX");
  entry->decoder=(DecoderHandler) ReadPCXImage;
  entry->encoder=(EncoderHandler) WritePCXImage;
  entry->seekable_stream=True;
  entry->magick=(MagickHandler) IsDCX;
  entry->description=AcquireString("ZSoft IBM PC multi-page Paintbrush");
  entry->module=AcquireString("PCX");
  (void) RegisterMagickInfo(entry);
  entry=SetMagickInfo("PCX");
  entry->decoder=(DecoderHandler) ReadPCXImage;
  entry->encoder=(EncoderHandler) WritePCXImage;
  entry->magick=(MagickHandler) IsPCX;
  entry->adjoin=False;
  entry->seekable_stream=True;
  entry->description=AcquireString("ZSoft IBM PC Paintbrush");
  entry->module=AcquireString("PCX");
  (void) RegisterMagickInfo(entry);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   U n r e g i s t e r P C X I m a g e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method UnregisterPCXImage removes format registrations made by the
%  PCX module from the list of supported formats.
%
%  The format of the UnregisterPCXImage method is:
%
%      UnregisterPCXImage(void)
%
*/
ModuleExport void UnregisterPCXImage(void)
{
  (void) UnregisterMagickInfo("DCX");
  (void) UnregisterMagickInfo("PCX");
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   W r i t e P C X I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method WritePCXImage writes an image in the ZSoft IBM PC Paintbrush file
%  format.
%
%  The format of the WritePCXImage method is:
%
%      unsigned int WritePCXImage(const ImageInfo *image_info,Image *image)
%
%  A description of each parameter follows.
%
%    o status: Method WritePCXImage return True if the image is written.
%      False is returned is there is a memory shortage or if the image file
%      fails to write.
%
%    o image_info: Specifies a pointer to a ImageInfo structure.
%
%    o image:  A pointer to an Image structure.
%
%
*/
static unsigned int WritePCXImage(const ImageInfo *image_info,Image *image)
{
  long
    y;

  PCXInfo
    pcx_info;

  register const PixelPacket
    *p;

  register IndexPacket
    *indexes;

  register long
    i,
    x;

  register unsigned char
    *q;

  size_t
    length;

  unsigned char
    count,
    packet,
    *pcx_colormap,
    *pcx_pixels,
    previous;

  unsigned int
    status;

  ExtendedSignedIntegralType
    *page_table;

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
  TransformColorspace(image,RGBColorspace);
  page_table=(ExtendedSignedIntegralType *) NULL;
  if (image_info->adjoin)
    {
      /*
        Write the DCX page table.
      */
      (void) WriteBlobLSBLong(image,0x3ADE68B1L);
      page_table=(ExtendedSignedIntegralType *)
        AcquireMemory(1024*sizeof(ExtendedSignedIntegralType));
      if (page_table == (ExtendedSignedIntegralType *) NULL)
        ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed",image);
      for (scene=0; scene < 1024; scene++)
        (void) WriteBlobLSBLong(image,0x00000000L);
    }
  scene=0;
  do
  {
    if (page_table != (ExtendedSignedIntegralType *) NULL)
      page_table[scene]=TellBlob(image);
    /*
      Initialize PCX raster file header.
    */
    pcx_info.identifier=0x0a;
    pcx_info.version=5;
    pcx_info.encoding=1;
    pcx_info.bits_per_pixel=8;
    if ((image->storage_class == PseudoClass) &&
        IsMonochromeImage(image,&image->exception))
      pcx_info.bits_per_pixel=1;
    pcx_info.left=0;
    pcx_info.top=0;
    pcx_info.right=(short) (image->columns-1);
    pcx_info.bottom=(short) (image->rows-1);
    pcx_info.horizontal_resolution=(short) image->columns;
    pcx_info.vertical_resolution=(short) image->rows;
    if (image->units == PixelsPerInchResolution)
      {
        pcx_info.horizontal_resolution=(short) image->x_resolution;
        pcx_info.vertical_resolution=(short) image->y_resolution;
      }
    if (image->units == PixelsPerCentimeterResolution)
      {
        pcx_info.horizontal_resolution=(short) (2.54*image->x_resolution);
        pcx_info.vertical_resolution=(short) (2.54*image->y_resolution);
      }
    pcx_info.reserved=0;
    pcx_info.planes=1;
    if (image->storage_class == DirectClass)
      {
        pcx_info.planes=3;
        if (image->matte)
          pcx_info.planes++;
      }
    pcx_info.bytes_per_line=(short)
      (image->columns*pcx_info.bits_per_pixel+7)/8;
    pcx_info.palette_info=1;
    pcx_info.colormap_signature=0x0c;
    /*
      Write PCX header.
    */
    (void) WriteBlobByte(image,pcx_info.identifier);
    (void) WriteBlobByte(image,pcx_info.version);
    (void) WriteBlobByte(image,pcx_info.encoding);
    (void) WriteBlobByte(image,pcx_info.bits_per_pixel);
    (void) WriteBlobLSBShort(image,(unsigned int) pcx_info.left);
    (void) WriteBlobLSBShort(image,(unsigned int) pcx_info.top);
    (void) WriteBlobLSBShort(image,(unsigned int) pcx_info.right);
    (void) WriteBlobLSBShort(image,(unsigned int) pcx_info.bottom);
    (void) WriteBlobLSBShort(image,(unsigned int)
      pcx_info.horizontal_resolution);
    (void) WriteBlobLSBShort(image,(unsigned int) pcx_info.vertical_resolution);
    /*
      Dump colormap to file.
    */
    pcx_colormap=(unsigned char *) AcquireMemory(3*256);
    if (pcx_colormap == (unsigned char *) NULL)
      ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed",image);
    for (i=0; i < (3*256); i++)
      pcx_colormap[i]=0;
    q=pcx_colormap;
    if (image->storage_class == PseudoClass)
      for (i=0; i < (long) image->colors; i++)
      {
        *q++=ScaleQuantumToChar(image->colormap[i].red);
        *q++=ScaleQuantumToChar(image->colormap[i].green);
        *q++=ScaleQuantumToChar(image->colormap[i].blue);
      }
    (void) WriteBlob(image,3*16,(char *) pcx_colormap);
    (void) WriteBlobByte(image,pcx_info.reserved);
    (void) WriteBlobByte(image,pcx_info.planes);
    (void) WriteBlobLSBShort(image,(unsigned int) pcx_info.bytes_per_line);
    (void) WriteBlobLSBShort(image,(unsigned int) pcx_info.palette_info);
    for (i=0; i < 58; i++)
      (void) WriteBlobByte(image,'\0');
    length=image->rows*pcx_info.bytes_per_line*pcx_info.planes;
    pcx_pixels=(unsigned char *) AcquireMemory(length);
    if (pcx_pixels == (unsigned char *) NULL)
      ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed",image);
    q=pcx_pixels;
    if (image->storage_class == DirectClass)
      {
        /*
          Convert DirectClass image to PCX raster pixels.
        */
        for (y=0; y < (long) image->rows; y++)
        {
          q=pcx_pixels+(y*pcx_info.bytes_per_line*pcx_info.planes);
          for (i=0; i < pcx_info.planes; i++)
          {
            p=AcquireImagePixels(image,0,y,image->columns,1,&image->exception);
            if (p == (const PixelPacket *) NULL)
              break;
            for (x=0; x < pcx_info.bytes_per_line; x++)
            {
              switch ((int) i)
              {
                case 0:
                {
                  *q++=ScaleQuantumToChar(p->red);
                  break;
                }
                case 1:
                {
                  *q++=ScaleQuantumToChar(p->green);
                  break;
                }
                case 2:
                {
                  *q++=ScaleQuantumToChar(p->blue);
                  break;
                }
                case 3:
                default:
                {
                  *q++=ScaleQuantumToChar(MaxRGB-p->opacity);
                  break;
                }
              }
              p++;
            }
          }
          if (QuantumTick(y,image->rows))
            if (!MagickMonitor(SaveImageText,y,image->rows,&image->exception))
              break;
        }
      }
    else
      if (pcx_info.bits_per_pixel > 1)
        for (y=0; y < (long) image->rows; y++)
        {
          p=AcquireImagePixels(image,0,y,image->columns,1,&image->exception);
          if (p == (const PixelPacket *) NULL)
            break;
          indexes=GetIndexes(image);
          q=pcx_pixels+y*pcx_info.bytes_per_line;
          for (x=0; x < (long) image->columns; x++)
            *q++=indexes[x];
          if (image->previous == (Image *) NULL)
            if (QuantumTick(y,image->rows))
              if (!MagickMonitor(SaveImageText,y,image->rows,&image->exception))
                break;
        }
      else
        {
          register unsigned char
            bit,
            byte,
            polarity;

          /*
            Convert PseudoClass image to a PCX monochrome image.
          */
          polarity=PixelIntensityToQuantum(&image->colormap[0]) < (MaxRGB/2);
          if (image->colors == 2)
            polarity=PixelIntensityToQuantum(&image->colormap[0]) <
              PixelIntensityToQuantum(&image->colormap[1]);
          for (y=0; y < (long) image->rows; y++)
          {
            p=AcquireImagePixels(image,0,y,image->columns,1,&image->exception);
            if (p == (const PixelPacket *) NULL)
              break;
            indexes=GetIndexes(image);
            bit=0;
            byte=0;
            q=pcx_pixels+y*pcx_info.bytes_per_line;
            for (x=0; x < (long) image->columns; x++)
            {
              byte<<=1;
              if (indexes[x] == polarity)
                byte|=0x01;
              bit++;
              if (bit == 8)
                {
                  *q++=byte;
                  bit=0;
                  byte=0;
                }
              p++;
            }
            if (bit != 0)
              *q++=byte << (8-bit);
            if (image->previous == (Image *) NULL)
              if (QuantumTick(y,image->rows))
                if (!MagickMonitor(SaveImageText,y,image->rows,&image->exception))
                  break;
          }
        }
    /*
      Runlength-encoded PCX pixels.
    */
    for (y=0; y < (long) image->rows; y++)
    {
      q=pcx_pixels+(y*pcx_info.bytes_per_line*pcx_info.planes);
      for (i=0; i < pcx_info.planes; i++)
      {
        previous=(*q++);
        count=1;
        for (x=0; x < (long) (pcx_info.bytes_per_line-1); x++)
        {
          packet=(*q++);
          if ((packet == previous) && (count < 63))
            {
              count++;
              continue;
            }
          if ((count > 1) || ((previous & 0xc0) == 0xc0))
            {
              count|=0xc0;
              (void) WriteBlobByte(image,count);
            }
          (void) WriteBlobByte(image,previous);
          previous=packet;
          count=1;
        }
        if ((count > 1) || ((previous & 0xc0) == 0xc0))
          {
            count|=0xc0;
            (void) WriteBlobByte(image,count);
          }
        (void) WriteBlobByte(image,previous);
        if (QuantumTick(y,image->rows))
          if (!MagickMonitor(SaveImageText,y,image->rows,&image->exception))
            break;
      }
    }
    (void) WriteBlobByte(image,pcx_info.colormap_signature);
    (void) WriteBlob(image,3*256,(char *) pcx_colormap);
    LiberateMemory((void **) &pcx_pixels);
    LiberateMemory((void **) &pcx_colormap);
    if (image->next == (Image *) NULL)
      break;
    image=SyncNextImageInList(image);
    status=MagickMonitor(SaveImagesText,scene++,GetImageListLength(image),
      &image->exception);
    if (status == False)
      break;
    if (scene >= 1023)
      break;
  } while (image_info->adjoin);
  if (image_info->adjoin)
    while (image->previous != (Image *) NULL)
      image=image->previous;
  if (page_table != (ExtendedSignedIntegralType *) NULL)
    {
      /*
        Write the DCX page table.
      */
      page_table[scene+1]=0;
      (void) SeekBlob(image,0L,SEEK_SET);
      (void) WriteBlobLSBLong(image,0x3ADE68B1L);
      for (i=0; i <= (long) scene; i++)
        (void) WriteBlobLSBLong(image,(unsigned long) page_table[i]);
      LiberateMemory((void **) &page_table);
    }
  if (status == False)
    ThrowWriterException(FileOpenError,"UnableToWriteFile",image);
  CloseBlob(image);
  return(True);
}
