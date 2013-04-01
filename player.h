///Player.h///
/// Combined Player ///

//          Copyright Lucas McIntosh 2011 - 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#ifndef __LMC_PLAYER_H__
#define __LMC_PLAYER_H__

#include <string>


namespace Mafia{
	class Player{
	public:
		Player(std::string /*name*/, std::string /*role*/, int /*id*/);
		void clearVote();
    void clearVoted();
		void setDeath();
		void setVote(const std::string /*vote*/);
    void setNick(const std::string);
    void setHealed();
    void incrVoted();
		void promoteRole(const std::string);
    int numVotes()const;
		bool isDead() const;
		int ID() const;
		std::string Nick()const;
		std::string Role()const;
		std::string Voted()const;
	protected:
		std::string role_;
		std::string nick_;
		std::string vote_;
		bool dead_;
		bool voted_;
    bool healed;
		int id_;
    int votes_against_;

	public: // Godfather abilities
		void setWhack(const std::string /*target*/);
		std::string Whack() const;
		void clearWhack();
	protected:
		std::string whack_;

	public: // Officer abilities
		void setInvestigate(const std::string /*target*/);
		std::string Investigate() const;
		void clearInvestigate();
	protected:
		std::string investigate_;

	public: // Doctor abilities
		void setHeal(const std::string /*target*/);
		std::string Heal()const;
		void clearHeal();
	protected:
		std::string heal_;

	};
}
#endif