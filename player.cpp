#include "player.h"
#include <fstream>
#include <iostream>

#define DEBUGPLAYER

namespace Mafia{
  Player::Player(std::string name, std::string role, int id) : nick_(name), role_(role), id_(id), dead_(false), voted_(false), votes_against_(0), healed(false){
#ifdef DEBUGPLAYER
    std::cout << "Player: " << nick_ << " created with ID:" << id_ << " and with Role:" << role_ << std::endl;
#endif
  }

  void Player::clearVote(){
    vote_ = '\0';
    voted_ = false;
  }

  void Player::setDeath(){
    dead_ = true;
  }

  void Player::setVote(const std::string vote){
    vote_ = vote;
    voted_ = true;
  }

  void Player::setHealed(){
    healed=true;
  }

  bool Player::isDead() const{
    return dead_;
  }

  std::string Player::Nick() const{
    return nick_;
  }

  std::string Player::Role() const{
    return role_;
  }

  std::string Player::Voted() const{
    return vote_;
  }

  int Player::ID() const{
    return id_;
  }

  void Mob::setNightAction(const std::string target){
    whack_ = target;
  }

  std::string Mob::NightAction() const{
    return whack_;
  }

  void Mob::clearNightAction(){
    whack_ = "0";
  }

  void Police::setNightAction(const std::string target){
    investigate_ = target;
  }

  std::string Police::NightAction() const{
    return investigate_;
  }

  void Police::clearNightAction(){
    investigate_ = "0";
  }

  void Doctor::setNightAction(const std::string target){
    heal_ = target;
  }

  std::string Doctor::NightAction() const{
    return heal_;
  }

  void Doctor::clearNightAction(){
    heal_ = "0";
  }

  void Player::promoteRole(const std::string role){
    role_ = role;
  }

  void Player::incrVoted(){
    votes_against_++;
  }
  int Player::numVotes()const{
    return votes_against_;
  }

  void Player::clearVoted(){
    votes_against_=0;
  }

  void Player::setNick(const std::string nick){
    nick_ = nick;
  }

  Mob::Mob(std::string name, std::string role, int id):Player(name, role, id){};
  Police::Police(std::string name, std::string role, int id):Player(name, role, id){};
  Doctor::Doctor(std::string name, std::string role, int id):Player(name, role, id){};
}