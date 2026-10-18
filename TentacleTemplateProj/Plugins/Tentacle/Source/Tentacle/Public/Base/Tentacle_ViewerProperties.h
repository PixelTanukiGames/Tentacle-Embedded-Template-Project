// © 2025 Eldritch One. All rights reserved.

#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Sdk/TclViewerPropertyType.h"
#include "Sdk/TclOrderDirection.h"
#include "Base/TclViewer.h"
#include "Base/TclDefinitionChange.h"
#include "Sdk/TclViewerPropertyCollection.h"
#include "Sdk/TclViewerPropertyDefinition.h"
#include "Tentacle_ViewerProperties.generated.h"

class FJsonObject;
class FJsonValue;

UCLASS()
class TENTACLE_API UTentacle_ViewerProperties : public UActorComponent
{
	GENERATED_BODY()

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInitialLoadComplete);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCollectionsRefreshed);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnFetchViewersByPropertyComplete, const UTclViewerPropertyCollection*, Collection, const TArray<UTclViewer*>&, Viewers, const FString&, PropertyName, ETclViewerPropertyType, PropertyType);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnViewerPropertyUpdated, const UTclViewerPropertyCollection*, Collection, const UTclViewer*, Viewer, const FString&, PropertyName, ETclViewerPropertyType, PropertyType);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnResetCollectionValuesComplete, const UTclViewerPropertyCollection*, Collection, bool, bSuccess);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnResetPropertyValuesComplete, const UTclViewerPropertyCollection*, Collection, const FString&, PropertyName, bool, bSuccess);

public:
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Viewer Properties")
	FOnInitialLoadComplete OnInitialLoadComplete;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Viewer Properties")
	FOnCollectionsRefreshed OnCollectionsRefreshed;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Viewer Properties")
	FOnFetchViewersByPropertyComplete OnFetchViewersByPropertyComplete;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Viewer Properties")
	FOnViewerPropertyUpdated OnViewerPropertyUpdated;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Viewer Properties")
	FOnResetCollectionValuesComplete OnResetCollectionValuesComplete;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Viewer Properties")
	FOnResetPropertyValuesComplete OnResetPropertyValuesComplete;

	// --- Collections + Definitions registry ---

	UPROPERTY(BlueprintReadOnly, Category="Tentacle|Viewer Properties")
	TArray<UTclViewerPropertyCollection*> Collections;

	/** definitionId -> definition metadata (name, type, default). Populated from
	 *  the registry alongside Collections so both become available together. */
	UPROPERTY()
	TMap<FString, UTclViewerPropertyDefinition*> DefinitionsById;

	UPROPERTY(BlueprintReadOnly, Category="Tentacle|Viewer Properties")
	bool bIsInitialLoadComplete = false;

	UFUNCTION(BlueprintPure, Category="Tentacle|Viewer Properties")
	UTclViewerPropertyCollection* FindCollectionByName(const FString& Name, bool& Found, bool bCaseSensitive = false) const;

	UFUNCTION(BlueprintCallable, Category="Tentacle|Viewer Properties")
	UTclViewerPropertyCollection* FindCollectionById(const FString& Id, bool& Found) const;

	/** Resolve a property definition by id (nullptr if the registry hasn't loaded
	 *  it yet). Used by the viewer merge to type each slim value. */
	UTclViewerPropertyDefinition* FindDefinitionById(const FString& Id) const;

	/** Resolve a property definition by its collection's id and its name (nullptr
	 *  if the registry doesn't know it). Name matching is case-insensitive by
	 *  default, like FindCollectionByName. */
	UTclViewerPropertyDefinition* FindDefinitionByName(const FString& CollectionId, const FString& Name, bool bCaseSensitive = false) const;

	/** True once the registry (collections + definitions) has loaded at least once. */
	bool AreDefinitionsLoaded() const { return bAreDefinitionsLoaded; }

	/** Populate both caches from a fetched registry, then re-merge all viewers so
	 *  anything that arrived before the registry resolves. Public so it can be
	 *  driven directly (e.g. in tests) without the HTTP round-trip. */
	void ApplyRegistry(const TArray<UTclViewerPropertyCollection*>& InCollections, const TArray<UTclViewerPropertyDefinition*>& InDefinitions);

	/** Request a single, debounced registry refresh because `UnknownDefinitionId`
	 *  was seen on a viewer but isn't cached (e.g. a definition created at runtime).
	 *  Each distinct id triggers at most one refresh for the session, so this can
	 *  never spin the backend. */
	void RequestRegistryRefresh(const FString& UnknownDefinitionId);

	/** Make sure a registry load is on its way, for a read or write that arrived
	 *  before the first one finished. Single-flight like RequestRegistryRefresh,
	 *  and a no-op once the registry is loaded, so a game polling a property in
	 *  Tick cannot turn a slow first load into a flood of requests. */
	void RequestRegistryLoad();

	UFUNCTION(BlueprintCallable, Category="Tentacle|Viewer Properties")
	void FetchViewersByDefinition(const FString& CollectionName, const FString& PropertyName, ETclViewerPropertyType PropertyType, ETclOrderDirection OrderDirection, int32 Take);

	/** Reset a collection: delete every per-viewer override across all of the
	 *  collection's definitions, reverting every viewer to each definition's
	 *  default value. Definitions and their default values are left untouched.
	 *  On success every locally cached viewer reverts immediately (firing
	 *  OnViewerPropertyUpdated per changed value); OnResetCollectionValuesComplete
	 *  then fires exactly once with the outcome. */
	UFUNCTION(BlueprintCallable, Category="Tentacle|Viewer Properties")
	void ResetCollectionValues(const FString& CollectionName);

	/** Reset a single property: delete every per-viewer override for the named
	 *  property in the collection, reverting every viewer to the property's
	 *  default value. The property definition, its default value, and the
	 *  collection's other properties are left untouched. On success every
	 *  locally cached viewer reverts immediately (firing OnViewerPropertyUpdated
	 *  per changed value); OnResetPropertyValuesComplete then fires exactly once
	 *  with the outcome. */
	UFUNCTION(BlueprintCallable, Category="Tentacle|Viewer Properties")
	void ResetPropertyValues(const FString& CollectionName, const FString& PropertyName);

	// --- Changing the definitions ---
	//
	// Each names what it acts on through the registry, under the usual case rule,
	// and sends Tentacle its id. After a change is made the registry is reloaded
	// before On Complete runs, so a name the change introduced resolves by then.

	/**
	 * Create a collection. Configuring viewer properties in Tentacle and shipping
	 * its export is the easier way to give a game the properties it needs; this
	 * is for ones the game decides on as it runs. On Complete runs once with
	 * Success, Name Taken when another collection has the name, or Failed (the
	 * reason is logged).
	 */
	UFUNCTION(BlueprintCallable, Category="Tentacle|Viewer Properties", meta=(AutoCreateRefTerm="OnComplete"))
	void CreateCollection(const FString& CollectionName, const FTclOnDefinitionComplete& OnComplete);

	/**
	 * Rename a collection. On Complete runs once with Success, Not Found, Name
	 * Taken when another collection has the new name, or Failed.
	 */
	UFUNCTION(BlueprintCallable, Category="Tentacle|Viewer Properties", meta=(AutoCreateRefTerm="OnComplete"))
	void RenameCollection(const FString& CollectionName, const FString& NewName, const FTclOnDefinitionComplete& OnComplete, bool bCaseSensitive = false);

	/**
	 * Delete a collection, its properties, and every viewer's values for them.
	 * On Complete runs once with Success, Not Found, or Failed.
	 */
	UFUNCTION(BlueprintCallable, Category="Tentacle|Viewer Properties", meta=(AutoCreateRefTerm="OnComplete"))
	void DeleteCollection(const FString& CollectionName, const FTclOnDefinitionComplete& OnComplete, bool bCaseSensitive = false);

	/**
	 * Create a whole-number property in a collection, which every viewer has at
	 * Default Value until a value of their own is written. On Complete runs once
	 * with Success, Not Found when there is no such collection, Name Taken when
	 * the collection already has a property by that name, or Failed.
	 */
	UFUNCTION(BlueprintCallable, Category="Tentacle|Viewer Properties", meta=(AutoCreateRefTerm="OnComplete"))
	void CreateProperty_Int(const FString& CollectionName, const FString& PropertyName, int64 DefaultValue, const FTclOnDefinitionComplete& OnComplete, bool bCaseSensitive = false);

	/** Create a floating-point property in a collection; see Create Property (Int). */
	UFUNCTION(BlueprintCallable, Category="Tentacle|Viewer Properties", meta=(AutoCreateRefTerm="OnComplete"))
	void CreateProperty_Float(const FString& CollectionName, const FString& PropertyName, double DefaultValue, const FTclOnDefinitionComplete& OnComplete, bool bCaseSensitive = false);

	/** Create a boolean property in a collection; see Create Property (Int). */
	UFUNCTION(BlueprintCallable, Category="Tentacle|Viewer Properties", meta=(AutoCreateRefTerm="OnComplete"))
	void CreateProperty_Bool(const FString& CollectionName, const FString& PropertyName, bool DefaultValue, const FTclOnDefinitionComplete& OnComplete, bool bCaseSensitive = false);

	/** Create a string property in a collection; see Create Property (Int). */
	UFUNCTION(BlueprintCallable, Category="Tentacle|Viewer Properties", meta=(AutoCreateRefTerm="OnComplete"))
	void CreateProperty_String(const FString& CollectionName, const FString& PropertyName, const FString& DefaultValue, const FTclOnDefinitionComplete& OnComplete, bool bCaseSensitive = false);

	/**
	 * Change a whole-number property's default, which every viewer without a
	 * value of their own then has. On Complete runs once with Success, Not
	 * Found, or Failed, including when the property is not a whole-number one
	 * (the log says which type it is).
	 */
	UFUNCTION(BlueprintCallable, Category="Tentacle|Viewer Properties", meta=(AutoCreateRefTerm="OnComplete"))
	void SetPropertyDefault_Int(const FString& CollectionName, const FString& PropertyName, int64 DefaultValue, const FTclOnDefinitionComplete& OnComplete, bool bCaseSensitive = false);

	/** Change a floating-point property's default; see Set Property Default (Int). */
	UFUNCTION(BlueprintCallable, Category="Tentacle|Viewer Properties", meta=(AutoCreateRefTerm="OnComplete"))
	void SetPropertyDefault_Float(const FString& CollectionName, const FString& PropertyName, double DefaultValue, const FTclOnDefinitionComplete& OnComplete, bool bCaseSensitive = false);

	/** Change a boolean property's default; see Set Property Default (Int). */
	UFUNCTION(BlueprintCallable, Category="Tentacle|Viewer Properties", meta=(AutoCreateRefTerm="OnComplete"))
	void SetPropertyDefault_Bool(const FString& CollectionName, const FString& PropertyName, bool DefaultValue, const FTclOnDefinitionComplete& OnComplete, bool bCaseSensitive = false);

	/** Change a string property's default; see Set Property Default (Int). */
	UFUNCTION(BlueprintCallable, Category="Tentacle|Viewer Properties", meta=(AutoCreateRefTerm="OnComplete"))
	void SetPropertyDefault_String(const FString& CollectionName, const FString& PropertyName, const FString& DefaultValue, const FTclOnDefinitionComplete& OnComplete, bool bCaseSensitive = false);

	/**
	 * Rename a property, keeping every viewer's value for it. On Complete runs
	 * once with Success, Not Found, Name Taken when the collection has another
	 * property by the new name, or Failed.
	 */
	UFUNCTION(BlueprintCallable, Category="Tentacle|Viewer Properties", meta=(AutoCreateRefTerm="OnComplete"))
	void RenameProperty(const FString& CollectionName, const FString& PropertyName, const FString& NewName, const FTclOnDefinitionComplete& OnComplete, bool bCaseSensitive = false);

	/**
	 * Delete a property and every viewer's value for it. On Complete runs once
	 * with Success, Not Found, or Failed.
	 */
	UFUNCTION(BlueprintCallable, Category="Tentacle|Viewer Properties", meta=(AutoCreateRefTerm="OnComplete"))
	void DeleteProperty(const FString& CollectionName, const FString& PropertyName, const FTclOnDefinitionComplete& OnComplete, bool bCaseSensitive = false);

	/** Fetch the registry now and run OnDone once it is applied, or once the
	 *  fetch has failed (its retries carry on without OnDone). Used after a
	 *  definition change, so the change's completion sees the new registry. */
	void ReloadRegistry(TFunction<void()> OnDone);

	// --- Internal ---

	void PostInitialize();
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	void SendViewerPropertyUpdate(const FString& CollectionName, const FString& PropertyName, const FString& ViewerId, double Value);
	void SendViewerPropertyUpdate(const FString& CollectionName, const FString& PropertyName, const FString& ViewerId, bool Value);
	void SendViewerPropertyUpdate(const FString& CollectionName, const FString& PropertyName, const FString& ViewerId, const FString& Value);

	/** Send a viewer's local write of one property to Tentacle. Written is the
	 *  viewer's entry as the write left it; if Tentacle does not take the write
	 *  (not running, or the request refused), the write's hold on that entry is
	 *  released (see UTclViewer::ReleaseLocalWrite) so the next snapshot applies. */
	void SendViewerPropertyWrite(UTclViewer* Viewer, const FString& CollectionName, const FString& PropertyName, const TSharedRef<FJsonValue>& Value, const FTclViewerPropertyEntry& Written);

private:
	FTimerHandle RefreshRetryTimerHandle;
	int32 RefreshRetryCount = 0;

	/** Set by EndPlay, after which a failed registry fetch is not retried. */
	bool bEndedPlay = false;

	/** The property-update subscriptions made on each viewer, removed at EndPlay. */
	TArray<TPair<TWeakObjectPtr<UTclViewer>, TTclEventSubject<FTclViewerPropertyUpdatedData>::FHandle>> ViewerSubscriptions;

	bool bAreDefinitionsLoaded = false;
	FTimerHandle RegistryRefreshDebounceTimer;
	bool bRegistryRefreshPending = false;
	/** Definition ids we've already requested a refresh for — bounds the
	 *  runtime-definition refresh to one per id, so it can never loop. */
	TSet<FString> RequestedDefinitionRefreshIds;

	void RefreshRegistry();

	/** Fetch the registry and apply it; OnDone, when set, runs after the apply or on failure. */
	void FetchRegistry(TFunction<void()> OnDone);

	/** Arm the debounced, single-flight refresh both request paths share. */
	void ScheduleRegistryRefresh();
	/** Send one setForViewer entry. OnRefused, when set, runs if the write does not reach Tentacle. */
	void SendViewerPropertyUpdateInternal(const TSharedRef<FJsonObject>& EntryObj, TFunction<void()> OnRefused = nullptr);

	UFUNCTION()
	void HandleViewerAdded(const UTclViewer* Viewer);
};
