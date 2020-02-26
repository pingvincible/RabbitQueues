#include "BoostConnectionHandler.h"
#include <vector>
#include <thread>
#include <chrono>
#include <cstring>
#include <cassert>
#include <iostream>
#include <memory>
#include <string>

namespace
{
    class buffer
    {
    public:
	    explicit buffer(size_t size) :
            m_data(size, 0),
            m_use(0)
        {
        }

        size_t write(const char* data, size_t size)
        {
            if (m_use == m_data.size())
            {
                return 0;
            }

            const size_t length = (size + m_use);
            const size_t write = length < m_data.size() ? size : m_data.size() - m_use;
            memcpy(m_data.data() + m_use, data, write);
            m_use += write;
            return write;
        }

        void drain()
        {
            m_use = 0;
        }

        size_t available() const
        {
            return m_use;
        }

        const char* data() const
        {
            return m_data.data();
        }

        void shl(size_t count)
        {
            assert(count < m_use);

            const size_t diff = m_use - count;
            std::memmove(m_data.data(), m_data.data() + count, diff);
            m_use = m_use - count;
        }

    private:
        std::vector<char> m_data;
        size_t m_use;
    };
}

struct BoostHandlerImpl
{
    BoostHandlerImpl(boost::asio::io_context& context) :
		socket(std::make_shared<tcp::socket>(context)),
        connected(false),
        connection(nullptr),
        quit(false),
        inputBuffer(BoostConnectionHandler::buffer_size),
        outBuffer(BoostConnectionHandler::buffer_size),
        tmpBuff(BoostConnectionHandler::temp_buffer_size)
    {
    }

    std::shared_ptr<tcp::socket> socket;
    bool connected;
    AMQP::Connection* connection;
    bool quit;
    buffer inputBuffer;
    buffer outBuffer;
    std::vector<char> tmpBuff;
};

BoostConnectionHandler::BoostConnectionHandler(boost::asio::io_context& context, const std::string& host, const std::string& port) :
    m_impl_(new BoostHandlerImpl(context))
{
    tcp::resolver resolver(context);
    boost::asio::connect(*m_impl_->socket, resolver.resolve(host, port));
}

BoostConnectionHandler::~BoostConnectionHandler()
{
    close();
}

void BoostConnectionHandler::loop() const
{
    try
    {
        while (!m_impl_->quit)
        {
            if (m_impl_->socket->available() > 0)
            {
	            const auto avail = m_impl_->socket->available();
                if (m_impl_->tmpBuff.size() < avail)
                {
                    m_impl_->tmpBuff.resize(avail, 0);
                }
                const auto bytes_read = boost::asio::read(*m_impl_->socket, boost::asio::buffer(&m_impl_->tmpBuff[0], avail));
                m_impl_->inputBuffer.write(m_impl_->tmpBuff.data(), avail);
            }

            if (m_impl_->connection && m_impl_->inputBuffer.available())
            {
	            const auto count = m_impl_->connection->parse(m_impl_->inputBuffer.data(),
                    m_impl_->inputBuffer.available());

                if (count == m_impl_->inputBuffer.available())
                {
                    m_impl_->inputBuffer.drain();
                }
                else if (count > 0) {
                    m_impl_->inputBuffer.shl(count);
                }
            }
            send_data_from_buffer();

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        if (m_impl_->quit && m_impl_->outBuffer.available())
        {
            send_data_from_buffer();
        }

    }
    catch (const std::exception & e)
    {
        std::cerr << "Asio exception " << e.what();
    }
}

void BoostConnectionHandler::quit() const
{
    m_impl_->quit = true;
}

void BoostConnectionHandler::close()
{
	m_impl_->socket->close();
}

void BoostConnectionHandler::onData(
    AMQP::Connection* connection, const char* data, size_t size)
{
    m_impl_->connection = connection;
    const size_t written = m_impl_->outBuffer.write(data, size);
    if (written != size)
    {
        send_data_from_buffer();
        m_impl_->outBuffer.write(data + written, size - written);
    }
}

void BoostConnectionHandler::onReady(AMQP::Connection* connection)
{
    m_impl_->connected = true;
}

void BoostConnectionHandler::onError(
    AMQP::Connection* connection, const char* message)
{
    std::cerr << "AMQP error " << message << std::endl;
}

void BoostConnectionHandler::onClosed(AMQP::Connection* connection)
{
    std::cout << "AMQP closed connection" << std::endl;
    m_impl_->quit = true;
}

bool BoostConnectionHandler::connected() const
{
    return m_impl_->connected;
}

void BoostConnectionHandler::send_data_from_buffer() const
{
    if (m_impl_->outBuffer.available())
    {
        boost::asio::write(*m_impl_->socket, boost::asio::buffer(m_impl_->outBuffer.data(), m_impl_->outBuffer.available()));
        m_impl_->outBuffer.drain();
    }
}
