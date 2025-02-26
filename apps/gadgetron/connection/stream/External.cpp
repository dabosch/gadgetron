#include "External.h"

#include "connection/Config.h"
#include "connection/SocketStreamBuf.h"
#include "connection/stream/common/Closer.h"
#include "connection/stream/common/ExternalChannel.h"

#include "external/Python.h"
#include "external/Matlab.h"

#include <boost/asio.hpp>
#include <boost/algorithm/string.hpp>


using namespace Gadgetron::Core;
using namespace Gadgetron::Server::Connection;
using namespace Gadgetron::Server::Connection::Stream;

namespace {

    const std::map<std::string, std::function<void(const Config::Execute &, unsigned short, const Context &)>> modules{
            {"python", start_python_module}
    };

    void process_input(GenericInputChannel input, std::shared_ptr<ExternalChannel> external) {
        auto closer = make_closer(external);
        for (auto message : input) {
            external->push_message(std::move(message));
        }
    }

    void process_output(OutputChannel output, std::shared_ptr<ExternalChannel> external) {
        while(true) {
            output.push_message(external->pop());
        }
    }
}

namespace Gadgetron::Server::Connection::Stream {

    std::unique_ptr<std::iostream> External::open_connection(Config::Connect connect, const Context &context) {

        GINFO_STREAM("Connecting to external module on port: " << connect.port);

        return std::unique_ptr<std::iostream>(nullptr);
    }

    std::unique_ptr<std::iostream> External::open_connection(Config::Execute execute, const Context &context) {

        boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v6(), 0);
        boost::asio::ip::tcp::acceptor acceptor(io_service, endpoint);

        auto port = acceptor.local_endpoint().port();
        boost::algorithm::to_lower(execute.type);

        GINFO_STREAM("Waiting for external module '" << execute.name << "' on port: " << port);

        modules.at(execute.type)(execute, port, context);

        auto socket = std::make_unique<boost::asio::ip::tcp::socket>(io_service);
        acceptor.accept(*socket);
        acceptor.close();

        GINFO_STREAM("Connected to external module '" << execute.name << "' on port: " << port);

        return Gadgetron::Connection::stream_from_socket(std::move(socket));
    }

    std::shared_ptr<ExternalChannel> External::open_external_channel(
            const Config::External &config,
            const Context &context
    ) {
        auto stream = Core::visit([&](auto action) { return open_connection(action, context); }, config.action);
        return std::make_shared<ExternalChannel>(
                std::move(stream),
                serialization,
                configuration
        );
    }

    External::External(
            const Config::External &config,
            const Core::Context &context,
            Loader &loader
    ) : serialization(std::make_shared<Serialization>(
                loader.load_default_and_additional_readers(config),
                loader.load_default_and_additional_writers(config)
        )),
        configuration(std::make_shared<Configuration>(
                context,
                config
        )) {
        channel = std::async(
                std::launch::async,
                [=](auto config, auto context) { return open_external_channel(config, context); },
                config,
                context
        );
    }

    void External::process(
            InputChannel input,
            OutputChannel output,
            ErrorHandler &error_handler
    ) {
        std::shared_ptr<ExternalChannel> external = channel.get();

        auto input_thread = error_handler.run(
                [=](auto input) { ::process_input(std::move(input), external); },
                std::move(input)
        );

        auto output_thread = error_handler.run(
                [=](auto output) { ::process_output(std::move(output), external); },
                std::move(output)
        );

        input_thread.join(); output_thread.join();
    }

    const std::string &External::name() {
        static const std::string name = "external";
        return name;
    }
}