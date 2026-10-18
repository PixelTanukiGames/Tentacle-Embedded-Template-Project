// © 2025 Eldritch One. All rights reserved.

#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Sdk/TclBotCommand.h"
#include "Tentacle_Bot.generated.h"

class UTclMsg_ServerToClient_BotCommands;

/**
 * Tentacle's chat bot, as the streamer configured it: whether it is on, and
 * every command it answers, kept current.
 *
 * Tentacle sends the whole command list as the session opens, and again whenever
 * the streamer creates, changes or removes a command or switches the bot on or
 * off, so what this holds is never more than a moment old. That is what a game
 * needs to talk about the commands in chat - listing the ones a viewer can use,
 * say - without keeping a copy of the streamer's settings.
 *
 * The list stays as it was while the session is closed, and is replaced by the
 * one Tentacle sends when it opens again.
 */
UCLASS()
class TENTACLE_API UTentacle_Bot : public UActorComponent
{
	GENERATED_BODY()

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCommandsChanged);

public:
	/**
	 * Fires whenever the command list changes, including when the first list
	 * arrives. Commands and the rest already hold the new list by the time it
	 * fires. A list Tentacle sends again unchanged, as it does every time the
	 * session opens, does not fire it.
	 */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Bot")
	FOnCommandsChanged OnCommandsChanged;

	/**
	 * Every command and phrase the streamer configured, enabled or not, ordered
	 * by name, with who may use it, where, how often, what it replies and what
	 * it does.
	 */
	UPROPERTY(BlueprintReadOnly, Category="Tentacle|Bot")
	TArray<UTclBotCommand*> Commands;

	/** Whether the bot answers in chat at all. When false, no command answers. False until a list arrives. */
	UPROPERTY(BlueprintReadOnly, Category="Tentacle|Bot")
	bool bIsEnabled = false;

	/** What a chat message starts with to be read as a command, such as "!". Empty until a list arrives. */
	UPROPERTY(BlueprintReadOnly, Category="Tentacle|Bot")
	FString CommandPrefix;

	/** The bot-wide cooldown that spaces out every command, in seconds. */
	UPROPERTY(BlueprintReadOnly, Category="Tentacle|Bot")
	int32 GlobalCooldownSeconds = 0;

	/** Whether Tentacle has sent a command list yet. Until it has, Commands is empty. */
	UPROPERTY(BlueprintReadOnly, Category="Tentacle|Bot")
	bool bAreCommandsLoaded = false;

	/**
	 * The command or phrase with this name, without the command prefix. Names
	 * are matched case-insensitively by default; set Case Sensitive if you need
	 * exact matching.
	 */
	UFUNCTION(BlueprintPure, Category="Tentacle|Bot")
	UTclBotCommand* FindCommandByName(const FString& Name, bool& Found, bool bCaseSensitive = false) const;

	/**
	 * Take the command list a Bot_Commands message carries. CommandsJson is the
	 * message as received, which is what tells a changed list from one sent
	 * again.
	 */
	void HandleMsg_Commands(UTclMsg_ServerToClient_BotCommands* Msg, const FString& CommandsJson);

private:
	/** The last command list as received, so a list sent again unchanged fires nothing. */
	FString CommandsKey;
};
