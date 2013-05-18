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
    Player(std::string name, std::string role, int id);
    void clearVote();
    void clearVoted();
    void setDeath();
    void setVote(const std::string /*vote*/);
    void setNick(const std::string);
    void setHealed();
    void incrVoted();
    void promoteRole(const std::string);
    virtual void setNightAction(const std::string target){}
    virtual void clearNightAction(){}
    virtual std::string NightAction() const{}
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
  };

  class Mob:public Player{
  public: // Godfather abilities
    Mob(std::string name, std::string role, int id);
    virtual void setNightAction(const std::string target);
    virtual void clearNightAction();
    virtual std::string NightAction() const;
  protected:
    std::string whack_;
  };

  class Police:public Player{
  public: // Officer abilities
    Police(std::string name, std::string role, int id);
    virtual void setNightAction(const std::string target) ;
    virtual void clearNightAction();
    virtual std::string NightAction() const;
  protected:
    std::string investigate_;
  };

  class Doctor:public Player{
  public: // Doctor abilities
    Doctor(std::string name, std::string role, int id);
    virtual void setNightAction(const std::string target);
    virtual void clearNightAction();
    virtual std::string NightAction() const;
  protected:
    std::string heal_;
  };
}
#endif