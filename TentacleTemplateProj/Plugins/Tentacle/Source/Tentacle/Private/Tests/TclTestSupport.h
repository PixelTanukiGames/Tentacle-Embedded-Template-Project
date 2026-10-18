// © 2025 Eldritch One. All rights reserved.

#pragma once

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Base/Tentacle.h"
#include "Base/Tentacle_Connections.h"
#include "Sdk/TclMsg_ServerToClient.h"
#include "Sdk/TclMsg_ServerToClient_ConnectionsPrompt.h"
#include "Sdk/TclMsg_ServerToClient_ConnectionsResult.h"
#include "Sdk/TclMsg_ServerToClient_ConnectionsStatus.h"
#include "Sdk/TclPlatformConnection.h"
#include "Dom/JsonObject.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/Package.h"

/**
 * Shared helpers for the plugin's automation tests: wire payloads shaped the way
 * Tentacle sends them, a capture of what a Tentacle actor sends back, and a bare
 * world for the tests that need components to begin play.
 *
 * Every payload carries every field the generated parsers require, because a
 * missing required field is logged as an error and fails the test that sent it.
 */
namespace TclTest
{
	/** A viewer payload with a Twitch profile. */
	inline FString TwitchViewerJson(const FString& Id, const FString& Name)
	{
		return FString::Printf(
			TEXT("{\"id\":\"%s\",\"createdAt\":\"2026-01-01T00:00:00Z\",\"updatedAt\":\"2026-01-01T00:00:00Z\",\"properties\":{},")
			TEXT("\"twitch\":{\"id\":\"tw-%s\",\"viewerId\":\"%s\",\"name\":\"%s\",\"displayName\":\"%s\",\"broadcasterType\":\"\",")
			TEXT("\"description\":\"\",\"isKnownBot\":false,\"offlineImageUrl\":\"\",\"profileImageUrl\":\"\",\"type\":\"\",")
			TEXT("\"createdAt\":\"2026-01-01T00:00:00Z\",\"updatedAt\":\"2026-01-01T00:00:00Z\"}}"),
			*Id, *Id, *Id, *Name.ToLower(), *Name);
	}

	/** A viewer payload with a Kick profile. */
	inline FString KickViewerJson(const FString& Id, const FString& Name)
	{
		return FString::Printf(
			TEXT("{\"id\":\"%s\",\"createdAt\":\"2026-01-01T00:00:00Z\",\"updatedAt\":\"2026-01-01T00:00:00Z\",\"properties\":{},")
			TEXT("\"kick\":{\"id\":42,\"viewerId\":\"%s\",\"username\":\"%s\",\"isAnonymous\":false,\"isVerified\":false,")
			TEXT("\"createdAt\":\"2026-01-01T00:00:00Z\",\"updatedAt\":\"2026-01-01T00:00:00Z\"}}"),
			*Id, *Id, *Name);
	}

	/** A Twitch follow event (channel.follow), with the fields its Anywhere view reads. */
	inline FString TwitchFollowEventJson(const FString& EventId, const FString& ViewerId)
	{
		return FString::Printf(
			TEXT("{\"$platform\":\"twitch\",\"$type\":\"channel.follow\",\"$id\":\"%s\",\"$createdAt\":\"2026-01-01T00:00:00Z\",")
			TEXT("\"$kind\":\"channel.follow\",\"$viewerId\":\"%s\",\"$followerUserId\":\"tw-%s\",\"$followerUserName\":\"Follower\",")
			TEXT("\"user_id\":\"tw-%s\",\"user_login\":\"follower\",\"user_name\":\"Follower\",\"broadcaster_user_id\":\"b\",")
			TEXT("\"broadcaster_user_login\":\"streamer\",\"broadcaster_user_name\":\"Streamer\",\"followed_at\":\"2026-01-01T00:00:00Z\"}"),
			*EventId, *ViewerId, *ViewerId, *ViewerId);
	}

	/** A Kick follow event (channel.followed), with the fields its Anywhere view reads. */
	inline FString KickFollowEventJson(const FString& EventId, const FString& ViewerId)
	{
		return FString::Printf(
			TEXT("{\"$platform\":\"kick\",\"$type\":\"channel.followed\",\"$id\":\"%s\",\"$createdAt\":\"2026-01-01T00:00:00Z\",")
			TEXT("\"$kind\":\"channel.followed\",\"$viewerId\":\"%s\",\"$followerUserId\":\"42\",\"$followerUserName\":\"Follower\",")
			TEXT("\"broadcaster\":{\"is_anonymous\":false,\"is_verified\":false,\"username\":\"streamer\"},")
			TEXT("\"follower\":{\"is_anonymous\":false,\"is_verified\":false,\"username\":\"follower\"}}"),
			*EventId, *ViewerId);
	}

	/**
	 * A Twitch chat message (channel.chat.message). An empty Command is a plain
	 * message; anything else is a command the app classified, without its prefix.
	 */
	inline FString TwitchChatMessageEventJson(const FString& EventId, const FString& ViewerId, const FString& Text, const FString& Command = FString())
	{
		const FString CommandJson = Command.IsEmpty() ? TEXT("null") : FString::Printf(TEXT("\"%s\""), *Command);
		return FString::Printf(
			TEXT("{\"$platform\":\"twitch\",\"$type\":\"channel.chat.message\",\"$id\":\"%s\",\"$createdAt\":\"2026-01-01T00:00:00Z\",")
			TEXT("\"$kind\":\"channel.chat.message\",\"$viewerId\":\"%s\",\"$command\":%s,\"$commandArgs\":[],\"$emojis\":{},\"$emotes\":{},")
			TEXT("\"$html\":\"%s\",\"$senderUserId\":\"tw-%s\",\"$senderUserName\":\"Chatter\",\"$text\":\"%s\",")
			TEXT("\"broadcaster_user_id\":\"b\",\"broadcaster_user_login\":\"streamer\",\"broadcaster_user_name\":\"Streamer\",")
			TEXT("\"chatter_user_id\":\"tw-%s\",\"chatter_user_login\":\"chatter\",\"chatter_user_name\":\"Chatter\",\"message_id\":\"m-%s\",")
			TEXT("\"message\":{\"text\":\"%s\",\"fragments\":[]},\"color\":\"\",\"badges\":[],\"message_type\":\"text\",\"cheer\":null,\"reply\":null,")
			TEXT("\"channel_points_custom_reward_id\":null,\"source_broadcaster_user_id\":null,\"source_broadcaster_user_login\":null,")
			TEXT("\"source_broadcaster_user_name\":null,\"source_message_id\":null,\"source_badges\":null,\"is_source_only\":null}"),
			*EventId, *ViewerId, *CommandJson, *Text, *ViewerId, *Text, *ViewerId, *EventId, *Text);
	}

	/** A raid on the streamer's Twitch channel (channel.raid). */
	inline FString TwitchRaidEventJson(const FString& EventId)
	{
		return FString::Printf(
			TEXT("{\"$platform\":\"twitch\",\"$type\":\"channel.raid\",\"$id\":\"%s\",\"$createdAt\":\"2026-01-01T00:00:00Z\",")
			TEXT("\"$kind\":\"channel.raid\",\"$viewerId\":null,\"from_broadcaster_user_id\":\"r\",\"from_broadcaster_user_login\":\"raider\",")
			TEXT("\"from_broadcaster_user_name\":\"Raider\",\"to_broadcaster_user_id\":\"b\",\"to_broadcaster_user_login\":\"streamer\",")
			TEXT("\"to_broadcaster_user_name\":\"Streamer\",\"viewers\":12}"),
			*EventId);
	}

	/** A Twitch event of a kind this plugin version has no class for. */
	inline FString TwitchFutureEventJson(const FString& EventId)
	{
		return FString::Printf(
			TEXT("{\"$platform\":\"twitch\",\"$type\":\"channel.some_future_thing\",\"$id\":\"%s\",\"$createdAt\":\"2026-01-01T00:00:00Z\",")
			TEXT("\"$kind\":\"channel.some_future_thing\"}"),
			*EventId);
	}

	/** A Stream_Event message; an empty ViewerJson leaves the viewer out. */
	inline FString StreamEventMessage(const FString& EventJson, const FString& ViewerJson)
	{
		return ViewerJson.IsEmpty()
			? FString::Printf(TEXT("{\"type\":\"Stream_Event\",\"data\":{\"event\":%s}}"), *EventJson)
			: FString::Printf(TEXT("{\"type\":\"Stream_Event\",\"data\":{\"event\":%s,\"viewer\":%s}}"), *EventJson, *ViewerJson);
	}

	/** A Stream_ChatJoin message. */
	inline FString ChatJoinMessage(const FString& ViewerJson)
	{
		return FString::Printf(TEXT("{\"type\":\"Stream_ChatJoin\",\"data\":{\"viewer\":%s}}"), *ViewerJson);
	}

	/** A CustomEvents_Event message; an empty ViewerJson leaves the viewer out. */
	inline FString CustomEventMessage(const FString& Name, const FString& ViewerJson)
	{
		return ViewerJson.IsEmpty()
			? FString::Printf(TEXT("{\"type\":\"CustomEvents_Event\",\"data\":{\"name\":\"%s\"}}"), *Name)
			: FString::Printf(TEXT("{\"type\":\"CustomEvents_Event\",\"data\":{\"name\":\"%s\",\"viewer\":%s}}"), *Name, *ViewerJson);
	}

	/** A Timers_Fired message for one fire of the named timer. */
	inline FString TimerFiredMessage(const FString& Name, int32 FireCount = 1, int32 NextFireInSeconds = 600)
	{
		return FString::Printf(
			TEXT("{\"type\":\"Timers_Fired\",\"data\":{\"id\":\"timer-%s\",\"name\":\"%s\",\"fireCount\":%d,\"nextFireInSeconds\":%d}}"),
			*Name, *Name, FireCount, NextFireInSeconds);
	}

	/**
	 * One timer definition, as Tentacle sends it. State is "stopped", "running"
	 * or "paused".
	 */
	inline FString TimerDefinitionJson(const FString& Name, const FString& State = TEXT("running"))
	{
		return FString::Printf(
			TEXT("{\"id\":\"timer-%s\",\"name\":\"%s\",\"state\":\"%s\",\"repeating\":true,\"onlyWhileLive\":true,")
			TEXT("\"intervalMinSeconds\":60,\"intervalMaxSeconds\":120,\"minChatMessages\":3}"),
			*Name, *Name, *State);
	}

	/** A Timers_Definitions message carrying the definitions given, already JSON. */
	inline FString TimerDefinitionsMessage(const TArray<FString>& DefinitionsJson)
	{
		return FString::Printf(
			TEXT("{\"type\":\"Timers_Definitions\",\"data\":{\"definitions\":[%s]}}"),
			*FString::Join(DefinitionsJson, TEXT(",")));
	}

	/**
	 * One bot command, as Tentacle sends it: every field the generated parser
	 * requires. ExtraJson is spliced in last, for the fields a test cares about.
	 */
	inline FString BotCommandJson(const FString& Name, const FString& ExtraJson = FString())
	{
		return FString::Printf(
			TEXT("{\"id\":\"command-%s\",\"name\":\"%s\",\"triggerType\":\"command\",\"enabled\":true,\"description\":null,")
			TEXT("\"responseTemplate\":\"%s answered\",\"actions\":[],\"permission\":\"everyone\",\"platforms\":[\"twitch\",\"kick\"],")
			TEXT("\"userCooldownSeconds\":0,\"globalCooldownSeconds\":0,\"ignoreFirstTimeChatters\":false,\"minAccountAgeDays\":0%s}"),
			*Name, *Name, *Name, *(ExtraJson.IsEmpty() ? FString() : TEXT(",") + ExtraJson));
	}

	/** A Bot_Commands message carrying these commands (a JSON array's contents) for an enabled bot. */
	inline FString BotCommandsMessage(const FString& CommandsJson, bool bBotEnabled = true)
	{
		return FString::Printf(
			TEXT("{\"type\":\"Bot_Commands\",\"data\":{\"botEnabled\":%s,\"commandPrefix\":\"!\",\"globalCooldownSeconds\":5,")
			TEXT("\"commands\":[%s]}}"),
			bBotEnabled ? TEXT("true") : TEXT("false"), *CommandsJson);
	}

	/** One custom event definition, as Tentacle sends it, fired by these chat commands (a JSON array's contents). */
	inline FString CustomEventDefinitionJson(const FString& Name, const FString& ChatCommandsJson = FString())
	{
		return FString::Printf(
			TEXT("{\"id\":\"definition-%s\",\"name\":\"%s\",\"enabled\":true,\"triggers\":{},\"conditionsMode\":\"any\",")
			TEXT("\"suppressStreamEvent\":false,\"createdAt\":\"2026-01-01T00:00:00Z\",\"updatedAt\":\"2026-01-01T00:00:00Z\",\"chatCommands\":[%s]}"),
			*Name, *Name, *ChatCommandsJson);
	}

	/** A CustomEvents_Definitions message carrying these definitions (a JSON array's contents). */
	inline FString CustomEventDefinitionsMessage(const FString& DefinitionsJson)
	{
		return FString::Printf(TEXT("{\"type\":\"CustomEvents_Definitions\",\"data\":{\"definitions\":[%s]}}"), *DefinitionsJson);
	}

	/** A CustomControls_ControlEvent message. Kind is ValueChange or ValueCommit; ValueJson is the raw JSON value. */
	inline FString ControlEventMessage(const FString& Kind, const FString& ControlId, const FString& ValueJson)
	{
		return FString::Printf(
			TEXT("{\"type\":\"CustomControls_ControlEvent\",\"data\":{\"event\":{\"type\":\"%s\",\"controlId\":\"%s\",\"value\":%s}}}"),
			*Kind, *ControlId, *ValueJson);
	}

	/** Twitch's sign-in page and code, as Connections_Prompt carries them. */
	inline FString TwitchConnectionsPromptJson()
	{
		return TEXT("{\"platform\":\"twitch\",\"url\":\"https://www.twitch.tv/activate?device-code=ABCD\",\"userCode\":\"ABCD\",\"expiresInSeconds\":1800}");
	}

	/** Twitch's connection with the account Streamer signed in, as Connections_Status carries it. */
	inline FString TwitchSignedInJson(const TCHAR* Status, bool bNeedsReconnect = false, bool bMissingPermissions = false)
	{
		return FString::Printf(
			TEXT("{\"platform\":\"twitch\",\"status\":\"%s\",\"needsReconnect\":%s,\"missingPermissions\":%s,\"account\":{\"userId\":\"1\",\"login\":\"streamer\",\"displayName\":\"Streamer\"}}"),
			Status, bNeedsReconnect ? TEXT("true") : TEXT("false"), bMissingPermissions ? TEXT("true") : TEXT("false"));
	}

	/** Parse a server-to-client message as the plugin does on receipt, cast to the message class the test expects. */
	template <typename TMessage>
	TMessage* ParseMessage(UObject* Outer, const FString& Json)
	{
		return Cast<TMessage>(UTclMsg_ServerToClient::ParseFromString(Outer, Json));
	}

	/** A platform connection read from its JSON, as the Connections component reads one. */
	inline UTclPlatformConnection* ParseConnection(const FString& Json)
	{
		return UTclPlatformConnection::ParseFromString(GetTransientPackage(), Json);
	}

	/** A Tentacle actor in a test world, embedded or in Paired mode, begun play but not connecting by itself. */
	inline ATentacle* SpawnTentacle(UWorld* World, ETclTentacleMode Mode)
	{
		ATentacle* Tentacle = World->SpawnActor<ATentacle>();
		Tentacle->EditorMode = Tentacle->BuildMode = Mode;
		Tentacle->bAutoConnect = false;
		Tentacle->DispatchBeginPlay();
		return Tentacle;
	}

	/** Hands Tentacle's Connections component a Connections_Status, Connections_Prompt, or Connections_Result, as the session would. */
	inline void ReceiveConnectionsMessage(ATentacle* Tentacle, const FString& Type, const FString& Data)
	{
		const FString Json = FString::Printf(TEXT("{\"type\":\"%s\",\"data\":%s}"), *Type, *Data);
		if (Type == TEXT("Connections_Status"))
			Tentacle->Connections->HandleMsg_Status(ParseMessage<UTclMsg_ServerToClient_ConnectionsStatus>(Tentacle, Json));
		else if (Type == TEXT("Connections_Prompt"))
			Tentacle->Connections->HandleMsg_Prompt(ParseMessage<UTclMsg_ServerToClient_ConnectionsPrompt>(Tentacle, Json));
		else
			Tentacle->Connections->HandleMsg_Result(ParseMessage<UTclMsg_ServerToClient_ConnectionsResult>(Tentacle, Json));
	}

	/** Parse a JSON object; null when the text is not one. */
	inline TSharedPtr<FJsonObject> ParseObject(const FString& Json)
	{
		TSharedPtr<FJsonObject> Object;
		const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
		if (!FJsonSerializer::Deserialize(Reader, Object))
			return nullptr;
		return Object;
	}

	/**
	 * The object field Name of Object, or null. Unlike GetObjectField, a missing
	 * field is not logged as an error, which would fail the test on its own.
	 */
	inline TSharedPtr<FJsonObject> ObjectField(const TSharedPtr<FJsonObject>& Object, const FString& Name)
	{
		const TSharedPtr<FJsonObject>* Field = nullptr;
		if (!Object.IsValid() || !Object->TryGetObjectField(Name, Field) || !Field)
			return nullptr;
		return *Field;
	}

	/** The string field Name of Object, or an empty string. */
	inline FString StringField(const TSharedPtr<FJsonObject>& Object, const FString& Name)
	{
		FString Value;
		if (Object.IsValid())
			Object->TryGetStringField(Name, Value);
		return Value;
	}

	/**
	 * Every message a Tentacle actor hands over to be sent, for as long as this
	 * capture lives. Works without a session: the actor reports each message
	 * whether or not a socket is open to carry it.
	 */
	class FSentMessages
	{
	public:
		/** Start capturing what Tentacle sends. */
		explicit FSentMessages(ATentacle* InTentacle)
			: Tentacle(InTentacle)
		{
			Handle = Tentacle->OnSendMsg.AddLambda([this](const FString& Json) { Messages.Add(Json); });
		}

		/** Stop capturing. */
		~FSentMessages()
		{
			if (Tentacle.IsValid())
				Tentacle->OnSendMsg.Remove(Handle);
		}

		FSentMessages(const FSentMessages&) = delete;
		FSentMessages& operator=(const FSentMessages&) = delete;

		/** Every captured message, as sent. */
		TArray<FString> Messages;

		/** How many messages were captured. */
		int32 Num() const { return Messages.Num(); }

		/** Forget what was captured so far. */
		void Reset() { Messages.Reset(); }

		/** The type of message Index, or an empty string. */
		FString Type(int32 Index) const
		{
			return StringField(Messages.IsValidIndex(Index) ? ParseObject(Messages[Index]) : nullptr, TEXT("type"));
		}

		/** The data object of message Index, or null. */
		TSharedPtr<FJsonObject> Data(int32 Index) const
		{
			return ObjectField(Messages.IsValidIndex(Index) ? ParseObject(Messages[Index]) : nullptr, TEXT("data"));
		}

		/** The control object a custom-control message Index carries, or null. */
		TSharedPtr<FJsonObject> Control(int32 Index) const
		{
			return ObjectField(Data(Index), TEXT("control"));
		}

	private:
		TWeakObjectPtr<ATentacle> Tentacle;
		FDelegateHandle Handle;
	};

	/**
	 * A bare game world. Actors spawn into it, but nothing begins play until a
	 * test calls DispatchBeginPlay on an actor, so a Tentacle actor placed here
	 * never dials out or makes a request. Destroyed with this object.
	 */
	class FWorld
	{
	public:
		/** Create the world, with the engine context an actor's Destroy expects. */
		FWorld()
		{
			World = UWorld::CreateWorld(EWorldType::Game, false);
			if (World && GEngine)
				GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
		}

		/** Destroy the world and everything in it. */
		~FWorld()
		{
			if (!World)
				return;

			if (GEngine)
				GEngine->DestroyWorldContext(World);
			World->DestroyWorld(false);
		}

		FWorld(const FWorld&) = delete;
		FWorld& operator=(const FWorld&) = delete;

		/** The world, for spawning into. */
		UWorld* Get() const { return World; }

	private:
		UWorld* World = nullptr;
	};
}

#endif // WITH_DEV_AUTOMATION_TESTS
