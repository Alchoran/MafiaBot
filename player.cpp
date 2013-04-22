#include "player.h"
#include <fstream>
#include <iostream>

#define DEBUGPLAYER


Mafia::Player::Player(std::string name, std::string role, int id) : nick_(name), role_(role), id_(id), dead_(false), voted_(false), votes_against_(0), healed(false){
#ifdef DEBUGPLAYER
  std::cout << "Player: " << nick_ << " created with ID:" << id_ << " and with Role:" << role_ << std::endl;
#endif
}

void Mafia::Player::clearVote(){
  vote_ = '\0';
  voted_ = false;
}

void Mafia::Player::setDeath(){
  dead_ = true;
}

void Mafia::Player::setVote(const std::string vote){
  vote_ = vote;
  voted_ = true;
}

void Mafia::Player::setHealed(){
  healed=true;
}

bool Mafia::Player::isDead() const{
  return dead_;
}

std::string Mafia::Player::Nick() const{
  return nick_;
}

std::string Mafia::Player::Role() const{
  return role_;
}

std::string Mafia::Player::Voted() const{
  return vote_;
}

int Mafia::Player::ID() const{
  return id_;
}

void Mafia::Mob::setWhack(const std::string target){
  whack_ = target;
}

std::string Mafia::Mob::Whack() const{
  return whack_;
}

void Mafia::Mob::clearWhack(){
  whack_ = "0";
}

void Mafia::Police::setInvestigate(const std::string target){
  investigate_ = target;
}

std::string Mafia::Police::Investigate() const{
  return investigate_;
}

void Mafia::Police::clearInvestigate(){
  investigate_ = "0";
}

void Mafia::Doctor::setHeal(const std::string target){
  heal_ = target;
}

std::string Mafia::Doctor::Heal() const{
  return heal_;
}

void Mafia::Doctor::clearHeal(){
  heal_ = "0";
}

void Mafia::Player::promoteRole(const std::string role){
  role_ = role;
}

void Mafia::Player::incrVoted(){
  votes_against_++;
}
int Mafia::Player::numVotes()const{
  return votes_against_;
}

void Mafia::Player::clearVoted(){
  votes_against_=0;
}

void Mafia::Player::setNick(const std::string nick){
  nick_ = nick;
}