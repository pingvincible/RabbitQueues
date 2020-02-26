#include <iostream>
#include <boost/asio.hpp>
#include "BoostConnectionHandler.h"

using boost::asio::ip::tcp;

enum { max_length = 4 };

int main(int argc, char* argv[])
{
    try
    {
        std::cout << "Client application" << std::endl;
        if (argc != 5)
        {
            std::cerr << "Usage: client <server_host> <server_port> <rabbit_host> <rabbit_port>\n";
            return 1;
        }
        
        boost::asio::io_context io_context;
        tcp::socket server_socket(io_context);
    	tcp::resolver resolver(io_context);
        boost::asio::connect(server_socket, resolver.resolve(argv[1], argv[2]));

        BoostConnectionHandler handler(io_context, argv[3], argv[4]);
        AMQP::Connection connection(&handler, AMQP::Login("guest", "guest"), "/");
        AMQP::Channel channel(&connection);

    	while (true) 
        {
            int message;
            const size_t message_length_read = boost::asio::read(server_socket, boost::asio::buffer(&message, sizeof message));
            std::cout << "Message is: " << message << std::endl;

            channel.onReady([&]()
                {
                    if (handler.connected())
                    {
                        channel.publish("", "QUEUE", std::to_string(message));
                        handler.quit();
                    }
                });

            handler.loop();
        }

    }
    catch (std::exception & e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}