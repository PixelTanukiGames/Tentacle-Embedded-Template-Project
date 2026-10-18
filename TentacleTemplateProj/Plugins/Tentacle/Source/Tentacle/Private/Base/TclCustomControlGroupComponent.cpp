// © 2025 Eldritch One. All rights reserved.

#include "Base/TclCustomControlGroupComponent.h"
#include "Base/TclCustomControlComponent.h"

void
UTclCustomControlGroupComponent::BeginPlay()
{
	Super::BeginPlay();
	UpdateDescendantControls();
}

void
UTclCustomControlGroupComponent::SetGroupName(const FString& NewGroupName)
{
	GroupName = NewGroupName;
	UpdateDescendantControls();
}

void
UTclCustomControlGroupComponent::UpdateDescendantControls()
{
	TArray<USceneComponent*> Children;
	GetChildrenComponents(true, Children);

	for (auto* Child : Children)
	{
		if (auto* Control = Cast<UTclCustomControlComponent>(Child))
		{
			Control->SetGroup(GroupName);
		}
	}
}
