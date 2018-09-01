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

#include <cstddef>  // std::size_t
#include <cstdint>  // std::u..._t types
#include <cstring>  // std::strlen
#include <string>

#include "slim/Exception.hpp"
#include "slim/proto/Command.hpp"


namespace slim
{
	namespace proto
	{
		namespace client
		{
			#pragma pack(push, 1)
			struct SETD
			{
				char          opcode[4];
				std::uint32_t size;
				std::uint8_t  id;
			};
			#pragma pack(pop)


			class CommandSETD : public Command<SETD>
			{
				public:
					CommandSETD(unsigned char* buffer, std::size_t size)
					{
						// validating there is SETD label in place
						std::string h{"SETD"};
						std::string s{(char*)buffer, h.size()};
						if (h.compare(s))
						{
							throw slim::Exception("Missing 'SETD' label in the header");
						}

						// validating provided data is sufficient for the fixed part of SETD command
						if (size < sizeof(setd))
						{
							throw slim::Exception("Message is too small for SETD command");
						}

						// serializing fixed part of SETD command
						memcpy(&setd, buffer, sizeof(setd));

						// converting command size data
						setd.size = ntohl(setd.size);

						// validating length attribute from SETD command
						if (setd.size > sizeof(setd) + sizeof(preferences) - sizeof(setd.opcode) - sizeof(setd.size))
						{
							throw slim::Exception("Length provided in SETD command is too big");
						}

						// making sure there is enough data provided for the dynamic part of SETD command
						if (!Command::isEnoughData(buffer, size))
						{
							throw slim::Exception("Message is too small for SETD command");
						}

						// serializing dynamic part of SETD command
						memcpy(preferences, buffer + sizeof(setd), setd.size + sizeof(setd.opcode) + sizeof(setd.size) - sizeof(setd) - 1);
					}

					// using Rule Of Zero
					virtual ~CommandSETD() = default;
					CommandSETD(const CommandSETD&) = default;
					CommandSETD& operator=(const CommandSETD&) = default;
					CommandSETD(CommandSETD&& rhs) = default;
					CommandSETD& operator=(CommandSETD&& rhs) = default;

					virtual SETD* getBuffer() override
					{
						return &setd;
					}

					virtual std::size_t getSize() override
					{
						return sizeof(setd) + std::strlen(preferences) + 1;
					}

				private:
					SETD setd;
					char preferences[2048]{0};
			};
		}
	}
}