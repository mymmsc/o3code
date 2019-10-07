#include "bli_random.h"
//#include <api_time.h>
#include <sys/time.h>
//////////////////////////////////////////////////////////////////////////////////////////

#define RANDOM_M       0x7FFFFFFF            // (2 << 30) - 1; // largest pos. integer: (1<<31)-1: 2,147,483,647
#define RANDOM_A       16807                 // m = a * q + r
#define RANDOM_Q       (RANDOM_M / RANDOM_A) // 127,773
#define RANDOM_R       (RANDOM_M % RANDOM_A) // 2,836
#define RANDOM_RAND(s) (RANDOM_A * ((s) % RANDOM_Q) - RANDOM_R * ((s) / RANDOM_Q))

//////////////////////////////////////////////////////////////////////////////////////////

API_DECLARE(void) bli_random_init(bli_random_t *r)
{

	struct timeval t;
    gettimeofday(&t, NULL);
    r->seed = (int)t.tv_sec;//t.tv_usec
}

API_DECLARE(void) bli_random_init_ex(bli_random_t *r, api_uint32_t seed)
{
	r->seed = seed; 
}

API_DECLARE(double) bli_random_rand(bli_random_t *r)
{
    double z;
	
	if (r->seed == 0)
    {
		r->seed = 1;
    }
    //r->seed = RANDOM_A * (r->seed % RANDOM_Q) - RANDOM_R * (r->seed / RANDOM_Q);
    r->seed = RANDOM_RAND(r->seed);
    if (r->seed < 0)
    {
        r->seed = r->seed + RANDOM_M;
    }
    z = (r->seed * 1.0) / RANDOM_M;
    return z;
}

API_DECLARE(void) bli_random_skip(bli_random_t *r, api_uint32_t num)
{
    api_uint32_t i;
	for (i = 0 ; i < num ; i ++ )	
	{
		if (r->seed == 0)
        {
			r->seed = 1;
        }
		//r->seed = RANDOM_A * (r->seed % RANDOM_Q) - RANDOM_R * (r->seed / RANDOM_Q);
        r->seed = RANDOM_RAND(r->seed);
		if (r->seed < 0)
        {
			r->seed = r->seed + RANDOM_M;
        }
    }
}
