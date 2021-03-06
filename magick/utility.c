/*
% Copyright (C) 2003 GraphicsMagick Group
% Copyright (c) 2000 Markus Friedl.  All rights reserved.
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
%             U   U  TTTTT  IIIII  L      IIIII  TTTTT  Y   Y                 %
%             U   U    T      I    L        I      T     Y Y                  %
%             U   U    T      I    L        I      T      Y                   %
%             U   U    T      I    L        I      T      Y                   %
%              UUU     T    IIIII  LLLLL  IIIII    T      Y                   %
%                                                                             %
%                                                                             %
%                     GraphicsMagick Utility Methods                          %
%                                                                             %
%                                                                             %
%                             Software Design                                 %
%                               John Cristy                                   %
%                              January 1993                                   %
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
#include "magick/color.h"
#include "magick/log.h"
#include "magick/magick.h"
#include "magick/signature.h"
#include "magick/tempfile.h"
#include "magick/utility.h"

/*
  Static declarations.
*/
static const char
  Base64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static unsigned char
  AsciiMap[] =
  {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b,
    0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23,
    0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b,
    0x3c, 0x3d, 0x3e, 0x3f, 0x40, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
    0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x70, 0x71, 0x72, 0x73,
    0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
    0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b,
    0x6c, 0x6d, 0x6e, 0x6f, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
    0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f, 0x80, 0x81, 0x82, 0x83,
    0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
    0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b,
    0x9c, 0x9d, 0x9e, 0x9f, 0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
    0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf, 0xb0, 0xb1, 0xb2, 0xb3,
    0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
    0xc0, 0xe1, 0xe2, 0xe3, 0xe4, 0xc5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb,
    0xec, 0xed, 0xee, 0xef, 0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7,
    0xf8, 0xf9, 0xfa, 0xdb, 0xdc, 0xdd, 0xde, 0xdf, 0xe0, 0xe1, 0xe2, 0xe3,
    0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
    0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb,
    0xfc, 0xfd, 0xfe, 0xff,
  };

/*
  Forward declaration.
*/
static int
  IsDirectory(const char *);

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   A c q u i r e S t r i n g                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AcquireString() allocates memory for a string and copies the source string
%  to that memory location (and returns it).
%
%  The format of the AcquireString method is:
%
%      char *AcquireString(const char *source)
%
%  A description of each parameter follows:
%
%    o allocated_string:  Method AcquireString returns a copy of the source
%      string.
%
%    o source: A character string.
%
%
*/
MagickExport const char *AcquireString(const char *source)
{
  char
    *destination;

  assert(source != (const char *) NULL);
  destination=(char *) AcquireMemory(strlen(source)+1);
  if (destination == (char *) NULL)
    MagickFatalError(ResourceLimitFatalError,"MemoryAllocationFailed",
      "UnableToAcquireString");
  *destination='\0';
  if (source != (char *) NULL)
    (void) strcpy(destination,source);
  return((const char *) destination);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   A l l o c a t e S t r i n g                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AllocateString() allocates memory for a string and copies the source string
%  to that memory location (and returns it).
%
%  The format of the AllocateString method is:
%
%      char *AllocateString(const char *source)
%
%  A description of each parameter follows:
%
%    o allocated_string:  Method AllocateString returns a copy of the source
%      string.
%
%    o source: A character string.
%
%
*/
MagickExport char *AllocateString(const char *source)
{
  char
    *destination;

  size_t
    length;

  length=MaxTextExtent;
  if (source != (char *) NULL)
    length+=strlen(source);
  destination=(char *) AcquireMemory(length+MaxTextExtent);
  if (destination == (char *) NULL)
    MagickFatalError(ResourceLimitFatalError,"MemoryAllocationFailed",
      "UnableToAllocateString");
  *destination='\0';
  if (source != (char *) NULL)
    (void) strcpy(destination,source);
  return(destination);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  A p p e n d I m a g e F o r m a t                                          %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AppendImageFormat() appends the image format type to the filename.  If an
%  extension to the file already exists, it is first removed.
%
%  The format of the AppendImageFormat method is:
%
%      void AppendImageFormat(const char *format,char *filename)
%
%  A description of each parameter follows.
%
%   o  format:  Specifies a pointer to an array of characters.  This is the
%      format of the image.
%
%   o  filename:  Specifies a pointer to an array of characters.  The unique
%      file name is returned in this array.
%
%
*/
MagickExport void AppendImageFormat(const char *format,char *filename)
{
  char
    root[MaxTextExtent];

  assert(format != (char *) NULL);
  assert(filename != (char *) NULL);
  if ((*format == '\0') || (*filename == '\0'))
    return;
  if (LocaleCompare(filename,"-") == 0)
    {
      char
        message[MaxTextExtent];

      FormatString(message,"%.1024s:%.1024s",format,filename);
      (void) strncpy(filename,message,MaxTextExtent-1);
      return;
    }
  GetPathComponent(filename,RootPath,root);
  FormatString(filename,"%.1024s.%.1024s",root,format);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   B a s e 6 4 D e c o d e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Base64Decode() decodes Base64-encoded text and returns its binary
%  equivalent.  NULL is returned if the text is not valid base64 data, or a
%  memory allocation failure occurs.
%
%  Contributed by Bob Friesenhahn.
%
%  The format of the Base64Decode method is:
%
%      unsigned char *Base64Decode(const char *source,length_t *length)
%
%  A description of each parameter follows:
%
%    o source:  A pointer to a Base64-encoded string.
%
%    o length: The number of bytes decoded.
%
*/
MagickExport unsigned char *Base64Decode(const char *source,size_t *length)
{
  int
    state;

  register const char
    *p,
    *q;

  register size_t
    i;

  size_t
    max_length;

  unsigned char
    *decode;

  assert(source != (char *) NULL);
  assert(length != (size_t *) NULL);
  *length=0;
  max_length=3*strlen(source)/4+1;
  decode=(unsigned char *) AcquireMemory(max_length);
  if (decode == (unsigned char *) NULL)
    return((unsigned char *) NULL);
  i=0;
  state=0;
  for (p=source; *p != '\0'; p++)
  {
    if (isspace((int) *p))
      continue;
    if (*p == '=')
      break;
    q=strchr(Base64,*p);
    if (q == (char *) NULL)
      {
        LiberateMemory((void **) &decode);
        return((unsigned char *) NULL);  /* non-base64 character */
      }
    switch (state)
    {
      case 0:
      {
        decode[i]=(q-Base64) << 2;
        state++;
        break;
      }
      case 1:
      {
        decode[i++]|=(q-Base64) >> 4;
        decode[i]=((q-Base64) & 0x0f) << 4;
        state++;
        break;
      }
      case 2:
      {
        decode[i++]|=(q-Base64) >> 2;
        decode[i]=((q-Base64) & 0x03) << 6;
        state++;
        break;
      }
      case 3:
      {
        decode[i++]|=(q-Base64);
        state=0;
        break;
      }
    }
  }
  /*
    Verify Base-64 string has proper terminal characters.
  */
  if (*p != '=')
    {
      if (state != 0)
        {
          LiberateMemory((void **) &decode);
          return((unsigned char *) NULL);
        }
    }
  else
    {
      p++;
      switch (state)
      {
        case 0:
        case 1:
        {
          /*
            Unrecognized '=' character.
          */
          LiberateMemory((void **) &decode);
          return((unsigned char *) NULL);
        }
        case 2:
        {
          for ( ; *p != '\0'; p++)
            if (!isspace((int) *p))
              break;
          if (*p != '=')
            {
              LiberateMemory((void **) &decode);
              return((unsigned char *) NULL);
            }
          p++;
        }
        case 3:
        {
          for ( ; *p != '\0'; p++)
            if (!isspace((int) *p))
              {
                LiberateMemory((void **) &decode);
                return((unsigned char *) NULL);
              }
          if (decode[i] != 0)
            {
              LiberateMemory((void **) &decode);
              return((unsigned char *) NULL);
            }
        }
      }
    }
  *length=i;
  assert(*length < max_length);
  return(decode);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   B a s e 6 4 E n c o d e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Base64Encode() encodes arbitrary binary data to base64 encoded format as
%  described by the "Base64 Content-Transfer-Encoding" section of RFC 2045 and
%  returns the result as a null-terminated ASCII string.  NULL is returned if
%  a memory allocation failure occurs.
%
%  Contributed by Bob Friesenhahn.
%
%  The format of the Base64Encode method is:
%
%      char *Base64Encode(const unsigned char *blob,const size_t blob_length,
%        size_t *encode_length)
%
%  A description of each parameter follows:
%
%    o blob:           A pointer to binary data to encode.
%
%    o blob_length:    The number of bytes to encode.
%
%    o encode_length:  The number of bytes encoded.
%
*/
MagickExport char *Base64Encode(const unsigned char *blob,
  const size_t blob_length,size_t *encode_length)
{
  char
    *encode;

  register const unsigned char
    *p;

  register size_t
    i;

  size_t
    max_length,
    remainder;

  assert(blob != (const unsigned char *) NULL);
  assert(blob_length != 0);
  assert(encode_length != (size_t *) NULL);
  *encode_length=0;
  max_length=4*blob_length/3+4;
  encode=(char *) AcquireMemory(max_length);
  if (encode == (char *) NULL)
    return((char *) NULL);
  i=0;
  for (p=blob; p < (blob+blob_length-2); p+=3)
  {
    encode[i++]=Base64[*p >> 2];
    encode[i++]=Base64[((*p & 0x03) << 4)+(*(p+1) >> 4)];
    encode[i++]=Base64[((*(p+1) & 0x0f) << 2)+(*(p+2) >> 6)];
    encode[i++]=Base64[*(p+2) & 0x3f];
  }
  remainder=blob_length % 3;
  if (remainder != 0)
    {
      long
        j;

      unsigned char
        code[3];

      code[0]='\0';
      code[1]='\0';
      code[2]='\0';
      for (j=0; j < (long) remainder; j++)
        code[j]=(*p++);
      encode[i++]=Base64[code[0] >> 2];
      encode[i++]=Base64[((code[0] & 0x03) << 4)+(code[1] >> 4)];
      if (remainder == 1)
        encode[i++]='=';
      else
        encode[i++]=Base64[((code[1] & 0x0f) << 2)+(code[2] >> 6)];
      encode[i++]='=';
    }
  *encode_length=i;
  encode[i++]='\0';
  assert(i <= max_length);
  return(encode);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   C l o n e S t r i n g                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method CloneString allocates memory for the destination string and copies
%  the source string to that memory location.
%
%  The format of the CloneString method is:
%
%      unsigned int CloneString(char **destination,const char *source)
%
%  A description of each parameter follows:
%
%    o status:  Method CloneString returns True is the string is cloned,
%      otherwise False is returned.
%
%    o destination:  A pointer to a character string.
%
%    o source: A character string.
%
%
*/
MagickExport unsigned int CloneString(char **destination,const char *source)
{
  assert(destination != (char **) NULL);
  if (source == (const char *) NULL)
    {
      if (*destination != (char *) NULL)
        LiberateMemory((void **) &*destination);
      *destination=(char *) NULL;
      return(True);
    }
  if (*destination == (char *) NULL)
    {
      *destination=AllocateString(source);
      return(True);
    }
  ReacquireMemory((void **) &(*destination),strlen(source)+MaxTextExtent);
  if (*destination == (char *) NULL)
    MagickFatalError(ResourceLimitFatalError,"MemoryAllocationFailed",
      "UnableToAllocateString");
  (void) strcpy(*destination,source);
  return(True);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   C o n c a t e n a t e S t r i n g                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method ConcatenateString appends a copy of string source, including
%  the terminating null character, to the end of string destination.
%
%  The format of the ConcatenateString method is:
%
%      unsigned int ConcatenateString(char **destination,const char *source)
%
%  A description of each parameter follows:
%
%    o status:  Method ConcatenateString returns True is the string is cloned,
%      otherwise False is returned.
%
%    o destination:  A pointer to a character string.
%
%    o source: A character string.
%
%
*/
MagickExport unsigned int ConcatenateString(char **destination,
  const char *source)
{
  assert(destination != (char **) NULL);
  if (source == (const char *) NULL)
    return(True);
  if (*destination == (char *) NULL)
    {
      *destination=AllocateString(source);
      return(True);
    }
  ReacquireMemory((void **) &(*destination),
    strlen(*destination)+strlen(source)+MaxTextExtent);
  if (*destination == (char *) NULL)
    MagickFatalError(ResourceLimitFatalError,"MemoryAllocationFailed",
      "UnableToConcatenateString");
  (void) strcat(*destination,source);
  return(True);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   E s c a p e S t r i n g                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  EscapeString() allocates memory for a backslash-escaped version of a
%  source text string, copies the escaped version of the text to that
%  memory location while adding backslash characters, and returns the
%  escaped string.
%
%  The format of the EscapeString method is:
%
%      char *EscapeString(const char *source,const char escape)
%
%  A description of each parameter follows:
%
%    o allocated_string:  Method EscapeString returns the escaped string.
%
%    o source: A character string.
%
%    o escape: The quoted string termination character to escape (e.g. '"').
%
*/
MagickExport char *EscapeString(const char *source,const char escape)
{
  char
    *destination;

  register char
    *q;

  register const char
    *p;

  unsigned int
    length;

  assert(source != (const char *) NULL);
  length=strlen(source)+1;
  for (p=source; *p; p++)
    if ((*p == '\\') || (*p == escape))
      length++;
  destination=(char *) AcquireMemory(length);
  if (destination == (char *) NULL)
    MagickFatalError(ResourceLimitFatalError,"MemoryAllocationFailed",
      "UnableToEscapeString");
  *destination='\0';
  if (source != (char *) NULL)
    {
      q=destination;
      for (p=source; *p; p++)
      {
        if ((*p == '\\') || (*p == escape))
          *q++= '\\';
        *q++=(*p);
      }
      *q=0;
    }
  return(destination);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   E x p a n d F i l e n a m e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method ExpandFilename expands '~' in a filename.
%
%  The format of the ExpandFilename function is:
%
%      ExpandFilename(char *filename)
%
%  A description of each parameter follows:
%
%    o filename: Specifies a pointer to a character array that contains the
%      filename.
%
%
*/
MagickExport void ExpandFilename(char *filename)
{
  char
    expanded_filename[MaxTextExtent];

  register char
    *p;

  if (filename == (char *) NULL)
    return;
  if (*filename != '~')
    return;
  (void) strncpy(expanded_filename,filename,MaxTextExtent-1);
  if (*(filename+1) == '/')
    {
      /*
        Substitute ~ with $HOME.
      */
      p=(char *) getenv("HOME");
      if (p == (char *) NULL)
        p=(char *) ".";
      (void) strncpy(expanded_filename,p,MaxTextExtent-1);
      (void) strncat(expanded_filename,filename+1,MaxTextExtent-
        strlen(expanded_filename)-1);
    }
  else
    {
#if defined(POSIX)
      char
        username[MaxTextExtent];

      struct passwd
        *entry;

      /*
        Substitute ~ with home directory from password file.
      */
      (void) strncpy(username,filename+1,MaxTextExtent-2);
      p=strchr(username,'/');
      if (p != (char *) NULL)
        *p='\0';
      entry=getpwnam(username);
      if (entry == (struct passwd *) NULL)
        return;
      (void) strncpy(expanded_filename,entry->pw_dir,MaxTextExtent-1);
      if (p != (char *) NULL)
        {
          (void) strcat(expanded_filename,"/");
          (void) strncat(expanded_filename,p+1,MaxTextExtent-
            strlen(expanded_filename)-1);
        }
#endif
    }
  (void) strncpy(filename,expanded_filename,MaxTextExtent-1);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   E x p a n d F i l e n a m e s                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method ExpandFilenames checks each argument of the command line vector and
%  expands it if they have a wildcard character.  For example, *.jpg might
%  expand to:  bird.jpg rose.jpg tiki.jpg.
%
%  The format of the ExpandFilenames function is:
%
%      status=ExpandFilenames(int *argc,char ***argv)
%
%  A description of each parameter follows:
%
%    o argc: Specifies a pointer to an integer describing the number of
%      elements in the argument vector.
%
%    o argv: Specifies a pointer to a text array containing the command line
%      arguments.
%
%
*/
MagickExport unsigned int ExpandFilenames(int *argc,char ***argv)
{
  char
    starting_directory[MaxTextExtent],
    *option,
    **vector;

  long
    count,
    number_files;

  register long
    i,
    j;

  /*
    Allocate argument vector.
  */
  assert(argc != (int *) NULL);
  assert(argv != (char ***) NULL);
  for (i=1; i < *argc; i++)
    if (strlen((*argv)[i]) > (MaxTextExtent/2-1))
      MagickFatalError(ResourceLimitFatalError,"Token length exceeds limit",
        (*argv)[i]);
  vector=(char **) AcquireMemory((*argc+MaxTextExtent)*sizeof(char *));
  if (vector == (char **) NULL)
    return(False);
  /*
    Expand any wildcard filenames.
  */
  (void) getcwd(starting_directory,MaxTextExtent-1);
  count=0;
  for (i=0; i < *argc; i++)
  {
    option=(*argv)[i];
    vector[count++]=AllocateString(option);

    /*
      Don't expand or process any VID: argument since the VID coder
      does its own expansion
    */
    if (LocaleNCompare("VID:",option,4) == 0)
      continue;

    /*
      Skip the argument to +profile since it can be a glob
      specification, and we don't want it interpreted as a file.
    */
    if (LocaleNCompare("+profile",option,8) == 0)
      {
        i++;
        if (i == *argc)
            continue;
        option=(*argv)[i];
        vector[count++]=AllocateString(option);
        continue;
      }

    /*
      Pass quotes through to the command-line parser
    */
    if ((*option == '"') || (*option == '\''))
      continue;

    /*
      Check for, and handle subimage specifications, i.e. img0001.pcd[2].
      FIXME: Should the subimage spec be applied to each filename
      expanded by a glob specification as it now is for the format?
      The apparent subimage specification could be a glob specification,
      so it could be tried as a glob first, and if there is no match, could
      be appended as a subimage specification.
    */
    if (strchr(option,'['))
      {
        ExceptionInfo
          exception;

        ImageInfo
          *image_info;

        unsigned int
          exempt;

        image_info=CloneImageInfo((ImageInfo *) NULL);
        (void) strncpy(image_info->filename,option,MaxTextExtent-1);
        GetExceptionInfo(&exception);
        (void) SetImageInfo(image_info,True,&exception);
        DestroyExceptionInfo(&exception);
        exempt=image_info->subimage;
        DestroyImageInfo(image_info);
        if (exempt)
          {
            continue;
          }
       }
    {
      char
        format[MaxTextExtent],
        path[MaxTextExtent];

      /*
        Extract format specification (if any) from filename
        specification. Set path to remaining string.
      */
      format[0]='\0';
      if (strchr(option,':'))
        {
          for(j=0; ((j< MaxTextExtent-2) && (option[j] != ':') &&
                    (isalnum((int) option[j]))); j++)
            format[j]=option[j];
          if (option[j] == ':')
            {
              format[j]=option[j];
              j++;
              format[j]='\0';
            }
          else
            format[0]='\0';
        }
      if (IsMagickConflict(format))
        format[0]='\0';

      (void) strncpy(path,option+strlen(format),MaxTextExtent-strlen(format)-1);

      /*
        Expand arguments of form ~path or format:~path such that
        tilde ('~') expands to the user's home directory (or $HOME).
      */
      if (path[0] == '~')
        {
          ExpandFilename(path);
          if (path[0] != '~')
            {
              char
                buffer[MaxTextExtent];

              strcpy(buffer,format);
              strcat(buffer,path);
              CloneString(&vector[count-1],buffer);
            }
        }
      
      if (!IsGlob(path))
        {
          continue;
        }

      {
        char
          **filelist,
          filename[MaxTextExtent],
          working_directory[MaxTextExtent];

        /*
          Fully qualify the base path for the file specification.
        */
        {
          char
            specified_directory[MaxTextExtent];
          
          GetPathComponent(path,HeadPath,specified_directory);
          if (specified_directory[0] == '\0')
            (void) getcwd(working_directory,MaxTextExtent-1);
          else if ((specified_directory[0] != DirectorySeparator[0])
#if defined(WIN32)
                   /*
                     Windows does allow relative inferior qualified
                     specifications like "c:image.gif" to access
                     image.gif in the current directory but we will
                     require a fully qualified specification like
                     "c:\path\image.gif" or an unqualified
                     specification like "image.gif". Testing shows
                     that Windows does not allow creating directories
                     named like drive letters so we can ignore that
                     possibility. For example, Windows won't allow
                     creating the directory "c:".
                   */
                   &&
                   !((strlen(specified_directory) >= 3) &&
                     (isalpha((int)specified_directory[0]) &&
                      (specified_directory[1] == ':') &&
                      (specified_directory[2] == DirectorySeparator[0])))
#endif
                     )
            {
              char
                current_directory[MaxTextExtent];
              
              (void) getcwd(current_directory,MaxTextExtent-1);
              if (current_directory[strlen(current_directory)-1] != DirectorySeparator[0])
                strcat(current_directory,DirectorySeparator);
              strcpy(working_directory,current_directory);
              strcat(working_directory,specified_directory);
            }
          else
            strcpy(working_directory,specified_directory);
        }

        GetPathComponent(path,TailPath,filename);

        /*
          Get the list of matching file names.
        */
        filelist=ListFiles(working_directory,filename,&number_files);
        if (filelist == (char **) NULL)
          continue;
        for (j=0; j < number_files; j++)
          if (IsDirectory(filelist[j]) <= 0)
            break;
        if (j == number_files)
          {
            for (j=0; j < number_files; j++)
              LiberateMemory((void **) &filelist[j]);
            LiberateMemory((void **) &filelist);
            continue;
          }
        /*
          Transfer file list to argument vector.
        */
        ReacquireMemory((void **) &vector,
                        (*argc+count+number_files+MaxTextExtent)*sizeof(char *));
        if (vector == (char **) NULL)
          return(False);
        count--;
        for (j=0; j < number_files; j++)
          {
            FormatString(filename,"%.1024s%s%.1024s",working_directory,
                         DirectorySeparator,filelist[j]);
            if (IsDirectory(filename) != 0)
              {
                LiberateMemory((void **) &filelist[j]);
                continue;
              }
            {
              char
                file_spec[MaxTextExtent];

              sprintf(file_spec,"%s%s",format,filename);
              vector[count]=AllocateString(file_spec);
            }
            LiberateMemory((void **) &filelist[j]);
            count++;
          }
        LiberateMemory((void **) &filelist);
      }
    }
  }
  (void) chdir(starting_directory);
  *argc=count;
  *argv=vector;
  return(True);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  F o r m a t S i z e                                                        %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  FormatSize() converts a size to a human readable format, for example,
%  14kb, 234mb, 2.7gb, or 3.0tb.  Scaling is done by repetitively dividing by
%  1024.
%
%  The format of the FormatSize method is:
%
%      char *FormatSize(const ExtendedSignedIntegralType size,char *format)
%
%  A description of each parameter follows:
%
%    o size:  convert this size to a human readable format.
%
%    o format:  human readable format.
%
%
*/
MagickExport void FormatSize(const ExtendedSignedIntegralType size,char *format)
{
  double
    length;

  register long
    i;

  length=size;
  for (i=0; length > 1024; i++)
    length/=1024.0;
  FormatString(format,"%.1f",length);
  switch (i)
  {
    default: break;
    case 0: break;
    case 1: (void) strcat(format,"k"); break;
    case 2: (void) strcat(format,"m"); break;
    case 3: (void) strcat(format,"g"); break;
    case 4: (void) strcat(format,"t"); break;
  }
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  F o r m a t S t r i n g                                                    %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method FormatString prints formatted output of a variable argument list.
%
%  The format of the FormatString method is:
%
%      void FormatString(char *string,const char *format,...)
%
%  A description of each parameter follows.
%
%   o  string:  Method FormatString returns the formatted string in this
%      character buffer.
%
%   o  format:  A string describing the format to use to write the remaining
%      arguments.
%
%
*/
MagickExport void FormatString(char *string,const char *format,...)
{
  va_list
    operands;

  va_start(operands,format);
#if defined(HAVE_VSNPRINTF)
  (void) vsnprintf(string,MaxTextExtent,format,operands);
#else
#  if defined(HAVE_VSPRINTF)
  (void) vsprintf(string,format,operands);
#  else
#    error Neither vsnprintf or vsprintf is available.
#  endif
#endif
  va_end(operands);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t E x e c u t i o n P a t h                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetExecutionPath() returns the pathname of the executable that started
%  the process.  On success True is returned, otherwise False.
%
%  The format of the GetExecutionPath method is:
%
%      unsigned int GetExecutionPath(char *path)
%
%  A description of each parameter follows:
%
%    o path: The pathname of the executable that started the process.
%
*/
MagickExport unsigned int GetExecutionPath(char *path)
{
  *path='\0';
#if defined(WIN32)
  return(NTGetExecutionPath(path));
#endif
#if defined(HAVE_GETEXECNAME)
  {
    const char
      *execution_path;

    execution_path=(const char *) getexecname();
    if (execution_path != (const char *) NULL)
      {
        if (*execution_path != *DirectorySeparator)
          {
            (void) getcwd(path,MaxTextExtent-1);
            (void) strcat(path,"/");
          }
        (void) strncat(path,execution_path,MaxTextExtent-strlen(path)-1);
        if (IsAccessible(path))
          return(True);
      }
  }
#endif
  return(False);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t G e o m e t r y                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetGeometry() parses a geometry specification and returns the width
%  height, x, and y values.  It also returns flags that indicates which
%  of the four values (width, height, x, y) were located in the string, and
%  whether the x and y values are negative.  In addition, there are flags to
%  report any meta characters (%, !, <, and >).
%
%  The format of the GetGeometry method is:
%
%      int GetGeometry(const char *image_geometry,long *x,long *y,
%        unsigned long *width,unsigned long *height)
%
%  A description of each parameter follows:
%
%    o flags:  Method GetGeometry returns a bitmask that indicates
%      which of the four values were located in the geometry string.
%
%    o image_geometry:  Specifies a character string representing the geometry
%      specification.
%
%    o x,y:  The x and y offset as determined by the geometry specification is
%      returned here.
%
%    o width,height:  The width and height as determined by the geometry
%      specification is returned here.
%
%
*/
MagickExport int GetGeometry(const char *image_geometry,long *x,long *y,
  unsigned long *width,unsigned long *height)
{
  const char
    *c;

  char
    geometry[MaxTextExtent],
    *p,
    *q;

  int
    flags;

  RectangleInfo
    bounds;

  /*
    Ensure the image geometry is valid.
  */
  assert(x != (long *) NULL);
  assert(y != (long *) NULL);
  assert(width != (unsigned long *) NULL);
  assert(height != (unsigned long *) NULL);
  if ((image_geometry == (char *) NULL) || (*image_geometry == '\0') ||
      (strlen(image_geometry) > MaxTextExtent-1))
    return(NoValue);

  /*
    Transfer base geometry while recording and stripping flags
  */
  q=geometry;
  flags=NoValue;

  for (c=image_geometry; *c != 0 ; c++)
    {
      if (isspace((int) (*c)))
        {
          continue;
        }
      else
        switch (*c)
          {
          case '%':
            {
              flags|=PercentValue;
              break;
            }
          case '!':
            {
              flags|=AspectValue;
              break;
            }
          case '<':
            {
              flags|=LessValue;
              break;
            }
          case '>':
            {
              flags|=GreaterValue;
              break;
            }
          case '@':
            {
              flags|=AreaValue;
              break;
            }
          case '+':
          case '-':
          case '.':
          case '0':
          case '1':
          case '2':
          case '3':
          case '4':
          case '5':
          case '6':
          case '7':
          case '8':
          case '9':
          case 'X':
          case 'x':
            {
              *q=*c;
              q++;
              break;
            }
          default:
            {
              /* Illegal character fails entire geometry translation */
              return NoValue;
            }
          }
    }
  *q='\0';

  /*
    Parse width/height/x/y.
  */
  p=geometry;
  while (isspace((int) *p))
    p++;
  if (*p == '\0')
    return(flags);
  if (*p == '=')
    p++;
  if ((*p != '+') && (*p != '-') && (*p != 'x') && (*p != 'X'))
    {
      /*
        Parse width.
      */
      q=p;
      bounds.width=(unsigned long) floor(strtol(p,&q,10)+0.5);
      if (p != q)
        flags|=WidthValue;
      if ((*q == 'x') || (*q == 'X'))
        p=q;
      else
        {
          bounds.width=(unsigned long) floor(strtod(p,&p)+0.5);
          bounds.height=bounds.width;
          flags|=HeightValue;
        }
    }
  if ((*p == 'x') || (*p == 'X'))
    {
      /*
        Parse height.
      */
      p++;
      q=p;
      bounds.height=(unsigned long) floor(strtod(p,&p)+0.5);
      if (p != q)
        flags|=HeightValue;
    }
  if ((*p == '+') || (*p == '-'))
    {
      /*
        Parse x value.
      */
      if (*p == '+')
        {
          p++;
          q=p;
          bounds.x=(long) ceil(strtod(p,&p)-0.5);
        }
      else
        {
          p++;
          q=p;
          bounds.x=(long) ceil(-strtod(p,&p)-0.5);
          if (p != q)
            flags|=XNegative;
        }
      if (p != q)
        flags|=XValue;
      if ((*p == '+') || (*p == '-'))
        {
          /*
            Parse y value.
          */
          if (*p == '+')
            {
              p++;
              q=p;
              bounds.y=(long) ceil(strtod(p,&p)-0.5);
            }
          else
            {
              p++;
              q=p;
              bounds.y=(long) ceil(-strtod(p,&p)-0.5);
              if (p != q)
                flags|=YNegative;
            }
          if (p != q)
            flags|=YValue;
        }
    }
  if (*p != '\0')
    return(flags);
  if (flags & XValue)
    *x=bounds.x;
  if (flags & YValue)
    *y=bounds.y;
  if (flags & WidthValue)
    *width=bounds.width;
  if (flags & HeightValue)
    *height=bounds.height;
  return(flags);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   G e t M a g i c k G e o m e t r y                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetMagickGeometry() is similar to GetGeometry() except the returned
%  geometry is modified as determined by the meta characters:  %, @, !,
%  <, and >, as well as +x, and +y offsets.  The geometry string has the
%  form:
%
%      <width>x<height>{+-}<x>{+-}<y>{%}{@} {!}{<}{>}
%
%  For example, the string "640x480>" is a valid geometry string.
%
%  The interpretation of the geometry string parameters are as follows:
%    %: The geometry width and height parameters are interpreted as a
%       percentage of the supplied width and height parameters.
%    @: The geometry parameter represents the desired total area (e.g.
%       "307520@") or width x height (e.g. "640*480@") of the final image.
%    !: Force the width and height values to be absolute values.  The
%       original image aspect ratio is not maintained.
%    <: Update the provided width and height parameters if its dimensions
%       are less than the geometry specification.
%    >: Update the provided width and height parameters if its dimensions
%       are greater than the geometry specification.
%
%  Any supplied offset parameters are used to adjust the image width,
%  height, and x/y offset values as required to center the scaled image
%  into the region specified by the supplied width and height.
%
%  The format of the GetMagickGeometry method is:
%
%      int GetMagickGeometry(const char *geometry,long *x,long *y,
%        unsigned long *width,unsigned long *height)
%
%  A description of each parameter follows:
%
%    o flags:  Method GetMagickGeometry returns a bitmask that indicates
%      which of the five values (PercentValue, AspectValue, LessValue,
%      GreaterValue, AreaValue) were located in the geometry string.
%
%    o image_geometry:  Specifies a character string representing the geometry
%      specification.
%
%    o x,y:  A pointer to an integer.  The x and y offset as determined by
%      the geometry specification is returned here.
%
%    o width,height:  A pointer to an unsigned integer.  The width and height
%      as determined by the geometry specification is returned here.
%
%
*/
MagickExport int GetMagickGeometry(const char *geometry,long *x,long *y,
  unsigned long *width,unsigned long *height)
{
  int
    flags;

  long
    count;

  unsigned long
    former_height,
    former_width;

  /*
    Ensure the image geometry is valid.
  */
  assert(x != (long *) NULL);
  assert(y != (long *) NULL);
  assert(width != (unsigned long *) NULL);
  assert(height != (unsigned long *) NULL);
  if ((geometry == (char *) NULL) || (*geometry == '\0'))
    return(NoValue);
  /*
    Parse geometry using GetGeometry.
  */
  former_width=(*width);
  former_height=(*height);
  flags=GetGeometry(geometry,x,y,width,height);
  if (flags & PercentValue)
    {
      double
        x_scale,
        y_scale;

      /*
        Geometry is a percentage of the image size.
      */
      x_scale=(*width);
      y_scale=(*height);
      count=sscanf(geometry,"%lf%%x%lf",&x_scale,&y_scale);
      if (count != 2)
        count=sscanf(geometry,"%lfx%lf",&x_scale,&y_scale);
      if (count == 1)
        y_scale=x_scale;
      *width=(unsigned long) floor((x_scale*former_width/100.0)+0.5);
      *height=(unsigned long) floor((y_scale*former_height/100.0)+0.5);
      former_width=(*width);
      former_height=(*height);
    }
  if (flags & AreaValue)
    {
      double
        scale_factor,
        original_area,
        target_area,
        target_width,
        target_height;

      /*
        Geometry is a maximum area in pixels.
      */
      target_width=(*width);
      target_height=(*height);
      target_area=target_width*target_height;
      count=sscanf(geometry,"%lf%%x%lf",&target_width,&target_height);
      if (count != 2)
        count=sscanf(geometry,"%lfx%lf",&target_width,&target_height);
      if (count == 2)
        target_area=target_width*target_height;
      if (count == 1)
        target_area=target_width;
      original_area=(double)former_width*former_height;
      scale_factor=1/sqrt(original_area/target_area);
      *width=(unsigned long) floor(former_width*scale_factor+0.25);
      *height=(unsigned long) floor(former_height*scale_factor+0.25);
      former_width=(*width);
      former_height=(*height);
    }
  if (!(flags & AspectValue) &&
      ((*width != former_width) || (*height != former_height)))
    {
      double
        scale_factor;

      /*
        Respect aspect ratio of the image.
      */
      if ((former_width == 0) || (former_height == 0))
        scale_factor=1.0;
      else
        if (((flags & WidthValue) != 0) && (flags & HeightValue) != 0)
          {
            scale_factor=(double) *width/former_width;
            if (scale_factor > ((double) *height/former_height))
              scale_factor=(double) *height/former_height;
          }
        else
          if ((flags & WidthValue) != 0)
            scale_factor=(double) *width/former_width;
          else
            scale_factor=(double) *height/former_height;
    *width=(unsigned long) floor(scale_factor*former_width+0.5);
    *height=(unsigned long) floor(scale_factor*former_height+0.5);
  }
  if (flags & GreaterValue)
    {
      if (former_width < *width)
        *width=former_width;
      if (former_height < *height)
        *height=former_height;
    }
  if (flags & LessValue)
    {
      if (former_width > *width)
        *width=former_width;
      if (former_height > *height)
        *height=former_height;
    }

#if 0  
    {
      RectangleInfo
        media_info;

      long
        delta;

      media_info.width=(*width);
      media_info.height=(*height);
      media_info.x=(*x);
      media_info.y=(*y);
      (void) GetGeometry(geometry,&media_info.x,&media_info.y,&media_info.width,
                         &media_info.height);
      if ((flags & XValue) == 0)
        {
          /*
            Center image in the X direction.
          */
          delta=(long) (media_info.width-(*width));
          if (delta >= 0)
            *x=delta >> 1;
        }
      else
        if ((flags & XNegative) != 0)
          *x+=media_info.width-(*width);
      if ((flags & YValue) == 0)
        {
          /*
            Center image in the Y direction.
          */
          delta=(long) (media_info.height-(*height));
          if (delta >= 0)
            *y=delta >> 1;
        }
      else
        if ((flags & YNegative) != 0)
          *y+=media_info.height-(*height);
      if (flags & GreaterValue)
        {
          if ((*width+((*x) << 1)) > media_info.width)
            {
              if ((long) *width > ((*x) << 1))
                *width-=(*x) << 1;
              if ((long) *height > ((*y) << 1))
                *height-=(*y) << 1;
            }
          if ((*height+((*y) << 1)) > media_info.height)
            {
              if ((long) *width > ((*x) << 1))
                *width-=(*x) << 1;
              if ((long) *height > ((*y) << 1))
                *height-=(*y) << 1;
            }
        }
    }
#endif
  return(flags);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  G e t P a g e G e o m e t r y                                              %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetPageGeometry() returns a new geometry string, with any embedded page
%  mneumonic in the original geometry string replaced with the equivalent
%  size in picas. For example, the geometry string "A4+36+36" is translated
%  to "595x842+36+36" in the returned value.
%
%  The format of the GetPageGeometry method is:
%
%      char *GetPageGeometry(const char *page_geometry)
%
%  A description of each parameter follows.
%
%   o  page_geometry:  The geometry string to translate.
%
%
*/
MagickExport char *GetPageGeometry(const char *page_geometry)
{
  static const char
    *PageSizes[][2]=
    {
      { "4x6",  "288x432" },
      { "5x7",  "360x504" },
      { "7x9",  "504x648" },
      { "8x10", "576x720" },
      { "9x11",  "648x792" },
      { "9x12",  "648x864" },
      { "10x13",  "720x936" },
      { "10x14",  "720x1008" },
      { "11x17",  "792x1224" },
      { "A0",  "2384x3370" },
      { "A1",  "1684x2384" },
      { "A10", "73x105" },
      { "A2",  "1191x1684" },
      { "A3",  "842x1191" },
      { "A4",  "595x842" },
      { "A4SMALL", "595x842" },
      { "A5",  "420x595" },
      { "A6",  "297x420" },
      { "A7",  "210x297" },
      { "A8",  "148x210" },
      { "A9",  "105x148" },
      { "ARCHA", "648x864" },
      { "ARCHB", "864x1296" },
      { "ARCHC", "1296x1728" },
      { "ARCHD", "1728x2592" },
      { "ARCHE", "2592x3456" },
      { "B0",  "2920x4127" },
      { "B1",  "2064x2920" },
      { "B10", "91x127" },
      { "B2",  "1460x2064" },
      { "B3",  "1032x1460" },
      { "B4",  "729x1032" },
      { "B5",  "516x729" },
      { "B6",  "363x516" },
      { "B7",  "258x363" },
      { "B8",  "181x258" },
      { "B9",  "127x181" },
      { "C0",  "2599x3676" },
      { "C1",  "1837x2599" },
      { "C2",  "1298x1837" },
      { "C3",  "918x1296" },
      { "C4",  "649x918" },
      { "C5",  "459x649" },
      { "C6",  "323x459" },
      { "C7",  "230x323" },
      { "EXECUTIVE", "540x720" },
      { "FLSA", "612x936" },
      { "FLSE", "612x936" },
      { "FOLIO",  "612x936" },
      { "HALFLETTER", "396x612" },
      { "ISOB0", "2835x4008" },
      { "ISOB1", "2004x2835" },
      { "ISOB10", "88x125" },
      { "ISOB2", "1417x2004" },
      { "ISOB3", "1001x1417" },
      { "ISOB4", "709x1001" },
      { "ISOB5", "499x709" },
      { "ISOB6", "354x499" },
      { "ISOB7", "249x354" },
      { "ISOB8", "176x249" },
      { "ISOB9", "125x176" },
      { "LEDGER",  "1224x792" },
      { "LEGAL",  "612x1008" },
      { "LETTER", "612x792" },
      { "LETTERSMALL",  "612x792" },
      { "QUARTO",  "610x780" },
      { "STATEMENT",  "396x612" },
      { "TABLOID",  "792x1224" },
      { (char *) NULL, (char *) NULL }
    };

  char
    *page;

  register long
    i;

  assert(page_geometry != (char *) NULL);
  page=AllocateString(page_geometry);
  for (i=0; *PageSizes[i] != (char *) NULL; i++)
    if (LocaleNCompare(PageSizes[i][0],page,strlen(PageSizes[i][0])) == 0)
      {
        int
          flags;

        RectangleInfo
          geometry;

        /*
          Replace mneumonic with the equivalent size in dots-per-inch.
        */
        (void) strncpy(page,PageSizes[i][1],MaxTextExtent-1);
        (void) strncat(page,page_geometry+strlen(PageSizes[i][0]),
          MaxTextExtent-strlen(page)-2);
        flags=GetGeometry(page,&geometry.x,&geometry.y,&geometry.width,
          &geometry.height);
        if (!(flags & GreaterValue))
          (void) strcat(page,">");
        break;
      }
  return(page);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t P a t h C o m p o n e n t                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method GetPathComponent returns the parent directory name, filename,
%  basename, or extension of a file path.
%
%  The format of the GetPathComponent function is:
%
%      GetPathComponent(const char *path,PathType type,char *component)
%
%  A description of each parameter follows:
%
%    o path: Specifies a pointer to a character array that contains the
%      file path.
%
%    o type: Specififies which file path component to return.
%
%    o component: The selected file path component is returned here.
%
*/
MagickExport void GetPathComponent(const char *path,PathType type,
  char *component)
{
  register char
    *p;

  /*
    Get basename of client.
  */
  assert(path != (const char *) NULL);
  assert(component != (const char *) NULL);
  (void) strncpy(component,path,MaxTextExtent-1);
  if (*path == '\0')
    return;
  for (p=component+(strlen(component)-1); p > component; p--)
    if (IsBasenameSeparator(*p))
      break;
  switch (type)
  {
    case RootPath:
    {
      for (p=component+(strlen(component)-1); p > component; p--)
        if (*p == '.')
          break;
      if (*p == '.')
        *p='\0';
      break;
    }
    case HeadPath:
    {
      *p='\0';
      break;
    }
    case TailPath:
    {
      if (IsBasenameSeparator(*p))
        (void) CloneMemory(component,p+1,strlen(p+1)+1);
      break;
    }
    case BasePath:
    {
      if (IsBasenameSeparator(*p))
        (void) strncpy(component,p+1,MaxTextExtent-1);
      for (p=component+(strlen(component)-1); p > component; p--)
        if (*p == '.')
          {
            *p='\0';
            break;
          }
      break;
    }
    case ExtensionPath:
    {
      if (IsBasenameSeparator(*p))
        (void) strncpy(component,p+1,MaxTextExtent-1);
      for (p=component+(strlen(component)-1); p > component; p--)
        if (*p == '.')
          break;
      *component='\0';
      if (*p == '.')
        (void) strncpy(component,p+1,MaxTextExtent-1);
      break;
    }
  }
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   G e t T o k e n                                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method GetToken gets a token from the token stream.  A token is defined
%  as sequence of characters delimited by whitespace (e.g. clip-path), a
%  sequence delimited with quotes (.e.g "Quote me"), or a sequence enclosed
%  in parenthesis (e.g. rgb(0,0,0)).
%
%  The format of the GetToken method is:
%
%      void GetToken(const char *start,char **end,char *token)
%
%  A description of each parameter follows:
%
%    o start: the start of the token sequence.
%
%    o end: point to the end of the token sequence.
%
%    o token: copy the token to this buffer.
%
%
*/
MagickExport void GetToken(const char *start,char **end,char *token)
{
  register char
    *p;

  register long
    i;

  i=0;
  for (p=(char *) start; *p != '\0'; )
  {
    while (isspace((int) (*p)) && (*p != '\0'))
      p++;
    switch (*p)
    {
      case '"':
      case '\'':
      case '{':
      {
        register char
          escape;

        escape=(*p);
        if (escape == '{')
          escape='}';
        for (p++; *p != '\0'; p++)
        {
          if ((*p == '\\') && ((*(p+1) == escape) || (*(p+1) == '\\')))
            p++;
          else
            if (*p == escape)
              {
                p++;
                break;
              }
          token[i++]=(*p);
        }
        break;
      }
      default:
      {
        char
          *q;

        (void) strtod(p,&q);
        if (p != q)
          {
            for ( ; p < q; p++)
              token[i++]=(*p);
            if (*p == '%')
              token[i++]=(*p++);
            break;
          }
        if (!isalpha((int) *p) && (*p != *DirectorySeparator) && (*p != '#') &&
            (*p != '<'))
          {
            token[i++]=(*p++);
            break;
          }
        for ( ; *p != '\0'; p++)
        {
          if ((isspace((int) *p) || (*p == '=')) && (*(p-1) != '\\'))
            break;
          token[i++]=(*p);
          if (*p == '(')
            for (p++; *p != '\0'; p++)
            {
              token[i++]=(*p);
              if ((*p == ')') && (*(p-1) != '\\'))
                break;
            }
        }
        break;
      }
    }
    break;
  }
  token[i]='\0';
  if (LocaleNCompare(token,"url(#",5) == 0)
    {
      i=(long) strlen(token);
      (void) strncpy(token,token+5,MaxTextExtent-1);
      token[i-6]='\0';
    }
  if (end != (char **) NULL)
    *end=p;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G l o b E x p r e s s i o n                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method GlobExpression returns True if the expression matches the pattern.
%
%  The format of the GlobExpression function is:
%
%      int GlobExpression(const char *expression,const char *pattern)
%
%  A description of each parameter follows:
%
%    o expression: Specifies a pointer to a text string containing a file name.
%
%    o pattern: Specifies a pointer to a text string containing a pattern.
%
%
*/
MagickExport int GlobExpression(const char *expression,const char *pattern)
{
  unsigned int
    done,
    exempt;

  /*
    Return on empty pattern or '*'.
  */
  if (pattern == (char *) NULL)
    return(True);
  if (strlen(pattern) == 0)
    return(True);
  if (LocaleCompare(pattern,"*") == 0)
    return(True);
  if ((strchr(pattern,'[') != (char *) NULL) && IsSubimage(pattern+1,False))
    {
      ExceptionInfo
        exception;

      ImageInfo
        *image_info;

      /*
        Determine if pattern is a subimage, i.e. img0001.pcd[2].
      */
      image_info=CloneImageInfo((ImageInfo *) NULL);
      (void) strncpy(image_info->filename,pattern,MaxTextExtent-1);
      GetExceptionInfo(&exception);
      (void) SetImageInfo(image_info,True,&exception);
      DestroyExceptionInfo(&exception);
      exempt=(LocaleCompare(image_info->magick,"VID") == 0) ||
        (image_info->subimage &&
        (LocaleCompare(expression,image_info->filename) == 0));
      DestroyImageInfo(image_info);
      if (exempt)
        return(False);
    }
  /*
    Evaluate glob expression.
  */
  done=False;
  while ((*pattern != '\0') && !done)
  {
    if (*expression == '\0')
      if ((*pattern != '{') && (*pattern != '*'))
        break;
    switch (*pattern)
    {
      case '\\':
      {
        pattern++;
        if (*pattern != '\0')
          pattern++;
        break;
      }
      case '*':
      {
        int
          status;

        pattern++;
        status=False;
        while ((*expression != '\0') && !status)
          status=GlobExpression((char *) expression++,pattern);
        if (status)
          {
            while (*expression != '\0')
              expression++;
            while (*pattern != '\0')
              pattern++;
          }
        break;
      }
      case '[':
      {
        char
          c;

        pattern++;
        for ( ; ; )
        {
          if ((*pattern == '\0') || (*pattern == ']'))
            {
              done=True;
              break;
            }
          if (*pattern == '\\')
            {
              pattern++;
              if (*pattern == '\0')
                {
                  done=True;
                  break;
                }
             }
          if (*(pattern+1) == '-')
            {
              c=(*pattern);
              pattern+=2;
              if (*pattern == ']')
                {
                  done=True;
                  break;
                }
              if (*pattern == '\\')
                {
                  pattern++;
                  if (*pattern == '\0')
                    {
                      done=True;
                      break;
                    }
                }
              if ((*expression < c) || (*expression > *pattern))
                {
                  pattern++;
                  continue;
                }
            }
          else
            if (*pattern != *expression)
              {
                pattern++;
                continue;
              }
          pattern++;
          while ((*pattern != ']') && (*pattern != '\0'))
          {
            if ((*pattern == '\\') && (*(pattern+1) != '\0'))
              pattern++;
            pattern++;
          }
          if (*pattern != '\0')
            {
              pattern++;
              expression++;
            }
          break;
        }
        break;
      }
      case '?':
      {
        pattern++;
        expression++;
        break;
      }
      case '{':
      {
        int
          match;

        register const char
          *p;

        pattern++;
        while ((*pattern != '}') && (*pattern != '\0'))
        {
          p=expression;
          match=True;
          while ((*p != '\0') && (*pattern != '\0') &&
                 (*pattern != ',') && (*pattern != '}') && match)
          {
            if (*pattern == '\\')
              pattern++;
            match=(*pattern == *p);
            p++;
            pattern++;
          }
          if (*pattern == '\0')
            {
              match=False;
              done=True;
              break;
            }
          else
            if (match)
              {
                expression=p;
                while ((*pattern != '}') && (*pattern != '\0'))
                {
                  pattern++;
                  if (*pattern == '\\')
                    {
                      pattern++;
                      if (*pattern == '}')
                        pattern++;
                    }
                }
              }
            else
              {
                while ((*pattern != '}') && (*pattern != ',') &&
                       (*pattern != '\0'))
                {
                  pattern++;
                  if (*pattern == '\\')
                    {
                      pattern++;
                      if ((*pattern == '}') || (*pattern == ','))
                        pattern++;
                    }
                }
              }
            if (*pattern != '\0')
              pattern++;
          }
        break;
      }
      default:
      {
        if (*expression != *pattern)
          done=True;
        else
          {
            expression++;
            pattern++;
          }
      }
    }
  }
  while (*pattern == '*')
    pattern++;
  return((*expression == '\0') && (*pattern == '\0'));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  I s A c c e s s i b l e                                                    %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsAccessible() returns True if the file as defined by filename exists
%  and is a regular file.
%
%  The format of the IsAccessible method is:
%
%      unsigned int IsAccessible(const char *filename)
%
%  A description of each parameter follows.
%
%    o status:  Method IsAccessible returns True if the file as defined by
%      filename exists and is a regular file, otherwise False is returned.
%
%    o filename:  A pointer to an array of characters containing the filename.
%
%
*/
MagickExport unsigned int IsAccessible(const char *filename)
{
  int
    status;

  struct stat
    file_info;

  if ((filename == (const char *) NULL) || (*filename == '\0'))
    return(False);
  status=stat(filename,&file_info);
  if ((status != 0))
    {
      /* Stat failed, log with errno */
      (void) LogMagickEvent(ConfigureEvent,GetMagickModule(),
        "Tried: %.1024s [%.1024s]",filename,strerror(errno));
      return(False);
    }

  /* Stat succeeded, log without errno and return access flags */
  if (S_ISREG(file_info.st_mode))
    (void) LogMagickEvent(ConfigureEvent,GetMagickModule(),
      "Found: %.1024s",filename);
  else
    (void) LogMagickEvent(ConfigureEvent,GetMagickModule(),
      "Tried: %.1024s [not a regular file]",filename);

  return(S_ISREG(file_info.st_mode));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  I s A c c e s s i b l e A n d N o t E m p t y                              %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsAccessibleAndNotEmpty() returns True if the file as defined by filename
%  exists, is a regular file, and contains at least one byte of data.
%
%  The format of the IsAccessibleAndNotEmpty method is:
%
%      unsigned int IsAccessibleAndNotEmpty(const char *filename)
%
%  A description of each parameter follows.
%
%    o status:  Method IsAccessibleAndNotEmpty returns True if the file as
%      defined by filename exists, is a regular file, and contains content,
%      otherwise False is returned.
%
%    o filename:  A pointer to an array of characters containing the filename.
%
%
*/
MagickExport unsigned int IsAccessibleAndNotEmpty(const char *filename)
{
  int
    status;

  struct stat
    file_info;

  if ((filename == (const char *) NULL) || (*filename == '\0'))
    return(False);
  status=stat(filename,&file_info);

  if ((status == 0) && S_ISREG(file_info.st_mode) && (file_info.st_size > 0))
    return (True);

  return (False);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  I s D i r e c t o r y                                                      %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsDirectory() returns -1 if the directory does not exist, 0 if the
%  directory represents a file, and 1 if the filename represents a directory.
%
%  The format of the IsAccessible method is:
%
%      int IsDirectory(const char *directory)
%
%  A description of each parameter follows.
%
%   o  status:  Method IsDirectory returns -1 if the filename does not exist,
%      0 if the filename represents a file, and 1 if the filename represents
%      a directory.
%
%   o  directory:  Specifies a pointer to an array of characters.  The unique
%      file name is returned in this array.
%
%
*/
static int IsDirectory(const char *directory)
{
  int
    status;

  struct stat
    file_info;

  if ((directory == (const char *) NULL) || (*directory == '\0'))
    return(False);
  status=stat(directory,&file_info);
  if (status != 0)
    return(-1);
  return(S_ISDIR(file_info.st_mode));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
+     I s G e o m e t r y                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method IsGeometry returns True if the geometry specification is valid
%  as determined by GetGeometry.
%
%  The format of the IsGeometry method is:
%
%      unsigned int IsGeometry(const char *geometry)
%
%  A description of each parameter follows:
%
%    o status: Method IsGeometry returns True if the geometry specification
%      is valid otherwise False is returned.
%
%    o geometry: This string is the geometry specification.
%
%
*/
MagickExport unsigned int IsGeometry(const char *geometry)
{
  long
    x,
    y;

  unsigned int
    flags;

  unsigned long
    height,
    width;

  if (geometry == (const char *) NULL)
    return(False);
  flags=GetGeometry(geometry,&x,&y,&width,&height);
  return(flags != NoValue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
+     I s G l o b                                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsGlob() returns True if the path specification contains a globbing patten.
%  as determined by GetGlob.
%
%  The format of the IsGlob method is:
%
%      unsigned int IsGlob(const char *geometry)
%
%  A description of each parameter follows:
%
%    o status: IsGlob() returns True if the path specification contains
%      a globbing patten.
%
%    o path: The path.
%
%
*/
MagickExport unsigned int IsGlob(const char *path)
{
  unsigned int
    status;

  status=(strchr(path,'*') != (char *) NULL) ||
    (strchr(path,'?') != (char *) NULL) ||
    (strchr(path,'{') != (char *) NULL) ||
    (strchr(path,'}') != (char *) NULL) ||
    (strchr(path,'[') != (char *) NULL) ||
    (strchr(path,']') != (char *) NULL);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   L i s t F i l e s                                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method ListFiles reads the directory specified and returns a list
%  of filenames contained in the directory sorted in ascending alphabetic
%  order.
%
%  The format of the ListFiles function is:
%
%      char **ListFiles(const char *directory,const char *pattern,
%        long *number_entries)
%
%  A description of each parameter follows:
%
%    o filelist: Method ListFiles returns a list of filenames contained
%      in the directory.  If the directory specified cannot be read or it is
%      a file a NULL list is returned.
%
%    o directory: Specifies a pointer to a text string containing a directory
%      name.
%
%    o pattern: Specifies a pointer to a text string containing a pattern.
%
%    o number_entries:  This integer returns the number of filenames in the
%      list.
%
%
*/

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

static int FileCompare(const void *x,const void *y)
{
  register char
    **p,
    **q;

  p=(char **) x;
  q=(char **) y;
  return(LocaleCompare(*p,*q));
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

MagickExport char **ListFiles(const char *directory,const char *pattern,
  long *number_entries)
{
  char
    **filelist,
    filename[MaxTextExtent];

  DIR
    *current_directory;

  int
    status;

  struct dirent
    *entry;

  unsigned int
    max_entries;

  /*
    Open directory.
  */
  assert(directory != (const char *) NULL);
  assert(pattern != (char *) NULL);
  assert(number_entries != (long *) NULL);
  *number_entries=0;
  status=chdir(directory);
  if (status != 0)
    return((char **) NULL);
  (void) getcwd(filename,MaxTextExtent-1);
  current_directory=opendir(filename);
  if (current_directory == (DIR *) NULL)
    return((char **) NULL);
  (void) chdir(filename);
  /*
    Allocate filelist.
  */
  max_entries=2048;
  filelist=(char **) AcquireMemory(max_entries*sizeof(char *));
  if (filelist == (char **) NULL)
    {
      (void) closedir(current_directory);
      return((char **) NULL);
    }
  /*
    Save the current and change to the new directory.
  */
  entry=readdir(current_directory);
  while (entry != (struct dirent *) NULL)
  {
    if (*entry->d_name == '.')
      {
        entry=readdir(current_directory);
        continue;
      }
    if ((IsDirectory(entry->d_name) > 0) ||
        GlobExpression(entry->d_name,pattern))
      {
        if (*number_entries >= (int) max_entries)
          {
            /*
              Extend the file list.
            */
            max_entries<<=1;
            ReacquireMemory((void **) &filelist,max_entries*sizeof(char *));
            if (filelist == (char **) NULL)
              {
                (void) closedir(current_directory);
                return((char **) NULL);
              }
          }
#if defined(vms)
        {
          register char
            *p;

          p=strchr(entry->d_name,';');
          if (p)
            *p='\0';
          if (*number_entries > 0)
            if (LocaleCompare(entry->d_name,filelist[*number_entries-1]) == 0)
              {
                entry=readdir(current_directory);
                continue;
              }
        }
#endif
        filelist[*number_entries]=(char *)
          AcquireMemory(strlen(entry->d_name)+MaxTextExtent);
        if (filelist[*number_entries] == (char *) NULL)
          break;
        (void) strncpy(filelist[*number_entries],entry->d_name,MaxTextExtent-1);
        if (IsDirectory(entry->d_name) > 0)
          (void) strcat(filelist[*number_entries],DirectorySeparator);
        (*number_entries)++;
      }
    entry=readdir(current_directory);
  }
  (void) closedir(current_directory);
  /*
    Sort filelist in ascending order.
  */
  qsort((void *) filelist,*number_entries,sizeof(char **),FileCompare);
  return(filelist);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   L o c a l e C o m p a r e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method LocaleCompare compares two strings byte-by-byte, according to
%  the ordering of the current locale encoding. LocaleCompare returns an
%  integer greater than, equal to, or less than 0, if the string pointed
%  to by p is greater than, equal to, or less than the string pointed to
%  by q respectively.  The sign of a non-zero return value is determined
%  by the sign of the difference between the values of the first pair of
%  bytes that differ in the strings being compared.
%
%  The format of the LocaleCompare method is:
%
%      int LocaleCompare(const char *p,const char *q)
%
%  A description of each parameter follows:
%
%    o p: A pointer to a character string.
%
%    o q: A pointer to a character string to compare to p.
%
%
*/
MagickExport int LocaleCompare(const char *p,const char *q)
{
  register unsigned char
    c,
    d;

  if (p == (char *) NULL)
    return(-1);
  if (q == (char *) NULL)
    return(1);
  for ( ; ; )
  {
    c=(unsigned char) *p;
    d=(unsigned char) *q;
    if ((c == '\0') || (AsciiMap[c] != AsciiMap[d]))
      break;
    p++;
    q++;
  }
  return(AsciiMap[c]-AsciiMap[d]);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   L o c a l e L o w e r                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method LocaleLower copies a null terminated string from source to
%  destination (including the null), changing all uppercase letters to
%  lowercase.
%
%  The format of the LocaleLower method is:
%
%      void LocaleLower(char *string)
%
%  A description of each parameter follows:
%
%    o string: A pointer to the string to convert to lower-case Locale.
%
%
*/
MagickExport void LocaleLower(char *string)
{
  register char
    *q;

  assert(string != (char *) NULL);
  for (q=string; *q != '\0'; q++)
    *q=(char) tolower((int) *q);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   L o c a l e N C o m p a r e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method LocaleNCompare compares two strings byte-by-byte, according to
%  the ordering of the currnet locale encoding. LocaleNCompare returns an
%  integer greater than, equal to, or less than 0, if the string pointed
%  to by p is greater than, equal to, or less than the string pointed to
%  by q respectively.  The sign of a non-zero return value is determined
%  by the sign of the difference between the values of the first pair of
%  bytes that differ in the strings being compared.  The LocaleNCompare
%  method makes the same comparison as LocaleCompare but looks at a
%  maximum of n bytes.  Bytes following a null byte are not compared.
%
%  The format of the LocaleNCompare method is:
%
%      int LocaleNCompare(const char *p,const char *q,const size_t n)
%
%  A description of each parameter follows:
%
%    o p: A pointer to a character string.
%
%    o q: A pointer to a character string to compare to p.
%
%    o length: The number of characters to compare in strings p & q.
%
%
*/
MagickExport int LocaleNCompare(const char *p,const char *q,const size_t length)
{
  register size_t
    n;

  register unsigned char
    c,
    d;

  if (p == (char *) NULL)
    return(-1);
  if (q == (char *) NULL)
    return(1);
  for (n=length; n != 0; n--)
  {
    c=(unsigned char) *p;
    d=(unsigned char) *q;
    if (AsciiMap[c] != AsciiMap[d])
      return(AsciiMap[c]-AsciiMap[d]);
    if (c == '\0')
      return(0);
    p++;
    q++;
  }
  return(0);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   L o c a l e U p p e r                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method LocaleUpper copies a null terminated string from source to
%  destination (including the null), changing all lowercase letters to
%  uppercase.
%
%  The format of the LocaleUpper method is:
%
%      void LocaleUpper(char *string)
%
%  A description of each parameter follows:
%
%    o string: A pointer to the string to convert to upper-case Locale.
%
%
*/
MagickExport void LocaleUpper(char *string)
{
  register char
    *q;

  assert(string != (char *) NULL);
  for (q=string; *q != '\0'; q++)
    *q=(char) toupper((int) *q);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  M u l t i l i n e C e n s u s                                              %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method MultilineCensus returns the number of lines within a label.  A line
%  is represented by a \n character.
%
%  The format of the MultilineCenus method is:
%
%      unsigned long MultilineCensus(const char *label)
%
%  A description of each parameter follows.
%
%   o  label:  This character string is the label.
%
%
*/
MagickExport unsigned long MultilineCensus(const char *label)
{
  long
    number_lines;

  /*
    Determine the number of lines within this label.
  */
  if (label == (char *) NULL)
    return(0);
  for (number_lines=1; *label != '\0'; label++)
    if (*label == '\n')
      number_lines++;
  return(number_lines);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S e t C l i e n t N a m e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method SetClientName sets the client name if the name is specified.
%  Otherwise the current client name is returned.
%
%  The format of the SetClientName method is:
%
%      char *SetClientName(const char *name)
%
%  A description of each parameter follows:
%
%    o client_name: Method SetClientName returns the current client name.
%
%    o status: Specifies the new client name.
%
%
*/
MagickExport char *SetClientName(const char *name)
{
  static char
    client_name[MaxTextExtent] = "Magick";

  if ((name != (char *) NULL) && (*name != '\0'))
    (void) strncpy(client_name,name,MaxTextExtent-1);
  return(client_name);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S e t C l i e n t P a t h                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method SetClientPath sets the client path if the name is specified.
%  Otherwise the current client path is returned. A zero-length string
%  is returned if the client path has never been set.
%
%  The format of the SetClientPath method is:
%
%      char *SetClientPath(const char *path)
%
%  A description of each parameter follows:
%
%    o client_path: Method SetClientPath returns the current client path.
%
%    o status: Specifies the new client path.
%
%
*/
MagickExport const char *SetClientPath(const char *path)
{
  static char
    client_path[MaxTextExtent] = "";

  if ((path != (char *) NULL) && (*path != '\0'))
    (void) strncpy(client_path,path,MaxTextExtent-1);
  return(client_path);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S e t G e o m e t r y                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetGeometry sets a grometry to its default values.
%
%  The format of the SetGeometry method is:
%
%      SetGeometry(const Image *image,RectangleInfo *geometry)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o geometry: The geometry.
%
%
*/
MagickExport void SetGeometry(const Image *image,RectangleInfo *geometry)
{
  assert(image != (Image *) NULL);
  assert(geometry != (RectangleInfo *) NULL);
  (void) memset(geometry,0,sizeof(RectangleInfo));
  geometry->width=image->columns;
  geometry->height=image->rows;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  S t r i n g T o A r g v                                                    %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method StringToArgv converts a text string into command line arguments.
%
%  The format of the StringToArgv method is:
%
%      char **StringToArgv(const char *text,int *argc)
%
%  A description of each parameter follows:
%
%    o argv:  Method StringToArgv returns the string list unless an error
%      occurs, otherwise NULL.
%
%    o text:  Specifies the string to segment into a list.
%
%    o argc:  This integer pointer returns the number of arguments in the
%      list.
%
%
*/
MagickExport char **StringToArgv(const char *text,int *argc)
{
  char
    **argv;

  register char
    *p,
    *q;

  register long
    i;

  *argc=0;
  if (text == (char *) NULL)
    return((char **) NULL);
  /*
    Determine the number of arguments.
  */
  for (p=(char *) text; *p != '\0'; )
  {
    while (isspace((int) (*p)))
      p++;
    (*argc)++;
    if (*p == '"')
      for (p++; (*p != '"') && (*p != '\0'); p++);
    if (*p == '\'')
      for (p++; (*p != '\'') && (*p != '\0'); p++);
    while (!isspace((int) (*p)) && (*p != '\0'))
      p++;
  }
  (*argc)++;
  argv=(char **) AcquireMemory((*argc+1)*sizeof(char *));
  if (argv == (char **) NULL)
    MagickFatalError(ResourceLimitFatalError,"MemoryAllocationFailed",
      "UnableToConvertStringToARGV");
  /*
    Convert string to an ASCII list.
  */
  argv[0]=AllocateString("magick");
  p=(char *) text;
  for (i=1; i < *argc; i++)
  {
    while (isspace((int) (*p)))
      p++;
    q=p;
    if (*q == '"')
      {
        p++;
        for (q++; (*q != '"') && (*q != '\0'); q++);
      }
    else
      if (*q == '\'')
        {
          for (q++; (*q != '\'') && (*q != '\0'); q++);
          q++;
        }
      else
        while (!isspace((int) (*q)) && (*q != '\0'))
          q++;
    argv[i]=(char *) AcquireMemory(q-p+MaxTextExtent);
    if (argv[i] == (char *) NULL)
      MagickFatalError(ResourceLimitFatalError,"MemoryAllocationFailed",
        "UnableToConvertStringToARGV");
    (void) strncpy(argv[i],p,q-p);
    argv[i][q-p]='\0';
    p=q;
    while (!isspace((int) (*p)) && (*p != '\0'))
      p++;
  }
  argv[i]=(char *) NULL;
  return(argv);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  S t r i n g T o D o u b l e                                                %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method StringToDouble() converts a text string to a double.  If the string
%  contains a percent sign (e.g. 50%) that percentage of the interval is
%  returned.
%
%  The format of the StringToDouble method is:
%
%      double StringToDouble(const char *text,const double interval)
%
%  A description of each parameter follows:
%
%    o value:  Method StringToDouble returns the converted value.
%
%    o text:  Specifies the string to segment into a list.
%
%    o interval:  Specifies the interval; used for obtaining a percentage.
%
%
*/
MagickExport double StringToDouble(const char *text,const double interval)
{
  char
    *q;

  double
    value;

  value=strtod(text,&q);
  if (strchr(q,'%') != (char *) NULL)
    value*=interval/100.0;
  return(value);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  S t r i n g T o L i s t                                                    %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method StringToList converts a text string into a list by segmenting the
%  text string at each carriage return discovered.  The list is converted to
%  HEX characters if any control characters are discovered within the text
%  string.
%
%  The format of the StringToList method is:
%
%      char **StringToList(const char *text)
%
%  A description of each parameter follows:
%
%    o list:  Method StringToList returns the string list unless an error
%      occurs, otherwise NULL.
%
%    o text:  Specifies the string to segment into a list.
%
%
*/
MagickExport char **StringToList(const char *text)
{
  char
    **textlist;

  register char
    *q;

  register const char
    *p;

  register long
    i;

  unsigned long
    lines;

  if (text == (char *) NULL)
    return((char **) NULL);
  for (p=text; *p != '\0'; p++)
    if (((unsigned char) *p < 32) && !isspace((int) (*p)))
      break;
  if (*p == '\0')
    {
      /*
        Convert string to an ASCII list.
      */
      lines=1;
      for (p=text; *p != '\0'; p++)
        if (*p == '\n')
          lines++;
      textlist=(char **) AcquireMemory((lines+MaxTextExtent)*sizeof(char *));
      if (textlist == (char **) NULL)
        MagickFatalError(ResourceLimitFatalError,"MemoryAllocationFailed",
          "UnableToConvertText");
      p=text;
      for (i=0; i < (long) lines; i++)
      {
        for (q=(char *) p; *q != '\0'; q++)
          if ((*q == '\r') || (*q == '\n'))
            break;
        textlist[i]=(char *) AcquireMemory(q-p+MaxTextExtent);
        if (textlist[i] == (char *) NULL)
          MagickFatalError(ResourceLimitFatalError,"MemoryAllocationFailed",
            "UnableToConvertText");
        (void) strncpy(textlist[i],p,q-p);
        textlist[i][q-p]='\0';
        if (*q == '\r')
          q++;
        p=q+1;
      }
    }
  else
    {
      char
        hex_string[MaxTextExtent];

      register long
        j;

      /*
        Convert string to a HEX list.
      */
      lines=(strlen(text)/0x14)+1;
      textlist=(char **) AcquireMemory((lines+MaxTextExtent)*sizeof(char *));
      if (textlist == (char **) NULL)
        MagickFatalError(ResourceLimitFatalError,"MemoryAllocationFailed",
          "UnableToConvertText");
      p=text;
      for (i=0; i < (long) lines; i++)
      {
        textlist[i]=(char *) AcquireMemory(2*MaxTextExtent);
        if (textlist[i] == (char *) NULL)
          MagickFatalError(ResourceLimitFatalError,"MemoryAllocationFailed",
            "UnableToConvertText");
        FormatString(textlist[i],"0x%08lx: ",0x14*i);
        q=textlist[i]+strlen(textlist[i]);
        for (j=1; j <= (long) Min(strlen(p),0x14); j++)
        {
          FormatString(hex_string,"%02x",*(p+j));
          (void) strncpy(q,hex_string,MaxTextExtent-1);
          q+=2;
          if ((j % 0x04) == 0)
            *q++=' ';
        }
        for (; j <= 0x14; j++)
        {
          *q++=' ';
          *q++=' ';
          if ((j % 0x04) == 0)
            *q++=' ';
        }
        *q++=' ';
        for (j=1; j <= (long) Min(strlen(p),0x14); j++)
        {
          if (isprint((int) (*p)))
            *q++=(*p);
          else
            *q++='-';
          p++;
        }
        *q='\0';
      }
    }
  textlist[i]=(char *) NULL;
  return(textlist);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S t r i p                                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method Strip strips any whitespace or quotes from the beginning and end of
%  a string of characters.
%
%  The format of the Strip method is:
%
%      void Strip(char *message)
%
%  A description of each parameter follows:
%
%    o message: Specifies an array of characters.
%
%
*/
MagickExport void Strip(char *message)
{
  register char
    *p,
    *q;

  assert(message != (char *) NULL);
  if (*message == '\0')
    return;
  if (strlen(message) == 1)
    return;
  p=message;
  while (isspace((int) (*p)))
    p++;
  if ((*p == '\'') || (*p == '"'))
    p++;
  q=message+strlen(message)-1;
  while (isspace((int) (*q)) && (q > p))
    q--;
  if (q > p)
    if ((*q == '\'') || (*q == '"'))
      q--;
  (void) memcpy(message,p,q-p+1);
  message[q-p+1]='\0';
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S u b s t i t u t e S t r i n g                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SubstituteString() performs string substitution on a buffer, replacing the
%  buffer with the substituted version. Buffer must be allocated from the heap.
%
%  The format of the SubstituteString method is:
%
%      void SubstituteString(char **buffer,const char* search,
%        const char *replace)
%
%  A description of each parameter follows:
%
%    o buffer: The buffer to perform replacements on. Replaced with new
%      allocation if a replacement is made.
%
%    o search: String to search for.
%
%    o replace: Replacement string.
%
*/
MagickExport int SubstituteString(char **buffer,const char *search,
  const char *replace)
{
  char
    *destination,
    *result;

  const char
    *match,
    *source;

  size_t
    allocated_length,
    copy_length,
    replace_length,
    result_length,
    search_length;

  assert(buffer != (char**) NULL);
  assert(*buffer != (char *) NULL);
  assert(search != (const char*) NULL);
  assert(replace != (const char*) NULL);
  source=(*buffer);
  match=strstr(source,search);
  if (match == (char *) NULL)
    return(False);
  allocated_length=strlen(source)+MaxTextExtent;
  result=(char *) AcquireMemory(allocated_length);
  if (result == (char *) NULL)
    MagickFatalError(ResourceLimitFatalError,"MemoryAllocationFailed",
      "UnableToAllocateString");
  *result='\0';
  result_length=0;
  destination=result;
  replace_length=strlen(replace);
  search_length=strlen(search);
  while (match != (char*) NULL)
  {
    /*
      Copy portion before match.
    */
    copy_length=match-source;
    if (copy_length != 0)
      {
        result_length+=copy_length;
        if (result_length >= allocated_length)
          {
            allocated_length+=copy_length+MaxTextExtent;
            ReacquireMemory((void **) &result,allocated_length);
            if (result == (char *) NULL)
              MagickFatalError(ResourceLimitFatalError,"MemoryAllocationFailed",
              "UnableToAllocateString");
          }
        (void) strncpy(destination,source,copy_length);
        destination+=copy_length;
        *destination='\0';
      }
      /*
        Copy replacement.
      */
      result_length+=replace_length;
      if (result_length >= allocated_length)
        {
          allocated_length+=replace_length+MaxTextExtent;
          ReacquireMemory((void **) &result,allocated_length);
          if (result == (char *) NULL)
            MagickFatalError(ResourceLimitFatalError,"MemoryAllocationFailed",
              "UnableToAllocateString");
        }
      (void) strcat(destination,replace);
      destination+=replace_length;
      /*
        Find next match.
      */
      source=match;
      source+=search_length;
      match=strstr(source,search);
    }
  /*
    Copy remaining string.
  */
  copy_length=strlen(source);
  result_length+=copy_length;
  if (result_length >= allocated_length)
    {
      allocated_length+=copy_length+MaxTextExtent;
      ReacquireMemory((void **) &result,allocated_length);
      if (result == (char *) NULL)
        MagickFatalError(ResourceLimitFatalError,"MemoryAllocationFailed",
          "UnableToAllocateString");
    }
  (void) strcat(destination,source);
  LiberateMemory((void **)buffer);
  *buffer=result;
  return True;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S y s t e m C o m m a n d                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method SystemCommand executes the specified command and waits until it
%  terminates.  The returned value is the exit status of the command.
%
%  The format of the SystemCommand method is:
%
%      int SystemCommand(const unsigned int verbose,const char *command)
%
%  A description of each parameter follows:
%
%    o status: Method SystemCommand returns False if the command is
%      executed successfully.
%
%    o verbose: An unsigned integer other than 0 prints the executed
%      command before it is invoked.
%
%    o command: This string is the command to execute.
%
%
*/
MagickExport int SystemCommand(const unsigned int verbose,const char *command)
{
  int
    status;

  errno=0;
#if !defined(vms) && !defined(macintosh) && !defined(WIN32)
  status=system(command);
#endif
#if defined(vms)
  status=!system(command);
#endif
#if defined(macintosh)
  status=MACSystemCommand(command);
#endif
#if defined(WIN32)
  status=NTSystemComman(command);
#endif
  if (verbose)
    MagickError(DelegateError,command,!status ? strerror(status) :
      (char *) NULL);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   T o k e n i z e r                                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method Tokenizer is a generalized, finite state token parser.  It extracts
%  tokens one at a time from a string of characters.  The characters used for
%  white space, for break characters, and for quotes can be specified.  Also,
%  characters in the string can be preceded by a specifiable escape character
%  which removes any special meaning the character may have.
%
%  Here is some terminology:
%
%    o token: A single unit of information in the form of a group of
%      characters.
%
%    o white space: Apace that gets ignored (except within quotes or when
%      escaped), like blanks and tabs. in addition, white space terminates a
%      non-quoted token.
%
%    o break set: One or more characters that separates non-quoted tokens.
%      Commas are a common break character. The usage of break characters to
%      signal the end of a token is the same as that of white space, except
%      multiple break characters with nothing or only white space between
%      generate a null token for each two break characters together.
%
%      For example, if blank is set to be the white space and comma is set to
%      be the break character, the line
%
%        A, B, C ,  , DEF
%
%        ... consists of 5 tokens:
%
%        1)  "A"
%        2)  "B"
%        3)  "C"
%        4)  "" (the null string)
%        5)  "DEF"
%
%    o Quote character: A character that, when surrounding a group of other
%      characters, causes the group of characters to be treated as a single
%      token, no matter how many white spaces or break characters exist in
%      the group. Also, a token always terminates after the closing quote.
%      For example, if ' is the quote character, blank is white space, and
%      comma is the break character, the following string
%
%        A, ' B, CD'EF GHI
%
%        ... consists of 4 tokens:
%
%        1)  "A"
%        2)  " B, CD" (note the blanks & comma)
%        3)  "EF"
%        4)  "GHI"
%
%      The quote characters themselves do not appear in the resultant
%      tokens.  The double quotes are delimiters i use here for
%      documentation purposes only.
%
%    o Escape character: A character which itself is ignored but which
%      causes the next character to be used as is.  ^ and \ are often used
%      as escape characters. An escape in the last position of the string
%      gets treated as a "normal" (i.e., non-quote, non-white, non-break,
%      and non-escape) character. For example, assume white space, break
%      character, and quote are the same as in the above examples, and
%      further, assume that ^ is the escape character. Then, in the string
%
%        ABC, ' DEF ^' GH' I ^ J K^ L ^
%
%        ... there are 7 tokens:
%
%        1)  "ABC"
%        2)  " DEF ' GH"
%        3)  "I"
%        4)  " "     (a lone blank)
%        5)  "J"
%        6)  "K L"
%        7)  "^"     (passed as is at end of line)
%
%  The format of the Tokenizer method is:
%
%      int Tokenizer(TokenInfo *token_info,unsigned flag,char *token,
%        size_t max_token_length,char *line,char *white,char *break_set,
%        char *quote,char escape,char *breaker,int *next,char *quoted)
%
%  A description of each parameter follows:
%
%    o flag: right now, only the low order 3 bits are used.
%
%        1 => convert non-quoted tokens to upper case
%        2 => convert non-quoted tokens to lower case
%        0 => do not convert non-quoted tokens
%
%    o token: a character string containing the returned next token
%
%    o max_token_length: the maximum size of "token".  Characters beyond
%      "max_token_length" are truncated.
%
%    o string: the string to be parsed.
%
%    o white: a string of the valid white spaces.  example:
%
%        char whitesp[]={" \t"};
%
%      blank and tab will be valid white space.
%
%    o break: a string of the valid break characters. example:
%
%        char breakch[]={";,"};
%
%      semicolon and comma will be valid break characters.
%
%    o quote: a string of the valid quote characters. An example would be
%
%        char whitesp[]={"'\"");
%
%      (this causes single and double quotes to be valid) Note that a
%      token starting with one of these characters needs the same quote
%      character to terminate it.
%
%      for example:
%
%        "ABC '
%
%      is unterminated, but
%
%        "DEF" and 'GHI'
%
%      are properly terminated.  Note that different quote characters
%      can appear on the same line; only for a given token do the quote
%      characters have to be the same.
%
%    o escape: the escape character (NOT a string ... only one
%      allowed). Use zero if none is desired.
%
%    o breaker: the break character used to terminate the current
%      token.  If the token was quoted, this will be the quote used.  If
%      the token is the last one on the line, this will be zero.
%
%    o next: this variable points to the first character of the
%      next token.  it gets reset by "tokenizer" as it steps through the
%      string.  Set it to 0 upon initialization, and leave it alone
%      after that.  You can change it if you want to jump around in the
%      string or re-parse from the beginning, but be careful.
%
%    o quoted: set to True if the token was quoted and False
%      if not.  You may need this information (for example:  in C, a
%      string with quotes around it is a character string, while one
%      without is an identifier).
%
%    o result: 0 if we haven't reached EOS (end of string), and 1
%      if we have.
%
*/

#define IN_WHITE 0
#define IN_TOKEN 1
#define IN_QUOTE 2
#define IN_OZONE 3

static long sindex(char c,char *string)
{
  register char
    *p;

  for (p=string; *p; p++)
    if (c == (*p))
      return(p-string);
  return(-1);
}

static void StoreToken(TokenInfo *token_info,char *string,
  size_t max_token_length,char c)
{
  register long
    i;

  if ((token_info->offset < 0) ||
      ((size_t) token_info->offset >= (max_token_length-1)))
    return;
  i=token_info->offset++;
  string[i]=c;
  if (token_info->state == IN_QUOTE)
    return;
  switch (token_info->flag & 0x03)
  {
    case 1:
    {
      string[i]=toupper(c);
      break;
    }
    case 2:
    {
      string[i]=tolower(c);
      break;
    }
    default:
      break;
  }
}

MagickExport int Tokenizer(TokenInfo *token_info,unsigned flag,char *token,
  size_t max_token_length,char *line,char *white,char *break_set,char *quote,
  char escape,char *breaker,int *next,char *quoted)
{
  char
    c;

  register long
    i;

  *breaker=False;
  *quoted=False;
  if (!line[*next])
    return(1);
  token_info->state=IN_WHITE;
  token_info->quote=False;
  token_info->flag=flag;
  for (token_info->offset=0; line[*next] != 0; (*next)++)
  {
    c=line[*next];
    i=sindex(c,break_set);
    if (i >= 0)
      {
        switch (token_info->state)
        {
          case IN_WHITE:
          case IN_TOKEN:
          case IN_OZONE:
          {
            (*next)++;
            *breaker=break_set[i];
            token[token_info->offset]=0;
            return(0);
          }
          case IN_QUOTE:
          {
            StoreToken(token_info,token,max_token_length,c);
            break;
          }
        }
        continue;
      }
    i=sindex(c,quote);
    if (i >= 0)
      {
        switch(token_info->state)
        {
          case IN_WHITE:
          {
            token_info->state=IN_QUOTE;
            token_info->quote=quote[i];
            *quoted=True;
            break;
          }
          case IN_QUOTE:
          {
            if (quote[i] != token_info->quote)
              StoreToken(token_info,token,max_token_length,c);
            else
              {
                token_info->state=IN_OZONE;
                token_info->quote=0;
              }
            break;
          }
          case IN_TOKEN:
          case IN_OZONE:
          {
            *breaker=c;
            token[token_info->offset]=0;
            return(0);
          }
        }
        continue;
      }
    i=sindex(c,white);
    if (i >= 0)
      {
        switch(token_info->state)
        {
          case IN_WHITE:
          case IN_OZONE:
            break;
          case IN_TOKEN:
          {
            token_info->state=IN_OZONE;
            break;
          }
          case IN_QUOTE:
          {
            StoreToken(token_info,token,max_token_length,c);
            break;
          }
        }
        continue;
      }
    if (c == escape)
      {
        if (line[(*next)+1] == 0)
          {
            *breaker=0;
            StoreToken(token_info,token,max_token_length,c);
            (*next)++;
            token[token_info->offset]=0;
            return(0);
          }
        switch(token_info->state)
        {
          case IN_WHITE:
          {
            (*next)--;
            token_info->state=IN_TOKEN;
            break;
          }
          case IN_TOKEN:
          case IN_QUOTE:
          {
            (*next)++;
            c=line[*next];
            StoreToken(token_info,token,max_token_length,c);
            break;
          }
          case IN_OZONE:
          {
            token[token_info->offset]=0;
            return(0);
          }
        }
        continue;
      }
    switch(token_info->state)
    {
      case IN_WHITE:
        token_info->state=IN_TOKEN;
      case IN_TOKEN:
      case IN_QUOTE:
      {
        StoreToken(token_info,token,max_token_length,c);
        break;
      }
      case IN_OZONE:
      {
        token[token_info->offset]=0;
        return(0);
      }
    }
  }
  token[token_info->offset]=0;
  return(0);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   T r a n s l a t e T e x t                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Method TranslateText replaces any embedded formatting characters with
%  the appropriate image attribute and returns the translated text.
%
%  The format of the TranslateText method is:
%
%      char *TranslateText(const ImageInfo *image_info,Image *image,
%        const char *formatted_text)
%
%  A description of each parameter follows:
%
%    o translated_text:  Method TranslateText returns the translated
%      text string.
%
%    o image_info: The imageInfo.
%
%    o image: The image.
%
%    o formatted_text: The address of a character string containing the embedded
%      formatting characters.
%
%
*/
MagickExport char *TranslateText(const ImageInfo *image_info,Image *image,
  const char *formatted_text)
{
  char
    filename[MaxTextExtent],
    *text,
    *translated_text;

  const ImageAttribute
    *attribute;

  ImageInfo
    *clone_info;

  register char
    *p,
    *q;

  register long
    i;

  size_t
    length;

  assert(image != (Image *) NULL);
  if ((formatted_text == (const char *) NULL) || (*formatted_text == '\0'))
    return((char *) NULL);
  text=(char *) formatted_text;
  if ((*text == '@') && IsAccessible(text+1))
    {
      text=(char *) FileToBlob(text+1,&length,&image->exception);
      if (text == (char *) NULL)
        return((char *) NULL);
    }
  /*
    Translate any embedded format characters.
  */
  length=strlen(text)+MaxTextExtent;
  translated_text=AllocateString(text);
  clone_info=CloneImageInfo(image_info);
  p=text;
  for (q=translated_text; *p != '\0'; p++)
  {
    *q='\0';
    if ((size_t) (q-translated_text+MaxTextExtent) >= length)
      {
        length<<=1;
        ReacquireMemory((void **) &translated_text,length);
        if (translated_text == (char *) NULL)
          break;
        q=translated_text+strlen(translated_text);
      }
    /*
      Process formatting characters in text.
    */
    if ((*p == '\\') && (*(p+1) == 'r'))
      {
        *q++='\r';
        p++;
        continue;
      }
    if ((*p == '\\') && (*(p+1) == 'n'))
      {
        *q++='\n';
        p++;
        continue;
      }
    if (*p != '%')
      {
        *q++=(*p);
        continue;
      }
    p++;
    switch (*p)
    {
      case 'b':
      {
        char
          format[MaxTextExtent];

        FormatSize(GetBlobSize(image),format);
        (void) strcat(q,format);
        while (*q != '\0')
          q++;
        break;
      }
      case 'c':
      {
        attribute=GetImageAttribute(image,"comment");
        if (attribute == (ImageAttribute *) NULL)
          break;
        (void) strncpy(q,attribute->value,MaxTextExtent-1);
        q+=strlen(attribute->value);
        break;
      }
      case 'd':
      case 'e':
      case 'f':
      case 't':
      {
        /*
          Label segment is the base of the filename.
        */
        if (strlen(image->magick_filename) == 0)
          break;
        switch (*p)
        {
          case 'd':
          {
            GetPathComponent(image->magick_filename,HeadPath,filename);
            (void) strncpy(q,filename,MaxTextExtent-1);
            q+=strlen(filename);
            break;
          }
          case 'e':
          {
            GetPathComponent(image->magick_filename,ExtensionPath,filename);
            (void) strncpy(q,filename,MaxTextExtent-1);
            q+=strlen(filename);
            break;
          }
          case 'f':
          {
            GetPathComponent(image->magick_filename,TailPath,filename);
            (void) strncpy(q,filename,MaxTextExtent-1);
            q+=strlen(filename);
            break;
          }
          case 't':
          {
            GetPathComponent(image->magick_filename,BasePath,filename);
            (void) strncpy(q,filename,MaxTextExtent-1);
            q+=strlen(filename);
            break;
          }
        }
        break;
      }
      case 'g':
      {
        FormatString(q,"0x%lx",clone_info->group);
        while (*q != '\0')
          q++;
        break;
      }
      case 'h':
      {
        FormatString(q,"%lu",image->magick_rows ? image->magick_rows : 256);
        while (*q != '\0')
          q++;
        break;
      }
      case 'i':
      {
        (void) strncpy(q,image->filename,MaxTextExtent-1);
        q+=strlen(image->filename);
        break;
      }
      case 'k':
      {
        FormatString(q,"%lu",GetNumberColors(image,(FILE *) NULL,
          &image->exception));
        while (*q != '\0')
          q++;
        break;
      }
      case 'l':
      {
        attribute=GetImageAttribute(image,"label");
        if (attribute == (ImageAttribute *) NULL)
          break;
        (void) strncpy(q,attribute->value,MaxTextExtent-1);
        q+=strlen(attribute->value);
        break;
      }
      case 'm':
      {
        (void) strncpy(q,image->magick,MaxTextExtent-1);
        q+=strlen(image->magick);
        break;
      }
      case 'n':
      {
        FormatString(q,"%lu",(unsigned long) GetImageListLength(image));
        while (*q != '\0')
          q++;
        break;
      }
      case 'o':
      {
        (void) strncpy(q,clone_info->filename,MaxTextExtent-1);
        q+=strlen(clone_info->filename);
        break;
      }
      case 'p':
      {
        register const Image
          *p;

        unsigned long
          page;

        p=image;
        for (page=1; p->previous != (Image *) NULL; page++)
          p=p->previous;
        FormatString(q,"%lu",page);
        while (*q != '\0')
          q++;
        break;
      }
      case 'q':
      {
        FormatString(q,"%lu",GetImageDepth(image,&image->exception));
        while (*q != '\0')
          q++;
        break;
      }
      case 'r':
      {
        static char
          *ClassTypes[] =
          {
            (char *) "Undefined", (char *) "DirectClass",
            (char *) "PseudoClass", (char *) NULL
          },
          *ColorspaceTypes[] =
          {
            (char *) "undefined", (char *) "RGB", (char *) "Gray",
            (char *) "Transparent", (char *) "OHTA", (char *) "XYZ",
            (char *) "YCbCr", (char *) "YCC", (char *) "YIQ", (char *) "YPbPr",
            (char *) "YUV", (char *) "CMYK", (char *) "sRGB", (char *) NULL
          };

        FormatString(q,"%s%s%s",ClassTypes[image->storage_class],
          ColorspaceTypes[image->colorspace],(image->matte ? "Matte" : ""));
        while (*q != '\0')
          q++;
        break;
      }
      case 's':
      {
        FormatString(q,"%lu",image->scene);
        if (clone_info->subrange != 0)
          FormatString(q,"%lu",clone_info->subimage);
        while (*q != '\0')
          q++;
        break;
      }
      case 'u':
      {
        (void) strncpy(filename,clone_info->unique,MaxTextExtent-1);
        if (*filename == '\0')
          if (!AcquireTemporaryFileName(filename))
            break;
        (void) strncpy(q,filename,MaxTextExtent-1);
        q+=strlen(filename);
        break;
      }
      case 'w':
      {
        FormatString(q,"%lu",
          image->magick_columns ? image->magick_columns : 256);
        while (*q != '\0')
          q++;
        break;
      }
      case 'x':
      {
        FormatString(q,"%g",image->x_resolution);
        while (*q != '\0')
          q++;
        break;
      }
      case 'y':
      {
        FormatString(q,"%g",image->y_resolution);
        while (*q != '\0')
          q++;
        break;
      }
      case 'z':
      {
        (void) strncpy(filename,clone_info->zero,MaxTextExtent-1);
        if (*filename == '\0')
          if (!AcquireTemporaryFileName(filename))
            break;
        (void) strncpy(q,filename,MaxTextExtent-1);
        q+=strlen(filename);
        break;
      }
      case '[':
      {
        char
          key[MaxTextExtent];

        ExtendedSignedIntegralType
          offset;

        if (strchr(p,']') == (char *) NULL)
          break;
        p++;
        for (i=0; (i < MaxTextExtent) && (*p != ']'); i++)
          key[i]=(*p++);
        key[i]='\0';
        attribute=GetImageAttribute(image,key);
        if (attribute != (const ImageAttribute *) NULL)
          {
            offset=strlen(attribute->value);
            if ((size_t) (q-translated_text+offset) >= length)
              {
                length+=(offset+MaxTextExtent);
                ReacquireMemory((void **) &translated_text,length);
                if (translated_text == (char *) NULL)
                  break;
                q=translated_text+strlen(translated_text);
              }
            (void) strcpy(q,attribute->value);
            q+=offset;
            break;
          }
        attribute=GetImageInfoAttribute(clone_info,image,key);
        if (attribute == (ImageAttribute *) NULL)
          break;
        offset=strlen(attribute->value);
        if ((size_t) (q-translated_text+offset) >= length)
          {
            length+=(offset+MaxTextExtent);
            ReacquireMemory((void **) &translated_text,length);
            if (translated_text == (char *) NULL)
              break;
            q=translated_text+strlen(translated_text);
          }
        (void) strcpy(q,attribute->value);
        q+=offset;
        break;
      }
      case '#':
      {
        (void) SignatureImage(image);
        attribute=GetImageAttribute(image,"signature");
        if (attribute == (ImageAttribute *) NULL)
          break;
        (void) strncpy(q,attribute->value,MaxTextExtent-1);
        q+=strlen(attribute->value);
        break;
      }
      case '%':
      {
        *q++=(*p);
        break;
      }
      default:
      {
        *q++='%';
        *q++=(*p);
        break;
      }
    }
  }
  *q='\0';
  DestroyImageInfo(clone_info);
  if (text != (char *) formatted_text)
    LiberateMemory((void **) &text);
  return(translated_text);
}
