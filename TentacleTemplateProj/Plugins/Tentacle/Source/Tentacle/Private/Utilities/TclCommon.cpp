// © 2025 Eldritch One. All rights reserved.

#include "Utilities/TclCommon.h"
#include "Tentacle/TentacleModule.h"
#include "Misc/Paths.h"

void
UTclCommon::Debug(const FString& Message)
{
	UE_LOG(LogTentacle, Verbose, TEXT("%s"), *Message);
}

void
UTclCommon::Debug_Text(const FText& Message)
{
	Debug(Message.ToString());
}

void
UTclCommon::DebugDisplay(const FString& Message, int32 Key)
{
	UE_LOG(LogTentacle, Verbose, TEXT("%s"), *Message);
	GEngine->AddOnScreenDebugMessage(Key, 5.0f, FColor::White, Message);
}

void
UTclCommon::DebugDisplay_Text(const FText& Message, int32 Key)
{
	DebugDisplay(Message.ToString(), Key);
}

void
UTclCommon::Info(const FString& Message)
{
	UE_LOG(LogTentacle, Log, TEXT("%s"), *Message);
}

void
UTclCommon::Info_Text(const FText& Message)
{
	Info(Message.ToString());
}

void
UTclCommon::InfoDisplay(const FString& Message, int32 Key)
{
	UE_LOG(LogTentacle, Log, TEXT("%s"), *Message);
	GEngine->AddOnScreenDebugMessage(Key, 5.0f, FColor::Cyan, Message);
}

void
UTclCommon::InfoDisplay_Text(const FText& Message, int32 Key)
{
	InfoDisplay(Message.ToString(), Key);
}

void
UTclCommon::SuccessDisplay(const FString& Message, int32 Key)
{
	UE_LOG(LogTentacle, Log, TEXT("%s"), *Message);
	GEngine->AddOnScreenDebugMessage(Key, 5.0f, FColor::Green, Message);
}

void
UTclCommon::SuccessDisplay_Text(const FText& Message, int32 Key)
{
	SuccessDisplay(Message.ToString(), Key);
}

void
UTclCommon::Warn(const FString& Message)
{
	UE_LOG(LogTentacle, Warning, TEXT("%s"), *Message);
}

void
UTclCommon::Warn_Text(const FText& Message)
{
	Warn(Message.ToString());
}

void
UTclCommon::WarnDisplay(const FString& Message, int32 Key)
{
	UE_LOG(LogTentacle, Warning, TEXT("%s"), *Message);
	GEngine->AddOnScreenDebugMessage(Key, 5.0f, FColor::Yellow, Message);
}

void
UTclCommon::WarnDisplay_Text(const FText& Message, int32 Key)
{
	WarnDisplay(Message.ToString(), Key);
}

void
UTclCommon::Error(const FString& Message)
{
	UE_LOG(LogTentacle, Error, TEXT("%s"), *Message);
}

void
UTclCommon::Error_Text(const FText& Message)
{
	Error(Message.ToString());
}

void
UTclCommon::ErrorDisplay(const FString& Message, int32 Key)
{
	UE_LOG(LogTentacle, Error, TEXT("%s"), *Message);
	GEngine->AddOnScreenDebugMessage(Key, 30.0f, FColor::Red, Message);
}

void
UTclCommon::ErrorDisplay_Text(const FText& Message, int32 Key)
{
	ErrorDisplay(Message.ToString(), Key);
}

void
UTclCommon::HandleAssert(const FString& File, const int32 Line, const FString& Message)
{
	const auto ErrorMessage = FString::Format(TEXT("Assert failed: {0}, line {1}, {2}"), {FPaths::GetCleanFilename(File), Line, Message});
	UE_LOG(LogTentacle, Error, TEXT("%s"), *ErrorMessage);
	ErrorDisplay(ErrorMessage);
}

FString
UTclCommon::MakeGuid()
{
	return FGuid::NewGuid().ToString();
}

FString
UTclCommon::MakeId()
{
	static auto Id = 0;
	return FString::FromInt(Id++);
}

bool
UTclCommon::IsRunningInEditor()
{
#if WITH_EDITOR
	return true;
#else
	return false;
#endif
}
