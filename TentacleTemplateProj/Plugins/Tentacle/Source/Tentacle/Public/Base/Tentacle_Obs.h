// © 2025 Eldritch One. All rights reserved.

#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Base/TclDefinitionChange.h"
#include "Dom/JsonObject.h"
#include "Tentacle_Obs.generated.h"

UCLASS()
class TENTACLE_API UTentacle_Obs : public UActorComponent
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Tentacle|OBS")
	void SetSourceFilterEnabled(const FString& SourceName, const FString& FilterName, bool bEnabled);

	/** Switch OBS to a scene by name. */
	UFUNCTION(BlueprintCallable, Category="Tentacle|OBS")
	void SetCurrentProgramScene(const FString& SceneName);

	/**
	 * Send any OBS WebSocket request by name, through the OBS connection
	 * Tentacle already holds. Fire and forget: OBS sends nothing back.
	 *
	 * @param RequestType     The request's name as the OBS WebSocket protocol
	 *                        spells it, for example SetInputMute.
	 * @param RequestDataJson The request's fields as a JSON object, for example
	 *                        {"inputName": "Mic", "inputMuted": true}. Leave it
	 *                        empty for a request that takes no fields. Anything
	 *                        that is not a JSON object is refused with a warning
	 *                        in the log and nothing is sent.
	 */
	UFUNCTION(BlueprintCallable, Category="Tentacle|OBS", meta=(DisplayName="Call"))
	void CallRequest(const FString& RequestType, const FString& RequestDataJson);

	/**
	 * Send any OBS WebSocket request by name, from C++. A null RequestData sends
	 * the request with no fields.
	 */
	void Call(const FString& RequestType, const TSharedPtr<FJsonObject>& RequestData);

	// The Blueprint default for Port is metadata keyed by the parameter's name,
	// because the completion pin comes last, as it does in the other plugins, and
	// C++ defaults would have to come after it. (Maintainer notes go above the
	// doc comment: UHT takes the comment nearest a UFUNCTION as its tooltip.)

	/**
	 * Tell Tentacle where OBS is, as the OBS fields in Tentacle's settings do,
	 * and Tentacle connects to it there.
	 *
	 * @param Host       The machine OBS runs on; empty means this one.
	 * @param Port       OBS's websocket port, 4455 unless it was changed in OBS.
	 * @param Password   OBS's websocket password, empty when it has none.
	 * @param OnComplete Runs once with Success, or Failed when Tentacle is not
	 *                   running or refused the settings, such as a port outside
	 *                   1 to 65535 (the reason is logged).
	 */
	UFUNCTION(BlueprintCallable, Category="Tentacle|OBS", meta=(AutoCreateRefTerm="OnComplete", Port="4455"))
	void SetConnectionSettings(const FString& Host, int32 Port, const FString& Password, const FTclOnDefinitionComplete& OnComplete);
};
