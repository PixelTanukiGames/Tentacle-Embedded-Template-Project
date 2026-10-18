// © 2025 Eldritch One. All rights reserved.

#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Base/TclCustomControlComponents.h"
#include "Sdk/TclDefinitionResult.h"
#include "Sdk/PlatformApi/TclTwitchApiTypes.h"
#include "Utilities/TclConnectionStatus.h"
#include "Base/Tentacle.h"
#include "UI/TclSignInState.h"
#include "TclTestDoubles.generated.h"

// Test doubles for the plugin's automation tests (Private/Tests). They are
// UCLASSes because a dynamic delegate can only be bound to a UFUNCTION and a
// BlueprintNativeEvent can only be overridden by a reflected subclass, and UHT
// has no way to leave a class out of a build configuration, so they compile
// into every build. Every one is Transient and hidden from class pickers and
// the Add Component menu; nothing outside the tests creates one.

class UTclViewer;
class UTclCustomEvent;
class UTclTimerEvent;
class UTclStreamEventBase;
class UTclStreamEvent_Twitch_Base;
class UTclStreamEvent_Kick_Base;
class UTclStreamEvent_Twitch_Follow;
class UTclStreamEvent_Kick_ChannelFollowed;
class UTclStreamEvent_Anywhere_Follow;
class UTclStreamEvent_Anywhere_ChatMessage;
class UTclStreamEvent_Twitch_ChatMessage;
class UTclStreamEvent_Twitch_Raid;

/**
 * One delegate call a UTclTestRecorder saw.
 */
struct FTclRecordedCall
{
	/** Which of the recorder's handlers ran, named after the delegate it is bound to. */
	FString Name;

	/** The event or payload object the delegate carried, if any. */
	TWeakObjectPtr<const UObject> Event;

	/** The viewer the delegate carried, if any. */
	TWeakObjectPtr<const UTclViewer> Viewer;

	/** Anything else worth asserting on, as text (a value, a pair of values). */
	FString Detail;
};

/**
 * A Tentacle actor whose play a test can end. A test world never initializes
 * its actors for play, so RouteEndPlay passes them by; this calls EndPlay as
 * leaving play does.
 */
UCLASS(Transient, HideDropdown, NotBlueprintable, NotBlueprintType, NotPlaceable)
class ATclTestTentacle : public ATentacle
{
	GENERATED_BODY()

public:
	void EndPlayForTest(const EEndPlayReason::Type Reason) { EndPlay(Reason); }
};

/**
 * Records every delegate it is bound to, in the order they fire.
 */
UCLASS(Transient, HideDropdown, NotBlueprintable, NotBlueprintType)
class UTclTestRecorder : public UObject
{
	GENERATED_BODY()

public:
	/** Every call, in the order it arrived. */
	TArray<FTclRecordedCall> Calls;

	/** Runs as each call is recorded, for a test that needs to act mid-dispatch. */
	TFunction<void(const FTclRecordedCall&)> OnRecord;

	/** The recorded call names, in order. */
	TArray<FString> Names() const;

	/** How many calls with this name were recorded. */
	int32 Count(const FString& Name) const;

	/** The first call with this name, or null. */
	const FTclRecordedCall* Find(const FString& Name) const;

	UFUNCTION() void OnEvent_Anywhere(const UTclStreamEventBase* Event, UTclViewer* Viewer);
	UFUNCTION() void OnEvent_Twitch(const UTclStreamEvent_Twitch_Base* Event, UTclViewer* Viewer);
	UFUNCTION() void OnEvent_Kick(const UTclStreamEvent_Kick_Base* Event, UTclViewer* Viewer);
	UFUNCTION() void OnFollow_Twitch(const UTclStreamEvent_Twitch_Follow* Event, UTclViewer* Viewer);
	UFUNCTION() void OnChannelFollowed_Kick(const UTclStreamEvent_Kick_ChannelFollowed* Event, UTclViewer* Viewer);
	UFUNCTION() void OnFollow_Anywhere(const UTclStreamEvent_Anywhere_Follow* Event, UTclViewer* Viewer);
	UFUNCTION() void OnChatJoin_Twitch(UTclViewer* Viewer);
	UFUNCTION() void OnChatJoin_Kick(UTclViewer* Viewer);
	UFUNCTION() void OnChatJoin_Anywhere(UTclViewer* Viewer);
	UFUNCTION() void OnRaidEnded_Twitch();

	// The entry point's and the platform components' stream members. Each handler
	// is named after its surface as well as its delegate, because the surfaces
	// can share delegate names (Tentacle's On Chat Message and Kick's).
	UFUNCTION() void Root_OnStreamEvent(const UTclStreamEventBase* Event, UTclViewer* Viewer);
	UFUNCTION() void Root_OnFollow(const UTclStreamEvent_Anywhere_Follow* Event, UTclViewer* Viewer);
	UFUNCTION() void Root_OnChatJoin(UTclViewer* Viewer);
	UFUNCTION() void Root_OnChatMessage(const UTclStreamEvent_Anywhere_ChatMessage* ChatMessage, UTclViewer* Viewer);
	UFUNCTION() void Root_OnChatCommand(const UTclStreamEvent_Anywhere_ChatMessage* ChatMessage, UTclViewer* Viewer);
	UFUNCTION() void Twitch_OnChannelFollow(const UTclStreamEvent_Twitch_Follow* Event, UTclViewer* Viewer);
	UFUNCTION() void Twitch_OnChannelChatMessage(const UTclStreamEvent_Twitch_ChatMessage* Event, UTclViewer* Viewer);
	UFUNCTION() void Twitch_OnChannelRaid(const UTclStreamEvent_Twitch_Raid* Event, UTclViewer* Viewer);
	UFUNCTION() void TwitchExtras_OnIncomingRaidEnded();
	UFUNCTION() void Kick_OnChannelFollow(const UTclStreamEvent_Kick_ChannelFollowed* Event, UTclViewer* Viewer);

	UFUNCTION() void OnViewerAdded_Actor(const UTclViewer* Viewer);
	UFUNCTION() void OnViewerAdded_Viewers(const UTclViewer* Viewer);
	UFUNCTION() void OnCustomEvent(const UTclCustomEvent* Event, UTclViewer* Viewer);
	UFUNCTION() void OnSubscribed(const UTclCustomEvent* Event, UTclViewer* Viewer);
	UFUNCTION() void OnSubscribedOther(const UTclCustomEvent* Event, UTclViewer* Viewer);
	UFUNCTION() void OnTriggered(const UTclCustomEvent* Event, UTclViewer* Viewer);
	UFUNCTION() void OnTimerFired(const UTclTimerEvent* Event);
	UFUNCTION() void OnTimerSubscribed(const UTclTimerEvent* Event);
	UFUNCTION() void OnTimerSubscribedOther(const UTclTimerEvent* Event);
	UFUNCTION() void OnTimerListenerFired(const UTclTimerEvent* Event);
	UFUNCTION() void OnInitialize();
	UFUNCTION() void OnCommandsChanged();
	UFUNCTION() void OnDefinitionsChanged();
	UFUNCTION() void OnIntValueChangedFromApp(int32 NewValue, int32 OldValue);
	UFUNCTION() void OnIntValueCommittedFromApp(int32 NewValue);
	UFUNCTION() void OnDefinitionComplete(ETclDefinitionResult Result);
	UFUNCTION() void OnConnectionStatusChanged(ETclConnectionStatus ConnectionStatus);

	// The Twitch and Kick API request nodes' pins. Get Streams records how many
	// streams it read, the first one's login, the next page's cursor, and the
	// status and message; the plain success records the status and message.
	UFUNCTION() void OnGetStreams(const FTclTwitchApiGetStreamsOutput& Response, int32 StatusCode, const FString& ErrorMessage);
	UFUNCTION() void OnPlatformRequestSucceeded(int32 StatusCode, const FString& ErrorMessage);
	UFUNCTION() void OnPlatformRequestFailed(int32 StatusCode, const FString& ErrorMessage);

	// The Sign In node's pins. Each records its pins as "url|code|message".
	UFUNCTION() void SignIn_OnPrompt(const FString& Url, const FString& UserCode, const FString& Message);
	UFUNCTION() void SignIn_OnConnected(const FString& Url, const FString& UserCode, const FString& Message);
	UFUNCTION() void SignIn_OnFailed(const FString& Url, const FString& UserCode, const FString& Message);

	// A sign-in controller's or Sign In Button's state, recorded as "Stage|Label".
	UFUNCTION() void OnSignInStateChanged(const FTclSignInState& State);

private:
	void Record(const FString& Name, const UObject* Event, const UTclViewer* Viewer, const FString& Detail = FString());
};

/**
 * What a control test double's value checks saw and answered.
 */
struct FTclVetoProbe
{
	/** Every check, in call order: "raw:<value>" for Should Receive Value, "typed:<value>" for the typed one. */
	TArray<FString> Calls;

	/** What Should Receive Value answers. */
	bool bAcceptRaw = true;

	/** What the typed Should Receive Value answers. */
	bool bAcceptTyped = true;

	/** Record a raw check and return its answer. */
	bool Raw(const FString& Value) { Calls.Add(TEXT("raw:") + Value); return bAcceptRaw; }

	/** Record a typed check and return its answer. */
	bool Typed(const FString& Value) { Calls.Add(TEXT("typed:") + Value); return bAcceptTyped; }
};

/** A Color control whose value checks are scripted by its probe. */
UCLASS(Transient, HideDropdown, NotBlueprintable)
class UTclTestControl_Color : public UTclCustomControlComponent_Color
{
	GENERATED_BODY()
public:
	FTclVetoProbe Probe;
	virtual bool ShouldReceiveValue_Implementation(const FString& NewValue) override { return Probe.Raw(NewValue); }
	virtual bool ShouldReceiveValue_Color_Implementation(const FColor& NewValue) override { return Probe.Typed(NewValue.ToHex()); }
};

/** An Int control whose value checks are scripted by its probe. */
UCLASS(Transient, HideDropdown, NotBlueprintable)
class UTclTestControl_Int : public UTclCustomControlComponent_Int
{
	GENERATED_BODY()
public:
	FTclVetoProbe Probe;
	virtual bool ShouldReceiveValue_Implementation(const FString& NewValue) override { return Probe.Raw(NewValue); }
	virtual bool ShouldReceiveValue_Int_Implementation(int32 NewValue) override { return Probe.Typed(FString::FromInt(NewValue)); }
};

/** A Number control whose value checks are scripted by its probe. */
UCLASS(Transient, HideDropdown, NotBlueprintable)
class UTclTestControl_Number : public UTclCustomControlComponent_Number
{
	GENERATED_BODY()
public:
	FTclVetoProbe Probe;
	virtual bool ShouldReceiveValue_Implementation(const FString& NewValue) override { return Probe.Raw(NewValue); }
	virtual bool ShouldReceiveValue_Float_Implementation(float NewValue) override { return Probe.Typed(FString::SanitizeFloat(NewValue)); }
};

/** A Select control whose value checks are scripted by its probe. */
UCLASS(Transient, HideDropdown, NotBlueprintable)
class UTclTestControl_Select : public UTclCustomControlComponent_Select
{
	GENERATED_BODY()
public:
	FTclVetoProbe Probe;
	virtual bool ShouldReceiveValue_Implementation(const FString& NewValue) override { return Probe.Raw(NewValue); }
	virtual bool ShouldReceiveValue_String_Implementation(const FString& NewValue) override { return Probe.Typed(NewValue); }
};

/** A Slider control whose value checks are scripted by its probe. */
UCLASS(Transient, HideDropdown, NotBlueprintable)
class UTclTestControl_Slider : public UTclCustomControlComponent_Slider
{
	GENERATED_BODY()
public:
	FTclVetoProbe Probe;
	virtual bool ShouldReceiveValue_Implementation(const FString& NewValue) override { return Probe.Raw(NewValue); }
	virtual bool ShouldReceiveValue_Float_Implementation(float NewValue) override { return Probe.Typed(FString::SanitizeFloat(NewValue)); }
};

/** A Switch control whose value checks are scripted by its probe. */
UCLASS(Transient, HideDropdown, NotBlueprintable)
class UTclTestControl_Switch : public UTclCustomControlComponent_Switch
{
	GENERATED_BODY()
public:
	FTclVetoProbe Probe;
	virtual bool ShouldReceiveValue_Implementation(const FString& NewValue) override { return Probe.Raw(NewValue); }
	virtual bool ShouldReceiveValue_Bool_Implementation(bool NewValue) override { return Probe.Typed(NewValue ? TEXT("true") : TEXT("false")); }
};

/** A Text control whose value checks are scripted by its probe. */
UCLASS(Transient, HideDropdown, NotBlueprintable)
class UTclTestControl_Text : public UTclCustomControlComponent_Text
{
	GENERATED_BODY()
public:
	FTclVetoProbe Probe;
	virtual bool ShouldReceiveValue_Implementation(const FString& NewValue) override { return Probe.Raw(NewValue); }
	virtual bool ShouldReceiveValue_String_Implementation(const FString& NewValue) override { return Probe.Typed(NewValue); }
};

/** A Vector2 control whose value checks are scripted by its probe. */
UCLASS(Transient, HideDropdown, NotBlueprintable)
class UTclTestControl_Vector2 : public UTclCustomControlComponent_Vector2
{
	GENERATED_BODY()
public:
	FTclVetoProbe Probe;
	virtual bool ShouldReceiveValue_Implementation(const FString& NewValue) override { return Probe.Raw(NewValue); }
	virtual bool ShouldReceiveValue_Vector2_Implementation(const FVector2D& NewValue) override { return Probe.Typed(FString::Printf(TEXT("%g,%g"), NewValue.X, NewValue.Y)); }
};

/** A Vector3 control whose value checks are scripted by its probe. */
UCLASS(Transient, HideDropdown, NotBlueprintable)
class UTclTestControl_Vector3 : public UTclCustomControlComponent_Vector3
{
	GENERATED_BODY()
public:
	FTclVetoProbe Probe;
	virtual bool ShouldReceiveValue_Implementation(const FString& NewValue) override { return Probe.Raw(NewValue); }
	virtual bool ShouldReceiveValue_Vector3_Implementation(const FVector& NewValue) override { return Probe.Typed(FString::Printf(TEXT("%g,%g,%g"), NewValue.X, NewValue.Y, NewValue.Z)); }
};

/** A Vector4 control whose value checks are scripted by its probe. */
UCLASS(Transient, HideDropdown, NotBlueprintable)
class UTclTestControl_Vector4 : public UTclCustomControlComponent_Vector4
{
	GENERATED_BODY()
public:
	FTclVetoProbe Probe;
	virtual bool ShouldReceiveValue_Implementation(const FString& NewValue) override { return Probe.Raw(NewValue); }
	virtual bool ShouldReceiveValue_Vector4_Implementation(const FVector4& NewValue) override { return Probe.Typed(FString::Printf(TEXT("%g,%g,%g,%g"), NewValue.X, NewValue.Y, NewValue.Z, NewValue.W)); }
};
