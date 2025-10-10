// GPS Acq. for HLS 2025-06-12 Naoki F., AIT (based on Yokoyama's code, 2025-01-08)

#include <stdio.h>
#include "define.hpp"
#include "codelut.cpp"
#include "multlut.cpp"
#include "ncolut.cpp"
#include "params.hpp"

// function to be high-level synthesized
void search(hls::stream<sig_t> &sig, int base_prn, int base_freq, int base_code,
            int *corr, int *par_prn, int *par_freq, int *par_code)
{
#pragma HLS INTERFACE s_axilite port=return bundle=ctrl 
#pragma HLS INTERFACE axis port=sig depth=65536
#pragma HLS INTERFACE s_axilite port=base_prn bundle=ctrl
#pragma HLS INTERFACE s_axilite port=base_freq bundle=ctrl
#pragma HLS INTERFACE s_axilite port=base_code bundle=ctrl
#pragma HLS INTERFACE s_axilite port=corr bundle=ctrl
#pragma HLS INTERFACE m_axi port=corr offset=slave bundle=gmem depth=262144
#pragma HLS INTERFACE s_axilite port=par_prn bundle=ctrl
#pragma HLS INTERFACE ap_none port=par_prn
#pragma HLS INTERFACE s_axilite port=par_freq bundle=ctrl
#pragma HLS INTERFACE ap_none port=par_freq
#pragma HLS INTERFACE s_axilite port=par_code bundle=ctrl
#pragma HLS INTERFACE ap_none port=par_code

  // let parameter constants readable from PS and exit (if base_prn == 0)
  if (base_prn == 0) {
    *par_prn  = PAR_PRN;
    *par_freq = PAR_FREQ;
    *par_code = PAR_CODE;
    return;
  }

  // carrier replica
  ap_uint<29> carr_nco_reg[PAR_FREQ];
  ap_uint<29> carr_nco_incr[PAR_FREQ];
  ap_uint<3>  phase[PAR_FREQ];
  #pragma HLS ARRAY_PARTITION variable=carr_nco_reg complete
  #pragma HLS ARRAY_PARTITION variable=carr_nco_incr complete
  #pragma HLS ARRAY_PARTITION variable=phase complete

  for (int j = 0; j < PAR_FREQ; j++) {
    carr_nco_reg[j]  = 0;
    carr_nco_incr[j] = ncolut[base_freq + j];
  }

  // code replica
  ap_uint<PAR_PRN> prompt_pos = -1;  // used if frequency offset is positive
  ap_uint<PAR_PRN> prompt_neg = -1;  // used if frequency offset is negative
  ap_uint<PAR_PRN> prompt[PAR_FREQ]; // NOTE: assumed that codelut[0] is 0b1111...
  ap_uint<15> prompt_phase = 0;      // 0 - 16369 (1023 x 16 + 1)

  // correlation
  ap_int<18> IP[PAR_PRN][PAR_FREQ][PAR_CODE];
  ap_int<18> QP[PAR_PRN][PAR_FREQ][PAR_CODE];
  #pragma HLS ARRAY_PARTITION variable=IP dim=0 complete
  #pragma HLS ARRAY_PARTITION variable=QP dim=0 complete

  // last values of sig
  sig_t      current_sig;
  ap_uint<2> last_sig[(PAR_CODE - 1) * 8 + 1];
  #pragma HLS ARRAY_PARTITION variable=last_sig complete

  // Initialize output array
  for (int k = 0; k < PAR_PRN; k++) {
    for (int j = 0; j < PAR_FREQ; j++) {
      for (int i = 0; i < PAR_CODE; i++) {
        IP[k][j][i] = 0;
        QP[k][j][i] = 0;
      }
    }
  }
  
  // Insert (PAR - 1) * 8 samples to the buffer
  for (int i = 0; i < (PAR_CODE - 1) * 8; i++) {
    for (int j = (PAR_CODE - 1) * 8; j >= 1; j--) {
      #pragma HLS unroll
      last_sig[j] = last_sig[j - 1];
    }
    sig >> current_sig;
    last_sig[0] = current_sig.data;
  }

  // main loop
  for (prompt_phase = 0; prompt_phase < 16370; prompt_phase++) {
    // carrier replica
    for (int j = 0; j < PAR_FREQ; j++) {
      phase[j] = carr_nco_reg[j].range(28, 26); // phase = 3 MSBs of NCO register
    }

    // code replica
    prompt_neg = prompt_pos;
    if (prompt_phase.range(3, 0) == 0)
      prompt_pos = codelut[prompt_phase.range(14, 4)].range(base_prn + PAR_PRN - 2, base_prn - 1);
    for (int j = 0; j < PAR_FREQ; j++) {
      prompt[j] = (base_freq + j >= 12) ? prompt_pos : prompt_neg;
    }

    // insert sig to last_sig 
    for (int j = (PAR_CODE - 1) * 8; j >= 1; j--) {
      #pragma HLS unroll
      last_sig[j] = last_sig[j - 1];
    }
    sig >> current_sig;
    last_sig[0] = current_sig.data.range(1, 0);

    // accumulation of correlation
    for (int k = 0; k < PAR_PRN; k++) {
      #pragma HLS unroll
      for (int j = 0; j < PAR_FREQ; j++) {
        if (base_freq + j < 12 || prompt_phase != 16369) {
          for (int i = 0; i < PAR_CODE; i++) {
            ap_uint<6> idx = (phase[j], last_sig[(PAR_CODE - 1 - i) * 8], prompt[j][k]);
            IP[k][j][i] += ilut[idx];
            QP[k][j][i] += qlut[idx];
          }
        }
      }
    }

    // update NCO registers
    for (int j = 0; j < PAR_FREQ; j++) {
      carr_nco_reg[j] += carr_nco_incr[j];
    }
  }
  
  // output the results
  for (int k = 0; k < PAR_PRN; k++) {
    for (int j = 0; j < PAR_FREQ; j++) {
      for (int i = 0; i < PAR_CODE; i++) {
        #pragma HLS pipeline II=1
        ap_int<18> ipi = IP[k][j][i];
        ap_int<18> qpi = QP[k][j][i];
        ipi = (ipi >= 32768 || ipi <= -32768) ? (ap_int<18>) 32768 : ipi;
        qpi = (qpi >= 32768 || qpi <= -32768) ? (ap_int<18>) 32768 : qpi;
        corr[(base_prn + k - 1) * (25 * 2046) + (base_freq + j) * 2046 + base_code + i] = ipi * ipi + qpi * qpi;
      }
    }
  }

  // discard unused signals
  // while (! current_sig.last)
  //   sig >> current_sig;
}