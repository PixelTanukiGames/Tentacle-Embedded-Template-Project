// © 2025 Eldritch One. All rights reserved.

#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Base/TclDefinitionChange.h"
#include "Base/TclTimerSettings.h"
#include "Sdk/TclTimerDefinition.h"
#include "Tentacle_Timers.generated.h"

class UTclTimerEvent;
class UTclMsg_ServerToClient_TimersFired;
class UTclMsg_ServerToClient_TimersDefinitions;

/**
 * A handler for one timer's name, passed to Subscribe. Event is the fire: which
 * timer, how many times it has fired, and how long until the next.
 */
DECLARE_DYNAMIC_DELEGATE_OneParam(FTclTimerHandler, const UTclTimerEvent*, Event);

/**
 * The timers the streamer defined in Tentacle, firing at the game, and the
 * definitions behind them.
 *
 * A timer fires on a schedule the streamer sets in Tentacle - a fixed interval
 * or a random one from a range, repeating or once, only while the stream is
 * live if they like, and only after enough chat - so the game reacts to one
 * name and keeps no clock of its own. The streamer starts, pauses, and resets
 * timers in Tentacle, and the game can too, and define its own: see Create
 * Timer.
 *
 * Tentacle sends every timer's definition as the session opens, and again
 * whenever one is created, changed, removed, started, paused, or stops after
 * firing once. The definitions stay as they were while the session is closed.
 */
UCLASS()
class TENTACLE_API UTentacle_Timers : public UActorComponent
{
	GENERATED_BODY()

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTimerFired, const UTclTimerEvent*, Event);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDefinitionsChanged);

public:
	/** Fires for every timer, whatever its name, before any handler added with Subscribe. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Timers")
	FOnTimerFired OnTimerFired;

	/**
	 * Fires whenever the timer definitions change, including when the first
	 * ones arrive. Definitions already holds the new ones by the time it fires.
	 * Definitions Tentacle sends again unchanged, as it does every time the
	 * session opens, do not fire it.
	 */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Timers")
	FOnDefinitionsChanged OnDefinitionsChanged;

	/** Every timer the streamer defined, ordered by name, with whether it is counting down. */
	UPROPERTY(BlueprintReadOnly, Category="Tentacle|Timers")
	TArray<UTclTimerDefinition*> Definitions;

	/** Whether Tentacle has sent the definitions yet. Until it has, Definitions is empty. */
	UPROPERTY(BlueprintReadOnly, Category="Tentacle|Timers")
	bool bAreDefinitionsLoaded = false;

	/**
	 * The timer with this name. Names are matched case-insensitively by
	 * default, the same rule Subscribe uses; set Case Sensitive if you need
	 * exact matching.
	 */
	UFUNCTION(BlueprintPure, Category="Tentacle|Timers")
	UTclTimerDefinition* FindDefinitionByName(const FString& Name, bool& Found, bool bCaseSensitive = false) const;

	/**
	 * Run Handler every time the timer with this name fires, after On Timer
	 * Fired.
	 *
	 * Names are matched case-insensitively by default; set Case Sensitive if
	 * you need exact matching. Subscribing the same handler to the same name
	 * twice runs it once.
	 */
	UFUNCTION(BlueprintCallable, Category="Tentacle|Timers")
	void Subscribe(const FString& TimerName, FTclTimerHandler Handler, bool bCaseSensitive = false);

	/**
	 * Stop running Handler for this name. The name is compared the same way its
	 * Subscribe call compares it: a case-insensitive subscription is removed
	 * whatever the letter case, a case-sensitive one only by its exact name.
	 */
	UFUNCTION(BlueprintCallable, Category="Tentacle|Timers")
	void Unsubscribe(const FString& TimerName, FTclTimerHandler Handler);

	/**
	 * Create a timer, stopped until Start Timer starts it.
	 *
	 * Configuring a timer in Tentacle and shipping its export is the easier way
	 * to give a game the timers it needs; this is for timers the game decides on
	 * as it runs. On Complete runs once with Success, Name Taken when another
	 * timer has the name, or Failed (the reason is logged).
	 */
	UFUNCTION(BlueprintCallable, Category="Tentacle|Timers", meta=(AutoCreateRefTerm="OnComplete"))
	void CreateTimer(const FTclTimerSettings& Settings, const FTclOnDefinitionComplete& OnComplete);

	/**
	 * Change how a timer runs.
	 *
	 * Every setting is sent, so start from the timer's definition to change only
	 * one; an empty name in Settings keeps the timer's own. A new interval starts
	 * its countdown over. On Complete runs once with Success, Not Found, Name
	 * Taken when renaming it to a name another timer has, or Failed. Names are
	 * matched case-insensitively by default.
	 */
	UFUNCTION(BlueprintCallable, Category="Tentacle|Timers", meta=(AutoCreateRefTerm="OnComplete"))
	void UpdateTimer(const FString& TimerName, const FTclTimerSettings& Settings, const FTclOnDefinitionComplete& OnComplete, bool bCaseSensitive = false);

	/** Delete a timer. On Complete runs once with Success, Not Found, or Failed. */
	UFUNCTION(BlueprintCallable, Category="Tentacle|Timers", meta=(AutoCreateRefTerm="OnComplete"))
	void DeleteTimer(const FString& TimerName, const FTclOnDefinitionComplete& OnComplete, bool bCaseSensitive = false);

	/**
	 * Start a timer: resume a paused countdown, or begin a fresh one. A running
	 * timer is left as it is. On Complete runs once with Success, Not Found, or
	 * Failed.
	 */
	UFUNCTION(BlueprintCallable, Category="Tentacle|Timers", meta=(AutoCreateRefTerm="OnComplete"))
	void StartTimer(const FString& TimerName, const FTclOnDefinitionComplete& OnComplete, bool bCaseSensitive = false);

	/**
	 * Pause a running timer, keeping what its countdown has left. On Complete
	 * runs once with Success, Not Found, or Failed.
	 */
	UFUNCTION(BlueprintCallable, Category="Tentacle|Timers", meta=(AutoCreateRefTerm="OnComplete"))
	void PauseTimer(const FString& TimerName, const FTclOnDefinitionComplete& OnComplete, bool bCaseSensitive = false);

	/**
	 * Start a timer's countdown over, leaving it running or paused as it was. On
	 * Complete runs once with Success, Not Found, or Failed.
	 */
	UFUNCTION(BlueprintCallable, Category="Tentacle|Timers", meta=(AutoCreateRefTerm="OnComplete"))
	void ResetTimer(const FString& TimerName, const FTclOnDefinitionComplete& OnComplete, bool bCaseSensitive = false);

	/**
	 * Fire a timer now, as its countdown ending would, whatever its chat
	 * minimum: On Timer Fired fires for it everywhere. A repeating timer then
	 * starts its countdown over; one that does not stops. On Complete runs once
	 * with Success, Not Found, or Failed.
	 */
	UFUNCTION(BlueprintCallable, Category="Tentacle|Timers", meta=(AutoCreateRefTerm="OnComplete"))
	void FireTimer(const FString& TimerName, const FTclOnDefinitionComplete& OnComplete, bool bCaseSensitive = false);

	void HandleMsg_Fired(UTclMsg_ServerToClient_TimersFired* Msg);

	/**
	 * Take the definitions a Timers_Definitions message carries.
	 * DefinitionsJson is the message as received, which is what tells changed
	 * definitions from ones sent again.
	 */
	void HandleMsg_Definitions(UTclMsg_ServerToClient_TimersDefinitions* Msg, const FString& DefinitionsJson);

private:
	/** The last definitions as received, so ones sent again unchanged fire nothing. */
	FString DefinitionsKey;

	// Kept out of the generated reference: Doxygen lists a nested type even from
	// a private section.
	/// @cond

	/** One Subscribe call: the name it asked for, how to compare it, and who to run. */
	struct FSubscription
	{
		FString TimerName;
		FTclTimerHandler Handler;
		bool bCaseSensitive = false;

		/** Whether this subscription answers to Name, under its own letter-case rule. */
		bool Matches(const FString& Name) const;
	};

	TArray<FSubscription> Subscriptions;

	/// @endcond
};
