// Queue.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include "BoostConnectionHandler.h"

int main(int argc, char* argv[])
{
	if (argc != 3)
	{
		std::cerr << "Usage: queue <rabbit_host> <rabbit_port>\n";
		return 1;
	}

	boost::asio::io_context io_context;
	BoostConnectionHandler handler(io_context, argv[1], argv[2]);
	std::cout << "Queue application" << std::endl;
	AMQP::Connection connection(&handler, AMQP::Login("guest", "guest"), "/");

	AMQP::Channel channel(&connection);
	channel.declareQueue("QUEUE");
	channel.consume("QUEUE", AMQP::noack).onReceived(
		[](const AMQP::Message& message,
			uint64_t deliveryTag,
			bool redelivered)
		{
			std::cout.write(&message.body()[0], message.bodySize());
			std::cout << std::endl;
		});
	handler.loop();
	return 0;
}

