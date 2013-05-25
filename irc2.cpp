// Lucas McIntosh
// 09/08/2011
// IRC functions/procedures definition file
// Version 2.0


#include "irc2.h"
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <boost/regex.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>

//// IRC client code ////

using boost::asio::ip::tcp; // IRC only uses TCP connect type, so no need to have to type out the full namespace address.

/// IRC Constructor ///
IRC::IRC::IRC(std::string server, std::string port, boost::asio::io_service& io_service, std::fstream& debfil, 
              std::fstream& errfil) : server_(server), port_(port), io_service_(io_service), resolver_(io_service_),
              socket_(io_service_), active_(true), connect_complete_(false),
              debugfile_(debfil), errorfile_(errfil){
                tcp::resolver::query query(server_, port_);
                tcp::resolver::iterator endpoint_iterator = resolver_.resolve(query);
                connect_to_host(endpoint_iterator);
}

/// Connection Functions ///
void IRC::IRC::connect_to_host(tcp::resolver::iterator endpoint_iterator){
#ifdef DEBUGIRC
  std::cout << "connect_to_host()." << std::endl;
#endif
  tcp::endpoint endpoint = *endpoint_iterator;
  socket_.close();
#ifdef DEBUGIRC
  std::cout << "Connecting..." << std::endl;
#endif
  socket_.async_connect(endpoint, boost::bind(&IRC::connect_complete, this, boost::asio::placeholders::error, ++endpoint_iterator));
}

void IRC::IRC::connect_complete(const boost::system::error_code& error,
                                tcp::resolver::iterator endpoint_iterator){
#ifdef DEBUGIRC
  std::cout << "connect_complete()" << std::endl;
#endif
  if(!error) {
#ifdef DEBUGIRC
    std::cout << "Connection Established, reading in from server."  << std::endl;
#endif
    start_read();
  }
  else if(endpoint_iterator != tcp::resolver::iterator())  {
#ifdef DEBUGIRC
    std::cout << "Failed to connect, trying next resolved." << std::endl;
#endif
    std::cerr << "Connection Error: " << error.message() << std::endl;
    errorfile_ << "Connection Error: " << error.message() << std::endl;
    do_close();
    connect_to_host(endpoint_iterator);
  }
}

/// Message Receiving Functions ///
void IRC::IRC::read(){
  io_service_.post(boost::bind(&IRC::start_read, this));
}

void IRC::IRC::start_read(void){
#ifdef DEBUGIRC
  std::cout << "Receiving message from server." << std::endl;
#endif
  // Receives the message from socket_ into a buffer, as an asynchronous operation.
  // Uses boost::bind to bind placeholder error codes and the amount of bytes transferred to pass to IRC::handle_read.
  socket_.async_read_some(boost::asio::buffer(array_read_msg_, 512), boost::bind(&IRC::handle_read, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
}

void IRC::IRC::handle_read(const boost::system::error_code& error,const size_t bytes_transferred){
  if(active()){
    bytes_transferred_=bytes_transferred;

#ifdef DEBUGIRC
    std::cout << "Handling read."<< std::endl;
    std::cout << "Bytes Transferred: " << bytes_transferred << std::endl;
    debugfile_ << "Handling read." << std::endl;
    debugfile_ << "Bytes Transferred: " << bytes_transferred << std::endl;
#endif

    if(!error) {
      str_read_msg_ = ":";
      try{
        for(size_t x = 1; x <= bytes_transferred-2; x++){
          str_read_msg_ += array_read_msg_[x];
#ifdef DEBUGIRC
          debugfile_ << array_read_msg_[x];
#endif
        }
      }
      catch(std::exception& e){
        std::cerr << "handle_read error: " << e.what() << std::endl;
      }
      std::cout << "\n" << str_read_msg_ << "\n";
      if(bytes_transferred == 0)
        str_read_msg_ = " ";
      work_string_message();
#ifdef DEBUGIRC
      debugfile_ << "\nconnect_complete_ is " << bool(connect_complete_) << std::endl;
#endif
      if(connect_complete_){
        if(!PingPong()){
          commands();
        }
      }
      start_read();
      connect_complete_ = true;
    }
    else{
      std::cerr << "Handle_read Error: " << error.message() << std::endl;
      errorfile_ << "Handle_read Error: " << error.message() << std::endl;
      do_close();
    }
  }
}

void IRC::IRC::work_string_message(void){
  try{
    msg_nick_ = msg_action_ = msg_channel_ = msg_msg_ = "";
    std::string my_expression = "^[[.colon.]]?([^[.exclamation-mark.]]+)(\\S+@\\S+)\\s([\\S]+)\\s([\\S]*)\\s*:?(.*)\\r?\\n?$"; // start at the : and keep going till a ! for the nick, drop the hostmask, grab the action, channel then the message body after :
    boost::regex test(my_expression);
    boost::smatch what;
    std::string test_string = str_read_msg_;
    if(boost::regex_match(test_string, what, test)){
      msg_nick_ = what[1];
      msg_action_ = what[3];
      msg_channel_ = what[4];
      msg_msg_ = what[5];
    }
  }
  catch(std::exception& e){
    std::cerr << "work_string_message() Error: " << e.what() << std::endl;
    errorfile_ << "work_string_message() Error: " << e.what() << std::endl;
    close();
    errorfile_.close();
    debugfile_.close();
  }
}

/// Message Sending Functions ///
void IRC::IRC::write(const std::string msg){ // pass the write data to the do_write function via the io service in the other thread
  io_service_.post(boost::bind(&IRC::do_write, this, msg));
}

void IRC::IRC::do_write(const std::string msg){
#ifdef DEBUGIRC
  std::cout << "do_write()" << std::endl;
#endif

  bool write_in_progress = !write_msg_.empty();
  if(msg.size() > 510){
    throw std::runtime_error("Message is too long");
  }
  std::string buffered_message = msg + " \n\r"; // All IRC messages must end in \n\r, this appends it to the message.
  write_msg_.push_back(buffered_message);
  if(!write_in_progress)
    write_start();
}

void IRC::IRC::write_start(void){
#ifdef DEBUGIRC
  std::cout << "write_start()" << std::endl;
#endif
  // Places the message into a buffered state that async_write is then able to send through socket_.
  // Uses boost::bind to bind placeholder error codes to pass to IRC::handler.
  boost::asio::async_write(socket_,
    boost::asio::buffer(write_msg_.front().data(), write_msg_.front().length()), 
    boost::bind(&IRC::write_complete, this,boost::asio::placeholders::error));
}

void IRC::IRC::write_complete(const boost::system::error_code& error){
#ifdef DEBUGIRC
  std::cout << "write_complete()" << std::endl;
#endif
  if(!error){
    write_msg_.pop_front();
    if(!write_msg_.empty()){
      write_start();
    }
  }
  else{
    std::cerr << "Write Error: " << error.message() << std::endl;
    errorfile_ << "Write Error: " << error.message() << std::endl;
    close();
  }
}

/// Socket Closure Functions ///
void IRC::IRC::close(void){ // call the do_close function via the io service in the other thread
  io_service_.post(boost::bind(&IRC::do_close, this));
}

void IRC::IRC::do_close(void){
  socket_.close();
  active_ = false;
}

/// Time Delay ///
void IRC::IRC::waiter(void){
  io_service_.post(boost::bind(&IRC::do_wait, this));
}

void IRC::IRC::do_wait(void){
  std::cout << "Waiting..." << std::endl;
  boost::asio::deadline_timer t(io_service_, boost::posix_time::seconds(4)); // Creates a timer on the io_service_ and sets it's length.
  t.wait(); // assigns work to the io_service_.
}

/// Ping Checker Function ///
bool IRC::IRC::PingPong(void){
  bool result=false;
#ifdef DEBUGIRC
  debugfile_ << "Entering Ping Checker." << std::endl;
#endif
  if(connect_complete_){
    boost::regex ping_test("^[[.colon.]](\\S+)\\s[[.colon.]](.*)$");
    boost::smatch what;
    if(boost::regex_match(str_read_msg_, what, ping_test)){
      std::string ping = what[1]; // Extracting first four char from message. This is where PING resides.
      const std::string PING = "ING";
      if(ping == PING){ // Checks if the server has sent a PING
        // std::string server = str_read_msg_.substr(6); // Not currently working.
        std::string message = "PONG :" + what[2];
        std::cout << "Ping? Pong!" << std::endl;
#ifdef DEBUGIRC
        std::cout << message << std::endl;
#endif
        write(message);
        result=true;
      }
    }
#ifdef DEBUGIRC
    std::cout << "Server did not Ping." << std::endl;
#endif
  }
  return result;
}
