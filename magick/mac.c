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
%                            M   M   AAA    CCCC                              %
%                            MM MM  A   A  C                                  %
%                            M M M  AAAAA  C                                  %
%                            M   M  A   A  C                                  %
%                            M   M  A   A   CCCC                              %
%                                                                             %
%                                                                             %
%                  Macintosh Utility Methods for GraphicsMagick               %
%                                                                             %
%                                                                             %
%                               Software Design                               %
%                                 John Cristy                                 %
%                                September 1996                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  The directory methods are strongly based on similar methods written
%  by Steve Summit, scs@eskimo.com.  The Ghostscript launch code is strongly
%  based on Dave Schooley's Mac Gnuplot and contributed by
%  schindall@wave14i.nrl.navy.mil.  Mac-centric improvements contributed by
%  leonardr@digapp.com.
%
%
*/

#if defined(macintosh)
/*
  Include declarations.
*/
#define _X_H
#define _WIDGET_H
#include <AppleEvents.h>
#include <AERegistry.h>
#include <AEObjects.h>
#include <AEPackObject.h>
#include <Processes.h>
#include <QuickDraw.h>
#include <QDOffscreen.h>
#include <Palettes.h>
#include <ImageCompression.h>
#include <PictUtils.h>
#include <Files.h>
#include <Gestalt.h>
#include <TextUtils.h>
#define ColorInfo  KolorInfo
#include "magick/studio.h"
#include "magick/blob.h"
#include "magick/monitor.h"
#include "magick/utility.h"
#include "magick/mac.h"

/*
  Global declaractions.
*/
ImageDescriptionHandle
  image_description = nil;

/*
  Forward declaractions.
*/
static Boolean
  SearchForFile(OSType,OSType,FSSpec *,short);

static pascal void
  ArcMethod(GrafVerb,Rect *,short,short),
  BitsMethod(BitMap *,Rect *,Rect *,short,RgnHandle),
  FilenameToFSSpec(const char *filename,FSSpec *fsspec),
  LineMethod(Point),
  OvalMethod(GrafVerb,Rect *),
  PolyMethod(GrafVerb,PolyHandle),
  RRectMethod(GrafVerb,Rect *,short,short),
  RectMethod(GrafVerb,Rect *),
  RegionMethod(GrafVerb,RgnHandle),
  StandardPixmap(PixMapPtr,Rect *,MatrixRecordPtr,short,RgnHandle,PixMapPtr,
    Rect *,short),
  TextMethod(short,Ptr,Point,Point);

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   B o t t l e n e c k T e s t                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method BottleneckTest intercepts any compressed images.
%
%  The format of the BottleneckTest method is:
%
%      int BottleneckTest(const char *magick)
%
%  A description of each parameter follows:
%
%    o picture: Specifies a pointer to a PicHandle structure.
%
%    o codec: The code type is returned in this CodecType pointer structure.
%
%    o depth: The image depth is returned as an integer pointer.
%
%    o colormap_id: The colormap ID is returned in this short pointer.
%
%
*/

static pascal void ArcMethod(GrafVerb verb,Rect *r,short startAngle,
  short arcAngle)
{
#pragma unused (verb,r,startAngle,arcAngle)
}

static pascal void BitsMethod(BitMap *bitPtr,Rect *source_rectangle,
  Rect *dstRect,short mode,RgnHandle maskRgn)
{
#pragma unused (bitPtr,source_rectangle,dstRect,mode,maskRgn)
}

static pascal void LineMethod(Point newPt)
{
#pragma unused (newPt)
}

static pascal void OvalMethod(GrafVerb verb,Rect *r)
{
#pragma unused (verb,r)
}

static pascal void PolyMethod(GrafVerb verb,PolyHandle poly)
{
#pragma unused (verb,poly)
}

static pascal void RectMethod(GrafVerb verb,Rect *r)
{
#pragma unused (verb,r)
}

static pascal void RegionMethod(GrafVerb verb,RgnHandle rgn)
{
#pragma unused (verb,rgn)
}

static pascal void RRectMethod(GrafVerb verb,Rect *r,short ovalWidth,
  short ovalHeight)
{
#pragma unused (verb,r,ovalWidth,ovalHeight)
}

static pascal void StandardPixmap(PixMapPtr source,Rect *source_rectangle,
  MatrixRecordPtr matrix,short mode,RgnHandle mask,PixMapPtr matte,
  Rect *matte_rectangle,short flags)
{
#pragma unused (source_rectangle,matrix,mode,mask,matte,matte_rectangle,flags)

  long
    size;

  Ptr
    data;

  GetCompressedPixMapInfo(source,&image_description,&data,&size,nil,nil);
}

static pascal void TextMethod(short byteCount,Ptr textBuf,Point numer,
  Point denom)
{
#pragma unused (byteCount,textBuf,numer,denom)
}

static short BottleneckTest(PicHandle picture,CodecType *codec,int *depth,
  short *colormap_id)
{
  CQDProcs
    bottlenecks;

  int
    status;

  long
    version;

  Rect
    rectangle;

  status=Gestalt(gestaltQuickTime,&version);
  if (status != noErr)
    {
      ParamText("\pQuickTime not installed.  Please install, then try again.",
        "\p", "\p", "\p" );
      Alert(128,nil);
      return(-1);
    }
  /*
    Define our own bottlenecks to do nothing.
  */
  SetStdCProcs(&bottlenecks);
  bottlenecks.textProc=NewQDTextUPP(&TextMethod);
  bottlenecks.lineProc=NewQDLineUPP(&LineMethod);
  bottlenecks.rectProc=NewQDRectUPP(&RectMethod);
  bottlenecks.rRectProc=NewQDRRectUPP(&RRectMethod);
  bottlenecks.ovalProc=NewQDOvalUPP(&OvalMethod);
  bottlenecks.arcProc=NewQDArcUPP(&ArcMethod);
  bottlenecks.polyProc=NewQDPolyUPP(&PolyMethod);
  bottlenecks.rgnProc=NewQDRgnUPP(&RegionMethod);
  bottlenecks.bitsProc=NewQDBitsUPP(&BitsMethod);
  bottlenecks.newProc1=(UniversalProcPtr) NewStdPixUPP(&StandardPixmap);
  /*
    Install our custom bottlenecks to intercept any compressed images.
  */
  (*(qd.thePort)).grafProcs=(QDProcs *) &bottlenecks;
  DrawPicture(picture,&((**picture).picFrame));
  PaintRect(&rectangle);
  (*(qd.thePort)).grafProcs=0L;
  /*
    Initialize our return values.
  */
  *codec='unkn';
  *depth=0;
  *colormap_id=(-1);
  if (image_description != nil)
    {
      *codec=(**image_description).cType;
      *depth=(**image_description).depth;
      *colormap_id=(**image_description).clutID;
    }
  DisposeQDTextUPP(bottlenecks.textProc);
  DisposeQDLineUPP(bottlenecks.lineProc);
  DisposeQDRectUPP(bottlenecks.rectProc);
  DisposeQDRRectUPP(bottlenecks.rRectProc);
  DisposeQDOvalUPP(bottlenecks.ovalProc);
  DisposeQDArcUPP(bottlenecks.arcProc);
  DisposeQDPolyUPP(bottlenecks.polyProc);
  DisposeQDRgnUPP(bottlenecks.rgnProc);
  DisposeQDBitsUPP(bottlenecks.bitsProc);
  DisposeStdPixUPP(bottlenecks.newProc1);
  return(0);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   c l o s e d i r                                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method closedir closes the named directory stream and frees the DIR
%  structure.
%
%  The format of the closedir method is:
%
%      closedir(entry)
%
%  A description of each parameter follows:
%
%    o entry: Specifies a pointer to a DIR structure.
%
%
*/
MagickExport void closedir(DIR *entry)
{
  assert(entry != (DIR *) NULL);
  LiberateMemory((void **) &entry);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   E x i t                                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method Exit exits the process.
%
%  The format of the Exit method is:
%
%      Exit(status)
%
%  A description of each parameter follows:
%
%    o status: an integer value representing the status of the terminating
%      process.
%
%
*/
MagickExport int Exit(int status)
{
  (void) fprintf(stdout,"Select File->Quit to exit.\n");
  exit(status);
  return(0);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   F i l e n a m e T o F S S p e c                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method FilenameToFSSpec sets the file type of an image.
%
%  The format of the FilenameToFSSpec method is:
%
%      FilenameToFSSpec(filename,fsspec)
%
%  A description of each parameter follows:
%
%    o filename: Specifies the name of the file.
%
%    o fsspec: A pointer to type FSSpec.
%
%
*/
MagickExport void pascal FilenameToFSSpec(const char *filename,FSSpec *fsspec)
{
  Str255
    name;

  assert(filename != (char *) NULL);
  (void) strncpy((char *) name,filename,254);
  c2pstr((char *) name);
  FSMakeFSSpec(0,0,name,fsspec);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I s M a g i c k C o n f l i c t                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method MACIsMagickConflict returns true if the image format conflicts with
%  a logical drive (.e.g. X:).
%
%  Contributed by Mark Gavin of Digital Applications, Inc.
%
%  The format of the MACIsMagickConflict method is:
%
%      status=MACIsMagickConflict(magick)
%
%  A description of each parameter follows:
%
%    o status: Method MACIsMagickConflict returns true if the image format
%      conflicts with a logical drive.
%
%    o magick: Specifies the image format.
%
%
*/

static OSErr HGetVInfo(short volume_index,StringPtr volume_name,short *volume,
  unsigned long *free_bytes,unsigned long *total_bytes)
{
  HParamBlockRec
    pb;

  OSErr
    result;

  unsigned long
    blocksize;

  unsigned short
    allocation_blocks,
    free_blocks;

  /*
    Use the File Manager to get the real vRefNum.
  */
  pb.volumeParam.ioVRefNum=0;
  pb.volumeParam.ioNamePtr=volume_name;
  pb.volumeParam.ioVolIndex=volume_index;
  result=PBHGetVInfoSync(&pb);
  if (result != noErr)
    return(result);
  *volume=pb.volumeParam.ioVRefNum;
  blocksize=(unsigned long) pb.volumeParam.ioVAlBlkSiz;
  allocation_blocks=(unsigned short) pb.volumeParam.ioVNmAlBlks;
  free_blocks=(unsigned short) pb.volumeParam.ioVFrBlk;
  *free_bytes=free_blocks*blocksize;
  *total_bytes=allocation_blocks*blocksize;
  return(result);
}

MagickExport unsigned int MACIsMagickConflict(const char *magick)
{
  unsigned long
    free_bytes,
    number_bytes;

  OSErr
    status;

  short
    volume,
    index;

  Str255
    volume_name;

  StringPtr
    p;

  assert(magick != (char *) NULL);
  p=(StringPtr) &volume_name;
  for (index=(-1); ; index--)
  {
    status=HGetVInfo(index,p,&volume,&free_bytes,&number_bytes);
    if (status)
      return(False);
    if (LocaleCompare(p2cstr(p),magick) == 0)
      return(True);
  }
  return(False);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   M A C E r r o r H a n d l e r                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method MACErrorHandler displays an error reason and then terminates
%  the program.
%
%  The format of the MACErrorHandler method is:
%
%      void MACErrorHandler(const ExceptionType error,const char *reason,
%        const char *description)
%
%  A description of each parameter follows:
%
%    o exception: Specifies the numeric error category.
%
%    o reason: Specifies the reason to display before terminating the
%      program.
%
%    o description: Specifies any description to the reason.
%
%
*/
MagickExport void MACErrorHandler(const ExceptionType error,const char *reason,
  const char *description)
{
  char
    buffer[3*MaxTextExtent];

  if (reason == (char *) NULL)
    {
      DestroyMagick();
      Exit(0);
    }
  if ((description != (char *) NULL) && errno)
    FormatString(buffer,"%.1024s: %.1024s (%.1024s) [%.1024s].\n",
      SetClientName((char *) NULL),reason,description,strerror(errno));
  else
    if (description != (char *) NULL)
      FormatString(buffer,"%.1024s: %.1024s (%.1024s).\n",
        SetClientName((char *) NULL),reason,description);
    else
      if (errno)
        FormatString(buffer,"%.1024s: %.1024s [%.1024s].\n",
          SetClientName((char *) NULL),reason,strerror(errno));
      else
        FormatString(buffer,"%.1024s: %.1024s.\n",SetClientName((char *) NULL),
          reason);
  puts(buffer);
  DestroyMagick();
  Exit(0);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a c G S E x e c u t e C o m m a n d                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method MacGSExecuteCommand executes the Ghostscript command.
%
%
*/
static OSErr MacGSExecuteCommand(const char *command,long length)
{
  AEAddressDesc
    event_descriptor;

  AEDesc
    reply = {typeNull, NULL};

  AppleEvent
    event = {typeNull, NULL};

  DescType
    descriptor_type;

  int
    error;

  OSType
    id = 'gsVR';

  Size
    actualSize;

  /*
    Send the Apple Event.
  */
  (void) AECreateDesc(typeApplSignature,&id,sizeof(id),&event_descriptor);
  (void) AECreateAppleEvent(id,'exec',&event_descriptor,-1,kAnyTransactionID,
    &event);
  (void) AEPutParamPtr(&event,keyDirectObject,typeChar,command,length);
  (void) AESend(&event,&reply,kAEWaitReply+kAENeverInteract,kAENormalPriority,
    kNoTimeOut,NULL,NULL);
  /*
    Handle the reply and exit.
  */
  (void) AEGetParamPtr(&reply,keyDirectObject,typeInteger,&descriptor_type,
    &error,sizeof(error),&actualSize);
  (void) AEDisposeDesc(&event_descriptor);
  (void) AEDisposeDesc(&event);
  if (reply.descriptorType != NULL)
    AEDisposeDesc(&reply);
  return((OSErr) error);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a c G S L a u n c h A p p l i c a t i o n C o r e                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method MacGSLaunchApplicationCore launches the Ghostscript command.
%
%
*/
static OSErr MacGSLaunchApplicationCore(long flags)
{
  FSSpec
    file_info;

  LaunchParamBlockRec
    launch_info;

  OSErr
    error;

  if (!SearchForFile('gsVR','APPL',&file_info,1))
    return(-43);
  launch_info.launchBlockID=extendedBlock;
  launch_info.launchEPBLength=extendedBlockLen;
  launch_info.launchFileFlags=0;
  launch_info.launchControlFlags=launchContinue+launchNoFileFlags+flags;
  launch_info.launchAppSpec=(&file_info);
  launch_info.launchAppParameters=nil;
  error=LaunchApplication(&launch_info);
  return(error);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a c G S L a u n c h A p p l i c a t i o n                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method MacGSLaunchApplication launches the Ghostscript command.
%
%
*/
static OSErr MacGSLaunchApplication(void)
{
  return(MacGSLaunchApplicationCore(launchDontSwitch));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a c G S L a u n c h A p p l i c a t i o n T o F r o n t                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method MacGSLaunchApplicationToFront moves the Ghostscript window to the
%  front.
%
%
*/
static OSErr MacGSLaunchApplicationToFront(void)
{
  return(MacGSLaunchApplicationCore(0));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a c G S Q u i t A p p l i c a t i o n                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method MacGSQuitApplication quits the Ghostscript application.
%
%
*/
static void MacGSQuitApplication(void)
{
  AEAddressDesc
    event_descriptor;

  AEDesc
    reply = {typeNull, NULL};

  AppleEvent
    event = {typeNull, NULL};

  OSType
    id = 'GPLT';

  /*
    Send the Apple Event.
  */
  (void) AECreateDesc(typeApplSignature,&id,sizeof(id),&event_descriptor);
  (void) AECreateAppleEvent(typeAppleEvent,kAEQuitApplication,
    &event_descriptor,-1,kAnyTransactionID,&event);
  (void) AESend(&event,&reply,kAENoReply,kAENormalPriority,kNoTimeOut,NULL,
    NULL);
  /*
    Clean up and exit.
  */
  (void) AEDisposeDesc(&event_descriptor);
  (void) AEDisposeDesc(&event);
  if (reply.descriptorType != NULL)
    AEDisposeDesc(&reply);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M a c G S S e t W o r k i n g F o l d e r                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method MacGSSetWorkingFolder set the Ghostscript working folder.
%
%
*/
static OSErr MacGSSetWorkingFolder(char *directory)
{
  AEDesc
    application_descriptor,
    event_descriptor,
    object,
    path_descriptor,
    type_descriptor,
    reply;

  AppleEvent
    event;

  DescType
    folder_type = 'wfdr';

  OSErr
    error;

  OSType
    id = 'GPLT';

  /*
    Send the Apple Event.
  */
  AECreateDesc(typeNull,NULL,0,&application_descriptor);
  AECreateDesc(typeChar,directory,strlen(directory),&path_descriptor);
  (void) AECreateDesc(typeType,&folder_type,sizeof(DescType),&type_descriptor);
  CreateObjSpecifier(cProperty,&application_descriptor,formPropertyID,
    &type_descriptor,0,&object);
  (void) AECreateDesc(typeApplSignature,&id,sizeof(id),&event_descriptor);
  (void) AECreateAppleEvent(kAECoreSuite,kAESetData,&event_descriptor,-1,
    kAnyTransactionID,&event);
  (void) AEPutParamDesc(&event,keyDirectObject,&object);
  (void) AEPutParamDesc(&event,keyAEData,&path_descriptor);
  error=AESend(&event,&reply,kAENoReply+kAENeverInteract,kAENormalPriority,
    kNoTimeOut,NULL,NULL);
  (void) AEDisposeDesc(&event);
  (void) AEDisposeDesc(&event_descriptor);
  (void) AEDisposeDesc(&object);
  (void) AEDisposeDesc(&type_descriptor);
  (void) AEDisposeDesc(&path_descriptor);
  (void) AEDisposeDesc(&application_descriptor);
  return(error);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M A C S y s t e m C o m m a n d                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%   Method MACSystemCommand executes the specified command and waits until it
%   terminates.  The returned value is the exit status of the command.
%
%  The format of the MACSystemCommand method is:
%
%      int MACSystemCommand(const char * command)
%
%  A description of each parameter follows:
%
%    o command: This string is the command to execute.
%
%
*/
MagickExport int MACSystemCommand(const char * command)
{
  /*
    We only know how to launch Ghostscript.
  */
  if (MacGSLaunchApplicationToFront())
    return(-1);
  return(MacGSExecuteCommand(command,strlen(command)));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M A C W a r n i n g H a n d l e r                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method MACWarningHandler displays a warning reason.
%
%  The format of the MACWarningHandler method is:
%
+      void MACWarningHandler(const ExceptionType warning,const char *reason,
%        const char *description)
%
%  A description of each parameter follows:
%
%    o warning: Specifies the numeric warning category.
%
%    o reason: Specifies the reason to display before terminating the
%      program.
%
%    o description: Specifies any description to the reason.
%
%
*/
MagickExport void MACWarningHandler(const ExceptionType warning,
  const char *reason,const char *description)
{
  char
    buffer[1664];

  if (reason == (char *) NULL)
    return;
  if (description == (char *) NULL)
    FormatString(buffer,"%.1024s: %.1024s.\n",
      SetClientName((char *) NULL),reason);
  else
    FormatString(buffer,"%.1024s: %.1024s (%.1024s).\n",
      SetClientName((char *) NULL),reason,description);
  puts(buffer);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   o p e n d i r                                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method opendir opens the directory named by filename and associates a
%  directory stream with it.
%
%  The format of the opendir method is:
%
%      MagickExport DIR *opendir(char *path)
%
%  A description of each parameter follows:
%
%    o entry: Specifies a pointer to a DIR structure.
%
%
*/
MagickExport DIR *opendir(const char *path)
{
  char
    pathname[MaxTextExtent];

  CInfoPBRec
    search_info;

  DIR
    *entry;

  int
    error;

  search_info.hFileInfo.ioNamePtr=0;
  if ((path != (char *) NULL) || (*path != '\0'))
    if ((path[0] != '.') || (path[1] != '\0'))
      search_info.hFileInfo.ioNamePtr=
        c2pstr(strncpy(pathname,path,MaxTextExtent-1));
  search_info.hFileInfo.ioCompletion=0;
  search_info.hFileInfo.ioVRefNum=0;
  search_info.hFileInfo.ioFDirIndex=0;
  search_info.hFileInfo.ioDirID=0;
  error=PBGetCatInfoSync(&search_info);
  if (error != noErr)
    {
      errno=error;
      return((DIR *) NULL);
    }
  entry=(DIR *) AcquireMemory(sizeof(DIR));
  if (entry == (DIR *) NULL)
    return((DIR *) NULL);
  entry->d_VRefNum=search_info.hFileInfo.ioVRefNum;
  entry->d_DirID=search_info.hFileInfo.ioDirID;
  entry->d_index=1;
  return(entry);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   P r o c e s s P e n d i n g E v e n t s                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method ProcessPendingEvents processes any pending events.  This prevents
%  GraphicsMagick from monopolizing the processor.
%
%  The format of the ProcessPendingEvents method is:
%
%      ProcessPendingEvents(text)
%
%  A description of each parameter follows:
%
%    o text: A character string representing the current process.
%
%
*/
MagickExport void ProcessPendingEvents(const char *text)
{
  static const char
    *mark = (char *) NULL;

  EventRecord
    event;

  while (WaitNextEvent(everyEvent,&event,0L,nil))
    SIOUXHandleOneEvent(&event);
  if (isatty(STDIN_FILENO) && (text != mark))
    {
      (void) puts(text);
      mark=text;
    }
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   r e a d d i r                                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method readdir returns a pointer to a structure representing the
%  directory entry at the current position in the directory stream to
%  which entry refers.
%
%  The format of the readdir
%
%      struct dirent *readdir(DIR *entry)
%
%  A description of each parameter follows:
%
%    o entry: Specifies a pointer to a DIR structure.
%
%
*/
MagickExport struct dirent *readdir(DIR *entry)
{
  CInfoPBRec
    search_info;

  int
    error;

  static struct dirent
    dir_entry;

  static unsigned char
    pathname[MaxTextExtent];

  if (entry == (DIR *) NULL)
    return((struct dirent *) NULL);
  search_info.hFileInfo.ioCompletion=0;
  search_info.hFileInfo.ioNamePtr=pathname;
  search_info.hFileInfo.ioVRefNum=0;
  search_info.hFileInfo.ioFDirIndex=entry->d_index;
  search_info.hFileInfo.ioDirID=entry->d_DirID;
  error=PBGetCatInfoSync(&search_info);
  if (error != noErr)
    {
      errno=error;
      return((struct dirent *) NULL);
    }
  entry->d_index++;
  (void) strncpy(dir_entry.d_name,p2cstr(search_info.hFileInfo.ioNamePtr),254);
  dir_entry.d_namlen=strlen(dir_entry.d_name);
  return(&dir_entry);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  R e a d P I C T I m a g e                                                  %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method ReadPICTImage reads an Apple Macintosh QuickDraw/PICT image
%  file using MacOS QuickDraw methods and returns it.  It allocates the
%  memory necessary for the new Image structure and returns a pointer to
%  the new image.
%
%  This method was written and contributed by spd@daphne.cps.unizar.es
%  (feel free to copy and use it as you want. No warranty).
%
%  The format of the ReadPICTImage method is:
%
%      Image *ReadPICTImage(const ImageInfo *image_info,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image:  Method ReadPICTImage returns a pointer to the image after
%      reading.  A null image is returned if there is a a memory shortage or
%      if the image cannot be read.
%
%    o image_info: The image info..
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport Image *ReadPICTImage(const ImageInfo *image_info,
  ExceptionInfo *exception)
{
#define LoadImageText  "  Loading image...  "
#define PICTHeaderSize    512

  CodecType
    codec;

  GDHandle
    device;

  GWorldPtr
    graphic_world,
    port;

  Image
    *image;

  int
    depth,
    status;

  long
    y;

  PicHandle
    picture_handle;

  PictInfo
    picture_info;

  QDErr
    theErr = noErr;

  Rect
    rectangle;

  register IndexPacket
    *indexes;

  register long
    x;

  register PixelPacket
    *q;

  RGBColor
    Pixel;

  short
    colormap_id;

  /*
    Open image file.
  */
  image=AllocateImage(image_info);
  status=OpenBlob(image_info,image,ReadBinaryBlobMode,exception);
  if (status == False)
    ThrowReaderException(FileOpenError,"UnableToOpenFile",image);
  picture_handle=(PicHandle)
    NewHandle(Max(GetBlobSize(image)-PICTHeaderSize,PICTHeaderSize));
  if (picture_handle == nil)
    ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed",image);
  HLock((Handle) picture_handle);
  (void) ReadBlob(image,PICTHeaderSize,*(char **) picture_handle);
  status=
    ReadBlob(image,GetBlobSize(image)-PICTHeaderSize,*(char **) picture_handle);
  if (status == False)
    {
      DisposeHandle((Handle) picture_handle);
      ThrowReaderException(CorruptImageError,"UnableToReadImageData",image);
    }
  GetGWorld(&port,&device);
  theErr=NewGWorld(&graphic_world,0,&(**picture_handle).picFrame,nil,nil,
    useTempMem | keepLocal);
  if ((theErr != noErr) && (graphic_world == nil))
    {
      DisposeHandle((Handle) picture_handle);
      ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed",image);
    }
  HUnlock((Handle) picture_handle);
  SetGWorld(graphic_world,nil);
  theErr=GetPictInfo(picture_handle,&picture_info,0,1,systemMethod,0);
  if (theErr != noErr)
    {
      DisposeGWorld(graphic_world);
      DisposeHandle((Handle) picture_handle);
      ThrowReaderException(CorruptImageError,"UnableToReadImageData",image);
    }
  BottleneckTest(picture_handle,&codec,&depth,&colormap_id);
  switch (codec)
  {
    case 'rpza':
    case 'jpeg':
    case 'rle ':
    case 'raw ':
    case 'smc ':
    {
      if (depth > 200)
        {
          depth-=32;
          picture_info.theColorTable=GetCTable(colormap_id);
        }
      break;
    }
    default:
    {
      depth=picture_info.depth;
      if (depth <= 8)
        (void) GetPictInfo(picture_handle,&picture_info,returnColorTable,
          (short) (1 << picture_info.depth),systemMethod,0);
      break;
    }
  }
  image->x_resolution=(picture_info.hRes) >> 16;
  image->y_resolution=(picture_info.vRes) >> 16;
  image->units=PixelsPerInchResolution;
  image->columns=picture_info.sourceRect.right-picture_info.sourceRect.left;
  image->rows=picture_info.sourceRect.bottom-picture_info.sourceRect.top;
  if ((depth <= 8) && ((*(picture_info.theColorTable))->ctSize != 0))
    {
      unsigned int
        number_colors;

      /*
        Colormapped PICT image.
      */
      number_colors=(*(picture_info.theColorTable))->ctSize;
      if (!AllocateImageColormap(image,number_colors))
        {
          if (picture_info.theColorTable != nil)
            DisposeHandle((Handle) picture_info.theColorTable);
          DisposeGWorld(graphic_world);
          DisposeHandle((Handle) picture_handle);
          ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed",
            image);
        }
      for (x=0; x < image->colors; x++)
      {
        image->colormap[x].red=
          (*(picture_info.theColorTable))->ctTable[x].rgb.red;
        image->colormap[x].green=
          (*(picture_info.theColorTable))->ctTable[x].rgb.green;
        image->colormap[x].blue=
          (*(picture_info.theColorTable))->ctTable[x].rgb.blue;
      }
    }
  SetRect(&rectangle,0,0,image->columns,image->rows);
  (void) UpdateGWorld(&graphic_world,depth,&rectangle,
    picture_info.theColorTable,nil,0);
  LockPixels(GetGWorldPixMap(graphic_world));  /*->portPixMap); */
  EraseRect(&rectangle);
  DrawPicture(picture_handle,&rectangle);
  if ((depth <= 8) && (colormap_id == -1))
    {
      DisposeHandle((Handle) picture_info.theColorTable);
      picture_info.theColorTable=nil;
    }
  DisposeHandle((Handle) picture_handle);
  /*
    Convert PICT pixels to pixel packets.
  */
  for (y=0; y < image->rows; y++)
  {
    q=SetImagePixels(image,0,y,image->columns,1);
    if (q == (PixelPacket *) NULL)
      break;
    indexes=GetIndexes(image);
    for (x=0; x < image->columns; x++)
    {
      GetCPixel(x,y,&Pixel);
      q->red=ScaleCharToQuantum(Pixel.red & 0xff);
      q->green=ScaleCharToQuantum(Pixel.green & 0xff);
      q->blue=ScaleCharToQuantum(Pixel.blue & 0xff);
      if (image->storage_class == PseudoClass)
        indexes[x]=Color2Index(&Pixel);
      q++;
    }
    if (!SyncImagePixels(image))
      break;
    if (QuantumTick(y,image->rows))
      if (!MagickMonitor(LoadImageText,y,image->rows,&image->exception))
        break;
  }
  UnlockPixels(GetGWorldPixMap(graphic_world));
  SetGWorld(port,device);
  if (picture_info.theColorTable != nil)
    DisposeHandle((Handle) picture_info.theColorTable);
  DisposeGWorld(graphic_world);
  CloseBlob(image);
  return(image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S e a r c h F o r F i l e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method SearchForFile searches for a file.
%
%
*/
static Boolean SearchForFile(OSType creator_type,OSType file_type,FSSpec *file,
  short count)
{
  char
    *buffer;

  CInfoPBRec
    search1_info,
    search2_info;

  FSSpec
    application;

  HParamBlockRec
    parameter_info;

  long
    buffer_size = 16384;

  OSErr
    error;

  ProcessInfoRec
    application_info;

  ProcessSerialNumber
    serial_number;

  serial_number.lowLongOfPSN=kCurrentProcess;
  serial_number.highLongOfPSN=0;
  application_info.processInfoLength=sizeof(ProcessInfoRec);
  application_info.processName=NULL;
  application_info.processAppSpec=(&application);
  GetProcessInformation(&serial_number,&application_info);
  buffer=NewPtr(buffer_size);
  if (buffer == (char *) NULL)
    return(false);
  parameter_info.csParam.ioCompletion=NULL;
  parameter_info.csParam.ioNamePtr=NULL;
  parameter_info.csParam.ioVRefNum=application.vRefNum;
  parameter_info.csParam.ioMatchPtr=file;
  parameter_info.csParam.ioReqMatchCount=count;
  parameter_info.csParam.ioSearchBits=fsSBFlFndrInfo;
  parameter_info.csParam.ioSearchInfo1=&search1_info;
  parameter_info.csParam.ioSearchInfo2=&search2_info;
  parameter_info.csParam.ioSearchTime=0;
  parameter_info.csParam.ioCatPosition.initialize=0;
  parameter_info.csParam.ioOptBuffer=buffer;
  parameter_info.csParam.ioOptBufSize=buffer_size;
  search1_info.hFileInfo.ioNamePtr=NULL;
  search1_info.hFileInfo.ioFlFndrInfo.fdType=file_type;
  search1_info.hFileInfo.ioFlFndrInfo.fdCreator=creator_type;
  search1_info.hFileInfo.ioFlAttrib=0;
  search1_info.hFileInfo.ioFlParID=0;
  search2_info=search1_info;
  search2_info.hFileInfo.ioFlAttrib=0x10;
  search2_info.hFileInfo.ioFlFndrInfo.fdCreator=creator_type;
  search2_info.hFileInfo.ioFlFndrInfo.fdType=(-1);
  search2_info.hFileInfo.ioFlFndrInfo.fdFlags=0;
  search2_info.hFileInfo.ioFlFndrInfo.fdLocation.h=0;
  search2_info.hFileInfo.ioFlFndrInfo.fdLocation.v=0;
  search2_info.hFileInfo.ioFlFndrInfo.fdFldr=0;
  search2_info.hFileInfo.ioFlParID=0;
  error=PBCatSearchSync((CSParamPtr) &parameter_info);
  DisposePtr(buffer);
  if (parameter_info.csParam.ioReqMatchCount ==
      parameter_info.csParam.ioActMatchCount)
    error=eofErr;
  if (parameter_info.csParam.ioActMatchCount == 0)
    error=0;
  return(error == eofErr);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   s e e k d i r                                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method seekdir sets the position of the next readdir() operation
%  on the directory stream.
%
%  The format of the seekdir method is:
%
%      void seekdir(DIR *entry,long position)
%
%  A description of each parameter follows:
%
%    o entry: Specifies a pointer to a DIR structure.
%
%    o position: specifies the position associated with the directory
%      stream.
%
%
%
*/
MagickExport void seekdir(DIR *entry,long position)
{
  assert(entry != (DIR *) NULL);
  entry->d_index=position;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S e t A p p l i c a t i o n T y p e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method SetApplicationType sets the file type of an image.
%
%  The format of the SetApplicationType method is:
%
%      void SetApplicationType(const char *filename,const char *magick,
%        OSType application)
%
%  A description of each parameter follows:
%
%    o filename: Specifies the name of the file.
%
%    o filename: Specifies the file type.
%
%    o application: Specifies the type of the application.
%
%
*/
MagickExport void SetApplicationType(const char *filename,const char *magick,
  OSType application)
{
  FSSpec
    file_specification;

  OSType
    filetype;

  Str255
    name;

  assert(filename != (char *) NULL);
  assert(magick != (const char *) NULL);
  filetype='    ';
  (void) strncpy((char *) &filetype,magick,Min(strlen(magick),4));
  if (LocaleCompare(magick,"JPG") == 0)
    (void) strcpy((char *) &filetype,"JPEG");
  (void) strncpy((char *) name,filename,254);
  c2pstr((char *) name);
  FSMakeFSSpec(0,0,name,&file_specification);
  FSpCreate(&file_specification,application,filetype,smSystemScript);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   t e l l d i r                                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%   Method telldir returns the current location associated  with  the
%   named directory stream.
%
%  The format of the telldir method is:
%
%      telldir(DIR *entry)
%
%  A description of each parameter follows:
%
%    o entry: Specifies a pointer to a DIR structure.
%
%
*/
MagickExport long telldir(DIR *entry)
{
  return(entry->d_index);
}
#endif
