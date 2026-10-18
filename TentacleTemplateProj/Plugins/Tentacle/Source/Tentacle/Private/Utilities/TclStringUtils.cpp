// © 2023 Eldritch One. All rights reserved.

#include "Utilities/TclStringUtils.h"

int32 UTclStringUtils::CountOccurences(const FString& Haystack, const FString& Needle, bool bCaseSensitive)
{
	auto Count = 0;
	auto StartPosition = 0;
	const ESearchCase::Type SearchCase = bCaseSensitive ? ESearchCase::CaseSensitive : ESearchCase::IgnoreCase;

	while (true)
	{
		const auto FoundPosition = Haystack.Find(Needle, SearchCase, ESearchDir::FromStart, StartPosition);
		if (FoundPosition == INDEX_NONE)
			break;

		Count++;
		StartPosition = FoundPosition + Needle.Len();
	}

	return Count;
}

FString UTclStringUtils::FindClosestMatch(const TArray<FString>& Options, const FString& Input, bool bCaseSensitive)
{
	auto ClosestMatch = FString();
	auto MinDistance = TNumericLimits<int32>::Max();

	for (const auto& Option : Options)
	{
		const auto Distance = LevenshteinDistance(Input, Option, bCaseSensitive);
		if (Distance < MinDistance)
		{
			MinDistance = Distance;
			ClosestMatch = Option;
		}
	}

	return ClosestMatch;
}

FName UTclStringUtils::FindClosestMatchName(const TArray<FName>& Options, FName Input, bool bCaseSensitive)
{
	auto OptionsStrings = TArray<FString>();
	for (const auto& Option : Options)
		OptionsStrings.Add(Option.ToString());

	return FName(FindClosestMatch(OptionsStrings, Input.ToString(), bCaseSensitive));
}

int32 UTclStringUtils::LevenshteinDistance(const FString& Source, const FString& Target, bool bCaseSensitive)
{
	const auto SourceLength = Source.Len();
	const auto TargetLength = Target.Len();

	// Create a DP table with (SourceLength + 1) rows and (TargetLength + 1) columns
	auto DP = TArray<TArray<int32>>();
	DP.SetNum(SourceLength + 1);

	// Initialize the first column of each row
	for (auto i = 0; i <= SourceLength; ++i)
	{
		DP[i].SetNum(TargetLength + 1);
		DP[i][0] = i; // Cost of transforming the first i characters of Source to an empty string
	}

	// Initialize the first row of each column
	for (auto j = 0; j <= TargetLength; ++j)
		DP[0][j] = j; // Cost of transforming an empty string to the first j characters of Target

	// Fill the DP table
	for (auto i = 1; i <= SourceLength; ++i)
	{
		for (auto j = 1; j <= TargetLength; ++j)
		{
			const TCHAR S = bCaseSensitive ? Source[i - 1] : FChar::ToLower(Source[i - 1]);
			const TCHAR T = bCaseSensitive ? Target[j - 1] : FChar::ToLower(Target[j - 1]);
			// Cost of substitution (0 if the characters are the same, 1 if they are different)
			const auto Cost = (S == T) ? 0 : 1;
			DP[i][j] = FMath::Min3(
				DP[i - 1][j] + 1,		// Deletion
				DP[i][j - 1] + 1,		// Insertion
				DP[i - 1][j - 1] + Cost // Substitution
			);
		}
	}

	// The last cell of the table contains the Levenshtein distance
	return DP[SourceLength][TargetLength];
}
