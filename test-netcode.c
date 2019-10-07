// main.c : 定义控制台应用程序的入口点。
//
#include "netcode.h"
//#include <api_time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define CODEV12 0

#include <sys/time.h>

//typedef int api_status_t;
typedef api_int64_t api_time_t;
static api_time_t api_time_now_OLD(void)
{
	struct timeval t;
    gettimeofday(&t, NULL);
    return t.tv_sec * 1000000+t.tv_usec;
}

#include <time.h>

/// @brief A wrapper for getting the current time.
/// @returns The current time.
static api_time_t api_time_now(void)
{
    struct timespec t;
    clock_gettime(CLOCK_REALTIME, &t);
    return t.tv_sec * 1000000 + t.tv_nsec / 1000;
}

/// @brief Calculates the time difference between two struct timespecs
/// @param t1 The first time.
/// @param t2 The second time.
/// @returns The difference between the two times.
double get_elapsed(struct timespec t1, struct timespec t2)
{
    double ft1 = t1.tv_sec + ((double)t1.tv_nsec / 1000000000.0);
    double ft2 = t2.tv_sec + ((double)t2.tv_nsec / 1000000000.0);
    return ft2 - ft1;
}

static api_int64_t api_time_sec_OLD(api_time_t t)
{
	return t/1000000;
}

static api_int64_t api_time_sec(api_time_t t)
{
	return t;
}


#if !CODEV12
int main(int argc, char * const argv[])
{
	api_status_t    rv         = API_SUCCESS;
	netcode_t       encodec; // 编码器
	netcode_t       decodec; // 解码器
	int	            nBlockSize = 1024;//16;
	int             nNumBlocks = 64;//96;
	api_byte_t    **original   = NULL;
	api_byte_t    **lpEncode   = NULL;
	api_byte_t    **lpDecode   = NULL;
    api_uint32_t   *seed       = NULL;
    int i, j, k, m;
	api_time_t      t;
    int             nTimes     = 20;
    double          nBytes     = 0;
    double          nSecsForE  = 0;
    double          nSecsForD  = 0;
	//srand(100);
	original = (api_byte_t **)malloc(sizeof(api_byte_t *) * nNumBlocks);
	lpEncode = (api_byte_t **)malloc(sizeof(api_byte_t *) * nNumBlocks);
    seed = (api_uint32_t *)malloc(sizeof(api_uint32_t) * nBlockSize);
    
#if 1
    //------------------< 全部编解码 >------------------
for(m = 0; m < nTimes; m++)
{
    //------------------< 编码 >------------------
    rv = netcode_create(&encodec, nBlockSize, nNumBlocks);
	
	for (i = 0; i < nNumBlocks; i ++)
	{
		original[i] = (api_byte_t *)allocAlignedMem(sizeof(api_byte_t) * (nBlockSize / sizeof(api_byte_t)));
        lpEncode[i] = (api_byte_t *)allocAlignedMem(sizeof(api_byte_t) * (nBlockSize / sizeof(api_byte_t)));
		for (j = 0; j < (nBlockSize / (int)sizeof(api_byte_t)); j ++)
		{
			original[i][j] = 1 + (int)(rand() * 255) ; //(int)(nc->r->rand() * 256);
			//	assert(original[i][j] != 0);
		}
        netcode_push(&encodec, original[i]);
	}
    t = api_time_now();
	for (i = 0; i < nNumBlocks; i ++)
    {
	    netcode_encode(&encodec, &seed[i], lpEncode[i]);
    }
    nSecsForE += api_time_now() - t;
	//------------------< 解码 >------------------
    t = api_time_now();
	rv = netcode_create(&decodec, nBlockSize, nNumBlocks);
	for(k = 0; k < nNumBlocks; k++)
	{
		rv = netcode_decode(&decodec, seed[k], lpEncode[k], &lpDecode);
	}
    nSecsForD += api_time_now() - t;
	if(lpDecode != NULL)
	{
		// Compare the original with decoded results.
		for (i = 0; i < nNumBlocks; i ++)
		{
			if ( memcmp(original[i], lpDecode[i], nBlockSize) != 0 )
			{
				lpDecode = NULL;
                break;
			}
		}
	}
	if(lpDecode != NULL)
	{
		printf("decode success!\n");
	}
	else
	{
		printf("decode failed!\n");
	}
    
	//api_tgcode_close(encodec);
	//api_tgcode_close(decodec);
    nBytes += nBlockSize * nNumBlocks;
}
	nBytes *= 1000000;
    nSecsForE = api_time_sec(nSecsForE);
    nSecsForD = api_time_sec(nSecsForD);
    printf("times encode/decode = %.3f / %.3f, speed = %.3f / %.3f\n", nSecsForE, nSecsForD, nBytes / nSecsForE, nBytes / nSecsForD);
#endif
#if 0
	//------------------< 分块编解码 >------------------
	lpDecode = NULL;
	rv = apn_netcode_create(&encodec, nBlockSize, nNumBlocks, 1);
	rv = apn_netcode_create(&decodec, nBlockSize, nNumBlocks, 1);
	xint_t **tmpEncode = new xint_t *[nNumBlocks];
	for (int i = 0; i < nNumBlocks; i ++)
	{
		tmpEncode[i] = (xint_t *) apn_alloc(nBlockSize);
	}
	for (int i = 0; i < nNumBlocks; i ++)
	{
		//int tmpSeed = 0;
		original[i] = (xint_t *) apn_alloc(nBlockSize);
		for (int j = 0; j < nBlockSize; j ++)
		{
			original[i][j] = 1 + (int)(rand() * 255);
		}
		xint_t *tmpEn = apn_netcode_encode(encodec, original[i], &seed[i]);
		for (int k = 0; k < nBlockSize; k ++)
		{
			tmpEncode[i][k] = tmpEn[k];
		}
		lpDecode = apn_netcode_decode(decodec, tmpEncode[i], seed[i]);
		printf("i = %d, %p, tmpSeed = %d\n", i, lpDecode, seed[i]);
	}
	if(lpDecode != NULL)
	{
		// Compare the original with decoded results.
		for (int i = 0; i < nNumBlocks; i ++)
		{
			if ( memcmp(original[i], lpDecode[i], nBlockSize) != 0 )
			{
				lpDecode = NULL;
				break;
			}
		}
	}
	if(lpDecode != NULL)
	{
		printf("解码成功!\n");
	}
	else
	{
		printf("解码失败!\n");
	}
	//------------------< 释放资源 >------------------
	for (int i = 0; i < nNumBlocks; i ++)
	{
		delete[] original[i];
		original[i] = NULL;
	}
	delete[] original;
	original = NULL;
	apn_netcode_close(&encodec);
	apn_netcode_close(&decodec);
	apn_netcode_destory();
#endif
	return 0;
}
#else

#include "tgcode.h"
#include <assert.h>
#include <api_time.h>
#include <iostream>
#include <string>

static char* g_mode_desc[] = { "base", "accelerated", "threaded", 
//							   "threaded_test", "threaded_test2",
"threaded_aggresive",
} ;

enum NetcodeType {	
	NETCODE_CLASSIC=0,
	NETCODE_ACCELERATED,
	NETCODE_THREADED,
	//	NETCODE_THREADED_TEST,
	//	NETCODE_THREADED_TEST2,
	NETCODE_THREADED_AGGRESSIVE,
}  ;
using namespace std;
NetcodeType g_mode = NETCODE_CLASSIC;
bool g_threads_num = 1;
int	 g_blockSize = 1024;//16;
int  g_numBlocks = 256;//96;
bool g_thread_binding = false;
int  g_test = 0;
bool g_dump_encoded_output = false;
char* g_output_filename = "";

#define TEST 0

bool encode_decode_test(Random *r, iNetcode * codec,
						int iterations,
						int blockSize, int numBlocks,
						int * seed, xint_t ** original, xint_t ** encoded,
						api_time_t& encodingTime, api_time_t& decodingTime,
						NetcodeType mode)
{
	xint_t ** decoded = NULL;
	api_time_t before, after;
	bool success = false;
    int iter, i , j;
	for ( iter = 0 ; iter < iterations ; iter ++ )
	{
		// reset the codec for encoding
		codec->reset();
        
		for (i = 0; i < numBlocks; i ++)
		{
			for (j = 0; j < blockSize; j ++)
			{
				original[i][j] = 1 + (int)(r -> rand() * 255);		
			}
			
			codec -> addOriginal(original[i]);
#if TEST
			before.setnow();
			seed[i] = codec -> encode(encoded[i]);
			after.setnow();
			encodingTime += after.subUsec(before);
#endif
		}

		//cout << "Encoding process begins." << endl;
#if !TEST
		before = api_time_now();
		for (i = 0; i < numBlocks; i ++)
		{
			seed[i] = codec -> encode(encoded[i]);
		}
		after = api_time_now();;
		
		encodingTime += (after - before);
#endif
		//cout << "Decoding process begins." << endl;
		// reset the codec for decoding
		codec->reset();
		
		before = api_time_now();
		for (i = 0; i < numBlocks; i ++)
		{
			decoded = codec -> decode(encoded[i], seed[i]);
		}
		after = api_time_now();
		
		decodingTime += (after - before);
		
		success = false;
		if (decoded != 0)
		{
			success = true;
			
			//#if 0
			// Compare the original with decoded results.
			for (i = 0; i < numBlocks; i ++)
			{
				if ( memcmp(original[i], decoded[i], blockSize) != 0 )
				{
					success = false;
					break;
				}
			}
			//#endif
		}
		
		if ( success == false )
			break ;
	}
	
	return success ;
}

bool codingTest(int blockSize, int numBlocks,
				int iterations, NetcodeType mode, int threads_num,
				double & encBW, double & decBW)
{
	Random * r = new Random();

	// to allow all codec modes start from repeatable random gen. state
	int initial_seed = 10000 ;
	if ( initial_seed >= 0 ) 
		r->initialize(initial_seed) ;

	iNetcode * codec = new NetcodeAccel(blockSize, numBlocks, NON_RECODING_MODE, 1.0, initial_seed);

	xint_t ** original = new xint_t *[numBlocks];
	xint_t ** encoded = new xint_t *[numBlocks];

	int * seed = new int[numBlocks];

	for (int i = 0; i < numBlocks; i ++)
	{
		original[i] = (xint_t *) allocAlignedMem(blockSize);
		encoded[i] = (xint_t *) allocAlignedMem(blockSize);
	}
	api_time_t encodingTime, decodingTime ;
	encodingTime = decodingTime = 0 ;
	
	bool success = encode_decode_test(r, codec,
		iterations,
		blockSize, numBlocks,
		seed, original, encoded,
		encodingTime, decodingTime,
		mode
		) ;

	for (i = 0; i < numBlocks; i ++)
	{
		//freeAlignedMem(original[i]);
		//freeAlignedMem(encoded[i]);
	}

	delete original;
	delete encoded;
	delete seed;

	delete codec;
	delete r;

	if ( success )
	{
		encBW = iterations * blockSize * numBlocks * 1.0 / encodingTime;
		decBW = iterations * blockSize * numBlocks * 1.0 / decodingTime;

		cout << "Decoding successful." << endl;

		// Reporting timing statistics

		cout << "Total encoding time of " <<
			blockSize * numBlocks <<
			" bytes: " << encodingTime << endl;
		cout << "Total decoding time of " <<
			blockSize * numBlocks <<
			" bytes: " << decodingTime << endl;
		/*        cout << "Encoding bandwidth: " <<
		blockSize * numBlocks * 1.0 / encodingTime
		<< " MBytes per second." << endl;
		cout << "Decoding bandwidth: " <<
		blockSize * numBlocks * 1.0 / decodingTime
		<< " MBytes per second." << endl;
		*/
		return true;
	}
	else
	{
		encBW = 0;
		decBW = 0;

		cout << "***Decoding unsuccessful.***" << endl;
		return false;
	}
}

namespace Library
{
	xint_t accel_test_coeff[32][32][32];
}

int main(int argc, char * const argv[])
{
	int  iterations = 10;
	double encBW_classic = 0 ;
	double decBW_classic = 0 ;
	double encBW_accel	= 0 ;
	double decBW_accel	= 0 ;
	double encBW_thread = 0 ;
	double decBW_thread = 0 ;
	NetcodeType mode ;

	int i = 1;
	while (i < argc)
	{
		string arg = argv[i];

		if (arg == "-accel")
		{
			g_mode = NETCODE_ACCELERATED;
		}
		else
			if (arg == "-h"  ||  arg == "--h"  ||  arg == "/?"  ||
				arg == "-help"  ||  arg == "--help")
			{
				cout << 
					"Usage is: netcode [-n <# of blocks>] [-s <size of block>] \
					[-accel] [-t <threads num>] [-agg] \
					[-i <# of iterations>] [-pin] [-file <filanme>]" 
					<< endl;
				cout << "Default is without SSE with "
					<< g_blockSize << " blocks of "
					<< g_numBlocks << " bytes." << endl;
				return 0;
			}
			else
				if (arg == "-i")
				{
					i ++;
					assert( i < argc ) ;
					sscanf(argv[i], "%d", &iterations);
				}
				else	
					if (arg == "-n")
					{
						i ++;
						assert( i < argc ) ;
						sscanf(argv[i], "%d", &g_numBlocks);
					}
					else
						if (arg == "-s")
						{
							i ++;
							assert( i < argc ) ;
							sscanf(argv[i], "%d", &g_blockSize);
						}
						else
							if (arg == "-t")
							{
								i ++;
								assert( i < argc ) ;
								sscanf(argv[i], "%d", &g_threads_num);
								g_mode = NETCODE_THREADED;
							}
							else
								if (arg == "-pin")
								{
									g_thread_binding = true;
								}
								else
									if (arg == "-agg")
									{
										g_mode = NETCODE_THREADED_AGGRESSIVE;
									}
									else
										if (arg == "-file")
										{
											g_dump_encoded_output = true;
											i ++;
											assert( i < argc ) ;
											g_output_filename = argv[i];
										}
										/*else
										if (arg == "-test")
										{
										i ++;
										assert( i < argc ) ;
										sscanf(argv[i], "%d", &g_test);
										if ( g_test == 1 )
										g_mode = NETCODE_THREADED_TEST;
										else
										if ( g_test == 2 )
										g_mode = NETCODE_THREADED_TEST2;		
										}*/				

										i ++;
	}


	cout << "Using " ;
	for ( int i = 0 ; i <= (int)g_mode ; i ++ )
		cout << g_mode_desc[i] << " " ;
	cout <<	" codecs." << endl;

	cout << "Testing " << iterations << " runs: "
		<< g_numBlocks << " blocks of size " << g_blockSize << "." << endl;

	mode = NETCODE_CLASSIC ;
	cout << "\nUsing " << g_mode_desc[mode] << " codec." << endl;
	codingTest(g_blockSize / sizeof(xint_t), g_numBlocks,
		iterations, mode, g_threads_num,
		encBW_classic, decBW_classic) ;
	cout << "Average encoding bandwidth: " <<
		encBW_classic  << " MBytes per second." << endl;
	cout << "Average decoding bandwidth: " <<
		decBW_classic  << " MBytes per second." << endl;

	if ( g_mode >= NETCODE_ACCELERATED )		{
		mode = NETCODE_ACCELERATED ;
		cout << "\nUsing " << g_mode_desc[mode] << " codec." << endl;		
		if ( codingTest(g_blockSize, g_numBlocks,
			iterations, mode, g_threads_num,
			encBW_accel, decBW_accel) == true )	{
				cout << "Average encoding bandwidth: " <<
					encBW_accel  << " MBytes per second." << endl;
				cout << "Average decoding bandwidth: " <<
					decBW_accel  << " MBytes per second." << endl;
				cout << "Encode speedup--  " 
					<< "accel/classic: "	<< encBW_accel << "/" << encBW_classic	<< " = " << encBW_accel/encBW_classic 
					<< endl ;
				cout << "Decode speedup--  "
					<< "accel/classic: "	<< decBW_accel << "/" << decBW_classic << " = " << decBW_accel/decBW_classic 
					<< endl ;		
		}	 
	}

	if ( g_mode >= NETCODE_THREADED )		{
		mode = NETCODE_THREADED ;
		cout << "\nUsing " << g_mode_desc[mode] << "(" << g_threads_num << ") codec." << endl;
		if ( codingTest(g_blockSize, g_numBlocks,
			iterations, mode, g_threads_num,
			encBW_thread, decBW_thread) == true )	{
				cout << "Average encoding bandwidth: " <<
					encBW_thread  << " MBytes per second." << endl;
				cout << "Average decoding bandwidth: " <<
					decBW_thread  << " MBytes per second." << endl;	
				cout << "Encode speedup--  " 
					<< "thread/accel: "	<< encBW_thread << "/" << encBW_accel	<< " = " << encBW_thread/encBW_accel 
					<< "\t"
					<< "thread/classic: "  << encBW_thread << "/" << encBW_classic << " = " << encBW_thread/encBW_classic 
					<< endl ;
				cout << "Decode speedup--  "
					<< "thread/accel: "	<< decBW_thread << "/" << decBW_accel	<< " = " << decBW_thread/decBW_accel 
					<< "\t"
					<< "thread/classic: "  << decBW_thread << "/" << decBW_classic << " = " << decBW_thread/decBW_classic 
					<< endl ;		
		}
	}

	if ( g_mode >= NETCODE_THREADED_AGGRESSIVE )		{
		double encBW_agg = 0 ;
		double decBW_agg = 0 ;
		mode = NETCODE_THREADED_AGGRESSIVE ;
		cout << "\nUsing " << g_mode_desc[mode] << "(" << g_threads_num << ") codec." << endl;
		if ( codingTest(g_blockSize, g_numBlocks,
			iterations, mode, g_threads_num,
			encBW_agg, decBW_agg) == true )	{
				cout << "Average encoding bandwidth: " <<
					encBW_agg  << " MBytes per second." << endl;
				cout << "Average decoding bandwidth: " <<
					decBW_agg  << " MBytes per second." << endl;	
				cout << "Encode speedup--  " 
					<< "agg/thread: "		<< encBW_agg << "/" << encBW_thread << " = " << encBW_agg/encBW_thread 
					<< "\t agg/accel: "	<< encBW_agg/encBW_accel
					<< "\t agg/classic: "	<< encBW_agg/encBW_classic
					<< endl ;
				cout << "Decode speedup--  "
					<< "agg/thread: "		<< decBW_agg << "/" << decBW_thread << " = " << decBW_agg/decBW_thread 
					<< "\t agg/accel: "	<< decBW_agg/decBW_accel
					<< "\t agg/classic: "	<< decBW_agg/decBW_classic
					<< endl ;		 
		}

	}

}

#endif
