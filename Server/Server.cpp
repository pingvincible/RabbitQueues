
#include <cstdlib>
#include <iostream>
#include <thread>
#include <boost/asio.hpp>
#include <random>

using boost::asio::ip::tcp;

void session(tcp::socket sock, int number)
{
	static std::mt19937 random_engine;
	static std::uniform_int_distribution<int> random_chars(0, 9);

	try
	{
		std::cout << "Server application" << std::endl;
		for (;;)
		{
			boost::system::error_code error;

			if (error == boost::asio::error::eof)
				break; // Connection closed cleanly by peer.
			else if (error)
				throw boost::system::system_error(error); // Some other error.

			int message = number * 10 + random_chars(random_engine);
			std::cout << sock.remote_endpoint().address().to_string() << ":" 
					  << sock.remote_endpoint().port() << ": " << message << std::endl;
			
			boost::asio::write(sock, boost::asio::buffer(&message, sizeof message));

			std::this_thread::sleep_for(std::chrono::seconds(1));
		}
	}
	catch (std::exception& e)
	{
		std::cerr << "Exception in thread: " << e.what() << "\n";
	}
}

void server(boost::asio::io_context& io_context, unsigned short port)
{
	int number = 1;
	tcp::acceptor a(io_context, tcp::endpoint(tcp::v4(), port));
	for (;;)
	{
		std::thread(session, a.accept(), number).detach();
		number++;
	}
}

int main(int argc, char* argv[])
{
	try
	{
		if (argc != 2)
		{
			std::cerr << "Usage: blocking_tcp_echo_server <port>\n";
			return 1;
		}

		boost::asio::io_context io_context;

		server(io_context, std::atoi(argv[1]));
	}
	catch (std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << "\n";
	}

	return 0;
}