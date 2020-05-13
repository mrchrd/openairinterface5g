#ifndef __NR_LDPC_DECODER_LYC__H__
#define __NR_LDPC_DECODER_LYC__H__

#include "nrLDPC_types.h"
#include "nrLDPC_init_mem.h"

/*! \file PHY/CODING/nrLDPC_decoder_LYC/nrLDPC_decoder_LYC.h
 * \brief LDPC implementation on Quadro p5000 by NCTU OpinCommect
 * \author NCTU OpinConnect Terng-Yin Hsu/Yu-Chi Liang/Kai-Tai Yang/Yan-Bo Lin 
 * \email tyhsu@cs.nctu.edu.tw
 * \date 13-05-2020
 * \version 
 * \note
 * \warning
 */

/**
   \brief LDPC decoder
   \param p_decParams LDPC decoder parameters
   \param p_llr Input LLRs
   \param p_llrOut Output vector
   \param p_profiler LDPC profiler statistics
*/

int32_t nrLDPC_decoder_LYC(t_nrLDPC_dec_params* p_decParams, int8_t* p_llr, int8_t* p_out, int block_length, time_stats_t *time_decoder);

#endif