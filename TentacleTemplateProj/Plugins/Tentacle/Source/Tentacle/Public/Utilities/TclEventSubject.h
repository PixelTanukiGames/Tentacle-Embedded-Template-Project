// © 2025 Eldritch One. All rights reserved.

#pragma once
#include "CoreMinimal.h"

// Replaces the TypeScript EventSubject<T> pattern.
/**
 * Internal C++ multicast event (not Blueprint-visible).
 *
 * Usage:
 *   TclEventSubject<bool> OnAuthChanged;
 *   auto Handle = OnAuthChanged.Subscribe([](bool bVal) { ... });
 *   OnAuthChanged.Next(true);
 *   OnAuthChanged.Unsubscribe(Handle);
 */
template <typename TEventData>
class TTclEventSubject
{
public:
	using FCallback = TFunction<void(const TEventData&)>;

	struct FHandle
	{
		uint64 Id = 0;
		bool IsValid() const { return Id != 0; }
	};

	FHandle
	Subscribe(FCallback Callback)
	{
		const FHandle Handle{++NextId};
		Subscribers.Add(Handle.Id, MoveTemp(Callback));
		return Handle;
	}

	void
	Unsubscribe(FHandle Handle)
	{
		Subscribers.Remove(Handle.Id);
	}

	void
	Next(const TEventData& Data)
	{
		// Copy keys to allow unsubscribe during iteration
		TArray<uint64> Keys;
		Subscribers.GetKeys(Keys);

		for (const uint64 Key : Keys)
		{
			if (const FCallback* Cb = Subscribers.Find(Key))
				(*Cb)(Data);
		}
	}

	int32
	Num() const { return Subscribers.Num(); }

private:
	TMap<uint64, FCallback> Subscribers;
	uint64 NextId = 0;
};

/**
 * Void specialization for events with no data payload.
 */
template <>
class TTclEventSubject<void>
{
public:
	using FCallback = TFunction<void()>;

	struct FHandle
	{
		uint64 Id = 0;
		bool IsValid() const { return Id != 0; }
	};

	FHandle
	Subscribe(FCallback Callback)
	{
		const FHandle Handle{++NextId};
		Subscribers.Add(Handle.Id, MoveTemp(Callback));
		return Handle;
	}

	void
	Unsubscribe(FHandle Handle)
	{
		Subscribers.Remove(Handle.Id);
	}

	void
	Next()
	{
		TArray<uint64> Keys;
		Subscribers.GetKeys(Keys);

		for (const uint64 Key : Keys)
		{
			if (const FCallback* Cb = Subscribers.Find(Key))
				(*Cb)();
		}
	}

	int32
	Num() const { return Subscribers.Num(); }

private:
	TMap<uint64, FCallback> Subscribers;
	uint64 NextId = 0;
};
