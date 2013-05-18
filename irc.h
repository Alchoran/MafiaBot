//IRC.H//
// IRC class header file.
// Lucas McIntosh
// 04/07/2011

#ifndef IRC_H
#define IRC_H

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/array.hpp>
#include <string>
#include <iostream>
#include <deque>

using boost::asio::ip::tcp;

namespace IRC
{
	class IRC
	{
	
	public:
		IRC(std::string /*server*/, std::string /*port*/, boost::asio::io_service&); 
		void waiter(void);
		bool active(void){return active_;}
		void write(const std::string);
		void close(void);
		bool PingPong(void);
	private:
		void do_write(const std::string);
		void connect_to_host(tcp::resolver::iterator);
		void connect_complete(const boost::system::error_code&,tcp::resolver::iterator);
		void start_read();
		void handle_read(const boost::system::error_code& /*error*/,const size_t /*bytes_transferred*/);
		void write_start(void); 
		void write_complete(const boost::system::error_code&);
		void do_close(); 
		void do_wait();
	protected:
		bool active_;
		boost::asio::io_service& io_service_;
		tcp::resolver resolver_;
		tcp::socket socket_;
		std::string server_;
		std::string port_;
		size_t bytes_transferred_;
		boost::array<char, 512> array_read_msg_;
		std::string str_read_msg_;
		std::deque<std::string> write_msg_;	
		tcp::resolver::iterator endpoint_iterator_;
	};

}
#endif