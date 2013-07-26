/* BSP includes. */
#include <xtmrctr.h>

#include <const.h>
#include <types.h>

PUBLIC err_t
microblaze_sanitize_cache()
{
	/* Tasks inherit the exception and cache configuration of the MicroBlaze
	at the point that they are created. */
	#if MICROBLAZE_EXCEPTIONS_ENABLED == 1
		microblaze_enable_exceptions();
	#endif

	#if XPAR_MICROBLAZE_USE_ICACHE == 1
		microblaze_invalidate_icache();
		microblaze_enable_icache();
	#endif

	#if XPAR_MICROBLAZE_USE_DCACHE == 1
		microblaze_invalidate_dcache();
		microblaze_enable_dcache();
	#endif

	return E_GOOD;
}
