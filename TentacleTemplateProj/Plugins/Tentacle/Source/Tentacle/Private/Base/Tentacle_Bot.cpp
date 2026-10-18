// © 2025 Eldritch One. All rights reserved.

#include "Base/Tentacle_Bot.h"
#include "Sdk/TclBotCommands.h"
#include "Sdk/TclMsg_ServerToClient_BotCommands.h"
#include "Utilities/TclCommon.h"

void
UTentacle_Bot::HandleMsg_Commands(UTclMsg_ServerToClient_BotCommands* Msg, const FString& CommandsJson)
{
	tclSoftAssert(Msg && Msg->Data);

	// Tentacle sends the current list every time a session opens, so a list
	// identical to the one held is not a change.
	if (bAreCommandsLoaded && CommandsJson == CommandsKey)
		return;

	Commands = Msg->Data->Commands;
	Commands.RemoveAll([](const UTclBotCommand* Command) { return Command == nullptr; });
	bIsEnabled = Msg->Data->bBotEnabled;
	CommandPrefix = Msg->Data->CommandPrefix;
	GlobalCooldownSeconds = Msg->Data->GlobalCooldownSeconds;
	bAreCommandsLoaded = true;
	CommandsKey = CommandsJson;

	OnCommandsChanged.Broadcast();
}

UTclBotCommand*
UTentacle_Bot::FindCommandByName(const FString& Name, bool& Found, bool bCaseSensitive) const
{
	const ESearchCase::Type SearchCase = bCaseSensitive ? ESearchCase::CaseSensitive : ESearchCase::IgnoreCase;
	for (UTclBotCommand* Command : Commands)
	{
		if (Command && !Name.IsEmpty() && Command->Name.Equals(Name, SearchCase))
		{
			Found = true;
			return Command;
		}
	}
	Found = false;
	return nullptr;
}
