// Fill out your copyright notice in the Description page of Project Settings.


#include "PerfCapMetaHumanEditorPipeline.h"

#include "Item/MetaHumanGroomPipeline.h"
#include "Item/MetaHumanSkeletalMeshPipeline.h"
#include "Item/MetaHumanOutfitPipeline.h"
#include "MetaHumanCharacterInstance.h"
#include "MetaHumanCharacterPaletteEditorModule.h"
#include "MetaHumanDefaultPipelineLegacy.h"
#include "ProjectUtilities/MetaHumanProjectUtilities.h"
#include "Subsystem/MetaHumanCharacterBuild.h"

#include "BlueprintCompilationManager.h"
#include "Components/SkeletalMeshComponent.h"
#include "GroomComponent.h"
#include "SubobjectDataSubsystem.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "GameFramework/Actor.h"
#include "ChaosClothAsset/ClothComponent.h"
#include "Logging/MessageLog.h"
#include "Misc/UObjectToken.h"
#include "Kismet2/ComponentEditorUtils.h"
#include "Kismet2/BlueprintEditorUtils.h"

#define LOCTEXT_NAMESPACE "PerfCapMetaHumanEditorPipeline"

namespace UE::MetaHuman::Private
{
	bool SetSkeletalMesh(UActorComponent* InComponent, TObjectPtr<USkeletalMesh> InSkelMesh)
	{
		if (USkeletalMeshComponent* SkelMeshComponent = Cast<USkeletalMeshComponent>(InComponent))
		{
			FProperty* SkelMeshProperty = USkeletalMeshComponent::StaticClass()->FindPropertyByName(SkelMeshComponent->GetSkeletalMeshAssetPropertyNameChecked());
			check(SkelMeshProperty);
			FPropertyChangedEvent SkelMeshChangedEvent{ SkelMeshProperty, EPropertyChangeType::ValueSet };

			SkelMeshComponent->SetSkeletalMeshAsset(InSkelMesh);

			// Empty the override materials since PostEditChangeProperty will recreate the slots in the component
			SkelMeshComponent->OverrideMaterials.Empty();
			SkelMeshComponent->PostEditChangeProperty(SkelMeshChangedEvent);


			// Update any instances of this component with the new mesh 
			TArray<UObject*> Instances;
			SkelMeshComponent->GetArchetypeInstances(Instances);

			for (UObject* Instance : Instances)
			{
				if (USkeletalMeshComponent* SkelMeshCompInstance = Cast<USkeletalMeshComponent>(Instance))
				{
					SkelMeshCompInstance->SetSkeletalMeshAsset(InSkelMesh);

					// Empty the override materials since PostEditChangeProperty will recreate the slots in the component
					SkelMeshCompInstance->OverrideMaterials.Empty();
					SkelMeshCompInstance->PostEditChangeProperty(SkelMeshChangedEvent);
				}
			}

			return true;
		}

		return false;
	}

	void AssignGroom(
		UActorComponent* InComponent,
		FMetaHumanGroomPipelineAssemblyOutput FMetaHumanDefaultAssemblyOutput::* InAssemblyStructMember,
		const FMetaHumanDefaultAssemblyOutput* InAssemblyOutput)
	{
		if (UGroomComponent* GroomComponent = Cast<UGroomComponent>(InComponent))
		{
			// Store the values that are going to be changed in the groom component so they can be propagated to any blueprint instances that are placed in a level
			UGroomAsset* OldGroomAsset = GroomComponent->GroomAsset;
			UGroomBindingAsset* OldGroomBindingAsset = GroomComponent->BindingAsset;
			TArray<TObjectPtr<UMaterialInterface>> OldOverrideMaterials = GroomComponent->OverrideMaterials;

			UMetaHumanGroomPipeline::ApplyGroomAssemblyOutputToGroomComponent(InAssemblyOutput->*InAssemblyStructMember, GroomComponent);

			const FProperty* GroomAssetProperty = UGroomComponent::StaticClass()->FindPropertyByName(GET_MEMBER_NAME_CHECKED(UGroomComponent, GroomAsset));
			const FProperty* GroomBindingAssetProperty = UGroomComponent::StaticClass()->FindPropertyByName(GET_MEMBER_NAME_CHECKED(UGroomComponent, BindingAsset));
			const FProperty* OverrideMaterialsProperty = UGroomComponent::StaticClass()->FindPropertyByName(GET_MEMBER_NAME_CHECKED(UGroomComponent, OverrideMaterials));

			// Finally, propagate the default value changes to the groom component instances
			TSet<USceneComponent*> UpdatedInstances;
			FComponentEditorUtils::PropagateDefaultValueChange(GroomComponent, GroomAssetProperty, OldGroomAsset, GroomComponent->GroomAsset.Get(), UpdatedInstances);

			UpdatedInstances.Empty();
			FComponentEditorUtils::PropagateDefaultValueChange(GroomComponent, GroomBindingAssetProperty, OldGroomBindingAsset, GroomComponent->BindingAsset.Get(), UpdatedInstances);

			UpdatedInstances.Empty();
			FComponentEditorUtils::PropagateDefaultValueChange(GroomComponent, OverrideMaterialsProperty, OldOverrideMaterials, GroomComponent->OverrideMaterials, UpdatedInstances);
		}
	}

	void AssignMaterialsToInstances(UMeshComponent* InMeshComponent)
	{
		TArray<UObject*> Instances;
		InMeshComponent->GetArchetypeInstances(Instances);

		for (UObject* Instance : Instances)
		{
			if (UMeshComponent* ComponentInstance = Cast<USkeletalMeshComponent>(Instance))
			{
				for (int32 MaterialIndex = 0; MaterialIndex < ComponentInstance->GetNumMaterials(); ++MaterialIndex)
				{
					ComponentInstance->SetMaterial(MaterialIndex, InMeshComponent->GetMaterial(MaterialIndex));
				}
			}
		}
	}

	void AssignClothOutfit(UChaosClothComponent* InClothComponent, const FMetaHumanOutfitPipelineAssemblyOutput& AssemblyData)
	{
		if (InClothComponent)
		{
			UMetaHumanOutfitPipeline::ApplyOutfitAssemblyOutputToClothComponent(AssemblyData, InClothComponent);

			// Cloth components are recreated so need to force the assignment for instances to be updated
			AssignMaterialsToInstances(InClothComponent);
		}
	}

	void AssignSkelMeshOutfit(USkeletalMeshComponent* InSkelMeshComponent, const FMetaHumanOutfitPipelineAssemblyOutput& AssemblyData)
	{
		if (InSkelMeshComponent)
		{
			const bool bUpdateskelMesh = true;
			UMetaHumanOutfitPipeline::ApplyOutfitAssemblyOutputToMeshComponent(AssemblyData, InSkelMeshComponent, bUpdateskelMesh);

			// Cloth components are recreated so need to force the assignment for instances to be updated
			AssignMaterialsToInstances(InSkelMeshComponent);
		}
	}

	void AssignSkelMeshClothing(USkeletalMeshComponent* InSkelMeshComponent, const FMetaHumanSkeletalMeshPipelineAssemblyOutput& AssemblyData)
	{
		if (InSkelMeshComponent)
		{
			// The leader pose component is set by the Blueprint's construction script so there is no need to set it here
			USkeletalMeshComponent* LeaderPoseComponent = nullptr;
			UMetaHumanSkeletalMeshPipeline::ApplySkeletalMeshAssemblyOutputToSkeletalMeshComponent(AssemblyData, InSkelMeshComponent, LeaderPoseComponent);

			// Cloth components are recreated so need to force the assignment for instances to be updated
			AssignMaterialsToInstances(InSkelMeshComponent);
		}
	}

	template<typename TComponent, typename TAssemblyData>
	TArray<FSubobjectDataHandle> AssignComponents(
		UBlueprint* InBlueprint,
		const TArray<TAssemblyData>& InAssemblyData,
		const FSubobjectDataHandle& InParentComponentHandle,
		TFunction<void(const TAssemblyData&, TObjectPtr<TComponent>)> InUpdateFun)
	{
		TArray<FSubobjectDataHandle> Result;
		USubobjectDataSubsystem* SubobjectDataSubsystem = USubobjectDataSubsystem::Get();

		for (const TAssemblyData& CurrentAssemblyData : InAssemblyData)
		{
			FAddNewSubobjectParams Params;
			Params.ParentHandle = InParentComponentHandle;
			Params.NewClass = TComponent::StaticClass();
			Params.bConformTransformToParent = true;
			Params.BlueprintContext = InBlueprint;
			Params.bSkipMarkBlueprintModified = true;

			FText OutFailText;
			FSubobjectDataHandle NewComponentHandle = SubobjectDataSubsystem->AddNewSubobject(Params, OutFailText);

			bool bAddOk = false;

			if (NewComponentHandle.IsValid())
			{
				// Update components with assembly data
				if (TComponent* Component = const_cast<TComponent*>(NewComponentHandle.GetData()->GetObjectForBlueprint<TComponent>(InBlueprint)))
				{
					bAddOk = true;
					InUpdateFun(CurrentAssemblyData, Component);
					Result.Add(NewComponentHandle);
				}
			}

			if (!bAddOk)
			{
				FFormatNamedArguments FormatArguments;
				FormatArguments.Add(TEXT("ComponentType"), FText::FromString(TComponent::StaticClass()->GetName()));
				FormatArguments.Add(TEXT("ErrorMessage"), OutFailText);

				FText Message = FText::Format(
					LOCTEXT("AddBPComponentFail", "Unable to add {ComponentType} component, error: {ErrorMessage}"),
					FormatArguments);

				FMessageLog(UE::MetaHuman::MessageLogName).Error(Message)
					->AddToken(FUObjectToken::Create(InBlueprint));
			}
		}

		return Result;
	}
}


bool UPerfCapMetaHumanEditorPipeline::UpdateActorBlueprint(const UMetaHumanCharacterInstance* InCharacterInstance, UBlueprint* InBlueprint) const
{
	using namespace UE::MetaHuman::Private;

	if (!InCharacterInstance || !InCharacterInstance->GetAssemblyOutput().IsValid())
	{
		return false;
	}

	const FMetaHumanDefaultAssemblyOutput* AssemblyStruct = InCharacterInstance->GetAssemblyOutput().GetPtr<FMetaHumanDefaultAssemblyOutput>();

	if (!AssemblyStruct)
	{
		return false;
	}

	AActor* ActorCDO = InBlueprint->GeneratedClass->GetDefaultObject<AActor>();

	if (!ActorCDO)
	{
		return false;
	}

	USubobjectDataSubsystem* SubobjectDataSubsystem = USubobjectDataSubsystem::Get();

	TArray<FSubobjectDataHandle> SubobjectDataHandles;
	SubobjectDataSubsystem->GatherSubobjectData(ActorCDO, SubobjectDataHandles);

	// Root subobject handle is always the first one
	FSubobjectDataHandle RootHandle = SubobjectDataHandles[0];

	if (SubobjectDataHandles.IsEmpty())
	{
		return false;
	}

	// Get rid of the duplicate subobject handles (for some reason they're not filtered by default)
	SubobjectDataHandles = TSet(SubobjectDataHandles).Array();

	// Explicitly set skeletal mesh root component mesh
	FSubobjectDataHandle BodyHandle = FSubobjectDataHandle::InvalidHandle;

	// Components to be removed
	TArray<FSubobjectDataHandle> OldComponentHandles;

	for (const FSubobjectDataHandle& Handle : SubobjectDataHandles)
	{
		if (UActorComponent* ActorComponent = const_cast<UActorComponent*>(Handle.GetData()->GetObjectForBlueprint<UActorComponent>(InBlueprint)))
		{
			UClass* ComponentClass = ActorComponent->GetClass();
			FString ComponentName = ActorComponent->GetName();
			ComponentName.RemoveFromEnd(UActorComponent::ComponentTemplateNameSuffix);

			if (ComponentName == TEXT("Face"))
			{
				SetSkeletalMesh(ActorComponent, AssemblyStruct->FaceMesh);
			}
			//ACapturePerformer inherits from Skeletal Mesh Actor where the SkelMeshComponent is the root
			else if (ComponentName == TEXT("SkeletalMeshComponent0")) 
			{
				BodyHandle = Handle;
				SetSkeletalMesh(ActorComponent, AssemblyStruct->BodyMesh);
			}
			else if (ComponentName == TEXT("Hair"))
			{
				AssignGroom(ActorComponent, &FMetaHumanDefaultAssemblyOutput::Hair, AssemblyStruct);
			}
			else if (ComponentName == TEXT("Eyebrows"))
			{
				AssignGroom(ActorComponent, &FMetaHumanDefaultAssemblyOutput::Eyebrows, AssemblyStruct);
			}
			else if (ComponentName == TEXT("Eyelashes"))
			{
				AssignGroom(ActorComponent, &FMetaHumanDefaultAssemblyOutput::Eyelashes, AssemblyStruct);
			}
			else if (ComponentName == TEXT("Mustache"))
			{
				AssignGroom(ActorComponent, &FMetaHumanDefaultAssemblyOutput::Mustache, AssemblyStruct);
			}
			else if (ComponentName == TEXT("Beard"))
			{
				AssignGroom(ActorComponent, &FMetaHumanDefaultAssemblyOutput::Beard, AssemblyStruct);
			}
			else if (ComponentName == TEXT("Fuzz"))
			{
				AssignGroom(ActorComponent, &FMetaHumanDefaultAssemblyOutput::Peachfuzz, AssemblyStruct);
			}
			else if ((ComponentClass->IsChildOf<UChaosClothComponent>()
				|| ComponentClass->IsChildOf<USkeletalMeshComponent>()) 
				&& Handle != RootHandle)
			{
				// All components that are not part of the blueprint template
				// will be deleted. As the pipeline reuses existing blueprint
				// if available, we want to prevent the code from accumulating
				// new components on each build run.
				OldComponentHandles.Add(Handle);
			}
		}
	}

	// Remove old components
	while (!OldComponentHandles.IsEmpty())
	{
		SubobjectDataSubsystem->DeleteSubobject(RootHandle, OldComponentHandles.Pop(), InBlueprint);
	}

	// Assign body components
	if (BodyHandle.IsValid())
	{
		USkeletalMeshComponent* BodyComponent = const_cast<USkeletalMeshComponent*>(static_cast<const USkeletalMeshComponent*>(BodyHandle.GetData()->GetObjectForBlueprint<UActorComponent>(InBlueprint)));

		if (!BodyComponent)
		{
			FMessageLog(UE::MetaHuman::MessageLogName).Error(LOCTEXT("BodyComponentInvalid", "Body component is expected to be skeletal mesh."))
				->AddToken(FUObjectToken::Create(InBlueprint));
			return false;
		}

		// Set up body mesh
		SetSkeletalMesh(BodyComponent, AssemblyStruct->BodyMesh);

		// Set up cloth components
		{
			TArray<FMetaHumanOutfitPipelineAssemblyOutput> OutfitAssetClothData;
			TArray<FMetaHumanOutfitPipelineAssemblyOutput> SkeletalMeshClothData;
			OutfitAssetClothData.Reserve(AssemblyStruct->ClothData.Num());
			SkeletalMeshClothData.Reserve(AssemblyStruct->ClothData.Num());

			for (const FMetaHumanOutfitPipelineAssemblyOutput& ClothData : AssemblyStruct->ClothData)
			{
				if (ClothData.Outfit)
				{
					OutfitAssetClothData.Add(ClothData);
				}
				else if (ClothData.OutfitMesh)
				{
					SkeletalMeshClothData.Add(ClothData);
				}
			}

			AssignComponents<UChaosClothComponent, FMetaHumanOutfitPipelineAssemblyOutput>(
				InBlueprint,
				OutfitAssetClothData,
				BodyHandle,
				[](const FMetaHumanOutfitPipelineAssemblyOutput& AssemblyData, TObjectPtr<UChaosClothComponent> Component)
				{
					AssignClothOutfit(Component, AssemblyData);
				});

			AssignComponents<USkeletalMeshComponent, FMetaHumanOutfitPipelineAssemblyOutput>(
				InBlueprint,
				SkeletalMeshClothData,
				BodyHandle,
				[](const FMetaHumanOutfitPipelineAssemblyOutput& AssemblyData, TObjectPtr<USkeletalMeshComponent> Component)
				{
					AssignSkelMeshOutfit(Component, AssemblyData);
				});
		}

		// Set up skeletal mesh components
		AssignComponents<USkeletalMeshComponent, FMetaHumanSkeletalMeshPipelineAssemblyOutput>(
			InBlueprint,
			AssemblyStruct->SkeletalMeshData,
			BodyHandle,
			[](const FMetaHumanSkeletalMeshPipelineAssemblyOutput& AssemblyData, TObjectPtr<USkeletalMeshComponent> Component)
			{
				AssignSkelMeshClothing(Component, AssemblyData);
			});
	}

	const FBPCompileRequest Request(InBlueprint, EBlueprintCompileOptions::None, nullptr);
	FBlueprintCompilationManager::CompileSynchronously(Request);

	InBlueprint->MarkPackageDirty();

	// Calls post edit change in all actors that derive from this blueprint
	const bool bComponentEditChange = true;
	FBlueprintEditorUtils::PostEditChangeBlueprintActors(InBlueprint, bComponentEditChange);

	return true;
}

#undef LOCTEXT_NAMESPACE 