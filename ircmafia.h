//ircMafia.h//
// The game logic file in the ircMafia Bot Project. 

//          Copyright Lucas McIntosh 2011 - 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)


#pragma once
#ifndef __LMC_IRCMAFIA_H__
#define __LMC_IRCMAFIA_H__

#include "player.h"
#include "timer.hpp"
#include <ctime>
#include <string>
#include <list>
#include <map>
#include <vector>
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
    IRC(std::string /*server*/, std::string /*port*/, boost::asio::io_service&, std::fstream& /*debugfile*/, std::fstream& /*errorfile*/); 
    void waiter(void);
    bool active(void){return active_;}
    void write(const std::string);
    void close(void);
    bool PingPong(void);
  private:
    void do_write(const std::string);
    void work_string_message(void);
    void connect_to_host(tcp::resolver::iterator);
    void connect_complete(const boost::system::error_code&,tcp::resolver::iterator);
    void start_read(void);
    void handle_read(const boost::system::error_code& /*error*/,const size_t /*bytes_transferred*/);
    void write_start(void); 
    void write_complete(const boost::system::error_code&);
    void do_close(void); 
    void do_wait(void);
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
    /*
    # IRC Class should end here,
    # look for a way to seperate classes
    # and pass IRC by reference to Mafia Game class.
    */

    /// Mafia Game Code ///
  public:
    void gameMain(void);
    void Commands(void);	
    void removeDead(void); 	
    void createPlayers(void);
    void joinPlayer(const std::string);
    void createChannels(void);
    void leaveChannels(void);
    void nightActions(void);
    void dayActions(void);
    void promoteMob(void);
    void promoteCop(void);
    void cleanUp(void);
  protected:
    int stop_count_;
    int player_count_; 
    int skip_count_;
    std::vector<std::string> signupList_;
    std::list<Mafia::Player> playerList_;
    std::map<std::string, std::string> voteList_;
    std::string channel_;
    std::string channel_mafia_;
    std::string channel_cops_;
    bool game_over_;
    bool day_phase_;
    bool night_phase_;
    bool game_active_;
    bool signup_phase_;
    int townie_count_; 
    int mafia_count_;
    int godfather_count_; // used for promoting Mob to Godfather.
    int doctor_count_;
    int cop_count_;
    int officer_count_; // used for promoting Cop to Officer.
  };

  class SignUpTimer : public timer::Timer
  {
    std::string ms_message, ms_chan;
  public:
    SignUpTimer(double seconds):Timer(seconds){}
    virtual void Done() {}	
  };
}
#endif