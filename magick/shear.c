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
%                      SSSSS  H   H  EEEEE   AAA    RRRR                      %
%                      SS     H   H  E      A   A   R   R                     %
%                       SSS   HHHHH  EEE    AAAAA   RRRR                      %
%                         SS  H   H  E      A   A   R R                       %
%                      SSSSS  H   H  EEEEE  A   A   R  R                      %
%                                                                             %
%                                                                             %
%            Methods to Shear or Rotate an Image by an Arbitrary Angle        %
%                                                                             %
%                                                                             %
%                               Software Design                               %
%                                 John Cristy                                 %
%                                  July 1992                                  %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method RotateImage, XShearImage, and YShearImage is based on the paper
%  "A Fast Algorithm for General Raster Rotatation" by Alan W. Paeth,
%  Graphics Interface '86 (Vancouver).  RotateImage is adapted from a similar
%  method based on the Paeth paper written by Michael Halle of the Spatial
%  Imaging Group, MIT Media Lab.
%
%
*/

/*
  Include declarations.
*/
#include "magick/studio.h"
#include "magick/cache.h"
#include "magick/decorate.h"
#include "magick/monitor.h"
#include "magick/render.h"
#include "magick/shear.h"
#include "magick/transform.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%     A f f i n e T r a n s f o r m I m a g e                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AffineTransformImage() transforms an image as dictated by the affine matrix.
%  It allocates the memory necessary for the new Image structure and returns
%  a pointer to the new image.
%
%  The format of the AffineTransformImage method is:
%
%      Image *AffineTransformImage(const Image *image,
%        AffineMatrix *affine,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o affine: The affine transform.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport Image *AffineTransformImage(const Image *image,
  const AffineMatrix *affine,ExceptionInfo *exception)
{
  AffineMatrix
    transform;

  Image
    *affine_image;

  long
    y;

  PointInfo
    extent[4],
    min,
    max;

  register long
    i,
    x;

  /*
    Determine bounding box.
  */
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickSignature);
  assert(affine != (AffineMatrix *) NULL);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  extent[0].x=0;
  extent[0].y=0;
  extent[1].x=image->columns;
  extent[1].y=0;
  extent[2].x=image->columns;
  extent[2].y=image->rows;
  extent[3].x=0;
  extent[3].y=image->rows;
  for (i=0; i < 4; i++)
  {
    x=(long) (extent[i].x+0.5);
    y=(long) (extent[i].y+0.5);
    extent[i].x=x*affine->sx+y*affine->ry+affine->tx;
    extent[i].y=x*affine->rx+y*affine->sy+affine->ty;
  }
  min=extent[0];
  max=extent[0];
  for (i=1; i < 4; i++)
  {
    if (min.x > extent[i].x)
      min.x=extent[i].x;
    if (min.y > extent[i].y)
      min.y=extent[i].y;
    if (max.x < extent[i].x)
      max.x=extent[i].x;
    if (max.y < extent[i].y)
      max.y=extent[i].y;
  }
  /*
    Affine transform image.
  */
  affine_image=CloneImage(image,(unsigned long) ceil(max.x-min.x-0.5),
    (unsigned long) ceil(max.y-min.y-0.5),True,exception);
  if (affine_image == (Image *) NULL)
    return((Image *) NULL);
  SetImage(affine_image,TransparentOpacity);
  transform.sx=affine->sx;
  transform.rx=affine->rx;
  transform.ry=affine->ry;
  transform.sy=affine->sy;
  transform.tx=(-min.x);
  transform.ty=(-min.y);
  DrawAffineImage(affine_image,image,&transform);
  return(affine_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   C r o p T o F i t I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method CropToFitImage crops the sheared image as determined by the bounding
%  box as defined by width and height and shearing angles.
%
%  The format of the CropToFitImage method is:
%
%      Image *CropToFitImage(Image **image,const double x_shear,
%        const double x_shear,const double width,const double height,
%        const unsigne int rotate,ExceptionInfo *exception)
%
%  A description of each parameter follows.
%
%    o image: The address of a structure of type Image.
%
%    o x_shear, y_shear, width, height: Defines a region of the image to crop.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
static void CropToFitImage(Image **image,const double x_shear,
  const double y_shear,const double width,const double height,
  const unsigned int rotate,ExceptionInfo *exception)
{
  Image
    *crop_image;

  PointInfo
    extent[4],
    min,
    max;

  RectangleInfo
    geometry;

  register long
    i;

  /*
    Calculate the rotated image size.
  */
  extent[0].x=(-width/2.0);
  extent[0].y=(-height/2.0);
  extent[1].x=width/2.0;
  extent[1].y=(-height/2.0);
  extent[2].x=(-width/2.0);
  extent[2].y=height/2.0;
  extent[3].x=width/2.0;
  extent[3].y=height/2.0;
  for (i=0; i < 4; i++)
  {
    extent[i].x+=x_shear*extent[i].y;
    extent[i].y+=y_shear*extent[i].x;
    if (rotate)
      extent[i].x+=x_shear*extent[i].y;
    extent[i].x+=(double) (*image)->columns/2.0;
    extent[i].y+=(double) (*image)->rows/2.0;
  }
  min=extent[0];
  max=extent[0];
  for (i=1; i < 4; i++)
  {
    if (min.x > extent[i].x)
      min.x=extent[i].x;
    if (min.y > extent[i].y)
      min.y=extent[i].y;
    if (max.x < extent[i].x)
      max.x=extent[i].x;
    if (max.y < extent[i].y)
      max.y=extent[i].y;
  }
  geometry.width=(unsigned long) floor(max.x-min.x+0.5);
  geometry.height=(unsigned long) floor(max.y-min.y+0.5);
  geometry.x=(long) ceil(min.x-0.5);
  geometry.y=(long) ceil(min.y-0.5);
  crop_image=CropImage(*image,&geometry,exception);
  if (crop_image != (Image *) NULL)
    {
      crop_image->page=(*image)->page;
      DestroyImage(*image);
      *image=crop_image;
    }
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   I n t e g r a l R o t a t e I m a g e                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method IntegralRotateImage rotates the image an integral of 90 degrees.
%  It allocates the memory necessary for the new Image structure and returns
%  a pointer to the rotated image.
%
%  The format of the IntegralRotateImage method is:
%
%      Image *IntegralRotateImage(const Image *image,unsigned int rotations,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows.
%
%    o rotate_image: Method IntegralRotateImage returns a pointer to the
%      rotated image.  A null image is returned if there is a a memory shortage.
%
%    o image: The image.
%
%    o rotations: Specifies the number of 90 degree rotations.
%
%
*/
static Image *IntegralRotateImage(const Image *image,unsigned int rotations,
  ExceptionInfo *exception)
{
#define RotateImageText  "  Rotate image...  "

  Image
    *rotate_image;

  long
    y;

  RectangleInfo
    page;

  register IndexPacket
    *indexes,
    *rotate_indexes;

  register const PixelPacket
    *p;

  register long
    x;

  register PixelPacket
    *q;

  /*
    Initialize rotated image attributes.
  */
  assert(image != (Image *) NULL);
  page=image->page;
  rotations%=4;
  if ((rotations == 1) || (rotations == 3))
    rotate_image=CloneImage(image,image->rows,image->columns,True,exception);
  else
    rotate_image=CloneImage(image,image->columns,image->rows,True,exception);
  if (rotate_image == (Image *) NULL)
    return((Image *) NULL);
  /*
    Integral rotate the image.
  */
  switch (rotations)
  {
    case 0:
    {
      /*
        Rotate 0 degrees.
      */
      for (y=0; y < (long) image->rows; y++)
      {
        p=AcquireImagePixels(image,0,y,image->columns,1,exception);
        q=SetImagePixels(rotate_image,0,y,rotate_image->columns,1);
        if ((p == (const PixelPacket *) NULL) || (q == (PixelPacket *) NULL))
          break;
        (void) memcpy(q,p,image->columns*sizeof(PixelPacket));
        indexes=GetIndexes(image);
        rotate_indexes=GetIndexes(rotate_image);
        if ((indexes != (IndexPacket *) NULL) &&
            (rotate_indexes != (IndexPacket *) NULL))
          (void) memcpy(rotate_indexes,indexes,image->columns*
            sizeof(IndexPacket));
        if (!SyncImagePixels(rotate_image))
          break;
        if (QuantumTick(y,image->rows))
          if (!MagickMonitor(RotateImageText,y,image->rows,exception))
            break;
      }
      break;
    }
    case 1:
    {
      /*
        Rotate 90 degrees.
      */
      for (y=0; y < (long) image->rows; y++)
      {
        p=AcquireImagePixels(image,0,y,image->columns,1,exception);
        q=SetImagePixels(rotate_image,(long) (image->rows-y-1),0,1,
          rotate_image->rows);
        if ((p == (const PixelPacket *) NULL) || (q == (PixelPacket *) NULL))
          break;
        (void) memcpy(q,p,image->columns*sizeof(PixelPacket));
        indexes=GetIndexes(image);
        rotate_indexes=GetIndexes(rotate_image);
        if ((indexes != (IndexPacket *) NULL) &&
            (rotate_indexes != (IndexPacket *) NULL))
          (void) memcpy(rotate_indexes,indexes,image->columns*
            sizeof(IndexPacket));
        if (!SyncImagePixels(rotate_image))
          break;
        if (QuantumTick(y,image->rows))
          if (!MagickMonitor(RotateImageText,y,image->rows,exception))
            break;
      }
      Swap(page.width,page.height);
      Swap(page.x,page.y);
      page.x=(long) (page.width-rotate_image->columns-page.x);
      break;
    }
    case 2:
    {
      /*
        Rotate 180 degrees.
      */
      for (y=0; y < (long) image->rows; y++)
      {
        p=AcquireImagePixels(image,0,y,image->columns,1,exception);
        q=SetImagePixels(rotate_image,0,(long) (image->rows-y-1),
          image->columns,1);
        if ((p == (const PixelPacket *) NULL) || (q == (PixelPacket *) NULL))
          break;
        q+=image->columns;
        indexes=GetIndexes(image);
        rotate_indexes=GetIndexes(rotate_image);
        if ((indexes != (IndexPacket *) NULL) &&
            (rotate_indexes != (IndexPacket *) NULL))
          for (x=0; x < (long) image->columns; x++)
            rotate_indexes[image->columns-x-1]=indexes[x];
        for (x=0; x < (long) image->columns; x++)
          *--q=(*p++);
        if (!SyncImagePixels(rotate_image))
          break;
        if (QuantumTick(y,image->rows))
          if (!MagickMonitor(RotateImageText,y,image->rows,exception))
            break;
      }
      page.x=(long) (page.width-rotate_image->columns-page.x);
      page.y=(long) (page.height-rotate_image->rows-page.y);
      break;
    }
    case 3:
    {
      /*
        Rotate 270 degrees.
      */
      for (y=0; y < (long) image->rows; y++)
      {
        p=AcquireImagePixels(image,0,y,image->columns,1,exception);
        q=SetImagePixels(rotate_image,y,0,1,rotate_image->rows);
        if ((p == (const PixelPacket *) NULL) || (q == (PixelPacket *) NULL))
          break;
        q+=image->columns;
        for (x=0; x < (long) image->columns; x++)
          *--q=(*p++);
        indexes=GetIndexes(image);
        rotate_indexes=GetIndexes(rotate_image);
        if ((indexes != (IndexPacket *) NULL) &&
            (rotate_indexes != (IndexPacket *) NULL))
          for (x=0; x < (long) image->columns; x++)
            rotate_indexes[image->columns-x-1]=indexes[x];
        if (!SyncImagePixels(rotate_image))
          break;
        if (QuantumTick(y,image->rows))
          if (!MagickMonitor(RotateImageText,y,image->rows,exception))
            break;
      }
      Swap(page.width,page.height);
      Swap(page.x,page.y);
      page.y=(long) (page.height-rotate_image->rows-page.y);
      break;
    }
  }
  rotate_image->page=page;
  rotate_image->is_grayscale=image->is_grayscale;
  return(rotate_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   X S h e a r I m a g e                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Procedure XShearImage shears the image in the X direction with a shear angle
%  of 'degrees'.  Positive angles shear counter-clockwise (right-hand rule),
%  and negative angles shear clockwise.  Angles are measured relative to a
%  vertical Y-axis.  X shears will widen an image creating 'empty' triangles
%  on the left and right sides of the source image.
%
%  The format of the XShearImage method is:
%
%      void XShearImage(Image *image,const double degrees,
%        const unsigned long width,const unsigned long height,
%        const long x_offset,long y_offset)
%
%  A description of each parameter follows.
%
%    o image: The image.
%
%    o degrees: A double representing the shearing angle along the X axis.
%
%    o width, height, x_offset, y_offset: Defines a region of the image
%      to shear.
%
*/

static inline PixelPacket BlendComposite(const PixelPacket *p,
  const PixelPacket *q,const double alpha)
{
  double
    color;

  PixelPacket
    composite;

  color=((double) p->red*(MaxRGB-alpha)+q->red*alpha)/MaxRGB;
  composite.red=(Quantum)
    ((color < 0) ? 0 : (color > MaxRGB) ? MaxRGB : color+0.5);
  color=((double) p->green*(MaxRGB-alpha)+q->green*alpha)/MaxRGB;
  composite.green=(Quantum)
    ((color < 0) ? 0 : (color > MaxRGB) ? MaxRGB : color+0.5);
  color=((double) p->blue*(MaxRGB-alpha)+q->blue*alpha)/MaxRGB;
  composite.blue=(Quantum)
    ((color < 0) ? 0 : (color > MaxRGB) ? MaxRGB : color+0.5);
  composite.opacity=p->opacity;
  return(composite);
}

static void XShearImage(Image *image,const double degrees,
  const unsigned long width,const unsigned long height,const long x_offset,
  long y_offset)
{
#define XShearImageText  "  X Shear image...  "

  double
    alpha,
    displacement;

  enum {LEFT, RIGHT}
    direction;

  long
    step,
    y;

  PixelPacket
    pixel;

  register long
    i;

  unsigned int
    is_grayscale;

  register PixelPacket
    *p,
    *q;

  assert(image != (Image *) NULL);
  is_grayscale=image->is_grayscale;
  y_offset--;
  for (y=0; y < (long) height; y++)
  {
    y_offset++;
    displacement=degrees*(y-height/2.0);
    if (displacement == 0.0)
      continue;
    if (displacement > 0.0)
      direction=RIGHT;
    else
      {
        displacement*=(-1.0);
        direction=LEFT;
      }
    step=(long) floor(displacement);
    alpha=(double) MaxRGB*(displacement-step);
    if (alpha == 0.0)
      {
        /*
          No fractional displacement-- just copy.
        */
        switch (direction)
        {
          case LEFT:
          {
            /*
              Transfer pixels left-to-right.
            */
            if (step > x_offset)
              break;
            p=GetImagePixels(image,0,y_offset,image->columns,1);
            if (p == (PixelPacket *) NULL)
              break;
            p+=x_offset;
            q=p-step;
            (void) memcpy(q,p,width*sizeof(PixelPacket));
            q+=width;
            for (i=0; i < (long) step; i++)
              *q++=image->background_color;
            break;
          }
          case RIGHT:
          {
            /*
              Transfer pixels right-to-left.
            */
            p=GetImagePixels(image,0,y_offset,image->columns,1);
            if (p == (PixelPacket *) NULL)
              break;
            p+=x_offset+width;
            q=p+step;
            for (i=0; i < (long) width; i++)
              *--q=(*--p);
            for (i=0; i < (long) step; i++)
              *--q=image->background_color;
            break;
          }
        }
        if (!SyncImagePixels(image))
          break;
        continue;
      }
    /*
      Fractional displacement.
    */
    step++;
    pixel=image->background_color;
    switch (direction)
    {
      case LEFT:
      {
        /*
          Transfer pixels left-to-right.
        */
        if (step > x_offset)
          break;
        p=GetImagePixels(image,0,y_offset,image->columns,1);
        if (p == (PixelPacket *) NULL)
          break;
        p+=x_offset;
        q=p-step;
        for (i=0; i < (long) width; i++)
        {
          if ((x_offset+i) < step)
            {
              pixel=(*++p);
              q++;
              continue;
            }
          *q++=BlendComposite(&pixel,p,alpha);
          pixel=(*p++);
        }
        *q++=BlendComposite(&pixel,&image->background_color,alpha);
        for (i=0; i < (step-1); i++)
          *q++=image->background_color;
        break;
      }
      case RIGHT:
      {
        /*
          Transfer pixels right-to-left.
        */
        p=GetImagePixels(image,0,y_offset,image->columns,1);
        if (p == (PixelPacket *) NULL)
          break;
        p+=x_offset+width;
        q=p+step;
        for (i=0; i < (long) width; i++)
        {
          p--;
          q--;
          if ((x_offset+width+step-i) >= image->columns)
            continue;
          *q=BlendComposite(&pixel,p,alpha);
          pixel=(*p);
        }
        *--q=BlendComposite(&pixel,&image->background_color,alpha);
        for (i=0; i < (step-1); i++)
          *--q=image->background_color;
        break;
      }
    }
    if (!SyncImagePixels(image))
      break;
    if (QuantumTick(y,height))
      if (!MagickMonitor(XShearImageText,y,height,&image->exception))
        break;
  }
  if (is_grayscale && IsGray(image->background_color))
    image->is_grayscale=True;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   Y S h e a r I m a g e                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Procedure YShearImage shears the image in the Y direction with a shear
%  angle of 'degrees'.  Positive angles shear counter-clockwise (right-hand
%  rule), and negative angles shear clockwise.  Angles are measured relative
%  to a horizontal X-axis.  Y shears will increase the height of an image
%  creating 'empty' triangles on the top and bottom of the source image.
%
%  The format of the YShearImage method is:
%
%      void YShearImage(Image *image,const double degrees,
%        const unsigned long width,const unsigned long height,long x_offset,
%        const long y_offset)
%
%  A description of each parameter follows.
%
%    o image: The image.
%
%    o degrees: A double representing the shearing angle along the Y axis.
%
%    o width, height, x_offset, y_offset: Defines a region of the image
%      to shear.
%
%
*/
static void YShearImage(Image *image,const double degrees,
  const unsigned long width,const unsigned long height,long x_offset,
  const long y_offset)
{
#define YShearImageText  "  Y Shear image...  "

  double
    alpha,
    displacement;

  enum {UP, DOWN}
    direction;

  long
    step,
    y;

  register PixelPacket
    *p,
    *q;

  register long
    i;

  unsigned int
    is_grayscale;

  PixelPacket
    pixel;

  assert(image != (Image *) NULL);
  is_grayscale=image->is_grayscale;
  x_offset--;
  for (y=0; y < (long) width; y++)
  {
    x_offset++;
    displacement=degrees*(y-width/2.0);
    if (displacement == 0.0)
      continue;
    if (displacement > 0.0)
      direction=DOWN;
    else
      {
        displacement*=(-1.0);
        direction=UP;
      }
    step=(long) floor(displacement);
    alpha=(double) MaxRGB*(displacement-step);
    if (alpha == 0.0)
      {
        /*
          No fractional displacement-- just copy the pixels.
        */
        switch (direction)
        {
          case UP:
          {
            /*
              Transfer pixels top-to-bottom.
            */
            if (step > y_offset)
              break;
            p=GetImagePixels(image,x_offset,0,1,image->rows);
            if (p == (PixelPacket *) NULL)
              break;
            p+=y_offset;
            q=p-step;
            (void) memcpy(q,p,height*sizeof(PixelPacket));
            q+=height;
            for (i=0; i < (long) step; i++)
              *q++=image->background_color;
            break;
          }
          case DOWN:
          {
            /*
              Transfer pixels bottom-to-top.
            */
            p=GetImagePixels(image,x_offset,0,1,image->rows);
            if (p == (PixelPacket *) NULL)
              break;
            p+=y_offset+height;
            q=p+step;
            for (i=0; i < (long) height; i++)
              *--q=(*--p);
            for (i=0; i < (long) step; i++)
              *--q=image->background_color;
            break;
          }
        }
        if (!SyncImagePixels(image))
          break;
        continue;
      }
    /*
      Fractional displacment.
    */
    step++;
    pixel=image->background_color;
    switch (direction)
    {
      case UP:
      {
        /*
          Transfer pixels top-to-bottom.
        */
        if (step > y_offset)
          break;
        p=GetImagePixels(image,x_offset,0,1,image->rows);
        if (p == (PixelPacket *) NULL)
          break;
        p+=y_offset;
        q=p-step;
        for (i=0; i < (long) height; i++)
        {
          if ((y_offset+i) < step)
            {
              pixel=(*++p);
              q++;
              continue;
            }
          *q++=BlendComposite(&pixel,p,alpha);
          pixel=(*p++);
        }
        *q++=BlendComposite(&pixel,&image->background_color,alpha);
        for (i=0; i < (step-1); i++)
          *q++=image->background_color;
        break;
      }
      case DOWN:
      {
        /*
          Transfer pixels bottom-to-top.
        */
        p=GetImagePixels(image,x_offset,0,1,image->rows);
        if (p == (PixelPacket *) NULL)
          break;
        p+=y_offset+height;
        q=p+step;
        for (i=0; i < (long) height; i++)
        {
          p--;
          q--;
          if ((y_offset+height+step-i) >= image->rows)
            continue;
          *q=BlendComposite(&pixel,p,alpha);
          pixel=(*p);
        }
        *--q=BlendComposite(&pixel,&image->background_color,alpha);
        for (i=0; i < (step-1); i++)
          *--q=image->background_color;
        break;
      }
    }
    if (!SyncImagePixels(image))
      break;
    if (QuantumTick(y,width))
      if (!MagickMonitor(YShearImageText,y,width,&image->exception))
        break;
  }
  if (is_grayscale && IsGray(image->background_color))
    image->is_grayscale=True;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R o t a t e I m a g e                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method RotateImage creates a new image that is a rotated copy of an
%  existing one.  Positive angles rotate counter-clockwise (right-hand rule),
%  while negative angles rotate clockwise.  Rotated images are usually larger
%  than the originals and have 'empty' triangular corners.  X axis.  Empty
%  triangles left over from shearing the image are filled with the color
%  specified by the image background_color.  RotateImage allocates the memory
%  necessary for the new Image structure and returns a pointer to the new
%  image.
%
%  Method RotateImage is based on the paper "A Fast Algorithm for General
%  Raster Rotatation" by Alan W. Paeth.  RotateImage is adapted from a similar
%  method based on the Paeth paper written by Michael Halle of the Spatial
%  Imaging Group, MIT Media Lab.
%
%  The format of the RotateImage method is:
%
%      Image *RotateImage(const Image *image,const double degrees,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows.
%
%    o status: Method RotateImage returns a pointer to the image after
%      rotating.  A null image is returned if there is a memory shortage.
%
%    o image: The image;  returned from
%      ReadImage.
%
%    o degrees: Specifies the number of degrees to rotate the image.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport Image *RotateImage(const Image *image,const double degrees,
  ExceptionInfo *exception)
{
  double
    angle;

  Image
    *integral_image,
    *rotate_image;

  long
    x_offset,
    y_offset;

  PointInfo
    shear;

  RectangleInfo
    border_info;

  unsigned long
    height,
    rotations,
    width,
    y_width;

  /*
    Adjust rotation angle.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  angle=degrees;
  while (angle < -45.0)
    angle+=360.0;
  for (rotations=0; angle > 45.0; rotations++)
    angle-=90.0;
  rotations%=4;
  /*
    Calculate shear equations.
  */
  integral_image=IntegralRotateImage(image,rotations,exception);
  if (integral_image == (Image *) NULL)
    ThrowImageException(ResourceLimitError,"MemoryAllocationFailed",
      "UnableToRotateImage");
  shear.x=(-tan(DegreesToRadians(angle)/2.0));
  shear.y=sin(DegreesToRadians(angle));
  if ((shear.x == 0.0) || (shear.y == 0.0))
    return(integral_image);
  /*
    Compute image size.
  */
  width=image->columns;
  height=image->rows;
  if ((rotations == 1) || (rotations == 3))
    {
      width=image->rows;
      height=image->columns;
    }
  x_offset=(long) ceil(fabs(2.0*height*shear.y)-0.5);
  y_width=(unsigned long) floor(fabs(height*shear.x)+width+0.5);
  y_offset=(long) ceil(fabs(y_width*shear.y)-0.5);
  /*
    Surround image with a border.
  */
  integral_image->border_color=integral_image->background_color;
  border_info.width=x_offset;
  border_info.height=y_offset;
  rotate_image=BorderImage(integral_image,&border_info,exception);
  DestroyImage(integral_image);
  if (rotate_image == (Image *) NULL)
    ThrowImageException(ResourceLimitError,"MemoryAllocationFailed",
      "UnableToRotateImage");
  /*
    Rotate the image.
  */
  rotate_image->storage_class=DirectClass;
  rotate_image->matte|=rotate_image->background_color.opacity != OpaqueOpacity;
  XShearImage(rotate_image,shear.x,width,height,x_offset,
    (long) (rotate_image->rows-height)/2);
  YShearImage(rotate_image,shear.y,y_width,height,
    (long) (rotate_image->columns-y_width)/2,y_offset);
  XShearImage(rotate_image,shear.x,y_width,rotate_image->rows,
    (long) (rotate_image->columns-y_width)/2,0);
  CropToFitImage(&rotate_image,shear.x,shear.y,width,height,True,exception);
  rotate_image->page.width=0;
  rotate_image->page.height=0;
  return(rotate_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S h e a r I m a g e                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method ShearImage creates a new image that is a shear_image copy of an
%  existing one.  Shearing slides one edge of an image along the X or Y
%  axis, creating a parallelogram.  An X direction shear slides an edge
%  along the X axis, while a Y direction shear slides an edge along the Y
%  axis.  The amount of the shear is controlled by a shear angle.  For X
%  direction shears, x_shear is measured relative to the Y axis, and
%  similarly, for Y direction shears y_shear is measured relative to the
%  X axis.  Empty triangles left over from shearing the image are filled
%  with the color defined by the pixel at location (0,0).  ShearImage
%  allocates the memory necessary for the new Image structure and returns
%  a pointer to the new image.
%
%  Method ShearImage is based on the paper "A Fast Algorithm for General
%  Raster Rotatation" by Alan W. Paeth.
%
%  The format of the ShearImage method is:
%
%      Image *ShearImage(const Image *image,const double x_shear,
%        const double y_shear,ExceptionInfo *exception)
%
%  A description of each parameter follows.
%
%    o status: Method ShearImage returns a pointer to the image after
%      rotating.  A null image is returned if there is a memory shortage.
%
%    o image: The image;  returned from
%      ReadImage.
%
%    o x_shear, y_shear: Specifies the number of degrees to shear the image.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport Image *ShearImage(const Image *image,const double x_shear,
  const double y_shear,ExceptionInfo *exception)
{
  Image
    *integral_image,
    *shear_image;

  long
    x_offset,
    y_offset;

  PointInfo
    shear;

  RectangleInfo
    border_info;

  unsigned long
    y_width;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  if ((x_shear == 180.0) || (y_shear == 180.0))
    ThrowImageException(ImageError,"UnableToShearImage","AngleIsDiscontinuous");
  /*
    Initialize shear angle.
  */
  integral_image=IntegralRotateImage(image,0,exception);
  if (integral_image == (Image *) NULL)
    ThrowImageException(ResourceLimitError,"MemoryAllocationFailed",
      "UnableToShearImage");
  shear.x=(-tan(DegreesToRadians(x_shear)/2.0));
  shear.y=sin(DegreesToRadians(y_shear));
  if ((shear.x == 0.0) || (shear.y == 0.0))
    return(integral_image);
  /*
    Compute image size.
  */
  x_offset=(long) ceil(fabs(2.0*image->rows*shear.x)-0.5);
  y_width=(unsigned long) floor(fabs(image->rows*shear.x)+image->columns+0.5);
  y_offset=(long) ceil(fabs(y_width*shear.y)-0.5);
  /*
    Surround image with border.
  */
  integral_image->border_color=integral_image->background_color;
  border_info.width=x_offset;
  border_info.height=y_offset;
  shear_image=BorderImage(integral_image,&border_info,exception);
  if (shear_image == (Image *) NULL)
    ThrowImageException(ResourceLimitError,"MemoryAllocationFailed",
      "UnableToShearImage");
  DestroyImage(integral_image);
  /*
    Shear the image.
  */
  shear_image->storage_class=DirectClass;
  shear_image->matte|=shear_image->background_color.opacity != OpaqueOpacity;
  XShearImage(shear_image,shear.x,image->columns,image->rows,x_offset,
    (long) (shear_image->rows-image->rows)/2);
  YShearImage(shear_image,shear.y,y_width,image->rows,
    (long) (shear_image->columns-y_width)/2,y_offset);
  CropToFitImage(&shear_image,shear.x,shear.y,image->columns,image->rows,
    False,exception);
  shear_image->page.width=0;
  shear_image->page.height=0;
  return(shear_image);
}
