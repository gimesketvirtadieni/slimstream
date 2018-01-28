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

#include <chrono>
#include <cstddef>  // std::size_t
#include <optional>
#include <string>

#include "slim/Exception.hpp"
#include "slim/log/log.hpp"
#include "slim/proto/CommandAUDE.hpp"
#include "slim/proto/CommandAUDG.hpp"
#include "slim/proto/CommandHELO.hpp"
#include "slim/proto/CommandSETD.hpp"
#include "slim/proto/CommandSTAT.hpp"
#include "slim/proto/CommandSTRM.hpp"
#include "slim/util/Buffer.hpp"
#include "slim/util/ScopeGuard.hpp"


namespace slim
{
	namespace proto
	{
		template<typename ConnectionType>
		class CommandSession
		{
			using HandlersMap = std::unordered_map<std::string, std::function<std::size_t(unsigned char*, std::size_t)>>;
			using TimePoint   = std::chrono::time_point<std::chrono::steady_clock>;

			public:
				CommandSession(ConnectionType& c, std::string id)
				: connection(c)
				, clientID{id}
				, handlersMap
				{
					{"HELO", [&](auto* buffer, auto size) {return onHELO(buffer, size);}},
					{"STAT", [&](auto* buffer, auto size) {return onSTAT(buffer, size);}},
				}
				{
					LOG(DEBUG) << LABELS{"proto"} << "SlimProto session object was created (id=" << this << ")";
				}

				~CommandSession()
				{
					LOG(DEBUG) << LABELS{"proto"} << "SlimProto session object was deleted (id=" << this << ")";
				}

				CommandSession(const CommandSession&) = delete;             // non-copyable
				CommandSession& operator=(const CommandSession&) = delete;  // non-assignable
				CommandSession(CommandSession&& rhs) = delete;              // non-movable
				CommandSession& operator=(CommandSession&& rhs) = delete;   // non-movable-assignable

				inline auto getClientID()
				{
					return clientID;
				}

				inline void onRequest(unsigned char* buffer, std::size_t size)
				{
					LOG(DEBUG) << "SlimProto onRequest size=" << size;

					// adding data to the buffer
					commandBuffer.append(buffer, size);

					// removing processed data from the buffer in exception safe way
					std::size_t processedSize{commandBuffer.size()};
					auto guard = util::makeScopeGuard([&]()
					{
						commandBuffer.shrinkLeft(processedSize);
					});

					std::size_t keySize{4};
					if (processedSize > keySize)
					{
						std::string s{(char*)commandBuffer.data(), keySize};
						auto found{handlersMap.find(s)};

						if (found != handlersMap.end())
						{
							// making sure session HELO is the first command provided by a client
							if (!commandHELO.has_value() && (*found).first.compare("HELO"))
							{
								throw slim::Exception("Did not receive HELO command");
							}

							processedSize = (*found).second(commandBuffer.data(), commandBuffer.size());
						}
						else
						{
							LOG(DEBUG) << "Unsupported SlimProto command received (header='" << s << "')";

							//for (unsigned int i = 0; i < size; i++)
							//{
							//	LOG(DEBUG) << (unsigned int)buffer[i] << " " << buffer[i];
							//}
						}
					}
					else
					{
						// it will keep buffer unchanged
						processedSize = 0;
					}
				}

				inline void ping()
				{
					auto  command{CommandSTRM{CommandSelection::Time}};
					auto  buffer{command.getBuffer()};
					auto  size{command.getSize()};
					auto& nativeSocket{connection.getNativeSocket()};

					if (nativeSocket.is_open())
					{
						auto asioBuffer{asio::buffer(buffer, size)};

						// saving previous time; will be restored in case of an error
						auto tmp{lastPingAt};

						// saving ping time as close as possible to the moment of sending data out
						lastPingAt = std::chrono::steady_clock::now();

						// sending first part
						auto sent{nativeSocket.send(asioBuffer)};

						// sending remainder (it almost never happens)
						while (sent > 0 && sent < size)
						{
							auto s{nativeSocket.send(asio::buffer(&buffer[sent], size - sent))};
							if (s > 0)
							{
								s += sent;
							}
							sent = s;
						}

						// restoring the last ping time value in case of an error
						lastPingAt = tmp;
					}
				}

				void stream(unsigned int samplingRate)
				{
			        send(CommandSTRM{CommandSelection::Start, samplingRate, getClientID()});
				}

			protected:
				inline auto onHELO(unsigned char* buffer, std::size_t size)
				{
					std::size_t result{0};

					LOG(INFO) << "HELO command received";

					// if there is enough data to process HELO message
					if (CommandHELO::enoughData(buffer, size))
					{
						// deserializing HELO command
						commandHELO = CommandHELO{buffer, size};
						result      = commandHELO.value().getSize();

						send(CommandSTRM{CommandSelection::Stop});
						send(CommandSETD{DeviceID::RequestName});
						send(CommandSETD{DeviceID::Squeezebox3});
						send(CommandAUDE{true, true});
						send(CommandAUDG{});
					}

					return result;
				}

				inline auto onSTAT(unsigned char* buffer, std::size_t size)
				{
					LOG(DEBUG) << "STAT command received";

					return size;
				}

				template<typename CommandType>
				inline void send(CommandType command)
				{
					// TODO: introduce buffer wrapper so it can be passed to a stream; then in can be moved to a Command class
					for (std::size_t i = 0; i < command.getSize();)
					{
						i += connection.send(command.getBuffer() + i, command.getSize() - i);
					}
				}

			private:
				ConnectionType&            connection;
				std::string                clientID;
				HandlersMap                handlersMap;
				util::Buffer               commandBuffer{std:size_t{0}, std:size_t{2048}};
				std::optional<CommandHELO> commandHELO{std::nullopt};
				std::optional<TimePoint>   lastPingAt{std::nullopt};
		};
	}
}
