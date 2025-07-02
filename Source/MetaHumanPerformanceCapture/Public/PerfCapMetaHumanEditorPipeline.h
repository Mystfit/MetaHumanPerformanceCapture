// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MetaHumanDefaultEditorPipelineLegacy.h"
#include "PerfCapMetaHumanEditorPipeline.generated.h"

/**
 * 
 */
UCLASS(EditInlineNew)
class METAHUMANPERFORMANCECAPTURE_API UPerfCapMetaHumanEditorPipeline : public UMetaHumanDefaultEditorPipelineLegacy
{
	GENERATED_BODY()

	virtual bool UpdateActorBlueprint(const UMetaHumanCharacterInstance* InCharacterInstance, UBlueprint* InBlueprint) const override;
	
};
