// © 2025 Eldritch One. All rights reserved.

#pragma once
#include "CoreMinimal.h"

/**
 * Ring-buffer deduplicator for message/event IDs.
 *
 * Uses a TSet for O(1) lookup and a TArray FIFO queue to evict old entries.
 */
class FTclDeduplicator
{
public:
	explicit
	FTclDeduplicator(int32 InMaxSize = 100)
		: MaxSize(InMaxSize)
	{
		Queue.Reserve(InMaxSize);
	}

	/**
	 * Try to add an ID. Returns true if new (not a duplicate), false if already seen.
	 */
	bool
	TryAdd(const FString& Id)
	{
		if (Seen.Contains(Id))
			return false;

		if (Queue.Num() >= MaxSize)
		{
			const FString& Oldest = Queue[0];
			Seen.Remove(Oldest);
			Queue.RemoveAt(0);
		}

		Seen.Add(Id);
		Queue.Add(Id);
		return true;
	}

	void
	Reset()
	{
		Seen.Empty();
		Queue.Empty();
	}

	int32
	Num() const { return Seen.Num(); }

private:
	TSet<FString> Seen;
	TArray<FString> Queue;
	int32 MaxSize;
};
