// This may look like C code, but it is really -*- C++ -*-
//
// Copyright Bob Friesenhahn, 1999, 2000, 2001, 2002
//
// Blob reference class
//
// This is an internal implementation class that should not be
// accessed by users.
//

#if !defined(Magick_Blob_header)
#define Magick_Blob_header

#include "Magick++/Include.h"
#include "Magick++/Thread.h"
#include "Magick++/Blob.h"

namespace Magick
{

  class BlobRef {
    friend class Blob;
  public:
    // There are no public methods in this class

  private:
    // Construct with data, making private copy of data
    BlobRef ( const void* data_, size_t length_ );

    // Destructor (actually destroys data)
    ~BlobRef ( void );

    // Copy constructor and assignment are not supported
    BlobRef (const BlobRef&);
    BlobRef& operator= (const BlobRef&);

    void *          _data;     // Blob data
    size_t          _length;   // Blob length
    Blob::Allocator _allocator; // Memory allocation system in use
    int             _refCount; // Reference count
    MutexLock       _mutexLock;// Mutex lock
  };

} // namespace Magick

#endif // Magick_Blob_header
