// © 2025 Eldritch One. All rights reserved.

#pragma once
#include "CoreMinimal.h"
#include "TentacleModule.h"
#include "Sdk/TclViewerPropertyType.h"
#include "Sdk/TclOrderDirection.h"
#include "Sdk/TclStreamPlatform.h"

/**
 * Bidirectional SDK string <-> UE enum converters.
 * Replaces EnumUtils.ts.
 */
namespace TclEnumUtils
{
	// --- ViewerPropertyType ---

	inline ETclViewerPropertyType
	ViewerPropertyTypeFromString(const FString& Str)
	{
		if (Str == TEXT("float"))  return ETclViewerPropertyType::Float;
		if (Str == TEXT("int"))    return ETclViewerPropertyType::Int;
		if (Str == TEXT("bool"))   return ETclViewerPropertyType::Bool;
		if (Str == TEXT("string")) return ETclViewerPropertyType::String;

		UE_LOG(LogTentacle, Error, TEXT("TclEnumUtils: Unknown ViewerPropertyType '%s'"), *Str);
		return ETclViewerPropertyType::String;
	}

	inline FString
	ViewerPropertyTypeToString(ETclViewerPropertyType Value)
	{
		switch (Value)
		{
		case ETclViewerPropertyType::Float:  return TEXT("float");
		case ETclViewerPropertyType::Int:    return TEXT("int");
		case ETclViewerPropertyType::Bool:   return TEXT("bool");
		case ETclViewerPropertyType::String: return TEXT("string");
		default:
			UE_LOG(LogTentacle, Error, TEXT("TclEnumUtils: Unknown ViewerPropertyType enum value %d"), static_cast<int>(Value));
			return TEXT("string");
		}
	}

	// --- OrderDirection ---

	inline ETclOrderDirection
	OrderDirectionFromString(const FString& Str)
	{
		if (Str == TEXT("asc"))  return ETclOrderDirection::Asc;
		if (Str == TEXT("desc")) return ETclOrderDirection::Desc;

		UE_LOG(LogTentacle, Error, TEXT("TclEnumUtils: Unknown OrderDirection '%s'"), *Str);
		return ETclOrderDirection::Asc;
	}

	inline FString
	OrderDirectionToString(ETclOrderDirection Value)
	{
		switch (Value)
		{
		case ETclOrderDirection::Asc:  return TEXT("asc");
		case ETclOrderDirection::Desc: return TEXT("desc");
		default:
			UE_LOG(LogTentacle, Error, TEXT("TclEnumUtils: Unknown OrderDirection enum value %d"), static_cast<int>(Value));
			return TEXT("asc");
		}
	}

	// --- StreamPlatform ---

	inline ETclStreamPlatform
	StreamPlatformFromString(const FString& Str)
	{
		if (Str == TEXT("twitch")) return ETclStreamPlatform::Twitch;
		if (Str == TEXT("kick"))   return ETclStreamPlatform::Kick;

		UE_LOG(LogTentacle, Error, TEXT("TclEnumUtils: Unknown StreamPlatform '%s'"), *Str);
		return ETclStreamPlatform::Twitch;
	}

	inline FString
	StreamPlatformToString(ETclStreamPlatform Value)
	{
		switch (Value)
		{
		case ETclStreamPlatform::Twitch: return TEXT("twitch");
		case ETclStreamPlatform::Kick:   return TEXT("kick");
		default:
			UE_LOG(LogTentacle, Error, TEXT("TclEnumUtils: Unknown StreamPlatform enum value %d"), static_cast<int>(Value));
			return TEXT("twitch");
		}
	}
}
