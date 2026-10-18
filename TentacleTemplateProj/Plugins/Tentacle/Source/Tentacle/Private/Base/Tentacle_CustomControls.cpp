// © 2025 Eldritch One. All rights reserved.

#include "Base/Tentacle_CustomControls.h"
#include "Base/Tentacle.h"
#include "Base/TclCustomControlComponent.h"
#include "Sdk/TclMsg_ServerToClient_CustomControlsInitialize.h"
#include "Sdk/TclMsg_ServerToClient_CustomControlsControlEvent.h"
#include "Sdk/TclCustomControlEvent.h"
#include "Sdk/TclCustomControlEvent_Click.h"
#include "Sdk/TclCustomControlEvent_MouseDown.h"
#include "Sdk/TclCustomControlEvent_MouseUp.h"
#include "Sdk/TclCustomControlEvent_ValueChange.h"
#include "Sdk/TclCustomControlEvent_ValueCommit.h"
#include "Utilities/TclCommon.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonWriter.h"
#include "Policies/CondensedJsonPrintPolicy.h"
#include "Serialization/JsonSerializer.h"

void
UTentacle_CustomControls::RegisterCustomControl(UTclCustomControlComponent* Control)
{
	if (!Control) return;

	// Check not already registered
	for (auto* C : CustomControls)
	{
		if (C && C->GetId() == Control->GetId())
		{
			tclWarnF("Custom control with id %s already registered", *Control->GetId());
			return;
		}
	}

	CustomControls.Add(Control);
	SendMsg_RegisterControl(Control);
}

void
UTentacle_CustomControls::UnregisterCustomControl(UTclCustomControlComponent* Control)
{
	if (!Control) return;

	const int32 Removed = CustomControls.RemoveAll([&Control](const UTclCustomControlComponent* C)
	{
		return C && C->GetId() == Control->GetId();
	});

	if (Removed > 0)
		SendMsg_UnregisterControl(Control);
}

UTclCustomControlComponent*
UTentacle_CustomControls::FindCustomControlById(const FString& ControlId, bool& Found) const
{
	for (UTclCustomControlComponent* Control : CustomControls)
	{
		if (Control && Control->GetId() == ControlId)
		{
			Found = true;
			return Control;
		}
	}

	Found = false;
	return nullptr;
}

void
UTentacle_CustomControls::HandleMsg_Initialize(UTclMsg_ServerToClient_CustomControlsInitialize* Msg)
{
	// Re-register all controls (reconnect flow)
	for (auto* Control : CustomControls)
	{
		if (Control)
			SendMsg_RegisterControl(Control);
	}
}

void
UTentacle_CustomControls::HandleMsg_ControlEvent(UTclMsg_ServerToClient_CustomControlsControlEvent* Msg)
{
	tclSoftAssert(Msg && Msg->Event);

	const FString& EventControlId = Msg->Event->ControlId;

	bool bFound = false;
	UTclCustomControlComponent* FoundControl = FindCustomControlById(EventControlId, bFound);

	if (!FoundControl)
	{
		tclWarnF("Received control event for unregistered control %s", *EventControlId);
		return;
	}

	if (auto* Click = Cast<UTclCustomControlEvent_Click>(Msg->Event))
	{
		FoundControl->ReceiveClick();
	}
	else if (auto* MouseDown = Cast<UTclCustomControlEvent_MouseDown>(Msg->Event))
	{
		FoundControl->ReceiveMouseDown();
	}
	else if (auto* MouseUp = Cast<UTclCustomControlEvent_MouseUp>(Msg->Event))
	{
		FoundControl->ReceiveMouseUp();
	}
	else if (auto* ValueChange = Cast<UTclCustomControlEvent_ValueChange>(Msg->Event))
	{
		FoundControl->ReceiveValueChange(ValueChange);
	}
	else if (auto* ValueCommit = Cast<UTclCustomControlEvent_ValueCommit>(Msg->Event))
	{
		FoundControl->ReceiveValueCommit(ValueCommit);
	}
}

void
UTentacle_CustomControls::SendMsg_RegisterControl(UTclCustomControlComponent* Control)
{
	auto* Tentacle = GetOwner<ATentacle>();
	tclSoftAssert(Tentacle && Control);

	const TSharedRef<FJsonObject> MsgObj = MakeShared<FJsonObject>();
	MsgObj->SetStringField(TEXT("type"), TEXT("CustomControls_RegisterControl"));

	const TSharedRef<FJsonObject> DataObj = MakeShared<FJsonObject>();
	DataObj->SetObjectField(TEXT("control"), Control->SerializeToJson());
	MsgObj->SetObjectField(TEXT("data"), DataObj);

	FString JsonString;
	const auto Writer = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&JsonString);
	FJsonSerializer::Serialize(MsgObj, Writer);

	Tentacle->SendMsg(JsonString);
}

void
UTentacle_CustomControls::SendMsg_UnregisterControl(UTclCustomControlComponent* Control)
{
	auto* Tentacle = GetOwner<ATentacle>();
	tclSoftAssert(Tentacle && Control);

	const TSharedRef<FJsonObject> MsgObj = MakeShared<FJsonObject>();
	MsgObj->SetStringField(TEXT("type"), TEXT("CustomControls_UnregisterControl"));

	const TSharedRef<FJsonObject> DataObj = MakeShared<FJsonObject>();
	DataObj->SetObjectField(TEXT("control"), Control->SerializeToJson());
	MsgObj->SetObjectField(TEXT("data"), DataObj);

	FString JsonString;
	const auto Writer = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&JsonString);
	FJsonSerializer::Serialize(MsgObj, Writer);

	Tentacle->SendMsg(JsonString);
}

void
UTentacle_CustomControls::SendMsg_ControlState(UTclCustomControlComponent* Control)
{
	auto* Tentacle = GetOwner<ATentacle>();
	tclSoftAssert(Tentacle && Control);

	const TSharedRef<FJsonObject> MsgObj = MakeShared<FJsonObject>();
	MsgObj->SetStringField(TEXT("type"), TEXT("CustomControls_ControlState"));

	const TSharedRef<FJsonObject> DataObj = MakeShared<FJsonObject>();
	DataObj->SetObjectField(TEXT("control"), Control->SerializeToJson());
	MsgObj->SetObjectField(TEXT("data"), DataObj);

	FString JsonString;
	const auto Writer = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&JsonString);
	FJsonSerializer::Serialize(MsgObj, Writer);

	Tentacle->SendMsg(JsonString);
}
