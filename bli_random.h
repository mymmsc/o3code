#ifndef O3CODE_BLI_RANDOM_H
#define O3CODE_BLI_RANDOM_H

//////////////////////////////////////////////////////////////////////////////////////////

#include <api.h>
//////////////////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup bli_random Random Operations - The pseudo-random number generator
 * @ingroup bli 
 * @{
 */

typedef struct bli_random_t bli_random_t;
struct bli_random_t
{
    api_uint32_t seed;
};

/**
 * Set a random seed for the pseudo-random number generator.
 */
API_DECLARE(void) bli_random_init(bli_random_t *r);

/**
 * Set a specific seed for the pseudo-random number generator.
 * @param s the seed for the pseudo-random number generator.
 */
API_DECLARE(void) bli_random_init_ex(bli_random_t *r, uint32_t seed);

/**
 * Generate a uniformly distributed random number in (0, 1).
 *
 * @return the random number in (0, 1).
 */
API_DECLARE(double) bli_random_rand(bli_random_t *r);

API_DECLARE(void) bli_random_skip(bli_random_t *r, uint32_t num);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ! O3CODE_BLI_RANDOM_H */
