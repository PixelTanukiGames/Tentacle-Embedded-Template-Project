// © 2025 Eldritch One. All rights reserved.

#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "TclStringUtils.generated.h"

/**
 * Utilities for working with strings.
 */
UCLASS()
class TENTACLE_API UTclStringUtils : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Counts the number of times a substring occurs in a string.
	 *
	 * @param Haystack The string to search in.
	 * @param Needle The substring to search for.
	 * @return The number of times the substring occurs in the string.
	 */
	UFUNCTION(BlueprintPure, Category="Tentacle|String Utils")
	static int32
	CountOccurences(const FString& Haystack, const FString& Needle, bool bCaseSensitive = false);

	/**
	 * Find the closest match among a list of strings.
	 *
	 * @param Options The list of strings to search in.
	 * @param Input The string to search for.
	 * @return The closest match among the options.
	 */
	UFUNCTION(BlueprintPure, Category="Tentacle|String Utils")
	static FString
	FindClosestMatch(const TArray<FString>& Options, const FString& Input, bool bCaseSensitive = false);

	/**
	 * Find the closest match among a list of names.
	 *
	 * @param Options The list of names to search in.
	 * @param Input The name to search for.
	 * @return The closest match among the options.
	 */
	UFUNCTION(BlueprintPure, Category="Tentacle|String Utils")
	static FName
	FindClosestMatchName(const TArray<FName>& Options, FName Input, bool bCaseSensitive = false);

	/**
	 * Calculate the Levenshtein distance between two strings (for finding matches).
	 *
	 * @param Source The source string.
	 * @param Target The target string.
	 * @return The Levenshtein distance between the two strings.
	 */
	UFUNCTION(BlueprintPure, Category="Tentacle|String Utils")
	static int32
	LevenshteinDistance(const FString& Source, const FString& Target, bool bCaseSensitive = false);
};
