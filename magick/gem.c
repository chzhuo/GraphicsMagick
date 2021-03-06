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
%                              GGGG  EEEEE  M   M                             %
%                             G      E      MM MM                             %
%                             G GG   EEE    M M M                             %
%                             G   G  E      M   M                             %
%                              GGGG  EEEEE  M   M                             %
%                                                                             %
%                                                                             %
%                    Graphic Gems - Graphic Support Methods                   %
%                                                                             %
%                                                                             %
%                               Software Design                               %
%                                 John Cristy                                 %
%                                 August 1996                                 %
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
#include "magick/cache.h"
#include "magick/gem.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   C o n s t r a s t                                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method Contrast enhances the intensity differences between the lighter
%  and darker elements of the image.
%
%  The format of the Contrast method is:
%
%      void Contrast(const int sign,Quantum *red,Quantum *green,Quantum *blue)
%
%  A description of each parameter follows:
%
%    o sign: A positive value enhances the contrast otherwise it is reduced.
%
%    o red, green, blue: A pointer to a pixel component of type Quantum.
%
%
*/
MagickExport void Contrast(const int sign,Quantum *red,Quantum *green,
  Quantum *blue)
{
  const static double
    alpha=0.5+MagickEpsilon;

  double
    brightness,
    hue,
    saturation;

  /*
    Enhance contrast: dark color become darker, light color become lighter.
  */
  assert(red != (Quantum *) NULL);
  assert(green != (Quantum *) NULL);
  assert(blue != (Quantum *) NULL);
  TransformHSL(*red,*green,*blue,&hue,&saturation,&brightness);
  brightness+=
    alpha*sign*(alpha*(sin(MagickPI*(brightness-alpha))+1.0)-brightness);
  if (brightness > 1.0)
    brightness=1.0;
  if (brightness < 0.0)
    brightness=0.0;
  HSLTransform(hue,saturation,brightness,red,green,blue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   E x p a n d A f f i n e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method ExpandAffine computes the affine's expansion factor, i.e. the
%  square root of the factor by which the affine transform affects area. In an
%  affine transform composed of scaling, rotation, shearing, and translation,
%  returns the amount of scaling.
%
%  The format of the ExpandAffine method is:
%
%      double ExpandAffine(const AffineMatrix *affine)
%
%  A description of each parameter follows:
%
%    o expansion: Method ExpandAffine returns the affine's expansion factor.
%
%    o affine: A pointer the the affine transform of type AffineMatrix.
%
%
*/
MagickExport double ExpandAffine(const AffineMatrix *affine)
{
  double
    expand;

  assert(affine != (const AffineMatrix *) NULL);
  expand=fabs(affine->sx*affine->sy)-fabs(affine->rx*affine->ry);
  if (fabs(expand) < MagickEpsilon)
    return(1.0);
  return(sqrt(fabs(expand)));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e n e r a t e N o i s e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method GenerateNoise adds noise to a pixel.
%
%  The format of the GenerateNoise method is:
%
%      Quantum GenerateNoise(const Quantum pixel,const NoiseType noise_type)
%
%  A description of each parameter follows:
%
%    o pixel: A structure of type Quantum.
%
%    o noise_type:  The type of noise: Gaussian, multiplicative Gaussian,
%      impulse, laplacian, or Poisson.
%
%
*/
MagickExport Quantum GenerateNoise(const Quantum pixel,
  const NoiseType noise_type)
{
#define NoiseEpsilon  1.0e-5
#define SigmaUniform  4.0
#define SigmaGaussian  4.0
#define SigmaImpulse  0.10
#define SigmaLaplacian 10.0
#define SigmaMultiplicativeGaussian  0.5
#define SigmaPoisson  0.05
#define TauGaussian  20.0

  double
    alpha,
    beta,
    sigma,
    value;

  alpha=(double) rand()/RAND_MAX;
  if (alpha == 0.0)
    alpha=1.0;
  switch (noise_type)
  {
    case UniformNoise:
    default:
    {
      value=(double) pixel+SigmaUniform*(alpha-0.5);
      break;
    }
    case GaussianNoise:
    {
      double
        tau;

      beta=(double) rand()/RAND_MAX;
      sigma=sqrt(-2.0*log(alpha))*cos(2.0*MagickPI*beta);
      tau=sqrt(-2.0*log(alpha))*sin(2.0*MagickPI*beta);
      value=(double) pixel+sqrt((double) pixel)*SigmaGaussian*sigma+
        TauGaussian*tau;
      break;
    }
    case MultiplicativeGaussianNoise:
    {
      if (alpha <= NoiseEpsilon)
        sigma=MaxRGB;
      else
        sigma=sqrt(-2.0*log(alpha));
      beta=(double) rand()/RAND_MAX;
      value=(double) pixel+pixel*SigmaMultiplicativeGaussian*sigma*
        cos(2.0*MagickPI*beta);
      break;
    }
    case ImpulseNoise:
    {
      if (alpha < (SigmaImpulse/2.0))
        value=0;
       else
         if (alpha >= (1.0-(SigmaImpulse/2.0)))
           value=MaxRGB;
         else
           value=pixel;
      break;
    }
    case LaplacianNoise:
    {
      if (alpha <= 0.5)
        {
          if (alpha <= NoiseEpsilon)
            value=(double) pixel-MaxRGB;
          else
            value=(double) pixel+SigmaLaplacian*log(2.0*alpha);
          break;
        }
      beta=1.0-alpha;
      if (beta <= (0.5*NoiseEpsilon))
        value=(double) pixel+MaxRGB;
      else
        value=(double) pixel-SigmaLaplacian*log(2.0*beta);
      break;
    }
    case PoissonNoise:
    {
      register long
        i;

      for (i=0; alpha > exp(-SigmaPoisson*pixel); i++)
      {
        beta=(double) rand()/RAND_MAX;
        alpha=alpha*beta;
      }
      value=i/SigmaPoisson;
      break;
    }
  }
  if (value < 0.0)
    return(0);
  if (value > MaxRGB)
    return(MaxRGB);
  return((Quantum) (value+0.5));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t O p t i m a l K e r n e l W i d t h                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method GetOptimalKernelWidth computes the optimal kernel radius for a
%  convolution filter.  Start with the minimum value of 3 pixels and walk out
%  until we drop below the threshold of one pixel numerical accuracy,
%
%  The format of the GetOptimalKernelWidth method is:
%
%      int GetOptimalKernelWidth(const double radius,const double sigma)
%
%  A description of each parameter follows:
%
%    o width: Method GetOptimalKernelWidth returns the optimal width of
%      a convolution kernel.
%
%    o radius: The radius of the Gaussian, in pixels, not counting the center
%      pixel.
%
%    o sigma: The standard deviation of the Gaussian, in pixels.
%
%
*/

MagickExport int GetOptimalKernelWidth1D(const double radius,const double sigma)
{
  double
    normalize,
    value;

  long
    width;

  register long
    u;

  if (radius > 0.0)
    return((int) (2.0*ceil(radius)+1.0));
  for (width=5; ;)
  {
    normalize=0.0;
    for (u=(-width/2); u <= (width/2); u++)
      normalize+=exp(-((double) u*u)/(2.0*sigma*sigma))/(MagickSQ2PI*sigma);
    u=width/2;
    value=exp(-((double) u*u)/(2.0*sigma*sigma))/(MagickSQ2PI*sigma)/normalize;
    if ((long) (MaxRGB*value) <= 0)
      break;
    width+=2;
  }
  return(width-2);
}

MagickExport int GetOptimalKernelWidth2D(const double radius,const double sigma)
{
  double
    alpha,
    normalize,
    value;

  long
    width;

  register long
    u,
    v;

  if (radius > 0.0)
    return((int) (2.0*ceil(radius)+1.0));
  for (width=5; ;)
  {
    normalize=0.0;
    for (v=(-width/2); v <= (width/2); v++)
    {
      for (u=(-width/2); u <= (width/2); u++)
      {
        alpha=exp(-((double) u*u+v*v)/(2.0*sigma*sigma));
        normalize+=alpha/(2.0*MagickPI*sigma*sigma);
      }
    }
    v=width/2;
    value=exp(-((double) v*v)/(2.0*sigma*sigma))/(MagickSQ2PI*sigma)/normalize;
    if ((long) (MaxRGB*value) <= 0)
      break;
    width+=2;
  }
  return(width-2);
}

MagickExport int GetOptimalKernelWidth(const double radius,const double sigma)
{
  return(GetOptimalKernelWidth1D(radius,sigma));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   H S L T r a n s f o r m                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method HSLTransform converts a (hue, saturation, luminosity) to a
%  (red, green, blue) triple.
%
%  The format of the HSLTransformImage method is:
%
%      void HSLTransform(const double hue,const double saturation,
%        const double luminosity,Quantum *red,Quantum *green,Quantum *blue)
%
%  A description of each parameter follows:
%
%    o hue, saturation, luminosity: A double value representing a
%      component of the HSL color space.
%
%    o red, green, blue: A pointer to a pixel component of type Quantum.
%
%
*/
MagickExport void HSLTransform(const double hue,const double saturation,
  const double luminosity,Quantum *red,Quantum *green,Quantum *blue)
{
  /*
    Convert HSL to RGB colorspace.
  */
  assert(red != (Quantum *) NULL);
  assert(green != (Quantum *) NULL);
  assert(blue != (Quantum *) NULL);
  if (saturation == 0.0)
    {
      *red=*green=*blue=(Quantum) (MaxRGB*luminosity+0.5);
    }
  else
    {
      double
        b,
        g,
        r,
        v,
        x,
        y,
        z,
        hue_times_six,
        hue_fract,
        vsf;

      int
        sextant;

      v=(luminosity <= 0.5) ? (luminosity*(1.0+saturation)) :
        (luminosity+saturation-luminosity*saturation);

      hue_times_six=6.0*hue;
      sextant=(int)hue_times_six;
      hue_fract=hue_times_six-(double) sextant;

      y=luminosity+luminosity-v;
      vsf=(v-y)*hue_fract;
      x=y+vsf;
      z=v-vsf;
      switch (sextant)
        {
        case 0: r=v; g=x; b=y; break;
        case 1: r=z; g=v; b=y; break;
        case 2: r=y; g=v; b=x; break;
        case 3: r=y; g=z; b=v; break;
        case 4: r=x; g=y; b=v; break;
        case 5: r=v; g=y; b=z; break;
        default: r=v; g=x; b=y; break;
        }
      *red=(Quantum) (MaxRGB*r+0.5);
      *green=(Quantum) (MaxRGB*g+0.5);
      *blue=(Quantum) (MaxRGB*b+0.5);
    }
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   H W B T r a n s f o r m                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method HWBTransform converts a (hue, whiteness, blackness) to a
%  (red, green, blue) triple.
%
%  Algorithm derived from "HWB: A more intuitive hue-based color model."
%  Alvy Ray Smith and Eric Ray Lyons, Journal of Graphics Tools, Volume 1
%  Number 1, 1996.  http://www.acm.org/jgt/papers/SmithLyons96/
%
%  The format of the HWBTransformImage method is:
%
%      void HWBTransform(const double hue,const double whiteness,
%        const double blackness,Quantum *red,Quantum *green,Quantum *blue)
%
%  A description of each parameter follows:
%
%    o hue, whiteness, blackness: A double value representing a
%      component of the HWB color space.
%
%    o red, green, blue: A pointer to a pixel component of type Quantum.
%
%
*/
MagickExport void HWBTransform(const double hue,const double whiteness,
  const double blackness,Quantum *red,Quantum *green,Quantum *blue)
{
  double
    b,
    f,
    g,
    n,
    r,
    v;

  register long
    i;

  /*
    Convert HWB to RGB colorspace.
  */
  assert(red != (Quantum *) NULL);
  assert(green != (Quantum *) NULL);
  assert(blue != (Quantum *) NULL);
  v=1.0-blackness;
  if (hue == 0.0)
    {
      *red=(Quantum) (MaxRGB*v+0.5);
      *green=(Quantum) (MaxRGB*v+0.5);
      *blue=(Quantum) (MaxRGB*v+0.5);
      return;
    }
  i=(long) floor(hue);
  f=hue-i;
  if (i & 0x01)
    f=1.0-f;
  n=whiteness+f*(v-whiteness);  /* linear interpolation */
  switch (i)
  {
    default:
    case 6:
    case 0: r=v; g=n; b=whiteness; break;
    case 1: r=n; g=v; b=whiteness; break;
    case 2: r=whiteness; g=v; b=n; break;
    case 3: r=whiteness; g=n; b=v; break;
    case 4: r=n; g=whiteness; b=v; break;
    case 5: r=v; g=whiteness; b=n; break;
  }
  *red=(Quantum) (MaxRGB*r+0.5);
  *green=(Quantum) (MaxRGB*g+0.5);
  *blue=(Quantum) (MaxRGB*b+0.5);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   H u l l                                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method Hull implements the eight hull algorithm described in Applied
%  Optics, Vol. 24, No. 10, 15 May 1985, "Geometric filter for Speckle
%  Reduction", by Thomas R Crimmins.  Each pixel in the image is replaced by
%  one of its eight of its surrounding pixels using a polarity and negative
%  hull function.
%
%  The format of the Hull method is:
%
%      void Hull(const long x_offset,const long y_offset,
%        const unsigned long columns,const unsigned long rows,Quantum *f,
%        Quantum *g,const int polarity)
%
%  A description of each parameter follows:
%
%    o x_offset, y_offset: An integer value representing the offset of the
%      current pixel within the image.
%
%    o columns, rows: Specifies the number of rows and columns in the image.
%
%    o polarity: An integer value declaring the polarity (+,-).
%
%    o f, g: A pointer to an image pixel and one of it's neighbor.
%
%
*/
MagickExport void Hull(const long x_offset,const long y_offset,
  const unsigned long columns,const unsigned long rows,Quantum *f,Quantum *g,
  const int polarity)
{
  double
    v,
    y;

  register long
    x;

  register Quantum
    *p,
    *q,
    *r,
    *s;

  assert(f != (Quantum *) NULL);
  assert(g != (Quantum *) NULL);
  p=f+(columns+2);
  q=g+(columns+2);
  r=p+(y_offset*((long) columns+2)+x_offset);
  for (y=0; y < (long) rows; y++)
  {
    p++;
    q++;
    r++;
    if (polarity > 0)
      for (x=(long) columns; x > 0; x--)
      {
        v=(*p);
        if (*r >= (v+ScaleCharToQuantum(2)))
          v+=ScaleCharToQuantum(1);
        *q=(Quantum) v;
        p++;
        q++;
        r++;
      }
    else
      for (x=(long) columns; x > 0; x--)
      {
        v=(*p);
        if (*r <= (v-(long) ScaleCharToQuantum(2)))
          v-=(long) ScaleCharToQuantum(1);
        *q=(Quantum) v;
        p++;
        q++;
        r++;
      }
    p++;
    q++;
    r++;
  }
  p=f+(columns+2);
  q=g+(columns+2);
  r=q+(y_offset*((long) columns+2)+x_offset);
  s=q-(y_offset*((long) columns+2)+x_offset);
  for (y=0; y < (long) rows; y++)
  {
    p++;
    q++;
    r++;
    s++;
    if (polarity > 0)
      for (x=(long) columns; x > 0; x--)
      {
        v=(*q);
        if ((*s >= (v+ScaleCharToQuantum(2))) && (*r > v))
          v+=ScaleCharToQuantum(1);
        *p=(Quantum) v;
        p++;
        q++;
        r++;
        s++;
      }
    else
      for (x=(long) columns; x > 0; x--)
      {
        v=(*q);
        if ((*s <= (v-(long) ScaleCharToQuantum(2))) && (*r < v))
          v-=(long) ScaleCharToQuantum(1);
        *p=(Quantum) v;
        p++;
        q++;
        r++;
        s++;
      }
    p++;
    q++;
    r++;
    s++;
  }
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I d e n t i t y A f f i n e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method IdentityAffine initializes the affine transform to the identity
%  matrix.
%
%  The format of the IdentityAffine method is:
%
%      IdentityAffine(AffineMatrix *affine)
%
%  A description of each parameter follows:
%
%    o affine: A pointer the the affine transform of type AffineMatrix.
%
%
*/
MagickExport void IdentityAffine(AffineMatrix *affine)
{
  assert(affine != (AffineMatrix *) NULL);
  (void) memset(affine,0,sizeof(AffineMatrix));
  affine->sx=1.0;
  affine->sy=1.0;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I n t e r p o l a t e C o l o r                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method InterpolateColor applies bi-linear interpolation between a pixel and
%  it's neighbors.
%
%  The format of the InterpolateColor method is:
%
%      PixelPacket InterpolateColor(const Image *image,const double x_offset,
%        const double y_offset,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o x_offset,y_offset: A double representing the current (x,y) position of
%      the pixel.
%
%
*/
MagickExport PixelPacket InterpolateColor(const Image *image,
  const double x_offset,const double y_offset,ExceptionInfo *exception)
{
  double
    alpha,
    beta,
    one_minus_alpha,
    one_minus_beta;

  PixelPacket
    color;

  register const PixelPacket
    *p;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  p=AcquireImagePixels(image,(long) x_offset,(long) y_offset,2,2,exception);
  if (p == (const PixelPacket *) NULL)
    return(AcquireOnePixel(image,(long) x_offset,(long) y_offset,exception));
  alpha=x_offset-floor(x_offset);
  beta=y_offset-floor(y_offset);
  one_minus_alpha=1.0-alpha;
  one_minus_beta=1.0-beta;
  color.red=(Quantum) (one_minus_beta*(one_minus_alpha*p[0].red+
    alpha*p[1].red)+beta*(one_minus_alpha*p[2].red+alpha*p[3].red)+0.5);
  color.green=(Quantum) (one_minus_beta*(one_minus_alpha*p[0].green+
    alpha*p[1].green)+beta*(one_minus_alpha*p[2].green+alpha*p[3].green)+0.5);
  color.blue=(Quantum) (one_minus_beta*(one_minus_alpha*p[0].blue+
    alpha*p[1].blue)+beta*(one_minus_alpha*p[2].blue+alpha*p[3].blue)+0.5);
  color.opacity=(Quantum) (one_minus_beta*(one_minus_alpha*p[0].opacity+
    alpha*p[1].opacity)+beta*(one_minus_alpha*p[2].opacity+alpha*p[3].opacity)+0.5);
  return(color);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M o d u l a t e                                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method Modulate modulates the hue, saturation, and brightness of an
%  image.
%
%  The format of the Modulate method is:
%
%      void Modulate(const double percent_hue,const double percent_saturation,
%        const double percent_brightness,Quantum *red,Quantum *green,
%        Quantum *blue)
%
%  A description of each parameter follows:
%
%    o percent_hue, percent_saturation, percent_brightness: A double value
%      representing the percent change in a component of the HSL color space.
%
%    o red, green, blue: A pointer to a pixel component of type Quantum.
%
%
*/
MagickExport void Modulate(const double percent_hue,
  const double percent_saturation,const double percent_brightness,
  Quantum *red,Quantum *green,Quantum *blue)
{
  double
    brightness,
    hue,
    saturation;

  /*
    Increase or decrease color brightness, saturation, or hue.
  */
  assert(red != (Quantum *) NULL);
  assert(green != (Quantum *) NULL);
  assert(blue != (Quantum *) NULL);
  TransformHSL(*red,*green,*blue,&hue,&saturation,&brightness);
  brightness*=(0.01+MagickEpsilon)*percent_brightness;
  if (brightness > 1.0)
    brightness=1.0;
  saturation*=(0.01+MagickEpsilon)*percent_saturation;
  if (saturation > 1.0)
    saturation=1.0;
  hue*=(0.01+MagickEpsilon)*percent_hue;
  if (hue > 1.0)
    hue-=1.0;
  HSLTransform(hue,saturation,brightness,red,green,blue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   T r a n s f o r m H S L                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method TransformHSL converts a (red, green, blue) to a (hue, saturation,
%  luminosity) triple.
%
%  The format of the TransformHSL method is:
%
%      void TransformHSL(const Quantum red,const Quantum green,
%        const Quantum blue,double *hue,double *saturation,double *luminosity)
%
%  A description of each parameter follows:
%
%    o red, green, blue: A Quantum value representing the red, green, and
%      blue component of a pixel..
%
%    o hue, saturation, luminosity: A pointer to a double value representing a
%      component of the HSL color space.
%
%
*/
MagickExport void TransformHSL(const Quantum red,const Quantum green,
  const Quantum blue,double *hue,double *saturation,double *luminosity)
{
  double
    b,
    delta,
    g,
    max,
    min,
    r;

  /*
    Convert RGB to HSL colorspace.
  */
  assert(hue != (double *) NULL);
  assert(saturation != (double *) NULL);
  assert(luminosity != (double *) NULL);
  r=(double) red/MaxRGB;
  g=(double) green/MaxRGB;
  b=(double) blue/MaxRGB;
  max=Max(r,Max(g,b));
  min=Min(r,Min(g,b));
  *hue=0.0;
  *saturation=0.0;
  *luminosity=(min+max)/2.0;
  delta=max-min;
  if (delta == 0.0)
    return;
  *saturation=delta/((*luminosity <= 0.5) ? (min+max) : (2.0-max-min));
  if (r == max)
    *hue=(g == min ? 5.0+(max-b)/delta : 1.0-(max-g)/delta);
  else
    if (g == max)
      *hue=(b == min ? 1.0+(max-r)/delta : 3.0-(max-b)/delta);
    else
      *hue=(r == min ? 3.0+(max-g)/delta : 5.0-(max-r)/delta);
  *hue/=6.0;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   T r a n s f o r m H W B                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method TransformHWB converts a (red, green, blue) to a
%  (hue, whiteness, blackness) triple.
%
%  Algorithm derived from "HWB: A more intuitive hue-based color model."
%  Alvy Ray Smith and Eric Ray Lyons, Journal of Graphics Tools, Volume 1
%  Number 1, 1996.  http://www.acm.org/jgt/papers/SmithLyons96/
%
%  The format of the TransformHWB method is:
%
%      void TransformHWB(const Quantum red,const Quantum green,
%        const Quantum blue,double *hue,double *whiteness,double *blackness)
%
%  A description of each parameter follows:
%
%    o red, green, blue: A Quantum value representing the red, green, and
%      blue component of a pixel.
%
%    o hue, whiteness, blackness: A pointer to a double value representing a
%      component of the HWB color space.
%
%
*/
MagickExport void TransformHWB(const Quantum red,const Quantum green,
  const Quantum blue,double *hue,double *whiteness,double *blackness)
{
  double
    f;

  register long
    i;

  Quantum
    v,
    w;

  /*
    Convert RGB to HWB colorspace.
  */
  assert(hue != (double *) NULL);
  assert(whiteness != (double *) NULL);
  assert(blackness != (double *) NULL);
  w=Min(red,Min(green,blue));
  v=Max(red,Max(green,blue));
  *blackness=(double) (MaxRGB-v)/MaxRGB;
  if (v == w)
    {
      *hue=0.0;
      *whiteness=1.0-(*blackness);
      return;
    }
  f=(red == w) ? (double) green-blue : ((green == w) ? (double) blue-red :
    (double) red-green);
  i=(red == w) ? 3 : ((green == w) ? 5 : 1);
  *hue=i-f/(v-w);
  *whiteness=(double) w/MaxRGB;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   U p s a m p l e                                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Upsample() doubles the size of the image.
%
%  The format of the Upsample method is:
%
%      void Upsample(const unsigned long width,const unsigned long height,
%        const unsigned long scaled_width,unsigned char *pixels)
%
%  A description of each parameter follows:
%
%    o width,height:  Unsigned values representing the width and height of
%      the image pixel array.
%
%    o scaled_width:  Specifies the final width of the upsampled pixel array.
%
%    o pixels:  An unsigned char containing the pixel data.  On output the
%      upsampled pixels are returns here.
%
%
*/
MagickExport void Upsample(const unsigned long width,const unsigned long height,
  const unsigned long scaled_width,unsigned char *pixels)
{
  register long
    x,
    y;

  register unsigned char
    *p,
    *q,
    *r;

  /*
    Create a new image that is a integral size greater than an existing one.
  */
  assert(pixels != (unsigned char *) NULL);
  for (y=0; y < (long) height; y++)
  {
    p=pixels+(height-1-y)*scaled_width+(width-1);
    q=pixels+((height-1-y) << 1)*scaled_width+((width-1) << 1);
    *q=(*p);
    *(q+1)=(*(p));
    for (x=1; x < (long) width; x++)
    {
      p--;
      q-=2;
      *q=(*p);
      *(q+1)=(unsigned char) ((((long) *p)+((long) *(p+1))+1) >> 1);
    }
  }
  for (y=0; y < (long) (height-1); y++)
  {
    p=pixels+(y << 1)*scaled_width;
    q=p+scaled_width;
    r=q+scaled_width;
    for (x=0; x < (long) (width-1); x++)
    {
      *q=(unsigned char) ((((long) *p)+((long) *r)+1) >> 1);
      *(q+1)=(unsigned char)
        ((((long) *p)+((long) *(p+2))+((long) *r)+((long) *(r+2))+2) >> 2);
      q+=2;
      p+=2;
      r+=2;
    }
    *q++=(unsigned char) ((((long) *p++)+((long) *r++)+1) >> 1);
    *q++=(unsigned char) ((((long) *p++)+((long) *r++)+1) >> 1);
  }
  p=pixels+(2*height-2)*scaled_width;
  q=pixels+(2*height-1)*scaled_width;
  (void) memcpy(q,p,2*width);
}
