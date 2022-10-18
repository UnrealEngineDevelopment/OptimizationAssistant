#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SGlobalSettingsPage : public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SGlobalSettingsPage) { }
	SLATE_END_ARGS()

public:

	/** Default constructor. */
	SGlobalSettingsPage();

	/** Destructor. */
	~SGlobalSettingsPage();

	/**
	 * Constructs the widget.
	 *
	 * @param InArgs The Slate argument list.
	 */
	void Construct(const FArguments& InArgs);
private:
	/** Property viewing widget */
	TSharedPtr<IDetailsView>   SettingsView;
};

