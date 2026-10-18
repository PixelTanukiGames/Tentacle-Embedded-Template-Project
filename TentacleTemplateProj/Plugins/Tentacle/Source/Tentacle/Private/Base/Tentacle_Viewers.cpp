// © 2025 Eldritch One. All rights reserved.

#include "Base/Tentacle_Viewers.h"
#include "Base/Tentacle.h"
#include "Base/TclViewer.h"
#include "Sdk/TclViewerData.h"
#include "Dom/JsonObject.h"

UTclViewer*
UTentacle_Viewers::UpsertViewer(const TSharedPtr<FJsonObject>& ViewerJson)
{
	if (!ViewerJson.IsValid())
		return nullptr;

	// Parse, then merge through the parsed-data overload: UpdateData (invoked
	// there) merges the properties, so both entry points populate the viewer's
	// property map the same way.
	UObject* Outer = this;
	if (AActor* Owner = GetOwner())
		Outer = Owner;

	return UpsertViewer(UTclViewerData::Parse(Outer, ViewerJson));
}

UTclViewer*
UTentacle_Viewers::UpsertViewer(UTclViewerData* ViewerData)
{
	if (!ViewerData)
		return nullptr;

	const FString& ViewerId = ViewerData->Id;
	if (ViewerId.IsEmpty())
		return nullptr;

	ATentacle* Tentacle = GetOwner<ATentacle>();

	const TObjectPtr<UTclViewer>* ExistingViewerPtr = ViewersById.Find(ViewerId);
	if (ExistingViewerPtr && IsValid(*ExistingViewerPtr))
	{
		UTclViewer* Viewer = *ExistingViewerPtr;
		Viewer->Tentacle = Tentacle;
		Viewer->UpdateData(ViewerData);
		return Viewer;
	}

	// Viewers are outered to the actor, as they were when the cache lived there.
	UObject* Outer = this;
	if (Tentacle)
		Outer = Tentacle;

	UTclViewer* Viewer = NewObject<UTclViewer>(Outer);
	Viewer->Tentacle = Tentacle;
	Viewer->UpdateData(ViewerData);
	ViewersById.Add(ViewerId, Viewer);

	// The actor's delegate first, exactly as it fired before the cache moved
	// here (the viewer-property hook binds to it), then this component's own.
	if (Tentacle)
		Tentacle->OnViewerAdded.Broadcast(Viewer);

	OnViewerAdded.Broadcast(Viewer);

	return Viewer;
}

UTclViewer*
UTentacle_Viewers::FindViewerById(const FString& ViewerId, bool& Found) const
{
	const TObjectPtr<UTclViewer>* ResultPtr = ViewersById.Find(ViewerId);
	Found = ResultPtr != nullptr && IsValid(*ResultPtr);
	return Found ? ResultPtr->Get() : nullptr;
}

void
UTentacle_Viewers::DestroyViewer(const FString& ViewerId)
{
	ViewersById.Remove(ViewerId);
}

void
UTentacle_Viewers::DestroyAllViewers()
{
	ViewersById.Empty();
}

TArray<UTclViewer*>
UTentacle_Viewers::GetAllViewers() const
{
	TArray<UTclViewer*> Result;
	Result.Reserve(ViewersById.Num());
	for (const auto& Pair : ViewersById)
	{
		if (IsValid(Pair.Value))
			Result.Add(Pair.Value);
	}
	return Result;
}

int32
UTentacle_Viewers::GetViewerCount() const
{
	int32 Count = 0;
	for (const auto& Pair : ViewersById)
	{
		if (IsValid(Pair.Value))
			++Count;
	}
	return Count;
}
