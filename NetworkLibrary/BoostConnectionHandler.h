#pragma once

#include <amqpcpp.h>
#include <boost/asio.hpp>
#include <memory>
using boost::asio::ip::tcp;

struct BoostHandlerImpl;

class BoostConnectionHandler final : public AMQP::ConnectionHandler
{
public:

    static constexpr size_t buffer_size = 8 * 1024 * 1024; //8Mb
    static constexpr size_t temp_buffer_size = 1024 * 1024; //1Mb

    BoostConnectionHandler(boost::asio::io_context& context, const std::string& host, const std::string& port);
    ~BoostConnectionHandler();

    void loop() const;
    void quit() const;

    bool connected() const;

private:

    void close();

    void onData(AMQP::Connection* connection, const char* data, size_t size) override;

    void onReady(AMQP::Connection* connection) override;

    void onError(AMQP::Connection* connection, const char* message) override;

    void onClosed(AMQP::Connection* connection) override;

    void send_data_from_buffer() const;

    std::shared_ptr<BoostHandlerImpl> m_impl_;
};
