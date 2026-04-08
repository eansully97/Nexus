#include "Nexus/HUD/Widgets/NexusMainHUDWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Widget.h"
#include "Nexus/HUD/Widgets/NexusAbilityContainer.h"
#include "Nexus/HUD/Widgets/NexusVitalsWidget.h"
#include "Nexus/Character/NexusCharacterBase.h"

void UNexusMainHUDWidget::SetObservedPawn(APawn* NewPawn)
{
	ObservedCharacter = Cast<ANexusCharacterBase>(NewPawn);

	if (VitalsWidget)
	{
		VitalsWidget->SetObservedPawn(NewPawn);
	}

	PropagateObservedPawnToChildWidgets(NewPawn);
}

void UNexusMainHUDWidget::PropagateObservedPawnToChildWidgets(APawn* NewPawn)
{
	if (!WidgetTree)
	{
		return;
	}

	TArray<UWidget*> AllWidgets;
	WidgetTree->GetAllWidgets(AllWidgets);

	for (UWidget* Widget : AllWidgets)
	{
		if (!Widget)
		{
			continue;
		}

		if (UNexusAbilityContainer* AbilityContainer = Cast<UNexusAbilityContainer>(Widget))
		{
			AbilityContainer->SetObservedPawn(NewPawn);
		}

		if (UNexusVitalsWidget* NestedVitalsWidget = Cast<UNexusVitalsWidget>(Widget))
		{
			if (NestedVitalsWidget != VitalsWidget)
			{
				NestedVitalsWidget->SetObservedPawn(NewPawn);
			}
		}
	}
}