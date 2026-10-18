// © 2025 Eldritch One. All rights reserved.

#include "Base/Tentacle_Connections.h"
#include "Base/Tentacle.h"
#include "Sdk/TclMsg_ServerToClient_ConnectionsPrompt.h"
#include "Sdk/TclMsg_ServerToClient_ConnectionsResult.h"
#include "Sdk/TclMsg_ServerToClient_ConnectionsStatus.h"
#include "Sdk/TclPlatformConnection.h"
#include "Sdk/TclPlatformConnectionPrompt.h"
#include "Sdk/TclPlatformConnectionResult.h"
#include "Utilities/TclCommon.h"
#include "Utilities/TclEnumUtils.h"
#include "Dom/JsonObject.h"
#include "Policies/CondensedJsonPrintPolicy.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

UTclPlatformConnection*
UTentacle_Connections::GetConnection(ETclStreamPlatform Platform, bool& Found) const
{
	UTclPlatformConnection* const* Connection = Connections.Find(Platform);
	Found = Connection != nullptr && *Connection != nullptr;
	return Found ? *Connection : nullptr;
}

void
UTentacle_Connections::Connect(ETclStreamPlatform Platform)
{
	SendFor(TEXT("Connections_Connect"), Platform);
}

void
UTentacle_Connections::CancelConnect(ETclStreamPlatform Platform)
{
	SendFor(TEXT("Connections_CancelConnect"), Platform);
}

void
UTentacle_Connections::Disconnect(ETclStreamPlatform Platform)
{
	SendFor(TEXT("Connections_Disconnect"), Platform);
}

void
UTentacle_Connections::HandleMsg_Status(UTclMsg_ServerToClient_ConnectionsStatus* Msg)
{
	if (Msg == nullptr || Msg->Data == nullptr)
		return;

	Connections.Add(Msg->Data->Platform, Msg->Data);
	OnConnectionChanged.Broadcast(Msg->Data);
}

void
UTentacle_Connections::HandleMsg_Prompt(UTclMsg_ServerToClient_ConnectionsPrompt* Msg)
{
	if (Msg != nullptr && Msg->Data != nullptr)
		OnConnectPrompt.Broadcast(Msg->Data);
}

void
UTentacle_Connections::HandleMsg_Result(UTclMsg_ServerToClient_ConnectionsResult* Msg)
{
	if (Msg != nullptr && Msg->Data != nullptr)
		OnConnectResult.Broadcast(Msg->Data);
}

void
UTentacle_Connections::SendFor(const TCHAR* MessageType, ETclStreamPlatform Platform)
{
	ATentacle* Tentacle = GetOwner<ATentacle>();
	tclSoftAssert(Tentacle);

	const TSharedRef<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("platform"), TclEnumUtils::StreamPlatformToString(Platform));

	const TSharedRef<FJsonObject> Message = MakeShared<FJsonObject>();
	Message->SetStringField(TEXT("type"), MessageType);
	Message->SetObjectField(TEXT("data"), Data);

	FString Json;
	const auto Writer = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Json);
	FJsonSerializer::Serialize(Message, Writer);

	Tentacle->SendMsg(Json);
}
