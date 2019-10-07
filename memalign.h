#ifndef O3CODE_MEMALIGN_H
#define O3CODE_MEMALIGN_H

// System and environment related definitions.

// To setup platform-specific settings
//#include <platform/system.h>

#ifndef API_FREEBSD
#  ifndef API_WIN32 // Linux
#    include <malloc.h>		// for memalign
#    define allocAlignedMem(bytes)     memalign(16, bytes)
#    define freeAlignedMem(buff)       free(buff)
#  else // Windows
#    include <malloc.h>		// for memalign
#    define memalign(blocksize, bytes) malloc(bytes)
#    define allocAlignedMem(bytes)     _aligned_malloc(bytes, 16)
#    define freeAlignedMem(buff)       _aligned_free(buff)
#  endif
#else // Mac OS X
#  include <stdlib.h>		// for malloc
#  define allocAlignedMem(bytes)       malloc(bytes)
#  define freeAlignedMem(buff)         free(buff)
#endif

#endif /* ! O3CODE_MEMALIGN_H */
