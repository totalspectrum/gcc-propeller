#if defined(__PROPELLER_XMM__) || defined(__PROPELLER_XMMC__)
	/* both debug and regular ar in the same kernel */
#  include "crt0_xmm.s"
#elif defined(__PROPELLER_CMM__)
	/* note: at the moment the same CMM source is used for both P1 and P2 */
	/* also, the debug source is the same as regular source, with */
	/* #ifdefs applied as necessary */
#    include "crt0_cmm.s"

#elif defined(__PROPELLER_LMM__)
# if defined(DEBUG_KERNEL)
	/* note: the debug kernel is the same for prop1 and prop2 */
#    include "crt0_lmm_debug.s"
# else
#    include "crt0_lmm.s"
# endif
#elif defined(__PROPELLER_COG__)
	/* NOTE: we actually build the real crt0_cog.o from the library,
	   so this probably does not need to be here
	*/
#include "crt0_cog.s"
#else
#error "unknown propeller configuration"
#endif
