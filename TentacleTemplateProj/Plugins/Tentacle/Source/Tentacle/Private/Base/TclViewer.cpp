// © 2025 Eldritch One. All rights reserved.

#include "Base/TclViewer.h"
#include "Base/Tentacle.h"
#include "Base/Tentacle_ViewerProperties.h"
#include "Sdk/TclViewerPropertyCollection.h"
#include "Sdk/TclViewerPropertyDefinition.h"
#include "Sdk/TclViewerPropertyValue.h"
#include "Sdk/TclViewerTwitch.h"
#include "Sdk/TclViewerKick.h"
#include "Sdk/TclTwitchViewerBan.h"
#include "Utilities/TclBadges.h"
#include "Utilities/TclCommon.h"
#include "Utilities/TclEnumUtils.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/Guid.h"

// --- Core ---

FString UTclViewer::GetViewerId() const
{
	return Data ? Data->Id : FString();
}

FString UTclViewer::GetCreatedAt() const
{
	return Data ? Data->CreatedAt : FString();
}

FString UTclViewer::GetUpdatedAt() const
{
	return Data ? Data->UpdatedAt : FString();
}

// --- Cross-platform ---

FString UTclViewer::GetUserName() const
{
	if (Data)
	{
		if (Data->Twitch)
		{
			return Data->Twitch->Name;
		}
		if (Data->Kick)
		{
			return Data->Kick->Username;
		}
		return FString::Printf(TEXT("Viewer %s"), *Data->Id);
	}
	return FString();
}

FString UTclViewer::GetDisplayName() const
{
	if (Data)
	{
		if (Data->Twitch && !Data->Twitch->DisplayName.IsEmpty())
		{
			return Data->Twitch->DisplayName;
		}
		return GetUserName();
	}
	return FString();
}

FLinearColor UTclViewer::GetColor() const
{
	if (Data)
	{
		FString HexColor;
		if (Data->Twitch && !Data->Twitch->Color.IsEmpty())
		{
			HexColor = Data->Twitch->Color;
		}
		else if (Data->Kick && !Data->Kick->Color.IsEmpty())
		{
			HexColor = Data->Kick->Color;
		}

		if (!HexColor.IsEmpty())
		{
			FColor ParsedColor = FColor::FromHex(HexColor);
			return ParsedColor.ReinterpretAsLinear();
		}
	}
	return FLinearColor::White;
}

bool UTclViewer::IsBroadcaster() const
{
	return IsBroadcaster_Twitch() || IsBroadcaster_Kick();
}

bool UTclViewer::IsModerator() const
{
	return IsModerator_Twitch() || IsModerator_Kick();
}

bool UTclViewer::IsVip() const
{
	return IsVip_Twitch() || IsVip_Kick();
}

bool UTclViewer::IsSubscriber() const
{
	return IsSubscriber_Twitch() || IsSubscriber_Kick();
}

// --- Twitch ---

bool UTclViewer::HasTwitchData() const
{
	return Data && Data->Twitch != nullptr;
}

FString UTclViewer::GetBroadcasterType_Twitch() const
{
	return (Data && Data->Twitch) ? Data->Twitch->BroadcasterType : FString();
}

FString UTclViewer::GetColor_Twitch() const
{
	return (Data && Data->Twitch) ? Data->Twitch->Color : FString();
}

FString UTclViewer::GetDescription_Twitch() const
{
	return (Data && Data->Twitch) ? Data->Twitch->Description : FString();
}

FString UTclViewer::GetDisplayName_Twitch() const
{
	return (Data && Data->Twitch) ? Data->Twitch->DisplayName : FString();
}

FString UTclViewer::GetId_Twitch() const
{
	return (Data && Data->Twitch) ? Data->Twitch->Id : FString();
}

bool UTclViewer::IsKnownBot_Twitch() const
{
	return (Data && Data->Twitch) ? Data->Twitch->bIsKnownBot : false;
}

FString UTclViewer::GetOfflineImageUrl_Twitch() const
{
	return (Data && Data->Twitch) ? Data->Twitch->OfflineImageUrl : FString();
}

FString UTclViewer::GetProfileImageUrl_Twitch() const
{
	return (Data && Data->Twitch) ? Data->Twitch->ProfileImageUrl : FString();
}

FString UTclViewer::GetType_Twitch() const
{
	return (Data && Data->Twitch) ? Data->Twitch->Type : FString();
}

FString UTclViewer::GetUserName_Twitch() const
{
	return (Data && Data->Twitch) ? Data->Twitch->Name : FString();
}

UTclTwitchViewerStanding* UTclViewer::GetStanding_Twitch() const
{
	return (Data && Data->Twitch) ? Data->Twitch->Standing : nullptr;
}

TArray<UTclTwitchChatBadge*> UTclViewer::GetBadges_Twitch() const
{
	return (Data && Data->Twitch) ? Data->Twitch->Badges : TArray<UTclTwitchChatBadge*>();
}

bool UTclViewer::HasBadge_Twitch(const FString& SetId, bool bCaseSensitive) const
{
	return TclBadges::AnyTwitch(GetBadges_Twitch(), { SetId }, bCaseSensitive);
}

// Each role reads what Twitch said (the standing) and falls back to the badges
// only for a viewer sent by a Tentacle that predates the standing.

bool UTclViewer::IsBroadcaster_Twitch() const
{
	const UTclTwitchViewerStanding* Standing = GetStanding_Twitch();
	return Standing ? Standing->bIsBroadcaster : TclBadges::AnyTwitch(GetBadges_Twitch(), TclBadges::TwitchBroadcaster());
}

bool UTclViewer::IsModerator_Twitch() const
{
	const UTclTwitchViewerStanding* Standing = GetStanding_Twitch();
	return Standing ? Standing->bIsModerator : TclBadges::AnyTwitch(GetBadges_Twitch(), TclBadges::TwitchModerator());
}

bool UTclViewer::IsVip_Twitch() const
{
	const UTclTwitchViewerStanding* Standing = GetStanding_Twitch();
	return Standing ? Standing->bIsVip : TclBadges::AnyTwitch(GetBadges_Twitch(), TclBadges::TwitchVip());
}

bool UTclViewer::IsSubscriber_Twitch() const
{
	const UTclTwitchViewerStanding* Standing = GetStanding_Twitch();
	return Standing ? Standing->bIsSubscriber : TclBadges::AnyTwitch(GetBadges_Twitch(), TclBadges::TwitchSubscriber());
}

bool UTclViewer::IsFollower_Twitch() const
{
	const UTclTwitchViewerStanding* Standing = GetStanding_Twitch();
	return Standing && Standing->bIsFollower;
}

bool UTclViewer::IsBanned_Twitch() const
{
	const UTclTwitchViewerStanding* Standing = GetStanding_Twitch();
	const UTclTwitchViewerBan* Ban = Standing ? Standing->Ban : nullptr;
	if (!Ban) return false;
	if (Ban->ExpiresAt.IsEmpty()) return true;
	// A time that does not read counts as in force, so a ban is never lifted by
	// a malformed expiry.
	FDateTime ExpiresAt;
	return !FDateTime::ParseIso8601(*Ban->ExpiresAt, ExpiresAt) || ExpiresAt > FDateTime::UtcNow();
}

// --- Kick ---

bool UTclViewer::HasKickData() const
{
	return Data && Data->Kick != nullptr;
}

TArray<UTclKickBadge*> UTclViewer::GetBadges_Kick() const
{
	return (Data && Data->Kick) ? Data->Kick->Badges : TArray<UTclKickBadge*>();
}

FString UTclViewer::GetChannelSlug_Kick() const
{
	return (Data && Data->Kick) ? Data->Kick->ChannelSlug : FString();
}

FString UTclViewer::GetColor_Kick() const
{
	return (Data && Data->Kick) ? Data->Kick->Color : FString();
}

int32 UTclViewer::GetId_Kick() const
{
	return (Data && Data->Kick) ? Data->Kick->Id : -1;
}

bool UTclViewer::IsAnonymous_Kick() const
{
	return (Data && Data->Kick) ? Data->Kick->bIsAnonymous : false;
}

bool UTclViewer::IsVerified_Kick() const
{
	return (Data && Data->Kick) ? Data->Kick->bIsVerified : false;
}

FString UTclViewer::GetProfilePicture_Kick() const
{
	return (Data && Data->Kick) ? Data->Kick->ProfilePicture : FString();
}

FString UTclViewer::GetUserName_Kick() const
{
	return (Data && Data->Kick) ? Data->Kick->Username : FString();
}

bool UTclViewer::HasBadge_Kick(const FString& Type, bool bCaseSensitive) const
{
	return TclBadges::AnyKick(GetBadges_Kick(), { Type }, bCaseSensitive);
}

bool UTclViewer::IsBroadcaster_Kick() const
{
	return TclBadges::AnyKick(GetBadges_Kick(), TclBadges::KickBroadcaster());
}

bool UTclViewer::IsModerator_Kick() const
{
	return TclBadges::AnyKick(GetBadges_Kick(), TclBadges::KickModerator());
}

bool UTclViewer::IsVip_Kick() const
{
	return TclBadges::AnyKick(GetBadges_Kick(), TclBadges::KickVip());
}

bool UTclViewer::IsSubscriber_Kick() const
{
	return TclBadges::AnyKick(GetBadges_Kick(), TclBadges::KickSubscriber());
}

// --- Mutators ---

void UTclViewer::UpdateData(UTclViewerData* NewData)
{
	Data = NewData;
	MergePropertiesFromData(NewData);
}

// --- Properties ---

FString
UTclViewer::BuildPropertyMapId(const FString& CollectionId, const FString& DefinitionId, const FString& ViewerId)
{
	return FString::Printf(TEXT("%s/%s/%s"), *CollectionId, *DefinitionId, *ViewerId);
}

ETclViewerPropertyType
UTclViewer::TypeStringToEnum(const FString& TypeString)
{
	if (TypeString == TEXT("float")) return ETclViewerPropertyType::Float;
	if (TypeString == TEXT("int")) return ETclViewerPropertyType::Int;
	if (TypeString == TEXT("bool")) return ETclViewerPropertyType::Bool;
	if (TypeString == TEXT("string")) return ETclViewerPropertyType::String;
	return ETclViewerPropertyType::String;
}

const FTclViewerPropertyEntry*
UTclViewer::FindProperty(const FString& CollectionName, const FString& PropertyName, bool bCaseSensitive, ETclFindPropertyResult* OutResult) const
{
	const auto SetResult = [OutResult](ETclFindPropertyResult Result)
	{
		if (OutResult) *OutResult = Result;
	};

	if (!Tentacle || !Tentacle->ViewerProperties)
	{
		SetResult(ETclFindPropertyResult::CollectionNotFound);
		return nullptr;
	}

	UTentacle_ViewerProperties* VP = Tentacle->ViewerProperties;

	// A lookup before the registry arrives cannot resolve any name, and the
	// collection lookup below would call that CollectionNotFound - which sends
	// whoever is debugging to look for a collection that is there. Say so in
	// the log and make sure a load is on its way; the result stays
	// CollectionNotFound rather than becoming a fourth exec pin for a state
	// that resolves itself.
	if (!VP->AreDefinitionsLoaded())
	{
		tclWarnF(
			"Tentacle: viewer property '%s/%s' was read before the property registry loaded; "
			"a load has been requested and the read will resolve once it arrives",
			*CollectionName, *PropertyName);
		VP->RequestRegistryLoad();
		SetResult(ETclFindPropertyResult::CollectionNotFound);
		return nullptr;
	}

	bool bFound = false;
	UTclViewerPropertyCollection* Collection = VP->FindCollectionByName(CollectionName, bFound, bCaseSensitive);
	if (!Collection)
	{
		SetResult(ETclFindPropertyResult::CollectionNotFound);
		return nullptr;
	}

	const FString ViewerId = GetViewerId();
	const ESearchCase::Type SearchCase = bCaseSensitive ? ESearchCase::CaseSensitive : ESearchCase::IgnoreCase;
	for (const auto& [Key, Entry] : Properties)
	{
		if (Entry.CollectionId == Collection->Id
			&& Entry.ViewerId == ViewerId
			&& Entry.Name.Equals(PropertyName, SearchCase))
		{
			SetResult(ETclFindPropertyResult::Found);
			return &Entry;
		}
	}
	SetResult(ETclFindPropertyResult::PropertyNotFound);
	return nullptr;
}

FTclViewerPropertyEntry*
UTclViewer::FindPropertyMutable(const FString& CollectionName, const FString& PropertyName, bool bCaseSensitive, ETclFindPropertyResult* OutResult)
{
	return const_cast<FTclViewerPropertyEntry*>(FindProperty(CollectionName, PropertyName, bCaseSensitive, OutResult));
}

ETclSetPropertyResult
UTclViewer::ToSetResult(ETclFindPropertyResult FindResult)
{
	switch (FindResult)
	{
	case ETclFindPropertyResult::Found:              return ETclSetPropertyResult::Success;
	case ETclFindPropertyResult::CollectionNotFound: return ETclSetPropertyResult::CollectionNotFound;
	default:                                         return ETclSetPropertyResult::PropertyNotFound;
	}
}

ETclViewerPropertyType
UTclViewer::GetPropertyType(const FString& CollectionName, const FString& PropertyName, bool bCaseSensitive) const
{
	const auto* Prop = FindProperty(CollectionName, PropertyName, bCaseSensitive);
	if (!Prop) return ETclViewerPropertyType::String;
	return TypeStringToEnum(Prop->TypeString);
}

double
UTclViewer::GetProperty_Float(const FString& CollectionName, const FString& PropertyName, ETclFindPropertyResult& OutResult, bool bCaseSensitive) const
{
	const auto* Prop = FindProperty(CollectionName, PropertyName, bCaseSensitive, &OutResult);
	return Prop ? Prop->NumberValue : 0.0;
}

int64
UTclViewer::GetProperty_Int(const FString& CollectionName, const FString& PropertyName, ETclFindPropertyResult& OutResult, bool bCaseSensitive) const
{
	const auto* Prop = FindProperty(CollectionName, PropertyName, bCaseSensitive, &OutResult);
	// double -> int64 truncates toward zero, matching the server's int storage. The
	// backing double is exact for integers up to 2^53, the JS-number / JSON-wire ceiling.
	return Prop ? static_cast<int64>(Prop->NumberValue) : 0;
}

bool
UTclViewer::GetProperty_Bool(const FString& CollectionName, const FString& PropertyName, ETclFindPropertyResult& OutResult, bool bCaseSensitive) const
{
	const auto* Prop = FindProperty(CollectionName, PropertyName, bCaseSensitive, &OutResult);
	return Prop ? Prop->BoolValue : false;
}

FString
UTclViewer::GetProperty_String(const FString& CollectionName, const FString& PropertyName, ETclFindPropertyResult& OutResult, bool bCaseSensitive) const
{
	const auto* Prop = FindProperty(CollectionName, PropertyName, bCaseSensitive, &OutResult);
	return Prop ? Prop->StringValue : FString();
}

double
UTclViewer::SetProperty_Float(const FString& CollectionName, const FString& PropertyName, double Value, ETclSetPropertyResult& OutResult, bool bCaseSensitive)
{
	ETclFindPropertyResult FindResult = ETclFindPropertyResult::Found;
	auto* Prop = FindPropertyMutable(CollectionName, PropertyName, bCaseSensitive, &FindResult);
	if (!Prop)
	{
		// Properties are defined server-side and never created client-side.
		OutResult = ToSetResult(FindResult);
		return Value;
	}

	Prop->NumberValue = Value;
	Prop->bLocallyModified = true;

	// FindProperty resolved the collection, so the component is valid here.
	UTentacle_ViewerProperties* VP = Tentacle->ViewerProperties;

	// Queue update for server sync (if not mock)
	if (Data && !Data->bIsMock)
	{
		VP->SendViewerPropertyWrite(this, CollectionName, PropertyName, MakeShared<FJsonValueNumber>(Value), *Prop);
	}

	// Fire event
	bool bFound = false;
	FTclViewerPropertyUpdatedData EventData;
	EventData.Viewer = this;
	EventData.Collection = VP->FindCollectionByName(CollectionName, bFound, bCaseSensitive);
	EventData.PropertyName = PropertyName;
	EventData.PropertyType = ETclViewerPropertyType::Float;
	OnPropertyUpdated.Next(EventData);

	OutResult = ETclSetPropertyResult::Success;
	return Value;
}

int64
UTclViewer::SetProperty_Int(const FString& CollectionName, const FString& PropertyName, int64 Value, ETclSetPropertyResult& OutResult, bool bCaseSensitive)
{
	ETclFindPropertyResult FindResult = ETclFindPropertyResult::Found;
	auto* Prop = FindPropertyMutable(CollectionName, PropertyName, bCaseSensitive, &FindResult);
	if (!Prop)
	{
		// Properties are defined server-side and never created client-side.
		OutResult = ToSetResult(FindResult);
		return Value;
	}

	Prop->NumberValue = static_cast<double>(Value);
	Prop->bLocallyModified = true;

	// FindProperty resolved the collection, so the component is valid here.
	UTentacle_ViewerProperties* VP = Tentacle->ViewerProperties;

	// Queue update for server sync (if not mock). The value is sent as a JSON
	// number (via the double overload); the server stores it for an int-typed
	// property. JSON numbers are exact only to 2^53, which bounds the int range.
	if (Data && !Data->bIsMock)
	{
		VP->SendViewerPropertyWrite(this, CollectionName, PropertyName, MakeShared<FJsonValueNumber>(static_cast<double>(Value)), *Prop);
	}

	// Fire event
	bool bFound = false;
	FTclViewerPropertyUpdatedData EventData;
	EventData.Viewer = this;
	EventData.Collection = VP->FindCollectionByName(CollectionName, bFound, bCaseSensitive);
	EventData.PropertyName = PropertyName;
	EventData.PropertyType = ETclViewerPropertyType::Int;
	OnPropertyUpdated.Next(EventData);

	OutResult = ETclSetPropertyResult::Success;
	return Value;
}

bool
UTclViewer::SetProperty_Bool(const FString& CollectionName, const FString& PropertyName, bool Value, ETclSetPropertyResult& OutResult, bool bCaseSensitive)
{
	ETclFindPropertyResult FindResult = ETclFindPropertyResult::Found;
	auto* Prop = FindPropertyMutable(CollectionName, PropertyName, bCaseSensitive, &FindResult);
	if (!Prop)
	{
		OutResult = ToSetResult(FindResult);
		return Value;
	}

	Prop->BoolValue = Value;
	Prop->bLocallyModified = true;

	UTentacle_ViewerProperties* VP = Tentacle->ViewerProperties;

	if (Data && !Data->bIsMock)
	{
		VP->SendViewerPropertyWrite(this, CollectionName, PropertyName, MakeShared<FJsonValueBoolean>(Value), *Prop);
	}

	bool bFound = false;
	FTclViewerPropertyUpdatedData EventData;
	EventData.Viewer = this;
	EventData.Collection = VP->FindCollectionByName(CollectionName, bFound, bCaseSensitive);
	EventData.PropertyName = PropertyName;
	EventData.PropertyType = ETclViewerPropertyType::Bool;
	OnPropertyUpdated.Next(EventData);

	OutResult = ETclSetPropertyResult::Success;
	return Value;
}

FString
UTclViewer::SetProperty_String(const FString& CollectionName, const FString& PropertyName, const FString& Value, ETclSetPropertyResult& OutResult, bool bCaseSensitive)
{
	ETclFindPropertyResult FindResult = ETclFindPropertyResult::Found;
	auto* Prop = FindPropertyMutable(CollectionName, PropertyName, bCaseSensitive, &FindResult);
	if (!Prop)
	{
		OutResult = ToSetResult(FindResult);
		return Value;
	}

	Prop->StringValue = Value;
	Prop->bLocallyModified = true;

	UTentacle_ViewerProperties* VP = Tentacle->ViewerProperties;

	if (Data && !Data->bIsMock)
	{
		VP->SendViewerPropertyWrite(this, CollectionName, PropertyName, MakeShared<FJsonValueString>(Value), *Prop);
	}

	bool bFound = false;
	FTclViewerPropertyUpdatedData EventData;
	EventData.Viewer = this;
	EventData.Collection = VP->FindCollectionByName(CollectionName, bFound, bCaseSensitive);
	EventData.PropertyName = PropertyName;
	EventData.PropertyType = ETclViewerPropertyType::String;
	OnPropertyUpdated.Next(EventData);

	OutResult = ETclSetPropertyResult::Success;
	return Value;
}

double
UTclViewer::AddToProperty_Float(const FString& CollectionName, const FString& PropertyName, double Amount, ETclSetPropertyResult& OutResult, bool bCaseSensitive)
{
	// Read the current value; bail out (without writing) if the property can't be resolved.
	ETclFindPropertyResult FindResult = ETclFindPropertyResult::Found;
	const double CurrentValue = GetProperty_Float(CollectionName, PropertyName, FindResult, bCaseSensitive);
	if (FindResult != ETclFindPropertyResult::Found)
	{
		OutResult = ToSetResult(FindResult);
		return 0.0;
	}

	// Delegate the write (server sync + event + OutResult) to SetProperty_Float.
	const double NewValue = CurrentValue + Amount;
	SetProperty_Float(CollectionName, PropertyName, NewValue, OutResult, bCaseSensitive);
	return NewValue;
}

int64
UTclViewer::AddToProperty_Int(const FString& CollectionName, const FString& PropertyName, int64 Amount, ETclSetPropertyResult& OutResult, bool bCaseSensitive)
{
	// Read the current value; bail out (without writing) if the property can't be resolved.
	ETclFindPropertyResult FindResult = ETclFindPropertyResult::Found;
	const int64 CurrentValue = GetProperty_Int(CollectionName, PropertyName, FindResult, bCaseSensitive);
	if (FindResult != ETclFindPropertyResult::Found)
	{
		OutResult = ToSetResult(FindResult);
		return 0;
	}

	// Delegate the write (server sync + event + OutResult) to SetProperty_Int.
	const int64 NewValue = CurrentValue + Amount;
	SetProperty_Int(CollectionName, PropertyName, NewValue, OutResult, bCaseSensitive);
	return NewValue;
}

bool
UTclViewer::ToggleProperty_Bool(const FString& CollectionName, const FString& PropertyName, ETclSetPropertyResult& OutResult, bool bCaseSensitive)
{
	// Read the current value; bail out (without writing) if the property can't be resolved.
	ETclFindPropertyResult FindResult = ETclFindPropertyResult::Found;
	const bool bCurrentValue = GetProperty_Bool(CollectionName, PropertyName, FindResult, bCaseSensitive);
	if (FindResult != ETclFindPropertyResult::Found)
	{
		OutResult = ToSetResult(FindResult);
		return false;
	}

	// Delegate the write (server sync + event + OutResult) to SetProperty_Bool.
	const bool bNewValue = !bCurrentValue;
	SetProperty_Bool(CollectionName, PropertyName, bNewValue, OutResult, bCaseSensitive);
	return bNewValue;
}

namespace
{
	/** Compares the typed value of two property entries (ignores metadata). */
	bool PropertyValuesEqual(const FTclViewerPropertyEntry& A, const FTclViewerPropertyEntry& B)
	{
		if (A.TypeString != B.TypeString)
		{
			return false;
		}
		if (A.TypeString == TEXT("float"))
		{
			// Server-side floats round-trip through float in the SDK; compare at
			// float precision so a confirmed echo reconciles against a local write.
			return static_cast<float>(A.NumberValue) == static_cast<float>(B.NumberValue);
		}
		if (A.TypeString == TEXT("int"))
		{
			// Ints are whole numbers; compare as integers so the echo reconciles.
			return static_cast<int64>(A.NumberValue) == static_cast<int64>(B.NumberValue);
		}
		if (A.TypeString == TEXT("bool"))
		{
			return A.BoolValue == B.BoolValue;
		}
		if (A.TypeString == TEXT("string"))
		{
			return A.StringValue == B.StringValue;
		}
		return false;
	}
}

bool
UTclViewer::ReleaseLocalWrite(const FTclViewerPropertyEntry& Written)
{
	FTclViewerPropertyEntry* Entry = Properties.Find(BuildPropertyMapId(Written.CollectionId, Written.DefinitionId, Written.ViewerId));
	if (!Entry || !Entry->bLocallyModified)
		return false;

	// The comparison is what makes this safe: a newer write, or an echo that
	// already reconciled this one, must not lose its own protection because an
	// older write reported failure.
	if (!PropertyValuesEqual(*Entry, Written))
		return false;

	Entry->bLocallyModified = false;
	return true;
}

void
UTclViewer::ResetPropertiesToDefaultsForCollection(const FString& CollectionId)
{
	// The server has deleted every override in the collection, so no optimistic
	// local write may survive reconciliation - clear the guards first.
	for (auto& [Key, Entry] : Properties)
	{
		if (Entry.CollectionId == CollectionId)
			Entry.bLocallyModified = false;
	}

	if (!Data) return;

	FTclViewerData_Properties* CollectionValues = Data->Properties.Find(CollectionId);
	if (!CollectionValues) return;

	UTentacle_ViewerProperties* VP = Tentacle ? Tentacle->ViewerProperties : nullptr;

	// Rewrite the stored slim payload to each definition's default so the data
	// stays consistent for future re-merges. Definitions the registry doesn't
	// know yet are left as-is (a registry refresh reconciles them later).
	for (auto& Pair : CollectionValues->Values)
	{
		UTclViewerPropertyValue* Slim = Pair.Value;
		if (!Slim) continue;

		const UTclViewerPropertyDefinition* Def = VP ? VP->FindDefinitionById(Pair.Key) : nullptr;
		if (!Def) continue;

		Slim->Value = Def->Value;
		Slim->bIsDefault = true;
	}

	// Re-merge to re-type the reverted values and fire OnPropertyUpdated for
	// entries whose value actually changed.
	MergePropertiesFromData(Data);
}

void
UTclViewer::ResetPropertyToDefaultForDefinition(const FString& DefinitionId)
{
	// The server has deleted the definition's override, so no optimistic local
	// write may survive reconciliation - clear the guard first.
	for (auto& [Key, Entry] : Properties)
	{
		if (Entry.DefinitionId == DefinitionId)
			Entry.bLocallyModified = false;
	}

	if (!Data) return;

	// Rewrite the stored slim payload to the definition's default so the data
	// stays consistent for future re-merges. If the registry doesn't know the
	// definition yet there is no default to revert to (a registry refresh
	// reconciles it later).
	UTentacle_ViewerProperties* VP = Tentacle ? Tentacle->ViewerProperties : nullptr;
	const UTclViewerPropertyDefinition* Def = VP ? VP->FindDefinitionById(DefinitionId) : nullptr;
	if (!Def) return;

	FTclViewerData_Properties* CollectionValues = Data->Properties.Find(Def->CollectionId);
	if (!CollectionValues) return;

	UTclViewerPropertyValue* Slim = CollectionValues->Values.FindRef(DefinitionId);
	if (!Slim) return;

	Slim->Value = Def->Value;
	Slim->bIsDefault = true;

	// Re-merge to re-type the reverted value and fire OnPropertyUpdated if it
	// actually changed.
	MergePropertiesFromData(Data);
}

void
UTclViewer::MergePropertiesFromData(const UTclViewerData* ViewerData)
{
	if (!ViewerData) return;

	// The per-property id/name/type are no longer on the wire (slim payload); the
	// viewer owns its id and the rest is resolved from the definitions cache.
	const FString ViewerId = ViewerData->Id;
	UTentacle_ViewerProperties* VP = Tentacle ? Tentacle->ViewerProperties : nullptr;

	// Properties arrive nested: collection id -> (definition id -> { value, isDefault }).
	for (const auto& CollectionPair : ViewerData->Properties)
	{
		const FString& CollectionId = CollectionPair.Key;
		for (const auto& Pair : CollectionPair.Value.Values)
		{
			const FString& DefinitionId = Pair.Key;
			const UTclViewerPropertyValue* Slim = Pair.Value;
			if (!Slim) continue;

			FTclViewerPropertyEntry Entry;
			Entry.DefinitionId = DefinitionId;
			Entry.CollectionId = CollectionId;
			Entry.ViewerId = ViewerId;
			Entry.bIsDefault = Slim->bIsDefault;
			Entry.RawValue = Slim->Value;

			// Resolve name + type from the definitions cache. If the registry
			// hasn't loaded yet, leave the entry unresolved (empty Name/TypeString);
			// FindProperty won't match it by name until the re-merge that runs when
			// the registry arrives fills it in. If the registry IS loaded but this
			// definition is unknown (created at runtime), ask for one debounced
			// refresh — each unknown id triggers at most one refresh, so no loop.
			const UTclViewerPropertyDefinition* Def = VP ? VP->FindDefinitionById(DefinitionId) : nullptr;
			if (Def)
			{
				Entry.Name = Def->Name;
				Entry.TypeString = TclEnumUtils::ViewerPropertyTypeToString(Def->Type);
				switch (Def->Type)
				{
				case ETclViewerPropertyType::Float: Entry.NumberValue = FCString::Atod(*Entry.RawValue); break;
				case ETclViewerPropertyType::Int:   Entry.NumberValue = FCString::Atod(*Entry.RawValue); break;
				case ETclViewerPropertyType::Bool:   Entry.BoolValue = Entry.RawValue.ToBool(); break;
				case ETclViewerPropertyType::String: Entry.StringValue = Entry.RawValue; break;
				}
			}
			else if (VP && VP->AreDefinitionsLoaded())
			{
				VP->RequestRegistryRefresh(DefinitionId);
			}

			const FString MapId = BuildPropertyMapId(CollectionId, DefinitionId, ViewerId);
			const FTclViewerPropertyEntry* Existing = Properties.Find(MapId);

			// Keep an optimistic local write until the server echoes the same value
			// back — a stale server snapshot must not revert it. (Only resolved
			// entries can be locally modified, since SetXProperty resolves by name
			// first, so an unresolved Existing is never locally modified.)
			if (Existing && Existing->bLocallyModified && !PropertyValuesEqual(*Existing, Entry))
			{
				continue;
			}

			// Fire a change event only when an already-RESOLVED value actually
			// changed — not when an entry merely transitions unresolved -> resolved
			// (that's resolution, not a value change, and avoids an event storm when
			// the registry loads and every viewer is re-merged).
			const bool bValueChanged = Existing && !Existing->TypeString.IsEmpty() && !PropertyValuesEqual(*Existing, Entry);
			const ETclViewerPropertyType PropertyType = Def ? Def->Type : ETclViewerPropertyType::String;
			const FString PropertyName = Entry.Name;

			Properties.Add(MapId, MoveTemp(Entry));

			if (bValueChanged)
			{
				bool bCollectionFound = false;
				FTclViewerPropertyUpdatedData EventData;
				EventData.Viewer = this;
				EventData.Collection = VP
					? VP->FindCollectionById(CollectionId, bCollectionFound)
					: nullptr;
				EventData.PropertyName = PropertyName;
				EventData.PropertyType = PropertyType;
				OnPropertyUpdated.Next(EventData);
			}
		}
	}
}
