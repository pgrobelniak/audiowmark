#ifndef AUDIOWMARK_WM_COMMON_HH
#define AUDIOWMARK_WM_COMMON_HH

#include <array>
#include <complex>

#include "random.hh"
#include "rawinputstream.hh"

#include <assert.h>

enum class Format { AUTO = 1, RAW = 2 };

class Params
{
public:
  static constexpr size_t frame_size      = 1024;
  static           int    frames_per_bit;
  static constexpr size_t bands_per_frame = 30;
  static constexpr int max_band          = 100;
  static constexpr int min_band          = 20;

  static           double water_delta;
  static           bool mix;
  static           bool hard;                      // hard decode bits? (soft decoding is better)
  static           bool snr;                       // compute/show snr while adding watermark
  static           int  have_key;

  static constexpr size_t payload_size     = 128;  // number of payload bits for the watermark

  static constexpr int sync_bits           = 6;
  static constexpr int sync_frames_per_bit = 85;
  static constexpr int sync_search_step    = 256;
  static constexpr int sync_search_fine    = 8;
  static constexpr double sync_threshold2  = 0.7; // minimum refined quality

  static constexpr size_t frames_pad_start = 250; // padding at start, in case track starts with silence
  static constexpr int mark_sample_rate = 44100; // watermark generation and detection sample rate

  static           int test_cut; // for sync test
  static           bool test_no_sync;
  static           int test_truncate;

  static           Format input_format;
  static           Format output_format;

  static           RawFormat raw_input_format;
  static           RawFormat raw_output_format;
};

typedef std::array<int, 30> UpDownArray;
class UpDownGen
{
  Random::Stream    random_stream;
  Random            random;
  std::vector<int>  bands_reorder;

public:
  UpDownGen (Random::Stream random_stream) :
    random_stream (random_stream),
    random (0, random_stream),
    bands_reorder (Params::max_band - Params::min_band + 1)
  {
    UpDownArray x;
    assert (x.size() == Params::bands_per_frame);
  }
  void
  get (int f, UpDownArray& up, UpDownArray& down)
  {
    for (size_t i = 0; i < bands_reorder.size(); i++)
      bands_reorder[i] = Params::min_band + i;

    random.seed (f, random_stream); // use per frame random seed
    random.shuffle (bands_reorder);

    assert (2 * Params::bands_per_frame < bands_reorder.size());
    for (size_t i = 0; i < Params::bands_per_frame; i++)
      {
        up[i]   = bands_reorder[i];
        down[i] = bands_reorder[Params::bands_per_frame + i];
      }
  }
};

class FFTAnalyzer
{
  int           m_n_channels = 0;
  std::vector<float> m_window;
  float        *m_frame = nullptr;
  float        *m_frame_fft = nullptr;
public:
  FFTAnalyzer (int n_channels);
  ~FFTAnalyzer();

  std::vector<std::vector<std::complex<float>>> run_fft (const std::vector<float>& samples, size_t start_index);
  std::vector<std::vector<std::complex<float>>> fft_range (const std::vector<float>& samples, size_t start_index, size_t frame_count);
};

struct MixEntry
{
  int  frame;
  int  up;
  int  down;
};

std::vector<MixEntry> gen_mix_entries();

double db_from_factor (double factor, double min_dB);

size_t mark_data_frame_count();
size_t mark_sync_frame_count();

int sync_frame_pos (int f);
int data_frame_pos (int f);

template<class T> std::vector<T>
randomize_bit_order (const std::vector<T>& bit_vec, bool encode)
{
  std::vector<unsigned int> order;

  for (size_t i = 0; i < bit_vec.size(); i++)
    order.push_back (i);

  Random random (/* seed */ 0, Random::Stream::bit_order);
  random.shuffle (order);

  std::vector<T> out_bits (bit_vec.size());
  for (size_t i = 0; i < bit_vec.size(); i++)
    {
      if (encode)
        out_bits[i] = bit_vec[order[i]];
      else
        out_bits[order[i]] = bit_vec[i];
    }
  return out_bits;
}

int add_watermark (const std::string& infile, const std::string& outfile, const std::string& bits);
int get_watermark (const std::string& infile, const std::string& orig_pattern);

#endif /* AUDIOWMARK_WM_COMMON_HH */
