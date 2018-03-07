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

#include <chrono>
#include <conwrap/ProcessorAsio.hpp>
#include <csignal>
#include <cxxopts.hpp>
#include <exception>
#include <g3log/logworker.hpp>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "slim/alsa/Parameters.hpp"
#include "slim/alsa/Source.hpp"
#include "slim/conn/Callbacks.hpp"
#include "slim/conn/Server.hpp"
#include "slim/Container.hpp"
#include "slim/Exception.hpp"
#include "slim/log/ConsoleSink.hpp"
#include "slim/log/log.hpp"
#include "slim/Pipeline.hpp"
#include "slim/proto/CommandSession.hpp"
#include "slim/proto/Destination.hpp"
#include "slim/proto/Streamer.hpp"
#include "slim/Scheduler.hpp"
#include "slim/wave/Destination.hpp"


using ContainerBase = slim::ContainerBase;
using Connection    = slim::conn::Connection<ContainerBase>;
using Server        = slim::conn::Server<ContainerBase>;
using Callbacks     = slim::conn::Callbacks<ContainerBase>;
using Streamer      = slim::proto::Streamer<Connection>;

using Source        = slim::alsa::Source;
//using Destination   = slim::wave::Destination;
using Destination   = slim::proto::Destination<Connection>;
using Pipeline      = slim::Pipeline<Source, Destination>;
using Scheduler     = slim::Scheduler<Source, Destination>;

using Container     = slim::Container<Scheduler, Server, Server, Streamer>;


static volatile bool running = true;


void signalHandler(int sig)
{
	running = false;
}


void printVersionInfo()
{
	std::cout << "SlimStreamer version " << VERSION << std::endl;
}


void printLicenseInfo()
{
	printVersionInfo();

	std::cout << std::endl;
	std::cout << "Copyright 2017, Andrej Kislovskij" << std::endl;
	std::cout << std::endl;
	std::cout << "This is PUBLIC DOMAIN software so use at your own risk as it comes" << std::endl;
	std::cout << "with no warranties. This code is yours to share, use and modify without" << std::endl;
	std::cout << "any restrictions or obligations." << std::endl;
	std::cout << std::endl;
	std::cout << "For more information see conwrap/LICENSE or refer refer to http://unlicense.org" << std::endl;
	std::cout << std::endl;
	std::cout << "Author: gimesketvirtadieni at gmail dot com (Andrej Kislovskij)" << std::endl;
	std::cout << std::endl;

	// TODO: references to dependancies
}


auto createCommandCallbacks(Streamer& streamer)
{
	return std::move(Callbacks{}
		.setOpenCallback([&](auto& connection)
		{
			streamer.onSlimProtoOpen(connection);
		})
		.setDataCallback([&](auto& connection, unsigned char* buffer, const std::size_t size)
		{
			streamer.onSlimProtoData(connection, buffer, size);
		})
		.setCloseCallback([&](auto& connection)
		{
			streamer.onSlimProtoClose(connection);
		}));
}


auto createStreamingCallbacks(Streamer& streamer)
{
	return std::move(Callbacks{}
		.setOpenCallback([&](auto& connection)
		{
			streamer.onHTTPOpen(connection);
		})
		.setDataCallback([&](auto& connection, unsigned char* buffer, const std::size_t size)
		{
			streamer.onHTTPData(connection, buffer, size);
		})
		.setCloseCallback([&](auto& connection)
		{
			streamer.onHTTPClose(connection);
		}));
}


auto createPipelines(Streamer& streamer)
{
	std::vector<std::tuple<unsigned int, std::string>> rates
	{
		{8000,   "hw:1,1,1"},
		{11025,  "hw:1,1,2"},
		{12000,  "hw:1,1,3"},
		{16000,  "hw:1,1,4"},
		{22500,  "hw:1,1,5"},
		{24000,  "hw:1,1,6"},
		{32000,  "hw:1,1,7"},
		{44100,  "hw:2,1,1"},
		{48000,  "hw:2,1,2"},
		{88200,  "hw:2,1,3"},
		{96000,  "hw:2,1,4"},
		{176400, "hw:2,1,5"},
		{192000, "hw:2,1,6"},
	};

	slim::alsa::Parameters parameters{"", 3, SND_PCM_FORMAT_S32_LE, 0, 128, 0, 8};
	std::vector<Pipeline>  pipelines;
	unsigned int           chunkDurationMilliSecond{100};

	for (auto& rate : rates)
	{
		auto[rateValue, deviceValue] = rate;

		parameters.setSamplingRate(rateValue);
		parameters.setDeviceName(deviceValue);
		parameters.setFramesPerChunk((rateValue * chunkDurationMilliSecond) / 1000);
		//pipelines.emplace_back(Source{parameters}, Destination{std::make_unique<std::ofstream>(std::to_string(std::get<0>(rate)) + ".wav", std::ios::binary), 2, std::get<0>(rate), 32});
		pipelines.emplace_back(Source{parameters}, Destination{streamer, rateValue});
	}

	return pipelines;
}


class custom_boolean_value : public cxxopts::values::standard_value<bool>
{
	public:
		custom_boolean_value()
		{
			m_default = false;
		}
		~custom_boolean_value() = default;
};


int main(int argc, char *argv[])
{
	// initializing log and adding custom sink
	auto logWorkerPtr = g3::LogWorker::createLogWorker();
	g3::initializeLogging(logWorkerPtr.get());
	g3::only_change_at_initialization::addLogLevel(ERROR);
    logWorkerPtr->addSink(std::make_unique<ConsoleSink>(), &ConsoleSink::print);

	try
	{
		// defining supported options
		cxxopts::Options options("SlimStreamer", "SlimStreamer - A multi-room bit-perfect streamer for systemwise audio\n");
		options
			.custom_help("[options]")
			.add_options()
				("c,maxclients", "Maximum amount of clients able to connect", cxxopts::value<int>()->default_value("10"), "<number>")
				("h,help", "Print this help message", std::make_shared<custom_boolean_value>())
				("l,license", "Print license details", std::make_shared<custom_boolean_value>())
				("s,slimprotoport", "SlimProto (command connection) server port", cxxopts::value<int>()->default_value("3483"), "<port>")
				("t,httpport", "HTTP (streaming connection) server port", cxxopts::value<int>()->default_value("9000"), "<port>")
				("v,version", "Print version details", std::make_shared<custom_boolean_value>());

		// parsing provided options
		auto result = options.parse(argc, argv);

		if (result.count("help"))
		{
			std::cout << options.help() << std::endl;
		}
		else if (result.count("license"))
		{
			printLicenseInfo();
		}
		else if (result.count("version"))
		{
			printVersionInfo();
		}
		else
		{
			auto maxClients    = result["maxclients"].as<int>();
			auto slimprotoPort = result["slimprotoport"].as<int>();
			auto httpPort      = result["httpport"].as<int>();

			// Callbacks objects 'glue' SlimProto Streamer with TCP Command Servers
			auto streamerPtr{std::make_unique<Streamer>(httpPort)};
			auto commandServerPtr{std::make_unique<Server>(slimprotoPort, maxClients, createCommandCallbacks(*streamerPtr))};
			auto streamingServerPtr{std::make_unique<Server>(httpPort, maxClients, createStreamingCallbacks(*streamerPtr))};

			// creating Scheduler object
			auto schedulerPtr{std::make_unique<Scheduler>(createPipelines(*streamerPtr))};

			// creating Container object within Asio Processor with Scheduler and Servers
			conwrap::ProcessorAsio<ContainerBase> processorAsio
			{
				std::unique_ptr<ContainerBase>
				{
					new Container(std::move(schedulerPtr), std::move(commandServerPtr), std::move(streamingServerPtr), std::move(streamerPtr))
				}
			};

			// start streaming
			LOG(INFO) << "Starting SlimStreamer...";
			auto startStatus = processorAsio.process([](auto context) -> bool
			{
				auto started{false};

				try
				{
					context.getResource()->start();
					started = true;
				}
				catch (const std::exception& error)
				{
					LOG(ERROR) << error.what();
				}
				catch (...)
				{
					std::cout << "Unexpected exception" << std::endl;
				}

				return started;
			});
			if (startStatus.get())
			{
				LOG(INFO) << "SlimStreamer was started";

				// registering signal handler
				signal(SIGHUP, signalHandler);
				signal(SIGTERM, signalHandler);
				signal(SIGINT, signalHandler);

				// waiting for Control^C
				while(running)
				{
					std::this_thread::sleep_for(std::chrono::milliseconds{200});
				}

				// stop streaming
				LOG(INFO) << "Stopping SlimStreamer...";
				processorAsio.process([](auto context)
				{
					context.getResource()->stop();
				}).wait();
				LOG(INFO) << "SlimStreamer was stopped";
			}
		}
	}
	catch (const cxxopts::OptionException& e)
	{
		std::cout << "Wrong option(s) provided: " << e.what() << std::endl;
	}
	catch (const slim::Exception& error)
	{
		LOG(ERROR) << error;
	}
	catch (const std::exception& error)
	{
		LOG(ERROR) << error.what();
	}
	catch (...)
	{
		std::cout << "Unexpected exception" << std::endl;
	}

	return 0;
}
