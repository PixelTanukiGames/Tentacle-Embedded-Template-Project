// © 2025 Eldritch One. All rights reserved.

#include "Base/Tentacle_ViewerProperties.h"
#include "Base/Tentacle.h"
#include "Base/TclDefinitionChangeInternal.h"
#include "Sdk/TclHttpClient.h"
#include "Sdk/TclViewerPropertyCollection.h"
#include "Sdk/TclViewerPropertyDefinition.h"
#include "Sdk/TclViewerData.h"
#include "Utilities/TclCommon.h"
#include "Utilities/TclEnumUtils.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "TimerManager.h"

namespace
{
	/** The HTTP client of the Tentacle actor that owns these properties, or null. */
	UTentacleHttpClient* PropertiesHttpClient(const UTentacle_ViewerProperties& Properties)
	{
		const ATentacle* Tentacle = Properties.GetOwner<ATentacle>();
		return Tentacle ? Tentacle->HttpClient : nullptr;
	}

	/**
	 * The collection with this name, or null after running OnComplete with why:
	 * Not Found when there is no such collection, Failed when the registry has
	 * not loaded, which says nothing about the name (a load is requested, as a
	 * read would).
	 */
	UTclViewerPropertyCollection* ResolveCollectionForChange(
		UTentacle_ViewerProperties& Properties, const FString& CollectionName, bool bCaseSensitive, const FTclOnDefinitionComplete& OnComplete)
	{
		if (!Properties.AreDefinitionsLoaded())
		{
			tclWarnF("Tentacle ViewerProperties: the registry has not loaded yet, so the collection '%s' cannot be found; a load has been requested", *CollectionName);
			Properties.RequestRegistryLoad();
			FTclDefinitionChange::Finish(OnComplete, ETclDefinitionResult::Failed);
			return nullptr;
		}

		bool bFound = false;
		UTclViewerPropertyCollection* Collection = Properties.FindCollectionByName(CollectionName, bFound, bCaseSensitive);
		if (!Collection)
			FTclDefinitionChange::Finish(OnComplete, ETclDefinitionResult::NotFound);
		return Collection;
	}

	/** The property with this name in this collection, or null after running OnComplete with why, as for a collection. */
	UTclViewerPropertyDefinition* ResolvePropertyForChange(
		UTentacle_ViewerProperties& Properties, const FString& CollectionName, const FString& PropertyName, bool bCaseSensitive, const FTclOnDefinitionComplete& OnComplete)
	{
		const UTclViewerPropertyCollection* Collection = ResolveCollectionForChange(Properties, CollectionName, bCaseSensitive, OnComplete);
		if (!Collection)
			return nullptr;

		UTclViewerPropertyDefinition* Definition = Properties.FindDefinitionByName(Collection->Id, PropertyName, bCaseSensitive);
		if (!Definition)
			FTclDefinitionChange::Finish(OnComplete, ETclDefinitionResult::NotFound);
		return Definition;
	}

	/**
	 * Make one change, and once it is made reload the registry before reporting
	 * it. A reload that fails keeps the registry the game had and is logged; the
	 * change was still made, so it still reports Success.
	 */
	void ChangeThenReload(
		UTentacle_ViewerProperties& Properties, FTclDefinitionChange::FCall Call, const TCHAR* Procedure,
		const TSharedRef<FJsonObject>& Input, const FTclOnDefinitionComplete& OnComplete)
	{
		const TWeakObjectPtr<UTentacle_ViewerProperties> WeakProperties(&Properties);
		FTclDefinitionChange::Change(PropertiesHttpClient(Properties), Call, Procedure, Input,
			[WeakProperties, OnComplete](ETclDefinitionResult Result)
			{
				UTentacle_ViewerProperties* Self = WeakProperties.Get();
				if (Result != ETclDefinitionResult::Success || !Self)
				{
					FTclDefinitionChange::Finish(OnComplete, Result);
					return;
				}
				Self->ReloadRegistry([OnComplete]() { FTclDefinitionChange::Finish(OnComplete, ETclDefinitionResult::Success); });
			});
	}

	/** Create a property of one type (as Tentacle spells it) in a collection named through the registry. */
	void CreatePropertyOfType(
		UTentacle_ViewerProperties& Properties, const FString& CollectionName, const FString& PropertyName,
		const TCHAR* Type, const TSharedRef<FJsonValue>& DefaultValue, const FTclOnDefinitionComplete& OnComplete, bool bCaseSensitive)
	{
		const UTclViewerPropertyCollection* Collection = ResolveCollectionForChange(Properties, CollectionName, bCaseSensitive, OnComplete);
		if (!Collection)
			return;

		const TSharedRef<FJsonObject> Input = MakeShared<FJsonObject>();
		Input->SetStringField(TEXT("collectionId"), Collection->Id);
		Input->SetStringField(TEXT("name"), PropertyName);
		Input->SetStringField(TEXT("type"), Type);
		Input->SetField(TEXT("defaultValue"), DefaultValue);
		ChangeThenReload(Properties, &UTentacleHttpClient::CreateDefinition, TEXT("viewerProperty.createDefinition"), Input, OnComplete);
	}

	/**
	 * Change a property's default, after checking it is of the type the caller
	 * wrote it as: Tentacle would store a value of another type as given.
	 */
	void SetDefaultOfType(
		UTentacle_ViewerProperties& Properties, const FString& CollectionName, const FString& PropertyName,
		ETclViewerPropertyType Type, const TSharedRef<FJsonValue>& DefaultValue, const FTclOnDefinitionComplete& OnComplete, bool bCaseSensitive)
	{
		const UTclViewerPropertyDefinition* Definition = ResolvePropertyForChange(Properties, CollectionName, PropertyName, bCaseSensitive, OnComplete);
		if (!Definition)
			return;

		if (Definition->Type != Type)
		{
			tclWarnF("Tentacle ViewerProperties: '%s/%s' is of type %s, so its default cannot be set as %s; nothing changed",
				*CollectionName, *PropertyName, *UEnum::GetDisplayValueAsText(Definition->Type).ToString(), *UEnum::GetDisplayValueAsText(Type).ToString());
			FTclDefinitionChange::Finish(OnComplete, ETclDefinitionResult::Failed);
			return;
		}

		const TSharedRef<FJsonObject> Input = MakeShared<FJsonObject>();
		Input->SetStringField(TEXT("definitionId"), Definition->Id);
		Input->SetField(TEXT("defaultValue"), DefaultValue);
		ChangeThenReload(Properties, &UTentacleHttpClient::UpdateDefinition, TEXT("viewerProperty.updateDefinition"), Input, OnComplete);
	}
}

void
UTentacle_ViewerProperties::CreateCollection(const FString& CollectionName, const FTclOnDefinitionComplete& OnComplete)
{
	const TSharedRef<FJsonObject> Input = MakeShared<FJsonObject>();
	Input->SetStringField(TEXT("name"), CollectionName);
	ChangeThenReload(*this, &UTentacleHttpClient::CreateCollection, TEXT("viewerProperty.createCollection"), Input, OnComplete);
}

void
UTentacle_ViewerProperties::RenameCollection(const FString& CollectionName, const FString& NewName, const FTclOnDefinitionComplete& OnComplete, bool bCaseSensitive)
{
	const UTclViewerPropertyCollection* Collection = ResolveCollectionForChange(*this, CollectionName, bCaseSensitive, OnComplete);
	if (!Collection)
		return;

	const TSharedRef<FJsonObject> Input = MakeShared<FJsonObject>();
	Input->SetStringField(TEXT("collectionId"), Collection->Id);
	Input->SetStringField(TEXT("name"), NewName);
	ChangeThenReload(*this, &UTentacleHttpClient::RenameCollection, TEXT("viewerProperty.renameCollection"), Input, OnComplete);
}

void
UTentacle_ViewerProperties::DeleteCollection(const FString& CollectionName, const FTclOnDefinitionComplete& OnComplete, bool bCaseSensitive)
{
	const UTclViewerPropertyCollection* Collection = ResolveCollectionForChange(*this, CollectionName, bCaseSensitive, OnComplete);
	if (!Collection)
		return;

	const TSharedRef<FJsonObject> Input = MakeShared<FJsonObject>();
	Input->SetStringField(TEXT("collectionId"), Collection->Id);
	ChangeThenReload(*this, &UTentacleHttpClient::DeleteCollection, TEXT("viewerProperty.deleteCollection"), Input, OnComplete);
}

void
UTentacle_ViewerProperties::CreateProperty_Int(const FString& CollectionName, const FString& PropertyName, int64 DefaultValue, const FTclOnDefinitionComplete& OnComplete, bool bCaseSensitive)
{
	CreatePropertyOfType(*this, CollectionName, PropertyName, TEXT("int"), MakeShared<FJsonValueNumber>(static_cast<double>(DefaultValue)), OnComplete, bCaseSensitive);
}

void
UTentacle_ViewerProperties::CreateProperty_Float(const FString& CollectionName, const FString& PropertyName, double DefaultValue, const FTclOnDefinitionComplete& OnComplete, bool bCaseSensitive)
{
	CreatePropertyOfType(*this, CollectionName, PropertyName, TEXT("float"), MakeShared<FJsonValueNumber>(DefaultValue), OnComplete, bCaseSensitive);
}

void
UTentacle_ViewerProperties::CreateProperty_Bool(const FString& CollectionName, const FString& PropertyName, bool DefaultValue, const FTclOnDefinitionComplete& OnComplete, bool bCaseSensitive)
{
	CreatePropertyOfType(*this, CollectionName, PropertyName, TEXT("bool"), MakeShared<FJsonValueBoolean>(DefaultValue), OnComplete, bCaseSensitive);
}

void
UTentacle_ViewerProperties::CreateProperty_String(const FString& CollectionName, const FString& PropertyName, const FString& DefaultValue, const FTclOnDefinitionComplete& OnComplete, bool bCaseSensitive)
{
	CreatePropertyOfType(*this, CollectionName, PropertyName, TEXT("string"), MakeShared<FJsonValueString>(DefaultValue), OnComplete, bCaseSensitive);
}

void
UTentacle_ViewerProperties::SetPropertyDefault_Int(const FString& CollectionName, const FString& PropertyName, int64 DefaultValue, const FTclOnDefinitionComplete& OnComplete, bool bCaseSensitive)
{
	SetDefaultOfType(*this, CollectionName, PropertyName, ETclViewerPropertyType::Int, MakeShared<FJsonValueNumber>(static_cast<double>(DefaultValue)), OnComplete, bCaseSensitive);
}

void
UTentacle_ViewerProperties::SetPropertyDefault_Float(const FString& CollectionName, const FString& PropertyName, double DefaultValue, const FTclOnDefinitionComplete& OnComplete, bool bCaseSensitive)
{
	SetDefaultOfType(*this, CollectionName, PropertyName, ETclViewerPropertyType::Float, MakeShared<FJsonValueNumber>(DefaultValue), OnComplete, bCaseSensitive);
}

void
UTentacle_ViewerProperties::SetPropertyDefault_Bool(const FString& CollectionName, const FString& PropertyName, bool DefaultValue, const FTclOnDefinitionComplete& OnComplete, bool bCaseSensitive)
{
	SetDefaultOfType(*this, CollectionName, PropertyName, ETclViewerPropertyType::Bool, MakeShared<FJsonValueBoolean>(DefaultValue), OnComplete, bCaseSensitive);
}

void
UTentacle_ViewerProperties::SetPropertyDefault_String(const FString& CollectionName, const FString& PropertyName, const FString& DefaultValue, const FTclOnDefinitionComplete& OnComplete, bool bCaseSensitive)
{
	SetDefaultOfType(*this, CollectionName, PropertyName, ETclViewerPropertyType::String, MakeShared<FJsonValueString>(DefaultValue), OnComplete, bCaseSensitive);
}

void
UTentacle_ViewerProperties::RenameProperty(const FString& CollectionName, const FString& PropertyName, const FString& NewName, const FTclOnDefinitionComplete& OnComplete, bool bCaseSensitive)
{
	const UTclViewerPropertyDefinition* Definition = ResolvePropertyForChange(*this, CollectionName, PropertyName, bCaseSensitive, OnComplete);
	if (!Definition)
		return;

	const TSharedRef<FJsonObject> Input = MakeShared<FJsonObject>();
	Input->SetStringField(TEXT("definitionId"), Definition->Id);
	Input->SetStringField(TEXT("name"), NewName);
	ChangeThenReload(*this, &UTentacleHttpClient::RenameDefinition, TEXT("viewerProperty.renameDefinition"), Input, OnComplete);
}

void
UTentacle_ViewerProperties::DeleteProperty(const FString& CollectionName, const FString& PropertyName, const FTclOnDefinitionComplete& OnComplete, bool bCaseSensitive)
{
	const UTclViewerPropertyDefinition* Definition = ResolvePropertyForChange(*this, CollectionName, PropertyName, bCaseSensitive, OnComplete);
	if (!Definition)
		return;

	const TSharedRef<FJsonObject> Input = MakeShared<FJsonObject>();
	Input->SetStringField(TEXT("definitionId"), Definition->Id);
	ChangeThenReload(*this, &UTentacleHttpClient::DeleteDefinition, TEXT("viewerProperty.deleteDefinition"), Input, OnComplete);
}

void
UTentacle_ViewerProperties::PostInitialize()
{
	auto* Tentacle = GetOwner<ATentacle>();
	tclSoftAssert(Tentacle);

	// Tentacle runs locally now (no auth gate) — fetch the registry (collections +
	// definitions) immediately. Embedded, its procedures only travel once a
	// session is open, and the actor fetches it then.
	if (!Tentacle->IsEmbedded())
		RefreshRegistry();

	// Re-broadcast viewer property updates
	Tentacle->OnViewerAdded.AddDynamic(this, &UTentacle_ViewerProperties::HandleViewerAdded);
}

void
UTentacle_ViewerProperties::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	bEndedPlay = true;

	// A viewer the game still holds would otherwise keep calling into this
	// component after it has ended.
	for (const auto& Subscription : ViewerSubscriptions)
	{
		if (UTclViewer* Viewer = Subscription.Key.Get())
			Viewer->OnPropertyUpdated.Unsubscribe(Subscription.Value);
	}
	ViewerSubscriptions.Reset();

	if (GetWorld())
	{
		if (RefreshRetryTimerHandle.IsValid())
		{
			GetWorld()->GetTimerManager().ClearTimer(RefreshRetryTimerHandle);
			RefreshRetryTimerHandle.Invalidate();
		}
		if (RegistryRefreshDebounceTimer.IsValid())
		{
			GetWorld()->GetTimerManager().ClearTimer(RegistryRefreshDebounceTimer);
			RegistryRefreshDebounceTimer.Invalidate();
		}
	}

	Super::EndPlay(EndPlayReason);
}

void
UTentacle_ViewerProperties::HandleViewerAdded(const UTclViewer* ViewerConst)
{
	if (!ViewerConst) return;

	// We need a non-const pointer to subscribe to the event
	auto* Viewer = const_cast<UTclViewer*>(ViewerConst);
	const auto Handle = Viewer->OnPropertyUpdated.Subscribe([WeakThis = TWeakObjectPtr<UTentacle_ViewerProperties>(this)](const FTclViewerPropertyUpdatedData& Data)
	{
		if (Data.Collection && WeakThis.IsValid())
		{
			WeakThis->OnViewerPropertyUpdated.Broadcast(Data.Collection, Data.Viewer, Data.PropertyName, Data.PropertyType);
		}
	});
	ViewerSubscriptions.Emplace(Viewer, Handle);
}

void
UTentacle_ViewerProperties::RefreshRegistry()
{
	FetchRegistry(nullptr);
}

void
UTentacle_ViewerProperties::ReloadRegistry(TFunction<void()> OnDone)
{
	FetchRegistry(MoveTemp(OnDone));
}

void
UTentacle_ViewerProperties::FetchRegistry(TFunction<void()> OnDone)
{
	bRegistryRefreshPending = false;  // a scheduled debounce/retry has now fired

	auto* Tentacle = GetOwner<ATentacle>();

	// No HTTP client means the actor has not begun play yet. A property read can
	// reach here now (it asks for the load it is waiting on), and so can an
	// unknown-definition refresh, so this is an ordinary "not ready" rather than
	// a contract violation: BeginPlay does the first fetch itself.
	if (!Tentacle || !Tentacle->HttpClient)
	{
		if (OnDone)
			OnDone();
		return;
	}

	// Runs OnDone on every way out of the answer, the early ones included.
	struct FDone
	{
		TFunction<void()> Callback;
		~FDone() { if (Callback) Callback(); }
	};

	Tentacle->HttpClient->GetRegistry(
		TEXT(""),
		UTentacleHttpClient::FOnSuccess::CreateLambda([tclLambdaCapture(this, Self), OnDone](const FString& ResponseJson)
		{
			const FDone Done{ OnDone };
			tclLambdaGet(Self);

			// A successful request clears the retry budget for any future refresh.
			Self->RefreshRetryCount = 0;

			TSharedPtr<FJsonObject> Root;
			const auto Reader = TJsonReaderFactory<>::Create(ResponseJson);
			if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
				return;

			const auto ResultObj = Root->GetObjectField(TEXT("result"));
			if (!ResultObj.IsValid()) return;

			const auto DataObj = ResultObj->GetObjectField(TEXT("data"));
			if (!DataObj.IsValid()) return;

			TArray<UTclViewerPropertyCollection*> ParsedCollections;
			const TArray<TSharedPtr<FJsonValue>>* CollectionsArray = nullptr;
			if (DataObj->TryGetArrayField(TEXT("collections"), CollectionsArray) && CollectionsArray)
			{
				for (const auto& Item : *CollectionsArray)
				{
					const auto ItemObj = Item->AsObject();
					if (ItemObj.IsValid())
					{
						if (auto* Collection = UTclViewerPropertyCollection::Parse(Self, ItemObj))
							ParsedCollections.Add(Collection);
					}
				}
			}

			TArray<UTclViewerPropertyDefinition*> ParsedDefinitions;
			const TArray<TSharedPtr<FJsonValue>>* DefinitionsArray = nullptr;
			if (DataObj->TryGetArrayField(TEXT("definitions"), DefinitionsArray) && DefinitionsArray)
			{
				for (const auto& Item : *DefinitionsArray)
				{
					const auto ItemObj = Item->AsObject();
					if (ItemObj.IsValid())
					{
						if (auto* Definition = UTclViewerPropertyDefinition::Parse(Self, ItemObj))
							ParsedDefinitions.Add(Definition);
					}
				}
			}

			Self->ApplyRegistry(ParsedCollections, ParsedDefinitions);
		}),
		UTentacleHttpClient::FOnError::CreateLambda([tclLambdaCapture(this, Self), OnDone](int32, const FString& Err)
		{
			const FDone Done{ OnDone };
			tclLambdaGet(Self);

			UE_LOG(LogTentacle, Warning, TEXT("Tentacle ViewerProperties: GetRegistry failed: %s"), *Err);

			constexpr int32 MaxRetries = 3;
			constexpr float RetryDelaySec = 5.0f;
			// Not once the component has ended: a fetch failing after EndPlay
			// (its session closing as it ends) would otherwise arm a retry that
			// EndPlay already cleared, for a game that is gone.
			if (Self->RefreshRetryCount < MaxRetries && Self->GetWorld() && !Self->bEndedPlay)
			{
				Self->RefreshRetryCount++;
				UE_LOG(LogTentacle, Warning, TEXT("Tentacle ViewerProperties: Retrying GetRegistry (%d/%d) in %.0fs..."), Self->RefreshRetryCount, MaxRetries, RetryDelaySec);
				Self->GetWorld()->GetTimerManager().SetTimer(
					Self->RefreshRetryTimerHandle,
					Self,
					&UTentacle_ViewerProperties::RefreshRegistry,
					RetryDelaySec,
					false
				);
			}
		})
	);
}

void
UTentacle_ViewerProperties::ApplyRegistry(const TArray<UTclViewerPropertyCollection*>& InCollections, const TArray<UTclViewerPropertyDefinition*>& InDefinitions)
{
	Collections = InCollections;

	DefinitionsById.Empty(InDefinitions.Num());
	for (auto* Definition : InDefinitions)
	{
		if (Definition)
			DefinitionsById.Add(Definition->Id, Definition);
	}

	bAreDefinitionsLoaded = true;
	OnCollectionsRefreshed.Broadcast();

	if (!bIsInitialLoadComplete)
	{
		bIsInitialLoadComplete = true;
		OnInitialLoadComplete.Broadcast();
	}

	// Resolve any viewers (and their properties) that arrived before the registry
	// was cached.
	if (auto* Tentacle = GetOwner<ATentacle>())
		Tentacle->RemergeAllViewerProperties();
}

UTclViewerPropertyDefinition*
UTentacle_ViewerProperties::FindDefinitionById(const FString& Id) const
{
	return DefinitionsById.FindRef(Id);
}

UTclViewerPropertyDefinition*
UTentacle_ViewerProperties::FindDefinitionByName(const FString& CollectionId, const FString& Name, bool bCaseSensitive) const
{
	const ESearchCase::Type SearchCase = bCaseSensitive ? ESearchCase::CaseSensitive : ESearchCase::IgnoreCase;
	for (const auto& Pair : DefinitionsById)
	{
		UTclViewerPropertyDefinition* Definition = Pair.Value;
		if (Definition && Definition->CollectionId == CollectionId && Definition->Name.Equals(Name, SearchCase))
			return Definition;
	}
	return nullptr;
}

void
UTentacle_ViewerProperties::RequestRegistryRefresh(const FString& UnknownDefinitionId)
{
	// One refresh per distinct unknown id, for the session — bounds this so a
	// genuinely missing definition (or a flood of traffic referencing one) can
	// never spin the backend.
	if (RequestedDefinitionRefreshIds.Contains(UnknownDefinitionId)) return;
	RequestedDefinitionRefreshIds.Add(UnknownDefinitionId);

	ScheduleRegistryRefresh();
}

void
UTentacle_ViewerProperties::RequestRegistryLoad()
{
	// Nothing to do once it is in: the caller's name simply does not exist.
	if (bAreDefinitionsLoaded) return;

	ScheduleRegistryRefresh();
}

void
UTentacle_ViewerProperties::ScheduleRegistryRefresh()
{
	if (bRegistryRefreshPending) return;  // single-flight
	bRegistryRefreshPending = true;

	if (GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(
			RegistryRefreshDebounceTimer,
			this,
			&UTentacle_ViewerProperties::RefreshRegistry,
			1.0f,
			false
		);
	}
	else
	{
		RefreshRegistry();
	}
}

UTclViewerPropertyCollection*
UTentacle_ViewerProperties::FindCollectionByName(const FString& Name, bool& Found, bool bCaseSensitive) const
{
	const ESearchCase::Type SearchCase = bCaseSensitive ? ESearchCase::CaseSensitive : ESearchCase::IgnoreCase;
	for (auto* C : Collections)
	{
		if (C && C->Name.Equals(Name, SearchCase))
		{
			Found = true;
			return C;
		}
	}
	Found = false;
	return nullptr;
}

UTclViewerPropertyCollection*
UTentacle_ViewerProperties::FindCollectionById(const FString& Id, bool& Found) const
{
	for (auto* C : Collections)
	{
		if (C && C->Id == Id)
		{
			Found = true;
			return C;
		}
	}
	Found = false;
	return nullptr;
}

void
UTentacle_ViewerProperties::FetchViewersByDefinition(
	const FString& CollectionName,
	const FString& PropertyName,
	ETclViewerPropertyType PropertyType,
	ETclOrderDirection OrderDirection,
	int32 Take)
{
	auto* Tentacle = GetOwner<ATentacle>();
	tclSoftAssert(Tentacle && Tentacle->HttpClient);

	// Build input JSON
	const TSharedRef<FJsonObject> InputObj = MakeShared<FJsonObject>();
	InputObj->SetStringField(TEXT("collectionName"), CollectionName);
	InputObj->SetStringField(TEXT("name"), PropertyName);
	InputObj->SetStringField(TEXT("orderDirection"), TclEnumUtils::OrderDirectionToString(OrderDirection));
	InputObj->SetNumberField(TEXT("take"), Take);

	FString InputJson;
	const auto Writer = TJsonWriterFactory<>::Create(&InputJson);
	FJsonSerializer::Serialize(InputObj, Writer);

	Tentacle->HttpClient->GetViewersByDefinition(
		InputJson,
		UTentacleHttpClient::FOnSuccess::CreateLambda([tclLambdaCapture(this, Self), CollectionName, PropertyName, PropertyType](const FString& ResponseJson)
		{
			tclLambdaGet(Self);

			// Parse whatever viewers we can; the completion event always fires below.
			TArray<UTclViewer*> ResultViewers;

			TSharedPtr<FJsonObject> Root;
			const auto Reader = TJsonReaderFactory<>::Create(ResponseJson);
			if (FJsonSerializer::Deserialize(Reader, Root) && Root.IsValid())
			{
				const TSharedPtr<FJsonObject> ResultObj = Root->GetObjectField(TEXT("result"));
				TSharedPtr<FJsonObject> DataObj;
				if (ResultObj.IsValid())
				{
					DataObj = ResultObj->GetObjectField(TEXT("data"));
				}

				const TArray<TSharedPtr<FJsonValue>>* ViewersArray = nullptr;
				if (DataObj.IsValid() && DataObj->TryGetArrayField(TEXT("viewers"), ViewersArray) && ViewersArray)
				{
					if (auto* Tentacle = Self->GetOwner<ATentacle>())
					{
						for (const auto& Item : *ViewersArray)
						{
							const auto ViewerObj = Item->AsObject();
							if (!ViewerObj.IsValid()) continue;

							if (!Tentacle->UpsertViewer(ViewerObj)) continue;

							const FString ViewerId = ViewerObj->GetStringField(TEXT("id"));
							bool Found = false;
							auto* Viewer = Tentacle->FindViewerById(ViewerId, Found);
							if (Found)
								ResultViewers.Add(Viewer);
						}
					}
				}
			}

			// Always broadcast — Collection may be null if it isn't cached yet.
			bool bCollectionFound = false;
			auto* Collection = Self->FindCollectionByName(CollectionName, bCollectionFound);
			Self->OnFetchViewersByPropertyComplete.Broadcast(Collection, ResultViewers, PropertyName, PropertyType);
		}),
		UTentacleHttpClient::FOnError::CreateLambda([tclLambdaCapture(this, Self), CollectionName, PropertyName, PropertyType](int32, const FString& Err)
		{
			tclLambdaGet(Self);

			UE_LOG(LogTentacle, Warning, TEXT("Tentacle ViewerProperties: GetViewersByDefinition failed: %s"), *Err);

			// Still broadcast (with no viewers) so callers always get exactly one completion.
			bool bCollectionFound = false;
			auto* Collection = Self->FindCollectionByName(CollectionName, bCollectionFound);
			Self->OnFetchViewersByPropertyComplete.Broadcast(Collection, TArray<UTclViewer*>(), PropertyName, PropertyType);
		})
	);
}

void
UTentacle_ViewerProperties::ResetCollectionValues(const FString& CollectionName)
{
	auto* Tentacle = GetOwner<ATentacle>();
	tclSoftAssert(Tentacle && Tentacle->HttpClient);

	// Build input JSON - the server resolves the collection by name.
	const TSharedRef<FJsonObject> InputObj = MakeShared<FJsonObject>();
	InputObj->SetStringField(TEXT("collectionName"), CollectionName);

	FString InputJson;
	const auto Writer = TJsonWriterFactory<>::Create(&InputJson);
	FJsonSerializer::Serialize(InputObj, Writer);

	Tentacle->HttpClient->ResetCollectionValues(
		InputJson,
		UTentacleHttpClient::FOnSuccess::CreateLambda([tclLambdaCapture(this, Self), CollectionName](const FString&)
		{
			tclLambdaGet(Self);

			// Revert the local cache so gameplay state matches the server without
			// waiting for each viewer's next snapshot. The collection may be
			// unknown locally (registry still loading) - then nothing is cached
			// under it, so there is nothing to revert.
			bool bCollectionFound = false;
			auto* Collection = Self->FindCollectionByName(CollectionName, bCollectionFound);
			if (Collection)
			{
				if (auto* OwnerTentacle = Self->GetOwner<ATentacle>())
					OwnerTentacle->ResetAllViewerPropertiesToDefaults(Collection->Id);
			}

			Self->OnResetCollectionValuesComplete.Broadcast(Collection, true);
		}),
		UTentacleHttpClient::FOnError::CreateLambda([tclLambdaCapture(this, Self), CollectionName](int32, const FString& Err)
		{
			tclLambdaGet(Self);

			UE_LOG(LogTentacle, Warning, TEXT("Tentacle ViewerProperties: ResetCollectionValues failed: %s"), *Err);

			// Still broadcast so callers always get exactly one completion.
			bool bCollectionFound = false;
			auto* Collection = Self->FindCollectionByName(CollectionName, bCollectionFound);
			Self->OnResetCollectionValuesComplete.Broadcast(Collection, false);
		})
	);
}

void
UTentacle_ViewerProperties::ResetPropertyValues(const FString& CollectionName, const FString& PropertyName)
{
	auto* Tentacle = GetOwner<ATentacle>();
	tclSoftAssert(Tentacle && Tentacle->HttpClient);

	// Build input JSON - the server resolves the definition by collection name +
	// property name.
	const TSharedRef<FJsonObject> InputObj = MakeShared<FJsonObject>();
	InputObj->SetStringField(TEXT("collectionName"), CollectionName);
	InputObj->SetStringField(TEXT("name"), PropertyName);

	FString InputJson;
	const auto Writer = TJsonWriterFactory<>::Create(&InputJson);
	FJsonSerializer::Serialize(InputObj, Writer);

	Tentacle->HttpClient->ResetPropertyValues(
		InputJson,
		UTentacleHttpClient::FOnSuccess::CreateLambda([tclLambdaCapture(this, Self), CollectionName, PropertyName](const FString&)
		{
			tclLambdaGet(Self);

			// Revert the local cache so gameplay state matches the server without
			// waiting for each viewer's next snapshot. The collection or the
			// definition may be unknown locally (registry still loading) - then
			// nothing is cached for it, so there is nothing to revert.
			bool bCollectionFound = false;
			auto* Collection = Self->FindCollectionByName(CollectionName, bCollectionFound);
			if (Collection)
			{
				if (const auto* Definition = Self->FindDefinitionByName(Collection->Id, PropertyName))
				{
					if (auto* OwnerTentacle = Self->GetOwner<ATentacle>())
						OwnerTentacle->ResetAllViewerPropertiesToDefaultForDefinition(Definition->Id);
				}
			}

			Self->OnResetPropertyValuesComplete.Broadcast(Collection, PropertyName, true);
		}),
		UTentacleHttpClient::FOnError::CreateLambda([tclLambdaCapture(this, Self), CollectionName, PropertyName](int32, const FString& Err)
		{
			tclLambdaGet(Self);

			UE_LOG(LogTentacle, Warning, TEXT("Tentacle ViewerProperties: ResetPropertyValues failed: %s"), *Err);

			// Still broadcast so callers always get exactly one completion.
			bool bCollectionFound = false;
			auto* Collection = Self->FindCollectionByName(CollectionName, bCollectionFound);
			Self->OnResetPropertyValuesComplete.Broadcast(Collection, PropertyName, false);
		})
	);
}

void
UTentacle_ViewerProperties::SendViewerPropertyUpdate(
	const FString& CollectionName, const FString& PropertyName, const FString& ViewerId, double Value)
{
	const TSharedRef<FJsonObject> EntryObj = MakeShared<FJsonObject>();
	EntryObj->SetStringField(TEXT("collectionName"), CollectionName);
	EntryObj->SetStringField(TEXT("name"), PropertyName);
	EntryObj->SetStringField(TEXT("viewerId"), ViewerId);
	EntryObj->SetNumberField(TEXT("value"), Value);
	SendViewerPropertyUpdateInternal(EntryObj);
}

void
UTentacle_ViewerProperties::SendViewerPropertyUpdate(
	const FString& CollectionName, const FString& PropertyName, const FString& ViewerId, bool Value)
{
	const TSharedRef<FJsonObject> EntryObj = MakeShared<FJsonObject>();
	EntryObj->SetStringField(TEXT("collectionName"), CollectionName);
	EntryObj->SetStringField(TEXT("name"), PropertyName);
	EntryObj->SetStringField(TEXT("viewerId"), ViewerId);
	EntryObj->SetBoolField(TEXT("value"), Value);
	SendViewerPropertyUpdateInternal(EntryObj);
}

void
UTentacle_ViewerProperties::SendViewerPropertyUpdate(
	const FString& CollectionName, const FString& PropertyName, const FString& ViewerId, const FString& Value)
{
	const TSharedRef<FJsonObject> EntryObj = MakeShared<FJsonObject>();
	EntryObj->SetStringField(TEXT("collectionName"), CollectionName);
	EntryObj->SetStringField(TEXT("name"), PropertyName);
	EntryObj->SetStringField(TEXT("viewerId"), ViewerId);
	EntryObj->SetStringField(TEXT("value"), Value);
	SendViewerPropertyUpdateInternal(EntryObj);
}

void
UTentacle_ViewerProperties::SendViewerPropertyWrite(
	UTclViewer* Viewer, const FString& CollectionName, const FString& PropertyName, const TSharedRef<FJsonValue>& Value, const FTclViewerPropertyEntry& Written)
{
	if (!Viewer) return;

	const TSharedRef<FJsonObject> EntryObj = MakeShared<FJsonObject>();
	EntryObj->SetStringField(TEXT("collectionName"), CollectionName);
	EntryObj->SetStringField(TEXT("name"), PropertyName);
	EntryObj->SetStringField(TEXT("viewerId"), Viewer->GetViewerId());
	EntryObj->SetField(TEXT("value"), Value);

	// Until Tentacle echoes the value back, the entry's hold throws away every
	// snapshot that disagrees with it, so a stale one cannot revert the write. A
	// write that never landed gets no echo: without this the game would read its
	// own value for the rest of the session while Tentacle held another one.
	SendViewerPropertyUpdateInternal(EntryObj, [WeakViewer = TWeakObjectPtr<UTclViewer>(Viewer), Written]()
	{
		UTclViewer* HeldViewer = WeakViewer.Get();
		if (HeldViewer && HeldViewer->ReleaseLocalWrite(Written))
		{
			UE_LOG(LogTentacle, Warning, TEXT("Tentacle ViewerProperties: a write to '%s' on viewer %s did not reach Tentacle; the local value stands until Tentacle sends its own"),
				*Written.Name, *Written.ViewerId);
		}
	});
}

void
UTentacle_ViewerProperties::SendViewerPropertyUpdateInternal(const TSharedRef<FJsonObject>& EntryObj, TFunction<void()> OnRefused)
{
	auto* Tentacle = GetOwner<ATentacle>();
	tclSoftAssert(Tentacle && Tentacle->HttpClient);

	// SetForViewer expects an array of entries; send this single update as a one-element array.
	FString InputJson;
	const auto Writer = TJsonWriterFactory<>::Create(&InputJson);
	Writer->WriteArrayStart();
	FJsonSerializer::Serialize(EntryObj, Writer);
	Writer->WriteArrayEnd();
	Writer->Close();

	Tentacle->HttpClient->SetForViewer(
		InputJson,
		UTentacleHttpClient::FOnSuccess::CreateLambda([](const FString&) {}),
		UTentacleHttpClient::FOnError::CreateLambda([OnRefused](int32, const FString& Err)
		{
			UE_LOG(LogTentacle, Warning, TEXT("Tentacle ViewerProperties: SetForViewer failed: %s"), *Err);

			if (OnRefused)
				OnRefused();
		})
	);
}
