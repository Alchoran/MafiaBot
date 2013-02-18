// Lucas McIntosh
// 09/08/2011
// IRC functions/procedures definition file

#include "functions.h"

// Causes the Bot to disconnect from the server.
void IRC::Disconnect(std::string nick)
{
	// NICK_LIST provides a list with which to check against to determine if !quit should be recognized.
	const boost::array<std::string, 4> NICK_LIST = { "Alch|Busy", "Alchoran", "Torqez", "Zardvark"}; 
	boost::array<std::string, 1>::const_iterator iter;
	
	if(DebugFunctions)
		std::cout << "Nick: " << nick << std::endl;

	// Checks if the person who said !quit is elligible to call it.
	for(iter=NICK_LIST.begin(); iter != NICK_LIST.end(); iter++)
	{
		if(nick == *iter)
			irc.sendMessage("QUIT :Quit command used by: " + nick);
	}
}

// Checks if the server has sent a PING and returns a PONG
bool IRC::PingPong(std::string message)
{
	if(DebugFunctions)
		std::cout << "Entering Ping Checker." << std::endl;

	std::string ping = message.substr(0,4); // Extracting first four char from message. This is where PING resides.
	const std::string PING = "PING";
	if(ping == PING) // Checks if the server has sent a PING
	{
		std::cout << "Ping? Pong!" << std::endl;
		//irc.sendMessage("PONG :imperial.irc");
		return true;
	}
	else if(DebugFunctions) std::cout << "Failed Ping extraction." << std::endl;
	return false;

}

// Wrapped Commands extraction function.
int IRC::Commands(std::string message, size_t bytes_transferred)
{
	const std::string COMMAND = "!";
	const std::string DELIM = " ";
	const std::string EMPTY = "";
	const std::string PM_DELIM = "PRIVMSG";
	std::string channel;
	std::string nick;
	std::string command;
	
	size_t exclamation;
	size_t found;
	size_t blank;
	size_t blank2;
	size_t chan;
	size_t end;
	size_t privmsg;
		
	if(DebugFunctions)
		std::cout << "Finding Nick." << std::endl;
	// Finds the ! after NICK
	exclamation = message.find(COMMAND); // Finds the first instance of COMMAND.
	nick = message.substr(1, exclamation-1); // NICK starts at the second position, substring runs to the position of COMMAND - 1 to prevent capture of "!".
	if(DebugFunctions)
				std::cout << "Nick is: " << nick << std::endl;

	if(DebugFunctions)
		std::cout << "Finding Command." << std::endl;

	found = message.rfind(COMMAND);
	if(found!=message.npos)
	{
		blank = message.find(DELIM, found);
		
		if(blank!=message.npos)
		{
			end = blank - found;
			if(DebugFunctions)
				std::cout << "Values are: Found:" << int(found) << ", Blank:" << int(blank) << ", End:" << int(end) << std::endl;
			command = message.substr(found,end);
		}
		else
		{
			end = (bytes_transferred -2) - found;
			if(DebugFunctions)
				std::cout << "Values are: Found:" << int(found) <<  ", Bytes_Transferred:" << int(bytes_transferred) << ", End:" << int(end) << std::endl;
			command = message.substr(found, end);
		}
	}

	if(DebugFunctions)
	{
		std::cout << command <<std::endl;
		std::cout << "Finding Channel." << std::endl;
	}
	end = 0;
	privmsg = message.find(PM_DELIM);
	chan = message.find("#");
	if(chan!=message.npos)
	{
		blank2 = message.find(DELIM, chan);
		end = blank2 - chan;
		if(blank2!=message.npos)
		{			
			channel = message.substr(chan, end); 
			if(DebugFunctions)
			{
				std::cout << "Values are: Chan:" << int(chan) << ", Blank2:" << int(blank2) << ", End:" << int(end) << std::endl;
				std::cout << "The channel is: " << channel << std::endl;
			}
		}
	}
	else if(privmsg!=message.npos)
			channel = nick;

	if(command == "!start")
	{
		if(channel==nick)
		{
			irc.sendMessage("privmsg " + channel + " :" + char(11) + "5 This command only works in a channel.");
			return 0;
		}
		else		
			return 1;					
	}
	else if(command == "!quit")
		return 2;

	return 0;
}

// Extracts the Channel that the message came through.
std::string IRC::Channel(std::string message, size_t bytes_transferred)
{

	const std::string COMMAND = "!";
	const std::string DELIM = " ";
	const std::string PM_DELIM = "PRIVMSG";
	std::string channel = "";		
	size_t blank;
	size_t chan;
	size_t end;

	end = 0;
	chan = message.find("#");
	if(chan!=message.npos)
	{
		blank = message.find(DELIM, chan);
		end = blank - chan;
		if(blank!=message.npos)
		{
			channel = message.substr(chan, end); 
		}
	}
	return channel;
}

// Extracts the nickname.
std::string IRC::Nick(std::string message, size_t bytes_transferred)
{
	const std::string COMMAND = "!";
	std::string nick;
	size_t exclamation;
	
	// Finds the ! after NICK
	exclamation = message.find(COMMAND); // Finds the first instance of COMMAND.
	nick = message.substr(1, exclamation-1); // NICK starts at the second position, substring runs to the position of COMMAND - 1 to prevent capture of "!".
	return nick;
}
