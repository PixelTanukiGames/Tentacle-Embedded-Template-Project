// © 2025 Eldritch One. All rights reserved.

#include "Base/TclTimerSettings.h"

TSharedRef<FJsonObject>
FTclTimerSettings::ToInput() const
{
	const TSharedRef<FJsonObject> Input = MakeShared<FJsonObject>();
	if (!Name.IsEmpty())
		Input->SetStringField(TEXT("name"), Name);
	Input->SetBoolField(TEXT("repeating"), bRepeating);
	Input->SetBoolField(TEXT("onlyWhileLive"), bOnlyWhileLive);
	Input->SetNumberField(TEXT("intervalMinSeconds"), IntervalMinSeconds);
	Input->SetNumberField(TEXT("intervalMaxSeconds"), IntervalMaxSeconds);
	Input->SetNumberField(TEXT("minChatMessages"), MinChatMessages);
	return Input;
}
