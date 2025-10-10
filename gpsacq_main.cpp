// GPS Acq. for HLS 2025-02-26 Naoki F., AIT (based on Yokoyama's code, 2025-01-08)

#include <stdio.h>
#include <stdlib.h>
#include "define.hpp"

#define NOM_IF_FREQ (4.092e6)           // IF Freq. [Hz]
#define SAMP_FREQ   (16.368e6)          // Sampling Freq. [Hz]
#define SAMP_IN_MS  (16368)             // # of Samples in 1 code (1 ms)
#define NOISE_FLOOR (4.5e6)
#define NCO_RES     (0.030487775802612) // NCO Resolution = 16.368 MHz / 2^29

void search(hls::stream<sig_t> &sig, int base_prn, int base_freq, int base_code,
            int *corr, int *par_prn, int *par_freq, int *par_code);

// corr. power (I^2 + Q^2)
int corr[32][25][2046];

void make_sig_stream(unsigned char *sig, hls::stream<sig_t> &sig_str, unsigned int size)
{
  sig_t s;
  for (unsigned int i = 0; i < size; i++) {
    s.last = (i == size - 1);
    s.data = sig[i];
    sig_str << s;
  }
}

int main()
{
  // File name of IF Signal
#ifndef WSL_FS
  const char *filename = "C:/doc/x/research/GPS/gpsacq/data/gps.bin";
#else
  const char *filename = "/mnt/c/doc/x/research/GPS/gpsacq/data/gps.bin";
#endif
  FILE *inp, *outp;

  // Satelite ID to acquire (either 9, 15, 18, 22, 25, 27, or 30)
  int prn = 9;

  // IF Signal Buffer (2-bit Sign/Magnitude)
  unsigned char sig[SAMP_IN_MS * 3];
  hls::stream<sig_t> sig_str;

  // parameter constants of hardware
  int par_prn, par_freq, par_code;

  // open the output file
  if (NULL == (outp = fopen("gpsacq.out", "wt"))) {
    fprintf(stderr, "Cannot open output file.\n");
    exit(1);
  }

  // open the input file
  if (NULL == (inp = fopen(filename, "rb"))) {
    fprintf(stderr, "Cannot open IF data file.\n");
    exit(1);
  }

  // read 3 ms of IF signal from the input file
  if (fread(sig, sizeof(unsigned char), SAMP_IN_MS * 3, inp) != SAMP_IN_MS * 3) {
    printf("Detect EOF. Abort.\n");
    exit(1);
  }

  // get the parameters
  search(sig_str, 0, 0, 0, NULL, &par_prn, &par_freq, &par_code);
  printf("## degree of parallelism - PRN %d, FREQ %d, CODE %d\n",
    par_prn, par_freq, par_code);

  // main loop of the serial search
  // for (freq_idx = -12; freq_idx <= 12; freq_idx++)
  for (int prn_idx = 9; prn_idx <= 11; prn_idx += par_prn) {
    for (int freq_idx = -2; freq_idx <= 2; freq_idx += par_freq) {
      for (int code_idx = 0; code_idx < 60; code_idx += par_code) {
        make_sig_stream(sig + code_idx * 8, sig_str, 16362 + par_code * 8);
        search(sig_str, prn_idx, freq_idx + 12, code_idx, (int *) corr, NULL, NULL, NULL);
      }
    }
  }
  for (int prn_idx = 9; prn_idx <= 11; prn_idx++) {
    for (int freq_idx = 11; freq_idx <= 13; freq_idx++) {
      for (int a = 0; a < 60; a++) { // print out the results
        double dopp = 500.0 * (freq_idx - 12);
        printf("%2d %4d %8.1f %7.2f\n", prn_idx, a, dopp, corr[prn_idx - 1][freq_idx][a] / NOISE_FLOOR);
        fprintf(outp, "%7.2f,", corr[prn_idx - 1][freq_idx][a] / NOISE_FLOOR);
      }
    }
  }
  fclose(inp);
  fclose(outp);
  return 0;
}