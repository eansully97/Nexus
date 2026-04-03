#include "MainMenuGameMode.h"

#include "Nexus/Controller/ANexusMenuPlayerController.h"

AMainMenuGameMode::AMainMenuGameMode()
{
	PlayerControllerClass = ANexusMenuPlayerController::StaticClass();
	DefaultPawnClass = nullptr;
	HUDClass = nullptr;
	bStartPlayersAsSpectators = true;
}