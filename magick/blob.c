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
%                         BBBB   L       OOO   BBBB                           %
%                         B   B  L      O   O  B   B                          %
%                         BBBB   L      O   O  BBBB                           %
%                         B   B  L      O   O  B   B                          %
%                         BBBB   LLLLL   OOO   BBBB                           %
%                                                                             %
%                                                                             %
%                  GraphicsMagick Binary Large OBject Methods                 %
%                                                                             %
%                                                                             %
%                              Software Design                                %
%                                John Cristy                                  %
%                                 July 1999                                   %
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
#if defined(WIN32) || defined(__CYGWIN__)
# include "magick/nt_feature.h"
#endif
#include "magick/blob.h"
#include "magick/cache.h"
#include "magick/constitute.h"
#include "magick/delegate.h"
#include "magick/log.h"
#include "magick/magick.h"
#include "magick/module.h"
#include "magick/resource.h"
#include "magick/stream.h"
#include "magick/tempfile.h"
#include "magick/utility.h"
#if defined(HasZLIB)
#include "zlib.h"
#endif
#if defined(HasBZLIB)
#include "bzlib.h"
#endif

/*
  Define declarations.
*/
#define DefaultBlobQuantum  65541

/*
  Some systems have unlocked versions of getc & putc which are faster
  when multi-threading is enabled.  Blobs do not require multi-thread
  support since Images are only allowed to be accessed by one thread at
  a time. Using the unlocked version improves performance by about 30%.
*/
#if defined(HAVE_GETC_UNLOCKED)
#  undef getc
#  define getc getc_unlocked
#endif
#if defined(HAVE_PUTC_UNLOCKED)
#  undef putc
#  define putc putc_unlocked
#endif

static const char *BlobMapModeToString(MapMode map_mode)
{
  char
    *mode_string="Undefined";

  switch (map_mode)
  {
  case ReadMode:
    mode_string="Read";
    break;
  case WriteMode:
    mode_string="Write";
    break;
  case IOMode:
    mode_string="IO";
    break;
  }
  return mode_string;
}

static const char *BlobModeToString(BlobMode blob_mode)
{
  char
    *mode_string="Undefined";

  switch (blob_mode)
  {
  case UndefinedBlobMode:
    mode_string="Undefined";
    break;
  case ReadBlobMode:
    mode_string="Read";
    break;
  case ReadBinaryBlobMode:
    mode_string="ReadBinary";
    break;
  case WriteBlobMode:
    mode_string="Write";
    break;
  case WriteBinaryBlobMode:
    mode_string="WriteBinary";
    break;
  case IOBinaryBlobMode:
    mode_string="IOBinary";
    break;
  }
  return mode_string;
}
static const char *BlobStreamTypeToString(StreamType stream_type)
{
  char
    *type_string="Undefined";

  switch (stream_type)
  {
  case UndefinedStream:
    type_string="Undefined";
    break;
  case FileStream:
    type_string="File";
    break;
  case StandardStream:
    type_string="Standard";
    break;
  case PipeStream:
    type_string="Pipe";
    break;
  case ZipStream:
    type_string="Zip";
    break;
  case BZipStream:
    type_string="BZip";
    break;
  case FifoStream:
    type_string="Fifo";
    break;
  case BlobStream:
    type_string="Blob";
    break;
  }
  return type_string;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  R e a d B l o b S t r e a m                                                %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadBlobStream() allocates the requested data size, or the amount
%  remaining (whichever is smaller) from a BlobStream.  This function
%  should only be invoked for Blobs of type 'BlobStream'.  The number of
%  bytes available from the requested length is returned.  If fewer bytes
%  are available than requested, the Blob EOF flag is set True.  A user
%  provided pointer is updated with the address of the data.  This pointer
%  is only valid while the BlobStream remains mapped or allocated.
%
%  The format of the ReadBlob method is:
%
%      unsigned char *ReadBlobStream(Image *image,size_t request,
%                                   size_t *available)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o length: The requested amount of data.
%
%    o data: A pointer to where the address of the data should be returned.
%
*/
static inline size_t ReadBlobStream(Image *image,const size_t length,void **data)
{
  size_t
    available;

  *data=(void *)(image->blob->data+image->blob->offset);
  available=Min(length,image->blob->length-image->blob->offset);
  image->blob->offset+=available;
  if (available < length)
    image->blob->eof=True;
  return available;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   A t t a c h B l o b                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AttachBlob() attaches a blob to the BlobInfo structure.
%
%  The format of the AttachBlob method is:
%
%      void AttachBlob(BlobInfo *blob_info,const void *blob,const size_t length)
%
%  A description of each parameter follows:
%
%    o blob_info: Specifies a pointer to a BlobInfo structure.
%
%    o blob: The address of a character stream in one of the image formats
%      understood by ImageMagick.
%
%    o length: This size_t integer reflects the length in bytes of the blob.
%
%
*/
MagickExport void AttachBlob(BlobInfo *blob_info,const void *blob,
  const size_t length)
{
  assert(blob_info != (BlobInfo *) NULL);
  blob_info->length=length;
  blob_info->extent=length;
  blob_info->quantum=DefaultBlobQuantum;
  blob_info->offset=0;
  blob_info->type=BlobStream;
  blob_info->file=(FILE *) NULL;
  blob_info->data=(unsigned char *) blob;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   B l o b T o F i l e                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  BlobToFile() writes a blob to a file.  It returns False if an error occurs
%  otherwise True.
%
%  The format of the BlobToFile method is:
%
%      unsigned int BlobToFile(const char *filename,const void *blob,
%        const size_t length,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o status:  BlobToFile returns True on success; otherwise,  it
%      returns False if an error occurs.
%
%    o filename: Write the blob to this file.
%
%    o blob: The address of a blob.
%
%    o length: This length in bytes of the blob.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport unsigned int BlobToFile(const char *filename,const void *blob,
  const size_t length,ExceptionInfo *exception)
{
  ExtendedSignedIntegralType
    count;

  int
    file;

  register size_t
    i;

  assert(filename != (const char *) NULL);
  assert(blob != (const void *) NULL);
  (void) LogMagickEvent(BlobEvent,GetMagickModule(),
    "Copying memory BLOB to file %s\n",filename);
  file=open(filename,O_WRONLY | O_CREAT | O_TRUNC | O_BINARY,0777);
  if (file == -1)
    {
      ThrowException(exception,BlobError,"UnableToWriteBlob",filename);
      return(False);
    }
  for (i=0; i < length; i+=count)
  {
    count=write(file,(char *) blob+i,length-i);
    if (count <= 0)
      break;
  }
  (void) close(file);
  if (i < length)
    {
      ThrowException(exception,BlobError,"UnableToWriteBlob",filename);
      return(False);
    }
  return(True);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   B l o b T o I m a g e                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  BlobToImage() implements direct to memory image formats.  It returns the
%  blob as an image.
%
%  The format of the BlobToImage method is:
%
%      Image *BlobToImage(const ImageInfo *image_info,const void *blob,
%        const size_t length,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: The image info.
%
%    o blob: The address of a character stream in one of the image formats
%      understood by ImageMagick.
%
%    o length: This size_t integer reflects the length in bytes of the blob.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport Image *BlobToImage(const ImageInfo *image_info,const void *blob,
  const size_t length,ExceptionInfo *exception)
{
  const MagickInfo
    *magick_info;

  Image
    *image;

  ImageInfo
    *clone_info;

  unsigned int
    status;

  assert(image_info != (ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  assert(exception != (ExceptionInfo *) NULL);
  (void) LogMagickEvent(BlobEvent,GetMagickModule(), "Entering BlobToImage");
  SetExceptionInfo(exception,UndefinedException);
  if ((blob == (const void *) NULL) || (length == 0))
    {
      ThrowException(exception,BlobError,"ZeroLengthBlobNotPermitted",
        image_info->magick);
      (void) LogMagickEvent(BlobEvent,GetMagickModule(),
        "Leaving BlobToImage");
      return((Image *) NULL);
    }
  clone_info=CloneImageInfo(image_info);
  clone_info->blob=(void *) blob;
  clone_info->length=length;
  if (clone_info->magick[0] == '\0')
    (void) SetImageInfo(clone_info,False,exception);
  magick_info=GetMagickInfo(clone_info->magick,exception);
  if (magick_info == (const MagickInfo *) NULL)
    {
      DestroyImageInfo(clone_info);
      (void) LogMagickEvent(BlobEvent,GetMagickModule(),
        "Leaving BlobToImage");
      return((Image *) NULL);
    }
  if (magick_info->blob_support)
    {
      /*
        Native blob support for this image format.
      */
      (void) LogMagickEvent(BlobEvent,GetMagickModule(),
        "Using native BLOB support");
      (void) strncpy(clone_info->filename,image_info->filename,
        MaxTextExtent-1);
      (void) strncpy(clone_info->magick,image_info->magick,MaxTextExtent-1);
      image=ReadImage(clone_info,exception);
      if (image != (Image *) NULL)
        DetachBlob(image->blob);
      DestroyImageInfo(clone_info);
      (void) LogMagickEvent(BlobEvent,GetMagickModule(),
        "Leaving BlobToImage");
      return(image);
    }
  /*
    Write blob to a temporary file on disk.
  */
  clone_info->blob=(void *) NULL;
  clone_info->length=0;
  if(!AcquireTemporaryFileName(clone_info->filename))
    {
      ThrowException(exception,FileOpenError,"UnableToCreateTemporaryFile",
        clone_info->filename);
      DestroyImageInfo(clone_info);
      return((Image *) NULL);
    }
  status=BlobToFile(clone_info->filename,blob,length,exception);
  if (status == False)
    {
      DestroyImageInfo(clone_info);
      (void) LogMagickEvent(BlobEvent,GetMagickModule(),
        "Leaving BlobToImage");
      return((Image *) NULL);
    }
  image=ReadImage(clone_info,exception);
  (void) LogMagickEvent(BlobEvent,GetMagickModule(),
    "Removing temporary file \"%s\"\n",clone_info->filename);
  LiberateTemporaryFile(clone_info->filename);
  DestroyImageInfo(clone_info);
  (void) LogMagickEvent(BlobEvent,GetMagickModule(), "Leaving BlobToImage");
  return(image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   C l o n e B l o b I n f o                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  CloneBlobInfo() makes a duplicate of the given blob info structure, or if
%  blob info is NULL, a new one.
%
%  The format of the CloneBlobInfo method is:
%
%      BlobInfo *CloneBlobInfo(const BlobInfo *blob_info)
%
%  A description of each parameter follows:
%
%    o clone_info: Method CloneBlobInfo returns a duplicate of the given
%      blob info, or if blob info is NULL a new one.
%
%    o quantize_info: a structure of type info.
%
%
*/
MagickExport BlobInfo *CloneBlobInfo(const BlobInfo *blob_info)
{
  BlobInfo
    *clone_info;

  clone_info=(BlobInfo *) AcquireMemory(sizeof(BlobInfo));
  if (clone_info == (BlobInfo *) NULL)
    MagickFatalError(ResourceLimitFatalError,"MemoryAllocationFailed",
      "UnableToCloneBlobInfo");
  GetBlobInfo(clone_info);
  if (blob_info == (BlobInfo *) NULL)
    return(clone_info);
  clone_info->length=blob_info->length;
  clone_info->extent=blob_info->extent;
  clone_info->quantum=blob_info->quantum;
  clone_info->mapped=blob_info->mapped;
  clone_info->eof=blob_info->eof;
  clone_info->offset=blob_info->offset;
  clone_info->size=blob_info->size;
  clone_info->exempt=blob_info->exempt;
  clone_info->status=blob_info->status;
  clone_info->temporary=blob_info->temporary;
  clone_info->type=blob_info->type;
  clone_info->file=blob_info->file;
  clone_info->stream=blob_info->stream;
  clone_info->data=blob_info->data;
  clone_info->reference_count=1;
  clone_info->semaphore=(SemaphoreInfo *) NULL;
  return(clone_info);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   C l o s e B l o b                                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  CloseBlob() closes a stream associated with the image.
%
%  The format of the CloseBlob method is:
%
%      void CloseBlob(Image *image)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%
*/
MagickExport void CloseBlob(Image *image)
{
  int
    status;

  /*
    Close image file.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  assert(image->blob != (BlobInfo *) NULL);
  assert(image->blob->type != UndefinedStream);
  (void) LogMagickEvent(BlobEvent,GetMagickModule(),
    "Closing %sStream blob %p",BlobStreamTypeToString(image->blob->type),
      &image->blob);
  status=0;
  switch (image->blob->type)
  {
    case UndefinedStream:
      break;
    case FileStream:
    case StandardStream:
    case PipeStream:
    {
      status=ferror(image->blob->file);
      break;
    }
    case ZipStream:
    {
#if defined(HasZLIB)
      gzerror(image->blob->file,&status);
#endif
      break;
    }
    case BZipStream:
    {
#if defined(HasBZLIB)
      BZ2_bzerror(image->blob->file,&status);
#endif
      break;
    }
    case FifoStream:
    case BlobStream:
      break;
  }
  errno=0;
  image->taint=False;
  image->blob->size=GetBlobSize(image);
  image->blob->eof=False;
  image->blob->status=status < 0;
  if (image->blob->exempt)
    return;
  switch (image->blob->type)
  {
    case UndefinedStream:
      break;
    case FileStream:
    case StandardStream:
    {
      status=fclose(image->blob->file);
      break;
    }
    case PipeStream:
    {
#if !defined(vms) && !defined(macintosh)
      status=pclose(image->blob->file);
#endif
      break;
    }
    case ZipStream:
    {
#if defined(HasZLIB)
      status=gzclose(image->blob->file);
#endif
      break;
    }
    case BZipStream:
    {
#if defined(HasBZLIB)
      BZ2_bzclose(image->blob->file);
#endif
      break;
    }
    case FifoStream:
    case BlobStream:
      break;
  }
  DetachBlob(image->blob);
  image->blob->status=status < 0;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D e s t r o y B l o b I n f o                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DestroyBlobInfo() deallocates memory associated with an BlobInfo structure.
%
%  The format of the DestroyBlobInfo method is:
%
%      void DestroyBlobInfo(BlobInfo *blob)
%
%  A description of each parameter follows:
%
%    o blob: Specifies a pointer to a BlobInfo structure.
%
%
*/
MagickExport void DestroyBlobInfo(BlobInfo *blob)
{
  assert(blob != (BlobInfo *) NULL);
  assert(blob->signature == MagickSignature);
  AcquireSemaphoreInfo(&blob->semaphore);
  blob->reference_count--;
  if (blob->reference_count > 0)
    {
      LiberateSemaphoreInfo(&blob->semaphore);
      return;
    }
  LiberateSemaphoreInfo(&blob->semaphore);
  if (blob->mapped)
    (void) UnmapBlob(blob->data,blob->length);
  if (blob->semaphore != (SemaphoreInfo *) NULL)
    DestroySemaphoreInfo(&blob->semaphore);
  memset((void *)blob,0xbf,sizeof(BlobInfo));
  LiberateMemory((void **) &blob);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D e t a c h B l o b                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DetachBlob() detaches a blob from the BlobInfo structure.
%
%  The format of the DetachBlob method is:
%
%      void DetachBlob(BlobInfo *blob_info)
%
%  A description of each parameter follows:
%
%    o blob_info: Specifies a pointer to a BlobInfo structure.
%
%
*/
MagickExport void DetachBlob(BlobInfo *blob_info)
{
  assert(blob_info != (BlobInfo *) NULL);
  if (blob_info->mapped)
    (void) UnmapBlob(blob_info->data,blob_info->length);
  blob_info->mapped=False;
  blob_info->length=0;
  blob_info->offset=0;
  blob_info->eof=False;
  blob_info->exempt=False;
  blob_info->type=UndefinedStream;
  blob_info->file=(FILE *) NULL;
  blob_info->data=(unsigned char *) NULL;
  blob_info->stream=(StreamHandler) NULL;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  E O F B l o b                                                              %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  EOFBlob() returns a non-zero value when EOF has been detected reading from
%  a blob or file.
%
%  The format of the EOFBlob method is:
%
%      int EOFBlob(const Image *image)
%
%  A description of each parameter follows:
%
%    o status:  Method EOFBlob returns 0 on success; otherwise,  it
%      returns -1 and set errno to indicate the error.
%
%    o image: The image.
%
%
*/
MagickExport int EOFBlob(const Image *image)
{
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  assert(image->blob != (BlobInfo *) NULL);
  assert(image->blob->type != UndefinedStream);
  switch (image->blob->type)
  {
    case UndefinedStream:
      break;
    case FileStream:
    case StandardStream:
    case PipeStream:
    {
      image->blob->eof=feof(image->blob->file);
      break;
    }
    case ZipStream:
    {
      image->blob->eof=False;
      break;
    }
    case BZipStream:
    {
#if defined(HasBZLIB)
      int
        status;

      (void) BZ2_bzerror(image->blob->file,&status);
      image->blob->eof=status == BZ_UNEXPECTED_EOF;
#endif
      break;
    }
    case FifoStream:
    {
      image->blob->eof=False;
      break;
    }
    case BlobStream:
      break;
  }
  return(image->blob->eof);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   F i l e T o B l o b                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  FileToBlob() returns the contents of a file as a blob.  It returns the
%  file as a blob and its length.  If an error occurs, NULL is returned.
%
%  The format of the FileToBlob method is:
%
%      void *FileToBlob(const char *filename,size_t *length,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o blob:  FileToBlob() returns the contents of a file as a blob.  If
%      an error occurs NULL is returned.
%
%    o filename: The filename.
%
%    o length: This pointer to a size_t integer sets the initial length of the
%      blob.  On return, it reflects the actual length of the blob.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport void *FileToBlob(const char *filename,size_t *length,
  ExceptionInfo *exception)
{
  ExtendedSignedIntegralType
    offset;

  int
    file;

  unsigned char
    *blob;

  void
    *map;

  assert(filename != (const char *) NULL);
  assert(exception != (ExceptionInfo *) NULL);
  (void) LogMagickEvent(BlobEvent,GetMagickModule(),
    "Copying file %s to memory BLOB",filename);
  SetExceptionInfo(exception,UndefinedException);
  file=open(filename,O_RDONLY | O_BINARY,0777);
  if (file == -1)
    {
      ThrowException(exception,BlobError,"UnableToOpenFile",filename);
      return((void *) NULL);
    }
  offset=MagickSeek(file,0,SEEK_END);
  if ((offset < 0) || (offset != (size_t) offset))
    {
      (void) close(file);
      ThrowException(exception,BlobError,"UnableToSeekToOffset",
        "UnableToCreateBlob");
      return((void *) NULL);
    }
  *length=(size_t) offset;
  blob=(unsigned char *) AcquireMemory(*length+1);
  if (blob == (unsigned char *) NULL)
    {
      (void) close(file);
      ThrowException(exception,BlobError,"MemoryAllocationFailed",
        "UnableToCreateBlob");
      return((void *) NULL);
    }
  map=MapBlob(file,ReadMode,0,*length);
  if (map != (void *) NULL)
    {
      (void) memcpy(blob,map,*length);
      UnmapBlob(map,*length);
    }
  else
    {
      ExtendedSignedIntegralType
        count;

      register size_t
        i;

      (void) MagickSeek(file,0,SEEK_SET);
      for (i=0; i < *length; i+=count)
      {
        count=read(file,blob+i,*length-i);
        if (count <= 0)
          break;
      }
      if (i < *length)
        {
          (void) close(file);
          LiberateMemory((void **) &blob);
          ThrowException(exception,BlobError,"UnableToReadToOffset",
            "UnableToCreateBlob");
          return((void *) NULL);
        }
    }
  blob[*length]='\0';
  (void) close(file);
  return(blob);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t B l o b I n f o                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetBlobInfo() initializes the BlobInfo structure.
%
%  The format of the GetBlobInfo method is:
%
%      void GetBlobInfo(BlobInfo *blob_info)
%
%  A description of each parameter follows:
%
%    o blob_info: Specifies a pointer to a BlobInfo structure.
%
%
*/
MagickExport void GetBlobInfo(BlobInfo *blob_info)
{
  assert(blob_info != (BlobInfo *) NULL);
  (void) memset(blob_info,0,sizeof(BlobInfo));
  blob_info->quantum=DefaultBlobQuantum;
  blob_info->reference_count=1;
  blob_info->signature=MagickSignature;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  G e t B l o b S i z e                                                      %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetBlobSize() returns the current length of the image file or blob; zero is
%  returned if the size cannot be determined.
%
%  The format of the GetBlobSize method is:
%
%      ExtendedSignedIntegralType GetBlobSize(const Image *image)
%
%  A description of each parameter follows:
%
%    o size:  Method GetBlobSize returns the current length of the image file
%      or blob.
%
%    o image: The image.
%
%
*/
MagickExport ExtendedSignedIntegralType GetBlobSize(const Image *image)
{
  struct stat
    attributes;

  ExtendedSignedIntegralType
    offset;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  assert(image->blob != (BlobInfo *) NULL);
  if (image->blob->type == UndefinedStream)
    return(image->blob->size);
  offset=0;
  switch (image->blob->type)
  {
    case UndefinedStream:
      break;
    case FileStream:
    {
      offset=fstat(fileno(image->blob->file),&attributes) < 0 ? 0 :
        attributes.st_size;
      break;
    }
    case StandardStream:
    case PipeStream:
      break;
    case ZipStream:
    {
#if defined(HasZLIB)
      offset=stat(image->filename,&attributes) < 0 ? 0 : attributes.st_size;
#endif
      break;
    }
    case BZipStream:
    {
#if defined(HasBZLIB)
      offset=stat(image->filename,&attributes) < 0 ? 0 : attributes.st_size;
#endif
      break;
    }
    case FifoStream:
      break;
    case BlobStream:
    {
      offset=image->blob->length;
      break;
    }
  }
  return(offset);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t B l o b S t r e a m D a t a                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetBlobStreamData() returns the stream data for the image.
%
%  The format of the GetBlobStreamData method is:
%
%      unsigned char *GetBlobStreamData(const Image *image)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%
*/
MagickExport unsigned char *GetBlobStreamData(const Image *image)
{
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickSignature);
  return(image->blob->data);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t B l o b S t r e a m T y p e                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetBlobStreamType() returns the stream type for the image.
%
%  The format of the GetBlobStreamType method is:
%
%      StreamType GetBlobStreamType(const Image *image)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%
*/
MagickExport StreamType GetBlobStreamType(const Image *image)
{
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickSignature);
  return(image->blob->type);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  G e t C o n f i g u r e B l o b                                            %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetConfigureBlob() returns the specified configure file as a blob.
%
%  The format of the GetConfigureBlob method is:
%
%      void *GetConfigureBlob(const char *filename,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o filename: The configure file name.
%
%    o path: return the full path information of the configure file.
%
%    o length: This pointer to a size_t integer sets the initial length of the
%      blob.  On return, it reflects the actual length of the blob.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/

#if !defined(UseInstalledMagick) && defined(POSIX)
static void ChopPathComponents(char *path,const unsigned long components)
{
  long
    count;

  register char
    *p;

  if (*path == '\0')
    return;
  p=path+strlen(path);
  if (*p == *DirectorySeparator)
    *p='\0';
  for (count=0; (count < (long) components) && (p > path); p--)
    if (*p == *DirectorySeparator)
      {
        *p='\0';
        count++;
      }
}
#endif

MagickExport void *GetConfigureBlob(const char *filename,char *path,
  size_t *length,ExceptionInfo *exception)
{
  assert(filename != (const char *) NULL);
  assert(path != (char *) NULL);
  assert(length != (size_t *) NULL);
  assert(exception != (ExceptionInfo *) NULL);
  (void) strncpy(path,filename,MaxTextExtent-1);
  (void) LogMagickEvent(ConfigureEvent,GetMagickModule(),
    "Searching for configure file \"%s\" ...",filename);
#if defined(UseInstalledMagick)
#if defined(MagickLibPath)
  /*
    Search hard coded paths.
  */
  FormatString(path,"%.1024s%.1024s",MagickLibPath,filename);
  if (!IsAccessible(path))
    ThrowException(exception,ConfigureError,"UnableToAccessConfigureFile",
      path);
  return(FileToBlob(path,length,exception));
#endif /* defined(MagickLibPath) */
#if defined(WIN32)
  {
    char
      *key_value;

    /*
      Locate file via registry key.
    */
    key_value=NTRegistryKeyLookup("ConfigurePath");
    if (key_value != (char *) NULL)
      {
        FormatString(path,"%.1024s%s%.1024s",key_value,DirectorySeparator,
          filename);
        if (!IsAccessible(path))
          ThrowException(exception,ConfigureError,
            "UnableToAccessConfigureFile",path);
        return(FileToBlob(path,length,exception));
      }
  }
#endif /* defined(WIN32) */
#else /* !defined(UseInstalledMagick) */
  if (*SetClientPath((char *) NULL) != '\0')
    {
#if defined(POSIX)
      char
        prefix[MaxTextExtent];

      /*
        Search based on executable directory if directory is known.
      */
      (void) strncpy(prefix,SetClientPath((char *) NULL),MaxTextExtent-1);
      ChopPathComponents(prefix,1);
      FormatString(path,"%.1024s/lib/%s/%.1024s",prefix,MagickLibSubdir,
        filename);
#else /* defined(POSIX) */
      FormatString(path,"%.1024s%s%.1024s",SetClientPath((char *) NULL),
        DirectorySeparator,filename);
#endif /* !defined(POSIX) */
      if (IsAccessible(path))
        return(FileToBlob(path,length,exception));
    }
  if (getenv("MAGICK_HOME") != (char *) NULL)
    {
      /*
        Search MAGICK_HOME.
      */
#if defined(POSIX)
      FormatString(path,"%.1024s/lib/%s/%.1024s",getenv("MAGICK_HOME"),
        MagickLibSubdir,filename);
#else
      FormatString(path,"%.1024s%s%.1024s",getenv("MAGICK_HOME"),
        DirectorySeparator,filename);
#endif /* defined(POSIX) */
      if (IsAccessible(path))
        return(FileToBlob(path,length,exception));
    }
  if (getenv("HOME") != (char *) NULL)
    {
      /*
        Search $HOME/.magick.
      */
      FormatString(path,"%.1024s%s%s%.1024s",getenv("HOME"),
        *getenv("HOME") == '/' ? "/.magick" : "",DirectorySeparator,filename);
      if (IsAccessible(path))
        return(FileToBlob(path,length,exception));
    }
  /*
    Search current directory.
  */
  if (IsAccessible(path))
    return(FileToBlob(path,length,exception));
#if defined(WIN32)
  return(NTResourceToBlob(filename));
#endif /* defined(WIN32) */
#endif /* !defined(UseInstalledMagick) */
  ThrowException(exception,ConfigureError,"UnableToAccessConfigureFile",path);
  return((void *) NULL);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I m a g e T o B l o b                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ImageToBlob() implements direct to memory image formats.  It returns the
%  image as a blob and its length.  The magick member of the Image structure
%  determines the format of the returned blob(GIG, JPEG,  PNG, etc.)
%
%  The format of the ImageToBlob method is:
%
%      void *ImageToBlob(const ImageInfo *image_info,Image *image,
%        size_t *length,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: The image info..
%
%    o image: The image.
%
%    o length: This pointer to a size_t integer sets the initial length of the
%      blob.  On return, it reflects the actual length of the blob.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport void *ImageToBlob(const ImageInfo *image_info,Image *image,
  size_t *length,ExceptionInfo *exception)
{
  char
    filename[MaxTextExtent],
    unique[MaxTextExtent];

  const MagickInfo
    *magick_info;

  ImageInfo
    *clone_info;

  unsigned char
    *blob;

  unsigned int
    status;

  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  assert(exception != (ExceptionInfo *) NULL);
  (void) LogMagickEvent(BlobEvent,GetMagickModule(),"Entering ImageToBlob");
  SetExceptionInfo(exception,UndefinedException);
  clone_info=CloneImageInfo(image_info);
  (void) strncpy(clone_info->magick,image->magick,MaxTextExtent-1);
  magick_info=GetMagickInfo(clone_info->magick,exception);
  if (magick_info == (const MagickInfo *) NULL)
     {
       DestroyImageInfo(clone_info);
       (void) LogMagickEvent(BlobEvent,GetMagickModule(),
         "Exiting ImageToBlob");
       return((void *) NULL);
     }
  if (magick_info->blob_support)
    {
      /*
        Native blob support for this image format.
      */
      clone_info->blob=(void *) AcquireMemory(65535L);
      if (clone_info->blob == (void *) NULL)
        {
          ThrowException(exception,BlobError,"MemoryAllocationFailed",
            "UnableToCreateBlob");
          DestroyImageInfo(clone_info);
          (void) LogMagickEvent(BlobEvent,GetMagickModule(),
            "Exiting ImageToBlob");
          return((void *) NULL);
        }
      clone_info->length=0;
      image->blob->exempt=True;
      *image->filename='\0';
      status=WriteImage(clone_info,image);
      if (status == False)
        {
          ThrowException(exception,BlobError,"UnableToWriteBlob",
            clone_info->magick);
          LiberateMemory((void **) &image->blob->data);
          DestroyImageInfo(clone_info);
          (void) LogMagickEvent(BlobEvent,GetMagickModule(),
            "Exiting ImageToBlob");
          return((void *) NULL);
        }
      ReacquireMemory((void **) &image->blob->data,image->blob->length+1);
      blob=image->blob->data;
      *length=image->blob->length;
      DetachBlob(image->blob);
      DestroyImageInfo(clone_info);
      (void) LogMagickEvent(BlobEvent,GetMagickModule(),
        "Exiting ImageToBlob");
      return(blob);
    }
  /*
    Write file to disk in blob image format.
  */
  (void) strncpy(filename,image->filename,MaxTextExtent-1);
  if(!AcquireTemporaryFileName(unique))
    {
      ThrowException(exception,FileOpenError,"UnableToCreateTemporaryFile",
        unique);
      DestroyImageInfo(clone_info);
      return((void *) NULL);
    }
  FormatString(image->filename,"%.1024s:%.1024s",image->magick,unique);
  status=WriteImage(clone_info,image);
  DestroyImageInfo(clone_info);
  if (status == False)
    {
      LiberateTemporaryFile(unique);
      ThrowException(exception,BlobError,"UnableToWriteBlob",image->filename);
      (void) LogMagickEvent(BlobEvent,GetMagickModule(),
        "Exiting ImageToBlob");
      return((void *) NULL);
    }
  /*
    Read image from disk as blob.
  */
  blob=(unsigned char *) FileToBlob(image->filename,length,exception);
  LiberateTemporaryFile(image->filename);
  (void) strncpy(image->filename,filename,MaxTextExtent-1);
  if (blob == (unsigned char ) NULL)
    {
      ThrowException(exception,BlobError,"UnableToReadFile",filename);
      (void) LogMagickEvent(BlobEvent,GetMagickModule(),
        "Exiting ImageToBlob");
      return((void *) NULL);
    }
    (void) LogMagickEvent(BlobEvent,GetMagickModule(),
      "Exiting ImageToBlob");
  return(blob);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I m a g e T o F i l e                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ImageToFile() copies the input image from an open blob stream to a file.
%  It returns False if an error occurs otherwise True.  This function is used
%  to handle coders which are unable to stream the data in using Blob I/O.
%  Instead of streaming the data in, the data is streammed to a temporary
%  file, and the coder accesses the temorary file directly.
%
%  The format of the ImageToFile method is:
%
%      unsigned int ImageToFile(Image *image,const char *filename,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o status:  ImageToFile returns True on success; otherwise,  it
%      returns False if an error occurs.
%
%    o image: The image.
%
%    o filename: Write the image to this file.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport unsigned int ImageToFile(Image *image,const char *filename,
  ExceptionInfo *exception)
{
#define MaxBufferSize  65541

  char
    *buffer;

  ExtendedSignedIntegralType
    count;

  int
    file;

  register size_t
    i;

  size_t
    length;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  assert(filename != (const char *) NULL);

  (void) LogMagickEvent(BlobEvent,GetMagickModule(),
    "Copying from Blob stream to file %s",filename);

  file=open(filename,O_WRONLY | O_CREAT | O_TRUNC | O_BINARY,0777);
  if (file == -1)
    {
      ThrowException(exception,BlobError,"UnableToWriteBlob",filename);
      return(False);
    }
  buffer=(char *) AcquireMemory(MaxBufferSize);
  if (buffer == (char *) NULL)
    {
      (void) close(file);
      ThrowException(exception,ResourceLimitError,"MemoryAllocationError",
        filename);
      return(False);
    }
  for (i=0; (length=ReadBlob(image,MaxBufferSize,buffer)) > 0; )
  {
    for (i=0; i < length; i+=count)
    {
      count=write(file,buffer+i,length-i);
      if (count <= 0)
        break;
    }
    if (i < length)
      break;
  }
  (void) close(file);
  LiberateMemory((void **) &buffer);
  return(i < length);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  M a p B l o b                                                              %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MapBlob() creates a mapping from a file to a binary large object.
%
%  The format of the MapBlob method is:
%
%      void *MapBlob(int file,const MapMode mode,off_t offset,size_t length)
%
%  A description of each parameter follows:
%
%    o status:  Method MapBlob returns the address of the blob as well as
%      its length in bytes.
%
%    o file: map this file descriptor.
%
%    o mode: ReadMode, WriteMode, or IOMode.
%
%    o offset: starting at this offset within the file.
%
%    o length: the length of the mapping is returned in this pointer.
%
%
*/
MagickExport void *MapBlob(int file,const MapMode mode,off_t offset,
  size_t length)
{
#if defined(HAVE_MMAP)
  void
    *map;

  /*
    Map file.
  */
  if (file == -1)
    return((void *) NULL);
  switch (mode)
  {
    case ReadMode:
    default:
    {
      map=(void *) mmap((char *) NULL,length,PROT_READ,MAP_PRIVATE,file,offset);
      break;
    }
    case WriteMode:
    {
      map=(void *) mmap((char *) NULL,length,PROT_WRITE,MAP_SHARED,file,offset);
      break;
    }
    case IOMode:
    {
      map=(void *) mmap((char *) NULL,length,(PROT_READ | PROT_WRITE),
        MAP_SHARED,file,offset);
      break;
    }
  }
  if (map == (void *) MAP_FAILED)
    return((void *) NULL);
  (void) LogMagickEvent(BlobEvent,GetMagickModule(),
    "Mmapped FD %d using %s mode at offset %lu and length %lu to address 0x%p",
    file,BlobMapModeToString(mode),(unsigned long)offset,(unsigned long)length,
    map);
  return((void *) map);
#else
  return((void *) NULL);
#endif
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  M S B O r d e r L o n g                                                    %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method MSBOrderLong converts a least-significant byte first buffer
%  of integers to most-significant byte first.
%
%  The format of the MSBOrderLong method is:
%
%      void MSBOrderLong(unsigned char *buffer,const size_t length)
%
%  A description of each parameter follows.
%
%   o  p:  Specifies a pointer to a buffer of integers.
%
%   o  length:  Specifies the length of the buffer.
%
%
*/
MagickExport void MSBOrderLong(unsigned char *buffer,const size_t length)
{
  int
    c;

  register unsigned char
    *p,
    *q;

  assert(buffer != (unsigned char *) NULL);
  q=buffer+length;
  while (buffer < q)
  {
    p=buffer+3;
    c=(*p);
    *p=(*buffer);
    *buffer++=(unsigned char) c;
    p=buffer+1;
    c=(*p);
    *p=(*buffer);
    *buffer++=(unsigned char) c;
    buffer+=2;
  }
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  M S B O r d e r S h o r t                                                  %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method MSBOrderShort converts a least-significant byte first buffer of
%  integers to most-significant byte first.
%
%  The format of the MSBOrderShort method is:
%
%      void MSBOrderShort(unsigned char *p,const size_t length)
%
%  A description of each parameter follows.
%
%   o  p:  Specifies a pointer to a buffer of integers.
%
%   o  length:  Specifies the length of the buffer.
%
%
*/
MagickExport void MSBOrderShort(unsigned char *p,const size_t length)
{
  int
    c;

  register unsigned char
    *q;

  assert(p != (unsigned char *) NULL);
  q=p+length;
  while (p < q)
  {
    c=(*p);
    *p=(*(p+1));
    p++;
    *p++=(unsigned char) c;
  }
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   O p e n B l o b                                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  OpenBlob() opens a file associated with the image.  A file name of '-' sets
%  the file to stdin for type 'r' and stdout for type 'w'.  If the filename
%  suffix is '.gz' or '.Z', the image is decompressed for type 'r' and
%  compressed for type 'w'.  If the filename prefix is '|', it is piped to or
%  from a system command.
%
%  The format of the OpenBlob method is:
%
%      unsigned int OpenBlob(const ImageInfo *image_info,Image *image,
%        const BlobMode mode,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o status:  Method OpenBlob returns True if the file is successfully
%      opened otherwise False.
%
%    o image_info: The image info.
%
%    o image: The image.
%
%    o mode: The mode for opening the file.
%
*/

static void FormMultiPartFilename(Image *image, const ImageInfo *image_info)
{
  char
    filename[MaxTextExtent],
    *p;

  /*
    Form filename for multi-part images.
  */
  (void) strncpy(filename,image->filename,MaxTextExtent-1);
  for (p=strchr(filename,'%'); p != (char *) NULL; p=strchr(p+1,'%'))
    {
      char
        *q;

      q=p+1;
      if (*q == '0')
        (void) strtol(q,&q,10);
      if (*q == 'd')
        {
          char
            format[MaxTextExtent];

          (void) strncpy(format,p,MaxTextExtent-1);
          FormatString(p,format,GetImageIndexInList(image));
          break;
        }
    }
  if (!image_info->adjoin)
    if ((image->previous != (Image *) NULL) ||
        (image->next != (Image *) NULL))
      {
        if (LocaleCompare(filename,image->filename) == 0)
          FormatString(filename,"%.1024s.%lu",image->filename,
                       GetImageIndexInList(image));
        if (image->next != (Image *) NULL)
          (void) strncpy(image->next->magick,image->magick,
                         MaxTextExtent-1);
      }
  (void) strncpy(image->filename,filename,MaxTextExtent-1);
}

MagickExport unsigned int OpenBlob(const ImageInfo *image_info,Image *image,
  const BlobMode mode,ExceptionInfo *exception)
{
  char
    filename[MaxTextExtent],
    *type;

  assert(image_info != (ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  (void) LogMagickEvent(BlobEvent,GetMagickModule(),
    "Opening Blob using %s mode ...",BlobModeToString(mode));

  if (image_info->blob != (void *) NULL)
    {
      /*
        Use existing blob supplied via image_info->blob
      */
      if (image_info->stream != (StreamHandler) NULL)
      {
        /*
          Use existing stream provided via image_info->stream
        */
        image->blob->stream=image_info->stream;
        (void) LogMagickEvent(BlobEvent,GetMagickModule(),
          "  using image_info->stream for blob %p",&image->blob);
      }
      AttachBlob(image->blob,image_info->blob,image_info->length);
      (void) LogMagickEvent(BlobEvent,GetMagickModule(),
        "  attached image_info->blob to blob %p",&image->blob);
      return(True);
    }
  DetachBlob(image->blob);
  switch (mode)
  {
    default: type=(char *) "r"; break;
    case ReadBlobMode: type=(char *) "r"; break;
    case ReadBinaryBlobMode: type=(char *) "rb"; break;
    case WriteBlobMode: type=(char *) "w"; break;
    case WriteBinaryBlobMode: type=(char *) "wb"; break;
    case IOBinaryBlobMode: type=(char *) "w+b"; break;
  }
  if (image_info->stream != (StreamHandler) NULL)
    {
      /*
        Use existing stream provided via image_info->stream and
        return if blob is in write mode.
      */
      image->blob->stream=image_info->stream;
      image->blob->type=FifoStream;
      if (*type == 'w')
      {
        (void) LogMagickEvent(BlobEvent,GetMagickModule(),
          "  opened image_info->stream as FifoStream blob %p",
            &image->blob);
        return(True);
      }
    }
  /*
    Open image file.
  */
  (void) strncpy(filename,image->filename,MaxTextExtent-1);
  if (LocaleCompare(filename,"-") == 0)
    {
      /*
        Handle stdin/stdout stream
      */
      if (*type == 'r')
      {
        image->blob->file=stdin;
        (void) LogMagickEvent(BlobEvent,GetMagickModule(),
          "  using stdin as StandardStream blob %p",
            &image->blob);
      }
      else
      {
        image->blob->file=stdout;
        (void) LogMagickEvent(BlobEvent,GetMagickModule(),
          "  using stdout as StandardStream blob %p",
            &image->blob);
      }
#if defined(WIN32)
      if (strchr(type,'b') != (char *) NULL)
        setmode(_fileno(image->blob->file),_O_BINARY);
#endif
      image->blob->type=StandardStream;
      image->blob->exempt=True;
    }
  else
#if !defined(vms) && !defined(macintosh)
    if (*filename == '|')
      {
        char
          mode[MaxTextExtent];

        /*
          Pipe image to ("w") or from ("r") a system command.
        */
#if !defined(WIN32)
        if (*type == 'w')
          (void) signal(SIGPIPE,SIG_IGN);
#endif
        (void) strncpy(mode,type,1);
        mode[1]='\0';
        image->blob->file=(FILE *) popen(filename+1,mode);
        if (image->blob->file != (FILE *) NULL)
        {
          image->blob->type=PipeStream;
          (void) LogMagickEvent(BlobEvent,GetMagickModule(),
            "  popened \"%s\" as PipeStream blob %p",
              filename+1,&image->blob);
        }
      }
    else
#endif
      {
        if (*type == 'w')
          {
            /*
              Form filename for multi-part images.
            */
            FormMultiPartFilename(image,image_info);
            strcpy(filename,image->filename);
#if defined(macintosh)
            /* What is this for? */
            SetApplicationType(filename,image_info->magick,'8BIM');
#endif
          }
#if defined(HasZLIB)
      if ((strlen(filename) > 3) &&
          ((LocaleCompare(filename+strlen(filename)-3,".gz") == 0) ||
           (LocaleCompare(filename+strlen(filename)-2,".Z") == 0)))
        {
          image->blob->file=(FILE *) gzopen(filename,type);
          if (image->blob->file != (FILE *) NULL)
          {
            image->blob->type=ZipStream;
            (void) LogMagickEvent(BlobEvent,GetMagickModule(),
              "  opened file %s as ZipStream blob %p",
                filename,&image->blob);
          }
        }
      else
#endif
#if defined(HasBZLIB)
        if ((strlen(filename) > 4) &&
            (LocaleCompare(filename+strlen(filename)-4,".bz2") == 0))
          {
            image->blob->file=(FILE *) BZ2_bzopen(filename,type);
            if (image->blob->file != (FILE *) NULL)
            {
              image->blob->type=BZipStream;
              (void) LogMagickEvent(BlobEvent,GetMagickModule(),
              "  opened file %s as BZipStream blob %p",
                filename,&image->blob);
            }
          }
        else
#endif
          if (image_info->file != (FILE *) NULL)
            {
              image->blob->file=image_info->file;
              image->blob->type=FileStream;
              image->blob->exempt=True;
              (void) rewind(image->blob->file);
              (void) LogMagickEvent(BlobEvent,GetMagickModule(),
              "  opened image_info->file (%d) as FileStream blob %p",
                fileno(image_info->file),&image->blob);
            }
          else
            {
              image->blob->file=(FILE *) fopen(filename,type);
              if (image->blob->file != (FILE *) NULL)
                {
                  unsigned char
                    magick[MaxTextExtent];

                  setvbuf(image->blob->file,NULL,_IOFBF,16384);
                  image->blob->type=FileStream;
                  (void) LogMagickEvent(BlobEvent,GetMagickModule(),
                    "  opened file %s as FileStream blob %p",
                      filename,&image->blob);
                  memset((void *) magick,0,MaxTextExtent);
                  (void) fread(magick,MaxTextExtent,1,image->blob->file);
                  (void) rewind(image->blob->file);
#if defined(HasZLIB)
                  if ((magick[0] == 0x1F) && (magick[1] == 0x8B) &&
                      (magick[2] == 0x08))
                    {
                      (void) fclose(image->blob->file);
                      image->blob->file=(FILE *) gzopen(filename,type);
                      if (image->blob->file != (FILE *) NULL)
                      {
                        image->blob->type=ZipStream;
                        (void) LogMagickEvent(BlobEvent,GetMagickModule(),
                          "  reopened file %s as ZipStream blob %p",
                            filename,&image->blob);
                      }
                     }
#endif
#if defined(HasBZLIB)
                  if (strncmp((char *) magick,"BZh",3) == 0)
                    {
                      (void) fclose(image->blob->file);
                      image->blob->file=(FILE *) BZ2_bzopen(filename,type);
                      if (image->blob->file != (FILE *) NULL)
                      {
                        image->blob->type=BZipStream;
                        (void) LogMagickEvent(BlobEvent,GetMagickModule(),
                          "  reopened file %s as BZipStream blob %p",
                            filename,&image->blob);
                      }
                    }
#endif
                }
            }
        if ((image->blob->type == FileStream) && (*type == 'r'))
          {
            const MagickInfo
              *magick_info;

            struct stat
              attributes;

            magick_info=GetMagickInfo(image_info->magick,&image->exception);
            if ((magick_info != (const MagickInfo *) NULL) &&
                magick_info->blob_support)
              if ((fstat(fileno(image->blob->file),&attributes) >= 0) &&
                  (attributes.st_size > MinBlobExtent) &&
                  (attributes.st_size == (size_t) attributes.st_size))
                {
                  size_t
                    length;

                  void
                    *blob;

                  length=(size_t) attributes.st_size;
                  blob=MapBlob(fileno(image->blob->file),ReadMode,0,length);
                  if (blob != (void *) NULL)
                    {
                      /*
                        Format supports blobs-- use memory-mapped I/O.
                      */
                      if (image_info->file != (FILE *) NULL)
                        image->blob->exempt=False;
                      else
                        (void) fclose(image->blob->file);
                      AttachBlob(image->blob,blob,length);
                      image->blob->mapped=True;
                    }
              }
          }
      }
  image->blob->status=False;
  if (image->blob->type != UndefinedStream)
    image->blob->size=GetBlobSize(image);
  if (*type == 'r')
    {
      image->next=(Image *) NULL;
      image->previous=(Image *) NULL;
    }
  return(image->blob->type != UndefinedStream);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   P i n g B l o b                                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  PingBlob() returns all the attributes of an image or image sequence except
%  for the pixels.  It is much faster and consumes far less memory than
%  BlobToImage().  On failure, a NULL image is returned and exception
%  describes the reason for the failure.
%
%
%  The format of the PingBlob method is:
%
%      Image *PingBlob(const ImageInfo *image_info,const void *blob,
%        const size_t length,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: The image info.
%
%    o blob: The address of a character stream in one of the image formats
%      understood by ImageMagick.
%
%    o length: This size_t integer reflects the length in bytes of the blob.
%
%    o exception: Return any errors or warnings in this structure.
%
%
%
*/

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

static unsigned int PingStream(const Image *image,const void *pixels,
  const size_t columns)
{
  return(True);
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

MagickExport Image *PingBlob(const ImageInfo *image_info,const void *blob,
  const size_t length,ExceptionInfo *exception)
{
  Image
    *image;

  ImageInfo
    *clone_info;

  assert(image_info != (ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  assert(exception != (ExceptionInfo *) NULL);
  SetExceptionInfo(exception,UndefinedException);
  if ((blob == (const void *) NULL) || (length == 0))
    {
      ThrowException(exception,BlobError,"UnrecognizedImageFormat",
        image_info->magick);
      return((Image *) NULL);
    }
  clone_info=CloneImageInfo(image_info);
  clone_info->blob=(void *) blob;
  clone_info->length=length;
  clone_info->ping=True;
  if (clone_info->size == (char *) NULL)
    clone_info->size=AllocateString(DefaultTileGeometry);
  image=ReadStream(clone_info,&PingStream,exception);
  DestroyImageInfo(clone_info);
  return(image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  R e a d B l o b                                                            %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadBlob() reads data from the blob or image file and returns it.  It
%  returns the number of bytes read.
%
%  The format of the ReadBlob method is:
%
%      size_t ReadBlob(Image *image,const size_t length,void *data)
%
%  A description of each parameter follows:
%
%    o count:  Method ReadBlob returns the number of bytes read.
%
%    o image: The image.
%
%    o length:  Specifies an integer representing the number of bytes
%      to read from the file.
%
%    o data:  Specifies an area to place the information requested from
%      the file.
%
%
*/
MagickExport size_t ReadBlob(Image *image,const size_t length,void *data)
{
  size_t
    count;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  assert(image->blob != (BlobInfo *) NULL);
  assert(image->blob->type != UndefinedStream);
  assert(data != (void *) NULL);

  count=0;
  switch (image->blob->type)
  {
    case UndefinedStream:
      break;
    case FileStream:
    case StandardStream:
    case PipeStream:
    {
      if (length == 1)
        {
          unsigned int
            c;

          if ((c=getc(image->blob->file)) != EOF)
            {
              *((unsigned char *)data)=c;
              count=1;
            }
          else
            {
              count=0;
            }
        }
      else
        {
          count=fread(data,1,length,image->blob->file);
        }
      break;
    }
    case ZipStream:
    {
#if defined(HasZLIB)
      count=gzread(image->blob->file,data,length);
#endif
      break;
    }
    case BZipStream:
    {
#if defined(HasBZLIB)
      count=BZ2_bzread(image->blob->file,data,length);
#endif
      break;
    }
    case FifoStream:
      break;
    case BlobStream:
    {
      const unsigned char
        *source;

      count=ReadBlobStream(image,length,(void *) (&source));
      if (count <= 10)
        {
          register size_t
            i;

          register unsigned char
            *target=(unsigned char*) data;

          for(i=count; i > 0; i--)
            {
              *target=*source;
              target++;
              source++;
            }
        }
      else 
        (void) memcpy(data,source,count);
      break;
    }
  }
  return(count);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  R e a d B l o b Z C                                                        %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadBlobZC() reads data from the blob or image file and returns it.  It
%  returns the number of bytes read.  Provision is made for a "zero-copy"
%  transfer if the blob data is already in memory.
%
%  This method is currently EXPERIMENTAL!
%
%  The format of the ReadBlob method is:
%
%      size_t ReadBlob(Image *image,const size_t length,void *data)
%
%  A description of each parameter follows:
%
%    o count:  Method ReadBlob returns the number of bytes read.
%
%    o image: The image.
%
%    o length:  Specifies an integer representing the number of bytes
%      to read from the file.
%
%    o data:  Specifies an area to place the information requested from
%      the file.  If the data may be accessed without a copy, then
%      the provided pointer is updated to point to the location of
%      the data in memory, and no copy is performed.
%
%
*/
MagickExport  size_t ReadBlobZC(Image *image,const size_t length,void **data)
{
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  assert(image->blob != (BlobInfo *) NULL);
  assert(image->blob->type != UndefinedStream);
  assert(data != (void *) NULL);

  if (image->blob->type == BlobStream)
    {
      return (ReadBlobStream(image,length,data));
    }
  assert(*data != (void *) NULL);
  return ReadBlob(image,length,*data);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  R e a d B l o b B y t e                                                    %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method ReadBlobByte reads a single byte from the image file and returns it.
%
%  The format of the ReadBlobByte method is:
%
%      int ReadBlobByte(Image *image)
%
%  A description of each parameter follows.
%
%    o value: Method ReadBlobByte returns an integer read from the file.
%
%    o image: The image.
%
%
*/
MagickExport int ReadBlobByte(Image *image)
{
  unsigned char
    c;
  
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  
  switch (image->blob->type)
    {
    case FileStream:
    case StandardStream:
    case PipeStream:
      {
        return getc(image->blob->file);
      }
    case BlobStream:
      {
        if (image->blob->offset < image->blob->length)
          {
            c=*((unsigned char *)image->blob->data+image->blob->offset);
            image->blob->offset++;
            return (c);
          }
        image->blob->eof=True;
        break;
      }
    default:
      {
        /* Do things the slow way */
        if (ReadBlob(image,1,&c) == 1)
          return (c);
      }
    }
  return(EOF);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  R e a d B l o b L S B L o n g                                              %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method ReadBlobLSBLong reads a long value as a 32 bit quantity in
%  least-significant byte first order.
%
%  The format of the ReadBlobLSBLong method is:
%
%      unsigned long ReadBlobLSBLong(Image *image)
%
%  A description of each parameter follows.
%
%    o value:  Method ReadBlobLSBLong returns an unsigned long read from
%      the file.
%
%    o image: The image.
%
%
*/
MagickExport unsigned long ReadBlobLSBLong(Image *image)
{
  unsigned char
    buffer[4],
    *source;

  unsigned long
    value;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->blob->type == BlobStream)
    {
      value=ReadBlobStream(image,4,(void **)&source);
    }
  else
    {
      value=ReadBlob(image,4,(void *)buffer);
      source=buffer;
    }
  if (value < 4)
    return((unsigned long) ~0);
  value=source[3] << 24;
  value|=source[2] << 16;
  value|=source[1] << 8;
  value|=source[0];
  return(value);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  R e a d B l o b L S B S h o r t                                            %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method ReadBlobLSBShort reads a short value as a 16 bit quantity in
%  least-significant byte first order.
%
%  The format of the ReadBlobLSBShort method is:
%
%      unsigned short ReadBlobLSBShort(Image *image)
%
%  A description of each parameter follows.
%
%    o value:  Method ReadBlobLSBShort returns an unsigned short read from
%      the file.
%
%    o image: The image.
%
%
*/
MagickExport unsigned short ReadBlobLSBShort(Image *image)
{
  unsigned char
    buffer[2],
    *source;

  unsigned short
    value;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->blob->type == BlobStream)
    {
      value=ReadBlobStream(image,2,(void **)&source);
    }
  else
    {
      value=ReadBlob(image,2,(void *)buffer);
      source=buffer;
    }
  if (value < 2)
    return((unsigned short) ~0);
  value=source[1] << 8;
  value|=source[0];
  return(value);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  R e a d B l o b M S B L o n g                                              %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadBlobMSBLong() reads a long value as a 32 bit quantity in
%  most-significant byte first order.
%
%  The format of the ReadBlobMSBLong method is:
%
%      unsigned long ReadBlobMSBLong(Image *image)
%
%  A description of each parameter follows.
%
%    o value:  Method ReadBlobMSBLong returns an unsigned long read from
%      the file.
%
%    o image: The image.
%
%
%
*/
MagickExport unsigned long ReadBlobMSBLong(Image *image)
{
  unsigned char
    buffer[4],
    *source;

  unsigned long
    value;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->blob->type == BlobStream)
    {
      value=ReadBlobStream(image,4,(void **)&source);
    }
  else
    {
      value=ReadBlob(image,4,(void *)buffer);
      source=buffer;
    }
  if (value < 4)
    return((unsigned long) ~0);
  value=source[0] << 24;
  value|=source[1] << 16;
  value|=source[2] << 8;
  value|=source[3];
  return(value);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  R e a d B l o b M S B S h o r t                                            %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method ReadBlobMSBShort reads a short value as a 16 bit quantity in
%  most-significant byte first order.
%
%  The format of the ReadBlobMSBShort method is:
%
%      unsigned short ReadBlobMSBShort(Image *image)
%
%  A description of each parameter follows.
%
%    o value:  Method ReadBlobMSBShort returns an unsigned short read from
%      the file.
%
%    o image: The image.
%
%
*/
MagickExport unsigned short ReadBlobMSBShort(Image *image)
{
  unsigned char
    buffer[2],
    *source;

  unsigned short
    value;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->blob->type == BlobStream)
    {
      value=ReadBlobStream(image,2,(void **)&source);
    }
  else
    {
      value=ReadBlob(image,2,(void *)buffer);
      source=buffer;
    }
  if (value < 2)
    return((unsigned short) ~0);
  value=source[0] << 8;
  value|=source[1];
  return(value);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   R e a d B l o b S t r i n g                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadBlobString() reads characters from a blob or file until a newline
%  character is read or an end-of-file condition is encountered.
%
%  The format of the ReadBlobString method is:
%
%      char *ReadBlobString(Image *image,char *string)
%
%  A description of each parameter follows:
%
%    o status:  Method ReadBlobString returns the string on success, otherwise,
%      a null is returned.
%
%    o image: The image.
%
%    o string: The address of a character buffer.
%
%
*/
MagickExport char *ReadBlobString(Image *image,char *string)
{
  int
    c;

  register long
    i;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  for (i=0; i < (MaxTextExtent-1); i++)
  {
    c=ReadBlobByte(image);
    if (c == EOF)
      {
        if (i == 0)
          return((char *) NULL);
        break;
      }
    string[i]=c;
    if ((string[i] == '\n') || (string[i] == '\r'))
      break;
  }
  string[i]='\0';
  return(string);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e f e r e n c e B l o b                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReferenceBlob() increments the reference count associated with the pixel
%  blob returning a pointer to the blob.
%
%  The format of the ReferenceBlob method is:
%
%      BlobInfo ReferenceBlob(BlobInfo *blob_info)
%
%  A description of each parameter follows:
%
%    o blob_info: The blob_info.
%
%
*/
MagickExport BlobInfo *ReferenceBlob(BlobInfo *blob)
{
  assert(blob != (BlobInfo *) NULL);
  assert(blob->signature == MagickSignature);
  AcquireSemaphoreInfo(&blob->semaphore);
  blob->reference_count++;
  LiberateSemaphoreInfo(&blob->semaphore);
  return(blob);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  S e e k B l o b                                                            %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SeekBlob() sets the offset in bytes from the beginning of a blob or file
%  and returns the resulting offset.
%
%  The format of the SeekBlob method is:
%
%      ExtendedSignedIntegralType SeekBlob(Image *image,
%        const ExtendedSignedIntegralType offset,const int whence)
%
%  A description of each parameter follows:
%
%    o offset:  Method SeekBlob returns the offset from the beginning
%      of the file or blob.
%
%    o image: The image.
%
%    o offset:  Specifies an integer representing the offset in bytes.
%
%    o whence:  Specifies an integer representing how the offset is
%      treated relative to the beginning of the blob as follows:
%
%        SEEK_SET  Set position equal to offset bytes.
%        SEEK_CUR  Set position to current location plus offset.
%        SEEK_END  Set position to EOF plus offset.
%
%
*/
MagickExport ExtendedSignedIntegralType SeekBlob(Image *image,
  const ExtendedSignedIntegralType offset,const int whence)
{
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  assert(image->blob != (BlobInfo *) NULL);
  assert(image->blob->type != UndefinedStream);
  switch (image->blob->type)
  {
    case UndefinedStream:
      break;
    case FileStream:
    {
      if (fseek(image->blob->file,(off_t) offset,whence) < 0)
        return(-1);
      image->blob->offset=TellBlob(image);
      break;
    }
    case StandardStream:
    case PipeStream:
    case ZipStream:
    {
#if defined(HasZLIB)
      if (gzseek(image->blob->file,(off_t) offset,whence) < 0)
        return(-1);
#endif
      image->blob->offset=TellBlob(image);
      break;
    }
    case BZipStream:
      return(-1);
    case FifoStream:
      return(-1);
    case BlobStream:
    {
      switch (whence)
      {
        case SEEK_SET:
        default:
        {
          if (offset < 0)
            return(-1);
          image->blob->offset=offset;
          break;
        }
        case SEEK_CUR:
        {
          if ((image->blob->offset+offset) < 0)
            return(-1);
          image->blob->offset+=offset;
          break;
        }
        case SEEK_END:
        {
          if ((ExtendedSignedIntegralType)
              (image->blob->offset+image->blob->length+offset) < 0)
            return(-1);
          image->blob->offset=image->blob->length+offset;
          break;
        }
      }
      if (image->blob->offset <= (ExtendedSignedIntegralType)
          image->blob->length)
        image->blob->eof=False;
      else
        if (image->blob->mapped)
          return(-1);
        else
          {
            image->blob->extent=image->blob->offset+image->blob->quantum;
            ReacquireMemory((void **) &image->blob->data,image->blob->extent+1);
            (void) SyncBlob(image);
            if (image->blob->data == (unsigned char *) NULL)
              {
                DetachBlob(image->blob);
                return(-1);
              }
          }
      break;
    }
  }
  return(image->blob->offset);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  S y n c B l o b                                                            %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SyncBlob() flushes the datastream if it is a file or synchonizes the data
%  attributes if it is an blob.
%
%  The format of the SyncBlob method is:
%
%      int SyncBlob(Image *image)
%
%  A description of each parameter follows:
%
%    o status:  Method SyncBlob returns 0 on success; otherwise,  it
%      returns -1 and set errno to indicate the error.
%
%    o image: The image.
%
%
*/
MagickExport int SyncBlob(Image *image)
{
  int
    status;

  register Image
    *p;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  assert(image->blob != (BlobInfo *) NULL);
  assert(image->blob->type != UndefinedStream);
  for (p=image; p->previous != (Image *) NULL; p=p->previous);
  for ( ; p->next != (Image *) NULL; p=p->next)
    *p->blob=(*image->blob);
  status=0;
  switch (image->blob->type)
  {
    case UndefinedStream:
      break;
    case FileStream:
    case StandardStream:
    case PipeStream:
    {
      status=fflush(image->blob->file);
      break;
    }
    case ZipStream:
    {
#if defined(HasZLIB)
      status=gzflush(image->blob->file,Z_SYNC_FLUSH);
#endif
      break;
    }
    case BZipStream:
    {
#if defined(HasBZLIB)
      status=BZ2_bzflush(image->blob->file);
#endif
      break;
    }
    case FifoStream:
    case BlobStream:
      break;
  }
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  T e l l B l o b                                                            %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  TellBlob() obtains the current value of the blob or file position.
%
%  The format of the TellBlob method is:
%
%      ExtendedSignedIntegralType TellBlob(const Image *image)
%
%  A description of each parameter follows:
%
%    o offset:  Method TellBlob returns the current value of the blob or
%      file position success; otherwise, it returns -1 and sets errno to
%      indicate the error.
%
%    o image: The image.
%
%
*/
MagickExport ExtendedSignedIntegralType TellBlob(const Image *image)
{
  ExtendedSignedIntegralType
    offset;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  assert(image->blob != (BlobInfo *) NULL);
  assert(image->blob->type != UndefinedStream);
  offset=(-1);
  switch (image->blob->type)
  {
    case UndefinedStream:
      break;
    case FileStream:
    {
      offset=ftell(image->blob->file);
      break;
    }
    case StandardStream:
    case PipeStream:
    case ZipStream:
    {
#if defined(HasZLIB)
      offset=gztell(image->blob->file);
#endif
      break;
    }
    case BZipStream:
      break;
    case FifoStream:
      break;
    case BlobStream:
    {
      offset=image->blob->offset;
      break;
    }
  }
  return(offset);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  U n m a p B l o b                                                          %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  UnmapBlob() deallocates the binary large object previously allocated with
%  the MapBlob method.
%
%  The format of the UnmapBlob method is:
%
%      unsigned int UnmapBlob(void *map,const size_t length)
%
%  A description of each parameter follows:
%
%    o status:  Method UnmapBlob returns True on success; otherwise,  it
%      returns False and sets errno to indicate the error.
%
%    o map: The address  of the binary large object.
%
%    o length: The length of the binary large object.
%
%
*/
MagickExport unsigned int UnmapBlob(void *map,const size_t length)
{
#if defined(HAVE_MMAP)
  int
    status;

  (void) LogMagickEvent(BlobEvent,GetMagickModule(),
    "Munmap file mapping at address 0x%p and length %lu",
    map,(unsigned long)length);
  status=munmap(map,length);
  return(status == 0);
#else
  return(False);
#endif
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  W r i t e B l o b                                                          %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WriteBlob() writes data to a blob or image file.  It returns the number of
%  bytes written.
%
%  The format of the WriteBlob method is:
%
%      size_t WriteBlob(Image *image,const size_t length,const void *data)
%
%  A description of each parameter follows:
%
%    o count:  Method WriteBlob returns the number of bytes written to the
%      blob.
%
%    o image: The image.
%
%    o length:  Specifies an integer representing the number of bytes to
%      write to the file.
%
%    o data:  The address of the data to write to the blob or file.
%
%
*/
MagickExport size_t WriteBlob(Image *image,const size_t length,const void *data)
{
  size_t
    count;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  assert(data != (const char *) NULL);
  assert(image->blob != (BlobInfo *) NULL);
  assert(image->blob->type != UndefinedStream);
  count=length;
  switch (image->blob->type)
  {
    case UndefinedStream:
      break;
    case FileStream:
    case StandardStream:
    case PipeStream:
    {
      if (length == 1)
        {
          if((putc((int)*((unsigned char *)data),image->blob->file)) != EOF)
            count=1;
          else
            count=0;
        }
      else
        {
          count=fwrite((char *) data,1,length,image->blob->file);
        }
      break;
    }
    case ZipStream:
    {
#if defined(HasZLIB)
      count=gzwrite(image->blob->file,(void *) data,length);
#endif
      break;
    }
    case BZipStream:
    {
#if defined(HasBZLIB)
      count=BZ2_bzwrite(image->blob->file,(void *) data,length);
#endif
      break;
    }
    case FifoStream:
    {
      count=image->blob->stream(image,data,length);
      break;
    }
    case BlobStream:
    {
      void
        *dest;

      if ((image->blob->offset+length) >= image->blob->extent)
        {
          if (image->blob->mapped)
            return(0);
          image->blob->quantum<<=1;
          image->blob->extent+=length+image->blob->quantum;
          ReacquireMemory((void **) &image->blob->data,image->blob->extent+1);
          (void) SyncBlob(image);
          if (image->blob->data == (unsigned char *) NULL)
            {
              DetachBlob(image->blob);
              return(0);
            }
        }

      dest=image->blob->data+image->blob->offset;
      if (length <= 10)
        {
          register size_t
            i;

          register const unsigned char
            *source=(const unsigned char*) data;

          register unsigned char
            *target=(unsigned char*) dest;

          for(i=length; i > 0; i--)
            {
              *target=*source;
              target++;
              source++;
            }
        }
      else
        {
          (void) memcpy(dest,data,length);
        }
      image->blob->offset+=length;
      if (image->blob->offset > (ExtendedSignedIntegralType)
          image->blob->length)
        image->blob->length=image->blob->offset;
      count=length;
    }
  }
  return(count);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  W r i t e B l o b B y t e                                                  %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method WriteBlobByte write an integer to a blob.  It returns the number of
%  bytes written (either 0 or 1);
%
%  The format of the WriteBlobByte method is:
%
%      size_t WriteBlobByte(Image *image,const unsigned long value)
%
%  A description of each parameter follows.
%
%    o count:  Method WriteBlobByte returns the number of bytes written.
%
%    o image: The image.
%
%    o value: Specifies the value to write.
%
%
*/
MagickExport size_t WriteBlobByte(Image *image,const unsigned long value)
{
  unsigned char
    c;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);

  switch (image->blob->type)
    {
    case FileStream:
    case StandardStream:
    case PipeStream:
      {
        if(putc((int)value,image->blob->file) != EOF)
          return 1;
        return 0;
      }
      /* case BlobStream: TBD */
    default:
      {
        c=(unsigned char) value;
        return(WriteBlob(image,1,&c));
      }
    }

}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  W r i t e B l o b L S B L o n g                                            %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method WriteBlobLSBLong writes a long value as a 32 bit quantity in
%  least-significant byte first order.
%
%  The format of the WriteBlobLSBLong method is:
%
%      size_t WriteBlobLSBLong(Image *image,const unsigned long value)
%
%  A description of each parameter follows.
%
%    o count: Method WriteBlobLSBLong returns the number of unsigned longs
%      written.
%
%    o image: The image.
%
%    o value: Specifies the value to write.
%
%
*/
MagickExport size_t WriteBlobLSBLong(Image *image,const unsigned long value)
{
  unsigned char
    buffer[4];

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  buffer[0]=(unsigned char) value;
  buffer[1]=(unsigned char) (value >> 8);
  buffer[2]=(unsigned char) (value >> 16);
  buffer[3]=(unsigned char) (value >> 24);
  return(WriteBlob(image,4,buffer));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   W r i t e B l o b L S B S h o r t                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method WriteBlobLSBShort writes a long value as a 16 bit quantity in
%  least-significant byte first order.
%
%  The format of the WriteBlobLSBShort method is:
%
%      size_t WriteBlobLSBShort(Image *image,const unsigned long value)
%
%  A description of each parameter follows.
%
%    o count: Method WriteBlobLSBShort returns the number of unsigned longs
%      written.
%
%    o image: The image.
%
%    o value:  Specifies the value to write.
%
%
*/
MagickExport size_t WriteBlobLSBShort(Image *image,const unsigned long value)
{
  unsigned char
    buffer[2];

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  buffer[0]=(unsigned char) value;
  buffer[1]=(unsigned char) (value >> 8);
  return(WriteBlob(image,2,buffer));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  W r i t e B l o b M S B L o n g                                            %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method WriteBlobMSBLong writes a long value as a 32 bit quantity in
%  most-significant byte first order.
%
%  The format of the WriteBlobMSBLong method is:
%
%      size_t WriteBlobMSBLong(Image *image,const unsigned long value)
%
%  A description of each parameter follows.
%
%    o count: Method WriteBlobMSBLong returns the number of unsigned longs
%      written.
%
%    o value:  Specifies the value to write.
%
%    o image: The image.
%
%
*/
MagickExport size_t WriteBlobMSBLong(Image *image,const unsigned long value)
{
  unsigned char
    buffer[4];

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  buffer[0]=(unsigned char) (value >> 24);
  buffer[1]=(unsigned char) (value >> 16);
  buffer[2]=(unsigned char) (value >> 8);
  buffer[3]=(unsigned char) value;
  return(WriteBlob(image,4,buffer));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  W r i t e B l o b M S B S h o r t                                          %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method WriteBlobMSBShort writes a long value as a 16 bit quantity in
%  most-significant byte first order.
%
%  The format of the WriteBlobMSBShort method is:
%
%      size_t WriteBlobMSBShort(Image *image,const unsigned long value)
%
%  A description of each parameter follows.
%
%   o  value:  Specifies the value to write.
%
%   o  file:  Specifies the file to write the data to.
%
%
*/
MagickExport size_t WriteBlobMSBShort(Image *image,const unsigned long value)
{
  unsigned char
    buffer[2];

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  buffer[0]=(unsigned char) (value >> 8);
  buffer[1]=(unsigned char) value;
  return(WriteBlob(image,2,buffer));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  W r i t e B l o b S t r i n g                                              %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method WriteBlobString write a string to a blob.  It returns the number of
%  characters written.
%
%  The format of the WriteBlobString method is:
%
%      size_t WriteBlobString(Image *image,const char *string)
%
%  A description of each parameter follows.
%
%    o count:  Method WriteBlobString returns the number of characters written.
%
%    o image: The image.
%
%    o string: Specifies the string to write.
%
%
*/
MagickExport size_t WriteBlobString(Image *image,const char *string)
{
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  assert(string != (const char *) NULL);
  return(WriteBlob(image,strlen(string),string));
}
