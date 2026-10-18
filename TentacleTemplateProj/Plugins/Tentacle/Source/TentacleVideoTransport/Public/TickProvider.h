// © 2025 Eldritch One. All rights reserved.

#pragma once
#include "CoreMinimal.h"
#include "HAL/Runnable.h"
#include "HAL/RunnableThread.h"

DECLARE_DELEGATE(FTickProviderTick);

class FTickProvider : public FRunnable
{
	FRunnableThread*
	Thread;

	FCriticalSection
	Mutex;

	bool
	bIsRunning = false;

	double
	NextFrameTime = 0;

	double
	FPS = 60.0;

public:
	FTickProvider(double FPS)
	{
		SetFPS(FPS);
		Thread = FRunnableThread::Create(this, TEXT("TickProvider"));
	}

	virtual ~FTickProvider() override
	{
		bIsRunning = false;
		if (Thread)
		{
			Thread->Kill();
			delete Thread;
			Thread = nullptr;
		}
	}

	virtual uint32
	Run() override
	{
		bIsRunning = true;
		while (bIsRunning)
		{
			const auto CurrentTime = GetCurrentTime();
			if (CurrentTime < NextFrameTime)
				continue;

			NextFrameTime = GetNextUpdateTime(FPS, CurrentTime);
			if (Tick.IsBound())
				Tick.Execute();
		}
		return 0;
	}

	virtual void
	Stop() override
	{
		bIsRunning = false;
	}

	void
	SetFPS(double value)
	{
		FScopeLock Lock(&Mutex);
		FPS = FMath::Clamp(value, 1, 120);
	}

	FTickProviderTick Tick;

private:
	static double
	GetCurrentTime()
	{
		const auto Cycle = FPlatformTime::Cycles64();
		const auto Delta = FPlatformTime::GetSecondsPerCycle64();
		return static_cast<double>(Cycle) * Delta;
	}

	static double
	GetNextUpdateTime(double FPS, double CurrentTime)
	{
		const auto FrameTime = 1.0 / FPS;
		return CurrentTime + FrameTime;
	}
};
