// © 2025 Eldritch One. All rights reserved.

#pragma once
#include "CoreMinimal.h"

// Replaces the TypeScript EventPromiseSubject<T> pattern.
/**
 * Fire-once event with late-subscriber replay.
 *
 * - Resolve(Data) fires all pending subscribers, then stores the value.
 * - Subscribe() after Resolve() immediately invokes the callback with the stored value.
 * - Can only be resolved once.
 */
template <typename TEventData>
class TTclEventPromiseSubject
{
public:
	using FCallback = TFunction<void(const TEventData&)>;

	struct FHandle
	{
		uint64 Id = 0;
		bool IsValid() const { return Id != 0; }
	};

	bool
	HasResolved() const { return bResolved; }

	void
	Resolve(const TEventData& Data)
	{
		checkf(!bResolved, TEXT("TTclEventPromiseSubject::Resolve called more than once"));
		bResolved = true;
		ResolvedValue = Data;

		for (auto& [Id, Cb] : PendingSubscribers)
			Cb(ResolvedValue);

		PendingSubscribers.Empty();
	}

	FHandle
	Subscribe(FCallback Callback)
	{
		if (bResolved)
		{
			Callback(ResolvedValue);
			return FHandle{0};
		}

		const FHandle Handle{++NextId};
		PendingSubscribers.Add(Handle.Id, MoveTemp(Callback));
		return Handle;
	}

	void
	Unsubscribe(FHandle Handle)
	{
		PendingSubscribers.Remove(Handle.Id);
	}

private:
	TMap<uint64, FCallback> PendingSubscribers;
	TEventData ResolvedValue{};
	uint64 NextId = 0;
	bool bResolved = false;
};

/**
 * Void specialization for promise events with no data payload.
 */
template <>
class TTclEventPromiseSubject<void>
{
public:
	using FCallback = TFunction<void()>;

	struct FHandle
	{
		uint64 Id = 0;
		bool IsValid() const { return Id != 0; }
	};

	bool
	HasResolved() const { return bResolved; }

	void
	Resolve()
	{
		checkf(!bResolved, TEXT("TTclEventPromiseSubject::Resolve called more than once"));
		bResolved = true;

		for (auto& [Id, Cb] : PendingSubscribers)
			Cb();

		PendingSubscribers.Empty();
	}

	FHandle
	Subscribe(FCallback Callback)
	{
		if (bResolved)
		{
			Callback();
			return FHandle{0};
		}

		const FHandle Handle{++NextId};
		PendingSubscribers.Add(Handle.Id, MoveTemp(Callback));
		return Handle;
	}

	void
	Unsubscribe(FHandle Handle)
	{
		PendingSubscribers.Remove(Handle.Id);
	}

private:
	TMap<uint64, FCallback> PendingSubscribers;
	uint64 NextId = 0;
	bool bResolved = false;
};
