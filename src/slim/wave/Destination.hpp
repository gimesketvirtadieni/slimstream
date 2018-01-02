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

#include <fstream>
#include <functional>
#include <iostream>
#include <memory>

#include "slim/Chunk.hpp"
#include "slim/log/log.hpp"
#include "slim/wave/WAVEFile.hpp"
#include "slim/util/OutputStreamCallback.hpp"


namespace slim
{
	namespace wave
	{
		class Destination
		{
			// TODO: it is possible to optimize by dropping std::function; an empty lambda can be used as a container
			using Callback             = std::function<std::streamsize(const char*, std::streamsize)>;
			using OutputStreamCallback = util::OutputStreamCallback<Callback>;

			public:
				explicit Destination(std::unique_ptr<std::ofstream> fs, unsigned int channels, unsigned int sampleRate, int bitsPerSample)
				: waveFile{std::move(fs), channels, sampleRate, bitsPerSample}
				, bytesPerFrame{channels * (bitsPerSample >> 3)}
				{
					waveFile.writeHeader();
				}

				// there is a need for a custom destructor so Rule Of Zero cannot be used
				// Instead of The Rule of The Big Four (and a half) the following approach is used: http://scottmeyers.blogspot.dk/2014/06/the-drawbacks-of-implementing-move.html
				~Destination()
				{
					if (!empty)
					{
						waveFile.getOutputStream().seekp(0, std::ios::end);
						auto size = waveFile.getOutputStream().tellp();
						waveFile.getOutputStream().seekp(0, std::ios::beg);
						waveFile.writeHeader(size);
					}
				}

				Destination(const Destination&) = delete;
				Destination& operator=(const Destination&) = delete;

				Destination(Destination&& rhs)
				: waveFile{std::move(rhs.waveFile)}
				, bytesPerFrame{std::move(rhs.bytesPerFrame)}
				{
					rhs.empty = true;
				}

				Destination& operator=(Destination&& rhs)
				{
					using std::swap;

					// any resources should be released for this object here because it will take over resources from rhs object
					if (!empty)
					{
						waveFile.getOutputStream().seekp(0, std::ios::end);
						auto size = waveFile.getOutputStream().tellp();
						waveFile.getOutputStream().seekp(0, std::ios::beg);
						waveFile.writeHeader(size);
					}
					rhs.empty = true;

					swap(waveFile, rhs.waveFile);
					swap(bytesPerFrame, rhs.bytesPerFrame);

					return *this;
				}

				inline void consume(Chunk& chunk)
				{
					auto size{chunk.getDataSize()};

					waveFile.write(chunk.getBuffer(), size);

					LOG(DEBUG) << "Written " << (size / bytesPerFrame) << " frames";
				}

			private:
				// TODO: empty attribute should be refactored to a separate class
				bool         empty = false;
				WAVEFile     waveFile;
				unsigned int bytesPerFrame;
		};
	}
}
