// © 2025 Eldritch One. All rights reserved.

#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Sdk/TclStreamPlatform.h"
#include "TclConnectionsWidget.generated.h"

class UTentacle_Connections;
class UTclConnectionsWidgetRowView;
class UTclPlatformConnection;
class UTclPlatformConnectionPrompt;
class UTclPlatformConnectionResult;
class UTextBlock;
class UVerticalBox;

// Unity's ConnectionsWidget and Godot's TclConnectionsWidget. Its tree is built
// in C++ rather than in a Widget Blueprint asset, so the plugin ships nothing a
// project would have to keep in step with an engine version.
/**
 * A ready-made sign-in screen for Embedded mode: one row per platform, with its
 * status and a button to connect, disconnect, or sign in again.
 *
 * Create it with Create Widget and add it to the viewport, or place it inside a
 * Widget Blueprint of your own, and the player can connect their Twitch or Kick
 * channel with no graph of yours. While signing in, it opens the sign-in page
 * in the player's browser, shows Twitch's code and how long it has left, and
 * offers to open the page again or cancel; afterwards it says why a sign-in
 * that did not connect failed. With the Tentacle actor in Paired mode it shows one
 * line saying the Tentacle app connects the platforms instead. For a sign-in
 * screen of your own, use Tentacle Sign In Buttons, or the Sign In node.
 */
UCLASS(meta=(DisplayName="Tentacle Connections Widget"))
class TENTACLE_API UTclConnectionsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UTclConnectionsWidget(const FObjectInitializer& ObjectInitializer);

	/** The platforms to offer, in the order they are listed. Read when the widget is built. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tentacle")
	TArray<ETclStreamPlatform> Platforms;

	/**
	 * Open the sign-in page in the player's browser as soon as signing in needs
	 * them. Off, the row offers a button for it instead.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tentacle")
	bool bOpenBrowserAutomatically = true;

	/** The size of the platform names and statuses; the detail line is a little smaller. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tentacle|Look")
	int32 FontSize = 18;

	/** The colour of the platform names and statuses. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tentacle|Look")
	FSlateColor TextColor = FSlateColor(FLinearColor::White);

	/** The colour of the detail line under a status. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tentacle|Look")
	FSlateColor MutedTextColor = FSlateColor(FLinearColor(1.f, 1.f, 1.f, 0.7f));

	/** The colour the buttons are tinted. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tentacle|Look")
	FLinearColor ButtonColor = FLinearColor(0.47f, 0.31f, 0.86f);

	/** The colour of the buttons' labels. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tentacle|Look")
	FSlateColor ButtonTextColor = FSlateColor(FLinearColor::White);

	/** How the widget opens a page. Opens the player's browser unless replaced. */
	TFunction<void(const FString& Url)> OpenUrl;

	/** The clock a prompt's time left is measured on, in seconds. */
	TFunction<double()> Clock;

	/** Called by a row's button. */
	void Press(UTclConnectionsWidgetRowView* Row, bool bPrimary);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<UTclConnectionsWidgetRowView>> Rows;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> Note;

	UPROPERTY(Transient)
	TObjectPtr<UTentacle_Connections> Connections;

	void BuildTree();
	UTextBlock* MakeText(const FString& Name, const FSlateColor& Color, int32 Size, bool bBold = false);
	void ShowNote(const FString& Text);
	void Refresh(UTclConnectionsWidgetRowView* Row);
	UTclConnectionsWidgetRowView* RowFor(ETclStreamPlatform Platform) const;

	UFUNCTION()
	void HandleConnectionChanged(const UTclPlatformConnection* Connection);

	UFUNCTION()
	void HandleConnectPrompt(const UTclPlatformConnectionPrompt* Prompt);

	UFUNCTION()
	void HandleConnectResult(const UTclPlatformConnectionResult* Result);
};
