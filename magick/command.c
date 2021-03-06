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
%                                                                             %
%               CCCC   OOO   M   M  M   M   AAA   N   N   DDDD                %
%              C      O   O  MM MM  MM MM  A   A  NN  N   D   D               %
%              C      O   O  M M M  M M M  AAAAA  N N N   D   D               %
%              C      O   O  M   M  M   M  A   A  N  NN   D   D               %
%               CCCC   OOO   M   M  M   M  A   A  N   N   DDDD                %
%                                                                             %
%                                                                             %
%                        Image Command Methods                                %
%                                                                             %
%                           Software Design                                   %
%                             John Cristy                                     %
%                            Bill Radcliffe                                   %
%                              July 2002                                      %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%
%
*/

/*
  Include declarations.
*/
#include "magick/studio.h"
#include "magick/attribute.h"
#include "magick/cache.h"
#include "magick/color.h"
#include "magick/constitute.h"
#include "magick/command.h"
#include "magick/composite.h"
#include "magick/decorate.h"
#include "magick/delegate.h"
#include "magick/effect.h"
#include "magick/enhance.h"
#include "magick/fx.h"
#include "magick/gem.h"
#include "magick/log.h"
#include "magick/magic.h"
#include "magick/magick.h"
#include "magick/module.h"
#include "magick/monitor.h"
#include "magick/montage.h"
#include "magick/paint.h"
#include "magick/quantize.h"
#include "magick/render.h"
#include "magick/resize.h"
#include "magick/resource.h"
#include "magick/shear.h"
#include "magick/transform.h"
#include "magick/utility.h"
#include "magick/version.h"
#include "magick/xwindow.h"

/*
  Typedef declarations.
*/
typedef struct _CompositeOptions
{
  char
    *displace_geometry,
    *geometry,
    *unsharp_geometry,
    *watermark_geometry;

  CompositeOperator
    compose;

  GravityType
    gravity;

  double
    dissolve;

  long
    stegano;

  unsigned int
    stereo,
    tile;
} CompositeOptions;

/*
  Liberate an allocated argument list
*/
static void LiberateArgumentList(const int argc,char **argv)
{
  unsigned int
    i;

  for (i=0; i< argc; i++)
    if (argv[i])
      LiberateMemory((void **)&argv[i]);
  LiberateMemory((void **) &argv);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  C o m p o s i t e I m a g e C o m m a n d                                  %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  CompositeImageCommand() reads one or more images and an optional mask and
%  composites them into a new image.
%
%  The format of the CompositeImageCommand method is:
%
%      unsigned int CompositeImageCommand(ImageInfo *image_info,const int argc,
%        char **argv,char **metadata,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: The image info.
%
%    o argc: The number of elements in the argument vector.
%
%    o argv: A text array containing the command line arguments.
%
%    o metadata: any metadata is returned here.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
static unsigned int CompositeImageList(ImageInfo *image_info,Image **image,
  Image *composite_image,Image *mask_image,CompositeOptions *option_info,
  ExceptionInfo *exception)
{
  long
    x,
    y;

  unsigned int
    matte,
    status;

  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  assert(image != (Image **) NULL);
  assert((*image)->signature == MagickSignature);
  assert(exception != (ExceptionInfo *) NULL);
  status=True;
  if (composite_image != (Image *) NULL)
    {
      assert(composite_image->signature == MagickSignature);
      if (mask_image != (Image *) NULL)
        {
          assert(mask_image->signature == MagickSignature);
          SetImageType(composite_image,TrueColorMatteType);
          if (!composite_image->matte)
            SetImageOpacity(composite_image,OpaqueOpacity);
          status&=CompositeImage(composite_image,CopyOpacityCompositeOp,
            mask_image,0,0);
          if (status == False)
            (void) CatchImageException(composite_image);
        }
      if (option_info->compose == DissolveCompositeOp)
        {
          register PixelPacket
            *q;

          /*
            Create mattes for dissolve.
          */
          if (!composite_image->matte)
            SetImageOpacity(composite_image,OpaqueOpacity);
          for (y=0; y < (long) composite_image->rows; y++)
          {
            q=GetImagePixels(composite_image,0,y,composite_image->columns,1);
            if (q == (PixelPacket *) NULL)
              break;
            for (x=0; x < (long) composite_image->columns; x++)
            {
              q->opacity=(Quantum)
                (((MaxRGB-q->opacity)*option_info->dissolve)/100.0);
              q++;
            }
            if (!SyncImagePixels(composite_image))
              break;
          }
        }
      if (option_info->compose == DisplaceCompositeOp)
        (void) CloneString(&composite_image->geometry,
          option_info->displace_geometry);
      if (option_info->compose == ModulateCompositeOp)
        (void) CloneString(&composite_image->geometry,
          option_info->watermark_geometry);
      if (option_info->compose == ThresholdCompositeOp)
        (void) CloneString(&composite_image->geometry,
          option_info->unsharp_geometry);
      /*
        Composite image.
      */
      matte=(*image)->matte;
      if (option_info->stegano != 0)
        {
          Image
            *stegano_image;

          (*image)->offset=option_info->stegano-1;
          stegano_image=SteganoImage(*image,composite_image,exception);
          if (stegano_image != (Image *) NULL)
            {
              DestroyImageList(*image);
              *image=stegano_image;
            }
        }
      else
        if (option_info->stereo)
          {
            Image
              *stereo_image;

            stereo_image=StereoImage(*image,composite_image,exception);
            if (stereo_image != (Image *) NULL)
              {
                DestroyImageList(*image);
                *image=stereo_image;
              }
          }
        else
          if (option_info->tile)
            {
              /*
                Tile the composite image.
              */
              for (y=0; y < (long) (*image)->rows; y+=composite_image->rows)
                for (x=0; x < (long) (*image)->columns; x+=composite_image->columns)
                {
                  status&=CompositeImage(*image,option_info->compose,
                    composite_image,x,y);
                  (void) CatchImageException(*image);
                }
            }
          else
            {
              char
                composite_geometry[MaxTextExtent];

              RectangleInfo
                geometry;

              /*
                Digitally composite image.
              */
              geometry.x=0;
              geometry.y=0;
              (void) GetGeometry(option_info->geometry,&geometry.x,&geometry.y,
                &geometry.width,&geometry.height);
              FormatString(composite_geometry,"%lux%lu%+ld%+ld",
                composite_image->columns,composite_image->rows,geometry.x,
                geometry.y);
              (*image)->gravity=option_info->gravity;
              (void) GetImageGeometry(*image,composite_geometry,False,
                &geometry);
              status&=CompositeImage(*image,option_info->compose,
                composite_image,geometry.x,geometry.y);
              (void) CatchImageException(*image);
            }
      (*image)->matte=matte;
    }
  return(status);
}

static void LiberateCompositeOptions(CompositeOptions *option_info)
{
  if (option_info->displace_geometry != (char *) NULL)
    LiberateMemory((void **) &(option_info->displace_geometry));
  if (option_info->geometry != (char *) NULL)
    LiberateMemory((void **) &(option_info->geometry));
  if (option_info->unsharp_geometry != (char *) NULL)
    LiberateMemory((void **) &(option_info->unsharp_geometry));
  if (option_info->watermark_geometry != (char *) NULL)
    LiberateMemory((void **) &(option_info->watermark_geometry));
}

#define NotInitialized  (unsigned int) (~0)
#define ThrowCompositeException(code,reason,description) \
{ \
  LiberateCompositeOptions(&option_info); \
  DestroyImageList(image); \
  DestroyImageList(composite_image); \
  DestroyImageList(mask_image); \
  ThrowException(exception,code,reason,description); \
  LiberateArgumentList(argc,argv); \
  return(False); \
}
MagickExport unsigned int CompositeImageCommand(ImageInfo *image_info,
  int argc,char **argv,char **metadata,ExceptionInfo *exception)
{
  char
    *filename,
    *format,
    *option;

  CompositeOptions
    option_info;

  double
    sans;

  Image
    *composite_image,
    *image,
    *mask_image;

  long
    j,
    x;

  register long
    i;

  unsigned int
    status;

  /*
    Set default.
  */
  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  assert(exception != (ExceptionInfo *) NULL);
  status=True;

  status=ExpandFilenames(&argc,&argv);
  if (status == False)
    MagickFatalError(ResourceLimitFatalError,"MemoryAllocationFailed",
    (char *) NULL);

  memset(&option_info,0,sizeof(CompositeOptions));
  option_info.dissolve=0.0;
  option_info.compose=OverCompositeOp;
  composite_image=NewImageList();
  option_info.displace_geometry=(char *) NULL;
  filename=(char *) NULL;
  format=(char *) NULL;
  option_info.geometry=(char *) NULL;
  image=NewImageList();
  (void) strncpy(image_info->filename,argv[argc-1],MaxTextExtent-1);
  (void) SetImageInfo(image_info,True,exception);
  mask_image=NewImageList();
  option_info.stegano=0;
  option_info.stereo=False;
  option_info.tile=False;
  option_info.watermark_geometry=(char *) NULL;
  option_info.unsharp_geometry=(char *) NULL;
  status=True;
  /*
    Check command syntax.
  */
  if (argc < 2 || ((argc < 3) && (LocaleCompare("-help",argv[1]) == 0 ||
      LocaleCompare("-?",argv[1]) == 0)))
    {
      CompositeUsage();
      ThrowCompositeException(OptionError,"UsageError",NULL);
    }
  j=1;
  for (i=1; i < (argc-1); i++)
  {
    option=argv[i];
    if ((strlen(option) == 1) || ((*option != '-') && (*option != '+')))
      {
        /*
          Read input images.
        */
        filename=argv[i];
        (void) strncpy(image_info->filename,filename,MaxTextExtent-1);
        if (composite_image == (Image *) NULL)
          {
            composite_image=ReadImage(image_info,exception);
            if (composite_image != (Image *) NULL)
              {
                status&=MogrifyImages(image_info,i-j,argv+j,&composite_image);
                (void) CatchImageException(composite_image);
              }
            j=i+1;
            continue;
          }
        if (mask_image != (Image *) NULL)
          ThrowCompositeException(OptionError,"InputImagesAlreadySpecified",
            filename);
        if (image == (Image *) NULL)
          {
            image=ReadImage(image_info,exception);
            if (image != (Image *) NULL)
              {
                status&=MogrifyImages(image_info,i-j,argv+j,&image);
                (void) CatchImageException(image);
              }
            j=i+1;
            continue;
          }
        mask_image=ReadImage(image_info,exception);
        status&=mask_image != (Image *) NULL;
        if (mask_image != (Image *) NULL)
          {
            status&=MogrifyImages(image_info,i-j,argv+j,&mask_image);
            (void) CatchImageException(mask_image);
          }
        j=i+1;
        continue;
      }
    switch(*(option+1))
    {
      case 'a':
      {
        if (LocaleCompare("affine",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%lf",&sans))
                  ThrowCompositeException(OptionError,"MissingAffineMatrix",
                    option);
              }
            break;
          }
        if (LocaleCompare("authenticate",option+1) == 0)
          {
            (void) CloneString(&image_info->authenticate,(char *) NULL);
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowCompositeException(OptionError,"MissingPassword",option);
                (void) CloneString(&image_info->authenticate,argv[i]);
              }
            break;
          }
        ThrowCompositeException(OptionError,"UnrecognizedOption",option)
      }
      case 'b':
      {
        if (LocaleCompare("background",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowCompositeException(OptionError,"MissingBackgroundColor",
                    option);
                (void) QueryColorDatabase(argv[i],&image_info->background_color,
                  exception);
              }
            break;
          }
        if (LocaleCompare("blue-primary",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  ThrowCompositeException(OptionError,"MissingPoint",option);
              }
            break;
          }
        ThrowCompositeException(OptionError,"UnrecognizedOption",option)
      }
      case 'c':
      {
        if (LocaleCompare("cache",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  ThrowCompositeException(OptionError,"MissingThreshold",
                    option);
                SetMagickResourceLimit(MemoryResource,atol(argv[i]));
                SetMagickResourceLimit(MapResource,2*atol(argv[i]));
              }
            break;
          }
        if (LocaleCompare("colors",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  ThrowCompositeException(OptionError,"MissingColors",option);
              }
            break;
          }
        if (LocaleCompare("colorspace",option+1) == 0)
          {
            image_info->colorspace=RGBColorspace;
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowCompositeException(OptionError,"MissingType",option);
                option=argv[i];
                image_info->colorspace=UndefinedColorspace;
                if (LocaleCompare("cmyk",option) == 0)
                  image_info->colorspace=CMYKColorspace;
                if (LocaleCompare("gray",option) == 0)
                  image_info->colorspace=GRAYColorspace;
                if (LocaleCompare("hsl",option) == 0)
                  image_info->colorspace=HSLColorspace;
                if (LocaleCompare("hwb",option) == 0)
                  image_info->colorspace=HWBColorspace;
                if (LocaleCompare("ohta",option) == 0)
                  image_info->colorspace=OHTAColorspace;
                if (LocaleCompare("rgb",option) == 0)
                  image_info->colorspace=RGBColorspace;
                if (LocaleCompare("srgb",option) == 0)
                  image_info->colorspace=sRGBColorspace;
                if (LocaleCompare("transparent",option) == 0)
                  image_info->colorspace=TransparentColorspace;
                if (LocaleCompare("xyz",option) == 0)
                  image_info->colorspace=XYZColorspace;
                if (LocaleCompare("ycbcr",option) == 0)
                  image_info->colorspace=YCbCrColorspace;
                if (LocaleCompare("ycc",option) == 0)
                  image_info->colorspace=YCCColorspace;
                if (LocaleCompare("yiq",option) == 0)
                  image_info->colorspace=YIQColorspace;
                if (LocaleCompare("ypbpr",option) == 0)
                  image_info->colorspace=YPbPrColorspace;
                if (LocaleCompare("yuv",option) == 0)
                  image_info->colorspace=YUVColorspace;
                if (image_info->colorspace == UndefinedColorspace)
                  ThrowCompositeException(OptionError,"UnrecognizedColorspace",
                    option);
              }
            break;
          }
        if (LocaleCompare("comment",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowCompositeException(OptionError,"MissingComment",option);
              }
            break;
          }
        if (LocaleCompare("compose",option+1) == 0)
          {
            option_info.compose=CopyCompositeOp;
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowCompositeException(OptionError,"MissingType",option);
                option=argv[i];
                option_info.compose=UndefinedCompositeOp;
                if (LocaleCompare("Over",option) == 0)
                  option_info.compose=OverCompositeOp;
                if (LocaleCompare("In",option) == 0)
                  option_info.compose=InCompositeOp;
                if (LocaleCompare("Out",option) == 0)
                  option_info.compose=OutCompositeOp;
                if (LocaleCompare("Atop",option) == 0)
                  option_info.compose=AtopCompositeOp;
                if (LocaleCompare("Xor",option) == 0)
                  option_info.compose=XorCompositeOp;
                if (LocaleCompare("Plus",option) == 0)
                  option_info.compose=PlusCompositeOp;
                if (LocaleCompare("Minus",option) == 0)
                  option_info.compose=MinusCompositeOp;
                if (LocaleCompare("Add",option) == 0)
                  option_info.compose=AddCompositeOp;
                if (LocaleCompare("Subtract",option) == 0)
                  option_info.compose=SubtractCompositeOp;
                if (LocaleCompare("Difference",option) == 0)
                  option_info.compose=DifferenceCompositeOp;
                if (LocaleCompare("Multiply",option) == 0)
                  option_info.compose=MultiplyCompositeOp;
                if (LocaleCompare("Bumpmap",option) == 0)
                  option_info.compose=BumpmapCompositeOp;
                if (LocaleCompare("Copy",option) == 0)
                  option_info.compose=CopyCompositeOp;
                if (LocaleCompare("CopyRed",option) == 0)
                  option_info.compose=CopyRedCompositeOp;
                if (LocaleCompare("CopyGreen",option) == 0)
                  option_info.compose=CopyGreenCompositeOp;
                if (LocaleCompare("CopyBlue",option) == 0)
                  option_info.compose=CopyBlueCompositeOp;
                if (LocaleCompare("CopyOpacity",option) == 0)
                  option_info.compose=CopyOpacityCompositeOp;
                if (LocaleCompare("Clear",option) == 0)
                  option_info.compose=ClearCompositeOp;
                if (option_info.compose == UndefinedCompositeOp)
                  ThrowCompositeException(OptionError,"UnrecognizedComposeOperator",
                    option);
              }
            break;
          }
        if (LocaleCompare("compress",option+1) == 0)
          {
            image_info->compression=NoCompression;
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowCompositeException(OptionError,"MissingType",option);
                option=argv[i];
                image_info->compression=UndefinedCompression;
                if (LocaleCompare("None",option) == 0)
                  image_info->compression=NoCompression;
                if (LocaleCompare("BZip",option) == 0)
                  image_info->compression=BZipCompression;
                if (LocaleCompare("Fax",option) == 0)
                  image_info->compression=FaxCompression;
                if (LocaleCompare("Group4",option) == 0)
                  image_info->compression=Group4Compression;
                if (LocaleCompare("JPEG",option) == 0)
                  image_info->compression=JPEGCompression;
                if (LocaleCompare("Lossless",option) == 0)
                  image_info->compression=LosslessJPEGCompression;
                if (LocaleCompare("LZW",option) == 0)
                  image_info->compression=LZWCompression;
                if (LocaleCompare("RLE",option) == 0)
                  image_info->compression=RLECompression;
                if (LocaleCompare("Zip",option) == 0)
                  image_info->compression=ZipCompression;
                if (image_info->compression == UndefinedCompression)
                  ThrowCompositeException(OptionError,"UnrecognizedImageCompression",
                    option);
              }
            break;
          }
        ThrowCompositeException(OptionError,"UnrecognizedOption",option)
      }
      case 'd':
      {
        if (LocaleCompare("debug",option+1) == 0)
          {
            (void) SetLogEventMask("None");
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowCompositeException(OptionError,"MissingEventMask",
                    option);
                (void) SetLogEventMask(argv[i]);
              }
            break;
          }
        if (LocaleCompare("density",option+1) == 0)
          {
            (void) CloneString(&image_info->density,(char *) NULL);
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !IsGeometry(argv[i]))
                  ThrowCompositeException(OptionError,"MissingGeometry",option);
                (void) CloneString(&image_info->density,argv[i]);
              }
            break;
          }
        if (LocaleCompare("depth",option+1) == 0)
          {
            image_info->depth=QuantumDepth;
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  ThrowCompositeException(OptionError,"MissingImageDepth",
                    option);
                image_info->depth=atol(argv[i]);
              }
            break;
          }
        if (LocaleCompare("displace",option+1) == 0)
          {
            (void) CloneString(&option_info.displace_geometry,(char *) NULL);
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%lf",&sans))
                  ThrowCompositeException(OptionError,"MissingGeometry",option);
                (void) CloneString(&option_info.displace_geometry,argv[i]);
                option_info.compose=DisplaceCompositeOp;
              }
            break;
          }
        if (LocaleCompare("display",option+1) == 0)
          {
            (void) CloneString(&image_info->server_name,(char *) NULL);
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowCompositeException(OptionError,"MissingServerName",
                    option);
                (void) CloneString(&image_info->server_name,argv[i]);
              }
            break;
          }
        if (LocaleCompare("dispose",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowCompositeException(OptionError,"MissingDisposeMethod",
                    option);
                option=argv[i];
                if ((LocaleCompare("0",option) != 0) &&
                    (LocaleCompare("1",option) != 0) &&
                    (LocaleCompare("2",option) != 0) &&
                    (LocaleCompare("3",option) != 0) &&
                    (LocaleCompare("Undefined",option) != 0) &&
                    (LocaleCompare("None",option) != 0) &&
                    (LocaleCompare("Background",option) != 0) &&
                    (LocaleCompare("Previous",option) != 0))
                  ThrowCompositeException(OptionError,"UnrecognizedDisposeMethod",
                    option);
              }
            break;
          }
        if (LocaleCompare("dissolve",option+1) == 0)
          {
            option_info.dissolve=0.0;
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  ThrowCompositeException(OptionError,"MissingDissolveValue",
                    option);
                option_info.dissolve=atof(argv[i]);
                option_info.compose=DissolveCompositeOp;
              }
            break;
          }
        if (LocaleCompare("dither",option+1) == 0)
          {
            image_info->dither=(*option == '-');
            break;
          }
        ThrowCompositeException(OptionError,"UnrecognizedOption",option)
      }
      case 'e':
      {
        if (LocaleCompare("encoding",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowCompositeException(OptionError,"MissingEnodingType",
                    option);
              }
            break;
          }
        if (LocaleCompare("endian",option+1) == 0)
          {
            image_info->endian=UndefinedEndian;
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowCompositeException(OptionError,"MissingType",option);
                option=argv[i];
                image_info->endian=UndefinedEndian;
                if (LocaleCompare("LSB",option) == 0)
                  image_info->endian=LSBEndian;
                if (LocaleCompare("MSB",option) == 0)
                  image_info->endian=MSBEndian;
                if (image_info->endian == UndefinedEndian)
                  ThrowCompositeException(OptionError,"UnrecognizedEndianType",
                    option);
              }
            break;
          }
        ThrowCompositeException(OptionError,"UnrecognizedOption",option)
      }
      case 'f':
      {
        if (LocaleCompare("filter",option+1) == 0)
          {
            if (*option == '-')
              {
                FilterTypes
                  filter;

                i++;
                if (i == argc)
                  ThrowCompositeException(OptionError,"MissingType",option);
                option=argv[i];
                filter=UndefinedFilter;
                if (LocaleCompare("Point",option) == 0)
                  filter=PointFilter;
                if (LocaleCompare("Box",option) == 0)
                  filter=BoxFilter;
                if (LocaleCompare("Triangle",option) == 0)
                  filter=TriangleFilter;
                if (LocaleCompare("Hermite",option) == 0)
                  filter=HermiteFilter;
                if (LocaleCompare("Hanning",option) == 0)
                  filter=HanningFilter;
                if (LocaleCompare("Hamming",option) == 0)
                  filter=HammingFilter;
                if (LocaleCompare("Blackman",option) == 0)
                  filter=BlackmanFilter;
                if (LocaleCompare("Gaussian",option) == 0)
                  filter=GaussianFilter;
                if (LocaleCompare("Quadratic",option) == 0)
                  filter=QuadraticFilter;
                if (LocaleCompare("Cubic",option) == 0)
                  filter=CubicFilter;
                if (LocaleCompare("Catrom",option) == 0)
                  filter=CatromFilter;
                if (LocaleCompare("Mitchell",option) == 0)
                  filter=MitchellFilter;
                if (LocaleCompare("Lanczos",option) == 0)
                  filter=LanczosFilter;
                if (LocaleCompare("Bessel",option) == 0)
                  filter=BesselFilter;
                if (LocaleCompare("Sinc",option) == 0)
                  filter=SincFilter;
                if (filter == UndefinedFilter)
                  ThrowCompositeException(OptionError,"UnrecognizedImageFilter",
                    option);
              }
            break;
          }
        if (LocaleCompare("font",option+1) == 0)
          {
            (void) CloneString(&image_info->font,(char *) NULL);
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowCompositeException(OptionError,"MissingFontName",option);
                (void) CloneString(&image_info->font,argv[i]);
              }
            break;
          }
        if (LocaleCompare("format",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowCompositeException(OptionError,"MissingFormatSpecifier",
                    option);
                format=argv[i];
              }
            break;
          }
        ThrowCompositeException(OptionError,"UnrecognizedOption",option)
      }
      case 'g':
      {
        if (LocaleCompare("geometry",option+1) == 0)
          {
            (void) CloneString(&option_info.geometry,(char *) NULL);
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !IsGeometry(argv[i]))
                  ThrowCompositeException(OptionError,"MissingGeometry",option);
                (void) CloneString(&option_info.geometry,argv[i]);
              }
            break;
          }
        if (LocaleCompare("gravity",option+1) == 0)
          {
            option_info.gravity=ForgetGravity;
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowCompositeException(OptionError,"MissingType",option);
                option=argv[i];
                if (LocaleCompare("Forget",option) == 0)
                  option_info.gravity=ForgetGravity;
                if (LocaleCompare("NorthWest",option) == 0)
                  option_info.gravity=NorthWestGravity;
                if (LocaleCompare("North",option) == 0)
                  option_info.gravity=NorthGravity;
                if (LocaleCompare("NorthEast",option) == 0)
                  option_info.gravity=NorthEastGravity;
                if (LocaleCompare("West",option) == 0)
                  option_info.gravity=WestGravity;
                if (LocaleCompare("Center",option) == 0)
                  option_info.gravity=CenterGravity;
                if (LocaleCompare("East",option) == 0)
                  option_info.gravity=EastGravity;
                if (LocaleCompare("SouthWest",option) == 0)
                  option_info.gravity=SouthWestGravity;
                if (LocaleCompare("South",option) == 0)
                  option_info.gravity=SouthGravity;
                if (LocaleCompare("SouthEast",option) == 0)
                  option_info.gravity=SouthEastGravity;
                if (option_info.gravity == ForgetGravity)
                  ThrowCompositeException(OptionError,"UnrecognizedGravityType",
                    option);
              }
            break;
          }
        if (LocaleCompare("green-primary",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  ThrowCompositeException(OptionError,"MissingPoint",option);
              }
            break;
          }
        ThrowCompositeException(OptionError,"UnrecognizedOption",option)
      }
      case 'h':
      {
        if (LocaleCompare("help",option+1) == 0)
          {
            CompositeUsage();
            ThrowCompositeException(OptionError,"UsageError",NULL);
            break;
          }
        ThrowCompositeException(OptionError,"UnrecognizedOption",option)
      }
      case 'i':
      {
        if (LocaleCompare("interlace",option+1) == 0)
          {
            image_info->interlace=NoInterlace;
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowCompositeException(OptionError,"MissingType",option);
                option=argv[i];
                image_info->interlace=UndefinedInterlace;
                if (LocaleCompare("None",option) == 0)
                  image_info->interlace=NoInterlace;
                if (LocaleCompare("Line",option) == 0)
                  image_info->interlace=LineInterlace;
                if (LocaleCompare("Plane",option) == 0)
                  image_info->interlace=PlaneInterlace;
                if (LocaleCompare("Partition",option) == 0)
                  image_info->interlace=PartitionInterlace;
                if (image_info->interlace == UndefinedInterlace)
                  ThrowCompositeException(OptionError,"UnrecognizedInterlaceType",
                    option);
              }
            break;
          }
        ThrowCompositeException(OptionError,"UnrecognizedOption",option)
      }
      case 'l':
      {
        if (LocaleCompare("label",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowCompositeException(OptionError,"MissingImageLabel",
                    option);
              }
            break;
          }
        if (LocaleCompare("limit",option+1) == 0)
          {
            if (*option == '-')
              {
                char
                  *type;

                i++;
                if (i == argc)
                  ThrowCompositeException(OptionError,"MissingResourceType",
                    option);
                type=argv[i];
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  ThrowCompositeException(OptionError,"MissingResourceLimit",
                    option);
                if (LocaleCompare("disk",type) == 0)
                  SetMagickResourceLimit(DiskResource,atol(argv[i]));
                else
                  if (LocaleCompare("map",type) == 0)
                    SetMagickResourceLimit(MapResource,atol(argv[i]));
                  else
                    if (LocaleCompare("memory",type) == 0)
                      SetMagickResourceLimit(MemoryResource,atol(argv[i]));
                    else
                      ThrowCompositeException(OptionError,"UnrecognizedResourceType",
                        type);
              }
            break;
          }
        if (LocaleCompare("log",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowCompositeException(OptionError,"MissingLogFormat",
                    option);
                (void) SetLogFormat(argv[i]);
              }
            break;
          }
        ThrowCompositeException(OptionError,"UnrecognizedOption",option)
      }
      case 'm':
      {
        if (LocaleCompare("matte",option+1) == 0)
          break;
        if (LocaleCompare("monochrome",option+1) == 0)
          break;
        ThrowCompositeException(OptionError,"UnrecognizedOption",option)
      }
      case 'n':
      {
        if (LocaleCompare("negate",option+1) == 0)
          break;
        if (LocaleCompare("noop",option+1) == 0)
          {
            status&=CompositeImageList(image_info,&image,composite_image,
              mask_image,&option_info,exception);
            if (composite_image != (Image *) NULL)
              {
                DestroyImageList(composite_image);
                composite_image=NewImageList();
              }
            if (mask_image != (Image *) NULL)
              {
                DestroyImageList(mask_image);
                mask_image=NewImageList();
              }
            (void) CatchImageException(image);
            break;
          }
        ThrowCompositeException(OptionError,"UnrecognizedOption",option)
      }
      case 'p':
      {
        if (LocaleCompare("page",option+1) == 0)
          {
            (void) CloneString(&image_info->page,(char *) NULL);
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowCompositeException(OptionError,"MissingPageGeometry",
                    option);
                image_info->page=GetPageGeometry(argv[i]);
              }
            break;
          }
        if (LocaleCompare("process",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowCompositeException(OptionError,"Missing argument",
                    option);
              }
            break;
          }
        if (LocaleCompare("profile",option+1) == 0)
          {
            i++;
            if (i == argc)
              ThrowCompositeException(OptionError,"Missing profile",option);
            break;
          }
        ThrowCompositeException(OptionError,"UnrecognizedOption",option)
      }
      case 'q':
      {
        if (LocaleCompare("quality",option+1) == 0)
          {
            image_info->quality=DefaultCompressionQuality;
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !IsGeometry(argv[i]))
                  ThrowCompositeException(OptionError,
                    "MissingCompressionQuality",option);
                image_info->quality=atol(argv[i]);
              }
            break;
          }
        ThrowCompositeException(OptionError,"UnrecognizedOption",option)
      }
      case 'r':
      {
        if (LocaleCompare("red-primary",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  ThrowCompositeException(OptionError,"MissingPoint",option);
              }
            break;
          }
        if (LocaleCompare("render",option+1) == 0)
          break;
        if (LocaleCompare("resize",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !IsGeometry(argv[i]))
                  ThrowCompositeException(OptionError,"MissingGeometry",option);
              }
            break;
          }
        if (LocaleCompare("rotate",option+1) == 0)
          {
            i++;
            if ((i == argc) || !IsGeometry(argv[i]))
              ThrowCompositeException(OptionError,"MissingImageRotation",
                option);
            break;
          }
        ThrowCompositeException(OptionError,"UnrecognizedOption",option)
      }
      case 's':
      {
        if (LocaleCompare("sampling-factor",option+1) == 0)
          {
            (void) CloneString(&image_info->sampling_factor,(char *) NULL);
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !IsGeometry(argv[i]))
                  ThrowCompositeException(OptionError,"MissingGeometry",option);
                (void) CloneString(&image_info->sampling_factor,argv[i]);
              }
            break;
          }
        if (LocaleCompare("scene",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  ThrowCompositeException(OptionError,"MissingImageScene",
                    option);
              }
            break;
          }
        if (LocaleCompare("sharpen",option+1) == 0)
          {
            i++;
            if ((i == argc) || !sscanf(argv[i],"%ld",&x))
              ThrowCompositeException(OptionError,"MissingGeometry",option);
            break;
          }
        if (LocaleCompare("size",option+1) == 0)
          {
            (void) CloneString(&image_info->size,(char *) NULL);
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !IsGeometry(argv[i]))
                  ThrowCompositeException(OptionError,"MissingGeometry",option);
                (void) CloneString(&image_info->size,argv[i]);
              }
            break;
          }
        if (LocaleCompare("stegano",option+1) == 0)
          {
            option_info.stegano=0;
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  ThrowCompositeException(OptionError,"Missing offset",option);
                option_info.stegano=atol(argv[i])+1;
              }
            break;
          }
        if (LocaleCompare("stereo",option+1) == 0)
          {
            option_info.stereo=(*option == '-');
            break;
          }
        ThrowCompositeException(OptionError,"UnrecognizedOption",option)
      }
      case 't':
      {
        if (LocaleCompare("tile",option+1) == 0)
          {
            option_info.tile=(*option == '-');
            break;
          }
        if (LocaleCompare("transform",option+1) == 0)
          break;
        if (LocaleCompare("treedepth",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  ThrowCompositeException(OptionError,"MissingTreeDepth",
                    option);
              }
            break;
          }
        if (LocaleCompare("type",option+1) == 0)
          {
            image_info->type=UndefinedType;
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowCompositeException(OptionError,"MissingType",option);
                option=argv[i];
                if (LocaleCompare("Bilevel",option) == 0)
                  image_info->type=BilevelType;
                if (LocaleCompare("Grayscale",option) == 0)
                  image_info->type=GrayscaleType;
                if (LocaleCompare("GrayscaleMatte",option) == 0)
                  image_info->type=GrayscaleMatteType;
                if (LocaleCompare("Palette",option) == 0)
                  image_info->type=PaletteType;
                if (LocaleCompare("PaletteMatte",option) == 0)
                  image_info->type=PaletteMatteType;
                if (LocaleCompare("TrueColor",option) == 0)
                  image_info->type=TrueColorType;
                if (LocaleCompare("TrueColorMatte",option) == 0)
                  image_info->type=TrueColorMatteType;
                if (LocaleCompare("ColorSeparation",option) == 0)
                  image_info->type=ColorSeparationType;
                if (LocaleCompare("ColorSeparationMatte",option) == 0)
                  image_info->type=ColorSeparationMatteType;
                if (LocaleCompare("Optimize",option) == 0)
                  image_info->type=OptimizeType;
                if (image_info->type == UndefinedType)
                  ThrowCompositeException(OptionError,"UnrecognizedImageType",
                    option);
              }
            break;
          }
        ThrowCompositeException(OptionError,"UnrecognizedOption",option)
      }
      case 'u':
      {
        if (LocaleCompare("units",option+1) == 0)
          {
            image_info->units=UndefinedResolution;
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowCompositeException(OptionError,"MissingType",option);
                option=argv[i];
                image_info->units=UndefinedResolution;
                if (LocaleCompare("PixelsPerInch",option) == 0)
                  image_info->units=PixelsPerInchResolution;
                if (LocaleCompare("PixelsPerCentimeter",option) == 0)
                  image_info->units=PixelsPerCentimeterResolution;
              }
            break;
          }
        if (LocaleCompare("unsharp",option+1) == 0)
          {
            (void) CloneString(&option_info.unsharp_geometry,(char *) NULL);
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%lf",&sans))
                  ThrowCompositeException(OptionError,"MissingGeometry",option);
                (void) CloneString(&option_info.unsharp_geometry,argv[i]);
                option_info.compose=ThresholdCompositeOp;
              }
            break;
          }
        ThrowCompositeException(OptionError,"UnrecognizedOption",option)
      }
      case 'v':
      {
        if (LocaleCompare("verbose",option+1) == 0)
          {
            image_info->verbose=(*option == '-');
            break;
          }
        if (LocaleCompare("verbose",option+1) == 0)
          break;
        if (LocaleCompare("virtual-pixel",option+1) == 0)
          {
            if (*option == '-')
              {
                VirtualPixelMethod
                  virtual_pixel_method;

                i++;
                if (i == argc)
                  ThrowCompositeException(OptionError,
                    "MissingVirtualPixelMethod",option);
                option=argv[i];
                virtual_pixel_method=UndefinedVirtualPixelMethod;
                if (LocaleCompare("Constant",option) == 0)
                  virtual_pixel_method=ConstantVirtualPixelMethod;
                if (LocaleCompare("Edge",option) == 0)
                  virtual_pixel_method=EdgeVirtualPixelMethod;
                if (LocaleCompare("Mirror",option) == 0)
                  virtual_pixel_method=MirrorVirtualPixelMethod;
                if (LocaleCompare("Tile",option) == 0)
                  virtual_pixel_method=TileVirtualPixelMethod;
                if (virtual_pixel_method == UndefinedVirtualPixelMethod)
                  ThrowCompositeException(OptionError,
                    "UnrecognizedVirtualPixelMethod",option);
              }
            break;
          }
        ThrowCompositeException(OptionError,"UnrecognizedOption",option)
      }
      case 'w':
      {
        if (LocaleCompare("watermark",option+1) == 0)
          {
            (void) CloneString(&option_info.watermark_geometry,(char *) NULL);
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%lf",&sans))
                  ThrowCompositeException(OptionError,"MissingGeometry",option);
                (void) CloneString(&option_info.watermark_geometry,argv[i]);
                option_info.compose=ModulateCompositeOp;
              }
            break;
          }
        if (LocaleCompare("white-point",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  ThrowCompositeException(OptionError,"MissingPoint",option);
              }
            break;
          }
        if (LocaleCompare("write",option+1) == 0)
          {
            i++;
            if (i == argc)
              ThrowCompositeException(OptionError,"MissingFilename",option);
            break;
          }
        ThrowCompositeException(OptionError,"UnrecognizedOption",option)
      }
      case '?':
        break;
      default:
        ThrowCompositeException(OptionError,"UnrecognizedOption",option)
    }
  }
  if (image == (Image *) NULL)
    {
      if (exception->severity == UndefinedException)
        ThrowCompositeException(OptionError,"MissingAnImageFilename",
          (char *) NULL);
      return(False);
    }
  if (i != (argc-1))
    ThrowCompositeException(OptionError,"MissingAnImageFilename",(char *) NULL);
  status&=MogrifyImages(image_info,i-j,argv+j,&image);
  (void) CatchImageException(image);
  status&=CompositeImageList(image_info,&image,composite_image,mask_image,
    &option_info,exception);
  /*
    Write composite images.
  */
  status&=WriteImages(image_info,image,argv[argc-1],exception);
  if (metadata != (char **) NULL)
    {
      char
        *text;

      text=TranslateText(image_info,image,(format != (char *) NULL) ? format : "%w,%h,%m");
      if (text == (char *) NULL)
        ThrowCompositeException(ResourceLimitError,"MemoryAllocationFailed",
          "UnableToFormatImageMetadata");
      (void) ConcatenateString(&(*metadata),text);
      (void) ConcatenateString(&(*metadata),"\n");
      LiberateMemory((void **) &text);
    }
  LiberateCompositeOptions(&option_info);
  DestroyImageList(composite_image);
  DestroyImageList(mask_image);
  DestroyImageList(image);
  LiberateArgumentList(argc,argv);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   C o n v e r t I m a g e C o m m a n d                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ConvertImageCommand() reads one or more images, applies one or more image
%  processing operations, and writes out the image in the same or differing
%  format.
%
%  The format of the ConvertImageCommand method is:
%
%      unsigned int ConvertImageCommand(ImageInfo *image_info,const int argc,
%        char **argv,char **metadata,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: The image info.
%
%    o argc: The number of elements in the argument vector.
%
%    o argv: A text array containing the command line arguments.
%
%    o metadata: any metadata is returned here.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/

static unsigned int ConcatenateImages(const int argc,char **argv,
  ExceptionInfo *exception)
{
  FILE
    *input,
    *output;

  int
    c;

  register long
    i;

  /*
    Open output file.
  */
  output=fopen(argv[argc-1],"wb");
  if (output == (FILE *) NULL)
    {
      ThrowException(exception,FileOpenError,"UnableToOpenFile",argv[argc-1]);
      return(False);
    }
  for (i=2; i < (argc-1); i++)
  {
    input=fopen(argv[i],"rb");
    if (input == (FILE *) NULL)
      {
        ThrowException(exception,FileOpenError,"UnableToOpenFile",argv[i]);
        continue;
      }
    for (c=fgetc(input); c != EOF; c=fgetc(input))
      (void) fputc((char) c,output);
    (void) fclose(input);
    (void) remove(argv[i]);
  }
  (void) fclose(output);
  return(True);
}

#define NotInitialized  (unsigned int) (~0)
#define ThrowConvertException(code,reason,description) \
{ \
  DestroyImageList(image); \
  DestroyImageList(image_list); \
  ThrowException(exception,code,reason,description); \
  LiberateArgumentList(argc,argv); \
  return(False); \
}
MagickExport unsigned int ConvertImageCommand(ImageInfo *image_info,
  int argc,char **argv,char **metadata,ExceptionInfo *exception)
{
  char
    *filename,
    *format,
    *option;

  double
    sans;

  Image
    *image,
    *image_list,
    *next_image;

  long
    j,
    k,
    x;

  register long
    i;

  unsigned int
    ping,
    status;

  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  assert(exception != (ExceptionInfo *) NULL);

  status=ExpandFilenames(&argc,&argv);
  if (status == False)
    MagickFatalError(ResourceLimitFatalError,"MemoryAllocationFailed",
      (char *) NULL);

  /*
    Set defaults.
  */
  filename=(char *) NULL;
  format=(char *) NULL;
  image=NewImageList();
  image_list=(Image *) NULL;
  (void) strncpy(image_info->filename,argv[argc-1],MaxTextExtent-1);
  (void) SetImageInfo(image_info,True,exception);
  ping=False;
  option=(char *) NULL;
  status=True;
  /*
    Parse command-line arguments.
  */
  if ((argc > 2) && (LocaleCompare("-concatenate",argv[1]) == 0))
    return(ConcatenateImages(argc,argv,exception));
  if (argc < 2 || ((argc < 3) && (LocaleCompare("-help",argv[1]) == 0 ||
      LocaleCompare("-?",argv[1]) == 0)))
    {
      ConvertUsage();
      ThrowConvertException(OptionError,"UsageError",NULL);
    }
  j=1;
  k=0;
  for (i=1; i < (argc-1); i++)
  {
    option=argv[i];
    if ((strlen(option) == 1) || ((*option != '-') && (*option != '+')))
      {
        /*
          Read input image.
        */
        k=i;
        filename=argv[i];
        (void) strncpy(image_info->filename,filename,MaxTextExtent-1);
        if (ping)
          next_image=PingImage(image_info,exception);
        else
          next_image=ReadImage(image_info,exception);
        status&=(next_image != (Image *) NULL) &&
          (exception->severity < ErrorException);
        if (next_image == (Image *) NULL)
          continue;
        if (image == (Image *) NULL)
          {
            image=next_image;
            continue;
          }
        AppendImageToList(&image,next_image);
        continue;
      }
    if ((image != (Image *) NULL) && (j != (k+1)))
      {
        status&=MogrifyImages(image_info,i-j,argv+j,&image);
        (void) CatchImageException(image);
        AppendImageToList(&image_list,image);
        image=NewImageList();
        j=k+1;
      }
    switch (*(option+1))
    {
      case 'a':
      {
        if (LocaleCompare("adjoin",option+1) == 0)
          {
            image_info->adjoin=(*option == '-');
            break;
          }
        if (LocaleCompare("affine",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%lf",&sans))
                  ThrowConvertException(OptionError,"MissingMatrix",option);
              }
            break;
          }
        if (LocaleCompare("antialias",option+1) == 0)
          {
            image_info->antialias=(*option == '-');
            break;
          }
        if (LocaleCompare("append",option+1) == 0)
          break;
        if (LocaleCompare("authenticate",option+1) == 0)
          {
            (void) CloneString(&image_info->authenticate,(char *) NULL);
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowConvertException(OptionError,"MissingPassword",option);
                (void) CloneString(&image_info->authenticate,argv[i]);
              }
            break;
          }
        if (LocaleCompare("average",option+1) == 0)
          break;
        ThrowConvertException(OptionError,"UnrecognizedOption",option)
      }
      case 'b':
      {
        if (LocaleCompare("background",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowConvertException(OptionError,"MissingBackgroundColor",
                    option);
                (void) QueryColorDatabase(argv[i],&image_info->background_color,
                  exception);
              }
            break;
          }
        if (LocaleCompare("blue-primary",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  ThrowConvertException(OptionError,"MissingPoint",option);
              }
            break;
          }
        if (LocaleCompare("blur",option+1) == 0)
          {
            i++;
            if ((i == argc) || !sscanf(argv[i],"%ld",&x))
              ThrowConvertException(OptionError,"MissingGeometry",option);
            break;
          }
        if (LocaleCompare("border",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !IsGeometry(argv[i]))
                  ThrowConvertException(OptionError,"MissingGeometry",option);
              }
            break;
          }
        if (LocaleCompare("bordercolor",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowConvertException(OptionError,"MissingBorderColor",
                    option);
                (void) QueryColorDatabase(argv[i],&image_info->border_color,
                  exception);
              }
            break;
          }
        if (LocaleCompare("box",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowConvertException(OptionError,"MissingColors",option);
              }
            break;
          }
        ThrowConvertException(OptionError,"UnrecognizedOption",option)
      }
      case 'c':
      {
        if (LocaleCompare("cache",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  ThrowConvertException(OptionError,"MissingThreshold",option);
                SetMagickResourceLimit(MemoryResource,atol(argv[i]));
                SetMagickResourceLimit(MapResource,2*atol(argv[i]));
              }
            break;
          }
        if (LocaleCompare("channel",option+1) == 0)
          {
            if (*option == '-')
              {
                ChannelType
                  channel;

                i++;
                if (i == argc)
                  ThrowConvertException(OptionError,"MissingType",option);
                option=argv[i];
                channel=UndefinedChannel;
                if (LocaleCompare("Red",option) == 0)
                  channel=RedChannel;
                if (LocaleCompare("Cyan",option) == 0)
                  channel=CyanChannel;
                if (LocaleCompare("Green",option) == 0)
                  channel=GreenChannel;
                if (LocaleCompare("Magenta",option) == 0)
                  channel=MagentaChannel;
                if (LocaleCompare("Blue",option) == 0)
                  channel=BlueChannel;
                if (LocaleCompare("Yellow",option) == 0)
                  channel=YellowChannel;
                if (LocaleCompare("Opacity",option) == 0)
                  channel=OpacityChannel;
                if (LocaleCompare("Black",option) == 0)
                  channel=BlackChannel;
                if (LocaleCompare("Matte",option) == 0)
                  channel=MatteChannel;
                if (channel == UndefinedChannel)
                  ThrowConvertException(OptionError,"UnrecognizedChannelType",
                    option);
              }
            break;
          }
        if (LocaleCompare("charcoal",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%lf",&sans))
                  ThrowConvertException(OptionError,"MissingPaintRadius",
                    option);
              }
            break;
          }
        if (LocaleCompare("chop",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !IsGeometry(argv[i]))
                  ThrowConvertException(OptionError,"MissingGeometry",option);
              }
            break;
          }
        if (LocaleCompare("clip",option+1) == 0)
          break;
        if (LocaleCompare("coalesce",option+1) == 0)
          break;
        if (LocaleCompare("colorize",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  ThrowConvertException(OptionError,"MissingImageGamma",option);
              }
            break;
          }
        if (LocaleCompare("colors",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  ThrowConvertException(OptionError,"MissingColors",option);
              }
            break;
          }
        if (LocaleCompare("colorspace",option+1) == 0)
          {
            if ((*option == '-') || (*option == '+'))
              {
                i++;
                if (i == argc)
                  ThrowConvertException(OptionError,"MissingType",option);
                option=argv[i];
                image_info->colorspace=UndefinedColorspace;
                if (LocaleCompare("cmyk",option) == 0)
                  image_info->colorspace=CMYKColorspace;
                if (LocaleCompare("gray",option) == 0)
                  image_info->colorspace=GRAYColorspace;
                if (LocaleCompare("hsl",option) == 0)
                  image_info->colorspace=HSLColorspace;
                if (LocaleCompare("hwb",option) == 0)
                  image_info->colorspace=HWBColorspace;
                if (LocaleCompare("ohta",option) == 0)
                  image_info->colorspace=OHTAColorspace;
                if (LocaleCompare("rgb",option) == 0)
                  image_info->colorspace=RGBColorspace;
                if (LocaleCompare("srgb",option) == 0)
                  image_info->colorspace=sRGBColorspace;
                if (LocaleCompare("transparent",option) == 0)
                  image_info->colorspace=TransparentColorspace;
                if (LocaleCompare("xyz",option) == 0)
                  image_info->colorspace=XYZColorspace;
                if (LocaleCompare("ycbcr",option) == 0)
                  image_info->colorspace=YCbCrColorspace;
                if (LocaleCompare("ycc",option) == 0)
                  image_info->colorspace=YCCColorspace;
                if (LocaleCompare("yiq",option) == 0)
                  image_info->colorspace=YIQColorspace;
                if (LocaleCompare("ypbpr",option) == 0)
                  image_info->colorspace=YPbPrColorspace;
                if (LocaleCompare("yuv",option) == 0)
                  image_info->colorspace=YUVColorspace;
                if (image_info->colorspace == UndefinedColorspace)
                  ThrowConvertException(OptionError,"UnrecognizedColorspace",option);
              }
            break;
          }
        if (LocaleCompare("comment",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowConvertException(OptionError,"MissingComment",option);
              }
            break;
          }
        if (LocaleCompare("compress",option+1) == 0)
          {
            image_info->compression=NoCompression;
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowConvertException(OptionError,"MissingType",option);
                option=argv[i];
                image_info->compression=UndefinedCompression;
                if (LocaleCompare("None",option) == 0)
                  image_info->compression=NoCompression;
                if (LocaleCompare("BZip",option) == 0)
                  image_info->compression=BZipCompression;
                if (LocaleCompare("Fax",option) == 0)
                  image_info->compression=FaxCompression;
                if (LocaleCompare("Group4",option) == 0)
                  image_info->compression=Group4Compression;
                if (LocaleCompare("JPEG",option) == 0)
                  image_info->compression=JPEGCompression;
                if (LocaleCompare("Lossless",option) == 0)
                  image_info->compression=LosslessJPEGCompression;
                if (LocaleCompare("LZW",option) == 0)
                  image_info->compression=LZWCompression;
                if (LocaleCompare("RLE",option) == 0)
                  image_info->compression=RLECompression;
                if (LocaleCompare("Zip",option) == 0)
                  image_info->compression=ZipCompression;
                if (image_info->compression == UndefinedCompression)
                  ThrowConvertException(OptionError,"UnrecognizedImageCompression",
                    option);
              }
            break;
          }
        if (LocaleCompare("contrast",option+1) == 0)
          break;
        if (LocaleCompare("convolve",option+1) == 0)
          {
           if (*option == '-')
             {
               i++;
               if ((i == (argc-1)) || !IsGeometry(argv[i]))
                 ThrowConvertException(OptionError,"MissingCoefficients",
                   option);
             }
           break;
         }
        if (LocaleCompare("crop",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !IsGeometry(argv[i]))
                  ThrowConvertException(OptionError,"MissingGeometry",option);
              }
            break;
          }
        if (LocaleCompare("cycle",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  ThrowConvertException(OptionError,"Missing amount",option);
              }
            break;
          }
        ThrowConvertException(OptionError,"UnrecognizedOption",option)
      }
      case 'd':
      {
        if (LocaleCompare("deconstruct",option+1) == 0)
          break;
        if (LocaleCompare("debug",option+1) == 0)
          {
            (void) SetLogEventMask("None");
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowConvertException(OptionError,"MissingEventMask",option);
                (void) SetLogEventMask(argv[i]);
              }
            break;
          }
        if (LocaleCompare("delay",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  ThrowConvertException(OptionError,"MissingDelay",option);
              }
            break;
          }
        if (LocaleCompare("density",option+1) == 0)
          {
            (void) CloneString(&image_info->density,(char *) NULL);
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !IsGeometry(argv[i]))
                  ThrowConvertException(OptionError,"MissingGeometry",option);
                (void) CloneString(&image_info->density,argv[i]);
              }
            break;
          }
        if (LocaleCompare("depth",option+1) == 0)
          {
            image_info->depth=QuantumDepth;
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  ThrowConvertException(OptionError,"MissingImageDepth",option);
                image_info->depth=atol(argv[i]);
              }
            break;
          }
        if (LocaleCompare("despeckle",option+1) == 0)
          break;
        if (LocaleCompare("display",option+1) == 0)
          {
            (void) CloneString(&image_info->server_name,(char *) NULL);
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowConvertException(OptionError,"MissingServerName",option);
                (void) CloneString(&image_info->server_name,argv[i]);
              }
            break;
          }
        if (LocaleCompare("dispose",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowConvertException(OptionError,"MissingDisposeMethod",
                    option);
                option=argv[i];
                if ((LocaleCompare("0",option) != 0) &&
                    (LocaleCompare("1",option) != 0) &&
                    (LocaleCompare("2",option) != 0) &&
                    (LocaleCompare("3",option) != 0) &&
                    (LocaleCompare("Undefined",option) != 0) &&
                    (LocaleCompare("None",option) != 0) &&
                    (LocaleCompare("Background",option) != 0) &&
                    (LocaleCompare("Previous",option) != 0))
                  ThrowConvertException(OptionError,"UnrecognizedDisposeMethod",
                    option);
              }
            break;
          }
        if (LocaleCompare("dither",option+1) == 0)
          {
            image_info->dither=(*option == '-');
            break;
          }
        if (LocaleCompare("draw",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowConvertException(OptionError,"MissingDrawingPrimitive",
                    option);
              }
            break;
          }
        ThrowConvertException(OptionError,"UnrecognizedOption",option)
      }
      case 'e':
      {
        if (LocaleCompare("edge",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%lf",&sans))
                  ThrowConvertException(OptionError,"MissingPaintRadius",
                    option);
              }
            break;
          }
        if (LocaleCompare("emboss",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !IsGeometry(argv[i]))
                  ThrowConvertException(OptionError,"MissingPaintRadius",
                    option);
              }
            break;
          }
        if (LocaleCompare("encoding",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowConvertException(OptionError,"MissingEnodingType",
                    option);
              }
            break;
          }
        if (LocaleCompare("endian",option+1) == 0)
          {
            image_info->endian=UndefinedEndian;
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowConvertException(OptionError,"MissingType",option);
                option=argv[i];
                image_info->endian=UndefinedEndian;
                if (LocaleCompare("LSB",option) == 0)
                  image_info->endian=LSBEndian;
                if (LocaleCompare("MSB",option) == 0)
                  image_info->endian=MSBEndian;
                if (image_info->endian == UndefinedEndian)
                  ThrowConvertException(OptionError,"UnrecognizedEndianType",
                    option);
              }
            break;
          }
        if (LocaleCompare("enhance",option+1) == 0)
          break;
        if (LocaleCompare("equalize",option+1) == 0)
          break;
        ThrowConvertException(OptionError,"UnrecognizedOption",option)
      }
      case 'f':
      {
        if (LocaleCompare("fill",option+1) == 0)
          {
            (void) QueryColorDatabase("none",&image_info->pen,exception);
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowConvertException(OptionError,"MissingFillColor",option);
                (void) QueryColorDatabase(argv[i],&image_info->pen,exception);
              }
            break;
          }
        if (LocaleCompare("filter",option+1) == 0)
          {
            if (*option == '-')
              {
                FilterTypes
                  filter;

                i++;
                if (i == argc)
                  ThrowConvertException(OptionError,"MissingType",option);
                option=argv[i];
                filter=UndefinedFilter;
                if (LocaleCompare("Point",option) == 0)
                  filter=PointFilter;
                if (LocaleCompare("Box",option) == 0)
                  filter=BoxFilter;
                if (LocaleCompare("Triangle",option) == 0)
                  filter=TriangleFilter;
                if (LocaleCompare("Hermite",option) == 0)
                  filter=HermiteFilter;
                if (LocaleCompare("Hanning",option) == 0)
                  filter=HanningFilter;
                if (LocaleCompare("Hamming",option) == 0)
                  filter=HammingFilter;
                if (LocaleCompare("Blackman",option) == 0)
                  filter=BlackmanFilter;
                if (LocaleCompare("Gaussian",option) == 0)
                  filter=GaussianFilter;
                if (LocaleCompare("Quadratic",option) == 0)
                  filter=QuadraticFilter;
                if (LocaleCompare("Cubic",option) == 0)
                  filter=CubicFilter;
                if (LocaleCompare("Catrom",option) == 0)
                  filter=CatromFilter;
                if (LocaleCompare("Mitchell",option) == 0)
                  filter=MitchellFilter;
                if (LocaleCompare("Lanczos",option) == 0)
                  filter=LanczosFilter;
                if (LocaleCompare("Bessel",option) == 0)
                  filter=BesselFilter;
                if (LocaleCompare("Sinc",option) == 0)
                  filter=SincFilter;
                if (filter == UndefinedFilter)
                  ThrowConvertException(OptionError,"UnrecognizedImageFilter",
                    option);
              }
            break;
          }
        if (LocaleCompare("flatten",option+1) == 0)
          break;
        if (LocaleCompare("flip",option+1) == 0)
          break;
        if (LocaleCompare("flop",option+1) == 0)
          break;
        if (LocaleCompare("font",option+1) == 0)
          {
            (void) CloneString(&image_info->font,(char *) NULL);
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowConvertException(OptionError,"MissingFontName",option);
                (void) CloneString(&image_info->font,argv[i]);
              }
            break;
          }
        if (LocaleCompare("format",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowConvertException(OptionError,"MissingFormatSpecifier",
                    option);
                format=argv[i];
              }
            break;
          }
        if (LocaleCompare("frame",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !IsGeometry(argv[i]))
                  ThrowConvertException(OptionError,"MissingGeometry",option);
              }
            break;
          }
        if (LocaleCompare("fuzz",option+1) == 0)
          {
            image_info->fuzz=0.0;
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  ThrowConvertException(OptionError,"Missing distance",option);
                image_info->fuzz=StringToDouble(argv[i],MaxRGB);
              }
            break;
          }
        ThrowConvertException(OptionError,"UnrecognizedOption",option)
      }
      case 'g':
      {
        if (LocaleCompare("gamma",option+1) == 0)
          {
            i++;
            if ((i == argc) || !sscanf(argv[i],"%lf",&sans))
              ThrowConvertException(OptionError,"MissingImageGamma",option);
            break;
          }
        if (LocaleCompare("gaussian",option+1) == 0)
          {
            i++;
            if ((i == argc) || !sscanf(argv[i],"%ld",&x))
              ThrowConvertException(OptionError,"MissingGeometry",option);
            break;
          }
        if (LocaleCompare("geometry",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !IsGeometry(argv[i]))
                  ThrowConvertException(OptionError,"MissingGeometry",option);
              }
            break;
          }
        if (LocaleCompare("gravity",option+1) == 0)
          {
            GravityType
              gravity;

            gravity=ForgetGravity;
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowConvertException(OptionError,"MissingType",option);
                option=argv[i];
                if (LocaleCompare("NorthWest",option) == 0)
                  gravity=NorthWestGravity;
                if (LocaleCompare("North",option) == 0)
                  gravity=NorthGravity;
                if (LocaleCompare("NorthEast",option) == 0)
                  gravity=NorthEastGravity;
                if (LocaleCompare("West",option) == 0)
                  gravity=WestGravity;
                if (LocaleCompare("Center",option) == 0)
                  gravity=CenterGravity;
                if (LocaleCompare("East",option) == 0)
                  gravity=EastGravity;
                if (LocaleCompare("SouthWest",option) == 0)
                  gravity=SouthWestGravity;
                if (LocaleCompare("South",option) == 0)
                  gravity=SouthGravity;
                if (LocaleCompare("SouthEast",option) == 0)
                  gravity=SouthEastGravity;
                if (gravity == ForgetGravity)
                  ThrowConvertException(OptionError,"UnrecognizedGravityType",
                    option);
              }
            break;
          }
        if (LocaleCompare("green-primary",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  ThrowConvertException(OptionError,"MissingPoint",option);
              }
            break;
          }
        ThrowConvertException(OptionError,"UnrecognizedOption",option)
      }
      case 'h':
      {
        if (LocaleCompare("help",option+1) == 0)
          {
            ConvertUsage();
            ThrowConvertException(OptionError,"UsageError",NULL);
          }
        ThrowConvertException(OptionError,"UnrecognizedOption",option)
      }
      case 'i':
      {
        if (LocaleCompare("implode",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%lf",&sans))
                  ThrowConvertException(OptionError,"MissingImplodeAmount",
                    option);
              }
            break;
          }
        if (LocaleCompare("intent",option+1) == 0)
          {
            if (*option == '-')
              {
                RenderingIntent
                  rendering_intent;

                i++;
                if (i == argc)
                  ThrowConvertException(OptionError,"MissingType",option);
                option=argv[i];
                rendering_intent=UndefinedIntent;
                if (LocaleCompare("Absolute",option) == 0)
                  rendering_intent=AbsoluteIntent;
                if (LocaleCompare("Perceptual",option) == 0)
                  rendering_intent=PerceptualIntent;
                if (LocaleCompare("Relative",option) == 0)
                  rendering_intent=RelativeIntent;
                if (LocaleCompare("Saturation",option) == 0)
                  rendering_intent=SaturationIntent;
                if (rendering_intent == UndefinedIntent)
                  ThrowConvertException(OptionError,"UnrecognizedIntentType",
                    option);
              }
            break;
          }
        if (LocaleCompare("interlace",option+1) == 0)
          {
            image_info->interlace=NoInterlace;
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowConvertException(OptionError,"MissingType",option);
                option=argv[i];
                image_info->interlace=UndefinedInterlace;
                if (LocaleCompare("None",option) == 0)
                  image_info->interlace=NoInterlace;
                if (LocaleCompare("Line",option) == 0)
                  image_info->interlace=LineInterlace;
                if (LocaleCompare("Plane",option) == 0)
                  image_info->interlace=PlaneInterlace;
                if (LocaleCompare("Partition",option) == 0)
                  image_info->interlace=PartitionInterlace;
                if (image_info->interlace == UndefinedInterlace)
                  ThrowConvertException(OptionError,"UnrecognizedInterlaceType",
                    option);
              }
            break;
          }
        ThrowConvertException(OptionError,"UnrecognizedOption",option)
      }
      case 'l':
      {
        if (LocaleCompare("label",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowConvertException(OptionError,"MissingImageLabel",option);
              }
            break;
          }
        if (LocaleCompare("lat",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%lf",&sans))
                  ThrowConvertException(OptionError,"MissingGeometry",option);
              }
            break;
          }
        if (LocaleCompare("level",option+1) == 0)
          {
            i++;
            if ((i == argc) || !sscanf(argv[i],"%lf",&sans))
              ThrowConvertException(OptionError,"MissingImageGamma",option);
            break;
          }
        if (LocaleCompare("linewidth",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  ThrowConvertException(OptionError,"MissingPointsize",option);
              }
            break;
          }
        if (LocaleCompare("limit",option+1) == 0)
          {
            if (*option == '-')
              {
                char
                  *type;

                i++;
                if (i == argc)
                  ThrowConvertException(OptionError,"MissingResourceType",
                    option);
                type=argv[i];
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  ThrowConvertException(OptionError,"MissingResourceLimit",
                    option);
                if (LocaleCompare("disk",type) == 0)
                  SetMagickResourceLimit(DiskResource,atol(argv[i]));
                else
                  if (LocaleCompare("map",type) == 0)
                    SetMagickResourceLimit(MapResource,atol(argv[i]));
                  else
                    if (LocaleCompare("memory",type) == 0)
                      SetMagickResourceLimit(MemoryResource,atol(argv[i]));
                    else
                      ThrowConvertException(OptionError,"UnrecognizedResourceType",
                        type);
              }
            break;
          }
        if (LocaleCompare("list",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowConvertException(OptionError,"MissingListName",option);
                option=argv[i];
                switch (*option)
                {
                  case 'C':
                  case 'c':
                  {
                    if (LocaleCompare("Color",option) == 0)
                      {
                        (void) ListColorInfo((FILE *) NULL,exception);
                        break;
                      }
                    ThrowConvertException(OptionError,"UnrecognizedListType",
                      option)
                  }
                  case 'D':
                  case 'd':
                  {
                    if (LocaleCompare("Delegate",option) == 0)
                      {
                        (void) ListDelegateInfo((FILE *) NULL,exception);
                        break;
                      }
                    ThrowConvertException(OptionError,"UnrecognizedListType",
                      option)
                  }
                  case 'F':
                  case 'f':
                  {
                    if (LocaleCompare("Format",option) == 0)
                      {
                        (void) ListMagickInfo((FILE *) NULL,exception);
                        break;
                      }
                    ThrowConvertException(OptionError,"UnrecognizedListType",
                      option)
                  }
                  case 'M':
                  case 'm':
                  {
                    if (LocaleCompare("Magic",option) == 0)
                      {
                        (void) ListMagicInfo((FILE *) NULL,exception);
                        break;
                      }
#if defined(SupportMagickModules)
                    if (LocaleCompare("Module",option) == 0)
                      {
                        (void) ListModuleInfo((FILE *) NULL,exception);
                        break;
                      }
#endif /* SupportMagickModules */
                    ThrowConvertException(OptionError,"UnrecognizedListType",
                      option)
                  }
                  case 'R':
                  case 'r':
                  {
                    if (LocaleCompare("Resource",option) == 0)
                      {
                        (void) ListMagickResourceInfo((FILE *) NULL,exception);
                        break;
                      }
                    ThrowConvertException(OptionError,"UnrecognizedListType",
                      option)
                  }
                  case 'T':
                  case 't':
                  {
                    if (LocaleCompare("Type",option) == 0)
                      {
                        (void) ListTypeInfo((FILE *) NULL,exception);
                        break;
                      }
                    ThrowConvertException(OptionError,"UnrecognizedListType",
                      option)
                  }
                  default:
                    ThrowConvertException(OptionError,"UnrecognizedListType",
                      option)
                }
                return(True);
              }
            break;
          }
        if (LocaleCompare("log",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowConvertException(OptionError,"MissingLogFormat",option);
                (void) SetLogFormat(argv[i]);
              }
            break;
          }
        if (LocaleCompare("loop",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  ThrowConvertException(OptionError,"MissingImageIterations",
                    option);
              }
            break;
          }
        ThrowConvertException(OptionError,"UnrecognizedOption",option)
      }
      case 'm':
      {
        if (LocaleCompare("map",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowConvertException(OptionError,"MissingFilename",option);
              }
            break;
          }
        if (LocaleCompare("mask",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowConvertException(OptionError,"MissingFilename",option);
              }
            break;
          }
        if (LocaleCompare("matte",option+1) == 0)
          break;
        if (LocaleCompare("mattecolor",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowConvertException(OptionError,"MissingMatteColor",option);
                (void) QueryColorDatabase(argv[i],&image_info->matte_color,
                  exception);
              }
            break;
          }
        if (LocaleCompare("median",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  ThrowConvertException(OptionError,"MissingPaintRadius",
                    option);
              }
            break;
          }
        if (LocaleCompare("modulate",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%lf",&sans))
                  ThrowConvertException(OptionError,"MissingImageGamma",option);
              }
            break;
          }
        if (LocaleCompare("monochrome",option+1) == 0)
          {
            image_info->monochrome=(*option == '-');
            break;
          }
        if (LocaleCompare("morph",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  ThrowConvertException(OptionError,"MissingImageframes",
                    option);
              }
            break;
          }
        if (LocaleCompare("mosaic",option+1) == 0)
          break;
        ThrowConvertException(OptionError,"UnrecognizedOption",option)
      }
      case 'n':
      {
        if (LocaleCompare("negate",option+1) == 0)
          break;
        if (LocaleCompare("noise",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%lf",&sans))
                  ThrowConvertException(OptionError,"MissingImageGamma",option);
              }
            if (*option == '+')
              {
                i++;
                if (i == argc)
                  ThrowConvertException(OptionError,"MissingType",option);
                option=argv[i];
                if ((LocaleCompare("Uniform",option) != 0) &&
                    (LocaleCompare("Gaussian",option) != 0) &&
                    (LocaleCompare("Multiplicative",option) != 0) &&
                    (LocaleCompare("Impulse",option) != 0) &&
                    (LocaleCompare("Laplacian",option) != 0) &&
                    (LocaleCompare("Poisson",option) != 0))
                  ThrowConvertException(OptionError,"UnrecognizedNoiseType",
                    option);
              }
            break;
          }
        if (LocaleCompare("noop",option+1) == 0)
          break;
        if (LocaleCompare("normalize",option+1) == 0)
          break;
        ThrowConvertException(OptionError,"UnrecognizedOption",option)
      }
      case 'o':
      {
        if (LocaleCompare("opaque",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowConvertException(OptionError,"MissingOpaqueColor",
                    option);
              }
            break;
          }
        if (LocaleCompare("ordered-dither",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowConvertException(OptionError,"MissingType",option);
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  ThrowConvertException(OptionError,"MissingOrder",option);
              }
            break;
          }
        ThrowConvertException(OptionError,"UnrecognizedOption",option)
      }
      case 'p':
      {
        if (LocaleCompare("page",option+1) == 0)
          {
            (void) CloneString(&image_info->page,(char *) NULL);
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowConvertException(OptionError,"MissingPageGeometry",
                    option);
                image_info->page=GetPageGeometry(argv[i]);
              }
            break;
          }
        if (LocaleCompare("paint",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  ThrowConvertException(OptionError,"MissingPaintRadius",
                    option);
              }
            break;
          }
        if (LocaleCompare("ping",option+1) == 0)
          {
            ping=(*option == '-');
            break;
          }
        if (LocaleCompare("pointsize",option+1) == 0)
          {
            image_info->pointsize=12;
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  ThrowConvertException(OptionError,"MissingPointsize",option);
                image_info->pointsize=atof(argv[i]);
              }
            break;
          }
        if (LocaleCompare("preview",option+1) == 0)
          {
            image_info->preview_type=UndefinedPreview;
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowConvertException(OptionError,"MissingType",option);
                option=argv[i];
                image_info->preview_type=UndefinedPreview;
                if (LocaleCompare("Rotate",option) == 0)
                  image_info->preview_type=RotatePreview;
                if (LocaleCompare("Shear",option) == 0)
                  image_info->preview_type=ShearPreview;
                if (LocaleCompare("Roll",option) == 0)
                  image_info->preview_type=RollPreview;
                if (LocaleCompare("Hue",option) == 0)
                  image_info->preview_type=HuePreview;
                if (LocaleCompare("Saturation",option) == 0)
                  image_info->preview_type=SaturationPreview;
                if (LocaleCompare("Brightness",option) == 0)
                  image_info->preview_type=BrightnessPreview;
                if (LocaleCompare("Gamma",option) == 0)
                  image_info->preview_type=GammaPreview;
                if (LocaleCompare("Spiff",option) == 0)
                  image_info->preview_type=SpiffPreview;
                if (LocaleCompare("Dull",option) == 0)
                  image_info->preview_type=DullPreview;
                if (LocaleCompare("Grayscale",option) == 0)
                  image_info->preview_type=GrayscalePreview;
                if (LocaleCompare("Quantize",option) == 0)
                  image_info->preview_type=QuantizePreview;
                if (LocaleCompare("Despeckle",option) == 0)
                  image_info->preview_type=DespecklePreview;
                if (LocaleCompare("ReduceNoise",option) == 0)
                  image_info->preview_type=ReduceNoisePreview;
                if (LocaleCompare("AddNoise",option) == 0)
                  image_info->preview_type=AddNoisePreview;
                if (LocaleCompare("Sharpen",option) == 0)
                  image_info->preview_type=SharpenPreview;
                if (LocaleCompare("Blur",option) == 0)
                  image_info->preview_type=BlurPreview;
                if (LocaleCompare("Threshold",option) == 0)
                  image_info->preview_type=ThresholdPreview;
                if (LocaleCompare("EdgeDetect",option) == 0)
                  image_info->preview_type=EdgeDetectPreview;
                if (LocaleCompare("Spread",option) == 0)
                  image_info->preview_type=SpreadPreview;
                if (LocaleCompare("Shade",option) == 0)
                  image_info->preview_type=ShadePreview;
                if (LocaleCompare("Raise",option) == 0)
                  image_info->preview_type=RaisePreview;
                if (LocaleCompare("Segment",option) == 0)
                  image_info->preview_type=SegmentPreview;
                if (LocaleCompare("Solarize",option) == 0)
                  image_info->preview_type=SolarizePreview;
                if (LocaleCompare("Swirl",option) == 0)
                  image_info->preview_type=SwirlPreview;
                if (LocaleCompare("Implode",option) == 0)
                  image_info->preview_type=ImplodePreview;
                if (LocaleCompare("Wave",option) == 0)
                  image_info->preview_type=WavePreview;
                if (LocaleCompare("OilPaint",option) == 0)
                  image_info->preview_type=OilPaintPreview;
                if (LocaleCompare("CharcoalDrawing",option) == 0)
                  image_info->preview_type=CharcoalDrawingPreview;
                if (LocaleCompare("JPEG",option) == 0)
                  image_info->preview_type=JPEGPreview;
                if (image_info->preview_type == UndefinedPreview)
                  ThrowConvertException(OptionError,"UnrecognizedPreviewType",
                     option);
              }
            break;
          }
        if (LocaleCompare("process",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowConvertException(OptionError,"MissingArgument",option);
              }
            break;
          }
        if (LocaleCompare("profile",option+1) == 0)
          {
            i++;
            if (i == argc)
              ThrowConvertException(OptionError,"MissingImageProfile",option);
            break;
          }
        ThrowConvertException(OptionError,"UnrecognizedOption",option)
      }
      case 'q':
      {
        if (LocaleCompare("quality",option+1) == 0)
          {
            image_info->quality=DefaultCompressionQuality;
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !IsGeometry(argv[i]))
                  ThrowConvertException(OptionError,"MissingCompressionQuality",
                    option);
                image_info->quality=atol(argv[i]);
              }
            break;
          }
        ThrowConvertException(OptionError,"UnrecognizedOption",option)
      }
      case 'r':
      {
        if (LocaleCompare("raise",option+1) == 0)
          {
            i++;
            if ((i == argc) || !sscanf(argv[i],"%ld",&x))
              ThrowConvertException(OptionError,"MissingBevelWidth",option);
            break;
          }
        if (LocaleCompare("random-threshold",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowConvertException(OptionError,"MissingType",option);
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  ThrowConvertException(OptionError,"MissingThreshold",option);
              }
            break;
          }
        if (LocaleCompare("red-primary",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  ThrowConvertException(OptionError,"MissingPoint",option);
              }
            break;
          }
        if (LocaleCompare("region",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !IsGeometry(argv[i]))
                  ThrowConvertException(OptionError,"MissingGeometry",option);
              }
            break;
          }
        if (LocaleCompare("render",option+1) == 0)
          break;
        if (LocaleCompare("resample",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == (argc-1)) || !IsGeometry(argv[i]))
                  ThrowConvertException(OptionError,"MissingGeometry",option);
              }
            break;
          }
        if (LocaleCompare("resize",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !IsGeometry(argv[i]))
                  ThrowConvertException(OptionError,"MissingGeometry",option);
              }
            break;
          }
        if (LocaleCompare("roll",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !IsGeometry(argv[i]))
                  ThrowConvertException(OptionError,"MissingGeometry",option);
              }
            break;
          }
        if (LocaleCompare("rotate",option+1) == 0)
          {
            i++;
            if ((i == argc) || !IsGeometry(argv[i]))
              ThrowConvertException(OptionError,"MissingImageRotation",option);
            break;
          }
        ThrowConvertException(OptionError,"UnrecognizedOption",option)
      }
      case 's':
      {
        if (LocaleCompare("sample",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !IsGeometry(argv[i]))
                  ThrowConvertException(OptionError,"MissingGeometry",option);
              }
            break;
          }
        if (LocaleCompare("sampling-factor",option+1) == 0)
          {
            (void) CloneString(&image_info->sampling_factor,(char *) NULL);
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !IsGeometry(argv[i]))
                  ThrowConvertException(OptionError,"MissingGeometry",option);
                (void) CloneString(&image_info->sampling_factor,argv[i]);
              }
            break;
          }
        if (LocaleCompare("scale",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !IsGeometry(argv[i]))
                  ThrowConvertException(OptionError,"MissingGeometry",option);
              }
            break;
          }
        if (LocaleCompare("scene",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  ThrowConvertException(OptionError,"MissingImageScene",option);
              }
            break;
          }
        if (LocaleCompare("segment",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%lf",&sans))
                  ThrowConvertException(OptionError,"MissingThreshold",option);
              }
            break;
          }
        if (LocaleCompare("shade",option+1) == 0)
          {
            i++;
            if ((i == argc) || !sscanf(argv[i],"%ld",&x))
              ThrowConvertException(OptionError,"MissingAzimuth",option);
            break;
          }
        if (LocaleCompare("sharpen",option+1) == 0)
          {
            i++;
            if ((i == argc) || !sscanf(argv[i],"%ld",&x))
              ThrowConvertException(OptionError,"MissingGeometry",option);
            break;
          }
        if (LocaleCompare("shave",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !IsGeometry(argv[i]))
                  ThrowConvertException(OptionError,"MissingGeometry",option);
              }
            break;
          }
        if (LocaleCompare("shear",option+1) == 0)
          {
            i++;
            if ((i == argc) || !sscanf(argv[i],"%lf",&sans))
              ThrowConvertException(OptionError,"MissingGeometry",option);
            break;
          }
        if (LocaleCompare("size",option+1) == 0)
          {
            (void) CloneString(&image_info->size,(char *) NULL);
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !IsGeometry(argv[i]))
                  ThrowConvertException(OptionError,"MissingGeometry",option);
                (void) CloneString(&image_info->size,argv[i]);
              }
            break;
          }
        if (LocaleCompare("solarize",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  ThrowConvertException(OptionError,"MissingThreshold",option);
              }
            break;
          }
        if (LocaleCompare("spread",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  ThrowConvertException(OptionError,"MissingSpreadAmount",
                    option);
              }
            break;
          }
        if (LocaleCompare("stroke",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowConvertException(OptionError,"MissingColors",option);
              }
            break;
          }
        if (LocaleCompare("strokewidth",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  ThrowConvertException(OptionError,"MissingPointsize",option);
              }
            break;
          }
        if (LocaleCompare("swirl",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%lf",&sans))
                  ThrowConvertException(OptionError,"MissingImageRotation",
                    option);
              }
            break;
          }
        ThrowConvertException(OptionError,"UnrecognizedOption",option)
      }
      case 't':
      {
        if (LocaleCompare("temporary",option+1) == 0)
          {
            image_info->temporary=(*option == '-');
            break;
          }
        if (LocaleCompare("texture",option+1) == 0)
          {
            (void) CloneString(&image_info->texture,(char *) NULL);
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowConvertException(OptionError,"MissingFilename",option);
                (void) CloneString(&image_info->texture,argv[i]);
              }
            break;
          }
        if (LocaleCompare("threshold",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  ThrowConvertException(OptionError,"MissingThreshold",option);
              }
            break;
          }
        if (LocaleCompare("tile",option+1) == 0)
          {
            i++;
            if (i == argc)
              ThrowConvertException(OptionError,"MissingTile",option);
            break;
          }
        if (LocaleCompare("transform",option+1) == 0)
          break;
        if (LocaleCompare("transparent",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowConvertException(OptionError,"MissingTransparentColor",
                    option);
              }
            break;
          }
        if (LocaleCompare("treedepth",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  ThrowConvertException(OptionError,"MissingTreeDepth",option);
              }
            break;
          }
        if (LocaleCompare("trim",option+1) == 0)
          break;
        if (LocaleCompare("type",option+1) == 0)
          {
            image_info->type=UndefinedType;
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowConvertException(OptionError,"MissingType",option);
                option=argv[i];
                if (LocaleCompare("Bilevel",option) == 0)
                  image_info->type=BilevelType;
                if (LocaleCompare("Grayscale",option) == 0)
                  image_info->type=GrayscaleType;
                if (LocaleCompare("GrayscaleMatte",option) == 0)
                  image_info->type=GrayscaleMatteType;
                if (LocaleCompare("Palette",option) == 0)
                  image_info->type=PaletteType;
                if (LocaleCompare("PaletteMatte",option) == 0)
                  image_info->type=PaletteMatteType;
                if (LocaleCompare("TrueColor",option) == 0)
                  image_info->type=TrueColorType;
                if (LocaleCompare("TrueColorMatte",option) == 0)
                  image_info->type=TrueColorMatteType;
                if (LocaleCompare("ColorSeparation",option) == 0)
                  image_info->type=ColorSeparationType;
                if (LocaleCompare("ColorSeparationMatte",option) == 0)
                  image_info->type=ColorSeparationMatteType;
                if (LocaleCompare("Optimize",option) == 0)
                  image_info->type=OptimizeType;
                if (image_info->type == UndefinedType)
                  ThrowConvertException(OptionError,"UnrecognizedImageType",option);
              }
            break;
          }
        ThrowConvertException(OptionError,"UnrecognizedOption",option)
      }
      case 'u':
      {
        if (LocaleCompare("undercolor",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowConvertException(OptionError,"MissingColors",option);
              }
            break;
          }
        if (LocaleCompare("units",option+1) == 0)
          {
            image_info->units=UndefinedResolution;
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowConvertException(OptionError,"MissingType",option);
                option=argv[i];
                image_info->units=UndefinedResolution;
                if (LocaleCompare("PixelsPerInch",option) == 0)
                  image_info->units=PixelsPerInchResolution;
                if (LocaleCompare("PixelsPerCentimeter",option) == 0)
                  image_info->units=PixelsPerCentimeterResolution;
              }
            break;
          }
        if (LocaleCompare("unsharp",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%lf",&sans))
                  ThrowConvertException(OptionError,"MissingGeometry",option);
              }
            break;
          }
        ThrowConvertException(OptionError,"UnrecognizedOption",option)
      }
      case 'v':
      {
        if (LocaleCompare("verbose",option+1) == 0)
          {
            image_info->verbose=(*option == '-');
            break;
          }
        if (LocaleCompare("verbose",option+1) == 0)
          break;
        if (LocaleCompare("view",option+1) == 0)
          {
            (void) CloneString(&image_info->view,(char *) NULL);
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowConvertException(OptionError,"MissingViewTransform",
                    option);
                (void) CloneString(&image_info->view,argv[i]);
              }
            break;
          }
        if (LocaleCompare("virtual-pixel",option+1) == 0)
          {
            if (*option == '-')
              {
                VirtualPixelMethod
                  virtual_pixel_method;

                i++;
                if (i == argc)
                  ThrowConvertException(OptionError,"MissingVirtualPixelMethod",
                    option);
                option=argv[i];
                virtual_pixel_method=UndefinedVirtualPixelMethod;
                if (LocaleCompare("Constant",option) == 0)
                  virtual_pixel_method=ConstantVirtualPixelMethod;
                if (LocaleCompare("Edge",option) == 0)
                  virtual_pixel_method=EdgeVirtualPixelMethod;
                if (LocaleCompare("Mirror",option) == 0)
                  virtual_pixel_method=MirrorVirtualPixelMethod;
                if (LocaleCompare("Tile",option) == 0)
                  virtual_pixel_method=TileVirtualPixelMethod;
                if (virtual_pixel_method == UndefinedVirtualPixelMethod)
                  ThrowConvertException(OptionError,
                    "UnrecognizedVirtualPixelMethod",option);
              }
            break;
          }
        ThrowConvertException(OptionError,"UnrecognizedOption",option)
      }
      case 'w':
      {
        if (LocaleCompare("wave",option+1) == 0)
          {
            i++;
            if ((i == argc) || !sscanf(argv[i],"%ld",&x))
              ThrowConvertException(OptionError,"MissingAmplitude",option);
            break;
          }
        if (LocaleCompare("white-point",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  ThrowConvertException(OptionError,"MissingPoint",option);
              }
            break;
          }
        if (LocaleCompare("write",option+1) == 0)
          {
            i++;
            if (i == argc)
              ThrowConvertException(OptionError,"MissingFilename",option);
            break;
          }
        ThrowConvertException(OptionError,"UnrecognizedOption",option)
      }
      case '?':
        {
          ConvertUsage();
          ThrowConvertException(OptionError,"UsageError",NULL);
        }
      default:
        ThrowConvertException(OptionError,"UnrecognizedOption",option)
    }
  }
  if ((image == (Image *) NULL) && (image_list == (Image *) NULL))
    {
      if (exception->severity == UndefinedException)
        ThrowConvertException(OptionError,"MissingAnImageFilename",
          (char *) NULL);
      return(False);
    }
  if (i != (argc-1))
    ThrowConvertException(OptionError,"MissingAnImageFilename",(char *) NULL);
  if (image == (Image *) NULL)
    {
      status&=MogrifyImages(image_info,i-j,argv+j,&image_list);
      (void) CatchImageException(image_list);
    }
  else
    {
      status&=MogrifyImages(image_info,i-j,argv+j,&image);
      (void) CatchImageException(image);
      AppendImageToList(&image_list,image);
    }
  status&=WriteImages(image_info,image_list,argv[argc-1],exception);
  if (metadata != (char **) NULL)
    {
      char
        *text;

      text=TranslateText(image_info,image_list,(format != (char *) NULL) ? format : "%w,%h,%m");
      if (text == (char *) NULL)
        ThrowConvertException(ResourceLimitError,"MemoryAllocationFailed",
          "UnableToFormatImageMetadata");
      (void) ConcatenateString(&(*metadata),text);
      (void) ConcatenateString(&(*metadata),"\n");
      LiberateMemory((void **) &text);
    }
  DestroyImageList(image_list);
  LiberateArgumentList(argc,argv);
  return(status);
}
#undef ThrowConvertException

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I d e n t i f y I m a g e C o m m a n d                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IdentifyImageCommand() describes the format and characteristics of one or
%  more image files. It will also report if an image is incomplete or corrupt.
%  The information displayed includes the scene number, the file name, the
%  width and height of the image, whether the image is colormapped or not,
%  the number of colors in the image, the number of bytes in the image, the
%  format of the image (JPEG, PNM, etc.), and finally the number of seconds
%  it took to read and process the image.
%
%  The format of the IdentifyImageCommand method is:
%
%      unsigned int IdentifyImageCommand(ImageInfo *image_info,const int argc,
%        char **argv,char **metadata,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: The image info.
%
%    o argc: The number of elements in the argument vector.
%
%    o argv: A text array containing the command line arguments.
%
%    o metadata: any metadata is returned here.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
#define ThrowIdentifyException(code,reason,description) \
{ \
  if (format != (char *) NULL) \
    LiberateMemory((void **) &format); \
  DestroyImageList(image); \
  ThrowException(exception,code,reason,description); \
  LiberateArgumentList(argc,argv); \
  return(False); \
}
MagickExport unsigned int IdentifyImageCommand(ImageInfo *image_info,
  int argc,char **argv,char **metadata,ExceptionInfo *exception)
{
  char
    *format,
    *option,
    *q;

  Image
    *image;

  long
    count,
    number_images,
    x;

  register Image
    *p;

  register long
    i;

  unsigned int
    ping,
    status;

  /*
    Set defaults.
  */
  count=0;
  format=(char *) NULL;
  image=NewImageList();
  number_images=0;
  status=True;
  ping=True;

  /*
    Expand argument list
  */
  status=ExpandFilenames(&argc,&argv);
  if (status == False)
    MagickFatalError(ResourceLimitFatalError,"MemoryAllocationFailed",
      (char *) NULL);

  /*
    Check for sufficient arguments
  */  
  if (argc < 2 || ((argc < 3) && (LocaleCompare("-help",argv[1]) == 0 ||
      LocaleCompare("-?",argv[1]) == 0)))
    {
      IdentifyUsage();
      ThrowIdentifyException(OptionError,"UsageError",NULL);
    }

  for (i=1; i < argc; i++)
  {
    option=argv[i];
    if (LocaleCompare("-format",argv[i]) == 0)
      {
        i++;
        if (i == argc)
          ThrowIdentifyException(OptionError,"MissingFormatSpecifier",option);
        (void) CloneString(&format,argv[i]);
        break;
      }
  }
  for (i=1; i < argc; i++)
  {
    option=argv[i];
    if ((strlen(option) == 1) || ((*option != '-') && (*option != '+')))
      {
        /*
          Identify image.
        */
        (void) strncpy(image_info->filename,argv[i],MaxTextExtent-1);
        if (format != (char *) NULL)
          for (q=strchr(format,'%'); q != (char *) NULL; q=strchr(q+1,'%'))
            if ((*(q+1) == 'k') || (*(q+1) == 'q') || (*(q+1) == '#'))
              {
                ping=False;
                break;
              }
        if (image_info->verbose || !ping)
          image=ReadImage(image_info,exception);
        else
          image=PingImage(image_info,exception);
        status&=image != (Image *) NULL;
        if (image == (Image *) NULL)
          continue;
        for (p=image; p != (Image *) NULL; p=p->next)
        {
          if (p->scene == 0)
            p->scene=count++;
          if (format == (char *) NULL)
            {
              DescribeImage(p,stdout,image_info->verbose);
              continue;
            }
          if (metadata != (char **) NULL)
            {
              char
                *text;

              text=TranslateText(image_info,p,format);
              if (text == (char *) NULL)
                ThrowIdentifyException(ResourceLimitError,
                  "MemoryAllocationFailed","UnableToFormatImageMetadata");
              (void) ConcatenateString(&(*metadata),text);
              (void) ConcatenateString(&(*metadata),"\n");
              LiberateMemory((void **) &text);
            }
        }
        DestroyImageList(image);
        image=NewImageList();
        number_images++;
        continue;
      }
    switch(*(option+1))
    {
      case 'c':
      {
        if (LocaleCompare("cache",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  ThrowIdentifyException(OptionError,"MissingThreshold",option);
                SetMagickResourceLimit(MemoryResource,atol(argv[i]));
                SetMagickResourceLimit(MapResource,2*atol(argv[i]));
              }
            break;
          }
        ThrowIdentifyException(OptionError,"UnrecognizedOption",option)
      }
      case 'd':
      {
        if (LocaleCompare("debug",option+1) == 0)
          {
            (void) SetLogEventMask("None");
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowIdentifyException(OptionError,"MissingEventMask",option);
                (void) SetLogEventMask(argv[i]);
              }
            break;
          }
        if (LocaleCompare("density",option+1) == 0)
          {
            (void) CloneString(&image_info->density,(char *) NULL);
            if (*option == '-')
              {
              i++;
                if ((i == argc) || !IsGeometry(argv[i]))
                  ThrowIdentifyException(OptionError,"MissingGeometry",option);
                (void) CloneString(&image_info->density,argv[i]);
              }
            break;
          }
        if (LocaleCompare("depth",option+1) == 0)
          {
            image_info->depth=QuantumDepth;
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  ThrowIdentifyException(OptionError,"MissingImageDepth",
                    option);
                image_info->depth=atol(argv[i]);
              }
            break;
          }
        ThrowIdentifyException(OptionError,"UnrecognizedOption",option)
      }
      case 'f':
      {
        if (LocaleCompare("format",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowIdentifyException(OptionError,"MissingFormatSpecifier",
                    option);
              }
            break;
          }
        ThrowIdentifyException(OptionError,"UnrecognizedOption",option)
      }
      case 'h':
      {
        if (LocaleCompare("help",option+1) == 0)
          break;
        ThrowIdentifyException(OptionError,"UnrecognizedOption",option)
      }
      case 'i':
      {
        if (LocaleCompare("interlace",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowIdentifyException(OptionError,"MissingType",option);
                option=argv[i];
                image_info->interlace=UndefinedInterlace;
                if (LocaleCompare("None",option) == 0)
                  image_info->interlace=NoInterlace;
                if (LocaleCompare("Line",option) == 0)
                  image_info->interlace=LineInterlace;
                if (LocaleCompare("Plane",option) == 0)
                  image_info->interlace=PlaneInterlace;
                if (LocaleCompare("Partition",option) == 0)
                  image_info->interlace=PartitionInterlace;
                if (image_info->interlace == UndefinedInterlace)
                  ThrowIdentifyException(OptionError,
                    "UnrecognizedInterlaceType",option);
              }
            break;
          }
        ThrowIdentifyException(OptionError,"UnrecognizedOption",option)
      }
      case 'l':
      {
        if (LocaleCompare("log",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowIdentifyException(OptionError,"MissingLogFormat",option);
                (void) SetLogFormat(argv[i]);
              }
            break;
          }
        ThrowIdentifyException(OptionError,"UnrecognizedOption",option)
      }
      case 'p':
      {
        if (LocaleCompare("ping",option+1) == 0)
          break;
        ThrowIdentifyException(OptionError,"UnrecognizedOption",option)
      }
      case 's':
      {
        if (LocaleCompare("sampling-factor",option+1) == 0)
          {
            (void) CloneString(&image_info->sampling_factor,(char *) NULL);
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !IsGeometry(argv[i]))
                  ThrowIdentifyException(OptionError,"MissingGeometry",option);
                (void) CloneString(&image_info->sampling_factor,argv[i]);
              }
            break;
          }
        if (LocaleCompare("size",option+1) == 0)
          {
            (void) CloneString(&image_info->size,(char *) NULL);
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !IsGeometry(argv[i]))
                  ThrowIdentifyException(OptionError,"MissingGeometry",option);
                (void) CloneString(&image_info->size,argv[i]);
              }
            break;
          }
        ThrowIdentifyException(OptionError,"UnrecognizedOption",option)
      }
      case 'v':
      {
        if (LocaleCompare("verbose",option+1) == 0)
          {
            image_info->verbose=(*option == '-');
            break;
          }
        if (LocaleCompare("verbose",option+1) == 0)
          break;
        if (LocaleCompare("virtual-pixel",option+1) == 0)
          {
            if (*option == '-')
              {
                VirtualPixelMethod
                  virtual_pixel_method;

                i++;
                if (i == argc)
                  ThrowIdentifyException(OptionError,
                    "MissingVirtualPixelMethod",option);
                option=argv[i];
                virtual_pixel_method=UndefinedVirtualPixelMethod;
                if (LocaleCompare("Constant",option) == 0)
                  virtual_pixel_method=ConstantVirtualPixelMethod;
                if (LocaleCompare("Edge",option) == 0)
                  virtual_pixel_method=EdgeVirtualPixelMethod;
                if (LocaleCompare("Mirror",option) == 0)
                  virtual_pixel_method=MirrorVirtualPixelMethod;
                if (LocaleCompare("Tile",option) == 0)
                  virtual_pixel_method=TileVirtualPixelMethod;
                if (virtual_pixel_method == UndefinedVirtualPixelMethod)
                  ThrowIdentifyException(OptionError,
                    "UnrecognizedVirtualPixelMethod",option);
              }
            break;
          }
        ThrowIdentifyException(OptionError,"UnrecognizedOption",option)
      }
      case '?':
        break;
      default:
        ThrowIdentifyException(OptionError,"UnrecognizedOption",option)
    }
  }
  if (number_images == 0)
    {
      if (exception->severity == UndefinedException)
        ThrowIdentifyException(OptionError,"MissingAnImageFilename",
          (char *) NULL);
      return(False);
    }
  if (i != argc)
    ThrowIdentifyException(OptionError,"MissingAnImageFilename",(char *) NULL);
  if (format != (char *) NULL)
    LiberateMemory((void **) &format);
  DestroyImageList(image);
  LiberateArgumentList(argc,argv);
  return(status);
}
#undef ThrowIdentifyException

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
+     M o g r i f y I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method MogrifyImage applies image processing options to an image as
%  prescribed by command line options.
%
%  The format of the MogrifyImage method is:
%
%      unsigned int MogrifyImage(const ImageInfo *image_info,const int argc,
%        char **argv,Image **image)
%
%  A description of each parameter follows:
%
%    o image_info: The image info..
%
%    o argc: Specifies a pointer to an integer describing the number of
%      elements in the argument vector.
%
%    o argv: Specifies a pointer to a text array containing the command line
%      arguments.
%
%    o image: The image;  returned from
%      ReadImage.
%
%
*/
MagickExport unsigned int MogrifyImage(const ImageInfo *image_info,
  int argc,char **argv,Image **image)
{
  char
    *option;

  DrawInfo
    *draw_info;

  Image
    *map_image,
    *region_image;

  ImageInfo
    *clone_info;

  int
    count;

  QuantizeInfo
    quantize_info;

  RectangleInfo
    geometry,
    region_geometry;

  register long
    i;

  unsigned int
    matte;

  /*
    Verify option length.
  */
  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  assert(image != (Image **) NULL);
  assert((*image)->signature == MagickSignature);
  if (argc <= 0)
    return(True);
  for (i=0; i < argc; i++)
    if (strlen(argv[i]) > (MaxTextExtent/2-1))
      MagickFatalError(OptionFatalError,"OptionLengthExceedsLimit",argv[i]);
  /*
    Initialize method variables.
  */
  clone_info=CloneImageInfo(image_info);
  draw_info=CloneDrawInfo(clone_info,(DrawInfo *) NULL);
  GetQuantizeInfo(&quantize_info);
  map_image=(Image *) NULL;
  quantize_info.number_colors=0;
  quantize_info.tree_depth=0;
  quantize_info.dither=True;
  if (clone_info->monochrome)
    if (!IsMonochromeImage(*image,&(*image)->exception))
      {
        quantize_info.number_colors=2;
        quantize_info.tree_depth=8;
        quantize_info.colorspace=GRAYColorspace;
      }
  SetGeometry(*image,&region_geometry);
  region_image=(Image *) NULL;
  /*
    Transmogrify the image.
  */
  for (i=0; i < argc; i++)
  {
    option=argv[i];
    if ((strlen(option) <= 1) || ((*option != '-') && (*option != '+')))
      continue;
    switch (*(option+1))
    {
      case 'a':
      {
        if (LocaleCompare("affine",option+1) == 0)
          {
            char
              *p;

            /*
              Draw affine matrix.
            */
            if (*option == '+')
              {
                IdentityAffine(&draw_info->affine);
                continue;
              }
            p=argv[++i];
            draw_info->affine.sx=strtod(p,&p);
            if (*p ==',')
              p++;
            draw_info->affine.rx=strtod(p,&p);
            if (*p ==',')
              p++;
            draw_info->affine.ry=strtod(p,&p);
            if (*p ==',')
              p++;
            draw_info->affine.sy=strtod(p,&p);
            if (*p ==',')
              p++;
            draw_info->affine.tx=strtod(p,&p);
            if (*p ==',')
              p++;
            draw_info->affine.ty=strtod(p,&p);
            break;
          }
        if (LocaleCompare("antialias",option+1) == 0)
          {
            clone_info->antialias=(*option == '-');
            draw_info->stroke_antialias=(*option == '-');
            draw_info->text_antialias=(*option == '-');
            break;
          }
        break;
      }
      case 'b':
      {
        if (LocaleCompare("background",option+1) == 0)
          {
            (void) QueryColorDatabase(argv[++i],&clone_info->background_color,
              &(*image)->exception);
            (*image)->background_color=clone_info->background_color;
            continue;
          }
        if (LocaleCompare("blue-primary",option+1) == 0)
          {
            /*
              Blue chromaticity primary point.
            */
            if (*option == '+')
              {
                (*image)->chromaticity.blue_primary.x=0.0;
                (*image)->chromaticity.blue_primary.y=0.0;
                continue;
              }
            (void) sscanf(argv[++i],"%lf%*[,/]%lf",
              &(*image)->chromaticity.blue_primary.x,
              &(*image)->chromaticity.blue_primary.y);
            continue;
          }
        if (LocaleCompare("blur",option+1) == 0)
          {
            double
              radius,
              sigma;

            Image
              *blur_image;

            /*
              Gaussian blur image.
            */
            radius=0.0;
            sigma=1.0;
            (void) sscanf(argv[++i],"%lfx%lf",&radius,&sigma);
            blur_image=BlurImage(*image,radius,sigma,&(*image)->exception);
            if (blur_image == (Image *) NULL)
              break;
            DestroyImage(*image);
            *image=blur_image;
            continue;
          }
        if (LocaleCompare("border",option+1) == 0)
          {
            Image
              *border_image;

            /*
              Surround image with a border of solid color.
            */
            (void) GetImageGeometry(*image,argv[++i],False,&geometry);
            border_image=BorderImage(*image,&geometry,&(*image)->exception);
            if (border_image == (Image *) NULL)
              break;
            DestroyImage(*image);
            *image=border_image;
            continue;
          }
        if (LocaleCompare("bordercolor",option+1) == 0)
          {
            (void) QueryColorDatabase(argv[++i],&clone_info->border_color,
              &(*image)->exception);
            draw_info->border_color=clone_info->border_color;
            (*image)->border_color=clone_info->border_color;
            continue;
          }
        if (LocaleCompare("box",option+1) == 0)
          {
            (void) QueryColorDatabase(argv[++i],&draw_info->undercolor,
              &(*image)->exception);
            continue;
          }
        break;
      }
      case 'c':
      {
        if (LocaleCompare("channel",option+1) == 0)
          {
            ChannelType
              channel;

            option=argv[++i];
            channel=UndefinedChannel;
            if (LocaleCompare("Red",option) == 0)
              channel=RedChannel;
            if (LocaleCompare("Cyan",option) == 0)
              channel=CyanChannel;
            if (LocaleCompare("Green",option) == 0)
              channel=GreenChannel;
            if (LocaleCompare("Magenta",option) == 0)
              channel=MagentaChannel;
            if (LocaleCompare("Blue",option) == 0)
              channel=BlueChannel;
            if (LocaleCompare("Yellow",option) == 0)
              channel=YellowChannel;
            if (LocaleCompare("Opacity",option) == 0)
              channel=OpacityChannel;
            if (LocaleCompare("Black",option) == 0)
              channel=BlackChannel;
            if (LocaleCompare("Matte",option) == 0)
              channel=MatteChannel;
            TransformColorspace(*image,clone_info->colorspace);
            (void) ChannelImage(*image,channel);
            continue;
          }
        if (LocaleCompare("charcoal",option+1) == 0)
          {
            double
              radius,
              sigma;

            Image
              *charcoal_image;

            /*
              Charcoal image.
            */
            radius=0.0;
            sigma=1.0;
            (void) sscanf(argv[++i],"%lfx%lf",&radius,&sigma);
            charcoal_image=
              CharcoalImage(*image,radius,sigma,&(*image)->exception);
            if (charcoal_image == (Image *) NULL)
              break;
            DestroyImage(*image);
            *image=charcoal_image;
            continue;
          }
        if (LocaleCompare("chop",option+1) == 0)
          {
            Image
              *chop_image;

            /*
              Chop the image.
            */
            (void) GetImageGeometry(*image,argv[++i],False,&geometry);
            chop_image=ChopImage(*image,&geometry,&(*image)->exception);
            if (chop_image == (Image *) NULL)
              break;
            DestroyImage(*image);
            *image=chop_image;
            continue;
          }
        if (LocaleCompare("clip",option+1) == 0)
          {
            if (*option == '+')
              {
                (void) SetImageClipMask(*image,(Image *) NULL);
                continue;
              }
            (void) ClipImage(*image);
            continue;
          }
        if (LocaleCompare("colorize",option+1) == 0)
          {
            Image
              *colorize_image;

            /*
              Colorize the image.
            */
            colorize_image=ColorizeImage(*image,argv[++i],draw_info->fill,
              &(*image)->exception);
            if (colorize_image == (Image *) NULL)
              break;
            DestroyImage(*image);
            *image=colorize_image;
            continue;
          }
        if (LocaleCompare("colors",option+1) == 0)
          {
            quantize_info.number_colors=atol(argv[++i]);
            continue;
          }
        if (LocaleCompare("colorspace",option+1) == 0)
          {
            char
              type;

            type=(*option);
            option=argv[++i];
            if (LocaleCompare("cmyk",option) == 0)
              {
                TransformColorspace(*image,CMYKColorspace);
                /* Ignore request to quantize in CMYK colorspace */
                quantize_info.colorspace=RGBColorspace;
              }
            if (LocaleCompare("gray",option) == 0)
              {
                TransformColorspace(*image,GRAYColorspace);
                quantize_info.colorspace=GRAYColorspace;
              }
            if (LocaleCompare("hsl",option) == 0)
              quantize_info.colorspace=HSLColorspace;
            if (LocaleCompare("hwb",option) == 0)
              quantize_info.colorspace=HWBColorspace;
            if (LocaleCompare("ohta",option) == 0)
              quantize_info.colorspace=OHTAColorspace;
            if (LocaleCompare("rgb",option) == 0)
              {
                TransformColorspace(*image,RGBColorspace);
                quantize_info.colorspace=RGBColorspace;
              }
            if (LocaleCompare("srgb",option) == 0)
              quantize_info.colorspace=sRGBColorspace;
            if (LocaleCompare("transparent",option) == 0)
              quantize_info.colorspace=TransparentColorspace;
            if (LocaleCompare("xyz",option) == 0)
              quantize_info.colorspace=XYZColorspace;
            if (LocaleCompare("ycbcr",option) == 0)
              quantize_info.colorspace=YCbCrColorspace;
            if (LocaleCompare("ycc",option) == 0)
              quantize_info.colorspace=YCCColorspace;
            if (LocaleCompare("yiq",option) == 0)
              quantize_info.colorspace=YIQColorspace;
            if (LocaleCompare("ypbpr",option) == 0)
              quantize_info.colorspace=YPbPrColorspace;
            if (LocaleCompare("yuv",option) == 0)
              quantize_info.colorspace=YUVColorspace;
            clone_info->colorspace=quantize_info.colorspace;
            if (type == '+')
              (*image)->colorspace=clone_info->colorspace;
            continue;
          }
        if (LocaleCompare("comment",option+1) == 0)
          {
            (void) SetImageAttribute(*image,"comment",(char *) NULL);
            if (*option == '-')
              (void) SetImageAttribute(*image,"comment",argv[++i]);
            continue;
          }
        if (LocaleCompare("compose",option+1) == 0)
          {
            CompositeOperator
              compose;

            option=argv[++i];
            compose=OverCompositeOp;
            if (LocaleCompare("Over",option) == 0)
              compose=OverCompositeOp;
            if (LocaleCompare("In",option) == 0)
              compose=InCompositeOp;
            if (LocaleCompare("Out",option) == 0)
              compose=OutCompositeOp;
            if (LocaleCompare("Atop",option) == 0)
              compose=AtopCompositeOp;
            if (LocaleCompare("Xor",option) == 0)
              compose=XorCompositeOp;
            if (LocaleCompare("Plus",option) == 0)
              compose=PlusCompositeOp;
            if (LocaleCompare("Minus",option) == 0)
              compose=MinusCompositeOp;
            if (LocaleCompare("Add",option) == 0)
              compose=AddCompositeOp;
            if (LocaleCompare("Subtract",option) == 0)
              compose=SubtractCompositeOp;
            if (LocaleCompare("Difference",option) == 0)
              compose=DifferenceCompositeOp;
            if (LocaleCompare("Multiply",option) == 0)
              compose=MultiplyCompositeOp;
            if (LocaleCompare("Bumpmap",option) == 0)
              compose=BumpmapCompositeOp;
            if (LocaleCompare("Copy",option) == 0)
              compose=CopyCompositeOp;
            if (LocaleCompare("CopyRed",option) == 0)
              compose=CopyRedCompositeOp;
            if (LocaleCompare("CopyGreen",option) == 0)
              compose=CopyGreenCompositeOp;
            if (LocaleCompare("CopyBlue",option) == 0)
              compose=CopyBlueCompositeOp;
            if (LocaleCompare("CopyOpacity",option) == 0)
              compose=CopyOpacityCompositeOp;
            if (LocaleCompare("Clear",option) == 0)
              compose=ClearCompositeOp;
            (*image)->compose=compose;
            break;
          }
        if (LocaleCompare("compress",option+1) == 0)
          {
            CompressionType
              compression;

            if (*option == '+')
              {
                (*image)->compression=UndefinedCompression;
                continue;
              }
            option=argv[++i];
            compression=UndefinedCompression;
            if (LocaleCompare("None",option) == 0)
              compression=NoCompression;
            if (LocaleCompare("BZip",option) == 0)
              compression=BZipCompression;
            if (LocaleCompare("Fax",option) == 0)
              compression=FaxCompression;
            if (LocaleCompare("Group4",option) == 0)
              compression=Group4Compression;
            if (LocaleCompare("JPEG",option) == 0)
              compression=JPEGCompression;
            if (LocaleCompare("Lossless",option) == 0)
              compression=LosslessJPEGCompression;
            if (LocaleCompare("LZW",option) == 0)
              compression=LZWCompression;
            if (LocaleCompare("RLE",option) == 0)
              compression=RLECompression;
            if (LocaleCompare("Zip",option) == 0)
              compression=ZipCompression;
            (*image)->compression=compression;
            continue;
          }
        if (LocaleCompare("contrast",option+1) == 0)
          {
            (void) ContrastImage(*image,*option == '-');
            continue;
          }
        if (LocaleCompare("convolve",option+1) == 0)
          {
            char
              *p,
              token[MaxTextExtent];

            double
              *kernel;

            register long
              x;

            unsigned int
              order;

            /*
              Convolve image.
            */
            p=argv[++i];
            for (x=0; *p != '\0'; x++)
            {
              GetToken(p,&p,token);
              if (*token == ',')
                GetToken(p,&p,token);
            }
            order=(unsigned int) sqrt(x+1);
            kernel=(double *) AcquireMemory(order*order*sizeof(double));
            if (kernel == (double *) NULL)
              MagickFatalError(ResourceLimitFatalError,"MemoryAllocationFailed",
                "UnableToAllocateCoefficients");
            p=argv[i];
            for (x=0; *p != '\0'; x++)
            {
              GetToken(p,&p,token);
              if (*token == ',')
                GetToken(p,&p,token);
              kernel[x]=atof(token);
            }
            for ( ; x < (long) (order*order); x++)
              kernel[x]=0.0;
            (void) ConvolveImage(*image,order,kernel,&(*image)->exception);
            LiberateMemory((void **) &kernel);
            continue;
          }
        if (LocaleCompare("crop",option+1) == 0)
          {
            TransformImage(image,argv[++i],(char *) NULL);
            continue;
          }
        if (LocaleCompare("cycle",option+1) == 0)
          {
            /*
              Cycle an image colormap.
            */
            CycleColormapImage(*image,atoi(argv[++i]));
            continue;
          }
        break;
      }
      case 'd':
      {
        if (LocaleCompare("delay",option+1) == 0)
          {
            double
              maximum_delay,
              minimum_delay;

            /*
              Set image delay.
            */
            if (*option == '+')
              {
                (*image)->delay=0;
                continue;
              }
            count=sscanf(argv[++i],"%lf-%lf",&minimum_delay,&maximum_delay);
            if (count == 1)
              (*image)->delay=(unsigned long) minimum_delay;
            else
              {
                if ((*image)->delay < minimum_delay)
                  (*image)->delay=(unsigned long) minimum_delay;
                if ((*image)->delay > maximum_delay)
                  (*image)->delay=(unsigned long) maximum_delay;
              }
            continue;
          }
        if (LocaleCompare("density",option+1) == 0)
          {
            /*
              Set image density.
            */
            (void) CloneString(&clone_info->density,argv[++i]);
            (void) CloneString(&draw_info->density,clone_info->density);
            count=sscanf(clone_info->density,"%lfx%lf",
              &(*image)->x_resolution,&(*image)->y_resolution);
            if (count != 2)
              (*image)->y_resolution=(*image)->x_resolution;
            continue;
          }
        if (LocaleCompare("depth",option+1) == 0)
          {
            (void) SetImageDepth(*image,atol(argv[++i]));
            continue;
          }
        if (LocaleCompare("despeckle",option+1) == 0)
          {
            Image
              *despeckle_image;

            /*
              Reduce the speckles within an image.
            */
            despeckle_image=DespeckleImage(*image,&(*image)->exception);
            if (despeckle_image == (Image *) NULL)
              break;
            DestroyImage(*image);
            *image=despeckle_image;
            continue;
          }
        if (LocaleCompare("display",option+1) == 0)
          {
            (void) CloneString(&clone_info->server_name,argv[++i]);
            (void) CloneString(&draw_info->server_name,clone_info->server_name);
            continue;
          }
        if (LocaleCompare("dispose",option+1) == 0)
          {
            DisposeType
              dispose;;

            if (*option == '+')
              {
                (*image)->dispose=UndefinedDispose;
                continue;
              }
            option=argv[++i];
            dispose=UndefinedDispose;
            if (LocaleCompare("0",option) == 0)
              dispose=UndefinedDispose;
            if (LocaleCompare("1",option) == 0)
              dispose=NoneDispose;
            if (LocaleCompare("2",option) == 0)
              dispose=BackgroundDispose;
            if (LocaleCompare("3",option) == 0)
              dispose=PreviousDispose;
            if (LocaleCompare("Background",option) == 0)
              dispose=BackgroundDispose;
            if (LocaleCompare("None",option) == 0)
              dispose=NoneDispose;
            if (LocaleCompare("Previous",option) == 0)
              dispose=PreviousDispose;
            if (LocaleCompare("Undefined",option) == 0)
              dispose=UndefinedDispose;
            (*image)->dispose=dispose;
            continue;
          }
        if (LocaleCompare("dither",option+1) == 0)
          {
            clone_info->dither=(*option == '-');
            quantize_info.dither=clone_info->dither;
            continue;
          }
        if (LocaleCompare("draw",option+1) == 0)
          {
            /*
              Draw image.
            */
            (void) CloneString(&draw_info->primitive,argv[++i]);
            (void) DrawImage(*image,draw_info);
            continue;
          }
        break;
      }
      case 'e':
      {
        if (LocaleCompare("edge",option+1) == 0)
          {
            double
              radius;

            Image
              *edge_image;

            /*
              Enhance edges in the image.
            */
            radius=atof(argv[++i]);
            edge_image=EdgeImage(*image,radius,&(*image)->exception);
            if (edge_image == (Image *) NULL)
              break;
            DestroyImage(*image);
            *image=edge_image;
            continue;
          }
        if (LocaleCompare("emboss",option+1) == 0)
          {
            double
              radius,
              sigma;

            Image
              *emboss_image;

            /*
              Emboss the image.
            */
            radius=0.0;
            sigma=1.0;
            (void) sscanf(argv[++i],"%lfx%lf",&radius,&sigma);
            emboss_image=EmbossImage(*image,radius,sigma,&(*image)->exception);
            if (emboss_image == (Image *) NULL)
              break;
            DestroyImage(*image);
            *image=emboss_image;
            continue;
          }
        if (LocaleCompare("encoding",option+1) == 0)
          {
            (void) CloneString(&draw_info->encoding,argv[++i]);
            continue;
          }
        if (LocaleCompare("endian",option+1) == 0)
          {
            EndianType
              endian;

            if (*option == '+')
              {
                clone_info->endian=LSBEndian;
                continue;
              }
            option=argv[++i];
            endian=UndefinedEndian;
            if (LocaleCompare("LSB",option) == 0)
              endian=LSBEndian;
            if (LocaleCompare("MSB",option) == 0)
              endian=MSBEndian;
            clone_info->endian=endian;
            continue;
          }
        if (LocaleCompare("enhance",option+1) == 0)
          {
            Image
              *enhance_image;

            /*
              Enhance image.
            */
            enhance_image=EnhanceImage(*image,&(*image)->exception);
            if (enhance_image == (Image *) NULL)
              break;
            DestroyImage(*image);
            *image=enhance_image;
            continue;
          }
        if (LocaleCompare("equalize",option+1) == 0)
          {
            /*
              Equalize image.
            */
            (void) EqualizeImage(*image);
            continue;
          }
        break;
      }
      case 'f':
      {
        if (LocaleCompare("fill",option+1) == 0)
          {
            (void) QueryColorDatabase(argv[++i],&draw_info->fill,
              &(*image)->exception);
            continue;
          }
        if (LocaleCompare("filter",option+1) == 0)
          {
            FilterTypes
              filter;

            if (*option == '+')
              {
                (*image)->filter=DefaultResizeFilter;
                continue;
              }
            option=argv[++i];
            filter=LanczosFilter;
            if (LocaleCompare("Point",option) == 0)
              filter=PointFilter;
            if (LocaleCompare("Box",option) == 0)
              filter=BoxFilter;
            if (LocaleCompare("Triangle",option) == 0)
              filter=TriangleFilter;
            if (LocaleCompare("Hermite",option) == 0)
              filter=HermiteFilter;
            if (LocaleCompare("Hanning",option) == 0)
              filter=HanningFilter;
            if (LocaleCompare("Hamming",option) == 0)
              filter=HammingFilter;
            if (LocaleCompare("Blackman",option) == 0)
              filter=BlackmanFilter;
            if (LocaleCompare("Gaussian",option) == 0)
              filter=GaussianFilter;
            if (LocaleCompare("Quadratic",option) == 0)
              filter=QuadraticFilter;
            if (LocaleCompare("Cubic",option) == 0)
              filter=CubicFilter;
            if (LocaleCompare("Catrom",option) == 0)
              filter=CatromFilter;
            if (LocaleCompare("Mitchell",option) == 0)
              filter=MitchellFilter;
            if (LocaleCompare("Lanczos",option) == 0)
              filter=LanczosFilter;
            if (LocaleCompare("Bessel",option) == 0)
              filter=BesselFilter;
            if (LocaleCompare("Sinc",option) == 0)
              filter=SincFilter;
            (*image)->filter=filter;
            continue;
          }
        if (LocaleCompare("flip",option+1) == 0)
          {
            Image
              *flip_image;

            /*
              Flip image scanlines.
            */
            flip_image=FlipImage(*image,&(*image)->exception);
            if (flip_image == (Image *) NULL)
              break;
            DestroyImage(*image);
            *image=flip_image;
            continue;
          }
        if (LocaleCompare("flop",option+1) == 0)
          {
            Image
              *flop_image;

            /*
              Flop image scanlines.
            */
            flop_image=FlopImage(*image,&(*image)->exception);
            if (flop_image == (Image *) NULL)
              break;
            DestroyImage(*image);
            *image=flop_image;
            continue;
          }
        if (LocaleCompare("frame",option+1) == 0)
          {
            Image
              *frame_image;

            FrameInfo
              frame_info;

            /*
              Surround image with an ornamental border.
            */
            (void) GetImageGeometry(*image,argv[++i],False,&geometry);
            frame_info.width=geometry.width;
            frame_info.height=geometry.height;
            frame_info.outer_bevel=geometry.x;
            frame_info.inner_bevel=geometry.y;
            frame_info.x=(long) frame_info.width;
            frame_info.y=(long) frame_info.height;
            frame_info.width=(*image)->columns+2*frame_info.width;
            frame_info.height=(*image)->rows+2*frame_info.height;
            frame_image=FrameImage(*image,&frame_info,&(*image)->exception);
            if (frame_image == (Image *) NULL)
              break;
            DestroyImage(*image);
            *image=frame_image;
            continue;
          }
        if (LocaleCompare("fuzz",option+1) == 0)
          {
            (*image)->fuzz=StringToDouble(argv[++i],MaxRGB);
            continue;
          }
        if (LocaleCompare("font",option+1) == 0)
          {
            (void) CloneString(&clone_info->font,argv[++i]);
            (void) CloneString(&draw_info->font,clone_info->font);
            continue;
          }
        break;
      }
      case 'g':
      {
        if (LocaleCompare("gamma",option+1) == 0)
          {
            if (*option == '+')
              (*image)->gamma=atof(argv[++i]);
            else
              (void) GammaImage(*image,argv[++i]);
            continue;
          }
        if (LocaleCompare("gaussian",option+1) == 0)
          {
            double
              radius,
              sigma;

            Image
              *blur_image;

            /*
              Gaussian blur image.
            */
            radius=0.0;
            sigma=1.0;
            (void) sscanf(argv[++i],"%lfx%lf",&radius,&sigma);
            blur_image=
              GaussianBlurImage(*image,radius,sigma,&(*image)->exception);
            if (blur_image == (Image *) NULL)
              break;
            DestroyImage(*image);
            *image=blur_image;
            continue;
          }
        if (LocaleCompare("geometry",option+1) == 0)
          {
            Image
              *zoom_image;

            /*
              Resize image.
            */
            (void) GetImageGeometry(*image,argv[++i],True,&geometry);
            if ((geometry.width == (*image)->columns) &&
                (geometry.height == (*image)->rows))
              break;
            zoom_image=ZoomImage(*image,geometry.width,geometry.height,
              &(*image)->exception);
            if (zoom_image == (Image *) NULL)
              break;
            DestroyImage(*image);
            *image=zoom_image;
            continue;
          }
        if (LocaleCompare("gravity",option+1) == 0)
          {
            GravityType
              gravity;

            if (*option == '+')
              {
                draw_info->gravity=(GravityType) ForgetGravity;
                (*image)->gravity=(GravityType) ForgetGravity;
                continue;
              }
            option=argv[++i];
            gravity=(GravityType) ForgetGravity;
            if (LocaleCompare("NorthWest",option) == 0)
              gravity=(GravityType) NorthWestGravity;
            if (LocaleCompare("North",option) == 0)
              gravity=(GravityType) NorthGravity;
            if (LocaleCompare("NorthEast",option) == 0)
              gravity=(GravityType) NorthEastGravity;
            if (LocaleCompare("West",option) == 0)
              gravity=(GravityType) WestGravity;
            if (LocaleCompare("Center",option) == 0)
              gravity=(GravityType) CenterGravity;
            if (LocaleCompare("East",option) == 0)
              gravity=(GravityType) EastGravity;
            if (LocaleCompare("SouthWest",option) == 0)
              gravity=(GravityType) SouthWestGravity;
            if (LocaleCompare("South",option) == 0)
              gravity=(GravityType) SouthGravity;
            if (LocaleCompare("SouthEast",option) == 0)
              gravity=(GravityType) SouthEastGravity;
            draw_info->gravity=gravity;
            (*image)->gravity=gravity;
            continue;
          }
        if (LocaleCompare("green-primary",option+1) == 0)
          {
            /*
              Green chromaticity primary point.
            */
            if (*option == '+')
              {
                (*image)->chromaticity.green_primary.x=0.0;
                (*image)->chromaticity.green_primary.y=0.0;
                continue;
              }
            (void) sscanf(argv[++i],"%lf%*[,/]%lf",
              &(*image)->chromaticity.green_primary.x,
              &(*image)->chromaticity.green_primary.y);
            continue;
          }
        break;
      }
      case 'i':
      {
        if (LocaleCompare("implode",option+1) == 0)
          {
            double
              amount;

            Image
              *implode_image;

            /*
              Implode image.
            */
            amount=atof(argv[++i]);
            implode_image=ImplodeImage(*image,amount,&(*image)->exception);
            if (implode_image == (Image *) NULL)
              break;
            DestroyImage(*image);
            *image=implode_image;
            continue;
          }
        if (LocaleCompare("interlace",option+1) == 0)
          {
            InterlaceType
              interlace;

            if (*option == '+')
              {
                clone_info->interlace=NoInterlace;
                continue;
              }
            option=argv[++i];
            interlace=UndefinedInterlace;
            if (LocaleCompare("None",option) == 0)
              interlace=NoInterlace;
            if (LocaleCompare("Line",option) == 0)
              interlace=LineInterlace;
            if (LocaleCompare("Plane",option) == 0)
              interlace=PlaneInterlace;
            if (LocaleCompare("Partition",option) == 0)
              interlace=PartitionInterlace;
            clone_info->interlace=interlace;
            continue;
          }
        if (LocaleCompare("intent",option+1) == 0)
          {
            RenderingIntent
              rendering_intent;

            if (*option == '+')
              {
                (*image)->rendering_intent=UndefinedIntent;
                continue;
              }
            option=argv[++i];
            rendering_intent=UndefinedIntent;
            if (LocaleCompare("Absolute",option) == 0)
              rendering_intent=AbsoluteIntent;
            if (LocaleCompare("Perceptual",option) == 0)
              rendering_intent=PerceptualIntent;
            if (LocaleCompare("Relative",option) == 0)
              rendering_intent=RelativeIntent;
            if (LocaleCompare("Saturation",option) == 0)
              rendering_intent=SaturationIntent;
            (*image)->rendering_intent=rendering_intent;
            continue;
          }
        break;
      }
      case 'l':
      {
        if (LocaleCompare("label",option+1) == 0)
          {
            (void) SetImageAttribute(*image,"label",(char *) NULL);
            if (*option == '-')
              (void) SetImageAttribute(*image,"label",argv[++i]);
            continue;
          }
        if (LocaleCompare("lat",option+1) == 0)
          {
            Image
              *threshold_image;

            double
              offset;

            unsigned long
              height,
              width;

            /*
              Local adaptive threshold image.
            */
            offset=0;
            height=3;
            width=3;
            (void) sscanf(argv[++i],"%lux%lu%lf",&width,&height,&offset);
            if (strchr(argv[i],'%') != (char *) NULL)
              offset*=((double) MaxRGB/100.0);
            threshold_image=AdaptiveThresholdImage(*image,width,height,offset,
              &(*image)->exception);
            if (threshold_image == (Image *) NULL)
              break;
            DestroyImage(*image);
            *image=threshold_image;
            continue;
          }
        if (LocaleCompare("level",option+1) == 0)
          {
            (void) LevelImage(*image,argv[++i]);
            continue;
          }
        if (LocaleCompare("linewidth",option+1) == 0)
          {
            draw_info->stroke_width=atof(argv[++i]);
            continue;
          }
        if (LocaleCompare("loop",option+1) == 0)
          {
            /*
              Set image iterations.
            */
            (*image)->iterations=atol(argv[++i]);
            continue;
          }
        break;
      }
      case 'm':
      {
        if (LocaleCompare("map",option+1) == 0)
          {
            /*
              Transform image colors to match this set of colors.
            */
            if (*option == '+')
              continue;
            (void) strncpy(clone_info->filename,argv[++i],MaxTextExtent-1);
            map_image=ReadImage(clone_info,&(*image)->exception);
            continue;
          }
        if (LocaleCompare("mask",option+1) == 0)
          {
            Image
              *mask;

            long
              y;

            register long
              x;

            register PixelPacket
              *q;

            if (*option == '+')
              {
                /*
                  Remove a clip mask.
                */
                (void) SetImageClipMask(*image,(Image *) NULL);
                continue;
              }
            /*
              Set the image clip mask.
            */
            (void) strncpy(clone_info->filename,argv[++i],MaxTextExtent-1);
            mask=ReadImage(clone_info,&(*image)->exception);
            if (mask == (Image *) NULL)
              continue;
            for (y=0; y < (long) mask->rows; y++)
            {
              q=GetImagePixels(mask,0,y,mask->columns,1);
              if (q == (PixelPacket *) NULL)
                break;
              for (x=0; x < (long) mask->columns; x++)
              {
                if (!mask->matte)
                  q->opacity=PixelIntensityToQuantum(q);
                q->red=q->opacity;
                q->green=q->opacity;
                q->blue=q->opacity;
                q++;
              }
              if (!SyncImagePixels(mask))
                break;
            }
            SetImageType(mask,TrueColorMatteType);
            (void) SetImageClipMask(*image,mask);
          }
        if (LocaleCompare("matte",option+1) == 0)
          {
            if (*option == '-')
              if (!(*image)->matte)
                SetImageOpacity(*image,OpaqueOpacity);
            (*image)->matte=(*option == '-');
            continue;
          }
        if (LocaleCompare("mattecolor",option+1) == 0)
          {
            (void) QueryColorDatabase(argv[++i],&clone_info->matte_color,
              &(*image)->exception);
            (*image)->matte_color=clone_info->matte_color;
            continue;
          }
        if (LocaleCompare("median",option+1) == 0)
          {
            double
              radius;

            Image
              *median_image;

            /*
              Median filter image.
            */
            radius=atof(argv[++i]);
            median_image=MedianFilterImage(*image,radius,&(*image)->exception);
            if (median_image == (Image *) NULL)
              break;
            DestroyImage(*image);
            *image=median_image;
            continue;
          }
        if (LocaleCompare("modulate",option+1) == 0)
          {
            (void) ModulateImage(*image,argv[++i]);
            continue;
          }
        if ((LocaleCompare("mono",option+1) == 0) ||
            (LocaleCompare("monochrome",option+1) == 0))
          {
            clone_info->monochrome=True;
            quantize_info.number_colors=2;
            quantize_info.tree_depth=8;
            quantize_info.colorspace=GRAYColorspace;
            continue;
          }
        break;
      }
      case 'n':
      {
        if (LocaleCompare("negate",option+1) == 0)
          {
            (void) NegateImage(*image,*option == '+');
            continue;
          }
        if (LocaleCompare("noise",option+1) == 0)
          {
            Image
              *noisy_image;

            if (*option == '-')
              noisy_image=
                ReduceNoiseImage(*image,atol(argv[++i]),&(*image)->exception);
            else
              {
                NoiseType
                  noise_type;

                /*
                  Add noise to image.
                */
                option=argv[++i];
                noise_type=UniformNoise;
                if (LocaleCompare("Gaussian",option) == 0)
                  noise_type=GaussianNoise;
                if (LocaleCompare("multiplicative",option) == 0)
                  noise_type=MultiplicativeGaussianNoise;
                if (LocaleCompare("impulse",option) == 0)
                  noise_type=ImpulseNoise;
                if (LocaleCompare("laplacian",option) == 0)
                  noise_type=LaplacianNoise;
                if (LocaleCompare("Poisson",option) == 0)
                  noise_type=PoissonNoise;
                noisy_image=
                  AddNoiseImage(*image,noise_type,&(*image)->exception);
              }
            if (noisy_image == (Image *) NULL)
              break;
            DestroyImage(*image);
            *image=noisy_image;
            continue;
          }
        if (LocaleCompare("normalize",option+1) == 0)
          {
            (void) NormalizeImage(*image);
            continue;
          }
        break;
      }
      case 'o':
      {
        if (LocaleCompare("opaque",option+1) == 0)
          {
            PixelPacket
              target;

            target=AcquireOnePixel(*image,0,0,&(*image)->exception);
            (void) QueryColorDatabase(argv[++i],&target,&(*image)->exception);
            (void) OpaqueImage(*image,target,draw_info->fill);
            continue;
          }
        if (LocaleCompare("ordered-dither",option+1) == 0)
          {
            /*
              Ordered-dither image.
            */
            (void) RandomChannelThresholdImage(*image,argv[i+1],argv[i+2],
                &(*image)->exception);
            i+=2;
            continue;
          }
        break;
      }
      case 'p':
      {
        if (LocaleCompare("page",option+1) == 0)
          {
            char
              *geometry;

            geometry=GetPageGeometry(argv[++i]);
            (void) GetGeometry(geometry,&(*image)->page.x,&(*image)->page.y,
              &(*image)->page.width,&(*image)->page.height);
            LiberateMemory((void **) &geometry);
          }
        if (LocaleCompare("paint",option+1) == 0)
          {
            double
              radius;

            Image
              *paint_image;

            /*
              Oil paint image.
            */
            radius=atof(argv[++i]);
            paint_image=OilPaintImage(*image,radius,&(*image)->exception);
            if (paint_image == (Image *) NULL)
              break;
            DestroyImage(*image);
            *image=paint_image;
            continue;
          }
        if (LocaleCompare("pen",option+1) == 0)
          {
            (void) QueryColorDatabase(argv[++i],&draw_info->fill,
              &(*image)->exception);
            continue;
          }
        if (LocaleCompare("pointsize",option+1) == 0)
          {
            clone_info->pointsize=atof(argv[++i]);
            draw_info->pointsize=clone_info->pointsize;
            continue;
          }
        if (LocaleCompare("profile",option+1) == 0)
          {
            Image
              *profile;

            register long
              j;

            void
              *client_data;

            if (*option == '+')
              {
                /*
                  Remove a ICM, IPTC, or generic profile from the image.
                */
                (void) ProfileImage(*image,argv[++i],
                  (const unsigned char *) NULL,0,True);
                continue;
              }
            /*
              Add a ICM, IPTC, or generic profile to the image.
            */
            client_data=clone_info->client_data;
            clone_info->client_data=(void *) &(*image)->iptc_profile;
            (void) strncpy(clone_info->filename,argv[++i],MaxTextExtent-1);
            profile=ReadImage(clone_info,&(*image)->exception);
            if (profile == (Image *) NULL)
              continue;
            if (profile->iptc_profile.length != 0)
              {
                (void) ProfileImage(*image,"IPTC",profile->iptc_profile.info,
                  profile->iptc_profile.length,False);
                profile->iptc_profile.info=(unsigned char *) NULL,
                profile->iptc_profile.length=0;
              }
            if (profile->color_profile.length != 0)
              {
                (void) ProfileImage(*image,"ICM",profile->color_profile.info,
                  profile->color_profile.length,False);
                profile->color_profile.info=(unsigned char *) NULL;
                profile->color_profile.length=0;
              }
            for (j=0; j < (long) profile->generic_profiles; j++)
            {
              ProfileInfo
                *generic;

              generic=profile->generic_profile+j;
              (void) ProfileImage(*image,generic->name,generic->info,
                generic->length,False);
              generic->info=(unsigned char *) NULL;
              generic->length=0;
            }
            DestroyImage(profile);
            clone_info->client_data=client_data;
            continue;
          }
        break;
      }
      case 'q':
      {
        if (LocaleCompare("quality",option+1) == 0)
          {
            /*
              Set image compression quality.
            */
            clone_info->quality=atol(argv[++i]);
            continue;
          }
        break;
      }
      case 'r':
      {
        if (LocaleCompare("raise",option+1) == 0)
          {
            /*
              Surround image with a raise of solid color.
            */
            (void) GetImageGeometry(*image,argv[++i],False,&geometry);
            (void) RaiseImage(*image,&geometry,*option == '-');
            continue;
          }
        if (LocaleCompare("random-threshold",option+1) == 0)
          {
            /*
              Threshold image.
            */
            (void) RandomChannelThresholdImage(*image,argv[i+1],argv[i+2],
                &(*image)->exception);
            i+=2;
            continue;
          }
        if (LocaleCompare("red-primary",option+1) == 0)
          {
            /*
              Red chromaticity primary point.
            */
            if (*option == '+')
              {
                (*image)->chromaticity.red_primary.x=0.0;
                (*image)->chromaticity.red_primary.y=0.0;
                continue;
              }
            (void) sscanf(argv[++i],"%lf%*[,/]%lf",
              &(*image)->chromaticity.red_primary.x,
              &(*image)->chromaticity.red_primary.y);
            continue;
          }
        if (LocaleCompare("region",option+1) == 0)
          {
            Image
              *crop_image;

            if (region_image != (Image *) NULL)
              {
                /*
                  Composite region.
                */
                (void) CompositeImage(region_image,(*image)->matte ?
                  OverCompositeOp : CopyCompositeOp,*image,region_geometry.x,
                  region_geometry.y);
                DestroyImage(*image);
                *image=region_image;
              }
            if (*option == '+')
              continue;
            /*
              Apply transformations to a selected region of the image.
            */
            (void) GetImageGeometry(*image,argv[++i],False,&region_geometry);
            crop_image=CropImage(*image,&region_geometry,&(*image)->exception);
            if (crop_image == (Image *) NULL)
              break;
            region_image=(*image);
            *image=crop_image;
            continue;
          }
        if (LocaleCompare("render",option+1) == 0)
          {
            draw_info->render=(*option == '+');
            continue;
          }
        if (LocaleCompare("resample",option+1) == 0)
          {
            Image
              *resample_image;

            char
              resample_density[MaxTextExtent];

            unsigned long
              x_resolution,
              y_resolution;

            unsigned long
              resample_height,
              resample_width;

            long
              x,
              y;

            int
              flags;

            /*
              Resample image.
            */
            flags=GetGeometry(argv[++i],&x,&y,&x_resolution,&y_resolution);
            if (!(flags & HeightValue))
              y_resolution=x_resolution;
            FormatString(resample_density,"%lux%lu",x_resolution,y_resolution);
            if ((*image)->x_resolution == 0)
              (*image)->x_resolution=72.0;
            if ((*image)->y_resolution == 0)
              (*image)->y_resolution=72.0;
            resample_width=(unsigned long)
              ((*image)->columns*((double)x_resolution/(*image)->x_resolution)+0.5);
            resample_height=(unsigned long)
              ((*image)->rows*((double)y_resolution/(*image)->y_resolution)+0.5);
            (void) CloneString(&clone_info->density,resample_density);
            (void) CloneString(&draw_info->density,resample_density);
            (*image)->x_resolution=x_resolution;
            (*image)->y_resolution=y_resolution;
            if ((((*image)->columns == resample_width)) && ((*image)->rows == resample_height))
              break;
            resample_image=ResizeImage(*image,resample_width,resample_height,(*image)->filter,
              (*image)->blur,&(*image)->exception);
            if (resample_image == (Image *) NULL)
              break;
            DestroyImage(*image);
            *image=resample_image;
            continue;
          }
        if (LocaleCompare("resize",option+1) == 0)
          {
            Image
              *resize_image;

            /*
              Resize image.
            */
            (void) GetImageGeometry(*image,argv[++i],True,&geometry);
            if ((geometry.width == (*image)->columns) &&
                (geometry.height == (*image)->rows))
              break;
            resize_image=ResizeImage(*image,geometry.width,geometry.height,
              (*image)->filter,(*image)->blur,&(*image)->exception);
            if (resize_image == (Image *) NULL)
              break;
            DestroyImage(*image);
            *image=resize_image;
            continue;
          }
        if (LocaleCompare("roll",option+1) == 0)
          {
            Image
              *roll_image;

            /*
              Roll image.
            */
            (void) GetImageGeometry(*image,argv[++i],False,&geometry);
            roll_image=RollImage(*image,geometry.x,geometry.y,
              &(*image)->exception);
            if (roll_image == (Image *) NULL)
              break;
            DestroyImage(*image);
            *image=roll_image;
            continue;
          }
        if (LocaleCompare("rotate",option+1) == 0)
          {
            double
              degrees;

            Image
              *rotate_image;

            /*
              Check for conditional image rotation.
            */
            i++;
            if (strchr(argv[i],'>') != (char *) NULL)
              if ((*image)->columns <= (*image)->rows)
                break;
            if (strchr(argv[i],'<') != (char *) NULL)
              if ((*image)->columns >= (*image)->rows)
                break;
            /*
              Rotate image.
            */
            degrees=atof(argv[i]);
            rotate_image=RotateImage(*image,degrees,&(*image)->exception);
            if (rotate_image == (Image *) NULL)
              break;
            DestroyImage(*image);
            *image=rotate_image;
            continue;
          }
        break;
      }
      case 's':
      {
        if (LocaleCompare("sample",option+1) == 0)
          {
            Image
              *sample_image;

            /*
              Sample image with pixel replication.
            */
            (void) GetImageGeometry(*image,argv[++i],True,&geometry);
            if ((geometry.width == (*image)->columns) &&
                (geometry.height == (*image)->rows))
              break;
            sample_image=SampleImage(*image,geometry.width,geometry.height,
              &(*image)->exception);
            if (sample_image == (Image *) NULL)
              break;
            DestroyImage(*image);
            *image=sample_image;
            continue;
          }
        if (LocaleCompare("sampling_factor",option+1) == 0)
          {
            /*
              Set image sampling factor.
            */
            (void) CloneString(&clone_info->sampling_factor,argv[++i]);
            continue;
          }
        if (LocaleCompare("sans",option+1) == 0)
          if (*option == '-')
            i++;
        if (LocaleCompare("scale",option+1) == 0)
          {
            Image
              *scale_image;

            /*
              Resize image.
            */
            (void) GetImageGeometry(*image,argv[++i],True,&geometry);
            if ((geometry.width == (*image)->columns) &&
                (geometry.height == (*image)->rows))
              break;
            scale_image=ScaleImage(*image,geometry.width,geometry.height,
              &(*image)->exception);
            if (scale_image == (Image *) NULL)
              break;
            DestroyImage(*image);
            *image=scale_image;
            continue;
          }
        if (LocaleCompare("scene",option+1) == 0)
          {
            (*image)->scene=atol(argv[++i]);
            continue;
          }
        if (LocaleCompare("segment",option+1) == 0)
          {
            double
              cluster_threshold,
              smoothing_threshold;

            /*
              Segment image.
            */
            cluster_threshold=1.0;
            smoothing_threshold=1.5;
            (void) sscanf(argv[++i],"%lfx%lf",&cluster_threshold,
              &smoothing_threshold);
            (void) SegmentImage(*image,quantize_info.colorspace,
              clone_info->verbose,cluster_threshold,smoothing_threshold);
            continue;
          }
        if (LocaleCompare("shade",option+1) == 0)
          {
            double
              azimuth,
              elevation;

            Image
              *shade_image;

            /*
              Shade image.
            */
            azimuth=30.0;
            elevation=30.0;
            (void) sscanf(argv[++i],"%lfx%lf",&azimuth,&elevation);
            shade_image=ShadeImage(*image,*option == '-',azimuth,elevation,
              &(*image)->exception);
            if (shade_image == (Image *) NULL)
              break;
            DestroyImage(*image);
            *image=shade_image;
            continue;
          }
        if (LocaleCompare("sharpen",option+1) == 0)
          {
            double
              radius,
              sigma;

            Image
              *sharp_image;

            /*
              Gaussian sharpen image.
            */
            radius=0.0;
            sigma=1.0;
            (void) sscanf(argv[++i],"%lfx%lf",&radius,&sigma);
            sharp_image=SharpenImage(*image,radius,sigma,&(*image)->exception);
            if (sharp_image == (Image *) NULL)
              break;
            DestroyImage(*image);
            *image=sharp_image;
            continue;
          }
        if (LocaleCompare("shave",option+1) == 0)
          {
            Image
              *shave_image;

            /*
              Shave the image edges.
            */
            (void) GetImageGeometry(*image,argv[++i],False,&geometry);
            shave_image=ShaveImage(*image,&geometry,&(*image)->exception);
            if (shave_image == (Image *) NULL)
              break;
            DestroyImage(*image);
            *image=shave_image;
            continue;
          }
        if (LocaleCompare("shear",option+1) == 0)
          {
            double
              x_shear,
              y_shear;

            Image
              *shear_image;

            /*
              Shear image.
            */
            x_shear=0.0;
            y_shear=0.0;
            (void) sscanf(argv[++i],"%lfx%lf",&x_shear,&y_shear);
            shear_image=ShearImage(*image,x_shear,y_shear,&(*image)->exception);
            if (shear_image == (Image *) NULL)
              break;
            DestroyImage(*image);
            *image=shear_image;
            continue;
          }
        if (LocaleCompare("solarize",option+1) == 0)
          {
            double
              threshold;

            threshold=StringToDouble(argv[++i],MaxRGB);
            SolarizeImage(*image,threshold);
            continue;
          }
        if (LocaleCompare("spread",option+1) == 0)
          {
            unsigned int
              amount;

            Image
              *spread_image;

            /*
              Spread an image.
            */
            amount=atoi(argv[++i]);
            spread_image=SpreadImage(*image,amount,&(*image)->exception);
            if (spread_image == (Image *) NULL)
              break;
            DestroyImage(*image);
            *image=spread_image;
            continue;
          }
        if (LocaleCompare("stroke",option+1) == 0)
          {
            (void) QueryColorDatabase(argv[++i],&draw_info->stroke,
              &(*image)->exception);
            continue;
          }
        if (LocaleCompare("strokewidth",option+1) == 0)
          {
            draw_info->stroke_width=atof(argv[++i]);
            continue;
          }
        if (LocaleCompare("swirl",option+1) == 0)
          {
            double
              degrees;

            Image
              *swirl_image;

            /*
              Swirl image.
            */
            degrees=atof(argv[++i]);
            swirl_image=SwirlImage(*image,degrees,&(*image)->exception);
            if (swirl_image == (Image *) NULL)
              break;
            DestroyImage(*image);
            *image=swirl_image;
            continue;
          }
        break;
      }
      case 't':
      {
        if (LocaleCompare("threshold",option+1) == 0)
          {
            /*
              Threshold image.
            */
            (void) ChannelThresholdImage(*image,argv[++i]);
            continue;
          }
        if (LocaleCompare("tile",option+1) == 0)
          {
            Image
              *fill_pattern;

            (void) strncpy(clone_info->filename,argv[++i],MaxTextExtent-1);
            fill_pattern=ReadImage(clone_info,&(*image)->exception);
            if (fill_pattern == (Image *) NULL)
              continue;
            draw_info->fill_pattern=
              CloneImage(fill_pattern,0,0,True,&(*image)->exception);
            DestroyImage(fill_pattern);
            continue;
          }
        if (LocaleCompare("transform",option+1) == 0)
          {
            Image
              *transform_image;

            /*
              Affine transform image.
            */
            transform_image=AffineTransformImage(*image,&draw_info->affine,
              &(*image)->exception);
            if (transform_image == (Image *) NULL)
              break;
            DestroyImage(*image);
            *image=transform_image;
            continue;
          }
        if (LocaleCompare("transparent",option+1) == 0)
          {
            PixelPacket
              target;

            target=AcquireOnePixel(*image,0,0,&(*image)->exception);
            (void) QueryColorDatabase(argv[++i],&target,&(*image)->exception);
            (void) TransparentImage(*image,target,TransparentOpacity);
            continue;
          }
        if (LocaleCompare("treedepth",option+1) == 0)
          {
            quantize_info.tree_depth=atoi(argv[++i]);
            continue;
          }
        if (LocaleCompare("trim",option+1) == 0)
          {
            TransformImage(image,"0x0",(char *) NULL);
            continue;
          }
        if (LocaleCompare("type",option+1) == 0)
          {
            ImageType
              image_type;

            option=argv[++i];
            image_type=GetImageType(*image,&(*image)->exception);
            if (LocaleCompare("Bilevel",option) == 0)
              image_type=BilevelType;
            if (LocaleCompare("Grayscale",option) == 0)
              image_type=GrayscaleType;
            if (LocaleCompare("GrayscaleMatte",option) == 0)
              image_type=GrayscaleMatteType;
            if (LocaleCompare("Palette",option) == 0)
              image_type=PaletteType;
            if (LocaleCompare("PaletteMatte",option) == 0)
              image_type=PaletteMatteType;
            if (LocaleCompare("TrueColor",option) == 0)
              image_type=TrueColorType;
            if (LocaleCompare("TrueColorMatte",option) == 0)
              image_type=TrueColorMatteType;
            if (LocaleCompare("ColorSeparation",option) == 0)
              image_type=ColorSeparationType;
            if (LocaleCompare("ColorSeparationMatte",option) == 0)
              image_type=ColorSeparationMatteType;
            if (LocaleCompare("Optimize",option) == 0)
              image_type=OptimizeType;
            (*image)->dither=image_info->dither;
            SetImageType(*image,image_type);
            continue;
          }
        break;
      }
      case 'u':
      {
        if (LocaleCompare("undercolor",option+1) == 0)
          {
            (void) QueryColorDatabase(argv[++i],&draw_info->undercolor,
              &(*image)->exception);
            continue;
          }
        if (LocaleCompare("units",option+1) == 0)
          {
            ResolutionType
              resolution;

            if (*option == '+')
              {
                (*image)->units=UndefinedResolution;
                continue;
              }
            option=argv[++i];
            resolution=UndefinedResolution;
            if (LocaleCompare("PixelsPerInch",option) == 0)
              resolution=PixelsPerInchResolution;
            if (LocaleCompare("PixelsPerCentimeter",option) == 0)
              resolution=PixelsPerCentimeterResolution;
            (*image)->units=resolution;
            continue;
          }
        if (LocaleCompare("unsharp",option+1) == 0)
          {
            double
              amount,
              radius,
              sigma,
              threshold;

            Image
              *unsharp_image;

            /*
              Gaussian unsharpen image.
            */
            amount=1.0;
            radius=0.0;
            sigma=1.0;
            threshold=0.05;
            (void) sscanf(argv[++i],"%lfx%lf%lf%lf",&radius,&sigma,&amount,
              &threshold);
            unsharp_image=UnsharpMaskImage(*image,radius,sigma,amount,threshold,
              &(*image)->exception);
            if (unsharp_image == (Image *) NULL)
              break;
            DestroyImage(*image);
            *image=unsharp_image;
            continue;
          }
        break;
      }
      case 'v':
      {
        if (LocaleCompare("verbose",option+1) == 0)
          {
            clone_info->verbose=(*option == '-');
            quantize_info.measure_error=(*option == '-');
            continue;
          }
        if (LocaleCompare("virtual-pixel",option+1) == 0)
          {
            VirtualPixelMethod
              virtual_pixel_method;

            if (*option == '+')
              {
                SetImageVirtualPixelMethod(*image,UndefinedVirtualPixelMethod);
                continue;
              }
            option=argv[++i];
            virtual_pixel_method=UndefinedVirtualPixelMethod;
            if (LocaleCompare("Constant",option) == 0)
              virtual_pixel_method=ConstantVirtualPixelMethod;
            if (LocaleCompare("Edge",option) == 0)
              virtual_pixel_method=EdgeVirtualPixelMethod;
            if (LocaleCompare("Mirror",option) == 0)
              virtual_pixel_method=MirrorVirtualPixelMethod;
            if (LocaleCompare("Tile",option) == 0)
              virtual_pixel_method=TileVirtualPixelMethod;
            SetImageVirtualPixelMethod(*image,virtual_pixel_method);
            continue;
          }
        break;
      }
      case 'w':
      {
        if (LocaleCompare("wave",option+1) == 0)
          {
            double
              amplitude,
              wavelength;

            Image
              *wave_image;

            /*
              Wave image.
            */
            amplitude=25.0;
            wavelength=150.0;
            (void) sscanf(argv[++i],"%lfx%lf",&amplitude,&wavelength);
            wave_image=WaveImage(*image,amplitude,wavelength,
              &(*image)->exception);
            if (wave_image == (Image *) NULL)
              break;
            DestroyImage(*image);
            *image=wave_image;
            continue;
          }
        if (LocaleCompare("white-point",option+1) == 0)
          {
            /*
              White chromaticity point.
            */
            if (*option == '+')
              {
                (*image)->chromaticity.white_point.x=0.0;
                (*image)->chromaticity.white_point.y=0.0;
                continue;
              }
            (void) sscanf(argv[++i],"%lf%*[,/]%lf",
              &(*image)->chromaticity.white_point.x,
              &(*image)->chromaticity.white_point.y);
            continue;
          }
        break;
      }
      default:
        break;
    }
  }
  if (region_image != (Image *) NULL)
    {
      /*
        Composite transformed region onto image.
      */
      matte=region_image->matte;
      (void) CompositeImage(region_image,
        (*image)->matte ? OverCompositeOp : CopyCompositeOp,*image,
        region_geometry.x,region_geometry.y);
      DestroyImage(*image);
      *image=region_image;
      (*image)->matte=matte;
    }

  if ( quantize_info.colorspace == GRAYColorspace )
    {
      /*
        If color reduction is requested, then quantize to the requested
        number of colors in the gray colorspace, otherwise simply
        transform the image to the gray colorspace.
      */

      if ( quantize_info.number_colors != 0 )
        (void) QuantizeImage(&quantize_info,*image);
      else
        TransformColorspace(*image,GRAYColorspace);
    }
  else
    {
      /*
        If color reduction is requested, and the image is DirectClass,
        or the image is PseudoClass and the number of colors exceeds
        the number requested, then quantize the image colors. Otherwise
        compress an existing colormap.
      */

      if ( quantize_info.number_colors != 0 )
        {
          if (((*image)->storage_class == DirectClass) ||
              ((*image)->colors > quantize_info.number_colors))
            (void) QuantizeImage(&quantize_info,*image);
          else
            CompressImageColormap(*image);
        }
    }

  if (map_image != (Image *) NULL)
    {
      (void) MapImage(*image,map_image,quantize_info.dither);
      DestroyImage(map_image);
    }
  /*
    Free resources.
  */
  DestroyDrawInfo(draw_info);
  DestroyImageInfo(clone_info);
  return((*image)->exception.severity == UndefinedException);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
+     M o g r i f y I m a g e s                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method MogrifyImages applies next processing options to a sequence of
%  images as prescribed by command line options.
%
%  The format of the MogrifyImage method is:
%
%      unsigned int MogrifyImages(const ImageInfo *image_info,const int argc,
%        char **argv,Image **images)
%
%  A description of each parameter follows:
%
%    o image_info: The image info..
%
%    o argc: Specifies a pointer to an integer describing the number of
%      elements in the argument vector.
%
%    o argv: Specifies a pointer to a text array containing the command line
%      arguments.
%
%    o images: The image;  returned from
%      ReadImage.
%
%
*/
MagickExport unsigned int MogrifyImages(const ImageInfo *image_info,
  int argc,char **argv,Image **images)
{
#define MogrifyImageText  "  Transform image...  "

  char
    *option;

  Image
    *clone_images,
    *image,
    *mogrify_images;

  ImageInfo
    *clone_info;

  MonitorHandler
    handler;

  register long
    i;

  unsigned int
    status;

  unsigned long
    number_images,
    scene;

  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  assert(images != (Image **) NULL);
  assert((*images)->signature == MagickSignature);
  if ((argc <= 0) || (*argv == (char *) NULL))
    return(True);
  clone_images=(Image *) NULL;
  scene=False;
  for (i=0; i < argc; i++)
  {
    option=argv[i];
    if ((strlen(option) <= 1) || ((*option != '-') && (*option != '+')))
      continue;
    switch (*(option+1))
    {
      case 's':
      {
        if (LocaleCompare("scene",option+1) == 0)
          scene=True;
        break;
      }
      case 'w':
      {
        if (LocaleCompare("+write",option) == 0)
          clone_images=CloneImageList(*images,&(*images)->exception);
        break;
      }
      default:
        break;
    }
  }
  /*
    Apply options to individual each image in the list.
  */
  status=True;
  mogrify_images=NewImageList();
  number_images=GetImageListLength(*images);
  for (i=0; i < (long) number_images; i++)
  {
    image=RemoveFirstImageFromList(images);
    handler=SetMonitorHandler((MonitorHandler) NULL);
    status&=MogrifyImage(image_info,argc,argv,&image);
    (void) SetMonitorHandler(handler);
    if (scene)
      image->scene+=i;
    if (image_info->verbose)
      DescribeImage(image,stdout,False);
    AppendImageToList(&mogrify_images,image);
    if (!MagickMonitor(MogrifyImageText,i,number_images,&image->exception))
      break;
  }
  /*
    Apply options to the entire image list.
  */
  for (i=0; i < argc; i++)
  {
    option=argv[i];
    if ((strlen(option) == 1) || ((*option != '-') && (*option != '+')))
      continue;
    switch (*(option+1))
    {
      case 'a':
      {
        if (LocaleCompare("append",option+1) == 0)
          {
            Image
              *append_image;

            append_image=AppendImages(mogrify_images,*option == '-',
              &mogrify_images->exception);
            if (append_image != (Image *) NULL)
              {
                DestroyImageList(mogrify_images);
                mogrify_images=append_image;
              }
            break;
          }
        if (LocaleCompare("average",option+1) == 0)
          {
            Image
              *average_image;

            average_image=AverageImages(mogrify_images,
              &mogrify_images->exception);
            if (average_image != (Image *) NULL)
              {
                DestroyImageList(mogrify_images);
                mogrify_images=average_image;
              }
            break;
          }
        break;
      }
      case 'c':
      {
        if (LocaleCompare("coalesce",option+1) == 0)
          {
            Image
              *coalesce_image;

            coalesce_image=CoalesceImages(mogrify_images,
              &mogrify_images->exception);
            if (coalesce_image != (Image *) NULL)
              {
                DestroyImageList(mogrify_images);
                mogrify_images=coalesce_image;
              }
            break;
          }
        break;
      }
      case 'd':
      {
        if (LocaleCompare("deconstruct",option+1) == 0)
          {
            Image
              *deconstruct_image;

            deconstruct_image=DeconstructImages(mogrify_images,
              &mogrify_images->exception);
            if (deconstruct_image != (Image *) NULL)
              {
                DestroyImageList(mogrify_images);
                mogrify_images=deconstruct_image;
              }
            break;
          }
        break;
      }
      case 'f':
      {
        if (LocaleCompare("flatten",option+1) == 0)
          {
            Image
              *flatten_image;

            flatten_image=FlattenImages(mogrify_images,
              &mogrify_images->exception);
            if (flatten_image != (Image *) NULL)
              {
                DestroyImageList(mogrify_images);
                mogrify_images=flatten_image;
              }
            break;
          }
        break;
      }
      case 'm':
      {
        if (LocaleCompare("map",option+1) == 0)
          {
             if (*option == '+')
               {
                 (void) MapImages(mogrify_images,(Image *) NULL,
                   image_info->dither);
                 break;
               }
             i++;
             break;
          }
        if (LocaleCompare("morph",option+1) == 0)
          {
            Image
              *morph_image;

            morph_image=MorphImages(mogrify_images,atol(argv[++i]),
              &mogrify_images->exception);
            if (morph_image != (Image *) NULL)
              {
                DestroyImageList(mogrify_images);
                mogrify_images=morph_image;
              }
            break;
          }
        if (LocaleCompare("mosaic",option+1) == 0)
          {
            Image
              *mosaic_image;

            mosaic_image=MosaicImages(mogrify_images,
              &mogrify_images->exception);
            if (mosaic_image != (Image *) NULL)
              {
                DestroyImageList(mogrify_images);
                mogrify_images=mosaic_image;
              }
            break;
          }
        break;
      }
      case 'p':
      {
        if (LocaleCompare("process",option+1) == 0)
          {
            char
              *arguments,
              breaker,
              quote,
              *token;

            int
              next,
              status;

            size_t
              length;

            TokenInfo
              token_info;

            length=strlen(argv[++i]);
            token=(char *) AcquireMemory(length+1);
            if (token == (char *) NULL)
              continue;
            next=0;
            arguments=argv[i];
            status=Tokenizer(&token_info,0,token,length,arguments,(char *) "",
              (char *) "=",(char *) "\"",0,&breaker,&next,&quote);
            if (status == 0)
              {
                char
                  *argv;

                argv=&(arguments[next]);
                (void) ExecuteModuleProcess(token,&mogrify_images,1,&argv);
              }
            LiberateMemory((void **) &token);
            continue;
          }
        break;
      }
      case 'w':
      {
        if (LocaleCompare("write",option+1) == 0)
          {
            clone_info=CloneImageInfo(image_info);
            status&=WriteImages(clone_info,mogrify_images,argv[++i],
              &mogrify_images->exception);
            DestroyImageInfo(clone_info);
            if (*option == '+')
              {
                DestroyImageList(mogrify_images);
                mogrify_images=clone_images;
              }
            break;
          }
        break;
      }
      default:
        break;
    }
  }
  *images=mogrify_images;
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%    M o g r i f y I m a g e C o m m a n d                                    %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MogrifyImageCommand() transforms an image or a sequence of images. These
%  transforms include image scaling, image rotation, color reduction, and
%  others. The transmogrified image overwrites the original image.
%
%  The format of the MogrifyImageCommand method is:
%
%      unsigned int MogrifyImageCommand(ImageInfo *image_info,const int argc,
%        char **argv,char **metadata,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: The image info.
%
%    o argc: The number of elements in the argument vector.
%
%    o argv: A text array containing the command line arguments.
%
%    o metadata: any metadata is returned here.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
#define ThrowMogrifyException(code,reason,description) \
{ \
  ThrowException(exception,code,reason,description); \
  LiberateArgumentList(argc,argv); \
  return(False); \
}
MagickExport unsigned int MogrifyImageCommand(ImageInfo *image_info,
  int argc,char **argv,char **metadata,ExceptionInfo *exception)
{

  char
    filename[MaxTextExtent],
    *format,
    *option;

  double
    sans;

  Image
    *image;

  long
    j,
    k,
    x;

  register long
    i;

  unsigned int
    global_colormap,
    status;

  /*
    Set defaults.
  */
  format=(char *) NULL;
  global_colormap=False;
  image=NewImageList();
  status=True;

  /*
    Expand argument list
  */
  status=ExpandFilenames(&argc,&argv);
  if (status == False)
    MagickFatalError(ResourceLimitFatalError,"MemoryAllocationFailed",
      (char *) NULL);

  /*
    Check for valid command line
  */
  if (argc < 2 || ((argc < 3) && (LocaleCompare("-help",argv[1]) == 0 ||
      LocaleCompare("-?",argv[1]) == 0)))
    {
      MogrifyUsage();
      ThrowMogrifyException(OptionError,"UsageError",NULL);
    }

  j=1;
  k=0;
  for (i=1; i < argc; i++)
  {
    option=argv[i];
    if ((strlen(option) == 1) || ((*option != '-') && (*option != '+')))
      {
        /*
          Option is a file name: begin by reading image from specified file.
        */
        k=i;
        (void) strncpy(image_info->filename,argv[i],MaxTextExtent-1);
        image=ReadImage(image_info,exception);
        status&=(image != (Image *) NULL) &&
          (exception->severity < ErrorException);
        if (image == (Image *) NULL)
          continue;
        if (format != (char *) NULL)
          {
            register char
              *p;

            /*
              Modify filename to include a user specified image format.
            */
            p=image->filename+strlen(image->filename)-1;
            while ((*p != '.') && (p > (image->filename+1)))
              p--;
            p++;
            if (LocaleCompare(p,image->magick) == 0)
              (void) strncpy(p,format,MaxTextExtent-(p-image->filename)-1);
            else
              {
                FormatString(image_info->filename,"%.1024s:%.1024s",format,
                  image->filename);
                (void) strncpy(image->filename,image_info->filename,
                  MaxTextExtent-1);
              }
          }
        /*
          Transmogrify image as defined by the image processing options.
        */
        status&=MogrifyImages(image_info,i-j,argv+j,&image);
        if (global_colormap)
          (void) MapImages(image,(Image *) NULL,image_info->dither);
        /*
          Write transmogrified image to disk.
        */
        if (format != (char *) NULL)
          AppendImageFormat(format,image->filename);
        else
          if (LocaleCompare(image_info->filename,"-") != 0)
            {
              (void) strncpy(filename,image->filename,MaxTextExtent-1);
              AppendImageFormat("tmp",image->filename);
              if (IsAccessible(image->filename))
                {
                  (void) strcat(image->filename,"~");
                  if (IsAccessible(image->filename))
                    ThrowMogrifyException(FileOpenFatalError,
                      "UnableToCreateTemporaryFile",filename);
                }
            }
        status&=WriteImages(image_info,image,image->filename,&image->exception);
        if ((status != False) && (LocaleCompare(image_info->filename,"-") != 0))
          {
            (void) remove(filename);
            (void) rename(image->filename,filename);
          }
        DestroyImage(image);
        continue;
      }
    j=k+1;
    switch (*(option+1))
    {
      case 'a':
      {
        if (LocaleCompare("affine",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%lf",&sans))
                  ThrowMogrifyException(OptionError,"MissingMatrix",option);
              }
            break;
          }
        if (LocaleCompare("antialias",option+1) == 0)
          {
            image_info->antialias=(*option == '-');
            break;
          }
        if (LocaleCompare("authenticate",option+1) == 0)
          {
            (void) CloneString(&image_info->authenticate,(char *) NULL);
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowMogrifyException(OptionError,"MissingPassword",option);
                (void) CloneString(&image_info->authenticate,argv[i]);
              }
            break;
          }
        ThrowMogrifyException(OptionError,"UnrecognizedOption",option)
      }
      case 'b':
      {
        if (LocaleCompare("background",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowMogrifyException(OptionError,"MissingBackgroundColor",
                    option);
                (void) QueryColorDatabase(argv[i],&image_info->background_color,
                  exception);
              }
            break;
          }
        if (LocaleCompare("blue-primary",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  ThrowMogrifyException(OptionError,"MissingPoint",option);
              }
            break;
          }
        if (LocaleCompare("blur",option+1) == 0)
          {
            i++;
            if ((i == argc) || !sscanf(argv[i],"%ld",&x))
              ThrowMogrifyException(OptionError,"MissingGeometry",option);
            break;
          }
        if (LocaleCompare("border",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !IsGeometry(argv[i]))
                  ThrowMogrifyException(OptionError,"MissingGeometry",option);
              }
            break;
          }
        if (LocaleCompare("bordercolor",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowMogrifyException(OptionError,"MissingBorderColor",
                    option);
                (void) QueryColorDatabase(argv[i],&image_info->border_color,
                  exception);
              }
            break;
          }
        if (LocaleCompare("box",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowMogrifyException(OptionError,"MissingColors",option);
              }
            break;
          }
        ThrowMogrifyException(OptionError,"UnrecognizedOption",option)
      }
      case 'c':
      {
        if (LocaleCompare("cache",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  ThrowMogrifyException(OptionError,"MissingThreshold",option);
                SetMagickResourceLimit(MemoryResource,atol(argv[i]));
                SetMagickResourceLimit(MapResource,2*atol(argv[i]));
              }
            break;
          }
        if (LocaleCompare("channel",option+1) == 0)
          {
            if (*option == '-')
              {
                ChannelType
                  channel;

                i++;
                if (i == argc)
                  ThrowMogrifyException(OptionError,"MissingType",option);
                option=argv[i];
                channel=UndefinedChannel;
               if (LocaleCompare("Red",option) == 0)
                 channel=RedChannel;
               if (LocaleCompare("Cyan",option) == 0)
                 channel=CyanChannel;
               if (LocaleCompare("Green",option) == 0)
                 channel=GreenChannel;
               if (LocaleCompare("Magenta",option) == 0)
                 channel=MagentaChannel;
               if (LocaleCompare("Blue",option) == 0)
                 channel=BlueChannel;
               if (LocaleCompare("Yellow",option) == 0)
                 channel=YellowChannel;
               if (LocaleCompare("Opacity",option) == 0)
                 channel=OpacityChannel;
               if (LocaleCompare("Black",option) == 0)
                 channel=BlackChannel;
               if (LocaleCompare("Matte",option) == 0)
                 channel=MatteChannel;
                if (channel == UndefinedChannel)
                  ThrowMogrifyException(OptionError,"UnrecognizedChannelType",
                    option);
              }
            break;
          }
        if (LocaleCompare("charcoal",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%lf",&sans))
                  ThrowMogrifyException(OptionError,"MissingPaintRadius",
                    option);
              }
            break;
          }
        if (LocaleCompare("chop",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !IsGeometry(argv[i]))
                  ThrowMogrifyException(OptionError,"MissingGeometry",option);
              }
            break;
          }
        if (LocaleCompare("colorize",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  ThrowMogrifyException(OptionError,"MissingColorizeValue",
                    option);
              }
            break;
          }
        if (LocaleCompare("colors",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  ThrowMogrifyException(OptionError,"MissingColors",option);
              }
            break;
          }
        if (LocaleCompare("colorspace",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowMogrifyException(OptionError,"MissingType",option);
                option=argv[i];
                image_info->colorspace=UndefinedColorspace;
                if (LocaleCompare("cmyk",option) == 0)
                  image_info->colorspace=CMYKColorspace;
                if (LocaleCompare("gray",option) == 0)
                  image_info->colorspace=GRAYColorspace;
                if (LocaleCompare("hsl",option) == 0)
                  image_info->colorspace=HSLColorspace;
                if (LocaleCompare("hwb",option) == 0)
                  image_info->colorspace=HWBColorspace;
                if (LocaleCompare("ohta",option) == 0)
                  image_info->colorspace=OHTAColorspace;
                if (LocaleCompare("rgb",option) == 0)
                  image_info->colorspace=RGBColorspace;
                if (LocaleCompare("srgb",option) == 0)
                  image_info->colorspace=sRGBColorspace;
                if (LocaleCompare("transparent",option) == 0)
                  image_info->colorspace=TransparentColorspace;
                if (LocaleCompare("xyz",option) == 0)
                  image_info->colorspace=XYZColorspace;
                if (LocaleCompare("ycbcr",option) == 0)
                  image_info->colorspace=YCbCrColorspace;
                if (LocaleCompare("ycc",option) == 0)
                  image_info->colorspace=YCCColorspace;
                if (LocaleCompare("yiq",option) == 0)
                  image_info->colorspace=YIQColorspace;
                if (LocaleCompare("ypbpr",option) == 0)
                  image_info->colorspace=YPbPrColorspace;
                if (LocaleCompare("yuv",option) == 0)
                  image_info->colorspace=YUVColorspace;
                if (image_info->colorspace == UndefinedColorspace)
                  ThrowMogrifyException(OptionError,"UnrecognizedColorspace",option);
              }
            break;
          }
        if (LocaleCompare("comment",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowMogrifyException(OptionError,"MissingComment",option);
              }
            break;
          }
        if (LocaleCompare("compress",option+1) == 0)
          {
            image_info->compression=NoCompression;
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowMogrifyException(OptionError,"MissingType",option);
                option=argv[i];
                image_info->compression=UndefinedCompression;
                if (LocaleCompare("None",option) == 0)
                  image_info->compression=NoCompression;
                if (LocaleCompare("BZip",option) == 0)
                  image_info->compression=BZipCompression;
                if (LocaleCompare("Fax",option) == 0)
                  image_info->compression=FaxCompression;
                if (LocaleCompare("Group4",option) == 0)
                  image_info->compression=Group4Compression;
                if (LocaleCompare("JPEG",option) == 0)
                  image_info->compression=JPEGCompression;
                if (LocaleCompare("Lossless",option) == 0)
                  image_info->compression=LosslessJPEGCompression;
                if (LocaleCompare("LZW",option) == 0)
                  image_info->compression=LZWCompression;
                if (LocaleCompare("RLE",option) == 0)
                  image_info->compression=RLECompression;
                if (LocaleCompare("Zip",option) == 0)
                  image_info->compression=ZipCompression;
                if (image_info->compression == UndefinedCompression)
                  ThrowMogrifyException(OptionError,"UnrecognizedImageCompression",
                    option);
              }
            break;
          }
        if (LocaleCompare("contrast",option+1) == 0)
          break;
        if (LocaleCompare("convolve",option+1) == 0)
          {
           if (*option == '-')
             {
               i++;
               if ((i == (argc-1)) || !IsGeometry(argv[i]))
                 ThrowMogrifyException(OptionError,"MissingCoefficients",
                   option);
             }
           break;
         }
        if (LocaleCompare("crop",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !IsGeometry(argv[i]))
                  ThrowMogrifyException(OptionError,"MissingGeometry",option);
              }
            break;
          }
        if (LocaleCompare("cycle",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  ThrowMogrifyException(OptionError,"MissingCycleAmount",
                    option);
              }
            break;
          }
        ThrowMogrifyException(OptionError,"UnrecognizedOption",option)
      }
      case 'd':
      {
        if (LocaleCompare("debug",option+1) == 0)
          {
            (void) SetLogEventMask("None");
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowMogrifyException(OptionError,"MissingEventMask",option);
                (void) SetLogEventMask(argv[i]);
              }
            break;
          }
        if (LocaleCompare("delay",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  ThrowMogrifyException(OptionError,"MissingDelay",option);
              }
            break;
          }
        if (LocaleCompare("density",option+1) == 0)
          {
            (void) CloneString(&image_info->density,(char *) NULL);
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !IsGeometry(argv[i]))
                  ThrowMogrifyException(OptionError,"MissingGeometry",
                    option);
                (void) CloneString(&image_info->density,argv[i]);
              }
            break;
          }
        if (LocaleCompare("depth",option+1) == 0)
          {
            image_info->depth=QuantumDepth;
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  ThrowMogrifyException(OptionError,"MissingImageDepth",option);
                image_info->depth=atol(argv[i]);
              }
            break;
          }
        if (LocaleCompare("despeckle",option+1) == 0)
          break;
        if (LocaleCompare("display",option+1) == 0)
          {
            (void) CloneString(&image_info->server_name,(char *) NULL);
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowMogrifyException(OptionError,"MissingServerName",option);
                (void) CloneString(&image_info->server_name,argv[i]);
              }
            break;
          }
        if (LocaleCompare("dispose",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowMogrifyException(OptionError,"MissingDisposeMethod",
                    option);
                option=argv[i];
                if ((LocaleCompare("0",option) != 0) &&
                    (LocaleCompare("1",option) != 0) &&
                    (LocaleCompare("2",option) != 0) &&
                    (LocaleCompare("3",option) != 0) &&
                    (LocaleCompare("Undefined",option) != 0) &&
                    (LocaleCompare("None",option) != 0) &&
                    (LocaleCompare("Background",option) != 0) &&
                    (LocaleCompare("Previous",option) != 0))
                  ThrowMogrifyException(OptionError,"UnrecognizedDisposeMethod",
                    option);
              }
            break;
          }
        if (LocaleCompare("dither",option+1) == 0)
          {
            image_info->dither=(*option == '-');
            break;
          }
        if (LocaleCompare("draw",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowMogrifyException(OptionError,"MissingDrawingPrimitive",
                    option);
              }
            break;
          }
        ThrowMogrifyException(OptionError,"UnrecognizedOption",option)
      }
      case 'e':
      {
        if (LocaleCompare("edge",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%lf",&sans))
                  ThrowMogrifyException(OptionError,"MissingPaintRadius",
                    option);
              }
            break;
          }
        if (LocaleCompare("emboss",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !IsGeometry(argv[i]))
                  ThrowMogrifyException(OptionError,"MissingPaintRadius",
                    option);
              }
            break;
          }
        if (LocaleCompare("encoding",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowMogrifyException(OptionError,"MissingEnodingType",
                    option);
              }
            break;
          }
        if (LocaleCompare("endian",option+1) == 0)
          {
            image_info->endian=UndefinedEndian;
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowMogrifyException(OptionError,"MissingType",option);
                option=argv[i];
                image_info->endian=UndefinedEndian;
                if (LocaleCompare("LSB",option) == 0)
                  image_info->endian=LSBEndian;
                if (LocaleCompare("MSB",option) == 0)
                  image_info->endian=MSBEndian;
                if (image_info->endian == UndefinedEndian)
                  ThrowMogrifyException(OptionError,"UnrecognizedEndianType",
                    option);
              }
            break;
          }
        if (LocaleCompare("enhance",option+1) == 0)
          break;
        if (LocaleCompare("equalize",option+1) == 0)
          break;
        ThrowMogrifyException(OptionError,"UnrecognizedOption",option)
      }
      case 'f':
      {
        if (LocaleCompare("fill",option+1) == 0)
          {
            (void) QueryColorDatabase("none",&image_info->pen,exception);
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowMogrifyException(OptionError,"MissingFillColor",option);
                (void) QueryColorDatabase(argv[i],&image_info->pen,exception);
              }
            break;
          }
        if (LocaleCompare("filter",option+1) == 0)
          {
            if (*option == '-')
              {
                FilterTypes
                  filter;

                i++;
                if (i == argc)
                  ThrowMogrifyException(OptionError,"MissingType",option);
                option=argv[i];
                filter=UndefinedFilter;
                if (LocaleCompare("Point",option) == 0)
                  filter=PointFilter;
                if (LocaleCompare("Box",option) == 0)
                  filter=BoxFilter;
                if (LocaleCompare("Triangle",option) == 0)
                  filter=TriangleFilter;
                if (LocaleCompare("Hermite",option) == 0)
                  filter=HermiteFilter;
                if (LocaleCompare("Hanning",option) == 0)
                  filter=HanningFilter;
                if (LocaleCompare("Hamming",option) == 0)
                  filter=HammingFilter;
                if (LocaleCompare("Blackman",option) == 0)
                  filter=BlackmanFilter;
                if (LocaleCompare("Gaussian",option) == 0)
                  filter=GaussianFilter;
                if (LocaleCompare("Quadratic",option) == 0)
                  filter=QuadraticFilter;
                if (LocaleCompare("Cubic",option) == 0)
                  filter=CubicFilter;
                if (LocaleCompare("Catrom",option) == 0)
                  filter=CatromFilter;
                if (LocaleCompare("Mitchell",option) == 0)
                  filter=MitchellFilter;
                if (LocaleCompare("Lanczos",option) == 0)
                  filter=LanczosFilter;
                if (LocaleCompare("Bessel",option) == 0)
                  filter=BesselFilter;
                if (LocaleCompare("Sinc",option) == 0)
                  filter=SincFilter;
                if (filter == UndefinedFilter)
                  ThrowMogrifyException(OptionError,"UnrecognizedImageFilter",
                    option);
              }
            break;
          }
        if (LocaleCompare("flip",option+1) == 0)
          break;
        if (LocaleCompare("flop",option+1) == 0)
          break;
        if (LocaleCompare("font",option+1) == 0)
          {
            (void) CloneString(&image_info->font,(char *) NULL);
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowMogrifyException(OptionError,"MissingFontName",
                    option);
                (void) CloneString(&image_info->font,argv[i]);
              }
            break;
          }
        if (LocaleCompare("format",option+1) == 0)
          {
            (void) CloneString(&format,(char *) NULL);
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowMogrifyException(OptionError,"Missing image type",
                    option);
                (void) CloneString(&format,argv[i]);
                (void) strncpy(image_info->filename,format,MaxTextExtent-1);
                (void) strcat(image_info->filename,":");
                (void) SetImageInfo(image_info,False,exception);
                if (*image_info->magick == '\0')
                  ThrowMogrifyException(OptionError,
                    "Unrecognized image format",format);
              }
            break;
          }
        if (LocaleCompare("frame",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !IsGeometry(argv[i]))
                  ThrowMogrifyException(OptionError,"MissingGeometry",
                    option);
              }
            break;
          }
        if (LocaleCompare("fuzz",option+1) == 0)
          {
            image_info->fuzz=0.0;
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  ThrowMogrifyException(OptionError,"Missing distance",
                    option);
                image_info->fuzz=StringToDouble(argv[i],MaxRGB);
              }
            break;
          }
        ThrowMogrifyException(OptionError,"UnrecognizedOption",option)
      }
      case 'g':
      {
        if (LocaleCompare("gamma",option+1) == 0)
          {
            i++;
            if ((i == argc) || !sscanf(argv[i],"%lf",&sans))
              ThrowMogrifyException(OptionError,"MissingImageGamma",option);
            break;
          }
        if (LocaleCompare("gaussian",option+1) == 0)
          {
            i++;
            if ((i == argc) || !sscanf(argv[i],"%ld",&x))
              ThrowMogrifyException(OptionError,"MissingGeometry",option);
            break;
          }
        if (LocaleCompare("geometry",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !IsGeometry(argv[i]))
                  ThrowMogrifyException(OptionError,"MissingGeometry",
                    option);
              }
            break;
          }
        if (LocaleCompare("gravity",option+1) == 0)
          {
            GravityType
              gravity;

            gravity=ForgetGravity;
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowMogrifyException(OptionError,"MissingType",option);
                option=argv[i];
                if (LocaleCompare("NorthWest",option) == 0)
                  gravity=NorthWestGravity;
                if (LocaleCompare("North",option) == 0)
                  gravity=NorthGravity;
                if (LocaleCompare("NorthEast",option) == 0)
                  gravity=NorthEastGravity;
                if (LocaleCompare("West",option) == 0)
                  gravity=WestGravity;
                if (LocaleCompare("Center",option) == 0)
                  gravity=CenterGravity;
                if (LocaleCompare("East",option) == 0)
                  gravity=EastGravity;
                if (LocaleCompare("SouthWest",option) == 0)
                  gravity=SouthWestGravity;
                if (LocaleCompare("South",option) == 0)
                  gravity=SouthGravity;
                if (LocaleCompare("SouthEast",option) == 0)
                  gravity=SouthEastGravity;
                if (gravity == ForgetGravity)
                  ThrowMogrifyException(OptionError,"UnrecognizedGravityType",
                    option);
              }
            break;
          }
        if (LocaleCompare("green-primary",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  ThrowMogrifyException(OptionError,"MissingPoint",option);
              }
            break;
          }
        ThrowMogrifyException(OptionError,"UnrecognizedOption",option)
      }
      case 'h':
      {
        if (LocaleCompare("help",option+1) == 0)
          break;
        ThrowMogrifyException(OptionError,"UnrecognizedOption",option)
      }
      case 'i':
      {
        if (LocaleCompare("implode",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%lf",&sans))
                  ThrowMogrifyException(OptionError,"Missing amount",option);
              }
            break;
          }
        if (LocaleCompare("interlace",option+1) == 0)
          {
            image_info->interlace=NoInterlace;
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowMogrifyException(OptionError,"MissingType",option);
                option=argv[i];
                image_info->interlace=UndefinedInterlace;
                if (LocaleCompare("None",option) == 0)
                  image_info->interlace=NoInterlace;
                if (LocaleCompare("Line",option) == 0)
                  image_info->interlace=LineInterlace;
                if (LocaleCompare("Plane",option) == 0)
                  image_info->interlace=PlaneInterlace;
                if (LocaleCompare("Partition",option) == 0)
                  image_info->interlace=PartitionInterlace;
                if (image_info->interlace == UndefinedInterlace)
                  ThrowMogrifyException(OptionError,"UnrecognizedInterlaceType",
                    option);
              }
            break;
          }
        ThrowMogrifyException(OptionError,"UnrecognizedOption",option)
      }
      case 'l':
      {
        if (LocaleCompare("label",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowMogrifyException(OptionError,"MissingImageLabel",
                    option);
              }
            break;
          }
        if (LocaleCompare("lat",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%lf",&sans))
                  ThrowMogrifyException(OptionError,"MissingGeometry",option);
              }
            break;
          }
        if (LocaleCompare("level",option+1) == 0)
          {
            i++;
            if ((i == argc) || !sscanf(argv[i],"%lf",&sans))
              ThrowMogrifyException(OptionError,"MissingImageGamma",option);
            break;
          }
        if (LocaleCompare("linewidth",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  ThrowMogrifyException(OptionError,"MissingPointsize",option);
              }
            break;
          }
        if (LocaleCompare("limit",option+1) == 0)
          {
            if (*option == '-')
              {
                char
                  *type;

                i++;
                if (i == argc)
                  ThrowMogrifyException(OptionError,"MissingResourceType",
                    option);
                type=argv[i];
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  ThrowMogrifyException(OptionError,"MissingResourceLimit",
                    option);
                if (LocaleCompare("disk",type) == 0)
                  SetMagickResourceLimit(DiskResource,atol(argv[i]));
                else
                  if (LocaleCompare("map",type) == 0)
                    SetMagickResourceLimit(MapResource,atol(argv[i]));
                  else
                    if (LocaleCompare("memory",type) == 0)
                      SetMagickResourceLimit(MemoryResource,atol(argv[i]));
                    else
                      ThrowMogrifyException(OptionError,
                        "UnrecognizedResourceType",type);
              }
            break;
          }
        if (LocaleCompare("list",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowMogrifyException(OptionError,"Missing list name",
                    option);
                option=argv[i];
                switch (*option)
                {
                  case 'C':
                  case 'c':
                  {
                    if (LocaleCompare("Color",option) == 0)
                      {
                        (void) ListColorInfo((FILE *) NULL,exception);
                        break;
                      }
                    ThrowMogrifyException(OptionError,"UnrecognizedListType",
                      option)
                  }
                  case 'D':
                  case 'd':
                  {
                    if (LocaleCompare("Delegate",option) == 0)
                      {
                        (void) ListDelegateInfo((FILE *) NULL,exception);
                        break;
                      }
                    ThrowMogrifyException(OptionError,"UnrecognizedListType",
                      option)
                  }
                  case 'F':
                  case 'f':
                  {
                    if (LocaleCompare("Format",option) == 0)
                      {
                        (void) ListMagickInfo((FILE *) NULL,exception);
                        break;
                      }
                    ThrowMogrifyException(OptionError,"UnrecognizedListType",
                      option)
                  }
                  case 'M':
                  case 'm':
                  {
                    if (LocaleCompare("Magic",option) == 0)
                      {
                        (void) ListMagicInfo((FILE *) NULL,exception);
                        break;
                      }
#if defined(SupportMagickModules)
                    if (LocaleCompare("Module",option) == 0)
                      {
                        (void) ListModuleInfo((FILE *) NULL,exception);
                        break;
                      }
#endif /* SupportMagickModules */
                    ThrowMogrifyException(OptionError,"UnrecognizedListType",
                      option)
                  }
                  case 'R':
                  case 'r':
                  {
                    if (LocaleCompare("Resource",option) == 0)
                      {
                        (void) ListMagickResourceInfo((FILE *) NULL,exception);
                        break;
                      }
                    ThrowMogrifyException(OptionError,"UnrecognizedListType",
                      option)
                  }
                  case 'T':
                  case 't':
                  {
                    if (LocaleCompare("Type",option) == 0)
                      {
                        (void) ListTypeInfo((FILE *) NULL,exception);
                        break;
                      }
                    ThrowMogrifyException(OptionError,"UnrecognizedListType",
                      option)
                  }
                  default:
                    ThrowMogrifyException(OptionError,"UnrecognizedListType",
                      option)
                }
                return(True);
              }
            break;
          }
        if (LocaleCompare("log",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowMogrifyException(OptionError,"MissingLogFormat",option);
                (void) SetLogFormat(argv[i]);
              }
            break;
          }
        if (LocaleCompare("loop",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  ThrowMogrifyException(OptionError,"Missing iterations",
                    option);
              }
            break;
          }
        ThrowMogrifyException(OptionError,"UnrecognizedOption",option)
      }
      case 'm':
      {
        if (LocaleCompare("map",option+1) == 0)
          {
            global_colormap=(*option == '+');
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowMogrifyException(OptionError,"Missing file name",
                    option);
              }
            break;
          }
        if (LocaleCompare("mask",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowMogrifyException(OptionError,"Missing file name",
                    option);
              }
            break;
          }
        if (LocaleCompare("matte",option+1) == 0)
          break;
        if (LocaleCompare("mattecolor",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowMogrifyException(OptionError,"Missing matte color",
                    option);
                (void) QueryColorDatabase(argv[i],&image_info->matte_color,
                  exception);
              }
            break;
          }
        if (LocaleCompare("modulate",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%lf",&sans))
                  ThrowMogrifyException(OptionError,"MissingImageGamma",option);
              }
            break;
          }
        if (LocaleCompare("median",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  ThrowMogrifyException(OptionError,"MissingPaintRadius",option);
              }
            break;
          }
        if (LocaleCompare("monochrome",option+1) == 0)
          {
            image_info->monochrome=(*option == '-');
            break;
          }
        ThrowMogrifyException(OptionError,"UnrecognizedOption",option)
      }
      case 'n':
      {
        if (LocaleCompare("negate",option+1) == 0)
          break;
        if (LocaleCompare("noise",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%lf",&sans))
                  ThrowMogrifyException(OptionError,"MissingImageGamma",option);
              }
            if (*option == '+')
              {
                i++;
                if (i == argc)
                  ThrowMogrifyException(OptionError,"MissingType",option);
                option=argv[i];
                if ((LocaleCompare("Uniform",option) != 0) &&
                    (LocaleCompare("Gaussian",option) != 0) &&
                    (LocaleCompare("Multiplicative",option) != 0) &&
                    (LocaleCompare("Impulse",option) != 0) &&
                    (LocaleCompare("Laplacian",option) != 0) &&
                    (LocaleCompare("Poisson",option) != 0))
                  ThrowMogrifyException(OptionError,"UnrecognizedNoiseType",
                    option);
              }
            break;
          }
        if (LocaleCompare("noop",option+1) == 0)
          break;
        if (LocaleCompare("normalize",option+1) == 0)
          break;
        ThrowMogrifyException(OptionError,"UnrecognizedOption",option)
      }
      case 'o':
      {
        if (LocaleCompare("opaque",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowMogrifyException(OptionError,"Missing opaque color",
                    option);
              }
            break;
          }
        if (LocaleCompare("ordered-dither",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowMogrifyException(OptionError,"MissingType",option);
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  ThrowMogrifyException(OptionError,"MissingOrder",option);
              }
            break;
          }
        ThrowMogrifyException(OptionError,"UnrecognizedOption",option)
      }
      case 'p':
      {
        if (LocaleCompare("page",option+1) == 0)
          {
            (void) CloneString(&image_info->page,(char *) NULL);
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowMogrifyException(OptionError,"MissingPageGeometry",
                    option);
                image_info->page=GetPageGeometry(argv[i]);
              }
            break;
          }
        if (LocaleCompare("paint",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  ThrowMogrifyException(OptionError,"MissingPaintRadius",
                    option);
              }
            break;
          }
        if (LocaleCompare("pointsize",option+1) == 0)
          {
            image_info->pointsize=12;
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  ThrowMogrifyException(OptionError,"MissingPointsize",option);
                image_info->pointsize=atof(argv[i]);
              }
            break;
          }
        if (LocaleCompare("profile",option+1) == 0)
          {
            i++;
            if (i == argc)
              ThrowMogrifyException(OptionError,"Missing profile",option);
            break;
          }
        ThrowMogrifyException(OptionError,"UnrecognizedOption",option)
      }
      case 'q':
      {
        if (LocaleCompare("quality",option+1) == 0)
          {
            image_info->quality=DefaultCompressionQuality;
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !IsGeometry(argv[i]))
                  ThrowMogrifyException(OptionError,"MissingCompressionQuality",
                    option);
                image_info->quality=atol(argv[i]);
              }
            break;
          }
        ThrowMogrifyException(OptionError,"UnrecognizedOption",option)
      }
      case 'r':
      {
        if (LocaleCompare("raise",option+1) == 0)
          {
            i++;
            if ((i == argc) || !sscanf(argv[i],"%ld",&x))
              ThrowMogrifyException(OptionError,"Missing bevel width",option);
            break;
          }
        if (LocaleCompare("random-threshold",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowMogrifyException(OptionError,"MissingType",option);
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  ThrowMogrifyException(OptionError,"MissingThreshold",option);
              }
          break;
          }
        if (LocaleCompare("red-primary",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  ThrowMogrifyException(OptionError,"MissingPoint",option);
              }
            break;
          }
        if (LocaleCompare("region",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !IsGeometry(argv[i]))
                  ThrowMogrifyException(OptionError,"MissingGeometry",
                    option);
              }
            break;
          }
        if (LocaleCompare("render",option+1) == 0)
          break;
        if (LocaleCompare("resample",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == (argc-1)) || !IsGeometry(argv[i]))
                  ThrowMogrifyException(OptionError,"MissingGeometry",option);
              }
            break;
          }
        if (LocaleCompare("resize",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !IsGeometry(argv[i]))
                  ThrowMogrifyException(OptionError,"MissingGeometry",option);
              }
            break;
          }
        if (LocaleCompare("resize",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !IsGeometry(argv[i]))
                  ThrowMogrifyException(OptionError,"MissingGeometry",
                    option);
              }
            break;
          }
        if (LocaleCompare("roll",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !IsGeometry(argv[i]))
                  ThrowMogrifyException(OptionError,"MissingGeometry",
                    option);
              }
            break;
          }
        if (LocaleCompare("rotate",option+1) == 0)
          {
            i++;
            if ((i == argc) || !IsGeometry(argv[i]))
              ThrowMogrifyException(OptionError,"MissingImageRotation",option);
            break;
          }
        ThrowMogrifyException(OptionError,"UnrecognizedOption",option)
      }
      case 's':
      {
        if (LocaleCompare("sample",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !IsGeometry(argv[i]))
                  ThrowMogrifyException(OptionError,"MissingGeometry",
                    option);
              }
            break;
          }
        if (LocaleCompare("sampling-factor",option+1) == 0)
          {
            (void) CloneString(&image_info->sampling_factor,(char *) NULL);
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !IsGeometry(argv[i]))
                  ThrowMogrifyException(OptionError,"MissingGeometry",
                    option);
                (void) CloneString(&image_info->sampling_factor,argv[i]);
              }
            break;
          }
        if (LocaleCompare("scale",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !IsGeometry(argv[i]))
                  ThrowMogrifyException(OptionError,"MissingGeometry",
                    option);
              }
            break;
          }
        if (LocaleCompare("scene",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  ThrowMogrifyException(OptionError,"MissingImageScene",
                    option);
              }
            break;
          }
        if (LocaleCompare("segment",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%lf",&sans))
                  ThrowMogrifyException(OptionError,"MissingThreshold",
                    option);
              }
            break;
          }
        if (LocaleCompare("shade",option+1) == 0)
          {
            i++;
            if ((i == argc) || !sscanf(argv[i],"%ld",&x))
              ThrowMogrifyException(OptionError,"Missing azimuth",option);
            break;
          }
        if (LocaleCompare("sharpen",option+1) == 0)
          {
            i++;
            if ((i == argc) || !sscanf(argv[i],"%ld",&x))
              ThrowMogrifyException(OptionError,"MissingGeometry",option);
            break;
          }
        if (LocaleCompare("shave",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !IsGeometry(argv[i]))
                  ThrowMogrifyException(OptionError,"MissingGeometry",
                    option);
              }
            break;
          }
        if (LocaleCompare("shear",option+1) == 0)
          {
            i++;
            if ((i == argc) || !sscanf(argv[i],"%lf",&sans))
              ThrowMogrifyException(OptionError,"MissingGeometry",option);
            break;
          }
        if (LocaleCompare("size",option+1) == 0)
          {
            (void) CloneString(&image_info->size,(char *) NULL);
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !IsGeometry(argv[i]))
                  ThrowMogrifyException(OptionError,"MissingGeometry",
                    option);
                (void) CloneString(&image_info->size,argv[i]);
              }
            break;
          }
        if (LocaleCompare("solarize",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  ThrowMogrifyException(OptionError,"MissingThreshold",
                    option);
              }
            break;
          }
        if (LocaleCompare("spread",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  ThrowMogrifyException(OptionError,"Missing amount",option);
              }
            break;
          }
        if (LocaleCompare("stroke",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowMogrifyException(OptionError,"MissingColors",option);
              }
            break;
          }
        if (LocaleCompare("strokewidth",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  ThrowMogrifyException(OptionError,"MissingPointsize",option);
              }
            break;
          }
        if (LocaleCompare("swirl",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%lf",&sans))
                  ThrowMogrifyException(OptionError,"MissingImageRotation",option);
              }
            break;
          }
        ThrowMogrifyException(OptionError,"UnrecognizedOption",option)
      }
      case 't':
      {
        if (LocaleCompare("texture",option+1) == 0)
          {
            (void) CloneString(&image_info->texture,(char *) NULL);
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowMogrifyException(OptionError,"MissingFilename",
                    option);
                (void) CloneString(&image_info->texture,argv[i]);
              }
            break;
          }
        if (LocaleCompare("threshold",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  ThrowMogrifyException(OptionError,"MissingImageGamma",option);
              }
            break;
          }
        if (LocaleCompare("tile",option+1) == 0)
          {
            i++;
            if (i == argc)
              ThrowMogrifyException(OptionError,"Missing tile",option);
            break;
          }
        if (LocaleCompare("transform",option+1) == 0)
          break;
        if (LocaleCompare("transparent",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowMogrifyException(OptionError,"MissingTransparentColor",
                    option);
              }
            break;
          }
        if (LocaleCompare("treedepth",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  ThrowMogrifyException(OptionError,"MissingTreeDepth",
                    option);
              }
            break;
          }
        if (LocaleCompare("trim",option+1) == 0)
          break;
        if (LocaleCompare("type",option+1) == 0)
          {
            image_info->type=UndefinedType;
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowMogrifyException(OptionError,"MissingType",option);
                option=argv[i];
                if (LocaleCompare("Bilevel",option) == 0)
                  image_info->type=BilevelType;
                if (LocaleCompare("Grayscale",option) == 0)
                  image_info->type=GrayscaleType;
                if (LocaleCompare("GrayscaleMatte",option) == 0)
                  image_info->type=GrayscaleMatteType;
                if (LocaleCompare("Palette",option) == 0)
                  image_info->type=PaletteType;
                if (LocaleCompare("PaletteMatte",option) == 0)
                  image_info->type=PaletteMatteType;
                if (LocaleCompare("TrueColor",option) == 0)
                  image_info->type=TrueColorType;
                if (LocaleCompare("TrueColorMatte",option) == 0)
                  image_info->type=TrueColorMatteType;
                if (LocaleCompare("ColorSeparation",option) == 0)
                  image_info->type=ColorSeparationType;
                if (LocaleCompare("ColorSeparationMatte",option) == 0)
                  image_info->type=ColorSeparationMatteType;
                if (LocaleCompare("Optimize",option) == 0)
                  image_info->type=OptimizeType;
                if (image_info->type == UndefinedType)
                  ThrowMogrifyException(OptionError,"UnrecognizedImageType",
                    option);
              }
            break;
          }
        ThrowMogrifyException(OptionError,"UnrecognizedOption",option)
      }
      case 'u':
      {
        if (LocaleCompare("undercolor",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowMogrifyException(OptionError,"MissingColors",option);
              }
            break;
          }
        if (LocaleCompare("units",option+1) == 0)
          {
            image_info->units=UndefinedResolution;
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowMogrifyException(OptionError,"MissingType",option);
                option=argv[i];
                image_info->units=UndefinedResolution;
                if (LocaleCompare("PixelsPerInch",option) == 0)
                  image_info->units=PixelsPerInchResolution;
                if (LocaleCompare("PixelsPerCentimeter",option) == 0)
                  image_info->units=PixelsPerCentimeterResolution;
              }
            break;
          }
        if (LocaleCompare("unsharp",option+1) == 0)
          {
            i++;
            if ((i == argc) || !sscanf(argv[i],"%lf",&sans))
              ThrowMogrifyException(OptionError,"MissingGeometry",option);
            break;
          }
        ThrowMogrifyException(OptionError,"UnrecognizedOption",option)
      }
      case 'v':
      {
        if (LocaleCompare("verbose",option+1) == 0)
          {
            image_info->verbose=(*option == '-');
            break;
          }
        if (LocaleCompare("verbose",option+1) == 0)
          break;
        if (LocaleCompare("view",option+1) == 0)
          {
            (void) CloneString(&image_info->view,(char *) NULL);
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowMogrifyException(OptionError,"MissingViewTransform",
                    option);
                (void) CloneString(&image_info->view,argv[i]);
              }
            break;
          }
        if (LocaleCompare("virtual-pixel",option+1) == 0)
          {
            if (*option == '-')
              {
                VirtualPixelMethod
                  virtual_pixel_method;

                i++;
                if (i == argc)
                  ThrowMogrifyException(OptionError,
                    "MissingVirtualPixelMethod",option);
                option=argv[i];
                virtual_pixel_method=UndefinedVirtualPixelMethod;
                if (LocaleCompare("Constant",option) == 0)
                  virtual_pixel_method=ConstantVirtualPixelMethod;
                if (LocaleCompare("Edge",option) == 0)
                  virtual_pixel_method=EdgeVirtualPixelMethod;
                if (LocaleCompare("Mirror",option) == 0)
                  virtual_pixel_method=MirrorVirtualPixelMethod;
                if (LocaleCompare("Tile",option) == 0)
                  virtual_pixel_method=TileVirtualPixelMethod;
                if (virtual_pixel_method == UndefinedVirtualPixelMethod)
                  ThrowMogrifyException(OptionError,
                    "UnrecognizedVirtualPixelMethod",option);
              }
            break;
          }
        ThrowMogrifyException(OptionError,"UnrecognizedOption",option)
      }
      case 'w':
      {
        if (LocaleCompare("wave",option+1) == 0)
          {
            i++;
            if ((i == argc) || !sscanf(argv[i],"%ld",&x))
              ThrowMogrifyException(OptionError,"Missing amplitude",option);
            break;
          }
        if (LocaleCompare("white-point",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  ThrowMogrifyException(OptionError,"MissingPoint",option);
              }
            break;
          }
        ThrowMogrifyException(OptionError,"UnrecognizedOption",option)
      }
      case '?':
        break;
      default:
        ThrowMogrifyException(OptionError,"UnrecognizedOption",option)
    }
  }
  if ((i != argc) || (image == (Image *) NULL))
    {
      if (exception->severity == UndefinedException)
        ThrowMogrifyException(OptionError,"MissingAnImageFilename",
          (char *) NULL);
      return(False);
    }
  LiberateArgumentList(argc,argv);
  return(status);
}
#undef ThrowMogrifyException

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%    M o n t a g e I m a g e C o m m a n d                                    %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MontageImageCommand() reads one or more images, applies one or more image
%  processing operations, and writes out the image in the same or
%  differing format.
%
%  The format of the MontageImageCommand method is:
%
%      unsigned int MontageImageCommand(ImageInfo *image_info,const int argc,
%        char **argv,char **metadata,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: The image info.
%
%    o argc: The number of elements in the argument vector.
%
%    o argv: A text array containing the command line arguments.
%
%    o metadata: any metadata is returned here.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
#define ThrowMontageException(code,reason,description) \
{ \
  DestroyImageList(image); \
  DestroyImageList(image_list); \
  DestroyImageList(montage_image); \
  ThrowException(exception,code,reason,description); \
  LiberateArgumentList(argc,argv); \
  return(False); \
}
MagickExport unsigned int MontageImageCommand(ImageInfo *image_info,
  int argc,char **argv,char **metadata,ExceptionInfo *exception)
{
  char
    *format,
    *option,
    *transparent_color;

  double
    sans;

  Image
    *image,
    *image_list,
    *montage_image,
    *next_image;

  long
    first_scene,
    j,
    k,
    last_scene,
    scene,
    x;

  MontageInfo
    *montage_info;

  QuantizeInfo
    quantize_info;

  register long
    i;

  unsigned int
    status;

  /*
    Set defaults.
  */
  format=(char *) NULL;
  first_scene=0;
  image=NewImageList();
  image_list=(Image *) NULL;
  montage_image=NewImageList();
  last_scene=0;

  /*
    Expand argument list
  */
  status=ExpandFilenames(&argc,&argv);
  if (status == False)
    MagickFatalError(ResourceLimitFatalError,"MemoryAllocationFailed",
      (char *) NULL);

  /*
    Validate command line.
  */
  if (argc < 2 || ((argc < 3) && (LocaleCompare("-help",argv[1]) == 0 ||
      LocaleCompare("-?",argv[1]) == 0)))
    {
      MontageUsage();
      ThrowMontageException(OptionError,"UsageError",NULL);
    }

  (void) strncpy(image_info->filename,argv[argc-1],MaxTextExtent-1);
  (void) SetImageInfo(image_info,True,exception);
  montage_info=CloneMontageInfo(image_info,(MontageInfo *) NULL);
  GetQuantizeInfo(&quantize_info);
  quantize_info.number_colors=0;
  scene=0;
  status=True;
  transparent_color=(char *) NULL;

  j=1;
  k=0;
  for (i=1; i < (argc-1); i++)
  {
    option=argv[i];
    if ((strlen(option) == 1) || ((*option != '-') && (*option != '+')))
      {
        k=i;
        for (scene=first_scene; scene <= last_scene ; scene++)
        {
          /*
            Option is a file name: begin by reading image from specified file.
          */
          (void) strncpy(image_info->filename,argv[i],MaxTextExtent-1);
          if (first_scene != last_scene)
            {
              char
                filename[MaxTextExtent];

              /*
                Form filename for multi-part images.
              */
              FormatString(filename,image_info->filename,scene);
              if (LocaleCompare(filename,image_info->filename) == 0)
                FormatString(filename,"%.1024s.%lu",image_info->filename,scene);
              (void) strncpy(image_info->filename,filename,MaxTextExtent-1);
            }
          (void) CloneString(&image_info->font,montage_info->font);
          image_info->colorspace=quantize_info.colorspace;
          image_info->dither=quantize_info.dither;
          if (image_info->size == (char *) NULL)
            (void) CloneString(&image_info->size,montage_info->geometry);
          next_image=ReadImage(image_info,exception);
          status&=(next_image != (Image *) NULL) &&
            (exception->severity < ErrorException);
          if (next_image == (Image *) NULL)
            continue;
          if (image == (Image *) NULL)
            {
              image=next_image;
              continue;
            }
          AppendImageToList(&image,next_image);
        }
        continue;
      }
    if ((image != (Image *) NULL) && (j != (k+1)))
      {
        status&=MogrifyImages(image_info,i-j,argv+j,&image);
        (void) CatchImageException(image);
        AppendImageToList(&image_list,image);
        image=NewImageList();
        j=k+1;
      }
    switch (*(option+1))
    {
      case 'a':
      {
        if (LocaleCompare("adjoin",option+1) == 0)
          {
            image_info->adjoin=(*option == '-');
            break;
          }
        if (LocaleCompare("authenticate",option+1) == 0)
          {
            (void) CloneString(&image_info->authenticate,(char *) NULL);
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowMontageException(OptionError,"MissingPassword",option);
                (void) CloneString(&image_info->authenticate,argv[i]);
              }
            break;
          }
        ThrowMontageException(OptionError,"UnrecognizedOption",option)
      }
      case 'b':
      {
        if (LocaleCompare("background",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowMontageException(OptionError,"MissingColors",option);
                (void) QueryColorDatabase(argv[i],
                  &montage_info->background_color,exception);
                (void) QueryColorDatabase(argv[i],&image_info->background_color,
                  exception);
              }
            break;
          }
        if (LocaleCompare("blue-primary",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  ThrowMontageException(OptionError,"MissingPoint",option);
              }
            break;
          }
        if (LocaleCompare("blur",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  ThrowMontageException(OptionError,"MissingGeometry",option);
              }
            break;
          }
        if (LocaleCompare("bordercolor",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowMontageException(OptionError,"MissingColors",option);
                (void) QueryColorDatabase(argv[i],&montage_info->border_color,
                  exception);
                (void) QueryColorDatabase(argv[i],&image_info->border_color,
                  exception);
              }
            break;
          }
        if (LocaleCompare("borderwidth",option+1) == 0)
          {
            montage_info->border_width=0;
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  ThrowMontageException(OptionError,"MissingWidth",option);
                montage_info->border_width=atol(argv[i]);
              }
            break;
          }
        ThrowMontageException(OptionError,"UnrecognizedOption",option)
      }
      case 'c':
      {
        if (LocaleCompare("cache",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  ThrowMontageException(OptionError,"MissingThreshold",option);
                SetMagickResourceLimit(MemoryResource,atol(argv[i]));
                SetMagickResourceLimit(MapResource,2*atol(argv[i]));
              }
            break;
          }
        if (LocaleCompare("colors",option+1) == 0)
          {
            quantize_info.number_colors=0;
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  ThrowMontageException(OptionError,"MissingColors",option);
                quantize_info.number_colors=atol(argv[i]);
              }
            break;
          }
        if (LocaleCompare("colorspace",option+1) == 0)
          {
            quantize_info.colorspace=RGBColorspace;
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowMontageException(OptionError,"MissingType",option);
                option=argv[i];
                quantize_info.colorspace=UndefinedColorspace;
                if (LocaleCompare("cmyk",option) == 0)
                  quantize_info.colorspace=CMYKColorspace;
                if (LocaleCompare("gray",option) == 0)
                  {
                    quantize_info.colorspace=GRAYColorspace;
                    quantize_info.number_colors=256;
                    quantize_info.tree_depth=8;
                  }
                if (LocaleCompare("hsl",option) == 0)
                  quantize_info.colorspace=HSLColorspace;
                if (LocaleCompare("hwb",option) == 0)
                  quantize_info.colorspace=HWBColorspace;
                if (LocaleCompare("ohta",option) == 0)
                  quantize_info.colorspace=OHTAColorspace;
                if (LocaleCompare("rgb",option) == 0)
                  quantize_info.colorspace=RGBColorspace;
                if (LocaleCompare("srgb",option) == 0)
                  quantize_info.colorspace=sRGBColorspace;
                if (LocaleCompare("transparent",option) == 0)
                  quantize_info.colorspace=TransparentColorspace;
                if (LocaleCompare("xyz",option) == 0)
                  quantize_info.colorspace=XYZColorspace;
                if (LocaleCompare("ycbcr",option) == 0)
                  quantize_info.colorspace=YCbCrColorspace;
                if (LocaleCompare("ycc",option) == 0)
                  quantize_info.colorspace=YCCColorspace;
                if (LocaleCompare("yiq",option) == 0)
                  quantize_info.colorspace=YIQColorspace;
                if (LocaleCompare("ypbpr",option) == 0)
                  quantize_info.colorspace=YPbPrColorspace;
                if (LocaleCompare("yuv",option) == 0)
                  quantize_info.colorspace=YUVColorspace;
                if (quantize_info.colorspace == UndefinedColorspace)
                  ThrowMontageException(OptionError,"UnrecognizedColorspace",
                    option);
              }
            break;
          }
        if (LocaleCompare("comment",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowMontageException(OptionError,"MissingComment",option);
              }
            break;
          }
        if (LocaleCompare("compress",option+1) == 0)
          {
            image_info->compression=NoCompression;
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowMontageException(OptionError,"MissingType",option);
                option=argv[i];
                image_info->compression=UndefinedCompression;
                if (LocaleCompare("None",option) == 0)
                  image_info->compression=NoCompression;
                if (LocaleCompare("BZip",option) == 0)
                  image_info->compression=BZipCompression;
                if (LocaleCompare("Fax",option) == 0)
                  image_info->compression=FaxCompression;
                if (LocaleCompare("Group4",option) == 0)
                  image_info->compression=Group4Compression;
                if (LocaleCompare("JPEG",option) == 0)
                  image_info->compression=JPEGCompression;
                if (LocaleCompare("Lossless",option) == 0)
                  image_info->compression=LosslessJPEGCompression;
                if (LocaleCompare("LZW",option) == 0)
                  image_info->compression=LZWCompression;
                if (LocaleCompare("RLE",option) == 0)
                  image_info->compression=RLECompression;
                if (LocaleCompare("Zip",option) == 0)
                  image_info->compression=ZipCompression;
                if (image_info->compression == UndefinedCompression)
                  ThrowMontageException(OptionError,"UnrecognizedImageCompression",
                    option);
              }
            break;
          }
        if (LocaleCompare("compose",option+1) == 0)
          {
            CompositeOperator
              compose;

            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowMontageException(OptionError,"MissingType",option);
                option=argv[i];
                compose=UndefinedCompositeOp;
                if (LocaleCompare("Over",option) == 0)
                  compose=OverCompositeOp;
                if (LocaleCompare("In",option) == 0)
                  compose=InCompositeOp;
                if (LocaleCompare("Out",option) == 0)
                  compose=OutCompositeOp;
                if (LocaleCompare("Atop",option) == 0)
                  compose=AtopCompositeOp;
                if (LocaleCompare("Xor",option) == 0)
                  compose=XorCompositeOp;
                if (LocaleCompare("Plus",option) == 0)
                  compose=PlusCompositeOp;
                if (LocaleCompare("Minus",option) == 0)
                  compose=MinusCompositeOp;
                if (LocaleCompare("Add",option) == 0)
                  compose=AddCompositeOp;
                if (LocaleCompare("Subtract",option) == 0)
                  compose=SubtractCompositeOp;
                if (LocaleCompare("Difference",option) == 0)
                  compose=DifferenceCompositeOp;
                if (LocaleCompare("Multiply",option) == 0)
                  compose=MultiplyCompositeOp;
                if (LocaleCompare("Bumpmap",option) == 0)
                  compose=BumpmapCompositeOp;
                if (LocaleCompare("Copy",option) == 0)
                  compose=CopyCompositeOp;
                if (LocaleCompare("CopyRed",option) == 0)
                  compose=CopyRedCompositeOp;
                if (LocaleCompare("CopyGreen",option) == 0)
                  compose=CopyGreenCompositeOp;
                if (LocaleCompare("CopyBlue",option) == 0)
                  compose=CopyBlueCompositeOp;
                if (LocaleCompare("CopyOpacity",option) == 0)
                  compose=CopyOpacityCompositeOp;
                if (compose == UndefinedCompositeOp)
                  ThrowMontageException(OptionError,"UnrecognizedComposeOperator",
                    option);
              }
            break;
          }
        if (LocaleCompare("crop",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !IsGeometry(argv[i]))
                  ThrowMontageException(OptionError,"MissingGeometry",option);
              }
            break;
          }
        ThrowMontageException(OptionError,"UnrecognizedOption",option)
      }
      case 'd':
      {
        if (LocaleCompare("debug",option+1) == 0)
          {
            (void) SetLogEventMask("None");
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowMontageException(OptionError,"MissingEventMask",option);
                (void) SetLogEventMask(argv[i]);
              }
            break;
          }
        if (LocaleCompare("density",option+1) == 0)
          {
            (void) CloneString(&image_info->density,(char *) NULL);
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !IsGeometry(argv[i]))
                  ThrowMontageException(OptionError,"MissingGeometry",option);
                (void) CloneString(&image_info->density,argv[i]);
              }
            break;
          }
        if (LocaleCompare("depth",option+1) == 0)
          {
            image_info->depth=QuantumDepth;
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  ThrowMontageException(OptionError,"MissingImageDepth",option);
                image_info->depth=atol(argv[i]);
              }
            break;
          }
        if (LocaleCompare("display",option+1) == 0)
          {
            (void) CloneString(&image_info->server_name,(char *) NULL);
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowMontageException(OptionError,"MissingServerName",
                    option);
                (void) CloneString(&image_info->server_name,argv[i]);
              }
            break;
          }
        if (LocaleCompare("dispose",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowMontageException(OptionError,"MissingDisposeMethod",
                    option);
                option=argv[i];
                if ((LocaleCompare("0",option) != 0) &&
                    (LocaleCompare("1",option) != 0) &&
                    (LocaleCompare("2",option) != 0) &&
                    (LocaleCompare("3",option) != 0) &&
                    (LocaleCompare("Undefined",option) != 0) &&
                    (LocaleCompare("None",option) != 0) &&
                    (LocaleCompare("Background",option) != 0) &&
                    (LocaleCompare("Previous",option) != 0))
                  ThrowMontageException(OptionError,"UnrecognizedDisposeMethod",
                    option);
              }
            break;
          }
        if (LocaleCompare("dither",option+1) == 0)
          {
            quantize_info.dither=(*option == '-');
            break;
          }
        if (LocaleCompare("draw",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowMontageException(OptionError,"MissingDrawingPrimitive",
                    option);
              }
            break;
          }
        ThrowMontageException(OptionError,"UnrecognizedOption",option)
      }
      case 'e':
      {
        if (LocaleCompare("encoding",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowMontageException(OptionError,"MissingEnodingType",
                    option);
              }
            break;
          }
        if (LocaleCompare("endian",option+1) == 0)
          {
            image_info->endian=UndefinedEndian;
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowMontageException(OptionError,"MissingType",option);
                option=argv[i];
                image_info->endian=UndefinedEndian;
                if (LocaleCompare("LSB",option) == 0)
                  image_info->endian=LSBEndian;
                if (LocaleCompare("MSB",option) == 0)
                  image_info->endian=MSBEndian;
                if (image_info->endian == UndefinedEndian)
                  ThrowMontageException(OptionError,"UnrecognizedEndianType",
                    option);
              }
            break;
          }
        ThrowMontageException(OptionError,"UnrecognizedOption",option)
      }
      case 'f':
      {
        if (LocaleCompare("fill",option+1) == 0)
          {
            (void) QueryColorDatabase("none",&image_info->pen,exception);
            (void) QueryColorDatabase("none",&montage_info->fill,exception);
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowMontageException(OptionError,"MissingFillColor",option);
                (void) QueryColorDatabase(argv[i],&image_info->pen,exception);
                (void) QueryColorDatabase(argv[i],&montage_info->fill,
                  exception);
              }
            break;
          }
        if (LocaleCompare("filter",option+1) == 0)
          {
            if (*option == '-')
              {
                FilterTypes
                  filter;

                i++;
                if (i == argc)
                  ThrowMontageException(OptionError,"MissingType",option);
                option=argv[i];
                filter=UndefinedFilter;
                if (LocaleCompare("Point",option) == 0)
                  filter=PointFilter;
                if (LocaleCompare("Box",option) == 0)
                  filter=BoxFilter;
                if (LocaleCompare("Triangle",option) == 0)
                  filter=TriangleFilter;
                if (LocaleCompare("Hermite",option) == 0)
                  filter=HermiteFilter;
                if (LocaleCompare("Hanning",option) == 0)
                  filter=HanningFilter;
                if (LocaleCompare("Hamming",option) == 0)
                  filter=HammingFilter;
                if (LocaleCompare("Blackman",option) == 0)
                  filter=BlackmanFilter;
                if (LocaleCompare("Gaussian",option) == 0)
                  filter=GaussianFilter;
                if (LocaleCompare("Quadratic",option) == 0)
                  filter=QuadraticFilter;
                if (LocaleCompare("Cubic",option) == 0)
                  filter=CubicFilter;
                if (LocaleCompare("Catrom",option) == 0)
                  filter=CatromFilter;
                if (LocaleCompare("Mitchell",option) == 0)
                  filter=MitchellFilter;
                if (LocaleCompare("Lanczos",option) == 0)
                  filter=LanczosFilter;
                if (LocaleCompare("Bessel",option) == 0)
                  filter=BesselFilter;
                if (LocaleCompare("Sinc",option) == 0)
                  filter=SincFilter;
                if (filter == UndefinedFilter)
                  ThrowMontageException(OptionError,"UnrecognizedImageFilter",
                    option);
              }
            break;
          }
        if (LocaleCompare("flip",option+1) == 0)
          break;
        if (LocaleCompare("flop",option+1) == 0)
          break;
        if (LocaleCompare("font",option+1) == 0)
          {
            (void) CloneString(&image_info->font,(char *) NULL);
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowMontageException(OptionError,"MissingFontName",option);
                (void) CloneString(&image_info->font,argv[i]);
                (void) CloneString(&montage_info->font,argv[i]);
              }
            break;
          }
        if (LocaleCompare("format",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowMontageException(OptionError,"MissingFormatSpecifier",
                    option);
                format=argv[i];
              }
            break;
          }
        if (LocaleCompare("frame",option+1) == 0)
          {
            (void) CloneString(&montage_info->frame,(char *) NULL);
            (void) strcpy(argv[i]+1,"sans");
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !IsGeometry(argv[i]))
                  ThrowMontageException(OptionError,"MissingGeometry",option);
                (void) CloneString(&montage_info->frame,argv[i]);
              }
            break;
          }
        ThrowMontageException(OptionError,"UnrecognizedOption",option)
      }
      case 'g':
      {
        if (LocaleCompare("gamma",option+1) == 0)
          {
            i++;
            if ((i == argc) || !sscanf(argv[i],"%lf",&sans))
              ThrowMontageException(OptionError,"MissingImageGamma",option);
            break;
          }
        if (LocaleCompare("geometry",option+1) == 0)
          {
            (void) CloneString(&montage_info->geometry,(char *) NULL);
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !IsGeometry(argv[i]))
                  ThrowMontageException(OptionError,"MissingGeometry",option);
                (void) CloneString(&montage_info->geometry,argv[i]);
              }
            break;
          }
        if (LocaleCompare("gravity",option+1) == 0)
          {
            GravityType
              gravity;

            gravity=ForgetGravity;
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowMontageException(OptionError,"MissingType",option);
                option=argv[i];
                if (LocaleCompare("Forget",option) == 0)
                  gravity=ForgetGravity;
                if (LocaleCompare("NorthWest",option) == 0)
                  gravity=NorthWestGravity;
                if (LocaleCompare("North",option) == 0)
                  gravity=NorthGravity;
                if (LocaleCompare("NorthEast",option) == 0)
                  gravity=NorthEastGravity;
                if (LocaleCompare("West",option) == 0)
                  gravity=WestGravity;
                if (LocaleCompare("Center",option) == 0)
                  gravity=CenterGravity;
                if (LocaleCompare("East",option) == 0)
                  gravity=EastGravity;
                if (LocaleCompare("SouthWest",option) == 0)
                  gravity=SouthWestGravity;
                if (LocaleCompare("South",option) == 0)
                  gravity=SouthGravity;
                if (LocaleCompare("SouthEast",option) == 0)
                  gravity=SouthEastGravity;
                if (LocaleCompare("Static",option) == 0)
                  gravity=StaticGravity;
              }
            montage_info->gravity=gravity;
            break;
          }
        if (LocaleCompare("green-primary",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  ThrowMontageException(OptionError,"MissingPoint",option);
              }
            break;
          }
        ThrowMontageException(OptionError,"UnrecognizedOption",option)
      }
      case 'h':
      {
        if (LocaleCompare("help",option+1) == 0)
          break;
        ThrowMontageException(OptionError,"UnrecognizedOption",option)
      }
      case 'i':
      {
        if (LocaleCompare("interlace",option+1) == 0)
          {
            image_info->interlace=NoInterlace;
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowMontageException(OptionError,"MissingType",option);
                option=argv[i];
                image_info->interlace=UndefinedInterlace;
                if (LocaleCompare("None",option) == 0)
                  image_info->interlace=NoInterlace;
                if (LocaleCompare("Line",option) == 0)
                  image_info->interlace=LineInterlace;
                if (LocaleCompare("Plane",option) == 0)
                  image_info->interlace=PlaneInterlace;
                if (LocaleCompare("Partition",option) == 0)
                  image_info->interlace=PartitionInterlace;
                if (image_info->interlace == UndefinedInterlace)
                  ThrowMontageException(OptionError,
                    "UnrecognizedInterlaceType",option);
              }
            break;
          }
        ThrowMontageException(OptionError,"UnrecognizedOption",option)
      }
      case 'l':
      {
        if (LocaleCompare("label",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowMontageException(OptionError,"MissingImageLabel",option);
              }
            break;
          }
        if (LocaleCompare("limit",option+1) == 0)
          {
            if (*option == '-')
              {
                char
                  *type;

                i++;
                if (i == argc)
                  ThrowMontageException(OptionError,"MissingResourceType",
                    option);
                type=argv[i];
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  ThrowMontageException(OptionError,"MissingResourceLimit",
                    option);
                if (LocaleCompare("disk",type) == 0)
                  SetMagickResourceLimit(DiskResource,atol(argv[i]));
                else
                  if (LocaleCompare("map",type) == 0)
                    SetMagickResourceLimit(MapResource,atol(argv[i]));
                  else
                    if (LocaleCompare("memory",type) == 0)
                      SetMagickResourceLimit(MemoryResource,atol(argv[i]));
                    else
                      ThrowMontageException(OptionError,
                        "UnrecognizedResourceType",type);
              }
            break;
          }
        if (LocaleCompare("log",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowMontageException(OptionError,"MissingLogFormat",option);
                (void) SetLogFormat(argv[i]);
              }
            break;
          }
        ThrowMontageException(OptionError,"UnrecognizedOption",option)
      }
      case 'm':
      {
        if (LocaleCompare("matte",option+1) == 0)
          break;
        if (LocaleCompare("mattecolor",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowMontageException(OptionError,"MissingColors",option);
                (void) QueryColorDatabase(argv[i],&montage_info->matte_color,
                  exception);
                (void) QueryColorDatabase(argv[i],&image_info->matte_color,
                  exception);
              }
            break;
          }
        if (LocaleCompare("mode",option+1) == 0)
          {
            if (*option == '-')
              {
                MontageMode
                  mode;

                i++;
                if (i == argc)
                  ThrowMontageException(OptionError,"MissingType",option);
                option=argv[i];
                mode=UndefinedMode;
                if (LocaleCompare("frame",option) == 0)
                  {
                    mode=FrameMode;
                    (void) CloneString(&montage_info->frame,"15x15+3+3");
                    montage_info->shadow=True;
                    break;
                  }
                if (LocaleCompare("unframe",option) == 0)
                  {
                    mode=UnframeMode;
                    montage_info->frame=(char *) NULL;
                    montage_info->shadow=False;
                    montage_info->border_width=0;
                    break;
                  }
                if (LocaleCompare("concatenate",option) == 0)
                  {
                    mode=ConcatenateMode;
                    montage_info->frame=(char *) NULL;
                    montage_info->shadow=False;
                    (void) CloneString(&montage_info->geometry,"+0+0");
                    montage_info->border_width=0;
                    break;
                  }
                if (mode == UndefinedMode)
                  ThrowMontageException(OptionError,"UnrecognizedImageMode",
                    option);
              }
            break;
          }
        if (LocaleCompare("monochrome",option+1) == 0)
          {
            image_info->monochrome=(*option == '-');
            if (image_info->monochrome)
              {
                quantize_info.number_colors=2;
                quantize_info.tree_depth=8;
                quantize_info.colorspace=GRAYColorspace;
              }
            break;
          }
        ThrowMontageException(OptionError,"UnrecognizedOption",option)
      }
      case 'n':
      {
        if (LocaleCompare("noop",option+1) == 0)
          break;
        ThrowMontageException(OptionError,"UnrecognizedOption",option)
      }
      case 'p':
      {
        if (LocaleCompare("page",option+1) == 0)
          {
            (void) CloneString(&image_info->page,(char *) NULL);
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowMontageException(OptionError,"MissingPageGeometry",
                    option);
                image_info->page=GetPageGeometry(argv[i]);
              }
            break;
          }
        if (LocaleCompare("pointsize",option+1) == 0)
          {
            image_info->pointsize=12;
            montage_info->pointsize=12;
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  ThrowMontageException(OptionError,"MissingPointsize",option);
                image_info->pointsize=atof(argv[i]);
                montage_info->pointsize=atof(argv[i]);
              }
            break;
          }
        ThrowMontageException(OptionError,"UnrecognizedOption",option)
      }
      case 'q':
      {
        if (LocaleCompare("quality",option+1) == 0)
          {
            image_info->quality=DefaultCompressionQuality;
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !IsGeometry(argv[i]))
                  ThrowMontageException(OptionError,"MissingCompressionQuality",
                    option);
                image_info->quality=atol(argv[i]);
              }
            break;
          }
        ThrowMontageException(OptionError,"UnrecognizedOption",option)
      }
      case 'r':
      {
        if (LocaleCompare("red-primary",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  ThrowMontageException(OptionError,"MissingPoint",option);
              }
            break;
          }
        if (LocaleCompare("render",option+1) == 0)
          break;
        if (LocaleCompare("resize",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !IsGeometry(argv[i]))
                  ThrowMontageException(OptionError,"MissingGeometry",option);
              }
            break;
          }
        if (LocaleCompare("rotate",option+1) == 0)
          {
            i++;
            if ((i == argc) || !IsGeometry(argv[i]))
              ThrowMontageException(OptionError,"MissingImageRotation",option);
            break;
          }
        ThrowMontageException(OptionError,"UnrecognizedOption",option)
      }
      case 's':
      {
        if (LocaleCompare("sampling-factor",option+1) == 0)
          {
            (void) CloneString(&image_info->sampling_factor,(char *) NULL);
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !IsGeometry(argv[i]))
                  ThrowMontageException(OptionError,"MissingGeometry",option);
                (void) CloneString(&image_info->sampling_factor,argv[i]);
              }
            break;
          }
        if (LocaleCompare("scenes",option+1) == 0)
          {
            first_scene=0;
            last_scene=0;
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  ThrowMontageException(OptionError,"MissingImageScene",option);
                first_scene=atol(argv[i]);
                last_scene=first_scene;
                (void) sscanf(argv[i],"%ld-%ld",&first_scene,&last_scene);
              }
            break;
          }
        if (LocaleCompare("shadow",option+1) == 0)
          {
            montage_info->shadow=(*option == '-');
            break;
          }
        if (LocaleCompare("sharpen",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  ThrowMontageException(OptionError,"MissingGeometry",option);
              }
            break;
          }
        if (LocaleCompare("size",option+1) == 0)
          {
            (void) CloneString(&image_info->size,(char *) NULL);
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !IsGeometry(argv[i]))
                  ThrowMontageException(OptionError,"MissingGeometry",option);
                (void) CloneString(&image_info->size,argv[i]);
              }
            break;
          }
        if (LocaleCompare("stroke",option+1) == 0)
          {
            (void) QueryColorDatabase("none",&montage_info->stroke,exception);
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowMontageException(OptionError,"MissingColors",option);
                (void) QueryColorDatabase(argv[i],&montage_info->stroke,
                  exception);
              }
            break;
          }
        if (LocaleCompare("strokewidth",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  ThrowMontageException(OptionError,"MissingPointsize",option);
              }
            break;
          }
        ThrowMontageException(OptionError,"UnrecognizedOption",option)
      }
      case 't':
      {
        if (LocaleCompare("texture",option+1) == 0)
          {
            (void) CloneString(&montage_info->texture,(char *) NULL);
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowMontageException(OptionError,"MissingFilename",option);
                (void) CloneString(&montage_info->texture,argv[i]);
              }
            break;
          }
        if (LocaleCompare("tile",option+1) == 0)
          {
            (void) CloneString(&montage_info->tile,(char *) NULL);
            (void) strcpy(argv[i]+1,"sans");
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !IsGeometry(argv[i]))
                  ThrowMontageException(OptionError,"MissingGeometry",option);
                (void) CloneString(&montage_info->tile,argv[i]);
              }
            break;
          }
        if (LocaleCompare("title",option+1) == 0)
          {
            (void) CloneString(&montage_info->title,(char *) NULL);
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowMontageException(OptionError,"MissingTitle",option);
                (void) CloneString(&montage_info->title,argv[i]);
              }
            break;
          }
        if (LocaleCompare("transform",option+1) == 0)
          break;
        if (LocaleCompare("transparent",option+1) == 0)
          {
            transparent_color=(char *) NULL;
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowMontageException(OptionError,"MissingColors",option);
                (void) CloneString(&transparent_color,argv[i]);
              }
            break;
          }
        if (LocaleCompare("treedepth",option+1) == 0)
          {
            quantize_info.tree_depth=0;
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  ThrowMontageException(OptionError,"MissingImageDepth",option);
                quantize_info.tree_depth=atoi(argv[i]);
              }
            break;
          }
        if (LocaleCompare("trim",option+1) == 0)
          break;
        if (LocaleCompare("type",option+1) == 0)
          {
            image_info->type=UndefinedType;
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  ThrowMontageException(OptionError,"MissingType",option);
                option=argv[i];
                if (LocaleCompare("Bilevel",option) == 0)
                  image_info->type=BilevelType;
                if (LocaleCompare("Grayscale",option) == 0)
                  image_info->type=GrayscaleType;
                if (LocaleCompare("GrayscaleMatte",option) == 0)
                  image_info->type=GrayscaleMatteType;
                if (LocaleCompare("Palette",option) == 0)
                  image_info->type=PaletteType;
                if (LocaleCompare("PaletteMatte",option) == 0)
                  image_info->type=PaletteMatteType;
                if (LocaleCompare("TrueColor",option) == 0)
                  image_info->type=TrueColorType;
                if (LocaleCompare("TrueColorMatte",option) == 0)
                  image_info->type=TrueColorMatteType;
                if (LocaleCompare("ColorSeparation",option) == 0)
                  image_info->type=ColorSeparationType;
                if (LocaleCompare("ColorSeparationMatte",option) == 0)
                  image_info->type=ColorSeparationMatteType;
                if (LocaleCompare("Optimize",option) == 0)
                  image_info->type=OptimizeType;
                if (image_info->type == UndefinedType)
                  ThrowMontageException(OptionError,"UnrecognizedImageType",
                    option);
              }
            break;
          }
        ThrowMontageException(OptionError,"UnrecognizedOption",option)
      }
      case 'v':
      {
        if (LocaleCompare("verbose",option+1) == 0)
          {
            image_info->verbose=(*option == '-');
            break;
          }
        if (LocaleCompare("verbose",option+1) == 0)
          break;
        if (LocaleCompare("virtual-pixel",option+1) == 0)
          {
            if (*option == '-')
              {
                VirtualPixelMethod
                  virtual_pixel_method;

                i++;
                if (i == argc)
                  ThrowMontageException(OptionError,"MissingVirtualPixelMethod",
                    option);
                option=argv[i];
                virtual_pixel_method=UndefinedVirtualPixelMethod;
                if (LocaleCompare("Constant",option) == 0)
                  virtual_pixel_method=ConstantVirtualPixelMethod;
                if (LocaleCompare("Edge",option) == 0)
                  virtual_pixel_method=EdgeVirtualPixelMethod;
                if (LocaleCompare("Mirror",option) == 0)
                  virtual_pixel_method=MirrorVirtualPixelMethod;
                if (LocaleCompare("Tile",option) == 0)
                  virtual_pixel_method=TileVirtualPixelMethod;
                if (virtual_pixel_method == UndefinedVirtualPixelMethod)
                  ThrowMontageException(OptionError,
                    "UnrecognizedVirtualPixelMethod",option);
              }
            break;
          }
        ThrowMontageException(OptionError,"UnrecognizedOption",option)
      }
      case 'w':
      {
        if (LocaleCompare("white-point",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  ThrowMontageException(OptionError,"MissingPoint",option);
              }
            break;
          }
        ThrowMontageException(OptionError,"UnrecognizedOption",option)
      }
      case '?':
        break;
      default:
        ThrowMontageException(OptionError,"UnrecognizedOption",option)
    }
  }
  if ((image == (Image *) NULL) && (image_list == (Image *) NULL))
    {
      if (exception->severity == UndefinedException)
        ThrowMontageException(OptionError,"MissingAnImageFilename",
          (char *) NULL);
      return(False);
    }
  if (i != (argc-1))
    ThrowMontageException(OptionError,"MissingAnImageFilename",(char *) NULL);
  if (image != (Image *) NULL)
    {
      status&=MogrifyImages(image_info,i-j,argv+j,&image);
      (void) CatchImageException(image);
      AppendImageToList(&image_list,image);
      image=NewImageList();
      j=i;
    }
  (void) strncpy(montage_info->filename,argv[argc-1],MaxTextExtent-1);
  montage_image=MontageImages(image_list,montage_info,exception);
  if (montage_image == (Image *) NULL)
    ThrowMontageException(OptionError,"MissingAnImageFilename",(char *) NULL);
  DestroyImageList(image_list);
  /*
    Write image.
  */
  status&=MogrifyImages(image_info,i-j,argv+j,&montage_image);
  (void) CatchImageException(montage_image);
  (void) strncpy(image_info->filename,argv[argc-1],MaxTextExtent-1);
  (void) strncpy(montage_image->magick_filename,argv[argc-1],MaxTextExtent-1);
  status&=WriteImages(image_info,montage_image,argv[argc-1],exception);
  if (metadata != (char **) NULL)
    {
      char
        *text;

      /*
	Return metadata to user
      */
      text=TranslateText(image_info,montage_image,(format != (char *) NULL) ? format : "%w,%h,%m");
      if (text == (char *) NULL)
        ThrowMontageException(ResourceLimitError,"MemoryAllocationFailed",
          "UnableToFormatImageMetadata");
      (void) ConcatenateString(&(*metadata),text);
      (void) ConcatenateString(&(*metadata),"\n");
      LiberateMemory((void **) &text);
    }
  DestroyImageList(montage_image);
  DestroyMontageInfo(montage_info);
  LiberateArgumentList(argc,argv);
  return(status);
}
#undef ThrowMontageException

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   A n i m a t e U s a g e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AnimateUsage() displays the program command syntax.
%
%  The format of the AnimateUsage method is:
%
%      void AnimateUsage()
%
%  A description of each parameter follows:
%
%
*/
MagickExport void AnimateUsage(void)
{
  const char
    **p;

  static const char
    *buttons[]=
    {
      "Press any button to map or unmap the Command widget",
      (char *) NULL
    },
    *options[]=
    {
      "-authenticate value  decrypt image with this password",
      "-backdrop            display image centered on a backdrop",
      "-colormap type       Shared or Private",
      "-colors value        preferred number of colors in the image",
      "-colorspace type     alternate image colorspace",
      "-crop geometry       preferred size and location of the cropped image",
      "-debug events        display copious debugging information",
      "-delay value         display the next image after pausing",
      "-density geometry    horizontal and vertical density of the image",
      "-depth value         image depth",
      "-display server      display image to this X server",
      "-dither              apply Floyd/Steinberg error diffusion to image",
      "-gamma value         level of gamma correction",
      "-geometry geometry   preferred size and location of the Image window",
      "-help                print program options",
      "-interlace type      None, Line, Plane, or Partition",
      "-limit type value    Disk, Map, or Memory resource limit",
      "-log format          format of debugging information",
      "-matte               store matte channel if the image has one",
      "-map type            display image using this Standard Colormap",
      "-monochrome          transform image to black and white",
      "-noop                do not apply options to image",
      "-pause               seconds to pause before reanimating",
      "-remote command      execute a command in an remote display process",
      "-rotate degrees      apply Paeth rotation to the image",
      "-sampling-factor geometry",
      "                     horizontal and vertical sampling factor",
      "-scenes range        image scene range",
      "-size geometry       width and height of image",
      "-treedepth value     color tree depth",
      "-trim                trim image edges",
      "-verbose             print detailed information about the image",
      "-version             print version information",
      "-visual type         display image using this visual type",
      "-virtual-pixel method",
      "                     Constant, Edge, Mirror, or Tile",
      "-window id           display image to background of this window",
      (char *) NULL
    };

  (void) printf("Version: %.1024s\n",GetMagickVersion((unsigned long *) NULL));
  (void) printf("Copyright: %.1024s\n\n",GetMagickCopyright());
  (void) printf("Usage: %.1024s [options ...] file [ [options ...] file ...]\n",
    SetClientName((char *) NULL));
  (void) printf("\nWhere options include: \n");
  for (p=options; *p != (char *) NULL; p++)
    (void) printf("  %.1024s\n",*p);
  (void) printf(
    "\nIn addition to those listed above, you can specify these standard X\n");
  (void) printf(
    "resources as command line options:  -background, -bordercolor,\n");
  (void) printf(
    "-borderwidth, -font, -foreground, -iconGeometry, -iconic, -name,\n");
  (void) printf("-mattecolor, -shared-memory, or -title.\n");
  (void) printf(
    "\nBy default, the image format of `file' is determined by its magic\n");
  (void) printf(
    "number.  To specify a particular image format, precede the filename\n");
  (void) printf(
    "with an image format name and a colon (i.e. ps:image) or specify the\n");
  (void) printf(
    "image type as the filename suffix (i.e. image.ps).  Specify 'file' as\n");
  (void) printf("'-' for standard input or output.\n");
  (void) printf("\nButtons: \n");
  for (p=buttons; *p != (char *) NULL; p++)
    (void) printf("  %.1024s\n",*p);
}
MagickExport unsigned int AnimateImageCommand(int argc,char **argv)
{
#if defined(HasX11)
  char
    *client_name,
    *option,
    *resource_value,
    *server_name;

  Display
    *display;

  double
    sans;

  ExceptionInfo
    exception;

  Image
    *image,
    *image_list,
    *loaded_image,
    *next_image;

  ImageInfo
    *image_info;

  long
    first_scene,
    j,
    k,
    last_scene,
    scene,
    x;

  QuantizeInfo
    *quantize_info;

  register Image
    *p;

  register long
    i;

  unsigned int
    status;

  XResourceInfo
    resource_info;

  XrmDatabase
    resource_database;

  /*
    Initialize command line arguments.
  */
  InitializeMagick(*argv);

  /*
    Set defaults.
  */
  SetNotifyHandlers;
  display=(Display *) NULL;
  GetExceptionInfo(&exception);
  first_scene=0;
  image=(Image *) NULL;
  image_list=(Image *) NULL;
  last_scene=0;
  server_name=(char *) NULL;
  status=True;
  /*
    Check for server name specified on the command line.
  */
  for (i=1; i < argc; i++)
  {
    /*
      Check command line for server name.
    */
    option=argv[i];
    if ((strlen(option) == 1) || ((*option != '-') && (*option != '+')))
      continue;
    if (LocaleCompare("display",option+1) == 0)
      {
        /*
          User specified server name.
        */
        i++;
        if (i == argc)
          MagickFatalError(OptionFatalError,"MissingServerName",option);
        server_name=argv[i];
        break;
      }
    if (LocaleCompare("help",option+1) == 0)
      {
        AnimateUsage();
        return False;
      }
    if (LocaleCompare("version",option+1) == 0)
      {
        (void) fprintf(stdout,"Version: %.1024s\n",
          GetMagickVersion((unsigned long *) NULL));
        (void) fprintf(stdout,"Copyright: %.1024s\n\n",GetMagickCopyright());
        return False;
      }
  }

  /*
    Expand argument list
  */
  status=ExpandFilenames(&argc,&argv);
  if (status == False)
    MagickFatalError(ResourceLimitFatalError,"MemoryAllocationFailed",
      (char *) NULL);

  /*
    Get user defaults from X resource database.
  */
  display=XOpenDisplay(server_name);
  if (display == (Display *) NULL)
    MagickFatalError(XServerFatalError,"UnableToOpenXServer",
      XDisplayName(server_name));
 (void) XSetErrorHandler(XError);
  client_name=SetClientName((char *) NULL);
  resource_database=XGetResourceDatabase(display,client_name);
  XGetResourceInfo(resource_database,client_name,&resource_info);
  image_info=resource_info.image_info;
  quantize_info=resource_info.quantize_info;
  image_info->density=
    XGetResourceInstance(resource_database,client_name,"density",(char *) NULL);
  if (image_info->density == (char *) NULL)
    image_info->density=XGetScreenDensity(display);
  resource_value=
    XGetResourceInstance(resource_database,client_name,"interlace","none");
  image_info->interlace=UndefinedInterlace;
  if (LocaleCompare("None",resource_value) == 0)
    image_info->interlace=NoInterlace;
  if (LocaleCompare("Line",resource_value) == 0)
    image_info->interlace=LineInterlace;
  if (LocaleCompare("Plane",resource_value) == 0)
    image_info->interlace=PlaneInterlace;
  if (LocaleCompare("Partition",resource_value) == 0)
    image_info->interlace=PartitionInterlace;
  if (image_info->interlace == UndefinedInterlace)
    MagickError(OptionFatalError,"InvalidImageInterlace",resource_value);
  resource_value=
    XGetResourceInstance(resource_database,client_name,"verbose","False");
  image_info->verbose=IsTrue(resource_value);
  resource_value=
    XGetResourceInstance(resource_database,client_name,"dither","True");
  quantize_info->dither=IsTrue(resource_value);
  /*
    Parse command line.
  */
  j=1;
  k=0;
  for (i=1; i <= argc; i++)
  {
    if (i < argc)
      option=argv[i];
    else
      if (image != (Image *) NULL)
        break;
      else
        if (!isatty(STDIN_FILENO))
          option=(char *) "-";
        else
          option=(char *) "logo:Untitled";
    if ((strlen(option) == 1) || ((*option != '-') && (*option != '+')))
      {
        /*
          Option is a file name.
        */
        k=i;
        for (scene=first_scene; scene <= last_scene ; scene++)
        {
          /*
            Read image.
          */
          (void) strncpy(image_info->filename,option,MaxTextExtent-1);
          if (first_scene != last_scene)
            {
              char
                filename[MaxTextExtent];

              /*
                Form filename for multi-part images.
              */
              FormatString(filename,image_info->filename,scene);
              if (LocaleCompare(filename,image_info->filename) == 0)
                FormatString(filename,"%.1024s[%lu]",image_info->filename,
                  scene);
              (void) strncpy(image_info->filename,filename,MaxTextExtent-1);
            }
          image_info->colorspace=quantize_info->colorspace;
          image_info->dither=quantize_info->dither;
          next_image=ReadImage(image_info,&exception);
          if (exception.severity != UndefinedException)
            CatchException(&exception);
          status&=next_image != (Image *) NULL;
          if (next_image == (Image *) NULL)
            continue;
          if (image == (Image *) NULL)
            {
              image=next_image;
              continue;
            }
          /*
            Link image into image list.
          */
          for (p=image; p->next != (Image *) NULL; p=p->next);
          next_image->previous=p;
          p->next=next_image;
        }
        continue;
      }
    if (j != (k+1))
      {
        status&=MogrifyImages(image_info,i-j,argv+j,&image);
        (void) CatchImageException(image);
        AppendImageToList(&image_list,image);
        image=(Image *) NULL;
        j=k+1;
      }
    switch (*(option+1))
    {
      case 'a':
      {
        if (LocaleCompare("authenticate",option+1) == 0)
          {
            (void) CloneString(&image_info->authenticate,(char *) NULL);
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  MagickFatalError(OptionFatalError,"MissingPassword",option);
                (void) CloneString(&image_info->authenticate,argv[i]);
              }
            break;
          }
        MagickFatalError(OptionFatalError,"UnrecognizedOption",option);
        break;
      }
      case 'b':
      {
        if (LocaleCompare("backdrop",option+1) == 0)
          {
            resource_info.backdrop=(*option == '-');
            break;
          }
        if (LocaleCompare("background",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  MagickFatalError(OptionFatalError,"MissingColor",option);
                resource_info.background_color=argv[i];
                (void) QueryColorDatabase(argv[i],&image_info->background_color,
                  &exception);
              }
            break;
          }
        if (LocaleCompare("bordercolor",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  MagickFatalError(OptionFatalError,"MissingColor",option);
                resource_info.border_color=argv[i];
                (void) QueryColorDatabase(argv[i],&image_info->border_color,
                  &exception);
              }
            break;
          }
        if (LocaleCompare("borderwidth",option+1) == 0)
          {
            resource_info.border_width=0;
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  MagickFatalError(OptionFatalError,"MissingWidth",option);
                resource_info.border_width=atoi(argv[i]);
              }
            break;
          }
        MagickFatalError(OptionFatalError,"UnrecognizedOption",option);
        break;
      }
      case 'c':
      {
        if (LocaleCompare("cache",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  MagickFatalError(OptionFatalError,"MissingThreshold",option);
                SetMagickResourceLimit(MemoryResource,atol(argv[i]));
                SetMagickResourceLimit(MapResource,2*atol(argv[i]));
              }
            break;
          }
        if (LocaleCompare("colormap",option+1) == 0)
          {
            resource_info.colormap=PrivateColormap;
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  MagickFatalError(OptionFatalError,"MissingType",option);
                option=argv[i];
                resource_info.colormap=UndefinedColormap;
                if (LocaleCompare("private",option) == 0)
                  resource_info.colormap=PrivateColormap;
                if (LocaleCompare("shared",option) == 0)
                  resource_info.colormap=SharedColormap;
                if (resource_info.colormap == UndefinedColormap)
                  MagickFatalError(OptionFatalError,"InvalidColormapType",
                    option);
              }
            break;
          }
        if (LocaleCompare("colors",option+1) == 0)
          {
            quantize_info->number_colors=0;
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  MagickFatalError(OptionFatalError,"MissingColors",option);
                quantize_info->number_colors=atol(argv[i]);
              }
            break;
          }
        if (LocaleCompare("colorspace",option+1) == 0)
          {
            quantize_info->colorspace=RGBColorspace;
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  MagickFatalError(OptionFatalError,"MissingType",option);
                option=argv[i];
                quantize_info->colorspace=UndefinedColorspace;
                if (LocaleCompare("cmyk",option) == 0)
                  quantize_info->colorspace=CMYKColorspace;
                if (LocaleCompare("gray",option) == 0)
                  {
                    quantize_info->colorspace=GRAYColorspace;
                    quantize_info->number_colors=256;
                    quantize_info->tree_depth=8;
                  }
                if (LocaleCompare("hsl",option) == 0)
                  quantize_info->colorspace=HSLColorspace;
                if (LocaleCompare("hwb",option) == 0)
                  quantize_info->colorspace=HWBColorspace;
                if (LocaleCompare("ohta",option) == 0)
                  quantize_info->colorspace=OHTAColorspace;
                if (LocaleCompare("rgb",option) == 0)
                  quantize_info->colorspace=RGBColorspace;
                if (LocaleCompare("srgb",option) == 0)
                  quantize_info->colorspace=sRGBColorspace;
                if (LocaleCompare("transparent",option) == 0)
                  quantize_info->colorspace=TransparentColorspace;
                if (LocaleCompare("xyz",option) == 0)
                  quantize_info->colorspace=XYZColorspace;
                if (LocaleCompare("ycbcr",option) == 0)
                  quantize_info->colorspace=YCbCrColorspace;
                if (LocaleCompare("ycc",option) == 0)
                  quantize_info->colorspace=YCCColorspace;
                if (LocaleCompare("yiq",option) == 0)
                  quantize_info->colorspace=YIQColorspace;
                if (LocaleCompare("ypbpr",option) == 0)
                  quantize_info->colorspace=YPbPrColorspace;
                if (LocaleCompare("yuv",option) == 0)
                  quantize_info->colorspace=YUVColorspace;
                if (quantize_info->colorspace == UndefinedColorspace)
                  MagickFatalError(OptionFatalError,"InvalidColorspaceType",
                    option);
              }
            break;
          }
        if (LocaleCompare("crop",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !IsGeometry(argv[i]))
                  MagickFatalError(OptionFatalError,"MissingGeometry",option);
              }
            break;
          }
        MagickFatalError(OptionFatalError,"UnrecognizedOption",option);
        break;
      }
      case 'd':
      {
        if (LocaleCompare("debug",option+1) == 0)
          {
            (void) SetLogEventMask("None");
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  MagickFatalError(OptionFatalError,"MissingEventMask",
                    option);
                (void) SetLogEventMask(argv[i]);
              }
            break;
          }
        if (LocaleCompare("delay",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  MagickFatalError(OptionFatalError,"MissingSeconds",option);
              }
            break;
          }
        if (LocaleCompare("density",option+1) == 0)
          {
            (void) CloneString(&image_info->density,(char *) NULL);
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !IsGeometry(argv[i]))
                  MagickFatalError(OptionFatalError,"MissingGeometry",option);
                (void) CloneString(&image_info->density,argv[i]);
              }
            break;
          }
        if (LocaleCompare("depth",option+1) == 0)
          {
            image_info->depth=QuantumDepth;
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  MagickFatalError(OptionFatalError,"MissingImageDepth",option);
                image_info->depth=atol(argv[i]);
              }
            break;
          }
        if (LocaleCompare("display",option+1) == 0)
          {
            (void) CloneString(&image_info->server_name,(char *) NULL);
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  MagickFatalError(OptionFatalError,"MissingServerName",option);
                (void) CloneString(&image_info->server_name,argv[i]);
              }
            break;
          }
        if (LocaleCompare("dither",option+1) == 0)
          {
            quantize_info->dither=(*option == '-');
            break;
          }
        MagickFatalError(OptionFatalError,"UnrecognizedOption",option);
        break;
      }
      case 'f':
      {
        if (LocaleCompare("font",option+1) == 0)
          {
            (void) CloneString(&image_info->font,(char *) NULL);
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  MagickFatalError(OptionFatalError,"MissingFontName",option);
                (void) CloneString(&image_info->font,argv[i]);
              }
            if ((image_info->font == (char *) NULL) ||
                (*image_info->font != '@'))
              resource_info.font=AllocateString(image_info->font);
            break;
          }
        if (LocaleCompare("foreground",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  MagickFatalError(OptionFatalError,"MissingForeground",
                    option);
                resource_info.foreground_color=argv[i];
              }
            break;
          }
        MagickFatalError(OptionFatalError,"UnrecognizedOption",option);
        break;
      }
      case 'g':
      {
        if (LocaleCompare("gamma",option+1) == 0)
          {
            i++;
            if ((i == argc) || !sscanf(argv[i],"%lf",&sans))
              MagickFatalError(OptionFatalError,"Missing value",option);
            break;
          }
        if (LocaleCompare("geometry",option+1) == 0)
          {
            resource_info.image_geometry=(char *) NULL;
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !IsGeometry(argv[i]))
                  MagickFatalError(OptionFatalError,"MissingGeometry",option);
                resource_info.image_geometry=argv[i];
              }
            break;
          }
        MagickFatalError(OptionFatalError,"UnrecognizedOption",option);
        break;
      }
      case 'h':
      {
        if (LocaleCompare("help",option+1) == 0)
          {
            AnimateUsage();
            break;
          }
        MagickFatalError(OptionFatalError,"UnrecognizedOption",option);
        break;
      }
      case 'i':
      {
        if (LocaleCompare("iconGeometry",option+1) == 0)
          {
            resource_info.icon_geometry=(char *) NULL;
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !IsGeometry(argv[i]))
                  MagickFatalError(OptionFatalError,"MissingGeometry",option);
                resource_info.icon_geometry=argv[i];
              }
            break;
          }
        if (LocaleCompare("iconic",option+1) == 0)
          {
            resource_info.iconic=(*option == '-');
            break;
          }
        if (LocaleCompare("interlace",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  MagickFatalError(OptionFatalError,"MissingType",option);
                option=argv[i];
                image_info->interlace=UndefinedInterlace;
                if (LocaleCompare("None",option) == 0)
                  image_info->interlace=NoInterlace;
                if (LocaleCompare("Line",option) == 0)
                  image_info->interlace=LineInterlace;
                if (LocaleCompare("Plane",option) == 0)
                  image_info->interlace=PlaneInterlace;
                if (LocaleCompare("Partition",option) == 0)
                  image_info->interlace=PartitionInterlace;
                if (image_info->interlace == UndefinedInterlace)
                  MagickFatalError(OptionFatalError,"InvalidInterlaceType",
                    option);
              }
            break;
          }
        MagickFatalError(OptionFatalError,"UnrecognizedOption",option);
        break;
      }
      case 'l':
      {
        if (LocaleCompare("limit",option+1) == 0)
          {
            if (*option == '-')
              {
                char
                  *type;

                i++;
                if (i == argc)
                  MagickFatalError(OptionFatalError,"MissingResourceType",
                    option);
                type=argv[i];
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  MagickFatalError(OptionFatalError,"MissingResourceLimit",
                    option);
                if (LocaleCompare("disk",type) == 0)
                  SetMagickResourceLimit(DiskResource,atol(argv[i]));
                else
                  if (LocaleCompare("map",type) == 0)
                    SetMagickResourceLimit(MapResource,atol(argv[i]));
                  else
                    if (LocaleCompare("memory",type) == 0)
                      SetMagickResourceLimit(MemoryResource,atol(argv[i]));
                    else
                      MagickFatalError(OptionFatalError,
                        "UnrecognizedResourceType",type);
              }
            break;
          }
        if (LocaleCompare("log",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  MagickFatalError(OptionFatalError,"MissingLogFormat",option);
                (void) SetLogFormat(argv[i]);
              }
            break;
          }
        MagickFatalError(OptionFatalError,"UnrecognizedOption",option);
        break;
      }
      case 'm':
      {
        if (LocaleCompare("map",option+1) == 0)
          {
            (void) strcpy(argv[i]+1,"sans");
            resource_info.map_type=(char *) NULL;
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  MagickFatalError(OptionFatalError,"MissingMapType",option);
                resource_info.map_type=argv[i];
              }
            break;
          }
        if (LocaleCompare("matte",option+1) == 0)
          break;
        if (LocaleCompare("mattecolor",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  MagickFatalError(OptionFatalError,"MissingColor",option);
                resource_info.matte_color=argv[i];
                (void) QueryColorDatabase(argv[i],&image_info->matte_color,
                  &exception);
              }
            break;
          }
        if (LocaleCompare("monochrome",option+1) == 0)
          {
            image_info->monochrome=(*option == '-');
            if (image_info->monochrome)
              {
                quantize_info->number_colors=2;
                quantize_info->tree_depth=8;
                quantize_info->colorspace=GRAYColorspace;
              }
            break;
          }
        MagickFatalError(OptionFatalError,"UnrecognizedOption",option);
        break;
      }
      case 'n':
      {
        if (LocaleCompare("name",option+1) == 0)
          {
            resource_info.name=(char *) NULL;
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  MagickFatalError(OptionFatalError,"MissingName",option);
                resource_info.name=argv[i];
              }
            break;
          }
        if (LocaleCompare("noop",option+1) == 0)
          break;
        MagickFatalError(OptionFatalError,"UnrecognizedOption",option);
        break;
      }
      case 'p':
      {
        if (LocaleCompare("pause",option+1) == 0)
          {
            resource_info.pause=0;
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  MagickFatalError(OptionFatalError,"MissingSeconds",option);
              }
            resource_info.pause=atoi(argv[i]);
            break;
          }
        break;
      }
      case 'r':
      {
        if (LocaleCompare("remote",option+1) == 0)
          {
            i++;
            if (i == argc)
              MagickFatalError(OptionFatalError,"MissingCommand",option);
            status=XRemoteCommand(display,resource_info.window_id,argv[i]);
            Exit(!status);
          }
        if (LocaleCompare("rotate",option+1) == 0)
          {
            i++;
            if ((i == argc) || !IsGeometry(argv[i]))
              MagickFatalError(OptionFatalError,"MissingDegrees",option);
            break;
          }
        MagickFatalError(OptionFatalError,"UnrecognizedOption",option);
        break;
      }
      case 's':
      {
        if (LocaleCompare("sampling-factor",option+1) == 0)
          {
            (void) CloneString(&image_info->sampling_factor,(char *) NULL);
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !IsGeometry(argv[i]))
                  MagickFatalError(OptionFatalError,"MissingGeometry",option);
                (void) CloneString(&image_info->sampling_factor,argv[i]);
              }
            break;
          }
        if (LocaleCompare("scenes",option+1) == 0)
          {
            first_scene=0;
            last_scene=0;
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  MagickFatalError(OptionFatalError,"MissingSceneNumber",
                    option);
                first_scene=atol(argv[i]);
                last_scene=first_scene;
                (void) sscanf(argv[i],"%ld-%ld",&first_scene,&last_scene);
              }
            break;
          }
        if (LocaleCompare("shared-memory",option+1) == 0)
          {
            resource_info.use_shared_memory=(*option == '-');
            break;
          }
        if (LocaleCompare("size",option+1) == 0)
          {
            (void) CloneString(&image_info->size,(char *) NULL);
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !IsGeometry(argv[i]))
                  MagickFatalError(OptionFatalError,"MissingGeometry",option);
                (void) CloneString(&image_info->size,argv[i]);
              }
            break;
          }
        MagickFatalError(OptionFatalError,"UnrecognizedOption",option);
        break;
      }
      case 't':
      {
        if (LocaleCompare("text-font",option+1) == 0)
          {
            resource_info.text_font=(char *) NULL;
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  MagickFatalError(OptionFatalError,"MissingFontName",option);
                resource_info.text_font=argv[i];
              }
            break;
          }
        if (LocaleCompare("title",option+1) == 0)
          {
            resource_info.title=(char *) NULL;
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  MagickFatalError(OptionFatalError,"MissingTitle",option);
                resource_info.title=argv[i];
              }
            break;
          }
        if (LocaleCompare("treedepth",option+1) == 0)
          {
            quantize_info->tree_depth=0;
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  MagickFatalError(OptionFatalError,"MissingDepth",option);
                quantize_info->tree_depth=atoi(argv[i]);
              }
            break;
          }
        if (LocaleCompare("trim",option+1) == 0)
          break;
        MagickFatalError(OptionFatalError,"UnrecognizedOption",option);
        break;
      }
      case 'v':
      {
        if (LocaleCompare("verbose",option+1) == 0)
          {
            image_info->verbose=(*option == '-');
            break;
          }
        if (LocaleCompare("version",option+1) == 0)
          break;
        if (LocaleCompare("virtual-pixel",option+1) == 0)
          {
            if (*option == '-')
              {
                VirtualPixelMethod
                  virtual_pixel_method;

                i++;
                if (i == argc)
                  MagickFatalError(OptionFatalError,"MissingVirtualPixelMethod",
                    option);
                option=argv[i];
                virtual_pixel_method=UndefinedVirtualPixelMethod;
                if (LocaleCompare("Constant",option) == 0)
                  virtual_pixel_method=ConstantVirtualPixelMethod;
                if (LocaleCompare("Edge",option) == 0)
                  virtual_pixel_method=EdgeVirtualPixelMethod;
                if (LocaleCompare("Mirror",option) == 0)
                  virtual_pixel_method=MirrorVirtualPixelMethod;
                if (LocaleCompare("Tile",option) == 0)
                  virtual_pixel_method=TileVirtualPixelMethod;
                if (virtual_pixel_method == UndefinedVirtualPixelMethod)
                  MagickFatalError(OptionFatalError,
                    "UnrecognizedVirtualPixelMethod",option);
              }
            break;
          }
        if (LocaleCompare("visual",option+1) == 0)
          {
            resource_info.visual_type=(char *) NULL;
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  MagickFatalError(OptionFatalError,"MissingVisualClass",
                    option);
                resource_info.visual_type=argv[i];
              }
            break;
          }
        MagickFatalError(OptionFatalError,"UnrecognizedOption",option);
        break;
      }
      case 'w':
      {
        if (LocaleCompare("window",option+1) == 0)
          {
            resource_info.window_id=(char *) NULL;
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  MagickFatalError(OptionFatalError,"MissingIDNameOrRoot",
                    option);
                resource_info.window_id=argv[i];
              }
            break;
          }
        MagickFatalError(OptionFatalError,"UnrecognizedOption",option);
        break;
      }
      case '?':
      {
        AnimateUsage();
        break;
      }
      default:
      {
        MagickFatalError(OptionFatalError,"UnrecognizedOption",option);
        break;
      }
    }
  }
  i--;
  if ((image == (Image *) NULL) && (image_list == (Image *) NULL))
    MagickFatalError(OptionFatalError,"MissingAnImageFilename",(char *) NULL);
  if (image == (Image *) NULL)
    {
      status&=MogrifyImages(image_info,i-j,argv+j,&image_list);
      (void) CatchImageException(image_list);
    }
  else
    {
      status&=MogrifyImages(image_info,i-j,argv+j,&image);
      (void) CatchImageException(image);
      AppendImageToList(&image_list,image);
    }
  if (resource_info.window_id != (char *) NULL)
    XAnimateBackgroundImage(display,&resource_info,image_list);
  else
    {
      /*
        Animate image to X server.
      */
      loaded_image=XAnimateImages(display,&resource_info,argv,argc,image_list);
      while (loaded_image != (Image *) NULL)
      {
        image_list=loaded_image;
        loaded_image=
          XAnimateImages(display,&resource_info,argv,argc,image_list);
      }
    }
  DestroyImageList(image_list);
  LiberateArgumentList(argc,argv);
  return(status);
#else
  MagickFatalError(MissingDelegateError,"XWindowLibraryIsNotAvailable",
    (char *) NULL);
  return(False);
#endif
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I m p o r t U s a g e                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ImportUsage() displays the program command syntax.
%
%  The format of the ImportUsage method is:
%
%      void ImportUsage()
%
%
*/
MagickExport void ImportUsage(void)
{
  const char
    **p;

  static const char
    *options[]=
    {
      "-adjoin              join images into a single multi-image file",
      "-border              include image borders in the output image",
      "-colors value        preferred number of colors in the image",
      "-colorspace type     alternate image colorspace",
      "-comment string      annotate image with comment",
      "-compress type       image compression type",
      "-crop geometry       preferred size and location of the cropped image",
      "-debug events        display copious debugging information",
      "-delay value         display the next image after pausing",
      "-density geometry    horizontal and vertical density of the image",
      "-depth value         image depth",
      "-descend             obtain image by descending window hierarchy",
      "-display server      X server to contact",
      "-dispose method      Undefined, None, Background, Previous",
      "-dither              apply Floyd/Steinberg error diffusion to image",
      "-frame               include window manager frame",
      "-encoding type       text encoding type",
      "-endian type         LSB or MSB",
      "-geometry geometry   perferred size or location of the image",
      "-interlace type      None, Line, Plane, or Partition",
      "-help                print program options",
      "-label name          assign a label to an image",
      "-limit type value    Disk, Map, or Memory resource limit",
      "-log format          format of debugging information",
      "-monochrome          transform image to black and white",
      "-negate              replace every pixel with its complementary color ",
      "-page geometry       size and location of an image canvas",
      "-pause value         seconds delay between snapshots",
      "-pointsize value     font point size",
      "-quality value       JPEG/MIFF/PNG compression level",
      "-resize geometry     resize the image",
      "-rotate degrees      apply Paeth rotation to the image",
      "-sampling-factor geometry",
      "                     horizontal and vertical sampling factor",
      "-scene value         image scene number",
      "-screen              select image from root window",
      "-silent              operate silently, i.e. don't ring any bells ",
      "-snaps value         number of screen snapshots",
      "-transparent color   make this color transparent within the image",
      "-treedepth value     color tree depth",
      "-trim                trim image edges",
      "-type type           image type",
      "-verbose             print detailed information about the image",
      "-version             print version information",
      "-virtual-pixel method",
      "                     Constant, Edge, Mirror, or Tile",
      "-window id           select window with this id or name",
      (char *) NULL
    };

  (void) printf("Version: %.1024s\n",GetMagickVersion((unsigned long *) NULL));
  (void) printf("Copyright: %.1024s\n\n",GetMagickCopyright());
  (void) printf("Usage: %.1024s [options ...] [ file ]\n",
    SetClientName((char *) NULL));
  (void) printf("\nWhere options include:\n");
  for (p=options; *p != (char *) NULL; p++)
    (void) printf("  %.1024s\n",*p);
  (void) printf(
  "\nBy default, 'file' is written in the MIFF image format.  To\n");
  (void) printf(
    "specify a particular image format, precede the filename with an image\n");
  (void) printf(
    "format name and a colon (i.e. ps:image) or specify the image type as\n");
  (void) printf(
    "the filename suffix (i.e. image.ps).  Specify 'file' as '-' for\n");
  (void) printf("standard input or output.\n");
}

MagickExport unsigned int ImportImageCommand(int argc,char **argv)
{
#if defined(HasX11)
  char
    *client_name,
    *filename,
    *option,
    *resource_value,
    *server_name,
    *target_window;

  Display
    *display;

  ExceptionInfo
    exception;

  Image
    *image,
    *next_image;

  ImageInfo
    *image_info;

  long
    snapshots,
    x;

  QuantizeInfo
    *quantize_info;

  register long
    i;

  unsigned int
    status;

  XImportInfo
    ximage_info;

  XResourceInfo
    resource_info;

  XrmDatabase
    resource_database;

  /*
    Check for server name specified on the command line.
  */
  server_name=(char *) NULL;
  for (i=1; i < argc; i++)
  {
    /*
      Check command line for server name.
    */
    option=argv[i];
    if ((strlen(option) == 1) || ((*option != '-') && (*option != '+')))
      continue;
    if (LocaleCompare("display",option+1) == 0)
      {
        /*
          User specified server name.
        */
        i++;
        if (i == argc)
          MagickFatalError(OptionFatalError,"MissingServerName",option);
        server_name=argv[i];
        break;
      }
    if (LocaleCompare("help",option+1) == 0 ||
        LocaleCompare("?", option+1) == 0)
      {
        ImportUsage();
        return False;
      }
    if (LocaleCompare("version",option+1) == 0)
      {
        (void) fprintf(stdout,"Version: %.1024s\n",
          GetMagickVersion((unsigned long *) NULL));
        (void) fprintf(stdout,"Copyright: %.1024s\n\n",GetMagickCopyright());
        return False;
      }
  }

  /*
    Expand argument list
  */
  status=ExpandFilenames(&argc,&argv);
  if (status == False)
    MagickFatalError(ResourceLimitFatalError,"MemoryAllocationFailed",
      (char *) NULL);

  /*
    Get user defaults from X resource database.
  */
  SetNotifyHandlers;
  display=XOpenDisplay(server_name);
  if (display == (Display *) NULL)
    MagickFatalError(OptionFatalError,"UnableToOpenXServer",
      XDisplayName(server_name));
  (void) XSetErrorHandler(XError);
  client_name=SetClientName((char *) NULL);
  resource_database=XGetResourceDatabase(display,client_name);
  XGetImportInfo(&ximage_info);
  XGetResourceInfo(resource_database,client_name,&resource_info);
  image_info=resource_info.image_info;
  quantize_info=resource_info.quantize_info;
  resource_value=
    XGetResourceInstance(resource_database,client_name,"border","False");
  ximage_info.borders=IsTrue(resource_value);
  resource_value=
    XGetResourceInstance(resource_database,client_name,"delay","0");
  resource_info.delay=atol(resource_value);
  image_info->density=XGetResourceInstance(resource_database,client_name,
    "density",(char *) NULL);
  resource_value=
    XGetResourceInstance(resource_database,client_name,"descend","True");
  ximage_info.descend=IsTrue(resource_value);
  resource_value=
    XGetResourceInstance(resource_database,client_name,"frame","False");
  ximage_info.frame=IsTrue(resource_value);
  resource_value=
    XGetResourceInstance(resource_database,client_name,"interlace","none");
  image_info->interlace=UndefinedInterlace;
  if (LocaleCompare("None",resource_value) == 0)
    image_info->interlace=NoInterlace;
  if (LocaleCompare("Line",resource_value) == 0)
    image_info->interlace=LineInterlace;
  if (LocaleCompare("Plane",resource_value) == 0)
    image_info->interlace=PlaneInterlace;
  if (LocaleCompare("Partition",resource_value) == 0)
    image_info->interlace=PartitionInterlace;
  if (image_info->interlace == UndefinedInterlace)
    MagickError(OptionError,"Unrecognized interlace type",resource_value);
  image_info->page=XGetResourceInstance(resource_database,client_name,
    "pageGeometry",(char *) NULL);
  resource_value=
    XGetResourceInstance(resource_database,client_name,"pause","0");
  resource_info.pause=atol(resource_value);
  resource_value=
    XGetResourceInstance(resource_database,client_name,"quality","85");
  image_info->quality=atol(resource_value);
  resource_value=
    XGetResourceInstance(resource_database,client_name,"screen","False");
  ximage_info.screen=IsTrue(resource_value);
  resource_value=
    XGetResourceInstance(resource_database,client_name,"silent","False");
  ximage_info.silent=IsTrue(resource_value);
  resource_value=
    XGetResourceInstance(resource_database,client_name,"verbose","False");
  image_info->verbose=IsTrue(resource_value);
  resource_value=
    XGetResourceInstance(resource_database,client_name,"dither","True");
  quantize_info->dither=IsTrue(resource_value);
  snapshots=1;
  status=True;
  filename=(char *) NULL;
  target_window=(char *) NULL;
  /*
    Check command syntax.
  */
  GetExceptionInfo(&exception);
  for (i=1; i < argc; i++)
  {
    option=argv[i];
    if ((strlen(option) == 1) || ((*option != '-') && (*option != '+')))
      {
        filename=argv[i];
        continue;
      }
    switch(*(option+1))
    {
      case 'a':
      {
        if (LocaleCompare("adjoin",option+1) == 0)
          {
            image_info->adjoin=(*option == '-');
            break;
          }
        MagickFatalError(OptionFatalError,"UnrecognizedOption",option);
        break;
      }
      case 'b':
      {
        if (LocaleCompare("border",option+1) == 0)
          {
            ximage_info.borders=(*option == '-');
            break;
          }
        if (LocaleCompare("bordercolor",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  MagickFatalError(OptionFatalError,"MissingBorderColor",
                    option);
                (void) QueryColorDatabase(argv[i],&image_info->border_color,
                  &exception);
              }
            break;
          }
        MagickFatalError(OptionFatalError,"UnrecognizedOption",option);
        break;
      }
      case 'c':
      {
        if (LocaleCompare("cache",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  MagickFatalError(OptionFatalError,"MissingThreshold",option);
                SetMagickResourceLimit(MemoryResource,atol(argv[i]));
                SetMagickResourceLimit(MapResource,2*atol(argv[i]));
              }
            break;
          }
        if (LocaleCompare("colors",option+1) == 0)
          {
            quantize_info->number_colors=0;
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  MagickFatalError(OptionFatalError,"MissingColors",option);
                quantize_info->number_colors=atol(argv[i]);
              }
            break;
          }
        if (LocaleCompare("colorspace",option+1) == 0)
          {
            quantize_info->colorspace=RGBColorspace;
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  MagickFatalError(OptionFatalError,"MissingType",option);
                option=argv[i];
                quantize_info->colorspace=UndefinedColorspace;
                if (LocaleCompare("cmyk",option) == 0)
                  quantize_info->colorspace=CMYKColorspace;
                if (LocaleCompare("gray",option) == 0)
                  {
                    quantize_info->colorspace=GRAYColorspace;
                    quantize_info->number_colors=256;
                    quantize_info->tree_depth=8;
                  }
                if (LocaleCompare("hsl",option) == 0)
                  quantize_info->colorspace=HSLColorspace;
                if (LocaleCompare("hwb",option) == 0)
                  quantize_info->colorspace=HWBColorspace;
                if (LocaleCompare("ohta",option) == 0)
                  quantize_info->colorspace=OHTAColorspace;
                if (LocaleCompare("rgb",option) == 0)
                  quantize_info->colorspace=RGBColorspace;
                if (LocaleCompare("srgb",option) == 0)
                  quantize_info->colorspace=sRGBColorspace;
                if (LocaleCompare("transparent",option) == 0)
                  quantize_info->colorspace=TransparentColorspace;
                if (LocaleCompare("xyz",option) == 0)
                  quantize_info->colorspace=XYZColorspace;
                if (LocaleCompare("ycbcr",option) == 0)
                  quantize_info->colorspace=YCbCrColorspace;
                if (LocaleCompare("ycc",option) == 0)
                  quantize_info->colorspace=YCCColorspace;
                if (LocaleCompare("yiq",option) == 0)
                  quantize_info->colorspace=YIQColorspace;
                if (LocaleCompare("ypbpr",option) == 0)
                  quantize_info->colorspace=YPbPrColorspace;
                if (LocaleCompare("yuv",option) == 0)
                  quantize_info->colorspace=YUVColorspace;
                if (quantize_info->colorspace == UndefinedColorspace)
                  MagickFatalError(OptionFatalError,"InvalidColorspaceType",
                    option);
              }
            break;
          }
        if (LocaleCompare("comment",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  MagickFatalError(OptionFatalError,"Missing comment",option);
              }
            break;
          }
        if (LocaleCompare("compress",option+1) == 0)
          {
            image_info->compression=NoCompression;
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  MagickFatalError(OptionFatalError,"MissingType",option);
                option=argv[i];
                image_info->compression=UndefinedCompression;
                if (LocaleCompare("None",option) == 0)
                  image_info->compression=NoCompression;
                if (LocaleCompare("BZip",option) == 0)
                  image_info->compression=BZipCompression;
                if (LocaleCompare("Fax",option) == 0)
                  image_info->compression=FaxCompression;
                if (LocaleCompare("Group4",option) == 0)
                  image_info->compression=Group4Compression;
                if (LocaleCompare("JPEG",option) == 0)
                  image_info->compression=JPEGCompression;
                if (LocaleCompare("Lossless",option) == 0)
                  image_info->compression=LosslessJPEGCompression;
                if (LocaleCompare("LZW",option) == 0)
                  image_info->compression=LZWCompression;
                if (LocaleCompare("RLE",option) == 0)
                  image_info->compression=RLECompression;
                if (LocaleCompare("Zip",option) == 0)
                  image_info->compression=ZipCompression;
                if (image_info->compression == UndefinedCompression)
                  MagickFatalError(OptionFatalError,
                    "UnrecognizedImageCompressionType",option);
              }
            break;
          }
        if (LocaleCompare("crop",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !IsGeometry(argv[i]))
                  MagickFatalError(OptionFatalError,"MissingGeometry",option);
              }
            break;
          }
        MagickFatalError(OptionFatalError,"UnrecognizedOption",option);
        break;
      }
      case 'd':
      {
        if (LocaleCompare("debug",option+1) == 0)
          {
            (void) SetLogEventMask("None");
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  MagickFatalError(OptionFatalError,"MissingEventMask",option);
                (void) SetLogEventMask(argv[i]);
              }
            break;
          }
        if (LocaleCompare("delay",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  MagickFatalError(OptionFatalError,"MissingSeconds",option);
              }
            break;
          }
        if (LocaleCompare("density",option+1) == 0)
          {
            (void) CloneString(&image_info->density,(char *) NULL);
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !IsGeometry(argv[i]))
                  MagickFatalError(OptionFatalError,"MissingGeometry",option);
                (void) CloneString(&image_info->density,argv[i]);
              }
            break;
          }
        if (LocaleCompare("depth",option+1) == 0)
          {
            image_info->depth=QuantumDepth;
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  MagickFatalError(OptionFatalError,"MissingImageDepth",option);
                image_info->depth=atol(argv[i]);
              }
            break;
          }
        if (LocaleCompare("descend",option+1) == 0)
          {
            ximage_info.descend=(*option == '-');
            break;
          }
        if (LocaleCompare("display",option+1) == 0)
          {
            (void) CloneString(&image_info->server_name,(char *) NULL);
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  MagickFatalError(OptionFatalError,"MissingServerName",
                    option);
                (void) CloneString(&image_info->server_name,argv[i]);
              }
            break;
          }
        if (LocaleCompare("dispose",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  MagickFatalError(OptionFatalError,"MissingDisposeMethod",
                    option);
                option=argv[i];
                if ((LocaleCompare("0",option) != 0) &&
                    (LocaleCompare("1",option) != 0) &&
                    (LocaleCompare("2",option) != 0) &&
                    (LocaleCompare("3",option) != 0) &&
                    (LocaleCompare("Undefined",option) != 0) &&
                    (LocaleCompare("None",option) != 0) &&
                    (LocaleCompare("Background",option) != 0) &&
                    (LocaleCompare("Previous",option) != 0))
                  MagickFatalError(OptionFatalError,"UnrecognizedDisposeMethod",
                    option);
              }
            break;
          }
        if (LocaleCompare("dither",option+1) == 0)
          {
            quantize_info->dither=(*option == '-');
            break;
          }
        MagickFatalError(OptionFatalError,"UnrecognizedOption",option);
        break;
      }
      case 'e':
      {
        if (LocaleCompare("encoding",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  MagickFatalError(OptionFatalError,"MissingEncodingType",
                    option);
              }
            break;
          }
        if (LocaleCompare("endian",option+1) == 0)
          {
            image_info->endian=UndefinedEndian;
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  MagickFatalError(OptionFatalError,"MissingType",option);
                option=argv[i];
                image_info->endian=UndefinedEndian;
                if (LocaleCompare("LSB",option) == 0)
                  image_info->endian=LSBEndian;
                if (LocaleCompare("MSB",option) == 0)
                  image_info->endian=MSBEndian;
                if (image_info->endian == UndefinedEndian)
                  MagickFatalError(OptionFatalError,"InvalidEndianType",option);
              }
            break;
          }
        MagickFatalError(OptionFatalError,"UnrecognizedOption",option);
        break;
      }
      case 'f':
      {
        if (LocaleCompare("frame",option+1) == 0)
          {
            argv[i]=(char *) "-ignore";  /* resolve option confict */
            ximage_info.frame=(*option == '-');
            break;
          }
        MagickFatalError(OptionFatalError,"UnrecognizedOption",option);
        break;
      }
      case 'g':
      {
        if (LocaleCompare("geometry",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !IsGeometry(argv[i]))
                  MagickFatalError(OptionFatalError,"MissingGeometry",option);
              }
            break;
          }
        MagickFatalError(OptionFatalError,"UnrecognizedOption",option);
        break;
      }
      case 'h':
      {
        if (LocaleCompare("help",option+1) == 0)
          break;
        MagickFatalError(OptionFatalError,"UnrecognizedOption",option);
        break;
      }
      case 'i':
      {
        if (LocaleCompare("interlace",option+1) == 0)
          {
            image_info->interlace=NoInterlace;
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  MagickFatalError(OptionFatalError,"MissingType",option);
                option=argv[i];
                image_info->interlace=UndefinedInterlace;
                if (LocaleCompare("No",option) == 0)
                  image_info->interlace=NoInterlace;
                if (LocaleCompare("Line",option) == 0)
                  image_info->interlace=LineInterlace;
                if (LocaleCompare("Plane",option) == 0)
                  image_info->interlace=PlaneInterlace;
                if (LocaleCompare("Partition",option) == 0)
                  image_info->interlace=PartitionInterlace;
                if (image_info->interlace == UndefinedInterlace)
                  MagickFatalError(OptionFatalError,"InvalidInterlaceType",
                    option);
              }
            break;
          }
        MagickFatalError(OptionFatalError,"UnrecognizedOption",option);
        break;
      }
      case 'l':
      {
        if (LocaleCompare("label",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  MagickFatalError(OptionFatalError,"MissingLabelName",option);
              }
            break;
          }
        if (LocaleCompare("limit",option+1) == 0)
          {
            if (*option == '-')
              {
                char
                  *type;

                i++;
                if (i == argc)
                  MagickFatalError(OptionFatalError,"MissingResourceType",
                    option);
                type=argv[i];
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  MagickFatalError(OptionFatalError,"MissingResourceLimit",
                    option);
                if (LocaleCompare("disk",type) == 0)
                  SetMagickResourceLimit(DiskResource,atol(argv[i]));
                else
                  if (LocaleCompare("map",type) == 0)
                    SetMagickResourceLimit(MapResource,atol(argv[i]));
                  else
                    if (LocaleCompare("memory",type) == 0)
                      SetMagickResourceLimit(MemoryResource,atol(argv[i]));
                    else
                      MagickFatalError(OptionFatalError,
                        "UnrecognizedResourceType",type);
              }
            break;
          }
        if (LocaleCompare("log",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  MagickFatalError(OptionFatalError,"MissingLogFormat",option);
                (void) SetLogFormat(argv[i]);
              }
            break;
          }
        MagickFatalError(OptionFatalError,"UnrecognizedOption",option);
        break;
      }
      case 'm':
      {
        if (LocaleCompare("monochrome",option+1) == 0)
          {
            image_info->monochrome=(*option == '-');
            if (image_info->monochrome)
              {
                quantize_info->number_colors=2;
                quantize_info->tree_depth=8;
                quantize_info->colorspace=GRAYColorspace;
              }
            break;
          }
        MagickFatalError(OptionFatalError,"UnrecognizedOption",option);
        break;
      }
      case 'n':
      {
        if (LocaleCompare("negate",option+1) == 0)
          break;
        MagickFatalError(OptionFatalError,"UnrecognizedOption",option);
        break;
      }
      case 'p':
      {
        if (LocaleCompare("page",option+1) == 0)
          {
            (void) CloneString(&image_info->page,(char *) NULL);
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  MagickFatalError(OptionFatalError,"MissingPageGeometry",
                    option);
                image_info->page=GetPageGeometry(argv[i]);
              }
            break;
          }
        if (LocaleCompare("pause",option+1) == 0)
          {
            resource_info.pause=0;
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  MagickFatalError(OptionFatalError,"MissingSeconds",option);
                resource_info.pause=atoi(argv[i]);
              }
            break;
          }
        if (LocaleCompare("ping",option+1) == 0)
          {
            MagickError(OptionFatalError,"Deprecated option",option);
            break;
          }
        if (LocaleCompare("pointsize",option+1) == 0)
          {
            image_info->pointsize=12;
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  MagickFatalError(OptionFatalError,"MissingSize",option);
                image_info->pointsize=atof(argv[i]);
              }
            break;
          }
        MagickFatalError(OptionFatalError,"UnrecognizedOption",option);
        break;
      }
      case 'q':
      {
        if (LocaleCompare("quality",option+1) == 0)
          {
            image_info->quality=DefaultCompressionQuality;
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !IsGeometry(argv[i]))
                  MagickFatalError(OptionFatalError,"MissingCompressionQuality",
                    option);
                image_info->quality=atol(argv[i]);
              }
            break;
          }
        MagickFatalError(OptionFatalError,"UnrecognizedOption",option);
        break;
      }
      case 'r':
      {
        if (LocaleCompare("resize",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !IsGeometry(argv[i]))
                  MagickFatalError(OptionFatalError,"MissingGeometry",option);
              }
            break;
          }
        if (LocaleCompare("rotate",option+1) == 0)
          {
            i++;
            if ((i == argc) || !IsGeometry(argv[i]))
               MagickFatalError(OptionFatalError,"MissingDegrees",option);
            break;
          }
        MagickFatalError(OptionFatalError,"UnrecognizedOption",option);
        break;
      }
      case 's':
      {
        if (LocaleCompare("sampling-factor",option+1) == 0)
          {
            (void) CloneString(&image_info->sampling_factor,(char *) NULL);
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !IsGeometry(argv[i]))
                  MagickFatalError(OptionFatalError,"MissingGeometry",option);
                (void) CloneString(&image_info->sampling_factor,argv[i]);
              }
            break;
          }
        if (LocaleCompare("scene",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  MagickFatalError(OptionFatalError,"MissingSceneNumber",
                    option);
              }
            break;
          }
        if (LocaleCompare("screen",option+1) == 0)
          {
            ximage_info.screen=(*option == '-');
            break;
          }
        if (LocaleCompare("silent",option+1) == 0)
          {
            ximage_info.silent=(*option == '-');
            break;
          }
        if (LocaleCompare("size",option+1) == 0)
          {
            (void) CloneString(&image_info->size,(char *) NULL);
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !IsGeometry(argv[i]))
                  MagickFatalError(OptionFatalError,"MissingGeometry",option);
                (void) CloneString(&image_info->size,argv[i]);
              }
            break;
          }
        if (LocaleCompare("snaps",option+1) == 0)
          {
            (void) strcpy(argv[i]+1,"sans");
            i++;
            if ((i == argc) || !sscanf(argv[i],"%ld",&x))
              MagickFatalError(OptionFatalError,"MissingSnaps",option);
            snapshots=atol(argv[i]);
            break;
          }
        MagickFatalError(OptionFatalError,"UnrecognizedOption",option);
        break;
      }
      case 't':
      {
        if (LocaleCompare("transparent",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  MagickFatalError(OptionFatalError,"MissingColor",option);
              }
            break;
          }
        if (LocaleCompare("treedepth",option+1) == 0)
          {
            quantize_info->tree_depth=0;
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  MagickFatalError(OptionFatalError,"MissingDepth",option);
                quantize_info->tree_depth=atoi(argv[i]);
              }
            break;
          }
        if (LocaleCompare("trim",option+1) == 0)
          break;
        if (LocaleCompare("type",option+1) == 0)
          {
            image_info->type=UndefinedType;
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  MagickFatalError(OptionFatalError,"MissingType",option);
                option=argv[i];
                if (LocaleCompare("Bilevel",option) == 0)
                  image_info->type=BilevelType;
                if (LocaleCompare("Grayscale",option) == 0)
                  image_info->type=GrayscaleType;
                if (LocaleCompare("GrayscaleMatte",option) == 0)
                  image_info->type=GrayscaleMatteType;
                if (LocaleCompare("Palette",option) == 0)
                  image_info->type=PaletteType;
                if (LocaleCompare("PaletteMatte",option) == 0)
                  image_info->type=PaletteMatteType;
                if (LocaleCompare("TrueColor",option) == 0)
                  image_info->type=TrueColorType;
                if (LocaleCompare("TrueColorMatte",option) == 0)
                  image_info->type=TrueColorMatteType;
                if (LocaleCompare("ColorSeparation",option) == 0)
                  image_info->type=ColorSeparationType;
                if (LocaleCompare("ColorSeparationMatte",option) == 0)
                  image_info->type=ColorSeparationMatteType;
                if (LocaleCompare("Optimize",option) == 0)
                  image_info->type=OptimizeType;
                if (image_info->type == UndefinedType)
                  MagickFatalError(OptionFatalError,"InvalidImageType",option);
              }
            break;
          }
        MagickFatalError(OptionFatalError,"UnrecognizedOption",option);
        break;
      }
      case 'v':
      {
        if (LocaleCompare("verbose",option+1) == 0)
          {
            image_info->verbose=(*option == '-');
            break;
          }
        if (LocaleCompare("version",option+1) == 0)
          break;
        MagickFatalError(OptionFatalError,"UnrecognizedOption",option);
        break;
      }
      case 'w':
      {
        i++;
        if (i == argc)
          MagickFatalError(OptionFatalError,"MissingIDNameOrRoot",option);
        (void) CloneString(&target_window,argv[i]);
        break;
      }
      case '?':
      {
        break;
      }
      default:
      {
        MagickFatalError(OptionFatalError,"UnrecognizedOption",option);
        break;
      }
    }
  }
  DestroyExceptionInfo(&exception);
  if (filename == (char *) NULL)
    filename=(char *) "magick.miff";
  /*
    Read image from X server.
  */
  if (target_window != (char *) NULL)
    (void) strncpy(image_info->filename,target_window,MaxTextExtent-1);
  image_info->colorspace=quantize_info->colorspace;
  image_info->dither=quantize_info->dither;
  image=(Image *) NULL;
  for (i=0; i < (long) Max(snapshots,1); i++)
  {
    (void) sleep(resource_info.pause);
    next_image=XImportImage(image_info,&ximage_info);
    status&=next_image != (Image *) NULL;
    if (next_image == (Image *) NULL)
      continue;
    (void) strncpy(next_image->filename,filename,MaxTextExtent-1);
    (void) strcpy(next_image->magick,"PS");
    next_image->scene=i;
    next_image->previous=image;
    if (image != (Image *) NULL)
      image->next=next_image;
    image=next_image;
  }
  if (image == (Image *) NULL)
    MagickFatalError(OptionFatalError,"MissingAnImageFilename",(char *) NULL);
  while (image->previous != (Image *) NULL)
    image=image->previous;
  status&=MogrifyImages(image_info,argc-1,argv,&image);
  (void) CatchImageException(image);
  status&=WriteImages(image_info,image,filename,&image->exception);
  DestroyImageList(image);
  DestroyMagick();
  LiberateArgumentList(argc,argv);
  return(!status);
#else
  MagickFatalError(MissingDelegateError,"XWindowLibraryIsNotAvailable",
    (char *) NULL);
  return(False);
#endif
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   C o n j u r e U s a g e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ConjureUsage() displays the program command syntax.
%
%  The format of the ConjureUsage method is:
%
%      void ConjureUsage()
%
*/
MagickExport void ConjureUsage(void)
{
  static const char
    *options[]=
    {
      "-debug events        display copious debugging information",
      "-help                print program options",
      "-log format          format of debugging information",
      "-verbose             print detailed information about the image",
      "-version             print version information",
      (char *) NULL
    };

  const char
    **p;

  (void) printf("Version: %.1024s\n",GetMagickVersion((unsigned long *) NULL));
  (void) printf("Copyright: %.1024s\n\n",GetMagickCopyright());
  (void) printf("Usage: %.1024s [options ...] file [ [options ...] file ...]\n",
    SetClientName((char *) NULL));
  (void) printf("\nWhere options include:\n");
  for (p=options; *p != (char *) NULL; p++)
    (void) printf("  %.1024s\n",*p);
  (void) printf("\nIn additiion, define any key value pairs required by "
    "your script.  For\nexample,\n\n");
  (void) printf("    conjure -size 100x100 -color blue -foo bar script.msl\n");
}

MagickExport unsigned int ConjureImageCommand(int argc,char **argv)
{
  char
    *option;

  ExceptionInfo
    exception;

  Image
    *image;

  ImageInfo
    *image_info;

  register long
    i;

  unsigned int
    status;

  InitializeMagick(*argv);
  ReadCommandlLine(argc,&argv);

  /*
    Expand argument list
  */
  status=ExpandFilenames(&argc,&argv);
  if (status == False)
    MagickFatalError(ResourceLimitFatalError,"MemoryAllocationFailed",
      (char *) NULL);

  /*
    Validate command list
  */
  if (argc < 2)
    {
      ConjureUsage();
      LiberateArgumentList(argc,argv);
      return (False);
    }

  GetExceptionInfo(&exception);
  image_info=CloneImageInfo((ImageInfo *) NULL);
  image_info->attributes=AllocateImage(image_info);
  status=True;
  for (i=1; i < argc; i++)
  {
    option=argv[i];
    if ((strlen(option) != 1) && ((*option == '-') || (*option == '+')))
      {
        if (LocaleCompare("debug",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  MagickFatalError(OptionFatalError,"MissingEventMask",option);
                (void) SetLogEventMask(argv[i]);
              }
            continue;
          }
        if (LocaleCompare("help",option+1) == 0 ||
            LocaleCompare("?",option+1) == 0)
          {
            if (*option == '-')
              ConjureUsage();
            continue;
          }
        if (LocaleCompare("log",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  MagickFatalError(OptionFatalError,"MissingLogFormat",option);
                (void) SetLogFormat(argv[i]);
              }
            continue;
          }
        if (LocaleCompare("verbose",option+1) == 0)
          {
            image_info->verbose=(*option == '-');
            continue;
          }
        if (LocaleCompare("version",option+1) == 0)
          {
            (void) fprintf(stdout,"Version: %.1024s\n",
              GetMagickVersion((unsigned long *) NULL));
            (void) fprintf(stdout,"Copyright: %.1024s\n\n",
              GetMagickCopyright());
            Exit(0);
            continue;
          }
        /*
          Persist key/value pair.
        */
        (void) SetImageAttribute(image_info->attributes,option+1,(char *) NULL);
        status&=SetImageAttribute(image_info->attributes,option+1,argv[i+1]);
        if (status == False)
          MagickFatalError(ImageFatalError,"UnableToPersistKey",option);
        i++;
        continue;
      }
    /*
      Interpret MSL script.
    */
    (void) SetImageAttribute(image_info->attributes,"filename",(char *) NULL);
    status&=SetImageAttribute(image_info->attributes,"filename",argv[i]);
    if (status == False)
      MagickFatalError(ImageFatalError,"UnableToPersistKey",argv[i]);
    (void) FormatString(image_info->filename,"msl:%.1024s",argv[i]);
    image=ReadImage(image_info,&exception);
    if (exception.severity != UndefinedException)
      CatchException(&exception);
    status&=image != (Image *) NULL;
    if (image != (Image *) NULL)
      DestroyImageList(image);
  }
  DestroyImageInfo(image_info);
  LiberateArgumentList(argc,argv);
  return(!status);
}
/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D i s p l a y U s a g e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DisplayUsage() displays the program command syntax.
%
%  The format of the DisplayUsage method is:
%
%      void DisplayUsage(void)
%
%  A description of each parameter follows:
%
%
*/
MagickExport void DisplayUsage(void)
{
  const char
    **p;

  static const char
    *buttons[]=
    {
      "1    press to map or unmap the Command widget",
      "2    press and drag to magnify a region of an image",
      "3    press to load an image from a visual image directory",
      (char *) NULL
    },
    *options[]=
    {
      "-authenticate value  decrypt image with this password",
      "-backdrop            display image centered on a backdrop",
      "-border geometry     surround image with a border of color",
      "-colormap type       Shared or Private",
      "-colors value        preferred number of colors in the image",
      "-colorspace type     alternate image colorspace",
      "-comment string      annotate image with comment",
      "-compress type       image compression tyhpe",
      "-contrast            enhance or reduce the image contrast",
      "-crop geometry       preferred size and location of the cropped image",
      "-debug events        display copious debugging information",
      "-delay value         display the next image after pausing",
      "-density geometry    horizontal and vertical density of the image",
      "-depth value         image depth",
      "-despeckle           reduce the speckles within an image",
      "-display server      display image to this X server",
      "-dispose method      Undefined, None, Background, Previous",
      "-dither              apply Floyd/Steinberg error diffusion to image",
      "-edge factor         apply a filter to detect edges in the image",
      "-endian type         LSB or MSB",
      "-enhance             apply a digital filter to enhance a noisy image",
      "-filter type         use this filter when resizing an image",
      "-flip                flip image in the vertical direction",
      "-flop                flop image in the horizontal direction",
      "-frame geometry      surround image with an ornamental border",
      "-gamma value         level of gamma correction",
      "-geometry geometry   preferred size and location of the Image window",
      "-help                print program options",
      "-immutable           displayed image cannot be modified",
      "-interlace type      None, Line, Plane, or Partition",
      "-label name          assign a label to an image",
      "-limit type value    Disk, Map, or Memory resource limit",
      "-log format          format of debugging information",
      "-map type            display image using this Standard Colormap",
      "-matte               store matte channel if the image has one",
      "-monochrome          transform image to black and white",
      "-negate              replace every pixel with its complementary color",
      "-noop                do not apply options to image",
      "-page geometry       size and location of an image canvas",
      "-quality value       JPEG/MIFF/PNG compression level",
      "-raise value         lighten/darken image edges to create a 3-D effect",
      "-remote command      execute a command in an remote display process",
      "-roll geometry       roll an image vertically or horizontally",
      "-rotate degrees      apply Paeth rotation to the image",
      "-sample geometry     scale image with pixel sampling",
      "-sampling-factor geometry",
      "                     horizontal and vertical sampling factor",
      "-scenes range        image scene range",
      "-segment value       segment an image",
      "-sharpen geometry    sharpen the image",
      "-size geometry       width and height of image",
      "-texture filename    name of texture to tile onto the image background",
      "-treedepth value     color tree depth",
      "-trim                trim image edges",
      "-update seconds      detect when image file is modified and redisplay",
      "-verbose             print detailed information about the image",
      "-version             print version information",
      "-visual type         display image using this visual type",
      "-virtual-pixel method",
      "                     Constant, Edge, Mirror, or Tile",
      "-window id           display image to background of this window",
      "-window_group id     exit program when this window id is destroyed",
      "-write filename      write image to a file",
      (char *) NULL
    };

  (void) printf("Version: %.1024s\n",GetMagickVersion((unsigned long *) NULL));
  (void) printf("Copyright: %.1024s\n\n",GetMagickCopyright());
  (void) printf("Usage: %.1024s [options ...] file [ [options ...] file ...]\n",
    SetClientName((char *) NULL));
  (void) printf("\nWhere options include: \n");
  for (p=options; *p != (char *) NULL; p++)
    (void) printf("  %.1024s\n",*p);
  (void) printf(
    "\nIn addition to those listed above, you can specify these standard X\n");
  (void) printf(
    "resources as command line options:  -background, -bordercolor,\n");
  (void) printf(
    "-borderwidth, -font, -foreground, -iconGeometry, -iconic, -mattecolor,\n");
  (void) printf("-name, -shared-memory, -usePixmap, or -title.\n");
  (void) printf(
    "\nBy default, the image format of `file' is determined by its magic\n");
  (void) printf(
    "number.  To specify a particular image format, precede the filename\n");
  (void) printf(
    "with an image format name and a colon (i.e. ps:image) or specify the\n");
  (void) printf(
    "image type as the filename suffix (i.e. image.ps).  Specify 'file' as\n");
  (void) printf("'-' for standard input or output.\n");
  (void) printf("\nButtons: \n");
  for (p=buttons; *p != (char *) NULL; p++)
    (void) printf("  %.1024s\n",*p);
}

MagickExport unsigned int DisplayImageCommand(int argc,char **argv)
{
#if defined(HasX11)
  char
    *client_name,
    *option,
    *resource_value,
    *server_name;

  Display
    *display;

  double
    sans;

  ExceptionInfo
    exception;

  Image
    *image,
    *next;

  ImageInfo
    *image_info;

  long
    first_scene,
    image_number,
    j,
    k,
    last_scene,
    scene,
    x;

  QuantizeInfo
    *quantize_info;

  register long
    i;

  unsigned int
    *image_marker,
    last_image,
    status;

  unsigned long
    state;

  XResourceInfo
    resource_info;

  XrmDatabase
    resource_database;

  /*
    Expand Argument List
  */
  status=ExpandFilenames(&argc,&argv);
  if (status == False)
    MagickFatalError(ResourceLimitFatalError,"MemoryAllocationFailed",
      (char *) NULL);

  /*
    Set defaults.
  */
  SetNotifyHandlers;
  display=(Display *) NULL;
  GetExceptionInfo(&exception);
  first_scene=0;
  image_number=0;
  last_image=0;
  last_scene=0;
  image_marker=(unsigned int *) AcquireMemory((argc+1)*sizeof(unsigned int));
  if (image_marker == (unsigned int *) NULL)
    MagickFatalError(ResourceLimitFatalError,"MemoryAllocationFailed",
      "UnableToDisplayImage");
  for (i=0; i <= argc; i++)
    image_marker[i]=argc;
  resource_database=(XrmDatabase) NULL;
  server_name=(char *) NULL;
  state=0;
  /*
    Check for server name specified on the command line.
  */
  for (i=1; i < argc; i++)
  {
    /*
      Check command line for server name.
    */
    option=argv[i];
    if ((strlen(option) == 1) || ((*option != '-') && (*option != '+')))
      continue;
    if (LocaleCompare("display",option+1) == 0)
      {
        /*
          User specified server name.
        */
        i++;
        if (i == argc)
          MagickFatalError(OptionFatalError,"MissingServerName",option);
        server_name=argv[i];
        break;
      }
    if (LocaleCompare("help",option+1) == 0)
      {
        DisplayUsage();
        LiberateArgumentList(argc,argv);
        return False;
      }
    if (LocaleCompare("version",option+1) == 0)
      {
        (void) fprintf(stdout,"Version: %.1024s\n",
          GetMagickVersion((unsigned long *) NULL));
        (void) fprintf(stdout,"Copyright: %.1024s\n\n",GetMagickCopyright());
        LiberateArgumentList(argc,argv);
        return False;
      }
  }
  /*
    Get user defaults from X resource database.
  */
  display=XOpenDisplay(server_name);
  if (display == (Display *) NULL)
    MagickFatalError(XServerFatalError,"UnableToOpenXServer",
      XDisplayName(server_name));
  (void) XSetErrorHandler(XError);
  client_name=SetClientName((char *) NULL);
  resource_database=XGetResourceDatabase(display,client_name);
  XGetResourceInfo(resource_database,client_name,&resource_info);
  image_info=resource_info.image_info;
  quantize_info=resource_info.quantize_info;
  image_info->density=
    XGetResourceInstance(resource_database,client_name,"density",(char *) NULL);
  if (image_info->density == (char *) NULL)
    image_info->density=XGetScreenDensity(display);
  resource_value=
    XGetResourceInstance(resource_database,client_name,"interlace","none");
  image_info->interlace=UndefinedInterlace;
  if (LocaleCompare("None",resource_value) == 0)
    image_info->interlace=NoInterlace;
  if (LocaleCompare("Line",resource_value) == 0)
    image_info->interlace=LineInterlace;
  if (LocaleCompare("Plane",resource_value) == 0)
    image_info->interlace=PlaneInterlace;
  if (LocaleCompare("Partition",resource_value) == 0)
    image_info->interlace=PartitionInterlace;
  if (image_info->interlace == UndefinedInterlace)
    MagickError(OptionError,"Unrecognized interlace type",resource_value);
  image_info->page=XGetResourceInstance(resource_database,client_name,
    "pageGeometry",(char *) NULL);
  resource_value=
    XGetResourceInstance(resource_database,client_name,"quality","75");
  image_info->quality=atol(resource_value);
  resource_value=
    XGetResourceInstance(resource_database,client_name,"verbose","False");
  image_info->verbose=IsTrue(resource_value);
  resource_value=
    XGetResourceInstance(resource_database,client_name,"dither","True");
  quantize_info->dither=IsTrue(resource_value);
  /*
    Parse command line.
  */
  status=True;
  j=1;
  k=0;
  for (i=1; ((i <= argc) && !(state & ExitState)); i++)
  {
    if (i < argc)
      option=argv[i];
    else
      if (image_number != 0)
        break;
      else
        if (!isatty(STDIN_FILENO))
          option=(char *) "-";
        else
          option=(char *) "logo:Untitled";
    if ((strlen(option) == 1) || ((*option != '-') && (*option != '+')))
      {
        /*
          Option is a file name.
        */
        k=i;
        for (scene=first_scene; scene <= last_scene ; scene++)
        {
          /*
            Read image.
          */
          (void) strncpy(image_info->filename,option,MaxTextExtent-1);
          if (first_scene != last_scene)
            {
              char
                filename[MaxTextExtent];

              /*
                Form filename for multi-part images.
              */
              FormatString(filename,image_info->filename,scene);
              if (LocaleCompare(filename,image_info->filename) == 0)
                FormatString(filename,"%.1024s.%lu",image_info->filename,scene);
              (void) strncpy(image_info->filename,filename,MaxTextExtent-1);
            }
          (void) strcpy(image_info->magick,"MIFF");
          image_info->colorspace=quantize_info->colorspace;
          image_info->dither=quantize_info->dither;
          image=ReadImage(image_info,&exception);
          if (exception.severity != UndefinedException)
            CatchException(&exception);
          status&=image != (Image *) NULL;
          if (image == (Image *) NULL)
            continue;
          status&=MogrifyImage(image_info,i-j,argv+j,&image);
          (void) CatchImageException(image);
          do
          {
            /*
              Transmogrify image as defined by the image processing options.
            */
            resource_info.quantum=1;
            if (first_scene != last_scene)
              image->scene=scene;
            /*
              Display image to X server.
            */
            if (resource_info.window_id != (char *) NULL)
              {
                /*
                  Display image to a specified X window.
                */
                if (XDisplayBackgroundImage(display,&resource_info,image))
                  state|=RetainColorsState;
              }
            else
              do
              {
                Image
                  *nexus;

                /*
                  Display image to X server.
                */
                nexus=
                  XDisplayImage(display,&resource_info,argv,argc,&image,&state);
                if (nexus == (Image *) NULL)
                  break;
                while ((nexus != (Image *) NULL) && (!(state & ExitState)))
                {
                  if (nexus->montage != (char *) NULL)
                    {
                      /*
                        User selected a visual directory image (montage).
                      */
                      DestroyImageList(image);
                      image=nexus;
                      break;
                    }
                  if (first_scene != last_scene)
                    image->scene=scene;
                  next=XDisplayImage(display,&resource_info,argv,argc,&nexus,
                    &state);
                  if ((next == (Image *) NULL) &&
                      (nexus->next != (Image *) NULL))
                    {
                      DestroyImageList(image);
                      image=nexus->next;
                      nexus=(Image *) NULL;
                    }
                  else
                    {
                      if (nexus != image)
                        DestroyImageList(nexus);
                      nexus=next;
                    }
                }
              } while (!(state & ExitState));
            if (resource_info.write_filename != (char *) NULL)
              {
                /*
                  Write image.
                */
                (void) strncpy(image->filename,resource_info.write_filename,
                  MaxTextExtent-1);
                (void) SetImageInfo(image_info,True,&image->exception);
                status&=WriteImage(image_info,image);
                (void) CatchImageException(image);
              }
            if (image_info->verbose)
              DescribeImage(image,stderr,False);
            /*
              Proceed to next/previous image.
            */
            next=image;
            if (state & FormerImageState)
              for (k=0; k < resource_info.quantum; k++)
              {
                next=next->previous;
                if (next == (Image *) NULL)
                  break;
              }
            else
              for (k=0; k < resource_info.quantum; k++)
              {
                next=next->next;
                if (next == (Image *) NULL)
                  break;
              }
            if (next != (Image *) NULL)
              image=next;
          } while ((next != (Image *) NULL) && !(state & ExitState));
          /*
            Free image resources.
          */
          DestroyImageList(image);
          if (!(state & FormerImageState))
            {
              last_image=image_number;
              image_marker[i]=image_number++;
            }
          else
            {
              /*
                Proceed to previous image.
              */
              for (i--; i > 0; i--)
                if ((int) image_marker[i] == (image_number-2))
                  break;
              image_number--;
            }
          if (state & ExitState)
            break;
        }
        /*
          Determine if we should proceed to the first image.
        */
        if (image_number < 0)
          {
            if (state & FormerImageState)
              {
                for (i=1; i < (argc-2); i++)
                  if (image_marker[i] == last_image)
                    break;
                image_number=image_marker[i]+1;
              }
            continue;
          }
        continue;
      }
    j=k+1;
    switch (*(option+1))
    {
      case 'a':
      {
        if (LocaleCompare("authenticate",option+1) == 0)
          {
            (void) CloneString(&image_info->authenticate,(char *) NULL);
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  MagickFatalError(OptionError,"Missing password",option);
                (void) CloneString(&image_info->authenticate,argv[i]);
              }
            break;
          }
        MagickFatalError(OptionFatalError,"UnrecognizedOption",option);
        break;
      }
      case 'b':
      {
        if (LocaleCompare("backdrop",option+1) == 0)
          {
            resource_info.backdrop=(*option == '-');
            break;
          }
        if (LocaleCompare("background",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  MagickFatalError(OptionFatalError,"MissingColor",option);
                resource_info.background_color=argv[i];
                (void) QueryColorDatabase(argv[i],&image_info->background_color,
                  &exception);
              }
            break;
          }
        if (LocaleCompare("border",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !IsGeometry(argv[i]))
                  MagickFatalError(OptionFatalError,"MissingGeometry",option);
              }
            break;
          }
        if (LocaleCompare("bordercolor",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  MagickFatalError(OptionFatalError,"MissingColor",option);
                resource_info.border_color=argv[i];
                (void) QueryColorDatabase(argv[i],&image_info->border_color,
                  &exception);
              }
            break;
          }
        if (LocaleCompare("borderwidth",option+1) == 0)
          {
            resource_info.border_width=0;
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  MagickFatalError(OptionFatalError,"MissingWidth",option);
                resource_info.border_width=atoi(argv[i]);
              }
            break;
          }
        MagickFatalError(OptionFatalError,"UnrecognizedOption",option);
        break;
      }
      case 'c':
      {
        if (LocaleCompare("cache",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  MagickFatalError(OptionFatalError,"MissingThreshold",option);
                SetMagickResourceLimit(MemoryResource,atol(argv[i]));
                SetMagickResourceLimit(MapResource,2*atol(argv[i]));
              }
            break;
          }
        if (LocaleCompare("colormap",option+1) == 0)
          {
            resource_info.colormap=PrivateColormap;
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  MagickFatalError(OptionFatalError,"MissingType",option);
                option=argv[i];
                resource_info.colormap=UndefinedColormap;
                if (LocaleCompare("private",option) == 0)
                  resource_info.colormap=PrivateColormap;
                if (LocaleCompare("shared",option) == 0)
                  resource_info.colormap=SharedColormap;
                if (resource_info.colormap == UndefinedColormap)
                  MagickFatalError(OptionFatalError,"InvalidColormapType",
                    option);
              }
            break;
          }
        if (LocaleCompare("colors",option+1) == 0)
          {
            quantize_info->number_colors=0;
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  MagickFatalError(OptionFatalError,"MissingColors",option);
                quantize_info->number_colors=atol(argv[i]);
              }
            break;
          }
        if (LocaleCompare("colorspace",option+1) == 0)
          {
            quantize_info->colorspace=RGBColorspace;
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  MagickFatalError(OptionFatalError,"MissingType",option);
                option=argv[i];
                quantize_info->colorspace=UndefinedColorspace;
                if (LocaleCompare("cmyk",option) == 0)
                  quantize_info->colorspace=CMYKColorspace;
                if (LocaleCompare("gray",option) == 0)
                  {
                    quantize_info->colorspace=GRAYColorspace;
                    quantize_info->number_colors=256;
                    quantize_info->tree_depth=8;
                  }
                if (LocaleCompare("hsl",option) == 0)
                  quantize_info->colorspace=HSLColorspace;
                if (LocaleCompare("hwb",option) == 0)
                  quantize_info->colorspace=HWBColorspace;
                if (LocaleCompare("ohta",option) == 0)
                  quantize_info->colorspace=OHTAColorspace;
                if (LocaleCompare("rgb",option) == 0)
                  quantize_info->colorspace=RGBColorspace;
                if (LocaleCompare("srgb",option) == 0)
                  quantize_info->colorspace=sRGBColorspace;
                if (LocaleCompare("transparent",option) == 0)
                  quantize_info->colorspace=TransparentColorspace;
                if (LocaleCompare("xyz",option) == 0)
                  quantize_info->colorspace=XYZColorspace;
                if (LocaleCompare("ycbcr",option) == 0)
                  quantize_info->colorspace=YCbCrColorspace;
                if (LocaleCompare("ycc",option) == 0)
                  quantize_info->colorspace=YCCColorspace;
                if (LocaleCompare("yiq",option) == 0)
                  quantize_info->colorspace=YIQColorspace;
                if (LocaleCompare("ypbpr",option) == 0)
                  quantize_info->colorspace=YPbPrColorspace;
                if (LocaleCompare("yuv",option) == 0)
                  quantize_info->colorspace=YUVColorspace;
                if (quantize_info->colorspace == UndefinedColorspace)
                  MagickFatalError(OptionFatalError,"InvalidColorspaceType",
                    option);
              }
            break;
          }
        if (LocaleCompare("comment",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  MagickFatalError(OptionFatalError,"Missing comment",option);
              }
            break;
          }
        if (LocaleCompare("compress",option+1) == 0)
          {
            image_info->compression=NoCompression;
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  MagickFatalError(OptionFatalError,"MissingType",option);
                option=argv[i];
                image_info->compression=UndefinedCompression;
                if (LocaleCompare("None",option) == 0)
                  image_info->compression=NoCompression;
                if (LocaleCompare("BZip",option) == 0)
                  image_info->compression=BZipCompression;
                if (LocaleCompare("Fax",option) == 0)
                  image_info->compression=FaxCompression;
                if (LocaleCompare("Group4",option) == 0)
                  image_info->compression=Group4Compression;
                if (LocaleCompare("JPEG",option) == 0)
                  image_info->compression=JPEGCompression;
                if (LocaleCompare("Lossless",option) == 0)
                  image_info->compression=LosslessJPEGCompression;
                if (LocaleCompare("LZW",option) == 0)
                  image_info->compression=LZWCompression;
                if (LocaleCompare("RLE",option) == 0)
                  image_info->compression=RLECompression;
                if (LocaleCompare("Zip",option) == 0)
                  image_info->compression=ZipCompression;
                if (image_info->compression == UndefinedCompression)
                  MagickFatalError(OptionFatalError,"UnrecognizedImageCompressionType",
                    option);
              }
            break;
          }
        if (LocaleCompare("contrast",option+1) == 0)
          break;
        if (LocaleCompare("crop",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !IsGeometry(argv[i]))
                  MagickFatalError(OptionFatalError,"MissingGeometry",option);
              }
            break;
          }
        MagickFatalError(OptionFatalError,"UnrecognizedOption",option);
        break;
      }
      case 'd':
      {
        if (LocaleCompare("debug",option+1) == 0)
          {
            (void) SetLogEventMask("None");
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  MagickFatalError(OptionFatalError,"MissingEventMask",option);
                (void) SetLogEventMask(argv[i]);
              }
            break;
          }
        if (LocaleCompare("delay",option+1) == 0)
          {
            resource_info.delay=0;
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  MagickFatalError(OptionFatalError,"MissingSeconds",option);
                resource_info.delay=atoi(argv[i]);
              }
            break;
          }
        if (LocaleCompare("density",option+1) == 0)
          {
            (void) CloneString(&image_info->density,(char *) NULL);
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !IsGeometry(argv[i]))
                  MagickFatalError(OptionFatalError,"MissingGeometry",option);
                (void) CloneString(&image_info->density,argv[i]);
              }
            break;
          }
        if (LocaleCompare("depth",option+1) == 0)
          {
            image_info->depth=QuantumDepth;
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  MagickFatalError(OptionFatalError,"MissingImageDepth",option);
                image_info->depth=atol(argv[i]);
              }
            break;
          }
        if (LocaleCompare("despeckle",option+1) == 0)
          break;
        if (LocaleCompare("display",option+1) == 0)
          {
            (void) CloneString(&image_info->server_name,(char *) NULL);
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  MagickFatalError(OptionFatalError,"MissingServerName",option);
                image_info->server_name=argv[i];
              }
            break;
          }
        if (LocaleCompare("dispose",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  MagickFatalError(OptionFatalError,"Missing method",option);
                option=argv[i];
                if ((LocaleCompare("0",option) != 0) &&
                    (LocaleCompare("1",option) != 0) &&
                    (LocaleCompare("2",option) != 0) &&
                    (LocaleCompare("3",option) != 0) &&
                    (LocaleCompare("Undefined",option) != 0) &&
                    (LocaleCompare("None",option) != 0) &&
                    (LocaleCompare("Background",option) != 0) &&
                    (LocaleCompare("Previous",option) != 0))
                  MagickFatalError(OptionFatalError,"UnrecognizedDisposeMethod",
                    option);
              }
            break;
          }
        if (LocaleCompare("dither",option+1) == 0)
          {
            quantize_info->dither=(*option == '-');
            break;
          }
        MagickFatalError(OptionFatalError,"UnrecognizedOption",option);
        break;
      }
      case 'e':
      {
        if (LocaleCompare("edge",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%lf",&sans))
                  MagickFatalError(OptionFatalError,"Missing factor",option);
              }
            break;
          }
        if (LocaleCompare("endian",option+1) == 0)
          {
            image_info->endian=UndefinedEndian;
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  MagickFatalError(OptionFatalError,"MissingType",option);
                option=argv[i];
                image_info->endian=UndefinedEndian;
                if (LocaleCompare("LSB",option) == 0)
                  image_info->endian=LSBEndian;
                if (LocaleCompare("MSB",option) == 0)
                  image_info->endian=MSBEndian;
                if (image_info->endian == UndefinedEndian)
                  MagickFatalError(OptionFatalError,"InvalidEndianType",option);
              }
            break;
          }
        if (LocaleCompare("enhance",option+1) == 0)
          break;
        MagickFatalError(OptionFatalError,"UnrecognizedOption",option);
        break;
      }
      case 'f':
      {
        if (LocaleCompare("filter",option+1) == 0)
          {
            if (*option == '-')
              {
                FilterTypes
                  filter;

                i++;
                if (i == argc)
                  MagickFatalError(OptionFatalError,"MissingType",option);
                option=argv[i];
                filter=UndefinedFilter;
                if (LocaleCompare("Point",option) == 0)
                  filter=PointFilter;
                if (LocaleCompare("Box",option) == 0)
                  filter=BoxFilter;
                if (LocaleCompare("Triangle",option) == 0)
                  filter=TriangleFilter;
                if (LocaleCompare("Hermite",option) == 0)
                  filter=HermiteFilter;
                if (LocaleCompare("Hanning",option) == 0)
                  filter=HanningFilter;
                if (LocaleCompare("Hamming",option) == 0)
                  filter=HammingFilter;
                if (LocaleCompare("Blackman",option) == 0)
                  filter=BlackmanFilter;
                if (LocaleCompare("Gaussian",option) == 0)
                  filter=GaussianFilter;
                if (LocaleCompare("Quadratic",option) == 0)
                  filter=QuadraticFilter;
                if (LocaleCompare("Cubic",option) == 0)
                  filter=CubicFilter;
                if (LocaleCompare("Catrom",option) == 0)
                  filter=CatromFilter;
                if (LocaleCompare("Mitchell",option) == 0)
                  filter=MitchellFilter;
                if (LocaleCompare("Lanczos",option) == 0)
                  filter=LanczosFilter;
                if (LocaleCompare("Bessel",option) == 0)
                  filter=BesselFilter;
                if (LocaleCompare("Sinc",option) == 0)
                  filter=SincFilter;
                if (filter == UndefinedFilter)
                  MagickFatalError(OptionFatalError,"UnrecognizedFilterType",
                    option);
              }
            break;
          }
        if (LocaleCompare("flip",option+1) == 0)
          break;
        if (LocaleCompare("flop",option+1) == 0)
          break;
        if (LocaleCompare("font",option+1) == 0)
          {
            (void) CloneString(&image_info->font,(char *) NULL);
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  MagickFatalError(OptionFatalError,"MissingFontName",option);
                image_info->font=argv[i];
              }
            if ((image_info->font == (char *) NULL) ||
                (*image_info->font != '@'))
              resource_info.font=AllocateString(image_info->font);
            break;
          }
        if (LocaleCompare("foreground",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  MagickFatalError(OptionFatalError,"MissingForeground",option);
                resource_info.foreground_color=argv[i];
              }
             break;
          }
        if (LocaleCompare("frame",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !IsGeometry(argv[i]))
                  MagickFatalError(OptionFatalError,"MissingGeometry",option);
              }
            break;
          }
        MagickFatalError(OptionFatalError,"UnrecognizedOption",option);
        break;
      }
      case 'g':
      {
        if (LocaleCompare("gamma",option+1) == 0)
          {
            i++;
            if ((i == argc) || !sscanf(argv[i],"%lf",&sans))
              MagickFatalError(OptionFatalError,"Missing value",option);
            break;
          }
        if (LocaleCompare("geometry",option+1) == 0)
          {
            resource_info.image_geometry=(char *) NULL;
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !IsGeometry(argv[i]))
                  MagickFatalError(OptionFatalError,"MissingGeometry",option);
                resource_info.image_geometry=argv[i];
              }
            break;
          }
        MagickFatalError(OptionFatalError,"UnrecognizedOption",option);
        break;
      }
      case 'h':
      {
        if (LocaleCompare("help",option+1) == 0)
          DisplayUsage();
        MagickFatalError(OptionFatalError,"UnrecognizedOption",option);
        break;
      }
      case 'i':
      {
        if (LocaleCompare("iconGeometry",option+1) == 0)
          {
            resource_info.icon_geometry=(char *) NULL;
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !IsGeometry(argv[i]))
                  MagickFatalError(OptionFatalError,"MissingGeometry",option);
                resource_info.icon_geometry=argv[i];
              }
            break;
          }
        if (LocaleCompare("iconic",option+1) == 0)
          {
            resource_info.iconic=(*option == '-');
            break;
          }
        if (LocaleCompare("immutable",option+1) == 0)
          {
            resource_info.immutable=(*option == '-');
            break;
          }
        if (LocaleCompare("interlace",option+1) == 0)
          {
            image_info->interlace=NoInterlace;
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  MagickFatalError(OptionFatalError,"MissingType",option);
                option=argv[i];
                image_info->interlace=UndefinedInterlace;
                if (LocaleCompare("None",option) == 0)
                  image_info->interlace=NoInterlace;
                if (LocaleCompare("Line",option) == 0)
                  image_info->interlace=LineInterlace;
                if (LocaleCompare("Plane",option) == 0)
                  image_info->interlace=PlaneInterlace;
                if (LocaleCompare("Partition",option) == 0)
                  image_info->interlace=PartitionInterlace;
                if (image_info->interlace == UndefinedInterlace)
                  MagickFatalError(OptionFatalError,"InvalidInterlaceType",
                    option);
              }
            break;
          }
        MagickFatalError(OptionFatalError,"UnrecognizedOption",option);
        break;
      }
      case 'l':
      {
        if (LocaleCompare("label",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  MagickFatalError(OptionFatalError,"MissingLabelName",option);
              }
            break;
          }
        if (LocaleCompare("limit",option+1) == 0)
          {
            if (*option == '-')
              {
                char
                  *type;

                i++;
                if (i == argc)
                  MagickFatalError(OptionFatalError,"MissingResourceType",
                    option);
                type=argv[i];
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  MagickFatalError(OptionFatalError,"MissingResourceLimit",
                    option);
                if (LocaleCompare("disk",type) == 0)
                  SetMagickResourceLimit(DiskResource,atol(argv[i]));
                else
                  if (LocaleCompare("map",type) == 0)
                    SetMagickResourceLimit(MapResource,atol(argv[i]));
                  else
                    if (LocaleCompare("memory",type) == 0)
                      SetMagickResourceLimit(MemoryResource,atol(argv[i]));
                    else
                      MagickFatalError(OptionFatalError,
                        "UnrecognizedResourceType",type);
              }
            break;
          }
        if (LocaleCompare("log",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  MagickFatalError(OptionFatalError,"MissingLogFormat",option);
                (void) SetLogFormat(argv[i]);
              }
            break;
          }
        MagickFatalError(OptionFatalError,"UnrecognizedOption",option);
        break;
      }
      case 'm':
      {
        if (LocaleCompare("magnify",option+1) == 0)
          {
            resource_info.magnify=2;
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  MagickFatalError(OptionFatalError,"MissingLevel",option);
                resource_info.magnify=atoi(argv[i]);
              }
            break;
          }
        if (LocaleCompare("map",option+1) == 0)
          {
            (void) strcpy(argv[i]+1,"sans");
            resource_info.map_type=(char *) NULL;
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  MagickFatalError(OptionFatalError,"MissingMapType",option);
                resource_info.map_type=argv[i];
              }
            break;
          }
        if (LocaleCompare("matte",option+1) == 0)
          break;
        if (LocaleCompare("mattecolor",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  MagickFatalError(OptionFatalError,"MissingColor",option);
                resource_info.matte_color=argv[i];
                (void) QueryColorDatabase(argv[i],&image_info->matte_color,
                  &exception);
              }
            break;
          }
          if (LocaleCompare("monochrome",option+1) == 0)
          {
            image_info->monochrome=(*option == '-');
            if (image_info->monochrome)
              {
                quantize_info->number_colors=2;
                quantize_info->tree_depth=8;
                quantize_info->colorspace=GRAYColorspace;
              }
            break;
          }
        MagickFatalError(OptionFatalError,"UnrecognizedOption",option);
        break;
      }
      case 'n':
      {
        if (LocaleCompare("name",option+1) == 0)
          {
            resource_info.name=(char *) NULL;
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  MagickFatalError(OptionFatalError,"MissingName",option);
                resource_info.name=argv[i];
              }
            break;
          }
        if (LocaleCompare("negate",option+1) == 0)
          break;
        if (LocaleCompare("noop",option+1) == 0)
          break;
        MagickFatalError(OptionFatalError,"UnrecognizedOption",option);
        break;
      }
      case 'p':
      {
        if (LocaleCompare("page",option+1) == 0)
          {
            (void) CloneString(&image_info->page,(char *) NULL);
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  MagickFatalError(OptionFatalError,"MissingPageGeometry",
                    option);
                image_info->page=GetPageGeometry(argv[i]);
              }
            break;
          }
        MagickFatalError(OptionFatalError,"UnrecognizedOption",option);
        break;
      }
      case 'q':
      {
        if (LocaleCompare("quality",option+1) == 0)
          {
            image_info->quality=DefaultCompressionQuality;
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !IsGeometry(argv[i]))
                  MagickFatalError(OptionFatalError,"MissingCompressionQuality",
                    option);
                image_info->quality=atol(argv[i]);
              }
            break;
          }
        MagickFatalError(OptionFatalError,"UnrecognizedOption",option);
        break;
      }
      case 'r':
      {
        if (LocaleCompare("raise",option+1) == 0)
          {
            i++;
            if ((i == argc) || !sscanf(argv[i],"%ld",&x))
              MagickFatalError(OptionFatalError,"MissingBevelWidth",option);
            break;
          }
        if (LocaleCompare("remote",option+1) == 0)
          {
            i++;
            if (i == argc)
              MagickFatalError(OptionFatalError,"MissingCommand",option);
            status=XRemoteCommand(display,resource_info.window_id,argv[i]);
            Exit(!status);
          }
        if (LocaleCompare("roll",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !IsGeometry(argv[i]))
                  MagickFatalError(OptionFatalError,"MissingGeometry",option);
              }
            break;
          }
        if (LocaleCompare("rotate",option+1) == 0)
          {
            i++;
            if ((i == argc) || !IsGeometry(argv[i]))
              MagickFatalError(OptionFatalError,"MissingDegrees",option);
            break;
          }
        MagickFatalError(OptionFatalError,"UnrecognizedOption",option);
        break;
      }
      case 's':
      {
        if (LocaleCompare("sample",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !IsGeometry(argv[i]))
                  MagickFatalError(OptionFatalError,"MissingGeometry",option);
              }
            break;
          }
        if (LocaleCompare("sampling-factor",option+1) == 0)
          {
            (void) CloneString(&image_info->sampling_factor,(char *) NULL);
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !IsGeometry(argv[i]))
                  MagickFatalError(OptionFatalError,"MissingGeometry",option);
                (void) CloneString(&image_info->sampling_factor,argv[i]);
              }
            break;
          }
        if (LocaleCompare("scenes",option+1) == 0)
          {
            first_scene=0;
            last_scene=0;
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  MagickFatalError(OptionFatalError,"MissingSceneNumber",
                    option);
                first_scene=atol(argv[i]);
                last_scene=first_scene;
                (void) sscanf(argv[i],"%ld-%ld",&first_scene,&last_scene);
              }
            break;
          }
        if (LocaleCompare("segment",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%lf",&sans))
                  MagickFatalError(OptionFatalError,"MissingThreshold",option);
              }
            break;
          }
        if (LocaleCompare("sharpen",option+1) == 0)
          {
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%lf",&sans))
                  MagickFatalError(OptionFatalError,"MissingGeometry",option);
              }
            break;
          }
        if (LocaleCompare("shared-memory",option+1) == 0)
          {
            resource_info.use_shared_memory=(*option == '-');
            break;
          }
        if (LocaleCompare("size",option+1) == 0)
          {
            (void) CloneString(&image_info->size,(char *) NULL);
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !IsGeometry(argv[i]))
                  MagickFatalError(OptionFatalError,"MissingGeometry",option);
                (void) CloneString(&image_info->size,argv[i]);
              }
            break;
          }
        MagickFatalError(OptionFatalError,"UnrecognizedOption",option);
        break;
      }
      case 't':
      {
        if (LocaleCompare("text_font",option+1) == 0)
          {
            resource_info.text_font=(char *) NULL;
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  MagickFatalError(OptionFatalError,"MissingFontName",option);
                resource_info.text_font=argv[i];
              }
            break;
          }
        if (LocaleCompare("texture",option+1) == 0)
          {
            (void) CloneString(&image_info->texture,(char *) NULL);
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  MagickFatalError(OptionFatalError,"MissingFilename",option);
                (void) CloneString(&image_info->texture,argv[i]);
              }
            break;
          }
        if (LocaleCompare("title",option+1) == 0)
          {
            resource_info.title=(char *) NULL;
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  MagickFatalError(OptionFatalError,"MissingTitle",option);
                resource_info.title=argv[i];
              }
            break;
          }
        if (LocaleCompare("treedepth",option+1) == 0)
          {
            quantize_info->tree_depth=0;
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  MagickFatalError(OptionFatalError,"MissingDepth",option);
                quantize_info->tree_depth=atoi(argv[i]);
              }
            break;
          }
        if (LocaleCompare("trim",option+1) == 0)
          break;
        MagickFatalError(OptionFatalError,"UnrecognizedOption",option);
        break;
      }
      case 'u':
      {
        if (LocaleCompare("update",option+1) == 0)
          {
            resource_info.update=(*option == '-');
            if (*option == '-')
              {
                i++;
                if ((i == argc) || !sscanf(argv[i],"%ld",&x))
                  MagickFatalError(OptionFatalError,"MissingSeconds",option);
                resource_info.update=atoi(argv[i]);
              }
            break;
          }
        if (LocaleCompare("use_pixmap",option+1) == 0)
          {
            resource_info.use_pixmap=(*option == '-');
            break;
          }
        MagickFatalError(OptionFatalError,"UnrecognizedOption",option);
        break;
      }
      case 'v':
      {
        if (LocaleCompare("verbose",option+1) == 0)
          {
            image_info->verbose=(*option == '-');
            break;
          }
        if (LocaleCompare("version",option+1) == 0)
          break;
        if (LocaleCompare("visual",option+1) == 0)
          {
            resource_info.visual_type=(char *) NULL;
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  MagickFatalError(OptionFatalError,"MissingVisualClass",
                    option);
                resource_info.visual_type=argv[i];
              }
            break;
          }
        if (LocaleCompare("virtual-pixel",option+1) == 0)
          {
            if (*option == '-')
              {
                VirtualPixelMethod
                  virtual_pixel_method;

                i++;
                if (i == argc)
                  MagickFatalError(OptionFatalError,"MissingVirtualPixelMethod",
                    option);
                option=argv[i];
                virtual_pixel_method=UndefinedVirtualPixelMethod;
                if (LocaleCompare("Constant",option) == 0)
                  virtual_pixel_method=ConstantVirtualPixelMethod;
                if (LocaleCompare("Edge",option) == 0)
                  virtual_pixel_method=EdgeVirtualPixelMethod;
                if (LocaleCompare("Mirror",option) == 0)
                  virtual_pixel_method=MirrorVirtualPixelMethod;
                if (LocaleCompare("Tile",option) == 0)
                  virtual_pixel_method=TileVirtualPixelMethod;
                if (virtual_pixel_method == UndefinedVirtualPixelMethod)
                  MagickFatalError(OptionFatalError,
                    "UnrecognizedVirtualPixelMethod",option);
              }
            break;
          }
        MagickFatalError(OptionFatalError,"UnrecognizedOption",option);
        break;
      }
      case 'w':
      {
        if (LocaleCompare("window",option+1) == 0)
          {
            resource_info.window_id=(char *) NULL;
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  MagickFatalError(OptionFatalError,"MissingIDNameOrRoot",
                    option);
                resource_info.window_id=argv[i];
              }
            break;
          }
        if (LocaleCompare("window_group",option+1) == 0)
          {
            resource_info.window_group=(char *) NULL;
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  MagickFatalError(OptionFatalError,"MissingIDNameOrRoot",
                    option);
                resource_info.window_group=argv[i];
              }
            break;
          }
        if (LocaleCompare("write",option+1) == 0)
          {
            resource_info.write_filename=(char *) NULL;
            if (*option == '-')
              {
                i++;
                if (i == argc)
                  MagickFatalError(OptionFatalError,"MissingFilename",option);
                resource_info.write_filename=argv[i];
                if (IsAccessible(resource_info.write_filename))
                  {
                    char
                      answer[2];

                    (void) fprintf(stderr,"Overwrite %.1024s? ",
                      resource_info.write_filename);
                    (void) fgets(answer,sizeof(answer),stdin);
                    if (!((*answer == 'y') || (*answer == 'Y')))
                      Exit(0);
                  }
              }
            break;
          }
        MagickFatalError(OptionFatalError,"UnrecognizedOption",option);
        break;
      }
      case '?':
      {
        DisplayUsage();
        break;
      }
      default:
      {
        MagickFatalError(OptionFatalError,"UnrecognizedOption",option);
        break;
      }
    }
  }
  if (state & RetainColorsState)
    {
      XRetainWindowColors(display,XRootWindow(display,XDefaultScreen(display)));
      XSync(display,False);
    }
  LiberateArgumentList(argc,argv);
  return(status);
#else
  MagickFatalError(MissingDelegateError,"XWindowLibraryIsNotAvailable",
    (char *) NULL);
  return(False);
#endif
}




/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   C o m p o s i t e U s a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  CompositeUsage() displays the program command syntax.
%
%  The format of the CompositeUsage method is:
%
%      void CompositeUsage()
%
%
*/
MagickExport void CompositeUsage(void)
{
  const char
    **p;

  static const char
    *options[]=
    {
      "-affine matrix       affine transform matrix",
      "-authenticate value  decrypt image with this password",
      "-blue-primary point  chomaticity blue primary point",
      "-colors value        preferred number of colors in the image",
      "-colorspace type     alternate image colorspace",
      "-comment string      annotate image with comment",
      "-compose operator    composite operator",
      "-compress type       image compression tyhpe",
      "-debug events        display copious debugging information",
      "-density geometry    horizontal and vertical density of the image",
      "-depth value         image depth",
      "-displace geometry   shift image pixels defined by a displacement map",
      "-display server      get image or font from this X server",
      "-dispose method      Undefined, None, Background, Previous",
      "-dissolve value      dissolve the two images a given percent",
      "-dither              apply Floyd/Steinberg error diffusion to image",
      "-encoding type       text encoding type",
      "-endian type         LSB or MSB",
      "-filter type         use this filter when resizing an image",
      "-font name           render text with this font",
      "-geometry geometry   location of the composite image",
      "-gravity type        which direction to gravitate towards",
      "-green-primary point chomaticity green primary point",
      "-help                print program options",
      "-interlace type      None, Line, Plane, or Partition",
      "-label name          ssign a label to an image",
      "-limit type value    Disk, Map, or Memory resource limit",
      "-log format          format of debugging information",
      "-matte               store matte channel if the image has one",
      "-monochrome          transform image to black and white",
      "-negate              replace every pixel with its complementary color ",
      "-page geometry       size and location of an image canvas",
      "-profile filename    add ICM or IPTC information profile to image",
      "-quality value       JPEG/MIFF/PNG compression level",
      "-red-primary point   chomaticity red primary point",
      "-rotate degrees      apply Paeth rotation to the image",
      "-resize geometry     resize the image",
      "-sampling-factor geometry",
      "                     horizontal and vertical sampling factor",
      "-scene value         image scene number",
      "-sharpen geometry    sharpen the image",
      "-size geometry       width and height of image",
      "-stegano offset      hide watermark within an image",
      "-stereo              combine two image to create a stereo anaglyph",
      "-tile                repeat composite operation across image",
      "-transform           affine transform image",
      "-treedepth value     color tree depth",
      "-type type           image type",
      "-units type          PixelsPerInch, PixelsPerCentimeter, or Undefined",
      "-unsharp geometry    sharpen the image",
      "-verbose             print detailed information about the image",
      "-version             print version information",
      "-virtual-pixel method",
      "                     Constant, Edge, Mirror, or Tile",
      "-watermark geometry  percent brightness and saturation of a watermark",
      "-white-point point   chomaticity white point",
      "-write filename      write images to this file",
      (char *) NULL
    };

  (void) printf("Version: %.1024s\n",GetMagickVersion((unsigned long *) NULL));
  (void) printf("Copyright: %.1024s\n\n",GetMagickCopyright());
  (void) printf("Usage: %.1024s [options ...] image [options ...] composite\n"
    "  [ [options ...] mask ] [options ...] composite\n",
    SetClientName((char *) NULL));
  (void) printf("\nWhere options include:\n");
  for (p=options; *p != (char *) NULL; p++)
    (void) printf("  %.1024s\n",*p);
}



/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   C o n v e r t U s a g e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Procedure ConvertUsage displays the program command syntax.
%
%  The format of the ConvertUsage method is:
%
%      void ConvertUsage()
%
*/
MagickExport void ConvertUsage(void)
{
  static const char
    *options[]=
    {
      "-adjoin              join images into a single multi-image file",
      "-affine matrix       affine transform matrix",
      "-antialias           remove pixel-aliasing",
      "-append              append an image sequence",
      "-authenticate value  decrypt image with this password",
      "-average             average an image sequence",
      "-background color    background color",
      "-blue-primary point  chomaticity blue primary point",
      "-blur geometry       blur the image",
      "-border geometry     surround image with a border of color",
      "-bordercolor color   border color",
      "-channel type        extract a particular color channel from image",
      "-charcoal radius     simulate a charcoal drawing",
      "-chop geometry       remove pixels from the image interior",
      "-clip                apply clipping path if the image has one",
      "-coalesce            merge a sequence of images",
      "-colorize value      colorize the image with the fill color",
      "-colors value        preferred number of colors in the image",
      "-colorspace type     alternate image colorspace",
      "-comment string      annotate image with comment",
      "-compress type       image compression tyhpe",
      "-contrast            enhance or reduce the image contrast",
      "-crop geometry       preferred size and location of the cropped image",
      "-cycle amount        cycle the image colormap",
      "-debug events        display copious debugging information",
      "-deconstruct         break down an image sequence into constituent parts",
      "-delay value         display the next image after pausing",
      "-density geometry    horizontal and vertical density of the image",
      "-depth value         image depth",
      "-despeckle           reduce the speckles within an image",
      "-display server      get image or font from this X server",
      "-dispose method      Undefined, None, Background, Previous",
      "-dither              apply Floyd/Steinberg error diffusion to image",
      "-draw string         annotate the image with a graphic primitive",
      "-edge radius         apply a filter to detect edges in the image",
      "-emboss radius       emboss an image",
      "-encoding type       text encoding type",
      "-endian type         LSB or MSB",
      "-enhance             apply a digital filter to enhance a noisy image",
      "-equalize            perform histogram equalization to an image",
      "-fill color          color to use when filling a graphic primitive",
      "-filter type         use this filter when resizing an image",
      "-flatten             flatten a sequence of images",
      "-flip                flip image in the vertical direction",
      "-flop                flop image in the horizontal direction",
      "-font name           render text with this font",
      "-frame geometry      surround image with an ornamental border",
      "-fuzz distance       colors within this distance are considered equal",
      "-gamma value         level of gamma correction",
      "-gaussian geometry   gaussian blur an image",
      "-geometry geometry   perferred size or location of the image",
      "-green-primary point chomaticity green primary point",
      "-gravity type        horizontal and vertical text placement",
      "-help                print program options",
      "-implode amount      implode image pixels about the center",
      "-intent type         Absolute, Perceptual, Relative, or Saturation",
      "-interlace type      None, Line, Plane, or Partition",
      "-label name          assign a label to an image",
      "-lat geometry        local adaptive thresholding",
      "-level value         adjust the level of image contrast",
      "-limit type value    Disk, Map, or Memory resource limit",
      "-list type           Color, Delegate, Format, Magic, Module, or Type",
      "-log format          format of debugging information",
      "-loop iterations     add Netscape loop extension to your GIF animation",
      "-map filename        transform image colors to match this set of colors",
      "-mask filename       set the image clip mask",
      "-matte               store matte channel if the image has one",
      "-median radius       apply a median filter to the image",
      "-modulate value      vary the brightness, saturation, and hue",
      "-monochrome          transform image to black and white",
      "-morph value         morph an image sequence",
      "-mosaic              create a mosaic from an image sequence",
      "-negate              replace every pixel with its complementary color ",
      "-noop                do not apply options to image",
      "-noise radius        add or reduce noise in an image",
      "-normalize           transform image to span the full range of colors",
      "-opaque color        change this color to the fill color",
      "-ordered-dither channeltype NxN",
      "                     ordered dither the image",
      "-page geometry       size and location of an image canvas",
      "-paint radius        simulate an oil painting",
      "-ping                efficiently determine image attributes",
      "-pointsize value     font point size",
      "-preview type        image preview type",
      "-profile filename    add ICM or IPTC information profile to image",
      "-quality value       JPEG/MIFF/PNG compression level",
      "-raise value         lighten/darken image edges to create a 3-D effect",
      "-region geometry     apply options to a portion of the image",
      "-raise value         lighten/darken image edges to create a 3-D effect",
      "-random-threshold channeltype LOWxHIGH",
      "                     random threshold the image",
      "-red-primary point   chomaticity red primary point",
      "-render              render vector graphics",
      "-resize geometry     resize the image",
      "-roll geometry       roll an image vertically or horizontally",
      "-rotate degrees      apply Paeth rotation to the image",
      "-sample geometry     scale image with pixel sampling",
      "-sampling-factor geometry",
      "                     horizontal and vertical sampling factor",
      "-scale geometry      scale the image",
      "-scene value         image scene number",
      "-seed value          pseudo-random number generator seed value",
      "-segment values      segment an image",
      "-shade degrees       shade the image using a distant light source",
      "-sharpen geometry    sharpen the image",
      "-shave geometry      shave pixels from the image edges",
      "-shear geometry      slide one edge of the image along the X or Y axis",
      "-size geometry       width and height of image",
      "-solarize threshold  negate all pixels above the threshold level",
      "-spread amount       displace image pixels by a random amount",
      "-stroke color        graphic primitive stroke color",
      "-strokewidth value   graphic primitive stroke width",
      "-swirl degrees       swirl image pixels about the center",
      "-texture filename    name of texture to tile onto the image background",
      "-threshold value     threshold the image",
      "-tile filename       tile image when filling a graphic primitive",
      "-transform           affine transform image",
      "-transparent color   make this color transparent within the image",
      "-treedepth value     color tree depth",
      "-trim                trim image edges",
      "-type type           image type",
      "-undercolor color    annotation bounding box color",
      "-units type          PixelsPerInch, PixelsPerCentimeter, or Undefined",
      "-unsharp geometry    sharpen the image",
      "-verbose             print detailed information about the image",
      "-version             print version information",
      "-view                FlashPix viewing transforms",
      "-virtual-pixel method",
      "                     Constant, Edge, Mirror, or Tile",
      "-wave geometry       alter an image along a sine wave",
      "-white-point point   chomaticity white point",
      "-write filename      write images to this file",
      (char *) NULL
    };

  const char
    **p;

  (void) printf("Version: %.1024s\n",GetMagickVersion((unsigned long *) NULL));
  (void) printf("Copyright: %.1024s\n\n",GetMagickCopyright());
  (void) printf("Usage: %.1024s [options ...] file [ [options ...] "
    "file ...] [options ...] file\n",SetClientName((char *) NULL));
  (void) printf("\nWhere options include:\n");
  for (p=options; *p != (char *) NULL; p++)
    (void) printf("  %.1024s\n",*p);
  (void) printf(
    "\nBy default, the image format of `file' is determined by its magic\n");
  (void) printf(
    "number.  To specify a particular image format, precede the filename\n");
  (void) printf(
    "with an image format name and a colon (i.e. ps:image) or specify the\n");
  (void) printf(
    "image type as the filename suffix (i.e. image.ps).  Specify 'file' as\n");
  (void) printf("'-' for standard input or output.\n");
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M o g r i f y U s a g e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MogrifyUsage() displays the program command syntax.
%
%  The format of the MogrifyUsage method is:
%
%      void MogrifyUsage()
%
%
*/
MagickExport void MogrifyUsage(void)
{
  static const char
    *options[]=
    {
      "-affine matrix       affine transform matrix",
      "-antialias           remove pixel-aliasing",
      "-authenticate value  decrypt image with this password",
      "-background color    background color",
      "-blue-primary point  chomaticity blue primary point",
      "-blur radius         blur the image",
      "-border geometry     surround image with a border of color",
      "-bordercolor color   border color",
      "-channel type        extract a particular color channel from image",
      "-charcoal radius     simulate a charcoal drawing",
      "-chop geometry       remove pixels from the image interior",
      "-colorize value      colorize the image with the fill color",
      "-colors value        preferred number of colors in the image",
      "-colorspace type     alternate image colorspace",
      "-comment string      annotate image with comment",
      "-compress type       image compression tyhpe",
      "-contrast            enhance or reduce the image contrast",
      "-crop geometry       preferred size and location of the cropped image",
      "-cycle amount        cycle the image colormap",
      "-debug events        display copious debugging information",
      "-delay value         display the next image after pausing",
      "-density geometry    horizontal and vertical density of the image",
      "-depth value         image depth",
      "-despeckle           reduce the speckles within an image",
      "-display server      get image or font from this X server",
      "-dispose method      Undefined, None, Background, Previous",
      "-dither              apply Floyd/Steinberg error diffusion to image",
      "-draw string         annotate the image with a graphic primitive",
      "-edge radius         apply a filter to detect edges in the image",
      "-emboss radius       emboss an image",
      "-encoding type       text encoding type",
      "-endian type         LSB or MSB",
      "-enhance             apply a digital filter to enhance a noisy image",
      "-equalize            perform histogram equalization to an image",
      "-fill color          color to use when filling a graphic primitive",
      "-filter type         use this filter when resizing an image",
      "-flip                flip image in the vertical direction",
      "-flop                flop image in the horizontal direction",
      "-font name           render text with this font",
      "-format type         image format type",
      "-frame geometry      surround image with an ornamental border",
      "-fuzz distance       colors within this distance are considered equal",
      "-gamma value         level of gamma correction",
      "-gaussian geometry   gaussian blur an image",
      "-geometry geometry   perferred size or location of the image",
      "-green-primary point chomaticity green primary point",
      "-implode amount      implode image pixels about the center",
      "-interlace type      None, Line, Plane, or Partition",
      "-help                print program options",
      "-label name          assign a label to an image",
      "-lat geometry        local adaptive thresholding",
      "-level value         adjust the level of image contrast",
      "-limit type value    Disk, Map, or Memory resource limit",
      "-list type           Color, Delegate, Format, Magic, Module, or Type",
      "-log format          format of debugging information",
      "-loop iterations     add Netscape loop extension to your GIF animation",
      "-map filename        transform image colors to match this set of colors",
      "-mask filename       set the image clip mask",
      "-matte               store matte channel if the image has one",
      "-median radius       apply a median filter to the image",
      "-modulate value      vary the brightness, saturation, and hue",
      "-monochrome          transform image to black and white",
      "-negate              replace every pixel with its complementary color ",
      "-noop                do not apply options to image"
      "-noise radius        add or reduce noise in an image.",
      "-normalize           transform image to span the full range of colors",
      "-opaque color        change this color to the fill color",
      "-ordered-dither channeltype NxN",
      "                     ordered dither the image",
      "-page geometry       size and location of an image canvas",
      "-paint radius        simulate an oil painting",
      "-fill color           color for annotating or changing opaque color",
      "-pointsize value     font point size",
      "-profile filename    add ICM or IPTC information profile to image",
      "-quality value       JPEG/MIFF/PNG compression level",
      "-raise value         lighten/darken image edges to create a 3-D effect",
      "-random-threshold channeltype LOWxHIGH",
      "                     random threshold the image",
      "-red-primary point  chomaticity red primary point",
      "-region geometry     apply options to a portion of the image",
      "-resize geometry     perferred size or location of the image",
      "-roll geometry       roll an image vertically or horizontally",
      "-rotate degrees      apply Paeth rotation to the image",
      "-sample geometry     scale image with pixel sampling",
      "-sampling-factor geometry",
      "                     horizontal and vertical sampling factor",
      "-scale geometry      scale the image",
      "-scene number        image scene number",
      "-seed value          pseudo-random number generator seed value",
      "-segment values      segment an image",
      "-shade degrees       shade the image using a distant light source",
      "-sharpen radius      sharpen the image",
      "-shear geometry      slide one edge of the image along the X or Y axis",
      "-size geometry       width and height of image",
      "-solarize threshold  negate all pixels above the threshold level",
      "-spread amount       displace image pixels by a random amount",
      "-stroke color        graphic primitive stroke color",
      "-strokewidth value   graphic primitive stroke width",
      "-swirl degrees       swirl image pixels about the center",
      "-texture filename    name of texture to tile onto the image background",
      "-threshold value     threshold the image",
      "-tile filename       tile image when filling a graphic primitive",
      "-transform           affine transform image",
      "-transparent color   make this color transparent within the image",
      "-treedepth value     color tree depth",
      "-trim                trim image edges",
      "-type type           image type",
      "-undercolor color    annotation bounding box color",
      "-units type          PixelsPerInch, PixelsPerCentimeter, or Undefined",
      "-unsharp geometry    sharpen the image",
      "-verbose             print detailed information about the image",
      "-version             print version information",
      "-view                FlashPix viewing transforms",
      "-virtual-pixel method",
      "                     Constant, Edge, Mirror, or Tile",
      "-wave geometry       alter an image along a sine wave",
      "-white-point point   chomaticity white point",
      (char *) NULL
    };

  const char
    **p;

  (void) printf("Version: %.1024s\n",GetMagickVersion((unsigned long *) NULL));
  (void) printf("Copyright: %.1024s\n\n",GetMagickCopyright());
  (void) printf("Usage: %.1024s [options ...] file [ [options ...] file ...]\n",
    SetClientName((char *) NULL));
  (void) printf("\nWhere options include: \n");
  for (p=options; *p != (char *) NULL; p++)
    (void) printf("  %.1024s\n",*p);
  (void) printf(
    "\nBy default, the image format of `file' is determined by its magic\n");
  (void) printf(
    "number.  To specify a particular image format, precede the filename\n");
  (void) printf(
    "with an image format name and a colon (i.e. ps:image) or specify the\n");
  (void) printf(
    "image type as the filename suffix (i.e. image.ps).  Specify 'file' as\n");
  (void) printf("'-' for standard input or output.\n");
}


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M o n t a g e U s a g e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MontageUsage() displays the program command syntax.
%
%  The format of the MontageUsage method is:
%
%      void MontageUsage()
%
%
*/
MagickExport void MontageUsage(void)
{
  const char
    **p;

  static const char
    *options[]=
    {
      "-adjoin              join images into a single multi-image file",
      "-affine matrix       affine transform matrix",
      "-authenticate value  decrypt image with this password",
      "-blue-primary point  chomaticity blue primary point",
      "-blur factor         apply a filter to blur the image",
      "-colors value        preferred number of colors in the image",
      "-colorspace type     alternate image colorsapce",
      "-comment string      annotate image with comment",
      "-compose operator    composite operator",
      "-compress type       image compression tyhpe",
      "-crop geometry       preferred size and location of the cropped image",
      "-debug events        display copious debugging information",
      "-density geometry    horizontal and vertical density of the image",
      "-depth value         image depth",
      "-display server      query font from this X server",
      "-dispose method      Undefined, None, Background, Previous",
      "-dither              apply Floyd/Steinberg error diffusion to image",
      "-draw string         annotate the image with a graphic primitive",
      "-encoding type       text encoding type",
      "-endian type         LSB or MSB",
      "-fill color          color to use when filling a graphic primitive",
      "-filter type         use this filter when resizing an image",
      "-flip                flip image in the vertical direction",
      "-flop                flop image in the horizontal direction",
      "-frame geometry      surround image with an ornamental border",
      "-gamma value         level of gamma correction",
      "-geometry geometry   preferred tile and border sizes",
      "-gravity direction   which direction to gravitate towards",
      "-green-primary point chomaticity green primary point",
      "-interlace type      None, Line, Plane, or Partition",
      "-help                print program options",
      "-label name          assign a label to an image",
      "-limit type value    Disk, Map, or Memory resource limit",
      "-log format          format of debugging information",
      "-matte               store matte channel if the image has one",
      "-mode type           Frame, Unframe, or Concatenate",
      "-monochrome          transform image to black and white",
      "-noop                do not apply options to image"
      "-page geometry       size and location of an image canvas",
      "-pointsize value     font point size",
      "-quality value       JPEG/MIFF/PNG compression level",
      "-red-primary point   chomaticity red primary point",
      "-resize geometry     resize the image",
      "-rotate degrees      apply Paeth rotation to the image",
      "-sampling-factor geometry",
      "                     horizontal and vertical sampling factor",
      "-scenes range        image scene range",
      "-shadow              add a shadow beneath a tile to simulate depth",
      "-size geometry       width and height of image",
      "-stroke color        color to use when stroking a graphic primitive",
      "-texture filename    name of texture to tile onto the image background",
      "-tile geometry       number of tiles per row and column",
      "-transform           affine transform image",
      "-transparent color   make this color transparent within the image",
      "-treedepth value     color tree depth",
      "-trim                trim image edges",
      "-type type           image type",
      "-verbose             print detailed information about the image",
      "-version             print version information",
      "-virtual-pixel method",
      "                     Constant, Edge, Mirror, or Tile",
      "-white-point point   chomaticity white point",
      (char *) NULL
    };

  (void) printf("Version: %.1024s\n",GetMagickVersion((unsigned long *) NULL));
  (void) printf("Copyright: %.1024s\n\n",GetMagickCopyright());
  (void) printf("Usage: %.1024s [options ...] file [ [options ...] file ...]\n",
    SetClientName((char *) NULL));
  (void) printf("\nWhere options include: \n");
  for (p=options; *p != (char *) NULL; p++)
    (void) printf("  %.1024s\n",*p);
  (void) printf(
    "\nIn addition to those listed above, you can specify these standard X\n");
  (void) printf(
    "resources as command line options:  -background, -bordercolor,\n");
  (void) printf(
    "-borderwidth, -font, -mattecolor, or -title\n");
  (void) printf(
    "\nBy default, the image format of `file' is determined by its magic\n");
  (void) printf(
    "number.  To specify a particular image format, precede the filename\n");
  (void) printf(
    "with an image format name and a colon (i.e. ps:image) or specify the\n");
  (void) printf(
    "image type as the filename suffix (i.e. image.ps).  Specify 'file' as\n");
  (void) printf("'-' for standard input or output.\n");
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I d e n t i f y U s a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IdentifyUsage() displays the program command syntax.
%
%  The format of the IdentifyUsage method is:
%
%      void IdentifyUsage()
%
%
*/
MagickExport void IdentifyUsage(void)
{
  const char
    **p;

  static const char
    *options[]=
    {
      "-authenticate value  decrypt image with this password",
      "-debug events        display copious debugging information",
      "-density geometry    horizontal and vertical density of the image",
      "-depth value         image depth",
      "-format \"string\"   output formatted image characteristics",
      "-help                print program options",
      "-interlace type      None, Line, Plane, or Partition",
      "-limit type value    Disk, Map, or Memory resource limit",
      "-log format          format of debugging information",
      "-size geometry       width and height of image",
      "-sampling-factor geometry",
      "                     horizontal and vertical sampling factor",
      "-verbose             print detailed information about the image",
      "-version             print version information",
      "-virtual-pixel method",
      "                     Constant, Edge, Mirror, or Tile",
      (char *) NULL
    };

  (void) printf("Version: %.1024s\n",GetMagickVersion((unsigned long *) NULL));
  (void) printf("Copyright: %.1024s\n\n",GetMagickCopyright());
  (void) printf("Usage: %.1024s [options ...] file [ [options ...] "
    "file ... ]\n",SetClientName((char *) NULL));
  (void) printf("\nWhere options include:\n");
  for (p=options; *p != (char *) NULL; p++)
    (void) printf("  %.1024s\n",*p);
}
