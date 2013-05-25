///IRC.H///
// Lucas McIntosh
// 30/11/2012
// An IRC Client class
// Version 2.0

//          Copyright Lucas McIntosh 2011 - 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)


#ifndef _LMC_IRC_H_
#define _LMC_IRC_H_

#include <string>
#include <deque>
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>

using boost::asio::ip::tcp;

namespace IRC
{
  class IRC
  {
    /// IRC Code ///
  public:
    IRC(std::string server, std::string port, boost::asio::io_service&, std::fstream& debugfile, std::fstream& errorfile); 
    void waiter(void);
    bool active(void){return active_;}
    void write(const std::string);
    void read();
    void close(void);
    bool PingPong(void);
  private:
    void do_write(const std::string);
    void work_string_message(void);
    void connect_to_host(tcp::resolver::iterator);
    void connect_complete(const boost::system::error_code&,tcp::resolver::iterator);
    void start_read(void);
    void handle_read(const boost::system::error_code& error,const size_t bytes_transferred);
    void write_start(void); 
    void write_complete(const boost::system::error_code&);
    void do_close(void); 
    void do_wait(void);
    virtual void commands() = 0;
  protected:
    std::fstream& debugfile_;
    std::fstream& errorfile_;
    bool connect_complete_;
    bool active_;
    boost::asio::io_service& io_service_;
    tcp::resolver resolver_;
    tcp::socket socket_;
    std::string server_;
    std::string port_;
    size_t bytes_transferred_;
    boost::array<char, 512> array_read_msg_;
    std::string msg_nick_;
    std::string msg_nick2_;
    std::string msg_channel_;
    std::string msg_action_;
    std::string msg_msg_;
    std::string str_read_msg_;
    std::deque<std::string> write_msg_;	
    tcp::resolver::iterator endpoint_iterator_;
  };
}
#endif