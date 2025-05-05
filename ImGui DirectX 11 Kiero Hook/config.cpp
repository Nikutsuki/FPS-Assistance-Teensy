#include "config.h"

void Config::addDebugMessage(std::string message)
{
	if (debug_messages.size() > 512)
	{
		debug_messages.pop_back();
	}
	debug_messages.push_front(message);
}
