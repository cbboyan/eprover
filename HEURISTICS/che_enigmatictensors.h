/*-----------------------------------------------------------------------

File  : che_enigmatictensors.h

Author: AI4REASON

Contents
 
  Auto generated. Your comment goes here ;-).

  Copyright 2016 by the author.
  This code is released under the GNU General Public Licence.
  See the file COPYING in the main directory for details.
  Run "eprover -h" for contact information.

Changes

<1> Tue Mar  8 22:40:31 CET 2016
    New

-----------------------------------------------------------------------*/

#ifndef CHE_ENIGMATICTENSORS

#define CHE_ENIGMATICTENSORS

#include <sys/socket.h>
#include <arpa/inet.h>
#include <cte_termbanks.h>
#include <ccl_clauses.h>

/*---------------------------------------------------------------------*/
/*                    Data type declarations                           */
/*---------------------------------------------------------------------*/

#define ETF_TENSOR_SIZE (10*1024*1024)
#define SOCKET_BUF_SIZE 1024
#define INDEXSETSIZE ( 128 * 64 )  // 2 ** 18 booleans because we use uint32_t


//#define DEBUG_ETF
#define DEBUG_ETF_SERVER

typedef struct inthash
{
   long   entries;
   NumTree_p store[INDEXSETSIZE];
}IntHashCell, *IntHash_p;


typedef struct enigmaticsocketcell
{
   int fd;
   int cur;
   int bytes_cnt;
   struct sockaddr_in addr;
   char buf[SOCKET_BUF_SIZE+128];
   int evals_size;
   char* evals;
} EnigmaticSocketCell, *EnigmaticSocket_p;

typedef struct enigmatictensorsparamcell
{
   TB_p         tmp_bank;
   long         tmp_bank_vars;

   // clause edges
   NumTree_p terms;
   NumTree_p syms;
   long fresh_t;
   long fresh_s;
   long fresh_c;
   long maxvar;
   PStack_p tedges;
   PStack_p cedges;
   IntHash_p tedges_set;
   IntHash_p cedges_set;
//   uint32_t tedges_set[INDEXSETSIZE];
//   uint32_t cedges_set[INDEXSETSIZE];

   // context
   long context_cnt;

   // conjecture edges
   bool conj_mode;
   NumTree_p conj_terms;
   NumTree_p conj_syms;
   long conj_fresh_t;
   long conj_fresh_s;
   long conj_fresh_c;
   long conj_maxvar;
   PStack_p conj_tedges;
   PStack_p conj_cedges;
   IntHash_p conj_tedges_set;
   IntHash_p conj_cedges_set;
//   uint32_t conj_tedges_set[INDEXSETSIZE];
//   uint32_t conj_cedges_set[INDEXSETSIZE];


   
   int n_is;
   int n_i1;
   int n_i2;
   int n_i3;
   
   // raw tensors data:
   int32_t ini_nodes[ETF_TENSOR_SIZE];
   int32_t ini_symbols[ETF_TENSOR_SIZE];
   int32_t ini_clauses[ETF_TENSOR_SIZE];
   int32_t clause_inputs_data[ETF_TENSOR_SIZE];
   int32_t clause_inputs_lens[ETF_TENSOR_SIZE];
   int32_t node_c_inputs_data[ETF_TENSOR_SIZE];
   int32_t node_c_inputs_lens[ETF_TENSOR_SIZE];
   int32_t symbol_inputs_nodes[3*ETF_TENSOR_SIZE]; 
   int32_t symbol_inputs_lens[ETF_TENSOR_SIZE]; 
   int32_t node_inputs_1_symbols[ETF_TENSOR_SIZE];
   int32_t node_inputs_1_nodes[2*ETF_TENSOR_SIZE];
   int32_t node_inputs_1_lens[ETF_TENSOR_SIZE];
   int32_t node_inputs_2_symbols[ETF_TENSOR_SIZE];
   int32_t node_inputs_2_nodes[2*ETF_TENSOR_SIZE];
   int32_t node_inputs_2_lens[ETF_TENSOR_SIZE];
   int32_t node_inputs_3_symbols[ETF_TENSOR_SIZE];
   int32_t node_inputs_3_nodes[2*ETF_TENSOR_SIZE];
   int32_t node_inputs_3_lens[ETF_TENSOR_SIZE];
   float symbol_inputs_sgn[ETF_TENSOR_SIZE]; 
   float node_inputs_1_sgn[ETF_TENSOR_SIZE];
   float node_inputs_2_sgn[ETF_TENSOR_SIZE];
   float node_inputs_3_sgn[ETF_TENSOR_SIZE];
   int32_t prob_segments_lens[ETF_TENSOR_SIZE];
   int32_t prob_segments_data[ETF_TENSOR_SIZE];
   int32_t labels[ETF_TENSOR_SIZE];

}EnigmaticTensorsCell, *EnigmaticTensors_p;

/*---------------------------------------------------------------------*/
/*                Exported Functions and Variables                     */
/*---------------------------------------------------------------------*/

#define EnigmaticTensorsCellAlloc() (EnigmaticTensorsCell*) \
        SizeMalloc(sizeof(EnigmaticTensorsCell))
#define EnigmaticTensorsCellFree(junk) \
        SizeFree(junk, sizeof(EnigmaticTensorsCell))

#define IntHashCellAlloc() (IntHashCell*) \
        SizeMalloc(sizeof(IntHashCell))
#define IntHashCellFree(junk) \
        SizeFree(junk, sizeof(IntHashCell))

#define EnigmaticSocketCellAlloc() (EnigmaticSocketCell*) \
        SizeMalloc(sizeof(EnigmaticSocketCell))
#define EnigmaticSocketCellFree(junk) \
        SizeFree(junk, sizeof(EnigmaticSocketCell))

EnigmaticTensors_p EnigmaticTensorsAlloc(void);
void              EnigmaticTensorsFree(EnigmaticTensors_p junk);

EnigmaticSocket_p EnigmaticSocketAlloc(void);
void              EnigmaticSocketFree(EnigmaticSocket_p junk);

void EnigmaticTensorsUpdateClause(Clause_p clause, EnigmaticTensors_p data);

void EnigmaticTensorsReset(EnigmaticTensors_p data);

void EnigmaticTensorsFill(EnigmaticTensors_p data);

void EnigmaticTensorsDump(FILE* out, EnigmaticTensors_p tensors);

void EnigmaticSocketSend(EnigmaticSocket_p sock, EnigmaticTensors_p tensors);

float* EnigmaticSocketRecv(EnigmaticSocket_p sock, int expected);

IntHash_p IntHashAlloc(void);

void IntHashInit(IntHash_p store);

void IntHashFree(IntHash_p junk);

void IntHashExit(IntHash_p store);


#endif

/*---------------------------------------------------------------------*/
/*                        End of File                                  */
/*---------------------------------------------------------------------*/

