#include <memory.h>
#include <malloc.h>
#include "miner.h"
#include "algo-gate-api.h"
#include "algo/blake/sph_blake.h"
#include "lyra2.h"

typedef struct {
	sph_blake256_context     blake;
} zcoin_ctx_holder;

zcoin_ctx_holder zcoin_ctx;

void init_zcoin_ctx()
{
	sph_blake256_init(&zcoin_ctx.blake);
}

void zcoin_hash(uint64_t* wholeMatrix, void *state, const void *input)
{
	zcoin_ctx_holder ctx;
	memcpy(&ctx, &zcoin_ctx, sizeof(zcoin_ctx));
	uint32_t hashA[8], hashB[8];

	sph_blake256(&ctx.blake, input, 80);
	sph_blake256_close(&ctx.blake, hashA);

	//LYRA2Z(uint64_t* matrix, void *K, uint64_t kLen, const void *pwd, uint64_t pwdlen, const void *salt, uint64_t saltlen, uint64_t timeCost, uint64_t nRows, uint64_t nCols )
	LYRA2Z(wholeMatrix, hashB, 32, hashA, 32, hashA, 32, 8, 8, 8);//new zcoin from block 20500

	memcpy(state, hashB, 32);
}


int scanhash_zcoin( int thr_id, struct work *work, uint32_t max_nonce, uint64_t *hashes_done )
{
	int64_t ROW_LEN_INT64 = BLOCK_LEN_INT64 * 8;
	int64_t ROW_LEN_BYTES = ROW_LEN_INT64 * 8;

	size_t size = (int64_t)((int64_t)8 * (int64_t)ROW_LEN_BYTES);
	uint64_t *wholeMatrix = (uint64_t*)_mm_malloc(size, 64);

	uint32_t _ALIGN(128) hash[8];
	uint32_t _ALIGN(128) endiandata[20];
	uint32_t *pdata = work->data;
	uint32_t *ptarget = work->target;
	const uint32_t Htarg = ptarget[7];
	const uint32_t first_nonce = pdata[19];
	uint32_t nonce = first_nonce;
	if (opt_benchmark)
		ptarget[7] = 0x0000ff;

	for (int i=0; i < 19; i++) {
		be32enc(&endiandata[i], pdata[i]);
	}

	do {
		be32enc(&endiandata[19], nonce);
		zcoin_hash(wholeMatrix, hash, endiandata );

		if (hash[7] <= Htarg && fulltest(hash, ptarget)) {
			work_set_target_ratio(work, hash);
			pdata[19] = nonce;
			*hashes_done = pdata[19] - first_nonce;
			_mm_free(wholeMatrix);
			return 1;
		}
		nonce++;

	} while (nonce < max_nonce && !work_restart[thr_id].restart);

	pdata[19] = nonce;
	*hashes_done = pdata[19] - first_nonce + 1;
	_mm_free(wholeMatrix);
	return 0;
}

//int64_t get_max64_0xffffLL() { return 0xffffLL; };

void zcoin_set_target( struct work* work, double job_diff )
{
 work_set_target( work, job_diff / (256.0 * opt_diff_factor) );
}

bool zcoin_get_work_height( struct work* work, struct stratum_ctx* sctx )
{
   work->height = sctx->bloc_height;
   return false;
}

bool register_zcoin_algo( algo_gate_t* gate )
{
  init_zcoin_ctx();
  gate->optimizations = SSE2_OPT | AES_OPT | AVX_OPT | AVX2_OPT;
  gate->scanhash   = (void*)&scanhash_zcoin;
  gate->hash       = (void*)&zcoin_hash;
  gate->hash_alt   = (void*)&zcoin_hash;
  gate->get_max64  = (void*)&get_max64_0xffffLL;
  gate->set_target = (void*)&zcoin_set_target;
  gate->prevent_dupes = (void*)&zcoin_get_work_height;
  return true;
};

