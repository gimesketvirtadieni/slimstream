/*
 * Copyright 2017, Andrej Kislovskij
 *
 * This is PUBLIC DOMAIN software so use at your own risk as it comes
 * with no warranties. This code is yours to share, use and modify without
 * any restrictions or obligations.
 *
 * For more information see conwrap/LICENSE or refer refer to http://unlicense.org
 *
 * Author: gimesketvirtadieni at gmail dot com (Andrej Kislovskij)
 */

#pragma once

#include <alsa/asoundlib.h>
#include <cstddef>  // std::size_t
#include <memory>
#include <string>


namespace slim
{
	namespace alsa
	{
		class Parameters
		{
			public:
				Parameters(std::string d, unsigned int c, snd_pcm_format_t f, unsigned int r, std::size_t qs, snd_pcm_uframes_t fc, unsigned int p)
				: deviceName{d}
				, channels{c}
				, format{f}
				, samplingRate{r}
				, queueSize{qs}
				, framesPerChunk{fc}
				, periods{p} {}

				// using Rule Of Zero
			   ~Parameters() = default;
				Parameters(const Parameters&) = default;
				Parameters& operator=(const Parameters&) = default;
				Parameters(Parameters&& rhs) = default;
				Parameters& operator=(Parameters&& rhs) = default;

				inline const auto getBitsPerSample() const
				{
					return static_cast<unsigned int>(snd_pcm_format_physical_width(format));
				}

				inline const auto getBitsPerValue() const
				{
					return static_cast<unsigned int>(snd_pcm_format_width(format));
				}

				inline const std::string getDeviceName() const
				{
					return deviceName;
				}

				inline const snd_pcm_format_t getFormat() const
				{
					return format;
				}

				inline const snd_pcm_uframes_t getFramesPerChunk() const
				{
					return framesPerChunk;
				}

				inline const unsigned int getLogicalChannels() const
				{
					return channels - 1;
				}

				inline const std::size_t getQueueSize() const
				{
					// TODO: calculate based on latency
					return queueSize;
				}

				inline const unsigned int getPeriods() const
				{
					return periods;
				}

				inline const unsigned int getSamplingRate() const
				{
					return samplingRate;
				}

				inline const unsigned int getStartThreshold() const
				{
					// TODO: parametrize
					return 5;
				}

				inline const unsigned int getTotalChannels() const
				{
					return channels;
				}

				inline void setDeviceName(std::string d)
				{
					deviceName = d;
				}

				inline void setFramesPerChunk(snd_pcm_uframes_t f)
				{
					framesPerChunk = f;
				}

				inline void setSamplingRate(unsigned int r)
				{
					samplingRate = r;
				}

			private:
				std::string       deviceName;
				unsigned int      channels;
				snd_pcm_format_t  format;
				unsigned int      samplingRate;
				std::size_t       queueSize;
				snd_pcm_uframes_t framesPerChunk;
				unsigned int      periods;
		};
	}
}
