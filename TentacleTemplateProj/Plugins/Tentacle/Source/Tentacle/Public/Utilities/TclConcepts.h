// © 2025 Eldritch One. All rights reserved.

#pragma once
#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

template <typename T>
concept IsNumber = std::disjunction_v<
	std::is_same<T, int32>,
	std::is_same<T, int64>,
	std::is_same<T, float>,
	std::is_same<T, double>
>;

template <typename T>
concept IsJsonValue = std::disjunction_v<
	std::is_same<T, bool>,
	std::is_same<T, int32>,
	std::is_same<T, int64>,
	std::is_same<T, float>,
	std::is_same<T, double>,
	std::is_same<T, FString>,
	std::is_same<T, TSharedPtr<FJsonObject>>,
	std::is_same<T, FName>,
	std::is_same<T, FDateTime>,
	std::is_same<T, FLinearColor>,
	std::is_same<T, TArray<bool>>,
	std::is_same<T, TArray<int32>>,
	std::is_same<T, TArray<int64>>,
	std::is_same<T, TArray<float>>,
	std::is_same<T, TArray<double>>,
	std::is_same<T, TArray<FString>>,
	std::is_same<T, TArray<TSharedPtr<FJsonObject>>>,
	std::is_same<T, TArray<FName>>,
	std::is_same<T, TArray<FDateTime>>,
	std::is_same<T, TArray<FLinearColor>>
>;
