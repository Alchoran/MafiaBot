#include "Game.h"


Game::Game(std::string server, std::string port, boost::asio::io_service& io_service, std::fstream& debfil, std::fstream& errfil) : IRC(server, port, io_service, debfil, errfil)
{
	
}


Game::~Game(void)
{
}
