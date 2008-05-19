#ifndef __ENDIANESS_HANDLER__

#ifndef __APPLE__
 #include <endian.h>
#else
 # include <machine/endian.h>
#endif

/* Get machine dependent optimized versions of byte swapping functions.  */
#include <byteswap.h>

#ifdef __OPTIMIZE__
/* We can optimize calls to the conversion functions.  Either nothing has
   to be done or we are using directly the byte-swapping functions which
   often can be inlined.  */

	#if __BYTE_ORDER == __BIG_ENDIAN
		#define le16_to_cpu(x)	__bswap_16 (x)
		#define le32_to_cpu(x)	__bswap_32 (x)
		#define le64_to_cpu(x)	__bswap_64 (x)
		#define cpu_to_le16(x)	__bswap_16 (x)
		#define cpu_to_le32(x)	__bswap_32 (x)
		#define cpu_to_le64(x)	__bswap_64 (x)
	#else
		#if __BYTE_ORDER == __LITTLE_ENDIAN
			#define le16_to_cpu(x)	(x)
			#define le32_to_cpu(x)	(x)
			#define cpu_to_le16(x)	(x)
			#define cpu_to_le16(x)	(x)
		#else
			#error "Unknown endianess"
		#endif
	#endif
#endif

#define __ENDIANESS_HANDLER__
#endif

