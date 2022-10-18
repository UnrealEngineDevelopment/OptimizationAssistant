#pragma once
#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "ParticleSystemOptimizationRules.generated.h"

UCLASS(config = OptimizationAssistant, defaultconfig)
class UParticleSystemOptimizationRules : public UObject
{
	GENERATED_BODY()
public:
	UParticleSystemOptimizationRules();

	// 如果ParticleSystem组件的粒子模板(Template)为空，则跳过该组件的检查。
	UPROPERTY(EditAnywhere, config)
	bool bSkipComponentIfTemplateIsNone;

	UPROPERTY(EditAnywhere, config, meta = (UIMin = "1", UIMax = "16", ClampMin = "1", ClampMax = "16"))
	int32 MaxEmitterNumber;

	UPROPERTY(EditAnywhere, config, meta = (UIMin = "30", UIMax = "120", ClampMin = "30", ClampMax = "120"))
	float MaxUpdateTimeFPS;

	/**
	 *	The maximum number of particles to DRAW for this emitter.
	 *	If set to 0, it will use whatever number are present.
	 */
	UPROPERTY(EditAnywhere, config, meta = (UIMin = "1", UIMax = "500", ClampMin = "1", ClampMax = "500"))
	int32 MaxParticleCountToDrawForEmitter;

	// 如果粒子系统总体Size(Component_Bounds_SphereRadius * 2) 大于该值，那么这个粒子系统不会受距离裁剪
	UPROPERTY(EditAnywhere, config)
	float NeverCullParticleSystemSize;

	// 当粒子系统的屏幕占比到达ParticleSystemCullScreenSize的值时，通过ComponentToCamera=Component_Bounds_SphereRadius / ParticleSystemCullScreenSize / Sin(FOV of Camera)
	// 计算出Particle组件到摄像机的距离，这个距离就是Particle 组件的标准裁剪距离，如果设置的裁剪距离大于标准裁剪距离会被认为是不合理的。
	UPROPERTY(EditAnywhere, config, meta = (UIMin = "0.005", UIMax = "0.1", ClampMin = "0.005", ClampMax = "0.1"))
	float ParticleSystemCullScreenSize;

	// 是否检查粒子系统具有高质量光照，通常来说绝大多数粒子系统并不需要高质量光照。
	UPROPERTY(EditAnywhere, config)
	bool bCheckHighQualityLights;

	// 是否检查粒子系统投影，通常来说绝大多数粒子系统并不需要投影。
	UPROPERTY(EditAnywhere, config)
	bool bCheckShadowCastingLights;

	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)override;
	virtual bool ValidateSettings();
};