#include "PPintrin.h"
#include "iostream"

// implementation of absSerial(), but it is vectorized using PP intrinsics
void absVector(float *values, float *output, int N)
{
  __pp_vec_float x;
  __pp_vec_float result;
  __pp_vec_float zero = _pp_vset_float(0.f);
  __pp_mask maskAll, maskIsNegative, maskIsNotNegative;

  //  Note: Take a careful look at this loop indexing.  This example
  //  code is not guaranteed to work when (N % VECTOR_WIDTH) != 0.
  //  Why is that the case?
  for (int i = 0; i < N; i += VECTOR_WIDTH)
  {

    // All ones
    maskAll = _pp_init_ones();

    // All zeros
    maskIsNegative = _pp_init_ones(0);

    // Load vector of values from contiguous memory addresses
    _pp_vload_float(x, values + i, maskAll); // x = values[i];

    // Set mask according to predicate
    _pp_vlt_float(maskIsNegative, x, zero, maskAll); // if (x < 0) {

    // Execute instruction using mask ("if" clause)
    _pp_vsub_float(result, zero, x, maskIsNegative); //   output[i] = -x;

    // Inverse maskIsNegative to generate "else" mask
    maskIsNotNegative = _pp_mask_not(maskIsNegative); // } else {

    // Execute instruction ("else" clause)
    _pp_vload_float(result, values + i, maskIsNotNegative); //   output[i] = x; }

    // Write results back to memory
    _pp_vstore_float(output + i, result, maskAll);
  }
}

void clampedExpVector(float *values, int *exponents, float *output, int N)
{
  //
  // PP STUDENTS TODO: Implement your vectorized version of
  // clampedExpSerial() here.
  //
  // Your solution should work for any value of
  // N and VECTOR_WIDTH, not just when VECTOR_WIDTH divides N
  //
  __pp_vec_int expo, counter, ones;
  __pp_vec_float result, vals, clamp_vals;
  __pp_mask maskAll, maskNotDone, maskClamped;
  ones = _pp_vset_int(1);
  maskAll = _pp_init_ones();
  clamp_vals = _pp_vset_float(9.999999);

  int remainder = N % VECTOR_WIDTH;
  int upper_bound = N - remainder;

  for (int i = 0; i < upper_bound; i += VECTOR_WIDTH)
  {
    _pp_vset_int(counter, 0, maskAll);
    _pp_vset_float(result, 1.0, maskAll);
    _pp_vload_float(vals, values + i, maskAll);
    _pp_vload_int(expo, exponents + i, maskAll);
    _pp_vlt_int(maskNotDone, counter, expo, maskAll);
    int bit_count = _pp_cntbits(maskNotDone);
    while (bit_count)
    {
      _pp_vmult_float(result, result, vals, maskNotDone);
      _pp_vadd_int(counter, counter, ones, maskNotDone);
      _pp_vlt_int(maskNotDone, counter, expo, maskAll);
      bit_count = _pp_cntbits(maskNotDone);
    }

    _pp_vgt_float(maskClamped, result, clamp_vals, maskAll);
    _pp_vset_float(result, 9.999999, maskClamped);
    _pp_vstore_float(output + i, result, maskAll);
  }

  for (int i = upper_bound; i < N; ++i){
    float x = values[i];
    int y = exponents[i];
    if (y == 0)
    {
      output[i] = 1.f;
    }
    else
    {
      float result = x;
      int count = y - 1;
      while (count > 0)
      {
        result *= x;
        count--;
      }
      if (result > 9.999999f)
      {
        result = 9.999999f;
      }
      output[i] = result;
    }
  }

}

// returns the sum of all elements in values
// You can assume N is a multiple of VECTOR_WIDTH
// You can assume VECTOR_WIDTH is a power of 2
float arraySumVector(float *values, int N)
{

  //
  // PP STUDENTS TODO: Implement your vectorized version of arraySumSerial here
  //

  for (int i = 0; i < N; i += VECTOR_WIDTH)
  {
  }

  return 0.0;
}