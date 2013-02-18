// Lucas McIntosh
// 08/08/2011
// IRC functions/procedures declaration file
#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include "IRC.h"
#include <string>
#include <boost\array.hpp>

namespace IRC
{
	const bool DebugFunctions = true;

	// Causes the Bot to disconnect from the server.
	void Disconnect(std::string);

	// Checks if the server has sent a PING and returns a PONG
	bool PingPong(std::string);

	// Wrapped Commands extraction function
	int Commands(std::string,size_t);

	// Extracts the channel that the message came into.
	std::string Channel(std::string,size_t);

	// Extracts the nickname used to send the message.
	std::string Nick(std::string,size_t);
}

#endif