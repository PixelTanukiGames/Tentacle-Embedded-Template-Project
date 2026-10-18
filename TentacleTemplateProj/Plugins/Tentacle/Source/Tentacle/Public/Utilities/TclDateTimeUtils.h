// © 2025 Eldritch One. All rights reserved.

#pragma once
#include "Kismet/BlueprintFunctionLibrary.h"
#include "TclDateTimeUtils.generated.h"

UCLASS()
class TENTACLE_API UTclDateTimeUtils : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	UFUNCTION(BlueprintPure, Category="Tentacle|DateTime Utils")
	static FDateTime
	FromIso(const FString& Iso)
	{
		auto DateTime = FDateTime();
		FDateTime::ParseIso8601(*Iso, DateTime);
		return DateTime;
	}

	UFUNCTION(BlueprintPure, Category="Tentacle|DateTime Utils")
	static FString
	ToIso(const FDateTime& DateTime)
	{
		return DateTime.ToIso8601();
	}
};
