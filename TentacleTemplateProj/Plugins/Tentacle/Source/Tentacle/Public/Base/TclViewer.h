// © 2025 Eldritch One. All rights reserved.

#pragma once
#include "CoreMinimal.h"
#include "Sdk/TclViewerPropertyType.h"
#include "Utilities/TclEventSubject.h"
#include "Sdk/TclViewerData.h"
#include "Sdk/TclViewerTwitch.h"
#include "Sdk/TclViewerKick.h"
#include "Sdk/TclKickBadge.h"
#include "Sdk/TclTwitchChatBadge.h"
#include "Sdk/TclTwitchViewerStanding.h"
#include "TclViewer.generated.h"

class ATentacle;
class UTclViewerPropertyCollection;

/**
 * Stored viewer property entry — local in-memory representation.
 * Keyed in Properties map by "collectionId/definitionId/viewerId".
 *
 * Name and TypeString (and the typed value) are resolved from the viewer-
 * property definitions cache at merge time, not from the per-viewer payload
 * (which carries only the slim value + isDefault). An entry merged before the
 * registry is cached is left "unresolved" (empty Name/TypeString) until a
 * re-merge fills it in; RawValue retains the wire value for that later typing.
 */
USTRUCT()
struct FTclViewerPropertyEntry
{
	GENERATED_BODY()

	FString DefinitionId;
	FString CollectionId;
	FString Name;
	FString ViewerId;
	FString TypeString; // "float", "int", "bool", "string" — empty until resolved
	FString RawValue;   // verbatim on-wire value (JSON-compact), typed on resolve
	double NumberValue = 0.0; // backs both float and int; holds integers exactly up to 2^53 (the JS-number / JSON-wire ceiling) — the effective range for int properties
	bool BoolValue = false;
	FString StringValue;
	bool bIsDefault = false;

	/** True once the client has written this property locally and is awaiting the
	 *  server echo. While set, incoming server snapshots will not revert it. */
	bool bLocallyModified = false;
};

/**
 * Event data broadcast when a viewer property changes.
 */
struct FTclViewerPropertyUpdatedData
{
	UTclViewer* Viewer = nullptr;
	UTclViewerPropertyCollection* Collection = nullptr;
	FString PropertyName;
	ETclViewerPropertyType PropertyType = ETclViewerPropertyType::String;
};

/**
 * Outcome of a viewer property lookup — lets callers distinguish
 * "collection not found" (e.g. collections still loading) from
 * "property not found" so they can report an accurate error.
 *
 * A registry that has not loaded yet is deliberately NOT a member here. It is a
 * transient state the plugin recovers from on its own - the registry is fetched
 * on connect, retried on failure, and re-fetched on demand - so a game has
 * nothing to do about it that waiting would not also do. Giving it an exec pin
 * would put a branch in every graph for a case that resolves itself; it is
 * reported to the log instead, and the lookup answers CollectionNotFound.
 */
UENUM(BlueprintType)
enum class ETclFindPropertyResult : uint8
{
	Found,
	CollectionNotFound,
	PropertyNotFound,
};

/**
 * Outcome of a viewer property set operation. The property must already
 * exist (defined server-side); setting never creates a property client-side.
 */
UENUM(BlueprintType)
enum class ETclSetPropertyResult : uint8
{
	Success,
	CollectionNotFound,
	PropertyNotFound,
};

UCLASS(BlueprintType, meta=(DisplayName="Viewer"))
class TENTACLE_API UTclViewer : public UObject
{
	GENERATED_BODY()

public:
	/** The owning Tentacle actor. Set at creation time. */
	UPROPERTY(BlueprintReadOnly, Category = "Tentacle|Viewer")
	ATentacle* Tentacle = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Tentacle|Viewer")
	UTclViewerData* Data = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = "Tentacle|Viewer")
	bool bIsHost = false;

	/** Local property storage — keyed by "collectionId/definitionId/viewerId" */
	TMap<FString, FTclViewerPropertyEntry> Properties;

	/** Internal C++ event fired when a viewer property changes */
	TTclEventSubject<FTclViewerPropertyUpdatedData> OnPropertyUpdated;

	// --- Core ---

	UFUNCTION(BlueprintPure, Category = "Tentacle|Viewer")
	FString GetViewerId() const;

	UFUNCTION(BlueprintPure, Category = "Tentacle|Viewer")
	FString GetCreatedAt() const;

	UFUNCTION(BlueprintPure, Category = "Tentacle|Viewer")
	FString GetUpdatedAt() const;

	// --- Cross-platform ---

	/**
	 * The viewer's account/login name, resolved across platforms.
	 * Precedence: Twitch name first, then Kick username. Falls back to
	 * "Viewer <id>" when the viewer has data for neither platform.
	 */
	UFUNCTION(BlueprintPure, Category = "Tentacle|Viewer")
	FString GetUserName() const;

	/**
	 * The viewer's display name, resolved across platforms.
	 * Precedence: Twitch display name first (when set); otherwise falls back to
	 * GetUserName() — i.e. Twitch name, then Kick username, then "Viewer <id>".
	 */
	UFUNCTION(BlueprintPure, Category = "Tentacle|Viewer")
	FString GetDisplayName() const;

	/**
	 * The viewer's chat color, resolved across platforms.
	 * Precedence: Twitch color first (when set), then Kick color. Defaults to
	 * white when neither platform provides a color.
	 */
	UFUNCTION(BlueprintPure, Category = "Tentacle|Viewer")
	FLinearColor GetColor() const;

	/** Whether the viewer is the broadcaster on any platform, by their Twitch or Kick badges. */
	UFUNCTION(BlueprintPure, Category = "Tentacle|Viewer")
	bool IsBroadcaster() const;

	/**
	 * Whether the viewer moderates the streamer's chat on any platform, by their
	 * Twitch or Kick badges. The broadcaster is not a moderator; ask Is
	 * Broadcaster as well.
	 */
	UFUNCTION(BlueprintPure, Category = "Tentacle|Viewer")
	bool IsModerator() const;

	/** Whether the viewer is a VIP on any platform, by their Twitch or Kick badges. */
	UFUNCTION(BlueprintPure, Category = "Tentacle|Viewer")
	bool IsVip() const;

	/** Whether the viewer subscribes to the streamer on any platform, by their Twitch or Kick badges. */
	UFUNCTION(BlueprintPure, Category = "Tentacle|Viewer")
	bool IsSubscriber() const;

	// --- Twitch ---

	UFUNCTION(BlueprintPure, Category = "Tentacle|Viewer")
	bool HasTwitchData() const;

	/**
	 * The viewer's Twitch broadcaster type: "partner", "affiliate", or an empty string for
	 * any other channel. Empty when the viewer has no Twitch data.
	 */
	UFUNCTION(BlueprintPure, Category = "Tentacle|Viewer", meta=(DisplayName="Get Broadcaster Type (Twitch)"))
	FString GetBroadcasterType_Twitch() const;

	UFUNCTION(BlueprintPure, Category = "Tentacle|Viewer", meta=(DisplayName="Get Color (Twitch)"))
	FString GetColor_Twitch() const;

	UFUNCTION(BlueprintPure, Category = "Tentacle|Viewer", meta=(DisplayName="Get Description (Twitch)"))
	FString GetDescription_Twitch() const;

	UFUNCTION(BlueprintPure, Category = "Tentacle|Viewer", meta=(DisplayName="Get Display Name (Twitch)"))
	FString GetDisplayName_Twitch() const;

	UFUNCTION(BlueprintPure, Category = "Tentacle|Viewer", meta=(DisplayName="Get Id (Twitch)"))
	FString GetId_Twitch() const;

	UFUNCTION(BlueprintPure, Category = "Tentacle|Viewer", meta=(DisplayName="Is Known Bot (Twitch)"))
	bool IsKnownBot_Twitch() const;

	UFUNCTION(BlueprintPure, Category = "Tentacle|Viewer", meta=(DisplayName="Get Offline Image Url (Twitch)"))
	FString GetOfflineImageUrl_Twitch() const;

	UFUNCTION(BlueprintPure, Category = "Tentacle|Viewer", meta=(DisplayName="Get Profile Image Url (Twitch)"))
	FString GetProfileImageUrl_Twitch() const;

	/**
	 * The viewer's Twitch-wide user type: "staff", "admin", or "global_mod" for Twitch
	 * staff, administrators, and global moderators, and an empty string for everyone else,
	 * so it is empty for almost every viewer. It is not the viewer's role in your channel;
	 * for that, use Is Moderator (Twitch), Is Vip (Twitch), and Is Subscriber (Twitch).
	 * Empty when the viewer has no Twitch data.
	 */
	UFUNCTION(BlueprintPure, Category = "Tentacle|Viewer", meta=(DisplayName="Get Type (Twitch)"))
	FString GetType_Twitch() const;

	UFUNCTION(BlueprintPure, Category = "Tentacle|Viewer", meta=(DisplayName="Get User Name (Twitch)"))
	FString GetUserName_Twitch() const;

	/**
	 * The viewer's standing in the streamer's Twitch channel, as Tentacle last
	 * learned it from Twitch: roles, subscription, follow, ban, and bits. None
	 * when they have no Twitch profile, or the Tentacle that sent them predates
	 * it.
	 */
	UFUNCTION(BlueprintPure, Category = "Tentacle|Viewer", meta=(DisplayName="Get Standing (Twitch)"))
	UTclTwitchViewerStanding* GetStanding_Twitch() const;

	/**
	 * The badges the viewer last wore in the streamer's Twitch chat. Twitch has
	 * no way to ask for someone's badges, so these are only as current as their
	 * last chat message; for their roles, use Is Moderator (Twitch) and its
	 * siblings, which ask Twitch. Empty when they have none, have no Twitch
	 * profile, or have not chatted yet.
	 */
	UFUNCTION(BlueprintPure, Category = "Tentacle|Viewer", meta=(DisplayName="Get Badges (Twitch)"))
	TArray<UTclTwitchChatBadge*> GetBadges_Twitch() const;

	/** Whether the viewer last wore a Twitch badge of this set, such as "bits" or "hype-train". */
	UFUNCTION(BlueprintPure, Category = "Tentacle|Viewer", meta=(DisplayName="Has Badge (Twitch)"))
	bool HasBadge_Twitch(const FString& SetId, bool bCaseSensitive = false) const;

	/** Whether the viewer is the streamer, on Twitch. */
	UFUNCTION(BlueprintPure, Category = "Tentacle|Viewer", meta=(DisplayName="Is Broadcaster (Twitch)"))
	bool IsBroadcaster_Twitch() const;

	/**
	 * Whether the viewer moderates the streamer's Twitch channel, Lead
	 * Moderators included. Tentacle asks Twitch rather than waiting for them to
	 * chat. The broadcaster is not a moderator; ask Is Broadcaster (Twitch) as
	 * well.
	 */
	UFUNCTION(BlueprintPure, Category = "Tentacle|Viewer", meta=(DisplayName="Is Moderator (Twitch)"))
	bool IsModerator_Twitch() const;

	/**
	 * Whether the viewer is a VIP in the streamer's Twitch channel. Tentacle
	 * asks Twitch rather than waiting for them to chat.
	 */
	UFUNCTION(BlueprintPure, Category = "Tentacle|Viewer", meta=(DisplayName="Is Vip (Twitch)"))
	bool IsVip_Twitch() const;

	/**
	 * Whether the viewer subscribes to the streamer on Twitch. Tentacle asks
	 * Twitch rather than waiting for them to chat.
	 */
	UFUNCTION(BlueprintPure, Category = "Tentacle|Viewer", meta=(DisplayName="Is Subscriber (Twitch)"))
	bool IsSubscriber_Twitch() const;

	/**
	 * Whether the viewer follows the streamer on Twitch, as of Tentacle's last
	 * check with Twitch. Twitch says nothing when someone unfollows, so an
	 * unfollow shows once Tentacle next looks the viewer up, within a day.
	 */
	UFUNCTION(BlueprintPure, Category = "Tentacle|Viewer", meta=(DisplayName="Is Follower (Twitch)"))
	bool IsFollower_Twitch() const;

	/**
	 * Whether the viewer is banned from the streamer's Twitch chat, or in a
	 * timeout that has not run out yet.
	 */
	UFUNCTION(BlueprintPure, Category = "Tentacle|Viewer", meta=(DisplayName="Is Banned (Twitch)"))
	bool IsBanned_Twitch() const;

	// --- Kick ---

	UFUNCTION(BlueprintPure, Category = "Tentacle|Viewer")
	bool HasKickData() const;

	UFUNCTION(BlueprintPure, Category = "Tentacle|Viewer", meta=(DisplayName="Get Badges (Kick)"))
	TArray<UTclKickBadge*> GetBadges_Kick() const;

	UFUNCTION(BlueprintPure, Category = "Tentacle|Viewer", meta=(DisplayName="Get Channel Slug (Kick)"))
	FString GetChannelSlug_Kick() const;

	UFUNCTION(BlueprintPure, Category = "Tentacle|Viewer", meta=(DisplayName="Get Color (Kick)"))
	FString GetColor_Kick() const;

	UFUNCTION(BlueprintPure, Category = "Tentacle|Viewer", meta=(DisplayName="Get Id (Kick)"))
	int32 GetId_Kick() const;

	UFUNCTION(BlueprintPure, Category = "Tentacle|Viewer", meta=(DisplayName="Is Anonymous (Kick)"))
	bool IsAnonymous_Kick() const;

	UFUNCTION(BlueprintPure, Category = "Tentacle|Viewer", meta=(DisplayName="Is Verified (Kick)"))
	bool IsVerified_Kick() const;

	UFUNCTION(BlueprintPure, Category = "Tentacle|Viewer", meta=(DisplayName="Get Profile Picture (Kick)"))
	FString GetProfilePicture_Kick() const;

	UFUNCTION(BlueprintPure, Category = "Tentacle|Viewer", meta=(DisplayName="Get User Name (Kick)"))
	FString GetUserName_Kick() const;

	/** Whether the viewer last wore a Kick badge of this type, such as "og" or "sub_gifter". */
	UFUNCTION(BlueprintPure, Category = "Tentacle|Viewer", meta=(DisplayName="Has Badge (Kick)"))
	bool HasBadge_Kick(const FString& Type, bool bCaseSensitive = false) const;

	/** Whether the viewer wears Kick's broadcaster badge. */
	UFUNCTION(BlueprintPure, Category = "Tentacle|Viewer", meta=(DisplayName="Is Broadcaster (Kick)"))
	bool IsBroadcaster_Kick() const;

	/**
	 * Whether the viewer moderates the streamer's Kick chat. The broadcaster is
	 * not a moderator; ask Is Broadcaster (Kick) as well.
	 */
	UFUNCTION(BlueprintPure, Category = "Tentacle|Viewer", meta=(DisplayName="Is Moderator (Kick)"))
	bool IsModerator_Kick() const;

	/** Whether the viewer is a VIP in the streamer's Kick chat. */
	UFUNCTION(BlueprintPure, Category = "Tentacle|Viewer", meta=(DisplayName="Is Vip (Kick)"))
	bool IsVip_Kick() const;

	/** Whether the viewer wears Kick's subscriber or founder badge. */
	UFUNCTION(BlueprintPure, Category = "Tentacle|Viewer", meta=(DisplayName="Is Subscriber (Kick)"))
	bool IsSubscriber_Kick() const;

	// --- Properties ---

	/** Returns the value type (String, Number, or Bool) of a viewer property — or String if the property is not found. */
	UFUNCTION(BlueprintPure, Category = "Tentacle|Viewer")
	ETclViewerPropertyType GetPropertyType(const FString& CollectionName, const FString& PropertyName, bool bCaseSensitive = false) const;

	/** Reads a viewer's float property. Returns the stored value on the Found pin, or 0 on the failure pins. */
	UFUNCTION(BlueprintCallable, BlueprintPure=false, Category = "Tentacle|Viewer", meta=(ExpandEnumAsExecs="OutResult", DisplayName="Get Property (Float)"))
	double GetProperty_Float(const FString& CollectionName, const FString& PropertyName, ETclFindPropertyResult& OutResult, bool bCaseSensitive = false) const;

	/** Reads a viewer's int property as a 64-bit integer. Returns the stored whole number on the Found pin, or 0 on the failure pins. Exact up to 2^53, the most the JSON wire carries exactly. */
	UFUNCTION(BlueprintCallable, BlueprintPure=false, Category = "Tentacle|Viewer", meta=(ExpandEnumAsExecs="OutResult", DisplayName="Get Property (Int)"))
	int64 GetProperty_Int(const FString& CollectionName, const FString& PropertyName, ETclFindPropertyResult& OutResult, bool bCaseSensitive = false) const;

	/** Reads a viewer's bool property. Returns the stored value on the Found pin, or false on the failure pins. */
	UFUNCTION(BlueprintCallable, BlueprintPure=false, Category = "Tentacle|Viewer", meta=(ExpandEnumAsExecs="OutResult", DisplayName="Get Property (Bool)"))
	bool GetProperty_Bool(const FString& CollectionName, const FString& PropertyName, ETclFindPropertyResult& OutResult, bool bCaseSensitive = false) const;

	/** Reads a viewer's string property. Returns the stored value on the Found pin, or an empty string on the failure pins. */
	UFUNCTION(BlueprintCallable, BlueprintPure=false, Category = "Tentacle|Viewer", meta=(ExpandEnumAsExecs="OutResult", DisplayName="Get Property (String)"))
	FString GetProperty_String(const FString& CollectionName, const FString& PropertyName, ETclFindPropertyResult& OutResult, bool bCaseSensitive = false) const;

	/** Sets a viewer's float property and syncs it. Returns Value unchanged, for pass-through chaining. */
	UFUNCTION(BlueprintCallable, Category = "Tentacle|Viewer", meta=(ExpandEnumAsExecs="OutResult", DisplayName="Set Property (Float)"))
	double SetProperty_Float(const FString& CollectionName, const FString& PropertyName, double Value, ETclSetPropertyResult& OutResult, bool bCaseSensitive = false);

	/** Sets a viewer's int property (64-bit) and syncs it. Returns Value unchanged, for pass-through chaining. The value is carried to the server as a JSON number, so the effective range is 2^53. */
	UFUNCTION(BlueprintCallable, Category = "Tentacle|Viewer", meta=(ExpandEnumAsExecs="OutResult", DisplayName="Set Property (Int)"))
	int64 SetProperty_Int(const FString& CollectionName, const FString& PropertyName, int64 Value, ETclSetPropertyResult& OutResult, bool bCaseSensitive = false);

	/** Sets a viewer's bool property and syncs it. Returns Value unchanged, for pass-through chaining. */
	UFUNCTION(BlueprintCallable, Category = "Tentacle|Viewer", meta=(ExpandEnumAsExecs="OutResult", DisplayName="Set Property (Bool)"))
	bool SetProperty_Bool(const FString& CollectionName, const FString& PropertyName, bool Value, ETclSetPropertyResult& OutResult, bool bCaseSensitive = false);

	/** Sets a viewer's string property and syncs it. Returns Value unchanged, for pass-through chaining. */
	UFUNCTION(BlueprintCallable, Category = "Tentacle|Viewer", meta=(ExpandEnumAsExecs="OutResult", DisplayName="Set Property (String)"))
	FString SetProperty_String(const FString& CollectionName, const FString& PropertyName, const FString& Value, ETclSetPropertyResult& OutResult, bool bCaseSensitive = false);

	/** Add Amount (may be positive, negative, or zero) to an existing float property. Returns the resulting value. */
	UFUNCTION(BlueprintCallable, Category = "Tentacle|Viewer", meta=(ExpandEnumAsExecs="OutResult", DisplayName="Add To Property (Float)"))
	double AddToProperty_Float(const FString& CollectionName, const FString& PropertyName, double Amount, ETclSetPropertyResult& OutResult, bool bCaseSensitive = false);

	/** Add Amount (may be positive, negative, or zero) to an existing int property (64-bit). Returns the resulting value. */
	UFUNCTION(BlueprintCallable, Category = "Tentacle|Viewer", meta=(ExpandEnumAsExecs="OutResult", DisplayName="Add To Property (Int)"))
	int64 AddToProperty_Int(const FString& CollectionName, const FString& PropertyName, int64 Amount, ETclSetPropertyResult& OutResult, bool bCaseSensitive = false);

	/** Flip an existing bool property to its opposite value. Returns the resulting value. */
	UFUNCTION(BlueprintCallable, Category = "Tentacle|Viewer", meta=(ExpandEnumAsExecs="OutResult", DisplayName="Toggle Property (Bool)"))
	bool ToggleProperty_Bool(const FString& CollectionName, const FString& PropertyName, ETclSetPropertyResult& OutResult, bool bCaseSensitive = false);

	/** Merge properties from already-parsed viewer data into this viewer's property map */
	void MergePropertiesFromData(const UTclViewerData* ViewerData);

	/** Revert every cached property entry in the given collection to its
	 *  definition's default value (bIsDefault = true), discarding any pending
	 *  optimistic local writes, and fire OnPropertyUpdated for each value that
	 *  actually changes. Local-cache counterpart of the server-side collection
	 *  reset (viewerProperty.resetCollectionValues) - the server has already deleted
	 *  every override when this runs. Entries in other collections are untouched. */
	void ResetPropertiesToDefaultsForCollection(const FString& CollectionId);

	/** Revert this viewer's cached property entry for the given definition to
	 *  the definition's default value (bIsDefault = true), discarding any pending
	 *  optimistic local write, and fire OnPropertyUpdated if the value actually
	 *  changes. Local-cache counterpart of the server-side definition reset
	 *  (viewerProperty.resetPropertyValues) - the server has already deleted the
	 *  override when this runs. Entries for other definitions are untouched. */
	void ResetPropertyToDefaultForDefinition(const FString& DefinitionId);

	/** Give up the hold a local write put on an entry, for a write that never
	 *  reached Tentacle, so the next snapshot from Tentacle applies again
	 *  instead of the refused value standing for the rest of the session.
	 *  Written is the entry as the write left it. Only an entry that still holds
	 *  exactly that value is released: a newer write, or an echo that already
	 *  reconciled this one, keeps its own hold. The value itself is left as it
	 *  is until Tentacle sends its own. Returns whether a hold was released. */
	bool ReleaseLocalWrite(const FTclViewerPropertyEntry& Written);

	// --- Mutators ---

	UFUNCTION(BlueprintCallable, Category = "Tentacle|Viewer")
	void UpdateData(UTclViewerData* NewData);

private:
	/** The set-operation outcome that matches a lookup outcome, so the two enums cannot drift. */
	static ETclSetPropertyResult ToSetResult(ETclFindPropertyResult FindResult);

	const FTclViewerPropertyEntry* FindProperty(const FString& CollectionName, const FString& PropertyName, bool bCaseSensitive = false, ETclFindPropertyResult* OutResult = nullptr) const;
	FTclViewerPropertyEntry* FindPropertyMutable(const FString& CollectionName, const FString& PropertyName, bool bCaseSensitive = false, ETclFindPropertyResult* OutResult = nullptr);
	static FString BuildPropertyMapId(const FString& CollectionId, const FString& DefinitionId, const FString& ViewerId);
	static ETclViewerPropertyType TypeStringToEnum(const FString& TypeString);
};
