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
%                  SSSSS  TTTTT   AAA   TTTTT  IIIII   CCCC                   %
%                  SS       T    A   A    T      I    C                       %
%                   SSS     T    AAAAA    T      I    C                       %
%                     SS    T    A   A    T      I    C                       %
%                  SSSSS    T    A   A    T    IIIII   CCCC                   %
%                                                                             %
%                                                                             %
%                        GraphicsMagick Static Methods                        %
%                                                                             %
%                                                                             %
%                              Software Design                                %
%                                John Cristy                                  %
%                                 March 2000                                  %
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
#include "magick/module.h"
#include "magick/static.h"
#include "magick/utility.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   E x e c u t e S t a t i c M o d u l e P r o c e s s                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ExecuteStaticModuleProcess() is just a template method. This version is
%  only used for static and moby builds.  See the version in module.c for
%  module-only builds. This function takes the place of ExecuteModuleProcess
%  for builds which do not support loading modules.
%
%  The format of the ExecuteStaticModuleProcess method is:
%
%      unsigned int ExecuteStaticModuleProcess(const char *tag,
%        Image **image,const int argc,char **argv)
%
%  A description of each parameter follows:
%
%    o tag: The module tag.
%
%    o image: The image.
%
%    o argc: The number of elements in the argument vector.
%
%    o argv: A text array containing the command line arguments.
%
*/
#if defined(SupportMagickModules)
MagickExport unsigned int ExecuteStaticModuleProcess(const char *tag,
  Image **image,const int argc,char **argv)
#else
MagickExport unsigned int ExecuteModuleProcess(const char *tag,
  Image **image,const int argc,char **argv)
#endif /* defined(SupportMagickModules) */
{
  unsigned int
    status = False;

#if !defined(BuildMagickModules)
  unsigned int
    (*method)(Image **,const int,char **) = 0;

  if (LocaleCompare("analyze",tag) == 0)
    method=AnalyzeImage;

  if (method)
    {
      LogMagickEvent(CoderEvent,GetMagickModule(),
        "Invoking \"%.1024s\" filter module",tag);

      status=(*method)(image,argc,argv);

      LogMagickEvent(CoderEvent,GetMagickModule(),
        "Returned from \"%.1024s\" filter module",tag);
    }

#endif /* !defined(BuildMagickModules) */
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e g i s t e r S t a t i c M o d u l e s                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RegisterStaticModules() statically registers all the available module
%  handlers.
%
%  The format of the RegisterStaticModules method is:
%
%      RegisterStaticModules(void)
%
%
*/
MagickExport void RegisterStaticModules(void)
{
#if !defined(BuildMagickModules)
  RegisterARTImage();
  RegisterAVIImage();
  RegisterAVSImage();
  RegisterBMPImage();
  RegisterCAPTIONImage();
  RegisterCINEONImage();
  RegisterCLIPBOARDImage();
  RegisterCMYKImage();
  RegisterCUTImage();
  RegisterDCMImage();
  RegisterDIBImage();
  RegisterDPSImage();
  RegisterDPXImage();
  RegisterEMFImage();
  RegisterEPTImage();
  RegisterFAXImage();
  RegisterFITSImage();
  RegisterFPXImage();
  RegisterGIFImage();
  RegisterGRAYImage();
  RegisterGRADIENTImage();
  RegisterHISTOGRAMImage();
  RegisterHTMLImage();
  RegisterICONImage();
  RegisterJBIGImage();
  RegisterJPEGImage();
  RegisterJP2Image();
  RegisterLABELImage();
  RegisterLOCALEImage();
  RegisterLOGOImage();
  RegisterMAPImage();
  RegisterMATImage();
  RegisterMATTEImage();
  RegisterMETAImage();
  RegisterMIFFImage();
  RegisterMONOImage();
  RegisterMPCImage();
  RegisterMPEGImage();
  RegisterMPRImage();
  RegisterMSLImage();
  RegisterMTVImage();
  RegisterMVGImage();
  RegisterNULLImage();
  RegisterOTBImage();
  RegisterPALMImage();
  RegisterPCDImage();
  RegisterPCLImage();
  RegisterPCXImage();
  RegisterPDBImage();
  RegisterPDFImage();
  RegisterPICTImage();
  RegisterPIXImage();
  RegisterPLASMAImage();
  RegisterPNGImage();
  RegisterPNMImage();
  RegisterPREVIEWImage();
  RegisterPSImage();
  RegisterPS2Image();
  RegisterPS3Image();
  RegisterPSDImage();
  RegisterPWPImage();
  RegisterRGBImage();
  RegisterRLAImage();
  RegisterRLEImage();
  RegisterSCTImage();
  RegisterSFWImage();
  RegisterSGIImage();
  RegisterSTEGANOImage();
  RegisterSUNImage();
  RegisterSVGImage();
  RegisterTGAImage();
  RegisterTIFFImage();
  RegisterTILEImage();
  RegisterTIMImage();
  RegisterTOPOLImage();
  RegisterTTFImage();
  RegisterTXTImage();
  RegisterUILImage();
  RegisterURLImage();
  RegisterUYVYImage();
  RegisterVICARImage();
  RegisterVIDImage();
  RegisterVIFFImage();
  RegisterWBMPImage();
  RegisterWMFImage();
  RegisterWPGImage();
  RegisterXImage();
  RegisterXBMImage();
  RegisterXCImage();
  RegisterXCFImage();
  RegisterXPMImage();
#if defined(_VISUALC_)
  RegisterXTRNImage();
#endif /* defined(_VISUALC_) */
  RegisterXWDImage();
  RegisterYUVImage();
#endif /* !defined(BuildMagickModules) */
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   U n r e g i s t e r S t a t i c M o d u l e s                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  UnregisterStaticModules() statically unregisters all the available module
%  handlers. This allows allocated resources to be freed.
%
%  The format of the UnRegisterStaticModules method is:
%
%      UnRegisterStaticModules(void)
%
%
*/
MagickExport void UnregisterStaticModules(void)
{
#if !defined(BuildMagickModules)
  UnregisterARTImage();
  UnregisterAVIImage();
  UnregisterAVSImage();
  UnregisterBMPImage();
  UnregisterCAPTIONImage();
  UnregisterCINEONImage();
  UnregisterCLIPBOARDImage();
  UnregisterCMYKImage();
  UnregisterCUTImage();
  UnregisterDCMImage();
  UnregisterDIBImage();
  UnregisterDPSImage();
  UnregisterDPXImage();
  UnregisterEMFImage();
  UnregisterEPTImage();
  UnregisterFAXImage();
  UnregisterFITSImage();
  UnregisterFPXImage();
  UnregisterGIFImage();
  UnregisterGRAYImage();
  UnregisterGRADIENTImage();
  UnregisterHISTOGRAMImage();
  UnregisterHTMLImage();
  UnregisterICONImage();
  UnregisterJBIGImage();
  UnregisterJPEGImage();
  UnregisterJP2Image();
  UnregisterLABELImage();
  UnregisterLOCALEImage();
  UnregisterLOGOImage();
  UnregisterMAPImage();
  UnregisterMATImage();
  UnregisterMATTEImage();
  UnregisterMETAImage();
  UnregisterMIFFImage();
  UnregisterMONOImage();
  UnregisterMPCImage();
  UnregisterMPEGImage();
  UnregisterMPRImage();
  UnregisterMSLImage();
  UnregisterMTVImage();
  UnregisterMVGImage();
  UnregisterNULLImage();
  UnregisterOTBImage();
  UnregisterPALMImage();
  UnregisterPCDImage();
  UnregisterPCLImage();
  UnregisterPCXImage();
  UnregisterPDBImage();
  UnregisterPDFImage();
  UnregisterPICTImage();
  UnregisterPIXImage();
  UnregisterPLASMAImage();
  UnregisterPNGImage();
  UnregisterPNMImage();
  UnregisterPREVIEWImage();
  UnregisterPSImage();
  UnregisterPS2Image();
  UnregisterPS3Image();
  UnregisterPSDImage();
  UnregisterPWPImage();
  UnregisterRGBImage();
  UnregisterRLAImage();
  UnregisterRLEImage();
  UnregisterSCTImage();
  UnregisterSFWImage();
  UnregisterSGIImage();
  UnregisterSTEGANOImage();
  UnregisterSUNImage();
  UnregisterSVGImage();
  UnregisterTGAImage();
  UnregisterTIFFImage();
  UnregisterTILEImage();
  UnregisterTIMImage();
  UnregisterTOPOLImage();
  UnregisterTTFImage();
  UnregisterTXTImage();
  UnregisterUILImage();
  UnregisterURLImage();
  UnregisterUYVYImage();
  UnregisterVICARImage();
  UnregisterVIDImage();
  UnregisterVIFFImage();
  UnregisterWBMPImage();
  UnregisterWMFImage();
  UnregisterWPGImage();
  UnregisterXImage();
  UnregisterXBMImage();
  UnregisterXCImage();
  UnregisterXCFImage();
  UnregisterXPMImage();
#if defined(_VISUALC_)
  UnregisterXTRNImage();
#endif /* defined(_VISUALC_) */
  UnregisterXWDImage();
  UnregisterYUVImage();
#endif /* !defined(BuildMagickModules) */
}
