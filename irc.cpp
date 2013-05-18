//IRC.cpp//
// IRC class implementation file.
// Lucas McIntosh
// 04/07/2011

#include "IRC.h"

using boost::asio::ip::tcp; // IRC uses TCP connect type, so no need to have to type out the full namespace address.

namespace IRC{bool DebugIRC = true;} // Debug Output switch.

IRC::IRC::IRC(std::string server, std::string port, boost::asio::io_service& io_service) : server_(server), port_(port), io_service_(io_service), resolver_(io_service_), socket_(io_service_), active_(true)
{
	tcp::resolver::query query(server_, port_);
	tcp::resolver::iterator endpoint_iterator = resolver_.resolve(query);
	connect_to_host(endpoint_iterator);
}

/// Creates a Connection ///
void IRC::IRC::connect_to_host(tcp::resolver::iterator endpoint_iterator)
{
	if (DebugIRC)
		std::cout << "asyncConnectToHost()." << std::endl;
	tcp::endpoint endpoint = *endpoint_iterator;
	socket_.close();
	if(DebugIRC)
		std::cout << "Connecting..." << std::endl;
	//boost::asio::async_connect(socket_, endpoint_iterator++, boost::bind(&IRC::handle_connect, this, boost::asio::placeholders::error));
	socket_.async_connect(endpoint, boost::bind(&IRC::connect_complete, this, boost::asio::placeholders::error, ++endpoint_iterator));
	if(DebugIRC)
		std::cout << "Test." << std::endl;
}

void IRC::IRC::connect_complete(const boost::system::error_code& error,tcp::resolver::iterator endpoint_iterator)
{
	if(DebugIRC)
		std::cout << "handle_connect()" << std::endl;
	if(!error)
	{
		if(DebugIRC)
			std::cout << "Connection Established, reading in from server."  << std::endl;
		start_read();
	}
	else if(endpoint_iterator != tcp::resolver::iterator())
	{
		if(DebugIRC)
			std::cout << "Failed to connect, trying next resolved." << std::endl;
		std::cerr << "Error: " << error.message() << std::endl;
		do_close();
		connect_to_host(endpoint_iterator);
	}
}

/// Read in a message ///
void IRC::IRC::start_read()
{
	if(DebugIRC)
		std::cout << "Receiving async message from server." << std::endl;
	// Receives the message from socket_ into a buffer, as an asynchronous operation.
	// Uses boost::bind to bind placeholder error codes to pass to IRC::handler, which it does not call.
	//boost::asio::async_read(socket_, boost::asio::buffer(array_read_msg_), boost::bind(&IRC::handle_read, this,boost::asio::placeholders::error, bytes_transferred));	 
	socket_.async_read_some(boost::asio::buffer(array_read_msg_, 512), boost::bind(&IRC::handle_read, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
}

void IRC::IRC::handle_read(const boost::system::error_code& error,const size_t bytes_transferred)
{
	bytes_transferred_=bytes_transferred;
	if(DebugIRC)
	{
			std::cout << "Handling read."<< std::endl;
			std::cout << "Bytes Transferred: " << bytes_transferred << std::endl;
			std::cout << "bytes_transferred_: " << bytes_transferred_ << std::endl;
	}
	if(!error)
	{
		if(DebugIRC)
			std::cout << "Moving message from array to string."<< std::endl;
		str_read_msg_ = "";
		for(unsigned int x = 0; x != bytes_transferred; x++)
		{
			str_read_msg_ += array_read_msg_[x];
			if(DebugIRC)
			std::cout << array_read_msg_[x];
		}
		std::cout << std::endl;
		if(!PingPong())
		{
		}
		start_read();
	}
	else
	{
		std::cerr << "Error: " << error.message() << std::endl;
		do_close();
	}
}

/// Sends a message. ///
void IRC::IRC::write(const std::string msg) // pass the write data to the do_write function via the io service in the other thread
{
	io_service_.post(boost::bind(&IRC::do_write, this, msg));
}
	
void IRC::IRC::do_write(const std::string msg)
{
	if(DebugIRC)
		std::cout << "do_write()" << std::endl;
	bool write_in_progress = !write_msg_.empty();
	if(msg.size() > 510)
		throw std::exception("Message is too long");
	std::string buffered_message = msg + " \n\r"; // All IRC messages must end in \n\r, this appends it to the message.
	write_msg_.push_back(buffered_message);
	if(!write_in_progress)
		write_start();
}

void IRC::IRC::write_start(void)
{
	if(DebugIRC)
		std::cout << "write_start()" << std::endl;
	// Places the message into a buffered state that async_write is then able to send through socket_.
	// Uses boost::bind to bind placeholder error codes to pass to IRC::handler.
	boost::asio::async_write(socket_,
		boost::asio::buffer(write_msg_.front().data(), write_msg_.front().length()), 
		boost::bind(&IRC::write_complete, this,boost::asio::placeholders::error));
}

void IRC::IRC::write_complete(const boost::system::error_code& error)
{
	if(DebugIRC)
		std::cout << "write_complete()" << std::endl;
	if(!error)
	{
		write_msg_.pop_front();
		if(!write_msg_.empty())
			write_start();
	}
	else
	{
		std::cerr << "Error: " << error.message() << std::endl;
		do_close();
	}
}

/// Close the socket ///
void IRC::IRC::close() // call the do_close function via the io service in the other thread
{
	io_service_.post(boost::bind(&IRC::do_close, this));
}

void IRC::IRC::do_close()
{
	socket_.close();
	active_ = false;
}

/// Time Delay. ///
void IRC::IRC::waiter()
{
	io_service_.post(boost::bind(&IRC::do_wait, this));
}

void IRC::IRC::do_wait()
{
	std::cout << "Waiting..." << std::endl;
	boost::asio::deadline_timer t(io_service_, boost::posix_time::seconds(4)); // Creates a timer on the io_service_ and sets it's length.
	t.wait(); // assigns work to the io_service_.
}

/// Misc Functions ///
bool IRC::IRC::PingPong()
{
	if(DebugIRC)
		std::cout << "Entering Ping Checker." << std::endl;

	std::string ping = str_read_msg_.substr(0,4); // Extracting first four char from message. This is where PING resides.
	const std::string PING = "PING";
	if(ping == PING) // Checks if the server has sent a PING
	{
		std::string server = str_read_msg_.substr(7,bytes_transferred_-4);
		std::string message = "PONG :" + server;
		std::cout << "Ping? Pong!" << std::endl;
		if(DebugIRC)
			std::cout << message << std::endl;
		write(message);
		return true;
	}
	else if(DebugIRC) std::cout << "Server did not Ping." << std::endl;
	return false;
}