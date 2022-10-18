#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Particles/ParticleSystemComponent.h"

class SParticleSystemOptimizationPage : public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SParticleSystemOptimizationPage) { }
	SLATE_END_ARGS()

public:

	/** Default constructor. */
	SParticleSystemOptimizationPage();

	/** Destructor. */
	~SParticleSystemOptimizationPage();

	/**
	 * Constructs the widget.
	 *
	 * @param InArgs The Slate argument list.
	 */
	void Construct(const FArguments& InArgs);

	void ProcessOptimizationCheck();
protected:
	void ProcessOptimizationCheck(UParticleSystemComponent* ParticleComponent, FOutputDevice& Ar);
	void ProcessOptimizationCheck(UParticleSystem* ParticleSystem, FOutputDevice& Ar);
private:
	/** Property viewing widget */
	TSharedPtr<IDetailsView>   SettingsView;

	class UParticleSystemOptimizationRules* RuleSettings;
};

