// © 2025 Eldritch One. All rights reserved.

#include "Embedded/TclCoreLibrary.h"
#include "HAL/PlatformProcess.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "Misc/ScopeLock.h"

// File-scope names are distinct from every other file's: a unity build compiles
// several .cpp files as one, where a shared name would shadow or collide.
namespace
{
	FCriticalSection CoreLibraryGuard;
	bool bCoreLibraryLoadAttempted = false;
	FTclCoreLibrary LoadedCoreLibrary;
	FString CoreLibraryLoadError;
	void* CoreLibraryHandle = nullptr;
	int32 CoreLibraryOpenConnections = 0;

	/** Resolve one export into its typed pointer, noting it in Missing when the library lacks it. */
	template <typename TExport>
	void ResolveCoreExport(void* Handle, const TCHAR* Name, TExport& Out, TArray<FString>& Missing)
	{
		Out = reinterpret_cast<TExport>(FPlatformProcess::GetDllExport(Handle, Name));
		if (Out == nullptr)
			Missing.Add(Name);
	}
}

FString
FTclCoreLibrary::ExpectedPath()
{
#if PLATFORM_WINDOWS
	const TCHAR* Platform = TEXT("Win64");
	const TCHAR* FileName = TEXT("tentacle_ffi.dll");
#elif PLATFORM_MAC
	const TCHAR* Platform = TEXT("Mac");
	const TCHAR* FileName = TEXT("libtentacle_ffi.dylib");
#elif PLATFORM_LINUX
	const TCHAR* Platform = TEXT("Linux");
	const TCHAR* FileName = TEXT("libtentacle_ffi.so");
#else
	return FString();
#endif

	const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("Tentacle"));
	if (!Plugin.IsValid())
		return FString();

	return FPaths::ConvertRelativePathToFull(
		FPaths::Combine(Plugin->GetBaseDir(), TEXT("Binaries/ThirdParty/TentacleCore"), Platform, FileName));
}

const FTclCoreLibrary*
FTclCoreLibrary::Get(FString& OutError)
{
	FScopeLock Lock(&CoreLibraryGuard);

	if (bCoreLibraryLoadAttempted)
	{
		OutError = CoreLibraryLoadError;
		return CoreLibraryLoadError.IsEmpty() ? &LoadedCoreLibrary : nullptr;
	}
	bCoreLibraryLoadAttempted = true;

	const FString Path = ExpectedPath();
	if (Path.IsEmpty())
		CoreLibraryLoadError = TEXT("the Tentacle core has no build for this platform");
	else if (!FPaths::FileExists(Path))
		CoreLibraryLoadError = FString::Printf(TEXT("the Tentacle core is not installed (expected %s)"), *Path);

	void* Handle = CoreLibraryLoadError.IsEmpty() ? FPlatformProcess::GetDllHandle(*Path) : nullptr;
	if (CoreLibraryLoadError.IsEmpty() && Handle == nullptr)
		CoreLibraryLoadError = FString::Printf(TEXT("the Tentacle core at %s could not be loaded"), *Path);

	if (!CoreLibraryLoadError.IsEmpty())
	{
		OutError = CoreLibraryLoadError;
		return nullptr;
	}

	TArray<FString> Missing;
	ResolveCoreExport(Handle, TEXT("tcl_version"), LoadedCoreLibrary.Version, Missing);
	ResolveCoreExport(Handle, TEXT("tcl_core_create"), LoadedCoreLibrary.CoreCreate, Missing);
	ResolveCoreExport(Handle, TEXT("tcl_core_destroy"), LoadedCoreLibrary.CoreDestroy, Missing);
	ResolveCoreExport(Handle, TEXT("tcl_connection_open"), LoadedCoreLibrary.ConnectionOpen, Missing);
	ResolveCoreExport(Handle, TEXT("tcl_connection_send"), LoadedCoreLibrary.ConnectionSend, Missing);
	ResolveCoreExport(Handle, TEXT("tcl_connection_poll"), LoadedCoreLibrary.ConnectionPoll, Missing);
	ResolveCoreExport(Handle, TEXT("tcl_connection_wait"), LoadedCoreLibrary.ConnectionWait, Missing);
	ResolveCoreExport(Handle, TEXT("tcl_connection_is_closed"), LoadedCoreLibrary.ConnectionIsClosed, Missing);
	ResolveCoreExport(Handle, TEXT("tcl_connection_close"), LoadedCoreLibrary.ConnectionClose, Missing);

	if (Missing.Num() > 0)
	{
		// A library from another core version: nothing is called through it.
		FPlatformProcess::FreeDllHandle(Handle);
		LoadedCoreLibrary = FTclCoreLibrary();
		CoreLibraryLoadError = FString::Printf(TEXT("the Tentacle core at %s lacks %s; rebuild it with just unreal::native"),
			*Path, *FString::Join(Missing, TEXT(", ")));
		OutError = CoreLibraryLoadError;
		return nullptr;
	}

	// Kept until module shutdown: the core's own thread runs code from it until
	// the core is destroyed, and every connection handle calls into it.
	CoreLibraryHandle = Handle;
	OutError.Empty();
	return &LoadedCoreLibrary;
}

void
FTclCoreLibrary::NoteConnectionOpened()
{
	FScopeLock Lock(&CoreLibraryGuard);
	++CoreLibraryOpenConnections;
}

void
FTclCoreLibrary::NoteConnectionClosed()
{
	FScopeLock Lock(&CoreLibraryGuard);
	--CoreLibraryOpenConnections;
}

void
FTclCoreLibrary::UnloadIfUnused()
{
	FScopeLock Lock(&CoreLibraryGuard);
	if (CoreLibraryHandle == nullptr)
		return;
	if (CoreLibraryOpenConnections > 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Tentacle: keeping the Tentacle core's library loaded; %d connections to it are still open"), CoreLibraryOpenConnections);
		return;
	}

	FPlatformProcess::FreeDllHandle(CoreLibraryHandle);
	CoreLibraryHandle = nullptr;
	LoadedCoreLibrary = FTclCoreLibrary();
	bCoreLibraryLoadAttempted = false;
	CoreLibraryLoadError.Empty();
}
