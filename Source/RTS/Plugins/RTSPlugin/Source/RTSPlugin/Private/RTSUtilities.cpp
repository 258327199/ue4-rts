#include "RTSPluginPrivatePCH.h"
#include "RTSUtilities.h"

#include "GameFramework/Actor.h"

#include "RTSConstructionSiteComponent.h"


URTSUtilities::URTSUtilities(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}


bool URTSUtilities::IsReadyToUse(AActor* Actor)
{
	if (!Actor)
	{
		return false;
	}

	auto ConstructionSiteComponent = Actor->FindComponentByClass<URTSConstructionSiteComponent>();

	return ConstructionSiteComponent == nullptr || ConstructionSiteComponent->IsFinished();
}

template<class T>
T* URTSUtilities::FindDefaultComponentByClass(const TSubclassOf<AActor> InActorClass)
{
	return (T*)FindDefaultComponentByClass(InActorClass, T::StaticClass());
}

UActorComponent* URTSUtilities::FindDefaultComponentByClass(const TSubclassOf<AActor> InActorClass, const TSubclassOf<UActorComponent> InComponentClass)
{
	// Check CDO.
	AActor* ActorCDO = InActorClass->GetDefaultObject<AActor>();;
	UActorComponent* FoundComponent = ActorCDO->FindComponentByClass(InComponentClass);

	if (FoundComponent)
	{
		return FoundComponent;
	}

	// Check blueprint nodes. Components added in blueprint editor only (and not in code) are not available from CDO.
	UBlueprintGeneratedClass* ActorBlueprintGeneratedClass = Cast<UBlueprintGeneratedClass>(InActorClass);

	if (!ActorBlueprintGeneratedClass)
	{
		return nullptr;
	}

	const TArray<USCS_Node*>& ActorBlueprintNodes = ActorBlueprintGeneratedClass->SimpleConstructionScript->GetAllNodes();

	for (USCS_Node* Node : ActorBlueprintNodes)
	{
		if (Node->ComponentClass->IsChildOf(InComponentClass))
		{
			return Node->ComponentTemplate;
		}
	}

	return nullptr;
}
