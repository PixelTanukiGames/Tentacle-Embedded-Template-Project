// © 2025 Eldritch One. All rights reserved.

#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Tentacle_Viewers.generated.h"

class UTclViewer;
class UTclViewerData;
class FJsonObject;

/**
 * The viewer cache: every viewer Tentacle has told this session about, keyed by
 * viewer id.
 *
 * A viewer is one object for the whole session rather than one per message, so
 * an actor spawned for a chatter keeps receiving that chatter's updates, and
 * property values written to a viewer stay on the viewer the rest of the game
 * holds. Every stream event, chat join and custom event hands over the cached
 * viewer.
 *
 * Features:
 * - The same viewer id always gives back the same viewer
 * - On Viewer Added fires once per viewer, never when a known viewer's data is refreshed
 * - Unbounded on purpose: viewers are only removed when the game removes them
 */
UCLASS()
class TENTACLE_API UTentacle_Viewers : public UActorComponent
{
	GENERATED_BODY()

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnViewerAdded, const UTclViewer*, Viewer);

public:
	/**
	 * Fires once per viewer, the first time Tentacle tells this session about
	 * them; never when a known viewer's data is refreshed. It fires right after
	 * the Tentacle actor's own On Viewer Added.
	 */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Viewers")
	FOnViewerAdded OnViewerAdded;

	/** The cached viewer with this id, and whether there is one. */
	UFUNCTION(BlueprintPure, Category="Tentacle|Viewers")
	UTclViewer*
	FindViewerById(const FString& ViewerId, bool& Found) const;

	/**
	 * Forget one viewer. Anything already holding the viewer keeps it; the next
	 * event from that viewer creates a fresh one and fires On Viewer Added again.
	 */
	UFUNCTION(BlueprintCallable, Category="Tentacle|Viewers")
	void
	DestroyViewer(const FString& ViewerId);

	/** Forget every viewer, for a game that runs long enough to want to prune the cache. */
	UFUNCTION(BlueprintCallable, Category="Tentacle|Viewers")
	void
	DestroyAllViewers();

	/** Every cached viewer, in no particular order. */
	UFUNCTION(BlueprintPure, Category="Tentacle|Viewers")
	TArray<UTclViewer*>
	GetAllViewers() const;

	/** How many viewers are cached. */
	UFUNCTION(BlueprintPure, Category="Tentacle|Viewers")
	int32
	GetViewerCount() const;

	/**
	 * Merge a viewer payload into the cache and return the viewer: the cached one
	 * updated with the new data, or a new one (firing On Viewer Added) the first
	 * time this id is seen. Null for a payload with no id.
	 */
	UTclViewer*
	UpsertViewer(UTclViewerData* ViewerData);

	/** Merge a viewer payload that arrived as raw JSON. Null for a missing or unreadable payload. */
	UTclViewer*
	UpsertViewer(const TSharedPtr<FJsonObject>& ViewerJson);

private:
	UPROPERTY(Transient)
	TMap<FString, TObjectPtr<UTclViewer>> ViewersById;
};
