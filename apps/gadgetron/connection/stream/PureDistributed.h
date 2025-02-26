#pragma once

#include <map>
#include <list>
#include <thread>

#include "Processable.h"
#include "Reader.h"
#include "Writer.h"

#include "connection/Loader.h"
#include "connection/Config.h"

#include "common/Serialization.h"
#include "common/Configuration.h"

#include "distributed/Worker.h"
#include "distributed/Pool.h"

namespace Gadgetron::Server::Connection::Stream {

    class PureDistributed : public Processable {

    public:
        PureDistributed(
                const Config::PureDistributed &,
                const Core::Context &,
                Loader &
        );

        void process(
                Core::GenericInputChannel,
                Core::OutputChannel,
                ErrorHandler &
        ) override;

        const std::string& name() override;

    private:
        using Job = std::future<Core::Message>;
        using Queue = Core::MPMCChannel<Job>;

        void process_outbound(Core::GenericInputChannel, Queue &);
        void process_inbound(Core::OutputChannel, Queue &);

        std::shared_ptr<Serialization> serialization;
        std::shared_ptr<Configuration> configuration;

        std::list<std::future<std::unique_ptr<Worker>>> pending_workers;
    };
}
