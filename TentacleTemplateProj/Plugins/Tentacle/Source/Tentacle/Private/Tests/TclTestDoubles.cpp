// © 2025 Eldritch One. All rights reserved.

#include "Tests/TclTestDoubles.h"
#include "Base/TclViewer.h"
#include "Sdk/TclCustomEvent.h"
#include "Sdk/TclTimerEvent.h"
#include "Sdk/TclStreamEventBase.h"
#include "Sdk/TclStreamEvent_Twitch_Base.h"
#include "Sdk/TclStreamEvent_Kick_Base.h"
#include "Sdk/TclStreamEvent_Twitch_Follow.h"
#include "Sdk/TclStreamEvent_Kick_ChannelFollowed.h"
#include "Sdk/TclStreamEvent_Anywhere_Follow.h"
#include "Sdk/TclStreamEvent_Anywhere_ChatMessage.h"
#include "Sdk/TclStreamEvent_Twitch_ChatMessage.h"
#include "Sdk/TclStreamEvent_Twitch_Raid.h"

// Not behind WITH_DEV_AUTOMATION_TESTS: UHT's generated code for the classes in
// TclTestDoubles.h is compiled into every configuration, so their functions have
// to be defined in every configuration too.

TArray<FString>
UTclTestRecorder::Names() const
{
	TArray<FString> Result;
	for (const FTclRecordedCall& Call : Calls)
		Result.Add(Call.Name);
	return Result;
}

int32
UTclTestRecorder::Count(const FString& Name) const
{
	int32 Result = 0;
	for (const FTclRecordedCall& Call : Calls)
	{
		if (Call.Name == Name)
			++Result;
	}
	return Result;
}

const FTclRecordedCall*
UTclTestRecorder::Find(const FString& Name) const
{
	return Calls.FindByPredicate([&Name](const FTclRecordedCall& Call) { return Call.Name == Name; });
}

void
UTclTestRecorder::Record(const FString& Name, const UObject* Event, const UTclViewer* Viewer, const FString& Detail)
{
	FTclRecordedCall& Call = Calls.AddDefaulted_GetRef();
	Call.Name = Name;
	Call.Event = Event;
	Call.Viewer = Viewer;
	Call.Detail = Detail;

	if (OnRecord)
		OnRecord(Call);
}

void UTclTestRecorder::OnEvent_Anywhere(const UTclStreamEventBase* Event, UTclViewer* Viewer) { Record(TEXT("OnEvent_Anywhere"), Event, Viewer); }
void UTclTestRecorder::OnEvent_Twitch(const UTclStreamEvent_Twitch_Base* Event, UTclViewer* Viewer) { Record(TEXT("OnEvent_Twitch"), Event, Viewer); }
void UTclTestRecorder::OnEvent_Kick(const UTclStreamEvent_Kick_Base* Event, UTclViewer* Viewer) { Record(TEXT("OnEvent_Kick"), Event, Viewer); }
void UTclTestRecorder::OnFollow_Twitch(const UTclStreamEvent_Twitch_Follow* Event, UTclViewer* Viewer) { Record(TEXT("OnFollow_Twitch"), Event, Viewer); }
void UTclTestRecorder::OnChannelFollowed_Kick(const UTclStreamEvent_Kick_ChannelFollowed* Event, UTclViewer* Viewer) { Record(TEXT("OnChannelFollowed_Kick"), Event, Viewer); }
void UTclTestRecorder::OnFollow_Anywhere(const UTclStreamEvent_Anywhere_Follow* Event, UTclViewer* Viewer) { Record(TEXT("OnFollow_Anywhere"), Event, Viewer); }
void UTclTestRecorder::OnChatJoin_Twitch(UTclViewer* Viewer) { Record(TEXT("OnChatJoin_Twitch"), nullptr, Viewer); }
void UTclTestRecorder::OnChatJoin_Kick(UTclViewer* Viewer) { Record(TEXT("OnChatJoin_Kick"), nullptr, Viewer); }
void UTclTestRecorder::OnChatJoin_Anywhere(UTclViewer* Viewer) { Record(TEXT("OnChatJoin_Anywhere"), nullptr, Viewer); }
void UTclTestRecorder::OnRaidEnded_Twitch() { Record(TEXT("OnRaidEnded_Twitch"), nullptr, nullptr); }
void UTclTestRecorder::Root_OnStreamEvent(const UTclStreamEventBase* Event, UTclViewer* Viewer) { Record(TEXT("Root_OnStreamEvent"), Event, Viewer); }
void UTclTestRecorder::Root_OnFollow(const UTclStreamEvent_Anywhere_Follow* Event, UTclViewer* Viewer) { Record(TEXT("Root_OnFollow"), Event, Viewer); }
void UTclTestRecorder::Root_OnChatJoin(UTclViewer* Viewer) { Record(TEXT("Root_OnChatJoin"), nullptr, Viewer); }
void UTclTestRecorder::Root_OnChatMessage(const UTclStreamEvent_Anywhere_ChatMessage* ChatMessage, UTclViewer* Viewer) { Record(TEXT("Root_OnChatMessage"), ChatMessage, Viewer); }
void UTclTestRecorder::Root_OnChatCommand(const UTclStreamEvent_Anywhere_ChatMessage* ChatMessage, UTclViewer* Viewer) { Record(TEXT("Root_OnChatCommand"), ChatMessage, Viewer); }
void UTclTestRecorder::Twitch_OnChannelFollow(const UTclStreamEvent_Twitch_Follow* Event, UTclViewer* Viewer) { Record(TEXT("Twitch_OnChannelFollow"), Event, Viewer); }
void UTclTestRecorder::Twitch_OnChannelChatMessage(const UTclStreamEvent_Twitch_ChatMessage* Event, UTclViewer* Viewer) { Record(TEXT("Twitch_OnChannelChatMessage"), Event, Viewer); }
void UTclTestRecorder::Twitch_OnChannelRaid(const UTclStreamEvent_Twitch_Raid* Event, UTclViewer* Viewer) { Record(TEXT("Twitch_OnChannelRaid"), Event, Viewer); }
void UTclTestRecorder::TwitchExtras_OnIncomingRaidEnded() { Record(TEXT("TwitchExtras_OnIncomingRaidEnded"), nullptr, nullptr); }
void UTclTestRecorder::Kick_OnChannelFollow(const UTclStreamEvent_Kick_ChannelFollowed* Event, UTclViewer* Viewer) { Record(TEXT("Kick_OnChannelFollow"), Event, Viewer); }
void UTclTestRecorder::OnViewerAdded_Actor(const UTclViewer* Viewer) { Record(TEXT("OnViewerAdded_Actor"), nullptr, Viewer); }
void UTclTestRecorder::OnViewerAdded_Viewers(const UTclViewer* Viewer) { Record(TEXT("OnViewerAdded_Viewers"), nullptr, Viewer); }
void UTclTestRecorder::OnCustomEvent(const UTclCustomEvent* Event, UTclViewer* Viewer) { Record(TEXT("OnCustomEvent"), Event, Viewer, Event ? Event->Name : FString()); }
void UTclTestRecorder::OnSubscribed(const UTclCustomEvent* Event, UTclViewer* Viewer) { Record(TEXT("OnSubscribed"), Event, Viewer, Event ? Event->Name : FString()); }
void UTclTestRecorder::OnSubscribedOther(const UTclCustomEvent* Event, UTclViewer* Viewer) { Record(TEXT("OnSubscribedOther"), Event, Viewer, Event ? Event->Name : FString()); }
void UTclTestRecorder::OnTriggered(const UTclCustomEvent* Event, UTclViewer* Viewer) { Record(TEXT("OnTriggered"), Event, Viewer, Event ? Event->Name : FString()); }
void UTclTestRecorder::OnTimerFired(const UTclTimerEvent* Event) { Record(TEXT("OnTimerFired"), Event, nullptr, Event ? FString::Printf(TEXT("%s#%d"), *Event->Name, Event->FireCount) : FString()); }
void UTclTestRecorder::OnTimerSubscribed(const UTclTimerEvent* Event) { Record(TEXT("OnTimerSubscribed"), Event, nullptr, Event ? Event->Name : FString()); }
void UTclTestRecorder::OnTimerSubscribedOther(const UTclTimerEvent* Event) { Record(TEXT("OnTimerSubscribedOther"), Event, nullptr, Event ? Event->Name : FString()); }
void UTclTestRecorder::OnTimerListenerFired(const UTclTimerEvent* Event) { Record(TEXT("OnTimerListenerFired"), Event, nullptr, Event ? Event->Name : FString()); }
void UTclTestRecorder::OnInitialize() { Record(TEXT("OnInitialize"), nullptr, nullptr); }
void UTclTestRecorder::OnCommandsChanged() { Record(TEXT("OnCommandsChanged"), nullptr, nullptr); }
void UTclTestRecorder::OnDefinitionsChanged() { Record(TEXT("OnDefinitionsChanged"), nullptr, nullptr); }
void UTclTestRecorder::OnIntValueChangedFromApp(int32 NewValue, int32 OldValue) { Record(TEXT("OnValueChangedFromApp"), nullptr, nullptr, FString::Printf(TEXT("%d<-%d"), NewValue, OldValue)); }
void UTclTestRecorder::OnIntValueCommittedFromApp(int32 NewValue) { Record(TEXT("OnValueCommittedFromApp"), nullptr, nullptr, FString::FromInt(NewValue)); }
void UTclTestRecorder::OnDefinitionComplete(ETclDefinitionResult Result) { Record(TEXT("OnDefinitionComplete"), nullptr, nullptr, UEnum::GetValueAsString(Result)); }
void UTclTestRecorder::OnConnectionStatusChanged(ETclConnectionStatus ConnectionStatus) { Record(TEXT("OnConnectionStatusChanged"), nullptr, nullptr, UEnum::GetValueAsString(ConnectionStatus)); }

void UTclTestRecorder::OnGetStreams(const FTclTwitchApiGetStreamsOutput& Response, int32 StatusCode, const FString& ErrorMessage)
{
	const FString FirstLogin = Response.Data.IsEmpty() ? FString() : Response.Data[0].UserLogin;
	const FString Cursor = Response.Pagination.bHasCursor ? Response.Pagination.Cursor : TEXT("<none>");
	Record(TEXT("OnGetStreams"), nullptr, nullptr, FString::Printf(TEXT("%d|%s|%s|%d|%s"), Response.Data.Num(), *FirstLogin, *Cursor, StatusCode, *ErrorMessage));
}

void UTclTestRecorder::OnPlatformRequestSucceeded(int32 StatusCode, const FString& ErrorMessage)
{
	Record(TEXT("OnPlatformRequestSucceeded"), nullptr, nullptr, FString::Printf(TEXT("%d|%s"), StatusCode, *ErrorMessage));
}

void UTclTestRecorder::OnPlatformRequestFailed(int32 StatusCode, const FString& ErrorMessage)
{
	Record(TEXT("OnPlatformRequestFailed"), nullptr, nullptr, FString::Printf(TEXT("%d|%s"), StatusCode, *ErrorMessage));
}

void UTclTestRecorder::SignIn_OnPrompt(const FString& Url, const FString& UserCode, const FString& Message)
{
	Record(TEXT("OnPrompt"), nullptr, nullptr, FString::Printf(TEXT("%s|%s|%s"), *Url, *UserCode, *Message));
}

void UTclTestRecorder::SignIn_OnConnected(const FString& Url, const FString& UserCode, const FString& Message)
{
	Record(TEXT("OnConnected"), nullptr, nullptr, FString::Printf(TEXT("%s|%s|%s"), *Url, *UserCode, *Message));
}

void UTclTestRecorder::SignIn_OnFailed(const FString& Url, const FString& UserCode, const FString& Message)
{
	Record(TEXT("OnFailed"), nullptr, nullptr, FString::Printf(TEXT("%s|%s|%s"), *Url, *UserCode, *Message));
}

void UTclTestRecorder::OnSignInStateChanged(const FTclSignInState& State)
{
	const FString Stage = StaticEnum<ETclSignInStage>()->GetNameStringByValue(static_cast<int64>(State.Stage));
	Record(TEXT("OnSignInStateChanged"), nullptr, nullptr, FString::Printf(TEXT("%s|%s"), *Stage, *State.Label));
}
