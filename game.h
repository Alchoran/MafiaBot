#pragma once
#include "IRC2.h"
#include "Mafia.h"
using namespace IRC;
using namespace Mafia;
class Game : public IRC, public Mafia {
public:
	Game(std::string /*server*/, std::string /*port*/, boost::asio::io_service&, std::fstream& /*debugfile*/, std::fstream& /*errorfile*/);
	~Game(void);
	void gameMain(void);

};

