// Copyright 2017 Julius Kammerl.
//
// Author: Julius Kammerl (julius@kammerl.com)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
// 
// See http://creativecommons.org/licenses/MIT/ for more information.
//
// -----------------------------------------------------------------------------
//
// Beat repeat effects ported from the Kammerl Kaske VST plugins.
// For more information, see
// http://www.kammerl.de/audio
// http://www.kammerl.de/audio/clouds

#ifndef CLOUDS_DSP_KAMMERL_PLAYER_H_
#define CLOUDS_DSP_KAMMERL_PLAYER_H_

#include <cmath>
#include <limits>

#include "stmlib/dsp/units.h"
#include "stmlib/stmlib.h"
#include "stmlib/utils/random.h"

#include "clouds/dsp/audio_buffer.h"
#include "clouds/dsp/frame.h"
#include "clouds/dsp/parameters.h"

#include "clouds/resources.h"

namespace clouds {

// Maximum clock divisor.
const int kMaxClockDividerLog2 = 3; // 2^3 = 8

// Size of slice pool.
const int kNumMaxSlices = 8;

enum PlaybackModes {
	PLAYBACK_MODE_UNINITIALIZED = 0,
	PLAYBACK_MODE_BYPASS = 1,
	PLAYBACK_MODE_FIXED_PITCH = 2,
	PLAYBACK_MODE_REVERSED_PITCH = 3,
	PLAYBACK_MODE_DECREASING_PITCH = 4,
	PLAYBACK_MODE_INCREASING_PITCH = 5,
	PLAYBACK_MODE_SCRATCH_PITCH = 6,
	NUM_PLAYBACK_MODES = 7
};
const size_t kNumPitchModes = 5;

enum SliceStepModes {
	DISABLED = 0,
	ONE_STEP = 1,
	TWO_STEP = 2,
	THREE_STEP = 3,
	FIVE_STEP = 4,
	SIX_STEP = 5,
	SEVEN_STEP = 6,
	RANDOM_STEP = 7,
	NUM_SLICE_STEP_MODES = 8
};

using namespace stmlib;

class KammerlPlayer {
public:
	KammerlPlayer() {
	}
	~KammerlPlayer() {
	}

	void Init(int32_t num_channels) {
		num_channels_ = num_channels;
		num_samples_since_trigger_ = 0;
		slice_buffer_pos_index_ = 0;
		slice_play_pos_samples_ = 0.0f;
		num_remaining_samples_in_slice_ = 0;

		playback_mode_ = PLAYBACK_MODE_UNINITIALIZED;
		slice_size_samples_ = 0;
		slice_loop_size_percent_ = 0.0f;
		slice_loop_begin_percent_ = 0.0f;
		slice_play_direction_ = 1.0f;
		alternating_loop_enabled_ = false;
	}

	// Quantize the loop begin and loop size as follows:
	// [0-1/64] free/unquantized, 1/64, 1/32, 1/16, 1/8, 1/4, 1/3, 1/2
	float quantizeSize(float size) const;

	bool isSlicePlaybackActive() const {
		return num_remaining_samples_in_slice_ > 0;
	}

	// Returns the number of slices to skip based on the slice modulation input.
	int getSliceStep(float slice_modulation) const {
		SliceStepModes slice_step_selection =
				static_cast<SliceStepModes>(slice_modulation
						* (NUM_SLICE_STEP_MODES - 1) + 0.5f);
		int slice_step = 0;
		switch (slice_step_selection) {
		case DISABLED:
			slice_step = 0;
			break;
		case ONE_STEP:
			slice_step += 1;
			break;
		case TWO_STEP:
			slice_step += 2;
			break;
		case THREE_STEP:
			slice_step += 3;
			break;
		case FIVE_STEP:
			slice_step += 5;
			break;
		case SIX_STEP:
			slice_step += 6;
			break;
		case SEVEN_STEP:
			slice_step += 7;
			break;
		default:
		case RANDOM_STEP:
			slice_step += stmlib::Random::GetFloat() * (kNumMaxSlices - 1)
					+ 0.5;
			break;
		}
		return slice_step;
	}

	template<Resolution resolution>
	void Play(const AudioBuffer<resolution>* buffer,
			const Parameters& parameters, float* out, size_t size) {
		int32_t max_delay = buffer->size() - 4;
		num_samples_since_trigger_ += size;
		if (num_samples_since_trigger_ > max_delay) {
			num_samples_since_trigger_ = 0;
			playback_mode_ = PLAYBACK_MODE_BYPASS;
		}
		if (parameters.trigger
				|| playback_mode_ == PLAYBACK_MODE_UNINITIALIZED) {
			const int32_t latest_trigger_interval_samples =
					num_samples_since_trigger_;
			num_samples_since_trigger_ = 0;

			const bool slice_still_playing = num_remaining_samples_in_slice_
					> latest_trigger_interval_samples / 2;
			const float rand_percentage = stmlib::Random::GetFloat();
			const bool trigger_slice = !slice_still_playing
					&& ((rand_percentage < parameters.kammerl.probability)
							|| parameters.freeze
							|| playback_mode_ == PLAYBACK_MODE_UNINITIALIZED);

			if (!trigger_slice && !slice_still_playing) {
				playback_mode_ = PLAYBACK_MODE_BYPASS;
			}

			if (trigger_slice) {
				int clock_divider_value = parameters.kammerl.clock_divider
						* kMaxClockDividerLog2 + 0.5f;
				CONSTRAIN(clock_divider_value, 0, kMaxClockDividerLog2);
				slice_size_samples_ = latest_trigger_interval_samples
						<< clock_divider_value;
				num_remaining_samples_in_slice_ = slice_size_samples_;

				// Select playback slice.
				const int slice_step = getSliceStep(
						parameters.kammerl.slice_modulation);
				int slice_idx_offset = parameters.kammerl.slice_selection
						* (kNumMaxSlices - 1) + 0.5f;
				slice_idx_offset = (slice_idx_offset + slice_step)
						% kNumMaxSlices;

				// Calculate slice position in recording buffer.
				const int32_t num_samples_back_in_time = (slice_idx_offset
						* slice_size_samples_) % buffer->size();
				slice_buffer_pos_index_ = buffer->head() - 4 - size
						+ buffer->size();
				slice_buffer_pos_index_ += buffer->size()
						- num_samples_back_in_time;
				slice_buffer_pos_index_ <<= 12;

				// Initialize slice play head position.
				slice_play_pos_samples_ = 0.0f;
				slice_play_direction_ = 1.0f;

				// Set playback mode and loop configuration.
				int playback_mode_enum = parameters.kammerl.pitch_mode
						* (kNumPitchModes - 1) + 0.5f;
				CONSTRAIN(playback_mode_enum, 0, kNumPitchModes - 1);
				playback_mode_ =
						static_cast<PlaybackModes>(PLAYBACK_MODE_FIXED_PITCH
								+ playback_mode_enum);
				slice_loop_begin_percent_ = quantizeSize(parameters.position);
				if (parameters.size < 0.5f) {
					slice_loop_size_percent_ = quantizeSize(parameters.size);
					alternating_loop_enabled_ = false;
				} else {
					slice_loop_size_percent_ = quantizeSize(
							1.0f - parameters.size);
					alternating_loop_enabled_ = true;
				}
			}
		}

		// Get pitch and loop scaling parameters.
		float pitch_parameter = parameters.kammerl.pitch;
		CONSTRAIN(pitch_parameter, 0.0f, 1.0f);
		float loop_scaling_parameter =
				parameters.kammerl.size_modulation > 0.1f ?
						parameters.kammerl.size_modulation : 0.0f;
		CONSTRAIN(loop_scaling_parameter, 0.0f, 1.0f);

		while (size--) {
			const int32_t buffer_playback_head = buffer->head() - 4 - size
					+ buffer->size();

			// Calculate the playback percentage of current slice.
			float slice_remaining_percentage = 0.0f;
			if (slice_size_samples_ != 0) {
				slice_remaining_percentage =
						static_cast<float>(num_remaining_samples_in_slice_)
								/ static_cast<float>(slice_size_samples_);
			}
			CONSTRAIN(slice_remaining_percentage, 0.0f, 1.0f);
			const float slice_processed_percentage = 1.0f
					- slice_remaining_percentage;
			if (num_remaining_samples_in_slice_ > 0) {
				--num_remaining_samples_in_slice_;
			}

			float playback_sample_step = 1.0f;
			switch (playback_mode_) {
			case PLAYBACK_MODE_FIXED_PITCH:
				playback_sample_step = pitch_parameter;
				break;
			case PLAYBACK_MODE_REVERSED_PITCH:
				// We'll reverse the playback direction when mapping slice_play_pos_samples_ to the buffer index.
				playback_sample_step = pitch_parameter;
				break;
			case PLAYBACK_MODE_DECREASING_PITCH:
				playback_sample_step = 1.0f
						- (1.0f - pitch_parameter) * slice_processed_percentage;
				break;
			case PLAYBACK_MODE_INCREASING_PITCH:
				playback_sample_step = 1.0f
						- (1.0f - pitch_parameter) * slice_remaining_percentage;
				break;
			case PLAYBACK_MODE_SCRATCH_PITCH:
				// TODO: Use precomputed sin lookup table.
				playback_sample_step = 1.0f
						- Interpolate(lut_sin, slice_processed_percentage,
								1024.0f) * (1.0f - pitch_parameter);
				break;
			default:
				playback_sample_step = pitch_parameter;
				break;
			}
			CONSTRAIN(playback_sample_step, -2.0f, 2.0f);

			// Loop size scaling
			float loop_size_scaling = 1.0f
					- loop_scaling_parameter * slice_processed_percentage;
			CONSTRAIN(loop_size_scaling, 0.0f, 1.0f);

			// Set current loop size.
			const float slice_loop_size_samples = slice_size_samples_
					* slice_loop_size_percent_ * loop_size_scaling;

			// Absolute loop bounds in samples.
			const float slice_loop_begin_samples = slice_loop_begin_percent_
					* slice_size_samples_;
			const float slice_loop_end_samples = slice_loop_begin_samples
					+ slice_loop_size_samples;

			// Adjust |slice_play_pos_samples_| when loop bounds are reached.
			if (slice_play_direction_ > 0.0f) {
				if (slice_play_pos_samples_ > slice_loop_end_samples) {
					if (slice_loop_size_samples < 1E-5f) {
						slice_play_pos_samples_ = slice_loop_end_samples;
					} else {
						float slice_within_loop_pos = std::fmod(
								slice_play_pos_samples_
										- slice_loop_begin_samples,
								slice_loop_size_samples);
						if (!alternating_loop_enabled_) {
							slice_play_pos_samples_ = slice_loop_begin_samples
									+ slice_within_loop_pos;
						} else {
							slice_play_pos_samples_ = slice_loop_begin_samples
									+ (slice_loop_size_samples
											- slice_within_loop_pos);
							slice_play_direction_ = -1.0f;
						}

					}
				}
			} else {
				if (slice_play_pos_samples_ < slice_loop_begin_samples) {
					if (slice_loop_size_samples < 1E-5f) {
						slice_play_pos_samples_ = slice_loop_begin_samples;
					} else {
						float slice_within_loop_pos = std::fmod(
								slice_loop_begin_samples
										- slice_play_pos_samples_,
								slice_loop_size_samples);
						slice_play_pos_samples_ = slice_loop_begin_samples
								+ slice_within_loop_pos;
						slice_play_direction_ = 1.0f;
					}
				}
			}

			const float slice_play_pos_samples_flipped =
					(playback_mode_ == PLAYBACK_MODE_REVERSED_PITCH) ?
							slice_play_pos_samples_ * -1.0f :
							slice_play_pos_samples_;

			int32_t buffer_pos_idx;
			if (playback_mode_ == PLAYBACK_MODE_BYPASS) {
				// Playback from head
				buffer_pos_idx = buffer_playback_head << 12;
			} else {
				buffer_pos_idx = slice_buffer_pos_index_
						+ (slice_play_pos_samples_flipped + buffer->size())
								* 4096.0f;
			}

			// Apply |playback_sample_step| to current slice play head.
			slice_play_pos_samples_ += playback_sample_step
					* slice_play_direction_;

			// Safety check to ensure the 4 sample lookahead doesn't lead to an out-of-bound read.
			uint32_t buffer_pos_idx_in_buffer = buffer_pos_idx >> 12;
			uint32_t buffer_pos_fract = buffer_pos_idx << 4;
			buffer_pos_idx_in_buffer = buffer_pos_idx_in_buffer
					% buffer->size();
			CONSTRAIN(buffer_pos_idx_in_buffer, 0, buffer->size() - 5);

			// Write to output.
			float l = buffer[0].ReadHermite(buffer_pos_idx_in_buffer,
					buffer_pos_fract);
			if (num_channels_ == 1) {
				*out++ = l;
				*out++ = l;
			} else if (num_channels_ == 2) {
				float r = buffer[1].ReadHermite(buffer_pos_idx_in_buffer,
						buffer_pos_fract);
				*out++ = l;
				*out++ = r;
			}
		}
	}

private:
	int32_t num_channels_;
	int32_t num_samples_since_trigger_;

	// Currently active playback mode.
	PlaybackModes playback_mode_;

	// Loop begin relative to the slice duration.
	float slice_loop_begin_percent_;

	// Loop size relative to the slice duration.
	float slice_loop_size_percent_;

	// Current slice size in samples.
	int32_t slice_size_samples_;

	// Index of slice position in recording buffer.
	int32_t slice_buffer_pos_index_;

	// Current slice playhead position in fractional samples.
	float slice_play_pos_samples_;

	// Slice playback direction (-1.0f, 1.0f).
	float slice_play_direction_;

	// Flag to enable alternating looping.
	bool alternating_loop_enabled_;

	// Number of remaining samples to be played back.
	int32_t num_remaining_samples_in_slice_;

	DISALLOW_COPY_AND_ASSIGN (KammerlPlayer);
};

}  // namespace clouds

#endif  // CLOUDS_DSP_KAMMERL_PLAYER_H_
