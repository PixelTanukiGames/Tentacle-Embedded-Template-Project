// © 2025 Eldritch One. All rights reserved.

#include "Base/TclCustomControlComponent.h"
#include "Base/Tentacle.h"
#include "Base/Tentacle_CustomControls.h"
#include "Sdk/TclCustomControlEvent_ValueChange.h"
#include "Sdk/TclCustomControlEvent_ValueCommit.h"
#include "Utilities/TclCommon.h"
#include "Dom/JsonObject.h"
#include "Misc/Guid.h"

void
UTclCustomControlComponent::BeginPlay()
{
	Super::BeginPlay();

	ControlId = FGuid::NewGuid().ToString();

	auto* Tentacle = UTclCommon::GetActorOfClass<ATentacle>(this);
	if (Tentacle && Tentacle->CustomControls)
	{
		Tentacle->CustomControls->RegisterCustomControl(this);
		BoundTentacle = Tentacle;
	}

	// On Initialize promises a registered control with an id, so it fires once
	// registration is done. It fires with no Tentacle actor in the level too, as
	// it always has: plenty of graphs set a control up here either way.
	OnInitialize.Broadcast();

	// A handler may have set a value, label or options directly, which pushes
	// nothing. Before On Initialize moved after registration that rode along in
	// the registration message; now the state goes once more so it still
	// reaches Tentacle. Skipped if a handler unregistered the control.
	if (IsValid(Tentacle) && Tentacle->CustomControls && Tentacle->CustomControls->CustomControls.Contains(this))
		Tentacle->CustomControls->SendMsg_ControlState(this);
}

void
UTclCustomControlComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// The actor registered with at BeginPlay rather than whichever one a lookup
	// finds now, which may already be gone or be another (and the one registered
	// with would keep this control in its list); a lookup only when none was.
	ATentacle* Tentacle = BoundTentacle.IsExplicitlyNull() ? UTclCommon::GetActorOfClass<ATentacle>(this) : BoundTentacle.Get();
	BoundTentacle.Reset();
	if (Tentacle && Tentacle->CustomControls)
		Tentacle->CustomControls->UnregisterCustomControl(this);

	Super::EndPlay(EndPlayReason);
}

void
UTclCustomControlComponent::SetLabel(const FString& NewLabel)
{
	Label = NewLabel;
	SendControlState();
}

void
UTclCustomControlComponent::SetSortIndex(int32 NewSortIndex)
{
	SortIndex = NewSortIndex;
	SendControlState();
}

void
UTclCustomControlComponent::SetGroup(const FString& NewGroup)
{
	Group = NewGroup;
	SendControlState();
}

TSharedRef<FJsonObject>
UTclCustomControlComponent::SerializeToJson() const
{
	const TSharedRef<FJsonObject> Obj = MakeShared<FJsonObject>();
	Obj->SetStringField(TEXT("id"), ControlId);
	Obj->SetStringField(TEXT("label"), Label);
	Obj->SetNumberField(TEXT("sortIndex"), SortIndex);
	Obj->SetStringField(TEXT("type"), GetControlType());
	if (!Group.IsEmpty())
		Obj->SetStringField(TEXT("group"), Group);
	return Obj;
}

void
UTclCustomControlComponent::ReceiveClick()
{
	OnClick.Broadcast();
}

void
UTclCustomControlComponent::ReceiveMouseDown()
{
	OnMouseDown.Broadcast();
}

void
UTclCustomControlComponent::ReceiveMouseUp()
{
	OnMouseUp.Broadcast();
}

void
UTclCustomControlComponent::ReceiveValueChange(UTclCustomControlEvent_ValueChange* Event)
{
	tclWarnF("Control '%s' does not support ValueChange events", *Label);
}

void
UTclCustomControlComponent::ReceiveValueCommit(UTclCustomControlEvent_ValueCommit* Event)
{
	tclWarnF("Control '%s' does not support ValueCommit events", *Label);
}

void
UTclCustomControlComponent::SendControlState()
{
	auto* Tentacle = UTclCommon::GetActorOfClass<ATentacle>(this);
	if (Tentacle && Tentacle->CustomControls)
		Tentacle->CustomControls->SendMsg_ControlState(this);
}
