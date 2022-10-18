#include "ParticleSystemOptimizationRules.h"


UParticleSystemOptimizationRules::UParticleSystemOptimizationRules()
	: Super()
	, bSkipComponentIfTemplateIsNone(false)
	, MaxEmitterNumber(16)
	, MaxUpdateTimeFPS(60.0f)
	, MaxParticleCountToDrawForEmitter(500)
	, NeverCullParticleSystemSize(10000.f)
	, ParticleSystemCullScreenSize(0.02f)
	, bCheckHighQualityLights(true)
	, bCheckShadowCastingLights(true)
{

}

void UParticleSystemOptimizationRules::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	ValidateSettings();
}

bool UParticleSystemOptimizationRules::ValidateSettings()
{
	return true;
}
