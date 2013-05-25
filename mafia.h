///MAFIA.H///
// Lucas McIntosh
// 30/11/2012
// Mafia Game logic header
// Version 2.0

#ifndef _LMC_MAIFA_H_
#define _LMC_MAFIA_H_

#include "irc2.h"
#include "player.h"
#include "timer.hpp"
#include <ctime>
#include <string>
#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <vector>
#include <deque>

using namespace IRC;

namespace Mafia{
	class Mafia:public IRC::IRC{
		public:
			Mafia(std::string server, std::string port, boost::asio::io_service& io_service, std::fstream& debfil, std::fstream& errfil);
			~Mafia();
			void commands(void);	
			void removeDead(void); 	
			void createPlayers(void);
			void joinPlayer(std::string);
			void createChannels(void);
			void leaveChannels(void);
			void nightActions(void);
			void dayActions(void);
			void promoteMob(void);
			void promoteCop(void);
			void cleanUp(void);
      void gameMain();
		protected:
			void doCommands(void);
			int stop_count_;
			int player_count_; 
			int skip_count_;
			std::vector<std::string> signupList_;
			std::list<Player*> playerList_;
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