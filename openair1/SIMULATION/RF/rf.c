/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.0  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.openairinterface.org/?page_id=698
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 *-------------------------------------------------------------------------------
 * Optimization using SIMD instructions
 * Frecuency Domain Analysis
 * Luis Felipe Ariza Vesga, email:lfarizav@unal.edu.co
 * Functions: rf_rx_simple_freq(), rf_rx_simple_freq_SSE_float().
 *-------------------------------------------------------------------------------
 */

//#include "defs.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

//#include "PHY/defs.h"
#include "SIMULATION/TOOLS/defs.h"

/*
extern void randominit(void);
extern double gaussdouble(double,double);
  //free(input_data);

//extern int write_output(const char *,const char *,void *,int,int,char);
//flag change
extern int write_output(const char *,const char *,void *,int,int,char);
*/

//double pn[1024];

//#define DEBUG_RF 1

//free(input_data);

void rf_rx(double **r_re,
           double **r_im,
           double **r_re_i1,
           double **r_im_i1,
           double I0_dB,
           unsigned int nb_rx_antennas,
           unsigned int length,
           double s_time,
           double f_off,
           double drift,
           double noise_figure,
           double rx_gain_dB,
           int IP3_dBm,
           double *initial_phase,
           double pn_cutoff,
           double pn_amp_dBc,
           double IQ_imb_dB,
           double IQ_phase)
{

  double phase       = *initial_phase;
  double phase2      = *initial_phase;
  double phase_inc   = 2*M_PI*f_off*s_time*1e-9;
  double IQ_imb_lin  = pow(10.0,-.05*IQ_imb_dB);
  double rx_gain_lin = pow(10.0,.05*rx_gain_dB);
  double IP3_lin     = pow(10.0,-.1*IP3_dBm);
  double p_noise     = 0.0;
  double tmp_re,tmp_im;
  double N0W         = pow(10.0,.1*(-174.0 - 10*log10(s_time*1e-9)));
  //  printf("s_time=%f, N0W=%g\n",s_time,10*log10(N0W));

  // phase-noise filter coefficients (2nd order digital Butterworth)
  double pn_cutoff_d = tan(M_PI*s_time*1e-9*pn_cutoff);
  double pn_c        = 1+2*cos(M_PI/4)*pn_cutoff_d + (pn_cutoff_d*pn_cutoff_d);
  double pn_a0       = pn_cutoff_d*pn_cutoff_d/pn_c;
  double pn_b1       = 2*((pn_cutoff_d*pn_cutoff_d) - 1)/pn_c;
  double pn_b2       = (4*pn_a0) - pn_b1 - 1;
  double x_n=0.0,x_n1=0.0,x_n2=0.0,y_n1=0.0,y_n2=0.0;

  double pn_amp      = pow(10.0,.1*pn_amp_dBc);
  double I0 = pow(10.0,.05*I0_dB);
  //  double dummy;

  int i,a,have_interference=0;


  if (pn_amp_dBc > -20.0) {
    printf("rf.c: Illegal pn_amp_dBc %f\n",pn_amp_dBc);
    exit(-1);
  }

  if ((pn_cutoff > 1e6) || (pn_cutoff<1e3)) {
    printf("rf.c: Illegal pn_cutoff %f\n",pn_cutoff);
    exit(-1);
  }

  if (fabs(IQ_imb_dB) > 1.0) {
    printf("rf.c: Illegal IQ_imb %f\n",IQ_imb_dB);
    exit(-1);
  }

  if (fabs(IQ_phase) > 0.1) {
    printf("rf.c: Illegal IQ_phase %f\n",IQ_phase);
    exit(-1);
  }

  if (fabs(f_off) > 10000.0) {
    printf("rf.c: Illegal f_off %f\n",f_off);
    exit(-1);
  }

  if (fabs(drift) > 1000.0) {
    printf("rf.c: Illegal drift %f\n",drift);
    exit(-1);
  }

#ifdef DEBUG_RF
  printf("pn_a0 = %f, pn_b1=%f,pn_b2=%f\n",pn_a0,pn_b1,pn_b2);
#endif

  /*
  for (i=0;i<nb_rx_antennas;i++)
    if (noise_figure[i] < 1.0) {
      printf("rf.c: Illegal noise_figure %d %f\n",i,noise_figure[i]);
      exit(-1);
    }
  */

  //Loop over input
#ifdef DEBUG_RF
  printf("N0W = %f dBm\n",10*log10(N0W));
  printf("rx_gain = %f dB(%f)\n",rx_gain_dB,rx_gain_lin);
  printf("IQ_imb = %f dB(%f)\n",IQ_imb_dB,IQ_imb_lin);
#endif
  p_noise=0.0;



  if ((r_re_i1) && (r_im_i1) )
    have_interference=1;

  for (i=0; i<length; i++) {


    for (a=0; a<nb_rx_antennas; a++) {

      if (have_interference==1) {
        r_re[a][i] = r_re[a][i] + (I0 * r_re_i1[a][i]);
        r_im[a][i] = r_im[a][i] + (I0 * r_im_i1[a][i]);
      }

      // Amplify by receiver gain and apply 3rd order non-linearity
      r_re[a][i] = rx_gain_lin*(r_re[a][i] + IP3_lin*(pow(r_re[a][i],3.0) + 3.0*r_re[a][i]*r_im[a][i]*r_im[a][i])) + rx_gain_lin*(sqrt(.5*N0W)*ziggurat(0.0,1.0));
      r_im[a][i] = rx_gain_lin*(r_im[a][i] + IP3_lin*(pow(r_im[a][i],3.0) + 3.0*r_im[a][i]*r_re[a][i]*r_re[a][i])) + rx_gain_lin*(sqrt(.5*N0W)*ziggurat(0.0,1.0));



      // Apply phase offsets
      tmp_re = r_re[a][i]*cos(phase2) - r_im[a][i]*sin(phase2);
      tmp_im = IQ_imb_lin*(r_re[a][i]*sin(phase2+IQ_phase) + r_im[a][i]*cos(phase2+IQ_phase));

      r_re[a][i] = tmp_re;
      r_im[a][i] = tmp_im;

    }


    //    if (nb_rx_antennas == 1) {
    //      dummy = gaussdouble(0.0,1.0);
    //      dummy = gaussdouble(0.0,1.0);
    //    }
    // First apply frequency/phase offsets + phase noise
    //    U[i%pn_len]=uniformrandom()*pn_amp_lin;
    //    p_noise = 0;
    //    for (j=0;j<pn_len;j++)
    //      p_noise += h_pn[j] * U[(i-j)%pn_len];

    // recompute phase offsets for next sample
    phase += phase_inc;
    phase2 = phase + sqrt(pn_amp)*p_noise;

    //    printf("phase = %f, phase2=%f\n",phase,phase2);

    //*initial_phase = phase2;

    //compute next realization of phase-noise process
    x_n2 = x_n1;
    x_n1 = x_n;
    x_n = ziggurat(0.0,1.0);
    y_n1 = p_noise;
    y_n2 = y_n1;
    p_noise = pn_a0*x_n + 2*pn_a0*x_n1 + pn_a0*x_n2 - pn_b1*y_n1 - pn_b2*y_n2;

    //    pn[i] = p_noise;
  }
}
void rf_rx_simple(double *r_re[2],
                  double *r_im[2],
                  unsigned int nb_rx_antennas,
                  unsigned int length,
                  double s_time,
                  double rx_gain_dB)
{
 /* static int first_run=0;
  static double sum;
  static int count;
  if (!first_run)
  {
     first_run=1;
     sum=0;
     count=0;
  } */
  int i,a;
  double rx_gain_lin = pow(10.0,.05*rx_gain_dB);
  //double rx_gain_lin = 1.0;
  double N0W         = pow(10.0,.1*(-174.0 - 10*log10(s_time*1e-9)));
  //double N0W = 0.0;

  //  printf("s_time=%f, N0W=%g\n",s_time,10*log10(N0W));

  //Loop over input
#ifdef DEBUG_RF
  printf("N0W = %f dBm\n",10*log10(N0W));
  printf("rx_gain = %f dB(%f)\n",rx_gain_dB,rx_gain_lin);
#endif

  for (i=0; i<length; i++) {
    for (a=0; a<nb_rx_antennas; a++) {
      // Amplify by receiver gain and apply 3rd order non-linearity
/*count++;
clock_t start=clock();*/
      r_re[a][i] = rx_gain_lin*(r_re[a][i] + sqrt(.5*N0W)*ziggurat(0.0,1.0));
      r_im[a][i] = rx_gain_lin*(r_im[a][i] + sqrt(.5*N0W)*ziggurat(0.0,1.0));
/*clock_t stop=clock();
printf("do_DL_sig time is %f s, AVERAGE time is %f s, count %d, sum %e\n",(float) (stop-start)/CLOCKS_PER_SEC,(float) (sum+stop-start)/(count*CLOCKS_PER_SEC),count,sum+stop-start);
sum=(sum+stop-start);*/
    }
  }

}
#define RF_RX_SSE
#ifdef  RF_RX_SSE
void rf_rx_simple_freq(double *r_re[2],
                  double *r_im[2],
                  unsigned int nb_rx_antennas,
                  unsigned int length,
                  double s_time,
                  double rx_gain_dB,
		  unsigned int symbols_per_tti,
		  unsigned int ofdm_symbol_size,
		  unsigned int n_samples)
{
 /* static int first_run=0;
  static double sum;
  static int count;
  if (!first_run)
  {
     first_run=1;
     sum=0;
     count=0;
  } 
  count++;*/
  __m128d rx128_re,rx128_im,rx128_gain_lin,gauss_0_128_sqrt_NOW,gauss_1_128_sqrt_NOW;//double
  int i,a;
  double rx_gain_lin = pow(10.0,.05*rx_gain_dB);
  //double rx_gain_lin = 1.0;
  double N0W         = pow(10.0,.1*(-174.0 - 10*log10(s_time*1e-9)));
  double sqrt_NOW = sqrt(.5*N0W);
  //double N0W = 0.0;

  //  printf("s_time=%f, N0W=%g\n",s_time,10*log10(N0W));

  //Loop over input
#ifdef DEBUG_RF
  printf("N0W = %f dBm\n",10*log10(N0W));
  printf("rx_gain = %f dB(%f)\n",rx_gain_dB,rx_gain_lin);
#endif
  //rx128_gain_lin=mm_loadu_pd(rx_gain_lin);
/*count++;
clock_t start=clock();*/
	  for (i=0; i<(length>>1); i++) {
	    for (a=0; a<nb_rx_antennas; a++) {
	      if (i%(ofdm_symbol_size>>1)>(n_samples>>1) && i%(ofdm_symbol_size>>1)<(ofdm_symbol_size>>1)-(n_samples>>1))
    	      {
		//printf("i = %d\n",i);
		//_mm_storeu_pd(&r_re[a][2*i],_mm_setzero_pd());
		//_mm_storeu_pd(&r_im[a][2*i],_mm_setzero_pd());
	 	break;
	      }
	      else
	      {
		      rx128_re =  _mm_loadu_pd(&r_re[a][2*i]);//r_re[a][i],r_re[a][i+1]
		      rx128_im =  _mm_loadu_pd(&r_im[a][2*i]);//r_im[a][i],r_im[a][i+1]
		      rx128_gain_lin = _mm_set1_pd(rx_gain_lin);
		      gauss_0_128_sqrt_NOW = _mm_set_pd(ziggurat(0.0,1.0),ziggurat(0.0,1.0));
		      gauss_1_128_sqrt_NOW = _mm_set_pd(ziggurat(0.0,1.0),ziggurat(0.0,1.0));
		      //boxmuller_SSE_float(&gauss_0_128_sqrt_NOW, &gauss_1_128_sqrt_NOW);
		      gauss_0_128_sqrt_NOW = _mm_mul_pd(gauss_0_128_sqrt_NOW,_mm_set1_pd(sqrt_NOW));
		      gauss_1_128_sqrt_NOW = _mm_mul_pd(gauss_1_128_sqrt_NOW,_mm_set1_pd(sqrt_NOW));
		      // Amplify by receiver gain and apply 3rd order non-linearity
		      rx128_re = _mm_add_pd(rx128_re,gauss_0_128_sqrt_NOW);
		      rx128_im = _mm_add_pd(rx128_im,gauss_1_128_sqrt_NOW);
		      rx128_re = _mm_mul_pd(rx128_re,rx128_gain_lin);
		      rx128_im = _mm_mul_pd(rx128_im,rx128_gain_lin);
		      _mm_storeu_pd(&r_re[a][2*i],rx128_re);
		      _mm_storeu_pd(&r_im[a][2*i],rx128_im);
	      }
	    }
	  }
/*clock_t stop=clock();
printf("do_DL_sig time is %f s, AVERAGE time is %f s, count %d, sum %e\n",(float) (stop-start)/CLOCKS_PER_SEC,(float) (sum+stop-start)/(count*CLOCKS_PER_SEC),count,sum+stop-start);
sum=(sum+stop-start);*/
}
#else
void rf_rx_simple_freq(double *r_re[2],
                  double *r_im[2],
                  unsigned int nb_rx_antennas,
                  unsigned int length,
                  double s_time,
                  double rx_gain_dB,
		  unsigned int symbols_per_tti,
		  unsigned int ofdm_symbol_size,
		  unsigned int n_samples)
{
  static int first_run=0;
  static double sum;
  static int count;
  if (!first_run)
  {
     first_run=1;
     sum=0;
     count=0;
  } 
  int i,j,a;
  double rx_gain_lin = pow(10.0,.05*rx_gain_dB);
  //double rx_gain_lin = 1.0;
  double N0W         = pow(10.0,.1*(-174.0 - 10*log10(s_time*1e-9)));
  //double N0W = 0.0;

  //  printf("s_time=%f, N0W=%g\n",s_time,10*log10(N0W));

  //Loop over input
#ifdef DEBUG_RF
  printf("N0W = %f dBm\n",10*log10(N0W));
  printf("rx_gain = %f dB(%f)\n",rx_gain_dB,rx_gain_lin);
#endif
  for (i=0; i<length; i++) {
    for (a=0; a<nb_rx_antennas; a++) {
      // Amplify by receiver gain and apply 3rd order non-linearity
/*count++;
clock_t start=clock();*/
      r_re[a][i] = rx_gain_lin*(r_re[a][i] + sqrt(.5*N0W)*ziggurat(0.0,1.0));
      r_im[a][i] = rx_gain_lin*(r_im[a][i] + sqrt(.5*N0W)*ziggurat(0.0,1.0));
/*clock_t stop=clock();
printf("do_DL_sig time is %f s, AVERAGE time is %f s, count %d, sum %e\n",(float) (stop-start)/CLOCKS_PER_SEC,(float) (sum+stop-start)/(count*CLOCKS_PER_SEC),count,sum+stop-start);
sum=(sum+stop-start);*/
    }
  }

}
#endif
void rf_rx_simple_freq_SSE_float(float *r_re[2],
                  float *r_im[2],
                  unsigned int nb_rx_antennas,
                  unsigned int length,
                  float s_time,
                  float rx_gain_dB,
		  unsigned int symbols_per_tti,
		  unsigned int ofdm_symbol_size,
		  unsigned int n_samples)
{
 /* static int first_run=0;
  static double sum;
  static int count;
  if (!first_run)
  {
     first_run=1;
     sum=0;
     count=0;
  } 
  count++;*/
  __m128 rx128_re,rx128_im,rx128_gain_lin,gauss_0_128_sqrt_NOW,gauss_1_128_sqrt_NOW;//double
  int i,a;
  float rx_gain_lin = pow(10.0,.05*rx_gain_dB);
  //static float out[4] __attribute__((aligned(16)));
  //static float out1[4] __attribute__((aligned(16)));
  //double rx_gain_lin = 1.0;
  float N0W         = pow(10.0,.1*(-174.0 - 10*log10(s_time*1e-9)));
  float sqrt_NOW = rx_gain_lin*sqrt(.5*N0W);
  //double N0W = 0.0;

  //  printf("s_time=%f, N0W=%g\n",s_time,10*log10(N0W));

  //Loop over input
#ifdef DEBUG_RF
  printf("N0W = %f dBm\n",10*log10(N0W));
  printf("rx_gain = %f dB(%f)\n",rx_gain_dB,rx_gain_lin);
#endif
  //rx128_gain_lin=mm_loadu_pd(rx_gain_lin);
/*count++;
clock_t start=clock();*/
  rx128_gain_lin = _mm_set1_ps(rx_gain_lin);

	  for (i=0; i<(length>>2); i++) {
	    for (a=0; a<nb_rx_antennas; a++) {
	      /*if (i%(ofdm_symbol_size>>2)>(n_samples>>2) && i%(ofdm_symbol_size>>2)<(ofdm_symbol_size>>2)-(n_samples>>2))
    	      {
		//printf("i = %d\n",i);
		//_mm_storeu_pd(&r_re[a][2*i],_mm_setzero_pd());
		//_mm_storeu_pd(&r_im[a][2*i],_mm_setzero_pd());
	 	break;
	      }
	      else
	      {*/
		      rx128_re =  _mm_loadu_ps(&r_re[a][4*i]);//r_re[a][i],r_re[a][i+1]
		      rx128_im =  _mm_loadu_ps(&r_im[a][4*i]);//r_im[a][i],r_im[a][i+1]
		      //start_meas(&desc->ziggurat);
		      //gauss_0_128_sqrt_NOW = _mm_set1_ps(1);
		      //gauss_1_128_sqrt_NOW = _mm_set1_ps(1);
		      //gauss_0_128_sqrt_NOW = _mm_set_ps(gaussdouble(0.0,1.0),gaussdouble(0.0,1.0),gaussdouble(0.0,1.0),gaussdouble(0.0,1.0));
		      //gauss_1_128_sqrt_NOW = _mm_set_ps(gaussdouble(0.0,1.0),gaussdouble(0.0,1.0),gaussdouble(0.0,1.0),gaussdouble(0.0,1.0));
		      //gauss_0_128_sqrt_NOW = _mm_set_ps(ziggurat(0.0,1.0),ziggurat(0.0,1.0),ziggurat(0.0,1.0),ziggurat(0.0,1.0));
		      //gauss_1_128_sqrt_NOW = _mm_set_ps(ziggurat(0.0,1.0),ziggurat(0.0,1.0),ziggurat(0.0,1.0),ziggurat(0.0,1.0));
		      //boxmuller_SSE_float(&gauss_0_128_sqrt_NOW, &gauss_1_128_sqrt_NOW);
		      gauss_0_128_sqrt_NOW = ziggurat_SSE_float();
		      gauss_1_128_sqrt_NOW = ziggurat_SSE_float();
		      //stop_meas(&desc->ziggurat);
		      gauss_0_128_sqrt_NOW = _mm_mul_ps(gauss_0_128_sqrt_NOW,_mm_set1_ps(sqrt_NOW));
		      gauss_1_128_sqrt_NOW = _mm_mul_ps(gauss_1_128_sqrt_NOW,_mm_set1_ps(sqrt_NOW));
		      // Amplify by receiver gain and apply 3rd order non-linearity
		      rx128_re = _mm_add_ps(_mm_mul_ps(rx128_re,rx128_gain_lin),gauss_0_128_sqrt_NOW);
		      rx128_im = _mm_add_ps(_mm_mul_ps(rx128_im,rx128_gain_lin),gauss_1_128_sqrt_NOW);
		      _mm_storeu_ps(&r_re[a][4*i],rx128_re);
		      _mm_storeu_ps(&r_im[a][4*i],rx128_im);
	      //}
	    }
	  }
		      /*rx128_re =  _mm_loadu_ps(&r_re[a][4*i+ofdm_symbol_size*j]);//r_re[a][i],r_re[a][i+1]
		      rx128_im =  _mm_loadu_ps(&r_im[a][4*i+ofdm_symbol_size*j]);//r_im[a][i],r_im[a][i+1]
		      rx128_re_1 =  _mm_loadu_ps(&r_re[a][(ofdm_symbol_size-n_samples)+4*i+ofdm_symbol_size*j]);//r_re[a][i],r_re[a][i+1]
		      rx128_im_1 =  _mm_loadu_ps(&r_im[a][(ofdm_symbol_size-n_samples)+4*i+ofdm_symbol_size*j]);//r_im[a][i],r_im[a][i+1]
		      //start_meas(&desc->ziggurat);
		      //gauss_0_128_sqrt_NOW = _mm_set_ps(ziggurat(0.0,1.0),ziggurat(0.0,1.0),ziggurat(0.0,1.0),ziggurat(0.0,1.0));
		      //gauss_1_128_sqrt_NOW = _mm_set_ps(ziggurat(0.0,1.0),ziggurat(0.0,1.0),ziggurat(0.0,1.0),ziggurat(0.0,1.0));
		      boxmuller_SSE_float(&gauss_0_128_sqrt_NOW, &gauss_1_128_sqrt_NOW);
		      boxmuller_SSE_float(&gauss_0_128_sqrt_NOW_1, &gauss_1_128_sqrt_NOW_1);
		      //stop_meas(&desc->ziggurat);
		      gauss_0_128_sqrt_NOW = _mm_mul_ps(gauss_0_128_sqrt_NOW,_mm_set1_ps(sqrt_NOW));
		      gauss_1_128_sqrt_NOW = _mm_mul_ps(gauss_1_128_sqrt_NOW,_mm_set1_ps(sqrt_NOW));
		      gauss_0_128_sqrt_NOW_1 = _mm_mul_ps(gauss_0_128_sqrt_NOW_1,_mm_set1_ps(sqrt_NOW));
		      gauss_1_128_sqrt_NOW_1 = _mm_mul_ps(gauss_1_128_sqrt_NOW_1,_mm_set1_ps(sqrt_NOW));
		      // Amplify by receiver gain and apply 3rd order non-linearity
		      rx128_re = _mm_add_ps(_mm_mul_ps(rx128_re,rx128_gain_lin),gauss_0_128_sqrt_NOW);
		      rx128_im = _mm_add_ps(_mm_mul_ps(rx128_im,rx128_gain_lin),gauss_1_128_sqrt_NOW);
		      rx128_re_1 = _mm_add_ps(_mm_mul_ps(rx128_re_1,rx128_gain_lin),gauss_0_128_sqrt_NOW_1);
		      rx128_im_1 = _mm_add_ps(_mm_mul_ps(rx128_im_1,rx128_gain_lin),gauss_1_128_sqrt_NOW_1);
		      _mm_storeu_ps(&r_re[a][4*i+ofdm_symbol_size*j],rx128_re);
		      _mm_storeu_ps(&r_im[a][4*i+ofdm_symbol_size*j],rx128_im);
		      _mm_storeu_ps(&r_re[a][(ofdm_symbol_size-n_samples)+4*i+ofdm_symbol_size*j],rx128_re_1);
		      _mm_storeu_ps(&r_im[a][(ofdm_symbol_size-n_samples)+4*i+ofdm_symbol_size*j],rx128_im_1);*/
/*clock_t stop=clock();
printf("do_DL_sig time is %f s, AVERAGE time is %f s, count %d, sum %e\n",(float) (stop-start)/CLOCKS_PER_SEC,(float) (sum+stop-start)/(count*CLOCKS_PER_SEC),count,sum+stop-start);
sum=(sum+stop-start);*/
}
#ifdef RF_MAIN
#define INPUT_dBm -70.0

int QPSK[4]= {AMP_OVER_SQRT2|(AMP_OVER_SQRT2<<16),AMP_OVER_SQRT2|((65536-AMP_OVER_SQRT2)<<16),((65536-AMP_OVER_SQRT2)<<16)|AMP_OVER_SQRT2,((65536-AMP_OVER_SQRT2)<<16)|(65536-AMP_OVER_SQRT2)};
int QPSK2[4]= {AMP_OVER_2|(AMP_OVER_2<<16),AMP_OVER_2|((65536-AMP_OVER_2)<<16),((65536-AMP_OVER_2)<<16)|AMP_OVER_2,((65536-AMP_OVER_2)<<16)|(65536-AMP_OVER_2)};


int main(int argc, char* argv[])
{

  // Fill input vector with complex sinusoid

  int nb_antennas = 1;
  int length      = 100000;
  int i,j,n;
  double input_amp = pow(10.0,(.05*INPUT_dBm))/sqrt(2);
  double nf[2] = {3.0,3.0};
  double ip =0.0;
  double path_loss_dB = -90, rx_gain_dB = 125;
  double tx_pwr, rx_pwr;

  uint32_t **input = malloc(nb_antennas*sizeof(uint32_t*));
  uint32_t **output = malloc(nb_antennas*sizeof(uint32_t*));

  double **s_re = malloc(nb_antennas*sizeof (double *));
  double **s_im = malloc(nb_antennas*sizeof (double *));
  double **r_re = malloc(nb_antennas*sizeof (double *));
  double **r_im = malloc(nb_antennas*sizeof (double *));

  channel_desc_t *channel;

  srand(0);
  randominit(0);
  set_taus_seed(0);

  channel = new_channel_desc_scm(nb_antennas,
                                 nb_antennas,
                                 AWGN,
                                 7.68e6,
                                 0,
                                 0,
                                 path_loss_dB);

  for (i=0; i<nb_antennas; i++) {
    s_re[i] = (double *)malloc(length * sizeof (double ));
    s_im[i] = (double *)malloc(length * sizeof (double ));
    r_re[i] = (double *)malloc(length * sizeof (double ));
    r_im[i] = (double *)malloc(length * sizeof (double ));
    input[i] = (uint32_t*)malloc(length * sizeof(uint32_t));
    output[i] = (uint32_t*)malloc(length * sizeof(uint32_t));
  }

  for (i=0; i<nb_antennas; i++) {
    // generate a random QPSK signal
    for (j=0; j<length/2; j++) {
      input[i][j] = QPSK[taus()&3];
    }
  }

  tx_pwr = dac_fixed_gain(s_re,
                          s_im,
                          input,
                          0,
                          nb_antennas,
                          length,
                          0,
                          512,
                          14,
                          15.0);

  multipath_channel(channel,s_re,s_im,r_re,r_im,
                    length,0);

  rf_rx_simple(r_re,
               r_im,
               nb_antennas,
               length,
               1.0/7.68e6 * 1e9,// sampling time (ns)
               rx_gain_dB - 66.227);

  /*
  rf_rx(r_re,
  r_im,
  nb_antennas,
  length,
  1.0/6.5e6 * 1e9,// sampling time (ns)
  1000.0  ,          //freq offset (Hz)
  0.0,            //drift (Hz) NOT YET IMPLEMENTED
  nf,             //noise_figure NOT YET IMPLEMENTED
  -INPUT_dBm,           //rx_gain (dB)
  200,            // IP3_dBm (dBm)
  &ip,            // initial phase
  30.0e3,         // pn_cutoff (kHz)
  -500.0,          // pn_amp (dBc)
  -0.0,              // IQ imbalance (dB),
  0.0);           // IQ phase imbalance (rad)
  */

  adc(r_re,
      r_im,
      0,
      0,
      output,
      nb_antennas,
      length,
      12);

  write_output("s_im.m","s_im",s_im[0],length,1,7);
  write_output("s_re.m","s_re",s_re[0],length,1,7);
  write_output("r_im.m","r_im",r_im[0],length,1,7);
  write_output("r_re.m","r_re",r_re[0],length,1,7);
  write_output("input.m","rfin",input[0],length,1,1);
  write_output("output.m","rfout",output[0],length,1,1);
}
#endif
