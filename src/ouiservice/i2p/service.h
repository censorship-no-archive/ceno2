#pragma once

#include <memory>

#include <boost/asio.hpp>

#include "client.h"
#include "server.h"

#include "../../ouiservice.h"

namespace ouinet {
namespace ouiservice {
namespace i2poui {

class Service : public std::enable_shared_from_this<Service> {
public:
    Service(const std::string& datadir, asio::io_service&);

    Service(const Service&) = delete;
    Service& operator=(const Service&) = delete;

    Service(Service&&);
    Service& operator=(Service&&);

    ~Service();

    uint32_t  get_i2p_tunnel_ready_timeout() { return 5*60; /* 5 minutes */ };

    asio::io_service& get_io_service() {   return _ios; };

    std::unique_ptr<Server> build_server(const std::string& private_key_filename);
    std::unique_ptr<Client> build_client(const std::string& target_id);

protected:
    asio::io_service& _ios;
    std::string _data_dir;
};

} // i2poui namespace
} // ouiservice namespace
} // ouinet namespace
