///Player.h///
// Lucas McIntosh
// 11/10/11
/// Combined Player ///
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