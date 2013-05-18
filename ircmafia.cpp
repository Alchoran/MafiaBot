//ircMafia.cpp//
// Lucas McIntosh
// 09/08/2011
// The game logic file in the ircMafia Bot Project.

//          Copyright Lucas McIntosh 2011 - 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)


#include "ircmafia.h"
#include <fstream>
#include <iostream>
#include <boost/regex.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>

#define DEBUGMAFIA
#define DEBUGIRC
#define MAFIAALPHA
//#define MAFIABETA

namespace IRC{
  boost::random::mt19937 gen(static_cast<uint32_t>(std::time(0)));/*Seed generator for random function.*/
}

typedef IRC::SignUpTimer dayTimer;
typedef IRC::SignUpTimer nightTimer;
typedef std::list<Mafia::Player>::iterator iterator_l;

//// IRC client code ////

using boost::asio::ip::tcp; // IRC only uses TCP connect type, so no need to have to type out the full namespace address.

/// IRC Constructor ///
IRC::IRC::IRC(std::string server, std::string port, boost::asio::io_service& io_service, std::fstream& debfil, 
              std::fstream& errfil) : server_(server), port_(port), io_service_(io_service), resolver_(io_service_),
              socket_(io_service_), active_(true), game_active_(false), connect_complete_(false), stop_count_(0),
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
          Commands();
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
    throw std::exception("Message is too long");
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

//// Mafia Game Functions ////
void IRC::IRC::gameMain(void){
  if(connect_complete_){
    // Makes sure that the program has finished connecting.
    //Issues with multiple threads was causing gameMain() to be called during program startup.
#ifdef DEBUGIRC
    std::cout << "gameMain()" << std::endl;
#endif

    channel_ = "#mafia1";
    std::string channel = channel_; // Confines the game to the channel, prevents games from being run in pm.
    // Resets all the game containers to 0.
    player_count_ = 0;
    game_active_ = true;
    signupList_.clear();
    playerList_.clear();
    player_count_ = 0;
    stop_count_ = 0;
    bool done = false;
    bool sent_signup1 = false;
    bool sent_signup2 = false;
    bool mafia_win = false;
    bool town_win = false;
    night_phase_ = false;
    day_phase_ = false;
    game_over_ = false;
    signup_phase_ = true;

    while(game_active_){
      // Game's starting message.
#ifdef MAFIAALPHA
      write("privmsg " + channel + " :IRCMafia is using ALPHA rule set.");
#endif
#ifdef MAFIABETA
      write("privmsg " + channel + " :IRCMafia is using BETA rule set.");
#endif
      write("privmsg " + channel + " :IRCMafia is currently in testing phase. Please expect bugs.");
      write("privmsg " + channel + " :Signup is now starting! Type !join to join.");
      write("privmsg " + channel + " :Signups will last 60 seconds.");

      /// Signup Phase ///
      SignUpTimer timer1(60);
      while(!done && game_active_) {
        if(timer1.elapsed() == int(0))
          start_read();
        done = timer1.poll();
        if(timer1.elapsed() == float(30.00) && !sent_signup1){
          write("privmsg " + channel + " :Hurry up! There are only 30 more seconds left");
          sent_signup1=true;
        }
        if(timer1.elapsed() == int(45) && !sent_signup2){
          write("privmsg " + channel + " :Hurry up! There are only 15 more seconds left");
          sent_signup2 = true;
        }
      }

      if(!game_active_){ // Checks to see if !stop was used during the Signup Phase.
        std::cout << "Game was stopped!";
        write("privmsg " + channel + " :Game was stopped!");
        game_over_ = true;
        break;
      }

      signup_phase_ = false;
      /// Role Assignment Phase ///
#if defined(MAFIAALPHA) //  In Alpha mode, only One player is required to start the game.
      if(player_count_ == 0){
        write("privmsg " + channel + " :Too few players");
        game_active_ = false;
        return;
      }
#elif defined(MAFIABETA) // In Beta mode, Five players is the minimum for a game.
      if(player_count_ < 3){
        write("privmsg " + channel_ + " :Too few players");
        game_active_ = false;
        return;
      }
#else
      if(player_count_ < 5){
        write("privmsg " + channel_ + " :Too few players");
        game_active_ = false;
        return;
      }
#endif
      if(game_active_){ // Makes sure the game is still running before proceeding.
        write("privmsg " + channel + " :Creating Players!");
        createPlayers();
        write("privmsg " + channel + " :*******************************");
        Sleep(100);
        write("privmsg " + channel + " :**        Game Starting!     **");
        Sleep(100);
        write("privmsg " + channel + " :*******************************");
        Sleep(100);
        write("privmsg " + channel + " :**          Players:         **");
        Sleep(100);
        write("privmsg " + channel + " :*******************************");
        Sleep(100);
        std::string message_playerlist;
        for(int x=1; x<=player_count_;x++){
          for(iterator_l it = playerList_.begin(); it != playerList_.end(); it++){
            if(it->ID()==x){
              message_playerlist += boost::lexical_cast<std::string>(it->ID()) + ". " + it->Nick() + "  ";
              write("privmsg " + it->Nick() + " :Your role is " + it->Role());
              break;
            }
          }
        }
        write("privmsg " + channel + " :**" + message_playerlist + "**");
      }
      /// Game Loop ///
      createChannels();
      while(!game_over_){
        Sleep(100);
        write("privmsg " + channel_ + " :*******************************");
        Sleep(10);
        write("PRIVMSG " + channel_ + " :**   It is now Night Time.   **");
        Sleep(10);
#ifdef MAFIAALPHA
        write("PRIVMSG " + channel_ + " :** It will last 60 seconds. **");
#endif
#ifdef MAFIABETA
        write("PRIVMSG " + channel_ + " :** It will last 120 seconds. **");
#endif
        Sleep(10);
        write("privmsg " + channel_ + " :*******************************");
        /// Night Phase ///
        night_phase_ = true;
        day_phase_ = false;
#ifdef MAFIAALPHA
        nightTimer timer2(60);
#endif
#ifdef MAFIABETA
        nightTimer timer2(120);
#endif
        bool night_done = false;
        bool sent_minute = false;
        bool sent_two_minute = false;
        while(!night_done && night_phase_){
          if(timer2.elapsed() == 0){
            start_read();
          }
          /* if(timer2.elapsed() == double(60.0) && !sent_minute){
          write("privmsg " + channel_ + " :90 seconds left!");
          sent_minute = true;
          }
          if(timer2.elapsed() == double(120.0) && !sent_two_minute){
          write("privmsg " + channel_ + " :30 seconds left!");
          sent_two_minute = true;
          }*/
          night_done = timer2.poll();
        }
        write("privmsg " + channel_ + " :*******************************");
        Sleep(10);
        write("PRIVMSG " + channel_ + " :**    Nightphase is Over!    **");
        Sleep(10);
        write("privmsg " + channel_ + " :*******************************");
        Sleep(10);
        nightActions();
        removeDead();
        game_over_ = false;
        day_phase_ = true;
        if(mafia_count_ >= townie_count_){
          day_phase_ = false;
          game_over_ = true; /*
                             Switch used when victory conditions have been met:
                             I) If the Mafia outnumber or have killed all the Townies.
                             II) If the Townies have killed all the Mafia.
                             III) Future Conditions.
                             */
          game_active_= false;
          write("PRIVMSG " + channel_ + " :Mafia Wins! Congrats!");
        }
        else if(mafia_count_ == 0){
          game_over_ = true;
          day_phase_ = false;
          game_active_= false;
          write("PRIVMSG " + channel_ + " :Town Wins! Congrats!");
        }
        else{
          game_over_ = false;
        }
        night_phase_ = false;

        /// Day Phase ///
        if(day_phase_ && !game_over_){
          write("privmsg " + channel_ + " :*******************************");
          Sleep(10);
          write("PRIVMSG " + channel_ + " :**    It is now Day Time.    **");
          Sleep(10);
#ifdef MAFIAALPHA
          write("PRIVMSG " + channel_ + " :** It will last 60 seconds. **");
#endif
#ifdef MAFIABETA
          write("PRIVMSG " + channel_ + " :** It will last 150 seconds. **");
#endif
          Sleep(10);
          write("privmsg " + channel_ + " :*******************************");
          day_phase_ = true;
          bool day_done = false;
#ifdef MAFIAALPHA
          dayTimer timer3(60);
#endif
#ifdef MAFIABETA
          dayTimer timer3(150);
#endif
          sent_minute = false;
          sent_two_minute = false;
          while(!day_done){
            if(timer3.elapsed() == 0){
              start_read();
            }
#ifdef MAFIABETA
            if(timer3.elapsed() == double(60.0) && !sent_minute){
              write("privmsg " + channel_ + " :90 seconds left!");
              sent_minute = true;
            }
            if(timer3.elapsed() == double(120.0) && !sent_two_minute){
              write("privmsg " + channel_ + " :30 seconds left!");
              sent_two_minute = true;
            }
#endif
            day_done = timer3.poll();
          }
          day_phase_ = false;
          write("privmsg " + channel_ + " :*******************************");
          Sleep(10);
          write("PRIVMSG " + channel_ + " :**     Day Time is Over!     **");
          Sleep(10);
          write("PRIVMSG " + channel_ + " :**   Voting is now closed.   **");
          Sleep(10);
          write("privmsg " + channel_ + " :*******************************");
          Sleep(10);
          dayActions();
          removeDead();
          if(mafia_count_ >= townie_count_){
            game_over_ = true;
            game_active_= false;
            write("PRIVMSG " + channel_ + " :Mafia Wins! Congrats!");
          }
          if(mafia_count_ == 0){
            game_over_ = true;
            game_active_= false;
            write("PRIVMSG " + channel_ + " :Town Wins! Congrats!");
          }
        }			
      }
    }
    cleanUp();
    //leaveChannels();
  }
  game_active_ = false;
}

void IRC::IRC::cleanUp(void) // Cleans up all game assets.
{
  mafia_count_ = 0;
  godfather_count_ = 0;
  officer_count_ = 0;
  doctor_count_ = 0;
  townie_count_ = 0;
  cop_count_ = 0;
  signupList_.clear();
  playerList_.clear();
  voteList_.clear();
  game_over_ = false;
  day_phase_ = false;
  night_phase_ = false;
  signup_phase_ = false;
  game_active_ = false;
  stop_count_ = 0;
  player_count_ = 0;
  skip_count_ = 0;	
  leaveChannels();
  debugfile_ << "\n CLEANUP() HAS BEEN SUCCESSFUL.\n";
  std::cout << "\n CLEANUP() HAS BEEN SUCCESSFUL.\n";
}

void IRC::IRC::Commands(void){
  /*
  This function is called whenever a message is received on the socket.
  It parses the message for specific keywords used as control commands.
  Raw text has already been processed in work_string_message() and has been
  broken down into nick, action(PRIVMSG or NICK), #channel, and then message body.
  The member objects that hold message body, channel, nick and action are assigned to local objects for concurrency sake.
  */
  std::string message, channel, nick = msg_nick_;
  std::string action = msg_action_;
  std::cout << "msg_action_:" << msg_action_ <<  " msg_channel_:" << msg_channel_ <<  " msg_nick_:" << msg_nick_ << std::endl;
  if(action == "NICK"){ 
    std::string nick_change = msg_channel_;
    if(game_active_){
      for(iterator_l nickIter = playerList_.begin();nickIter!=playerList_.end();nickIter++){
        if(nick==nickIter->Nick()){
          nickIter->setNick(nick);
        }
      }
    }
  }
  if(action == "PRIVMSG"){
    message = msg_msg_;
    channel = msg_channel_;
    std::string command, command_subject;
    if(message.length() < 20)
      std::cout << message << std::endl;
    boost::regex command_test("^(!\\S+)\\s?(\\S*).*"); // !(not a space) space (not a space) drop the rest
    boost::smatch what;	
    if(boost::regex_match(message, what, command_test)) {
      command = what[1];
      command_subject=what[2];
      std::cout << command << " " << command_subject << std::endl;
    }
    else{
      std::cout << "\nCommand failed\n";
    }
    if(!game_active_){
      channel_=channel;
    }

    /// The following are the commands that can be used, and any conditions that surround their usage. ///
    if(command=="!start"){
#ifdef DEBUGMAFIA
      std::cout << "!start command used." << std::endl;
#endif
      if(!game_active_){
        if(channel==nick){
          write("privmsg " + nick + " this command can only be used in a channel");
        }
        else{
          channel_=channel;
          gameMain();
        }
      }
      else if(game_active_){
        write("privmsg " + channel + " :Game has already started.");
      }
    }
    else if(command=="!stop"){
#ifdef MAFIAALPHA
      if(game_active_){
        stop_count_++;
        write("privmsg " + channel + " :!stop used. 1 stop needed to end game.");
#ifdef DEBUGMAFIA
        std::cout << "stop_count_: " << int(stop_count_) << std::endl;
#endif
        if(stop_count_ == 1){
          game_active_ = false;
          game_over_ = true;
          cleanUp();
        }
      }
#endif
#ifdef MAFIABETA
      if(game_active_ && game_over_){
        stop_count_++;
        write("privmsg " + channel + " :!stop used. 3 stops needed to end game.");
        if(DebugMafia){
          std::cout << "stop_count_: " << int(stop_count_) << std::endl;
        }
        if(stop_count_ >= 3 || nick == "Alchoran" || nick=="Zardvark"){
          game_active_ = false;
          game_over_ = true;
        }
      }
#endif
    }
    else if(command=="!skip") {} // Command used to skip phases.
    else if(command=="!join")
    {
#ifdef DEBUGMAFIA
      debugfile_ << "!join command used, entering joinPlayer()." << std::endl;
      std::cout << "!join command used, entering joinPlayer()." << std::endl;
#endif
      if(signup_phase_)
        joinPlayer(nick);
      else
        write("privmsg " + channel_ + " :Signups are over!");
    }

    if(game_active_){
      if(command=="!vote" || command=="!lynch"){
#ifdef MAFIAALPHA
        for(iterator_l iter = playerList_.begin();iter!=playerList_.end();++iter){
          if(iter->Nick()==nick){
            bool vote_true = false;
#ifdef DEBUGMAFIA
            std::cout << "\n" << nick << " has voted for " << command_subject << ".\n";
#endif
            for(iterator_l iter2=playerList_.begin();iter2!=playerList_.end();++iter2){
              if(command_subject==iter2->Nick()){
                iter->setVote(command_subject);
                vote_true = true;
                break;
              }
            }
            if(vote_true){
              write("privmsg " + channel_ + " :" + nick + " has voted for " + command_subject + ".");
            }
            else{
              write("privmsg " + channel_ + " :" + command_subject + " is not a player.");
            }
          }
        }
#endif
#ifndef MAFIAALPHA
        if(night_phase_)
          write("privmsg " + channel_ + " :" + nick + " voting is disabled in Night Phase.");
        else if(day_phase_)
        {
          for(iterator_l iter = playerList_.begin();iter!=playerList_.end();++iter)
          {
            if(iter->Nick()==nick)
            {
              bool vote_true = false;
              if(DebugMafia)
                std::cout << "\n" << nick << " has voted for " << command_subject << ".\n";
              for(iterator_l iter2=playerList_.begin();iter2!=playerList_.end();++iter2)
              {
                if(command_subject==iter2->Nick())
                {
                  iter->setVote(command_subject);
                  vote_true = true;
                  break;
                }
              }
              if(vote_true)
                write("privmsg " + channel_ + " :" + nick + " has voted for " + command_subject + ".");
              else
                write("privmsg " + channel_ + " :" + command_subject + " is not a player.");
            }
          }
        }
#endif
      }
      else if(command=="!heal" || command=="!save"){
#ifdef MAFIAALPHA // !heal command can be used anytime, for testing purposes

        for(iterator_l iter = playerList_.begin();iter!=playerList_.end();iter++){
          if(iter->Nick()==nick){
            if(iter->Role() == "Doctor"){
#ifdef DEBUGMAFIA
              std::cout << nick << " is healing " << command_subject << "." << std::endl;
#endif
              std::string player_name = "0";
              for(iterator_l iter2 = playerList_.begin();iter2!=playerList_.end();iter2++){
                if(command_subject == iter2->Nick()){
                  iter->setHeal(command_subject);
                  player_name = command_subject;
                  write("privmsg " + nick + " :You are healing " + command_subject + ".");
                }
              }
              if(player_name == "0"){
                write("privmsg " + nick + " :" + command_subject + " is not a player.");
              }
            }
            else{
              write("privmsg " + nick + " :Your role is not capable of this action.");
            }
          }
        }

#endif
#ifndef MAFIAALPHA

        if(night_phase_)
        {
          for(iterator_l iter = playerList_.begin();iter!=playerList_.end();iter++)
          {
            if(iter->Nick()==nick)
            {
              if(iter->Role() == "Doctor")
              {
                if(DebugMafia)
                  std::cout << nick << " is healing " << command_subject << "." << std::endl;
                std::string player_name = "0";
                for(iterator_l iter2 = playerList_.begin();iter2!=playerList_.end();iter2++)
                {
                  if(command_subject == iter2->Nick())
                  {
                    iter->setHeal(command_subject);
                    player_name = command_subject;
                    write("privmsg " + nick + " :You are healing " + command_subject + ".");
                  }
                }
                if(player_name == "0")
                  write("privmsg " + nick + " :" + command_subject + " is not a player.");
              }
              else
                write("privmsg " + nick + " :Your role is not capable of this action.");
            }
          }
        }
        else if(day_phase_)
          write("privmsg " + nick + " :This command can only be used at night.");
#endif
      }
      else if(command=="!kill")
      {
#ifdef MAFIAALPHA
        for(iterator_l iter = playerList_.begin();iter!=playerList_.end();iter++){
          if(iter->Nick()==nick){
            if(iter->Role() == "Godfather"){
#ifdef DEBUGMAFIA
              std::cout << nick << " is killing " << command_subject << std::endl;
#endif
              std::string player_name = "0";
              for(iterator_l iter2 = playerList_.begin();iter2!=playerList_.end();iter2++){
                if(command_subject == iter2->Nick()){
                  player_name = iter2->Nick();
                  iter->setWhack(command_subject);
                  write("privmsg " + nick + " : You are going to kill " + command_subject + ".");
                }
              }
              if(player_name == "0"){
                write("privmsg " + nick + " :" + command_subject + " is not a player!");
              }
<<<<<<< HEAD
            }
            else{
              write("privmsg " + nick + " : Your role is not capable of this action.");
            }
=======
            }
            else{
              write("privmsg " + nick + " : Your role is not capable of this action.");
            }
>>>>>>> master
          }
        }
#endif
#ifndef MAFIAALPHA
        if(night_phase_){
          for(iterator_l iter = playerList_.begin();iter!=playerList_.end();iter++){
            if(iter->Nick()==nick){
              if(iter->Role() == "Godfather"){
#ifdef DEBUGMAFIA
                std::cout << nick << " is killing " << command_subject << std::endl;
#endif
                std::string player_name = "0";
                for(iterator_l iter2 = playerList_.begin();iter2!=playerList_.end();iter2++){
                  if(command_subject == iter2->Nick()){
                    player_name = iter2->Nick();
                    iter->setWhack(command_subject);
                    write("privmsg " + nick + " : You are going to kill " + command_subject + ".");
                  }
                }
                if(player_name == "0"){
                  write("privmsg " + nick + " :" + command_subject + " is not a player!");
                }
              }
              else{
                write("privmsg " + nick + " :Your role is not capable of this action.");
              }
            }
          }
        }
        else
          write("privmsg " + nick + " :Your role is only capable of this at night time.");
#endif
      }
      else if(command=="!investigate" || command=="!check"){
#if defined(MAFIAALPHA)
        for(iterator_l iter = playerList_.begin();iter!=playerList_.end();iter++){
          if(iter->Nick()==nick){
            if(iter->Role() == "Officer"){
#ifdef DEBUGMAFIA
              std::cout << nick << " is investigating " << command_subject << "." << std::endl;
#endif
              std::string player_name = "0";
              for(iterator_l iter2 = playerList_.begin();iter2!=playerList_.end();iter2++){
                if(command_subject == iter2->Nick()){
                  iter->setInvestigate(command_subject);
                  player_name = command_subject;
                  write("privmsg " + nick + " :You are investigating " + command_subject + ".");
                }
              }
              if(player_name == "0"){
                write("privmsg " + nick + " :" + command_subject + " is not a player!");
              }
            }
            else{
              write("privmsg " + nick + " :Your role is not capable of this action.");
            }
          }
        }
#else
        if(night_phase_){					
          for(iterator_l iter = playerList_.begin();iter!=playerList_.end();iter++){
            if(iter->Nick()==nick){
              if(iter->Role() == "Officer"){
#ifdef DEBUGMAFIA
                std::cout << nick << " is investigating " << command_subject << "." << std::endl;
#endif
                std::string player_name = "0";
                for(iterator_l iter2 = playerList_.begin();iter2!=playerList_.end();iter2++){
                  if(command_subject == iter2->Nick()){
                    iter->setInvestigate(command_subject);
                    player_name = command_subject;
                    write("privmsg " + nick + " :You are investigating " + command_subject + ".");
                  }
                }
                if(player_name == "0"){
                  write("privmsg " + nick + " :" + command_subject + " is not a player!");
                }
              }
              else{
                write("privmsg " + nick + " :Your role is not capable of this action.");
              }
            }
          }
        }
        else{
          write("privmsg " + nick + " :This action can only be taken at night time.");
        }
#endif
      }
      else if(command=="!list" || command=="!players"){
        std::string message_playerlist;
        for(int x=0; x<=player_count_;x++){
          for(iterator_l it = playerList_.begin(); it != playerList_.end(); it++){
            if(it->ID()==x){
              message_playerlist += boost::lexical_cast<std::string>(it->ID()) + ". " + it->Nick() + "  ";
              break;
            }
          }
        }
        write("privmsg " + channel + " :**" + message_playerlist + "**");
      }
      else if(command=="!role"){
        std::string message_playerlist;
        iterator_l it=playerList_.begin();
        for(int x=0; x<player_count_;it++,x++){
          if(it->Nick()==nick){
            write("privmsg " + it->Nick() + " :Your role is: " + it->Role()); 
            break;
          }
        }
      }
    }
  }
}

void IRC::IRC::joinPlayer(const std::string nick){

#ifdef DEBUGMAFIA
  std::cout << "Inside joinPlayer()." << std::endl;
#endif

  std::string buffered_message;

  /*
  Checks to makesure that the game is still running.
  Possible Conditions for false:
  I) !stop was used to end the game.
  II) There were too few players.
  */
  if(!game_active_) 
  {
    buffered_message = "privmsg " + channel_ + " :The game is not running, use !start to start the game.";
#ifdef DEBUGMAFIA
    std::cout << buffered_message << std::endl;
#endif
    write(buffered_message);
  }
  else{ // If the game is still running it then checks to see if the player has already joined, if not then it adds their name to the signupList_.
    bool hasJoined = false;
    if(signupList_.empty()){
      buffered_message = "privmsg " + channel_ + " :" + nick + " has signed up!";
#ifdef DEBUGMAFIA
      debugfile_ <<  "List empty, adding first Nick: " << nick << std::endl;
      std::cout << "List empty, adding first Nick: " << nick << std::endl;
#endif
      write(buffered_message);
      signupList_.push_back(nick);
      player_count_++;
    }
    else{
#ifdef DEBUGMAFIA
      std::cout << "Looping through and checking if the Player has already joined." << std::endl;
#endif
      for(int x = 0; x != signupList_.size(); x++){
        if(signupList_[x]==nick){
          buffered_message = "privmsg " + channel_ + " :" + nick + " you have already joined!";
#ifdef DEBUGMAFIA
          std::cout << nick << " has already joined." << std::endl;
          std::cout << buffered_message << std::endl;
#endif
          write(buffered_message);
          hasJoined = true;
        }
      }
      if(!hasJoined){
        buffered_message = "privmsg " + channel_ + " :" + nick + " has signed up!";
#ifdef DEBUGMAFIA
        std::cout << "Player has not joined, adding nick to the list." << std::endl;
        std::cout << buffered_message << std::endl;
#endif
        write(buffered_message);
        signupList_.push_back(nick);
        player_count_ = player_count_ + 1;
#ifdef DEBUGMAFIA
        std::cout << "player_count_: " << player_count_ << std::endl;
#endif
      }
    }
  }
}

void IRC::IRC::createPlayers(void){
#ifdef DEBUGMAFIA
  std::cout << "Creating players." << std::endl;
#endif
  // Resets the role counters to 0.
  mafia_count_ = 0;
  godfather_count_ = 0;
  officer_count_ = 0;
  doctor_count_ = 0;
  townie_count_ = 0;
  cop_count_ = 0;

  // Formula for determining the number of mafia members based on how many signed up.
  int MAX_MAFIA = player_count_ / 3;
  if(MAX_MAFIA < 1)
    MAX_MAFIA = 1;
#ifdef DEBUGMAFIA
  std::cout << "MAX_MAFIA = " << int(MAX_MAFIA) << std::endl;
#endif
  // Determines how many of the players will have town sided roles.
  const int MAX_TOWNIE = player_count_ - MAX_MAFIA;
#ifdef DEBUGMAFIA
  std::cout << "MAX_TOWNIE = " << int(MAX_TOWNIE) << std::endl;
#endif
  // Determines how many of the townies will infact be cops.
  int MAX_COP = MAX_TOWNIE / 8;
  if(MAX_COP < 1)
    MAX_COP = 1;
#ifdef DEBUGMAFIA
  std::cout << "MAX_COP = " << int(MAX_COP) << std::endl;
#endif

  int x;
  int player_count = 0;
  boost::random::uniform_int_distribution<> rand(0,(player_count_-1)); // Random number generator function.

  while(player_count != player_count_){ // Role assignment loop.
#ifdef DEBUGMAFIA
    std::cout << "Player Count: " << player_count << " player_count_: " << player_count_ << std::endl;
#endif
    x = rand(gen);
#ifdef DEBUGMAFIA
    std::cout << "x = " << x << std::endl;
#endif
    // Checks to see if the player has already been assigned a role.
    bool player_created = false;
    for(iterator_l iter = playerList_.begin();iter != playerList_.end(); iter++){
      if(iter->Nick() == signupList_[x]){
        player_created = true;
      }
    }

    /*
    If they haven't been assigned a role, the program goes through a control structure.
    First by checking if the mafia family is full, if not then assigning roles from the family.
    Second, once the mafia family is full then by assigning the Police roles, followed by Doctor.
    Last, once all the special roles have been filled everyone left is given the role of Townie.
    */
    if(!player_created){
      if(mafia_count_ != MAX_MAFIA){
        if(godfather_count_ == 0){
          playerList_.push_back(Mafia::Player(signupList_[x], "Godfather", x+1));
#ifdef DEBUGMAFIA
          std::cout << "Godfather created: " << playerList_.back().Nick() << " " << playerList_.back().Role() << std::endl;
#endif
          mafia_count_++;
          godfather_count_ = 1;
          player_count++;
        }
        else{
          playerList_.push_back(Mafia::Player(signupList_[x], "Mob",  x+1));
#ifdef DEBUGMAFIA
          std::cout << "Mob created: " << playerList_.back().Nick() << " " <<  playerList_.back().Role() << std::endl;
#endif
          mafia_count_++;
          player_count++;
        }
      }
      else if(townie_count_ != MAX_TOWNIE){
        if(cop_count_ != MAX_COP){
          if(cop_count_ == 0){
            playerList_.push_back(Mafia::Player(signupList_[x], "Officer", x+1));
#ifdef DEBUGMAFIA
            std::cout << "Officer created: " << playerList_.back().Nick() << " " <<  playerList_.back().Role() << std::endl;
#endif
            cop_count_++;
            officer_count_ = 1;
            townie_count_++;
            player_count++;
          }
          else{
            playerList_.push_back(Mafia::Player(signupList_[x], "Cop", x+1));
#ifdef DEBUGMAFIA
            std::cout << "Cop created: " << playerList_.back().Nick() << " " <<  playerList_.back().Role() << std::endl;
#endif
            cop_count_++;
            townie_count_++;
            player_count++;
          }
        }
        else if(doctor_count_ == 0){
          playerList_.push_back(Mafia::Player(signupList_[x], "Doctor", x+1));
#ifdef DEBUGMAFIA
          std::cout << "Doctor created: " << playerList_.back().Nick() << " " <<  playerList_.back().Role() << std::endl;
#endif
          doctor_count_ = 1;
          townie_count_++;
          player_count++;
        }
        else
        {
          playerList_.push_back(Mafia::Player(signupList_[x], "Townie", x+1));
#ifdef DEBUGMAFIA
          std::cout << "Townie created: " << playerList_.back().Nick() << " " <<  playerList_.back().Role() << std::endl;
#endif
          townie_count_++;
          player_count++;
        }
      }
    }
#ifdef DEBUGMAFIA
    std::cout << "Exiting Creating Players" << std::endl;
#endif
  }
}

void IRC::IRC::createChannels(void){
  boost::random::uniform_int_distribution<> rand(0,10000);
  int rand_1_id = rand(gen);
  int rand_2_id = rand(gen);
  std::string chan_1_id = "#mafia" + boost::lexical_cast<std::string>(rand_1_id);
  std::string chan_2_id = "#cop" + boost::lexical_cast<std::string>(rand_2_id);
#if defined(MAFIADEBUG)
  std::cout << "channel mafia: " << chan_1_id << std::endl;
  std::cout << "channel cop: " << chan_2_id << std::endl;
#endif
  channel_mafia_ = chan_1_id;
  channel_cops_ = chan_2_id;
  write("JOIN " + channel_mafia_);
  write("JOIN " + channel_cops_);
  Sleep(100);
  write("mode " + channel_mafia_ + " +inpst");
  write("mode " + channel_cops_ + " +inpst");
  Sleep(100);
  for(iterator_l iter = playerList_.begin(); iter != playerList_.end(); iter++){
    Sleep(100);
    if(iter->Role() == "Cop" || iter->Role() == "Officer"){
      std::string message = "INVITE " + iter->Nick() + " " + channel_cops_;
      write(message);
      std::cout << message << std::endl;
    }
    if(iter->Role() == "Mob" || iter->Role() == "Godfather"){
      std::string message = "INVITE " + iter->Nick() + " " + channel_mafia_;
      write(message);
      std::cout << message << std::endl;
    }
  }
}

void IRC::IRC::leaveChannels(void)
{
  write("PART " + channel_mafia_);
  write("PART " + channel_cops_);
}

void IRC::IRC::nightActions(void)
{
#ifdef DEBUGMAFIA
  std::cout << "nightActions()" << std::endl;
#endif
  std::string kill_target="0";
  std::string heal_target="0";
  std::string investigate_target="0";
  std::string target_="0";
  std::string inv_role;
  std::string kill_role;
  iterator_l iter;
  bool healed = false;

  for(iter = playerList_.begin(); iter != playerList_.end(); iter++){
    if(iter->Role() == "Godfather")
      kill_target = iter->Whack();
    if(iter->Role() == "Officer")
      investigate_target = iter->Investigate();
    if(iter->Role() == "Doctor")
      heal_target = iter->Heal();
  }
  for(iter = playerList_.begin(); iter != playerList_.end(); iter++){
    if(iter->Nick()==heal_target){
      iter->setHealed();
    }
  }
#ifdef DEBUGMAFIA
  std::cout << "Kill target: " << kill_target << " Heal target: " << heal_target << " Investigate target: " << investigate_target << "." << std::endl;
#endif
  if(heal_target == kill_target && heal_target != "\0"){
    healed = true;
  }
  for(iter = playerList_.begin(); iter != playerList_.end(); iter++){
    if(investigate_target == iter->Nick()){
      if(iter->Role() == "Godfather"){
        inv_role = "Townie";
      }
      else{
        inv_role = iter->Role();
      }
    }
  }
  if(investigate_target != "0"){
    for(iter = playerList_.begin(); iter != playerList_.end(); iter++){
      if(iter->Role()=="Officer" || iter->Role()=="Cop"){
#ifdef DEBUGMAFIA
        std::cout << "privmsg " + iter->Nick() + " :" + investigate_target + "'s role is " + inv_role + "." << std::endl;
#endif
        write("privmsg " + iter->Nick() + " :" + investigate_target + "'s role is " + inv_role + ".");
      }
    }
  }

  if(healed){
    write("privmsg " + channel_ + " :" + heal_target + " was walking down the street when they felt an impact on their chest. They looked down and saw that they had been shot. As the darkness closed in, "
      + heal_target + " felt someone at their side. " + heal_target + " awoke to see the Doctor over them. 'You're safe now' the Doctor said.");
  }
  else{
    for(iter = playerList_.begin(); iter != playerList_.end(); iter++){
      if(kill_target == iter->Nick()){
        iter->setDeath();
        kill_role = iter->Role();
      }
    }
    if(kill_target != "0"){
      write("privmsg " + channel_ + " :" + kill_target + " was walking down the street when they felt an impact on their chest. They looked down and saw that they had been shot. "
        + kill_target + " fell down in a quickly growing pool of their own blood." + kill_target + " the " + kill_role + " is dead.");
      //aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
    }
    else{
      write("privmsg " + channel_ + " :The Mafia laid low this night");
    }
  }
  for(iter = playerList_.begin(); iter != playerList_.end(); iter++)
  {
    if(iter->Role() == "Godfather"){
      iter->clearWhack();
    }
    if(iter->Role() == "Officer"){
      iter->clearInvestigate();
    }
    if(iter->Role() == "Doctor"){
      iter->clearHeal();
    }
  }
}

void IRC::IRC::dayActions(void){
#ifdef DEBUGMAFIA
  std::cout << "dayActions()" << std::endl;
  debugfile_ << "dayActions()" << std::endl;
#endif
  unsigned int x=0;
  unsigned int y=0;
  bool tie = false;
  std::string voted;
  iterator_l iterPL;
  std::list<Mafia::Player>::const_iterator lynch;
  iterator_l lynched;

#ifdef DEBUGMAFIA
  for(iterPL=playerList_.begin();iterPL!=playerList_.end();iterPL++){
    std::cout << iterPL->Nick() << " is voting for " << iterPL->Voted() << std::endl;
    debugfile_ << iterPL->Nick() << " is voting for " << iterPL->Voted() << std::endl;
  }
#endif
  for(iterPL=playerList_.begin();iterPL!=playerList_.end();iterPL++){
    voted=iterPL->Voted();
    for(iterator_l iter=playerList_.begin();iter!=playerList_.end();iter++){
      if(voted==iter->Nick()){
        iter->incrVoted();
        break;
      }
    }
  }
  for(iterPL=playerList_.begin();iterPL!=playerList_.end();iterPL++){
    if(iterPL==playerList_.begin()){
      lynch=iterPL;
    }
    else{
      if(iterPL->numVotes()>lynch->numVotes()){
#ifdef DEBUGMAFIA
        std::cout << "iterPL numVotes:" << iterPL->numVotes() << " lync numVotes:" << lynch->numVotes() << std::endl;
#endif
        lynch=iterPL;
      }
      else if(iterPL->numVotes()==lynch->numVotes()){
#ifdef DEBUGMAFIA
        std::cout << "There is a tie!" << std::endl;
#endif
        tie=true;
      }
    }
  }
  if(tie){
    write("privmsg " + channel_ + " :There is a tie! There will be no lynch this day.");
  }
  else{
    for(lynched=playerList_.begin();lynched!=playerList_.end();lynched++){
      if(lynched->Nick()== lynch->Nick()){
        break;
      }
    }
    lynched->setDeath();
    write("privmsg " + channel_ + " :" + lynched->Nick() + " has been lynched! A(n) " + lynched->Role() + " is no longer with us anymore.");
  }
  for(iterPL=playerList_.begin();iterPL!=playerList_.end();iterPL++){
    iterPL->clearVote();
    iterPL->clearVoted();
  }
}

void IRC::IRC::removeDead(void){
  iterator_l iter2;
#ifdef DEBUGMAFIA
  std::cout << "Removing dead." << std::endl;
  debugfile_ << "Removing dead." << std::endl;
  for(iterator_l iter1=playerList_.begin();iter1!=playerList_.end();iter1++)
  {
    std::cout << iter1->Nick() << " " << iter1->Role() << " " << iter1->ID() << " " << iter1->isDead() << std::endl;
    debugfile_ << iter1->Nick() << " " << iter1->Role() << " " << iter1->ID() << " " << iter1->isDead() <<  std::endl;
  }
#endif
  try{
    for(iterator_l iter=playerList_.begin();iter!=playerList_.end();++iter){
      if(iter->isDead()){
        if(iter->Role() == "Godfather"){
          godfather_count_ = 0;
          mafia_count_--;
          player_count_--;
          playerList_.erase(iter);
          if(player_count_)
            promoteMob();
        }
        else if(iter->Role() == "Officer"){
          officer_count_ = 0;
          townie_count_--;
          player_count_--;
          playerList_.erase(iter);
          if(player_count_){
            promoteCop();
          }
        }
        else if(iter->Role() == "Doctor"){
          doctor_count_ = 0;
          player_count_--;
          townie_count_--;
          playerList_.erase(iter);
        }
        else if(iter->Role() == "Townie"){
          townie_count_--;
          player_count_--;
          playerList_.erase(iter);
        }
        else if(iter->Role() == "Mob"){
          mafia_count_--;
          player_count_--;
          playerList_.erase(iter);
        }
        else if(iter->Role() == "Cop"){
          cop_count_--;
          player_count_--;
          playerList_.erase(iter);
        }
        iter=playerList_.begin();
      }
    }
  }
  catch(std::exception& e){
    std::cerr << e.what() << std::endl;
    errorfile_ << e.what() << std::endl;
  }
#ifdef DEBUGMAFIA
  try{
    for(iterator_l iter2=playerList_.begin();iter2!=playerList_.end();++iter2)
    {
      debugfile_ << iter2->Nick() << " " << iter2->Role() << " " << iter2->ID() << std::endl;
      std::cout << iter2->Nick() << " " << iter2->Role() << " " << iter2->ID() << std::endl;
    }
  }
  catch(std::exception& e){
    std::cerr << e.what() << std::endl;
    errorfile_ << e.what() << std::endl;
  }
#endif
}

void IRC::IRC::promoteMob(void){
#ifdef DEBUGMAFIA
  std::cout << "Promoting to Godfather." << std::endl;
#endif
  try{
    for(iterator_l iter=playerList_.begin();iter!=playerList_.end();iter++){
      if(godfather_count_==0 && player_count_ > 1){
        if(iter->Role() == "Mob"){
          iter->promoteRole("Godfather");
          write("privmsg " + iter->Nick() + " :You have been promoted to Godfather, congrats!");
          //write("privmsg " + channel_mafia_ + " :" + iter->Nick() + " has been promoted to Godfather, congrats!");
          godfather_count_ = 1;
#ifdef DEBUGMAFIA
          std::cout << iter->Nick() << " has been promoted." << std::endl;
          debugfile_ << iter->Nick() << " has been promoted." << std::endl;
#endif
        }
      }
    }
  }
  catch(std::exception& e) {
    std::cerr << e.what() << std::endl;
    errorfile_ << e.what() << std::endl;
  }
}

void IRC::IRC::promoteCop(void){
#ifdef DEBUGMAFIA
  std::cout << "Promoting to Officer." << std::endl;
  debugfile_ << "Promoting to Officer." << std::endl;
#endif
  try{
    for(iterator_l iter=playerList_.begin();iter!=playerList_.end();iter++){
      if(officer_count_==0 && player_count_ > 1){
        if(iter->Role() == "Cop"){
          iter->promoteRole("Officer");
          write("privmsg " + iter->Nick() + " :You have been promoted to Officer, congrats!");
          // write("privmsg " + channel_cops_ + " :" + iter->Nick() + " has been promoted to Officer, congrats!");
          officer_count_ = 1;
#ifdef DEBUGMAFIA
          std::cout << iter->Nick() << " has been promoted." << std::endl;
          debugfile_ << iter->Nick() << " has been promoted." << std::endl;
#endif
        }
      }
    }
  }
  catch(std::exception& e){
    std::cerr << e.what() << std::endl;
    errorfile_ << e.what() << std::endl;
  }
}