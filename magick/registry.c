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
%           RRRR    EEEEE    GGG   IIIII  SSSSS  TTTTT  RRRR   Y   Y          %
%           R   R   E       G        I    SS       T    R   R   Y Y           %
%           RRRR    EEE     G GGG    I     SSS     T    RRRR     Y            %
%           R R     E       G   G    I       SS    T    R R      Y            %
%           R  R    EEEEE    GGG   IIIII  SSSSS    T    R  R     Y            %
%                                                                             %
%                                                                             %
%                          GraphicsMagick Registry.                           %
%                                                                             %
%                                                                             %
%                              Software Design                                %
%                                John Cristy                                  %
%                                October 2001                                 %
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
#include "magick/registry.h"
#include "magick/utility.h"

/*
  Global declarations.
*/
static SemaphoreInfo
  *registry_semaphore = (SemaphoreInfo *) NULL;

static long
  id = 0;

static RegistryInfo
  *registry_list = (RegistryInfo *) NULL;

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D e l e t e M a g i c k R e g i s t r y                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DeleteMagickRegistry() deletes an entry in the registry as defined by the id.
%  It returns True if the entry is deleted otherwise False if no entry is found
%  in the registry that matches the id.
%
%  The format of the DeleteMagickRegistry method is:
%
%      unsigned int DeleteMagickRegistry(const long id)
%
%  A description of each parameter follows:
%
%    o id: The registry id.
%
%
*/
MagickExport unsigned int DeleteMagickRegistry(const long id)
{
  register RegistryInfo
    *p;

  RegistryInfo
    *registry_info;

  AcquireSemaphoreInfo(&registry_semaphore);
  for (p=registry_list; p != (RegistryInfo *) NULL; p=p->next)
  {
    if (id != p->id)
      continue;
    registry_info=p;
    switch (registry_info->type)
    {
      case ImageRegistryType:
      {
        DestroyImage((Image *) registry_info->blob);
        break;
      }
      case ImageInfoRegistryType:
      {
        DestroyImageInfo((ImageInfo *) registry_info->blob);
        break;
      }
      default:
      {
        LiberateMemory((void **) &registry_info->blob);
        break;
      }
    }
    if (registry_info == registry_list)
      registry_list=registry_info->next;
    if (registry_info->previous != (RegistryInfo *) NULL)
      registry_info->previous->next=registry_info->next;
    if (registry_info->next != (RegistryInfo *) NULL)
      registry_info->next->previous=registry_info->previous;
    LiberateMemory((void **) &registry_info);
    registry_info=(RegistryInfo *) NULL;
    break;
  }
  LiberateSemaphoreInfo(&registry_semaphore);
  return(p != (RegistryInfo *) NULL);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   D e s t r o y M a g i c k R e g i s t r y                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DestroyMagickRegistry() deallocates memory associated the magick registry.
%
%  The format of the DestroyMagickRegistry method is:
%
%       void DestroyMagickRegistry(void)
%
%
*/
MagickExport void DestroyMagickRegistry(void)
{
  register RegistryInfo
    *p;

  RegistryInfo
    *registry_info;

  AcquireSemaphoreInfo(&registry_semaphore);
  for (p=registry_list; p != (RegistryInfo *) NULL; )
  {
    registry_info=p;
    p=p->next;
    switch (registry_info->type)
    {
      case ImageRegistryType:
      {
        DestroyImage((Image *) registry_info->blob);
        break;
      }
      case ImageInfoRegistryType:
      {
        DestroyImageInfo((ImageInfo *) registry_info->blob);
        break;
      }
      default:
      {
        LiberateMemory((void **) &registry_info->blob);
        break;
      }
    }
    LiberateMemory((void **) &registry_info);
  }
  registry_list=(RegistryInfo *) NULL;
  DestroySemaphoreInfo(&registry_semaphore);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t I m a g e F r o m M a g i c k R e g i s t y                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetImageFromMagickRegistry() gets an image from the registry as defined by
%  its name.  If the blob that matches the name is not found, NULL is returned.
%
%  The format of the GetImageFromMagickRegistry method is:
%
%      Image *GetImageFromMagickRegistry(const char *name,long *id,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o name: The name of the image to retrieve from the registry.
%
%    o id: The registry id.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport Image *GetImageFromMagickRegistry(const char *name,long *id,
  ExceptionInfo *exception)
{
  Image
    *image;

  register RegistryInfo
    *p;

  *id=(-1);
  image=(Image *) NULL;
  AcquireSemaphoreInfo(&registry_semaphore);
  for (p=registry_list; p != (RegistryInfo *) NULL; p=p->next)
  {
    if (p->type != ImageRegistryType)
      continue;
    if (LocaleCompare(((Image *) p->blob)->filename,name) == 0)
      {
        *id=p->id;
        image=CloneImageList((Image *) p->blob,exception);
        break;
      }
  }
  LiberateSemaphoreInfo(&registry_semaphore);
  if (image == (Image *) NULL)
    ThrowException(exception,RegistryError,"UnableToLocateImage",name);
  return(image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t M a g i c k R e g i s t r y                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetMagickRegistry() gets a blob from the registry as defined by the id.  If
%  the blob that matches the id is not found, NULL is returned.
%
%  The format of the GetMagickRegistry method is:
%
%      const void *GetMagickRegistry(const long id,RegistryType *type,
%        size_t *length,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o id: The registry id.
%
%    o type: The registry type.
%
%    o length: The blob length in number of bytes.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport void *GetMagickRegistry(const long id,RegistryType *type,
  size_t *length,ExceptionInfo *exception)
{
  register RegistryInfo
    *p;

  void
    *blob;

  RegistryInfo
    *registry_info;

  blob=(void *) NULL;
  *type=UndefinedRegistryType;
  *length=0;
  AcquireSemaphoreInfo(&registry_semaphore);
  for (p=registry_list; p != (RegistryInfo *) NULL; p=p->next)
  {
    if (id != p->id)
      continue;
    registry_info=p;
    switch (registry_info->type)
    {
      case ImageRegistryType:
      {
        Image
          *image;

        image=(Image *) registry_info->blob;
        blob=(void *) CloneImageList(image,exception);
        break;
      }
      case ImageInfoRegistryType:
      {
        ImageInfo
          *image_info;

        image_info=(ImageInfo *) registry_info->blob;
        blob=(void *) CloneImageInfo(image_info);
        break;
      }
      default:
      {
        blob=(void *) AcquireMemory(registry_info->length);
        if (blob == (void *) NULL)
          {
            ThrowException(exception,ResourceLimitError,
              "MemoryAllocationFailed","UnableToGetFromRegistry");
            break;
          }
        (void) memcpy(blob,registry_info->blob,registry_info->length);
        break;
      }
    }
    *type=registry_info->type;
    *length=registry_info->length;
    break;
  }
  LiberateSemaphoreInfo(&registry_semaphore);
  if (blob == (void *) NULL)
    {
      char
        description[MaxTextExtent];

      FormatString(description,"id=%ld",id);
      ThrowException(exception,RegistryError,"UnableToLocateRegistryID",
        description);
    }
  return(blob);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S e t M a g i c k R e g i s t r y                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetMagickRegistry() sets a blob into the registry and returns a unique ID.
%  If an error occurs, -1 is returned.
%
%  The format of the SetMagickRegistry method is:
%
%      long SetMagickRegistry(const RegistryType type,const void *blob,
%        const size_t length,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o type: The registry type.
%
%    o blob: The address of a Binary Large OBject.
%
%    o length: For a registry type of ImageRegistryType use sizeof(Image)
%      otherise the blob length in number of bytes.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport long SetMagickRegistry(const RegistryType type,const void *blob,
  const size_t length,ExceptionInfo *exception)
{
  RegistryInfo
    *registry_info;

  void
    *clone_blob;

  switch (type)
  {
    case ImageRegistryType:
    {
      Image
        *image;

      image=(Image *) blob;
      if (length != sizeof(Image))
        {
          ThrowException(exception,RegistryError,"UnableToSetRegistry",
            "StructureSizeMismatch");
          return(-1);
        }
      if (image->signature != MagickSignature)
        {
          ThrowException(exception,RegistryError,"UnableToSetRegistry",
            "ImageExpected");
          return(-1);
        }
      clone_blob=(void *) CloneImageList(image,exception);
      if (clone_blob == (void *) NULL)
        return(-1);
      break;
    }
    case ImageInfoRegistryType:
    {
      ImageInfo
        *image_info;

      image_info=(ImageInfo *) blob;
      if (length != sizeof(ImageInfo))
        {
          ThrowException(exception,RegistryError,"UnableToSetRegistry",
            "StructureSizeMismatch");
          return(-1);
        }
      if (image_info->signature != MagickSignature)
        {
          ThrowException(exception,RegistryError,"UnableToSetRegistry",
            "ImageInfoExpected");
          return(-1);
        }
      clone_blob=(void *) CloneImageInfo(image_info);
      if (clone_blob == (void *) NULL)
        return(-1);
      break;
    }
    default:
    {
      clone_blob=(void *) AcquireMemory(length);
      if (clone_blob == (void *) NULL)
        return(-1);
      (void) memcpy(clone_blob,blob,length);
    }
  }
  registry_info=(RegistryInfo *) AcquireMemory(sizeof(RegistryInfo));
  if (registry_info == (RegistryInfo *) NULL)
    MagickFatalError(ResourceLimitFatalError,"MemoryAllocationFailed",
      "UnableToAllocateRegistryInfo");
  (void) memset(registry_info,0,sizeof(RegistryInfo));
  registry_info->type=type;
  registry_info->blob=clone_blob;
  registry_info->length=length;
  registry_info->signature=MagickSignature;
  AcquireSemaphoreInfo(&registry_semaphore);
  registry_info->id=id++;
  if (registry_list == (RegistryInfo *) NULL)
    registry_list=registry_info;
  else
    {
      register RegistryInfo
        *p;

      for (p=registry_list; p->next != (RegistryInfo *) NULL; p=p->next);
      registry_info->previous=p;
      p->next=registry_info;
    }
  LiberateSemaphoreInfo(&registry_semaphore);
  return(registry_info->id);
}
