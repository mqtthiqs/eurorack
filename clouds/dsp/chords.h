#include "stmlib/dsp/units.h"
#include "clouds/resources.h"
#include "clouds/dsp/frame.h"

#ifndef CLOUDS_DSP_CHORDS_H_
#define CLOUDS_DSP_CHORDS_H_

using namespace stmlib;

namespace clouds {
  
  enum ModulationType {
    FM,
    AM,
  };

  const float a3 = 440.0f / 32000;

  const int kNumVoices = 6;

  const uint8_t permutation[kNumVoices] = { 0, 3, 1, 4, 2, 5 };

  const float chords_table[12][6/*kNumVoices*/] = {
    { 0.0f, 0.0f, 5.0f, 5.0f, 12.0f, 12.0f},
    { 0.0f, 0.0f, 3.0f, 3.0f, 10.0f, 12.0f},
    { 0.0f, 3.0f, 5.0f, 5.0f, 12.0f, 12.0f},
    { 0.0f, 2.0f, 3.0f, 7.0f, 10.0f, 12.0f},
    { 0.0f, 1.0f, 3.0f, 5.0f, 11.0f, 12.0f},
    { 0.0f, 0.0f, 4.0f, 7.0f, 11.0f, 12.0f},
    { 0.0f, 0.0f, 3.0f, 6.0f, 9.0f, 12.0f},
    { 0.0f, 1.0f, 3.0f, 5.0f, 8.0f, 12.0f},
    { 0.0f, 4.0f, 5.0f, 8.0f, 11.0f, 12.0f},
    { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f},
    { 0.0f, 2.0f, 4.0f, 6.0f, 8.0f, 10.0f},
  };

  const int kNumStructures = 8;

  const float modulation_table[6/* kNumVoices */][kNumStructures] = {
    {0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f},
    {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f},
    {0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f},
    {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f},
    {0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f},
    {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
  };

  const float modulation_table_am[15/* kNumVoices */][kNumStructures] = {
    {0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f}, /* 0->1 [0] */
    {0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f}, /* 0->2 [1] */
    {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f}, /* 0->3 [2] */
    {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f}, /* 0->4 [3] */
    {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f}, /* 0->5 [4] */
    {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}, /* 1->2 [5] */
    {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}, /* 1->3 [6] */
    {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}, /* 1->4 [7] */
    {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}, /* 1->5 [8] */
    {0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f}, /* 2->3 [9] */
    {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}, /* 2->4 [10] */
    {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}, /* 2->5 [11] */
    {0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f}, /* 3->4 [12] */
    {0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f}, /* 3->5 [13] */
    {0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f}, /* 4->5 [14] */
  };

  const float indexes[kNumStructures] = {0, 5, 9, 12, 14, 15};

  class Chords {
  public:
    Chords() { }
    ~Chords() { }


    void Init() {
      for (int i=0; i<kNumVoices; i++) {
        phase_[i] = 0.0f;
        modulation_sample_[i][0] = 1.0f;
        modulation_sample_[i][1] = 1.0f;
      }
      bitcrush_ = decimate_ = 65535.0f;
      softclip_ = 0.0001f;
    }


    template<ModulationType modulation_type>
      void Process(FloatFrame* in_out, size_t size) {

      if (modulation_type == FM) {
        for (int i=0; i<kNumVoices; i++) {
          modulation_matrix_[i] = InterpolateSine(modulation_table[i],
                                                  structure_,
                                                  kNumStructures-1);
        }
      } else {                  /* AM */
        for (int u=0; u<kNumVoices*(kNumVoices-1)/2; u++) {
          modulation_matrix_am_[u] = InterpolateSine(modulation_table_am[u],
                                                     structure_,
                                                     kNumStructures-1);
          /* printf("%d -> %f\n", u, modulation_matrix_am_[u]); */
        }
      }

      while (size--) {

        float in_l = in_out->l;
        float in_r = in_out->r;

        in_out->l = in_out->r = 0.0f;

        float total_gain = 0.0f;

        for (int i=0, u=0; i<kNumVoices; i++) {

          /* decimate */
          float phase_l = phase_[i];
          float phase_r = phase_l;

          if (modulation_type == FM) {
            phase_l += in_l
              + self_feedback_sample_[i][1] * self_feedback_
              + self_feedback_
              + modulation_sample_[i][0]
              + 10.0f;
            phase_r += in_r
              + self_feedback_sample_[i][3] * self_feedback_
              + self_feedback_
              + modulation_sample_[i][1]
              + 10.0f;          /* so that it's always positive */
          } else {              /* AM */
            phase_l += in_l
              + self_feedback_sample_[i][1] * self_feedback_ /* TODO
                                                              * factorize
                                                              * with above */
              + self_feedback_
              + 10.0f;
            phase_r += in_r
              + self_feedback_sample_[i][3] * self_feedback_
              + self_feedback_
              + 10.0f;
          }

          phase_l = phase_l - (int32_t)phase_l;
          phase_r = phase_r - (int32_t)phase_r;

          phase_l = (int32_t)(phase_l * decimate_) / decimate_;
          phase_r = (int32_t)(phase_r * decimate_) / decimate_;

          float sin = Interpolate(lut_sin, phase_l, 1024.0f);
          float cos = Interpolate(lut_sin + 256, phase_r, 1024.0f);

          /* bitcrush */
          sin = (int32_t)(sin * bitcrush_) / bitcrush_;
          cos = (int32_t)(cos * bitcrush_) / bitcrush_;

          /* softclip */
          sin = SoftLimit(sin * softclip_) / SoftLimit(softclip_);
          cos = SoftLimit(cos * softclip_) / SoftLimit(softclip_);

          if (modulation_type == AM) {
            sin *= modulation_sample_[i][0];
            cos *= modulation_sample_[i][1];
          }

          modulation_sample_[i][0] = 1.0f;
          modulation_sample_[i][1] = 1.0f;

          self_feedback_sample_[i][0] = sin;
          ONE_POLE(self_feedback_sample_[i][1], self_feedback_sample_[i][0], 0.1f);
          self_feedback_sample_[i][2] = cos;
          ONE_POLE(self_feedback_sample_[i][3], self_feedback_sample_[i][2], 0.1f);

          if (i != kNumVoices-1) {
            if (modulation_type == AM) {
              float index = modulation_index_ * 5.0f;
              index *= index;
              for (int j=i+1; j<kNumVoices; j++, u++) {
                /* printf("i=%d, j=%d, u=%d\n", i, j, u); */
                modulation_sample_[j][0] *= cauchy(sin * index * modulation_matrix_am_[u]);
                modulation_sample_[j][1] *= cauchy(cos * index * modulation_matrix_am_[u]);
              }
            } else if (modulation_type == FM) {
              modulation_sample_[i+1][0] = sin * modulation_matrix_[i] * modulation_index_;
              modulation_sample_[i+1][1] = cos * modulation_matrix_[i] * modulation_index_;
            }
          }

          float gain;

          if (modulation_type == FM) {
            gain = 1.0f - modulation_matrix_[i];
          } else {
            int idx = indexes[i];
            gain = 1.0f - modulation_matrix_am_[idx];
            /* printf("gain %d = %f\n", i, gain); */
          }

          total_gain += gain;

          if (modulation_type == FM) {
            in_out->l += sin * gain;
            in_out->r += cos * gain;
          } else {              /* AM */
            if (i & 1) {
              in_out->l += sin * gain;
            } else {
              in_out->r += sin * gain;
            }
          }

          phase_[i] += phase_increment_[i];
          if (phase_[i] > 1.0) phase_[i]--;
        }

        total_gain += 2.0f;

        in_out->l /= total_gain;
        in_out->r /= total_gain;

        in_out++;
      }
    }

    void set_frequencies(float note, float spread, float fine, float distrib) {

      float n = 0.0f;
      float ratios[kNumVoices];

      for (int i=0; i<kNumVoices; i++) {
        ratios[i] = n;
        n += l_exp(-distrib * l_log(i+1));
      }

      note += fine - 69.0f;

      for (int i=0; i<kNumVoices; i++) {
        if (i&1 || !freeze_) {
          float n = note + (ratios[i] / ratios[kNumVoices-1]) * spread * (kNumVoices-1);
          phase_increment_[permutation[i]] = SemitonesToRatio(n) * a3;
        }
      }
    }

    void set_chords(float note, float spread, float fine, float chord) {

      int ch = static_cast<int>(chord * 12.0f);

      for (int i=0; i<kNumVoices; i++) {

        float note_q = note;

        int octaves = (int32_t)(note / 12.0f) * 12.0f;
        note_q = binary_search(note - octaves, chords_table[ch], 6) + octaves;

        float detune = ((float)i - 2.5f) * detune_;
        note_q += detune;

        if (note_q + fine - 69.0f < 64.0f &&
            note_q + fine - 69.0f > -64.0f)
          if (i&1 || !freeze_)
            phase_increment_[permutation[i]] = SemitonesToRatio(note_q + fine - 69.0f) * a3;

        note += spread;
      }
    }

    void set_rationals(float note, float spread, float fine, int max_denom) {

      float fundamental = SemitonesToRatio(45.0f + fine - 69.0f) * a3;

      float freq = fundamental * note;

      for (int i=0; i<kNumVoices; i++) {

        float ratio = freq / fundamental; /* TODO what if < 1 */
        ratio = closest_rational(ratio, max_denom);
        if (i&1 || !freeze_)
          phase_increment_[permutation[i]] = ratio * fundamental;

        freq *= spread;
      }
    }

    void set_harmonics(float note, float spread, float fine, float detune) {

      float fundamental = SemitonesToRatio(45.0f + fine - 69.0f) * a3;

      int k = static_cast<int>(note);

      float j=1;
      for (int i=0; i<kNumVoices; i++, j+=spread) {

        int h = static_cast<int>(k + j);

        if (i&1 || !freeze_) {
          if (h >= 0) {
            phase_increment_[permutation[i]] = fundamental * h * sqrtf(1.0f + detune * j * j);
          } else {
            phase_increment_[permutation[i]] = -fundamental / (h - 2) * sqrtf(1.0f + detune * j * j);
          }
        }
      }
    }

    void set_structure(float structure) {
      structure_ = structure;
    }

    void set_self_feedback(float feedback) {
      self_feedback_ = feedback;
    }

    void set_modulation_index(float index) {
      modulation_index_ = index;
    }

    void set_freeze(float freeze) {
      freeze_ = freeze;
    }

    void set_bitcrush(float bitcrush) {
      bitcrush_ = bitcrush;
    }

    void set_softclip(float softclip) {
      softclip_ = softclip;
    }

    void set_decimate(float decimate) {
      decimate_ = decimate;
    }

    void set_detune(float detune) {
      detune_ = detune;
    }

    void Reset() {
      for (int i=0; i<kNumVoices; i++) {
        phase_[i] = 0.0f;
      }
    }

  private:

    /* returns the closest value to x in the given sorted array */
    float binary_search(float x, const float array[], int size) {
      int low = 0;
      int high = size-1;

      while (low < high) {
        int mid = (low + high) / 2;
        if (fabs(array[mid] - x) < fabs(array[mid+1] - x)) {
          high = mid;
        } else {
          low = mid + 1;
        }
      }
      return array[high];
    }

    /* returns the closest rational to x with denominator smaller than
     * n */

    float closrat(float x, int n) {
      float a, b, c, d;
      a = 0;
      b = c = d = 1;

      while (b <= n && d <= n) {
        float med = (a+c)/(b+d);
        if (x == med) {
          if (b+d <= n) return med;
          else if (d > b) return c/d;
          else return a/b;
        } else if (x > med) {
          a += c;
          b += d;
        } else {
          c += a;
          d += b;
        }
      }

      if (b > n) return c/d;
      else return a/b;
    }
    float closest_rational(float x, int n) {
      int i=0;

      while (x > 2) {x /= 2.0f; i++; }
      x = closrat(x - 1.0f, n) + 1.0f;
      while (i--) { x *= 2.0f; }

      return x;
    }

    float l_exp(float x) {
      int i=0, j=0;
      while (x >= 20) { i++; x -= 20; }
      while (x <= -20) { j++; x += 20; }
      float y = Interpolate(lut_exp, (x + 20.0f) / 40.0f, 256.0f);
      while (i--) y *= 485165195.4; /* e^20 */
      while (j--) y /= 485165195.4; /* e^20 */
      return y;
    }

    float cauchy(float x) {
      return (1.0f / (1.0f + x * x));
    }

    float l_log(float x) {
      x = (x - 1) / 9.0f;
      return Interpolate(lut_log, x, 16.0f);
    }

    float InterpolateSine(const float* table, float index, float size) {
      index *= size;
      MAKE_INTEGRAL_FRACTIONAL(index)
        float a = table[index_integral];
      float b = table[index_integral + 1];
      index_fractional = Interpolate(lut_raised_cos, index_fractional, 256.0f);
      return a + (b - a) * index_fractional;
    }

    /* parameters */
    bool freeze_;
    float self_feedback_;
    float bitcrush_;
    float decimate_;
    float softclip_;
    float modulation_index_;
    float structure_;
    float detune_;

    float phase_[kNumVoices];
    float phase_increment_[kNumVoices];
    float self_feedback_sample_[kNumVoices][4];
    float modulation_sample_[kNumVoices][2];
    float modulation_matrix_[kNumVoices];
    float modulation_matrix_am_[kNumVoices*(kNumVoices+1)/2];
  };
}

#endif
