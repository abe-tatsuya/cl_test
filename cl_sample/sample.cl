__kernel void sample(__global int *data, __global int *out) {
  int i = get_global_id(0);
  out[i] = (22 * data[i] * (data[i] + 1)) % 6700417;
  out[i] = (22 * out[i] * (out[i] + 1)) % 6700417;
  out[i] = (22 * out[i] * (out[i] + 1)) % 6700417;
  out[i] = (22 * out[i] * (out[i] + 1)) % 6700417;
  out[i] = (22 * out[i] * (out[i] + 1)) % 6700417;
  out[i] = (22 * out[i] * (out[i] + 1)) % 6700417;
  out[i] = (22 * out[i] * (out[i] + 1)) % 6700417;
  out[i] = (22 * out[i] * (out[i] + 1)) % 6700417;
  out[i] = (22 * out[i] * (out[i] + 1)) % 6700417;
  out[i] = (22 * out[i] * (out[i] + 1)) % 6700417;
};


