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
%                            TTTTT  X   X  TTTTT                              %
%                              T     X X     T                                %
%                              T      X      T                                %
%                              T     X X     T                                %
%                              T    X   X    T                                %
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
#include "magick/constitute.h"
#include "magick/magick.h"
#include "magick/monitor.h"
#include "magick/render.h"
#include "magick/utility.h"

/*
  Forward declarations.
*/
static unsigned int
  WriteTXTImage(const ImageInfo *,Image *);

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e a d T X T I m a g e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method ReadTXTImage reads a text file and returns it as an image.  It
%  allocates the memory necessary for the new Image structure and returns a
%  pointer to the new image.
%
%  The format of the ReadTXTImage method is:
%
%      Image *ReadTXTImage(const ImageInfo *image_info,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image:  Method ReadTXTImage returns a pointer to the image after
%      reading. A null image is returned if there is a memory shortage or if
%      the image cannot be read.
%
%    o image_info: Specifies a pointer to a ImageInfo structure.
%
%    o exception: return any errors or warnings in this structure.
%
%
*/
static Image *ReadTXTImage(const ImageInfo *image_info,ExceptionInfo *exception)
{
  char
    filename[MaxTextExtent],
    geometry[MaxTextExtent],
    *p,
    text[MaxTextExtent];

  double
    dx_resolution,
    dy_resolution;

  DrawInfo
    *draw_info;

  Image
    *image,
    *texture;

  long
    count,
    offset;

  RectangleInfo
    page;

  TypeMetric
    metrics;

  unsigned int
    status;

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
    Set the page geometry.
  */
  dx_resolution=72.0;
  dy_resolution=72.0;
  if ((image->x_resolution == 0.0) || (image->y_resolution == 0.0))
    {
      char
        density[MaxTextExtent];

      (void) strcpy(density,PSDensityGeometry);
      count=sscanf(density,"%lfx%lf",&image->x_resolution,
        &image->y_resolution);
      if (count != 2)
        image->y_resolution=image->x_resolution;
    }
  SetGeometry(image,&page);
  page.width=612;
  page.height=792;
  (void) GetGeometry("612x792+43+43",&page.x,&page.y,&page.width,&page.height);
  if (image_info->page != (char *) NULL)
    (void) GetGeometry(image_info->page,&page.x,&page.y,&page.width,
      &page.height);
  /*
    Initialize Image structure.
  */
  image->columns=(unsigned long)
    ceil(((page.width*image->x_resolution)/dx_resolution)-0.5);
  image->rows=(unsigned long)
    ceil(((page.height*image->y_resolution)/dy_resolution)-0.5);
  texture=(Image *) NULL;
  if (image_info->texture != (char *) NULL)
    {
      ImageInfo
        *clone_info;

      clone_info=CloneImageInfo(image_info);
      clone_info->blob=(void *) NULL;
      clone_info->length=0;
      (void) strncpy(clone_info->filename,image_info->texture,MaxTextExtent-1);
      texture=ReadImage(clone_info,exception);
      DestroyImageInfo(clone_info);
    }
  /*
    Annotate the text image.
  */
  SetImage(image,OpaqueOpacity);
  draw_info=CloneDrawInfo(image_info,(DrawInfo *) NULL);
  draw_info->fill=image_info->pen;
  (void) CloneString(&draw_info->text,image_info->filename);
  FormatString(geometry,"0x0%+ld%+ld",page.x,page.y);
  (void) CloneString(&draw_info->geometry,geometry);
  status=GetTypeMetrics(image,draw_info,&metrics);
  if (status == False)
    ThrowReaderException(TypeError,"UnableToGetTypeMetrics",image);
  (void) strncpy(filename,image_info->filename,MaxTextExtent-1);
  if (draw_info->text != '\0')
    *draw_info->text='\0';
  p=ReadBlobString(image,text);
  for (offset=2*page.y; p != (char *) NULL; )
  {
    /*
      Annotate image with text.
    */
    (void) ConcatenateString(&draw_info->text,text);
    (void) ConcatenateString(&draw_info->text,"\\n");
    offset+=(long) (metrics.ascent-metrics.descent);
    if (image->previous == (Image *) NULL)
      if (QuantumTick(offset,image->rows))
        if (!MagickMonitor(LoadImageText,offset,image->rows,&image->exception))
          break;
    p=ReadBlobString(image,text);
    if ((offset < (long) image->rows) && (p != (char *) NULL))
      continue;
    if (texture != (Image *) NULL)
      {
        MonitorHandler
          handler;

        handler=SetMonitorHandler((MonitorHandler) NULL);
        TextureImage(image,texture);
        (void) SetMonitorHandler(handler);
      }
    (void) AnnotateImage(image,draw_info);
    if (p == (char *) NULL)
      break;
    /*
      Page is full-- allocate next image structure.
    */
    *draw_info->text='\0';
    offset=2*page.y;
    AllocateNextImage(image_info,image);
    if (image->next == (Image *) NULL)
      {
        DestroyImageList(image);
        return((Image *) NULL);
      }
    image->next->columns=image->columns;
    image->next->rows=image->rows;
    image=SyncNextImageInList(image);
    (void) strncpy(image->filename,filename,MaxTextExtent-1);
    SetImage(image,OpaqueOpacity);
    if (!MagickMonitor(LoadImagesText,TellBlob(image),GetBlobSize(image),exception))
      break;
  }
  if (texture != (Image *) NULL)
    {
      MonitorHandler
        handler;

      handler=SetMonitorHandler((MonitorHandler) NULL);
      TextureImage(image,texture);
      (void) SetMonitorHandler(handler);
    }
  (void) AnnotateImage(image,draw_info);
  if (texture != (Image *) NULL)
    DestroyImage(texture);
  DestroyDrawInfo(draw_info);
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
%   R e g i s t e r T X T I m a g e                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method RegisterTXTImage adds attributes for the TXT image format to
%  the list of supported formats.  The attributes include the image format
%  tag, a method to read and/or write the format, whether the format
%  supports the saving of more than one frame to the same file or blob,
%  whether the format supports native in-memory I/O, and a brief
%  description of the format.
%
%  The format of the RegisterTXTImage method is:
%
%      RegisterTXTImage(void)
%
*/
ModuleExport void RegisterTXTImage(void)
{
  MagickInfo
    *entry;

  entry=SetMagickInfo("TEXT");
  entry->decoder=(DecoderHandler) ReadTXTImage;
  entry->encoder=(EncoderHandler) WriteTXTImage;
  entry->raw=True;
  entry->description=AcquireString("Text");
  entry->module=AcquireString("TXT");
  (void) RegisterMagickInfo(entry);
  entry=SetMagickInfo("TXT");
  entry->decoder=(DecoderHandler) ReadTXTImage;
  entry->encoder=(EncoderHandler) WriteTXTImage;
  entry->description=AcquireString("Text");
  entry->module=AcquireString("TXT");
  (void) RegisterMagickInfo(entry);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   U n r e g i s t e r T X T I m a g e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method UnregisterTXTImage removes format registrations made by the
%  TXT module from the list of supported formats.
%
%  The format of the UnregisterTXTImage method is:
%
%      UnregisterTXTImage(void)
%
*/
ModuleExport void UnregisterTXTImage(void)
{
  (void) UnregisterMagickInfo("TEXT");
  (void) UnregisterMagickInfo("TXT");
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   W r i t e T X T I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method WriteTXTImage writes the pixel values as text numbers.
%
%  The format of the WriteTXTImage method is:
%
%      unsigned int WriteTXTImage(const ImageInfo *image_info,Image *image)
%
%  A description of each parameter follows.
%
%    o status: Method WriteTXTImage return True if the image is written.
%      False is returned is there is a memory shortage or if the image file
%      fails to write.
%
%    o image_info: Specifies a pointer to a ImageInfo structure.
%
%    o image:  A pointer to an Image structure.
%
%
*/
static unsigned int WriteTXTImage(const ImageInfo *image_info,Image *image)
{
  char
    buffer[MaxTextExtent];

  long
    y;

  register const PixelPacket
    *p;

  register long
    x;

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
  status=OpenBlob(image_info,image,WriteBlobMode,&image->exception);
  if (status == False)
    ThrowWriterException(FileOpenError,"UnableToOpenFile",image);
  scene=0;
  do
  {
    /*
      Convert MIFF to TXT raster pixels.
    */
    TransformColorspace(image,RGBColorspace);
    for (y=0; y < (long) image->rows; y++)
    {
      p=AcquireImagePixels(image,0,y,image->columns,1,&image->exception);
      if (p == (const PixelPacket *) NULL)
        break;
      for (x=0; x < (long) image->columns; x++)
      {
        if (image->matte)
          {
            FormatString(buffer,"%ld,%ld: %u,%u,%u,%u ",x,y,
              p->red,p->green,p->blue,p->opacity);
            (void) WriteBlobString(image,buffer);
          }
        else
          {
            FormatString(buffer,"%ld,%ld: %u,%u,%u ",x,y,
              p->red,p->green,p->blue);
            (void) WriteBlobString(image,buffer);
            (void) QueryColorname(image,p,SVGCompliance,buffer,
              &image->exception);
            (void) WriteBlobString(image,buffer);
          }
        (void) WriteBlobByte(image,'\n');
        if (image->previous == (Image *) NULL)
          if (QuantumTick(y,image->rows))
            if (!MagickMonitor(SaveImageText,y,image->rows,&image->exception))
              break;
        p++;
      }
    }
    if (image->next == (Image *) NULL)
      break;
    image=SyncNextImageInList(image);
    status=MagickMonitor(SaveImagesText,scene++,GetImageListLength(image),
      &image->exception);
    if (status == False)
      break;
  } while (image_info->adjoin);
  if (image_info->adjoin)
    while (image->previous != (Image *) NULL)
      image=image->previous;
  CloseBlob(image);
  return(True);
}
