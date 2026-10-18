// © 2025 Eldritch One. All rights reserved.

#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "TclCommon.generated.h"

/// @cond

UCLASS()
class TENTACLE_API UTclCommon : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	inline static const auto SpawnedTag = FName(TEXT("TentacleSpawned"));

	template <typename T>
		requires TIsDerivedFrom<T, UActorComponent>::IsDerived
	static auto
	AddComponent(AActor* Actor) -> T*
	{
		return Cast<T>(Actor->AddComponentByClass(T::StaticClass(), false, FTransform(), false));
	}

	template <typename T>
		requires TIsDerivedFrom<T, AActor>::IsDerived
	static T*
	GetActorOfClass(const UObject* WorldContext)
	{
		return Cast<T>(UGameplayStatics::GetActorOfClass(WorldContext, T::StaticClass()));
	}

	template <typename T, class AllocatorType>
		requires TIsDerivedFrom<T, AActor>::IsDerived
	static void
	GetAllActorsOfClass(const UObject* WorldContext, TArray<T*, AllocatorType>& OutActors)
	{
		TArray<AActor*> Actors;
		UGameplayStatics::GetAllActorsOfClass(WorldContext, T::StaticClass(), Actors);

		for (auto Actor : Actors)
			OutActors.Add(Cast<T>(Actor));
	}

	template <typename T>
		requires TIsDerivedFrom<T, AActor>::IsDerived
	static T*
	GetActorOfClassWithTag(const UObject* WorldContext, FName Tag)
	{
		auto Actors = TArray<AActor*>();
		UGameplayStatics::GetAllActorsOfClassWithTag(WorldContext, T::StaticClass(), Tag, Actors);
		return Actors.Num() > 0 ? Cast<T>(Actors[0]) : nullptr;
	}

	static void
	GetAllSpawnedActors(const UObject* WorldContext, TArray<AActor*>& OutActors)
	{
		UGameplayStatics::GetAllActorsWithTag(WorldContext, SpawnedTag, OutActors);
	}

	template <typename T>
		requires TIsDerivedFrom<T, AActor>::IsDerived
	static T*
	SpawnActor(const UObject* WorldContext, const FActorSpawnParameters& SpawnParameters)
	{
		T* Actor = SpawnActor<T>(WorldContext, T::StaticClass(), FTransform(), SpawnParameters);
		Actor->Tags.Add(SpawnedTag);
		return Actor;
	}

	template <typename T>
		requires TIsDerivedFrom<T, AActor>::IsDerived
	static T*
	SpawnActor(const UObject* WorldContext, UClass* Class, const FActorSpawnParameters& SpawnParameters)
	{
		return SpawnActor<T>(WorldContext, Class, FTransform(), SpawnParameters);
	}

	template <typename T>
		requires TIsDerivedFrom<T, AActor>::IsDerived
	static T*
	SpawnActor(
		const UObject* WorldContext,
		const FTransform& Transform = FTransform(),
		const FActorSpawnParameters& SpawnParameters = FActorSpawnParameters()
	)
	{
		return SpawnActor<T>(WorldContext, T::StaticClass(), Transform, SpawnParameters);
	}

	template <typename T>
		requires TIsDerivedFrom<T, AActor>::IsDerived
	static T*
	SpawnActor(
		const UObject* WorldContext,
		UClass* Class,
		const FTransform& Transform = FTransform(),
		const FActorSpawnParameters& SpawnParameters = FActorSpawnParameters()
	)
	{
		const auto World = GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::Assert);
		T* Actor = World->SpawnActor<T>(Class, Transform, SpawnParameters);
		Actor->Tags.Add(SpawnedTag);
		return Actor;
	}

	UFUNCTION(BlueprintCallable, Category="Tentacle|Debug", DisplayName="Debug (String)")
	static void
	Debug(const FString& Message);

	UFUNCTION(BlueprintCallable, Category="Tentacle|Debug", DisplayName="Debug (Text)")
	static void
	Debug_Text(const FText& Message);

	UFUNCTION(BlueprintCallable, Category="Tentacle|Debug", DisplayName="Debug Display (String)")
	static void
	DebugDisplay(const FString& Message, int32 Key = -1);

	UFUNCTION(BlueprintCallable, Category="Tentacle|Debug", DisplayName="Debug Display (Text)")
	static void
	DebugDisplay_Text(const FText& Message, int32 Key = -1);

	UFUNCTION(BlueprintCallable, Category="Tentacle|Debug", DisplayName="Info (String)")
	static void
	Info(const FString& Message);

	UFUNCTION(BlueprintCallable, Category="Tentacle|Debug", DisplayName="Info (Text)")
	static void
	Info_Text(const FText& Message);

	UFUNCTION(BlueprintCallable, Category="Tentacle|Debug", DisplayName="Info Display (String)")
	static void
	InfoDisplay(const FString& Message, int32 Key = -1);

	UFUNCTION(BlueprintCallable, Category="Tentacle|Debug", DisplayName="Info Display (Text)")
	static void
	InfoDisplay_Text(const FText& Message, int32 Key = -1);

	UFUNCTION(BlueprintCallable, Category="Tentacle|Debug", DisplayName="Success Display (String)")
	static void
	SuccessDisplay(const FString& Message, int32 Key = -1);

	UFUNCTION(BlueprintCallable, Category="Tentacle|Debug", DisplayName="Success Display (Text)")
	static void
	SuccessDisplay_Text(const FText& Message, int32 Key = -1);

	UFUNCTION(BlueprintCallable, Category="Tentacle|Debug", DisplayName="Warn (String)")
	static void
	Warn(const FString& Message);

	UFUNCTION(BlueprintCallable, Category="Tentacle|Debug", DisplayName="Warn (Text)")
	static void
	Warn_Text(const FText& Message);

	UFUNCTION(BlueprintCallable, Category="Tentacle|Debug", DisplayName="Warn Display (String)")
	static void
	WarnDisplay(const FString& Message, int32 Key = -1);

	UFUNCTION(BlueprintCallable, Category="Tentacle|Debug", DisplayName="Warn Display (Text)")
	static void
	WarnDisplay_Text(const FText& Message, int32 Key = -1);

	UFUNCTION(BlueprintCallable, Category="Tentacle|Debug", DisplayName="Error (String)")
	static void
	Error(const FString& Message);

	UFUNCTION(BlueprintCallable, Category="Tentacle|Debug", DisplayName="Error (Text)")
	static void
	Error_Text(const FText& Message);

	UFUNCTION(BlueprintCallable, Category="Tentacle|Debug", DisplayName="Error Display (String)")
	static void
	ErrorDisplay(const FString& Message, int32 Key = -1);

	UFUNCTION(BlueprintCallable, Category="Tentacle|Debug", DisplayName="Error Display (Text)")
	static void
	ErrorDisplay_Text(const FText& Message, int32 Key = -1);

	static void
	HandleAssert(const FString& File, const int32 Line, const FString& Message);

	UFUNCTION(BlueprintPure, Category="Tentacle|Common")
	static FString
	MakeGuid();

	UFUNCTION(BlueprintPure, Category="Tentacle|Common")
	static FString
	MakeId();

	UFUNCTION(BlueprintPure, Category="Tentacle|Common")
	static bool
	IsRunningInEditor();
};

#define tclDebug(Msg) \
	UTclCommon::Debug(TEXT(Msg))

#define tclDebugF(MsgTemplate, ...) \
	UTclCommon::Debug(FString::Printf(TEXT(MsgTemplate), __VA_ARGS__))

#define tclInfo(Msg) \
	UTclCommon::Info(TEXT(Msg))

#define tclInfoF(MsgTemplate, ...) \
	UTclCommon::Info(FString::Printf(TEXT(MsgTemplate), __VA_ARGS__))

#define tclSuccess(Msg) \
	UTclCommon::Success(TEXT(Msg))

#define tclSuccessF(MsgTemplate, ...) \
	UTclCommon::Success(FString::Printf(TEXT(MsgTemplate), __VA_ARGS__))

#define tclWarn(Msg) \
	UTclCommon::Warn(TEXT(Msg))

#define tclWarnF(MsgTemplate, ...) \
	UTclCommon::Warn(FString::Printf(TEXT(MsgTemplate), __VA_ARGS__))

#define tclError(Msg) \
	UTclCommon::Error(TEXT(Msg))

#define tclErrorF(MsgTemplate, ...) \
	UTclCommon::Error(FString::Printf(TEXT(MsgTemplate), __VA_ARGS__))

#define tclSoftAssertValid(ObjectPtr) \
	tclSoftAssert(IsValid(ObjectPtr))

#define tclSoftAssertValidR(ObjectPtr, Return) \
	tclSoftAssertR(IsValid(ObjectPtr), Return)

#define tclSoftAssert(Condition) \
	if (!(Condition)) \
	{ \
		UTclCommon::HandleAssert(__FILE__, __LINE__, #Condition); \
		return; \
	}

#define tclSoftAssertR(Condition, Return) \
	if (!(Condition)) \
	{ \
		UTclCommon::HandleAssert(__FILE__, __LINE__, #Condition); \
		return Return; \
	}

#define tclSoftAssertRErr(Condition, Message) \
	if (!(Condition)) \
	{ \
		UTclCommon::HandleAssert(__FILE__, __LINE__, #Condition); \
		return FErr(Message); \
	}

#define tclSoftAssertMsg(Condition, Message) \
	if (!(Condition)) \
	{ \
		UTclCommon::HandleAssert(__FILE__, __LINE__, Message); \
		return; \
	}

#define tclSoftAssertMsgF(Condition, MsgTemplate, ...) \
	if (!(Condition)) \
	{ \
		UTclCommon::HandleAssert(__FILE__, __LINE__, FString::Printf(TEXT(MsgTemplate), __VA_ARGS__)); \
		return; \
	}

#define tclSoftAssertMsgR(Condition, Message, Return) \
	if (!(Condition)) \
	{ \
		UTclCommon::HandleAssert(__FILE__, __LINE__, Message); \
		return Return; \
	}

#define tclSilentAssert(Condition) \
	if (!(Condition)) \
	  return;

#define tclSilentAssertR(Condition, Return) \
	if (!(Condition)) \
	  return Return;

#define tclSilentAssertValid(ObjectPtr) \
	tclSilentAssert(IsValid(ObjectPtr))

#define tclSilentAssertValidR(ObjectPtr, Return) \
	tclSilentAssertR(IsValid(ObjectPtr), Return)

#define tclThruAssert(Condition) \
	if (!(Condition)) \
	{ \
		UTclCommon::HandleAssert(__FILE__, __LINE__, #Condition); \
	}

#define tclThruAssertValid(ObjectPtr) \
	tclThruAssert(IsValid(ObjectPtr))

#define tclThruAssertMsg(Condition, Message) \
	if (!(Condition)) \
	{ \
		UTclCommon::HandleAssert(__FILE__, __LINE__, Message); \
	}

#define tclTodo() \
	tclSoftAssertMsgF(false, "TODO: %s, line %s", *FPaths::GetCleanFilename(__FILE__), __LINE__);

#define tclTodoReturn(Return) \
	tclTodo() \
	return Return;

#define tclNotImplemented() \
	tclSoftAssertMsgF(false, "Not Implemented: %s, line %s", *FPaths::GetCleanFilename(__FILE__), __LINE__);

#define tclNotImplementedR(Return) \
	tclNotImplemented() \
	return Return;

#define tclAddDynamicSafe(Delegate, Function) \
	if (!Delegate.IsAlreadyBound(this, &Function)) \
		Delegate.AddDynamic(this, &Function);

#define tclRemoveDynamicSafe(Delegate, Function) \
	if (Delegate.IsAlreadyBound(this, &Function)) \
		Delegate.RemoveDynamic(this, &Function);

#define tclLambdaCapture(ObjPtr, CaptureName) \
	_##CaptureName##Weak=MakeWeakObjectPtr(ObjPtr)

#define tclLambdaGet(CaptureName) \
	const auto CaptureName = _##CaptureName##Weak.Get(); \
	if (!CaptureName) \
		return;

#define tclLambdaForward(CaptureName) \
	_##CaptureName##Weak

/// @endcond
