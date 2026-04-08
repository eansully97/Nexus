#include "MainMenuGameMode.h"

#include "Nexus/Controller/NexusMenuPlayerController.h"

AMainMenuGameMode::AMainMenuGameMode()
{
	PlayerControllerClass = ANexusMenuPlayerController::StaticClass();
	DefaultPawnClass = nullptr;
	HUDClass = nullptr;
	bStartPlayersAsSpectators = true;
}
