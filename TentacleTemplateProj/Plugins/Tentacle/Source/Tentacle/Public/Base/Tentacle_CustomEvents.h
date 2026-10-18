// © 2025 Eldritch One. All rights reserved.

#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Sdk/TclCustomEventDefinition.h"
#include "Tentacle_CustomEvents.generated.h"

class UTclViewer;
class UTclCustomEvent;
class UTclMsg_ServerToClient_CustomEventsEvent;
class UTclMsg_ServerToClient_CustomEventsDefinitions;

/**
 * A handler for one custom event name, passed to Subscribe. Event is the custom
 * event that fired; Viewer is the viewer who triggered it, or null when the
 * event names nobody.
 */
DECLARE_DYNAMIC_DELEGATE_TwoParams(FTclCustomEventHandler, const UTclCustomEvent*, Event, UTclViewer*, Viewer);

UCLASS()
class TENTACLE_API UTentacle_CustomEvents : public UActorComponent
{
	GENERATED_BODY()

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCustomEvent, const UTclCustomEvent*, Event, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDefinitionsChanged);

public:
	/** Fires for every custom event, whatever its name, before any handler added with Subscribe. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Custom Events")
	FOnCustomEvent OnCustomEvent;

	/**
	 * Fires whenever the custom event definitions change, including when the
	 * first ones arrive. Definitions already holds the new ones by the time it
	 * fires. Definitions Tentacle sends again unchanged, as it does every time
	 * the session opens, do not fire it.
	 */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Custom Events")
	FOnDefinitionsChanged OnDefinitionsChanged;

	/**
	 * Every custom event the streamer configured, enabled or not, ordered by
	 * name. Chat Commands on each lists the chat commands that fire it, without
	 * the command prefix: the commands a trigger requires exactly.
	 *
	 * Tentacle sends them as the session opens and again whenever one is
	 * created, changed or removed. They stay as they were while the session is
	 * closed.
	 */
	UPROPERTY(BlueprintReadOnly, Category="Tentacle|Custom Events")
	TArray<UTclCustomEventDefinition*> Definitions;

	/** Whether Tentacle has sent the definitions yet. Until it has, Definitions is empty. */
	UPROPERTY(BlueprintReadOnly, Category="Tentacle|Custom Events")
	bool bAreDefinitionsLoaded = false;

	/**
	 * The definition with this name. Names are matched case-insensitively by
	 * default, the same rule Subscribe uses; set Case Sensitive if you need
	 * exact matching. When two definitions share a name, the first in
	 * Definitions is the one found.
	 */
	UFUNCTION(BlueprintPure, Category="Tentacle|Custom Events")
	UTclCustomEventDefinition* FindDefinitionByName(const FString& Name, bool& Found, bool bCaseSensitive = false) const;

	/**
	 * Run Handler for every custom event with this name, after On Custom Event.
	 *
	 * Names are matched case-insensitively by default; set Case Sensitive if
	 * you need exact matching. Subscribing the same handler to the same name
	 * twice runs it once.
	 */
	UFUNCTION(BlueprintCallable, Category="Tentacle|Custom Events")
	void Subscribe(const FString& EventName, FTclCustomEventHandler Handler, bool bCaseSensitive = false);

	/**
	 * Stop running Handler for this name. The name is compared the same way its
	 * Subscribe call compares it: a case-insensitive subscription is removed
	 * whatever the letter case, a case-sensitive one only by its exact name.
	 */
	UFUNCTION(BlueprintCallable, Category="Tentacle|Custom Events")
	void Unsubscribe(const FString& EventName, FTclCustomEventHandler Handler);

	void HandleMsg_Event(UTclMsg_ServerToClient_CustomEventsEvent* Msg);

	/**
	 * Take the definitions a CustomEvents_Definitions message carries.
	 * DefinitionsJson is the message as received, which is what tells changed
	 * definitions from ones sent again.
	 */
	void HandleMsg_Definitions(UTclMsg_ServerToClient_CustomEventsDefinitions* Msg, const FString& DefinitionsJson);

private:
	/** The last definitions as received, so ones sent again unchanged fire nothing. */
	FString DefinitionsKey;

	// Kept out of the generated reference: Doxygen lists a nested type even from
	// a private section.
	/// @cond

	/** One Subscribe call: the name it asked for, how to compare it, and who to run. */
	struct FSubscription
	{
		FString EventName;
		FTclCustomEventHandler Handler;
		bool bCaseSensitive = false;

		/** Whether this subscription answers to Name, under its own letter-case rule. */
		bool Matches(const FString& Name) const;
	};

	TArray<FSubscription> Subscriptions;

	/// @endcond
};
