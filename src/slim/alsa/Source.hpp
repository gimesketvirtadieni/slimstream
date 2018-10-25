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
#include <atomic>
#include <chrono>
#include <conwrap2/Processor.hpp>
#include <conwrap2/ProcessorProxy.hpp>
#include <functional>
#include <memory>
#include <mutex>
#include <type_safe/optional.hpp>

#include "slim/alsa/Parameters.hpp"
#include "slim/Chunk.hpp"
#include "slim/Consumer.hpp"
#include "slim/ContainerBase.hpp"
#include "slim/log/log.hpp"
#include "slim/Producer.hpp"
#include "slim/util/RealTimeQueue.hpp"
#include "slim/util/BigInteger.hpp"


namespace slim
{
	namespace alsa
	{
		namespace ts = type_safe;

		enum class StreamMarker : unsigned char
		{
			beginningOfStream = 1,
			endOfStream       = 2,
			data              = 3,
		};

		class Source : public Producer
		{
			using QueueType = util::RealTimeQueue<Chunk>;

			public:
				Source(conwrap2::ProcessorProxy<std::unique_ptr<ContainerBase>> pp, Parameters pa, std::function<void()> oc = [] {})
				: Producer{pp}
				, parameters{pa}
				, overflowCallback{std::move(oc)}
				, queuePtr{std::make_unique<QueueType>(parameters.getQueueSize(), [&](Chunk& chunk)
				{
					// no need to store data from the last channel as it contains commands
					chunk.setCapacity(pa.getFramesPerChunk() * pa.getLogicalChannels() * (pa.getBitsPerSample() >> 3));
				})} {}

				virtual ~Source()
				{
					// it is safe to call stop method multiple times
					stop(false);
				}

				Source(const Source&) = delete;             // non-copyable
				Source& operator=(const Source&) = delete;  // non-assignable
				Source(Source&& rhs) = delete;              // non-movable
				Source& operator=(Source&& rhs) = delete;   // non-move-assignable

				inline auto getParameters()
				{
					return parameters;
				}

				virtual bool isRunning() override
				{
					return running;
				}

				virtual ts::optional<unsigned int> produceChunk(std::function<bool(Chunk&)>& consumer) override
				{
					auto result{ts::optional<unsigned int>{ts::nullopt}};

					if (chunkCounter > parameters.getStartThreshold())
					{
						result = producer(consumer);
					}

					return result;
				}

				virtual ts::optional<unsigned int> skipChunk() override
				{
					// using an empty lambda that returns 'true' to 'consume' chunk without a real consumer
					return producer([](auto&)
					{
						return true;
					});
				}

				virtual void start() override;
				virtual void stop(bool gracefully = true) override;

			protected:
				void              close();
				snd_pcm_sframes_t containsData(unsigned char* buffer, snd_pcm_uframes_t frames);
				snd_pcm_uframes_t copyData(unsigned char* srcBuffer, unsigned char* dstBuffer, snd_pcm_uframes_t frames);

				inline auto formatError(std::string message, int error = 0)
				{
					return message + ": name='" + parameters.getDeviceName() + (error != 0 ? std::string{"' error='"} + snd_strerror(error) + "'" : "");
				}

				void open();

				template<typename ConsumerType>
				inline ts::optional<unsigned int> producer(const ConsumerType& consumer)
				{
					auto result{ts::optional<unsigned int>{ts::nullopt}};

					queuePtr->dequeue([&](Chunk& chunk) -> bool  // 'mover' function
					{
						auto consumed{false};

						// feeding consumer with a chunk
						if (consumer(chunk))
						{
							consumed = true;

							// if chunk was consumed and it is the end of the stream
							if (chunk.isEndOfStream())
							{
								chunkCounter = 0;
								LOG(DEBUG) << LABELS{"slim"} << "EndOfStream";
							}
							else
							{
								result = 0;
							}
						}
						else
						{
							// if consumer did not accept a chunk then deferring further processing
							// TODO: cruise control should be implemented
							result = 10;
						}

						return consumed;
					}, [&]  // underflow callback
					{
						result = 10;
					});

					// if there are more chunks to be consumed
					return result;
				}

				bool restore(snd_pcm_sframes_t error);

			private:
				Parameters                    parameters;
				std::function<void()>         overflowCallback;
				std::unique_ptr<QueueType>    queuePtr;
				snd_pcm_t*                    handlePtr{nullptr};
				std::atomic<bool>             running{false};
				// TODO: get rid of this atomic
				std::atomic<util::BigInteger> chunkCounter{0};
				bool                          streaming{true};
				std::mutex                    lock;
		};
	}
}
