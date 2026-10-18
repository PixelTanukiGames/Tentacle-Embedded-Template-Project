// © 2025 Eldritch One. All rights reserved.

#include "Embedded/TclConnectionProcedures.h"
#include "Utilities/TclCommon.h"
#include "Dom/JsonObject.h"
#include "Policies/CondensedJsonPrintPolicy.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FTclConnectionProcedures::FTclConnectionProcedures(TFunction<bool(const FString& Frame)> InSend)
	: Send(MoveTemp(InSend))
{
}

void
FTclConnectionProcedures::Call(const FString& Procedure, bool bIsQuery, const FString& InputJson,
	const UTentacleHttpClient::FOnSuccess& OnSuccess, const UTentacleHttpClient::FOnError& OnError)
{
	const FString Id = FString::Printf(TEXT("rpc-%llu"), NextId++);

	// The input travels as the text a query puts in ?input= or a mutation sends
	// as its body; the core reads an absent one as no input, as tRPC does.
	const TSharedRef<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("id"), Id);
	Data->SetStringField(TEXT("procedure"), Procedure);
	if (!InputJson.IsEmpty())
		Data->SetStringField(TEXT("input"), InputJson);

	const TSharedRef<FJsonObject> Message = MakeShared<FJsonObject>();
	Message->SetStringField(TEXT("type"), TEXT("Rpc_Request"));
	Message->SetObjectField(TEXT("data"), Data);

	FString Frame;
	const auto Writer = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Frame);
	FJsonSerializer::Serialize(Message, Writer);

	Pending.Add(Id, FPendingCall{ Procedure, OnSuccess, OnError });
	if (!Send || !Send(Frame))
	{
		Pending.Remove(Id);
		tclDebugF("Tentacle: %s failed: Tentacle is not connected", *Procedure);
		OnError.ExecuteIfBound(0, TEXT("Tentacle is not connected"));
	}
}

void
FTclConnectionProcedures::HandleResponse(const FString& Id, int32 Status, const FString& Body)
{
	FPendingCall Call;
	if (!Pending.RemoveAndCopyValue(Id, Call))
	{
		tclDebugF("Tentacle: ignoring an answer to no waiting call (%s)", *Id);
		return;
	}

	if (Status >= 400)
	{
		tclWarnF("Tentacle: %s failed (%d): %s", *Call.Procedure, Status, *Body.Left(500));
		Call.OnError.ExecuteIfBound(Status, Body);
		return;
	}

	Call.OnSuccess.ExecuteIfBound(Body);
}

void
FTclConnectionProcedures::FailAll(const FString& Why)
{
	// Moved out first: a callback may make a new call, which must not land in
	// the map being emptied.
	TMap<FString, FPendingCall> Waiting = MoveTemp(Pending);
	Pending.Reset();
	for (const TPair<FString, FPendingCall>& Entry : Waiting)
		Entry.Value.OnError.ExecuteIfBound(0, Why);
}
