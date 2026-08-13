/*
 * Copyright (C) 2023-2026  PvZ TV Touch Team
 *
 * This file is part of PlantsVsZombies-AndroidTV.
 *
 * PlantsVsZombies-AndroidTV is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * PlantsVsZombies-AndroidTV is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
 * Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * PlantsVsZombies-AndroidTV.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "PvZ/HookInit.h"
#include "Homura/HookUtils.h"
#include "Homura/MemberUtils.h"
#include "PvZ/Android/IntroVideo.h"
#include "PvZ/Android/Native/AudioOutput.h"
#include "PvZ/Lawn/Board/Board.h"
#include "PvZ/Lawn/Board/Challenge.h"
#include "PvZ/Lawn/Board/Coin.h"
#include "PvZ/Lawn/Board/CursorObject.h"
#include "PvZ/Lawn/Board/CutScene.h"
#include "PvZ/Lawn/Board/GridItem.h"
#include "PvZ/Lawn/Board/MessageWidget.h"
#include "PvZ/Lawn/Board/Plant.h"
#include "PvZ/Lawn/Board/Projectile.h"
#include "PvZ/Lawn/Board/SeedBank.h"
#include "PvZ/Lawn/Board/SeedPacket.h"
#include "PvZ/Lawn/Board/ZenGarden.h"
#include "PvZ/Lawn/Board/Zombie.h"
#include "PvZ/Lawn/GamepadControls.h"
#include "PvZ/Lawn/LawnApp.h"
#include "PvZ/Lawn/System/Music.h"
#include "PvZ/Lawn/System/PoolEffect.h"
#include "PvZ/Lawn/System/ReanimationLawn.h"
#include "PvZ/Lawn/System/SaveGame.h"
#include "PvZ/Lawn/Widget/AlmanacDialog.h"
#include "PvZ/Lawn/Widget/AwardScreen.h"
#include "PvZ/Lawn/Widget/ChallengeScreen.h"
#include "PvZ/Lawn/Widget/ConfirmBackToMainDialog.h"
#include "PvZ/Lawn/Widget/CreditScreen.h"
#include "PvZ/Lawn/Widget/HelpBarWidget.h"
#include "PvZ/Lawn/Widget/HelpOptionsDialog.h"
#include "PvZ/Lawn/Widget/HelpTextScreen.h"
#include "PvZ/Lawn/Widget/HouseChooserDialog.h"
#include "PvZ/Lawn/Widget/ImitaterDialog.h"
#include "PvZ/Lawn/Widget/MailScreen.h"
#include "PvZ/Lawn/Widget/MainMenu.h"
#include "PvZ/Lawn/Widget/NewOptionsDialog.h"
#include "PvZ/Lawn/Widget/SeedChooserScreen.h"
#include "PvZ/Lawn/Widget/SettingsDialog.h"
#include "PvZ/Lawn/Widget/StoreScreen.h"
#include "PvZ/Lawn/Widget/TitleScreen.h"
#include "PvZ/Lawn/Widget/TrashBin.h"
#include "PvZ/Lawn/Widget/VSResultsMenu.h"
#include "PvZ/Lawn/Widget/VSSetupMenu.h"
#include "PvZ/Lawn/Widget/WaitForSecondPlayerDialog.h"
#include "PvZ/SexyAppFramework/Graphics/DeviceImage.h"
#include "PvZ/SexyAppFramework/Graphics/Graphics.h"
#include "PvZ/SexyAppFramework/Widget/ButtonWidget.h"
#include "PvZ/SexyAppFramework/Widget/EditWidget.h"
#include "PvZ/SexyAppFramework/Widget/WidgetManager.h"
#include "PvZ/Symbols.h"
#include "PvZ/TodLib/Effect/Reanimator.h"

void InitHookFunction() {
    homura::HookFunc(LawnApp_LawnAppAddr, &LawnApp::_constructor, &old_LawnApp_LawnApp);
    homura::HookFunc(LawnApp__destructorAddr, &LawnApp::_destructor, &old_LawnApp__destructor);
    homura::HookFunc(LawnApp_InitAddr, &LawnApp::Init, &old_LawnApp_Init);
    homura::HookFunc(LawnApp_IsNightAddr, &LawnApp::IsNight, &old_LawnApp_IsNight);
    homura::HookFunc(LawnApp_HardwareInitAddr, &LawnApp::HardwareInit, &old_LawnApp_HardwareInit);
    homura::HookFunc(LawnApp_DoBackToMainAddr, &LawnApp::DoBackToMain, &old_LawnApp_DoBackToMain);
    homura::HookFunc(LawnApp_DoSettingsDialogAddr, &LawnApp::DoSettingsDialog, nullptr);
    homura::HookFunc(LawnApp_CanShopLevelAddr, &LawnApp::CanShopLevel, &old_LawnApp_CanShopLevel);
    homura::HookFunc(LawnApp_DoNewOptionsAddr, &LawnApp::DoNewOptions, &old_LawnApp_DoNewOptions);
    homura::HookFunc(LawnApp_GetNumPreloadingTasksAddr, &LawnApp::GetNumPreloadingTasks, &old_LawnApp_GetNumPreloadingTasks);
    homura::HookFunc(LawnApp_DoConfirmBackToMainAddr, &LawnApp::DoConfirmBackToMain, nullptr);
    homura::HookFunc(LawnApp_TrophiesNeedForGoldSunflowerAddr, &LawnApp::TrophiesNeedForGoldSunflower, nullptr);
    homura::HookFunc(LawnApp_GamepadToPlayerIndexAddr, &LawnApp::GamepadToPlayerIndex, nullptr);
    homura::HookFunc(LawnApp_ShowCreditScreenAddr, &LawnApp::ShowCreditScreen, &old_LawnApp_ShowCreditScreen);
    homura::HookFunc(LawnApp_OnSessionTaskFailedAddr, &LawnApp::OnSessionTaskFailed, nullptr);
    homura::HookFunc(LawnApp_UpdateAppAddr, &LawnApp::UpdateApp, &old_LawnApp_UpDateApp);
    homura::HookFunc(LawnApp_ShowAwardScreenAddr, &LawnApp::ShowAwardScreen, &old_LawnApp_ShowAwardScreen);
    homura::HookFunc(LawnApp_KillAwardScreenAddr, &LawnApp::KillAwardScreen, &old_LawnApp_KillAwardScreen);
    homura::HookFunc(LawnApp_LoadLevelConfigurationAddr, &LawnApp::LoadLevelConfiguration, &old_LawnApp_LoadLevelConfiguration);
    homura::HookFunc(LawnApp_LoadingThreadProcAddr, &LawnApp::LoadingThreadProc, &old_LawnApp_LoadingThreadProc);
    homura::HookFunc(LawnApp_IsChallengeWithoutSeedBankAddr, &LawnApp::IsChallengeWithoutSeedBank, &old_LawnApp_IsChallengeWithoutSeedBank);
    homura::HookFunc(LawnApp_TryHelpTextScreenAddr, &LawnApp::TryHelpTextScreen, nullptr);
    homura::HookFunc(LawnApp_GetSeedsAvailableAddr, &LawnApp::GetSeedsAvailable, &old_LawnApp_GetSeedsAvailable);
    homura::HookFunc(LawnApp_ClearSecondPlayerAddr, &LawnApp::ClearSecondPlayer, &old_LawnApp_ClearSecondPlayer);
    homura::HookFunc(LawnApp_HasSeedTypeAddr, &LawnApp::HasSeedType, &old_LawnApp_HasSeedType);
    homura::HookFunc(LawnApp_UpdateFramesAddr, &LawnApp::UpdateFrames, nullptr);
    homura::HookFunc(LawnApp_FinishLoadGameAddr, &LawnApp::FinishLoadGame, nullptr);
    homura::HookFunc(LawnApp_ShowSeedChooserScreenAddr, &LawnApp::ShowSeedChooserScreen, nullptr);
    homura::HookFunc(LawnApp_KillSeedChooserScreenAddr, &LawnApp::KillSeedChooserScreen, nullptr);
    homura::HookFunc(LawnApp_ShowZombieChooserScreenAddr, &LawnApp::ShowZombieChooserScreen, nullptr);
    homura::HookFunc(LawnApp_KillZombieChooserScreenAddr, &LawnApp::KillZombieChooserScreen, nullptr);
    homura::HookFunc(LawnApp_ShowChallengeScreenAddr, &LawnApp::ShowChallengeScreen, nullptr);
    homura::HookFunc(LawnApp_KillChallengeScreenAddr, &LawnApp::KillChallengeScreen, nullptr);
    homura::HookFunc(LawnApp_ShowVSSetupScreenAddr, &LawnApp::ShowVSSetupScreen, nullptr);
    homura::HookFunc(LawnApp_PreNewGameAddr, &LawnApp::PreNewGame, &old_LawnApp_PreNewGame);
    homura::HookFunc(LawnApp_NewGameAddr, &LawnApp::NewGame, nullptr);
    homura::HookFunc(LawnApp_MakeNewBoardAddr, &LawnApp::MakeNewBoard, nullptr);
    homura::HookFunc(LawnApp_HasBeatenChallengeAddr, &LawnApp::HasBeatenChallenge, nullptr);
    homura::HookFunc(LawnApp_ShowVSResultsScreenAddr, &LawnApp::ShowVSResultsScreen, nullptr);
    homura::HookFunc(LawnApp_KillVSResultsScreenAddr, &LawnApp::KillVSResultsScreen, nullptr);
    homura::HookFunc(LawnApp_LoadingCompletedAddr, &LawnApp::LoadingCompleted, nullptr);
    homura::HookFunc(LawnApp_TryLoadGameAddr, &LawnApp::TryLoadGame, nullptr);


    homura::HookFunc(Board_DrawAddr, &Board::Draw, &old_Board_Draw);
    homura::HookFunc(Board_MouseDownWithPlantAddr, &Board::MouseDownWithPlant, &old_Board_MouseDownWithPlant);
    homura::HookFunc(Board_UpdateAddr, &Board::Update, &old_Board_Update);
    homura::HookFunc(Board_BoardAddr, &Board::_constructor, nullptr);
    homura::HookFunc(Board__destructorAddr, &Board::_destructor, nullptr);
    homura::HookFunc(Board_InitLevelAddr, &Board::InitLevel, &old_Board_InitLevel);
    homura::HookFunc(Board_StartLevelAddr, &Board::StartLevel, &old_Board_StartLevel);
    homura::HookFunc(Board_RemovedFromManagerAddr, &Board::RemovedFromManager, &old_Board_RemovedFromManager);
    homura::HookFunc(Board_FadeOutLevelAddr, &Board::FadeOutLevel, &old_Board_FadeOutLevel);
    homura::HookFunc(Board_AddPlantAddr, &Board::AddPlant, &old_Board_AddPlant);
    homura::HookFunc(Board_AddSunMoneyAddr, &Board::AddSunMoney, &old_Board_AddSunMoney);
    homura::HookFunc(Board_AddDeathMoneyAddr, &Board::AddDeathMoney, &old_Board_AddDeathMoney);
    homura::HookFunc(Board_CanPlantAtAddr, &Board::CanPlantAt, nullptr);
    homura::HookFunc(Board_PlantingRequirementsMetAddr, &Board::PlantingRequirementsMet, &old_Board_PlantingRequirementsMet);
    homura::HookFunc(Board_GetFlowerPotAtAddr, &Board::GetFlowerPotAt, nullptr);
    homura::HookFunc(Board_GetPumpkinAtAddr, &Board::GetPumpkinAt, nullptr);
    homura::HookFunc(Board_ZombiesWonAddr, &Board::ZombiesWon, &old_BoardZombiesWon);
    homura::HookFunc(Board_KeyDownAddr, &Board::KeyDown, &old_Board_KeyDown);
    homura::HookFunc(Board_KeyUpAddr, &Board::KeyUp, &old_Board_KeyUp);
    homura::HookFunc(Board_GameButtonUpAddr, &Board::GameButtonUp, nullptr);
    homura::HookFunc(Board_UpdateSunSpawningAddr, &Board::UpdateSunSpawning, nullptr);
    homura::HookFunc(Board_UpdateZombieSpawningAddr, &Board::UpdateZombieSpawning, &old_Board_UpdateZombieSpawning);
    homura::HookFunc(Board_PickBackgroundAddr, &Board::PickBackground, &old_Board_PickBackground);
    homura::HookFunc(Board_DrawCoverLayerAddr, &Board::DrawCoverLayer, nullptr);
    homura::HookFunc(Board_UpdateGameAddr, &Board::UpdateGame, &old_Board_UpdateGame);
    homura::HookFunc(Board_UpdateGameObjectsAddr, &Board::UpdateGameObjects, &old_Board_UpdateGameObjects);
    homura::HookFunc(Board_IsFlagWaveAddr, &Board::IsFlagWave, &old_Board_IsFlagWave);
    homura::HookFunc(Board_SpawnZombieWaveAddr, &Board::SpawnZombieWave, &old_Board_SpawnZombieWave);
    homura::HookFunc(Board_DrawProgressMeterAddr, &Board::DrawProgressMeter, &old_Board_DrawProgressMeter);
    homura::HookFunc(Board_GetNumWavesPerFlagAddr, &Board::GetNumWavesPerFlag, nullptr);
    homura::HookFunc(Board_IsLevelDataLoadedAddr, &Board::IsLevelDataLoaded, &old_Board_IsLevelDataLoaded);
    homura::HookFunc(Board_NeedSaveGameAddr, &Board::NeedSaveGame, &old_Board_NeedSaveGame);
    homura::HookFunc(Board_UpdateFwooshAddr, &Board::UpdateFwoosh, &old_Board_UpdateFwoosh);
    homura::HookFunc(Board_UpdateFogAddr, &Board::UpdateFog, &old_Board_UpdateFog);
    homura::HookFunc(Board_DrawFogAddr, &Board::DrawFog, &old_Board_DrawFog);
    homura::HookFunc(Board_UpdateIceAddr, &Board::UpdateIce, &old_Board_UpdateIce);
    homura::HookFunc(Board_DrawBackdropAddr, &Board::DrawBackdrop, &old_Board_DrawBackdrop);
    homura::HookFunc(Board_RowCanHaveZombieTypeAddr, &Board::RowCanHaveZombieType, nullptr);
    homura::HookFunc(Board_DrawDebugTextAddr, &Board::DrawDebugText, &old_Board_DrawDebugText);
    homura::HookFunc(Board_DrawDebugObjectRectsAddr, &Board::DrawDebugObjectRects, &old_Board_DrawDebugObjectRects);
    homura::HookFunc(Board_DrawFadeOutAddr, &Board::DrawFadeOut, nullptr);
    homura::HookFunc(Board_GetCurrentPlantCostAddr, &Board::GetCurrentPlantCost, &old_Board_GetCurrentPlantCost);
    homura::HookFunc(Board_PauseAddr, &Board::Pause, &old_Board_Pause);
    homura::HookFunc(Board_AddSecondPlayerAddr, &Board::AddSecondPlayer, nullptr);
    homura::HookFunc(Board_IsLastStandFinalStageAddr, &Board::IsLastStandFinalStage, nullptr);
    homura::HookFunc(Board_MouseHitTestAddr, &Board::MouseHitTest, &old_Board_MouseHitTest);
    homura::HookFunc(Board_DrawShovelAddr, &Board::DrawShovel, nullptr);
    homura::HookFunc(Board_StageHasPoolAddr, &Board::StageHasPool, nullptr);
    homura::HookFunc(Board_AddZombieInRowAddr, &Board::AddZombieInRow, &old_Board_AddZombieInRow);
    homura::HookFunc(Board_AddZombieAddr, &Board::AddZombie, nullptr);
    homura::HookFunc(Board_DoPlantingEffectsAddr, &Board::DoPlantingEffects, nullptr);
    homura::HookFunc(Board_InitLawnMowersAddr, &Board::InitLawnMowers, &old_Board_InitLawnMowers);
    homura::HookFunc(Board_PickZombieTypeAddr, &Board::PickZombieType, &old_Board_PickZombieType);
    homura::HookFunc(Board_PickZombieWavesAddr, &Board::PickZombieWaves, &old_Board_PickZombieWaves);
    homura::HookFunc(Board_DrawUITopAddr, &Board::DrawUITop, &old_Board_DrawUITop);
    homura::HookFunc(Board_GetShovelButtonRectAddr, &Board::GetShovelButtonRect, &old_Board_GetShovelButtonRect);
    homura::HookFunc(Board_UpdateLevelEndSequenceAddr, &Board::UpdateLevelEndSequence, &old_Board_UpdateLevelEndSequence);
    homura::HookFunc(Board_UpdateGridItemsAddr, &Board::UpdateGridItems, &old_Board_UpdateGridItems);
    homura::HookFunc(Board_ShakeBoardAddr, &Board::ShakeBoard, &old_Board_ShakeBoard);
    homura::HookFunc(Board_DrawZenButtonsAddr, &Board::DrawZenButtons, &old_Board_DrawZenButtons);
    homura::HookFunc(Board_DrawGameObjectsAddr, &Board::DrawGameObjects, &old_Board_DrawGameObjects);
    homura::HookFunc(Board_AddProjectileAddr, &Board::AddProjectile, nullptr);
    homura::HookFunc(Board_IterateProjectilesAddr, &Board::IterateProjectiles, nullptr);
    homura::HookFunc(Board_ProcessDeleteQueueAddr, &Board::ProcessDeleteQueue, nullptr);
    // homura::HookFunc(Board_PixelToGridXAddr, &Board::PixelToGridX, &old_Board_PixelToGridX);
    // homura::HookFunc(Board_PixelToGridYAddr, &Board::PixelToGridY, &old_Board_PixelToGridY);
    homura::HookFunc(Board_GetNumSeedsInBankAddr, &Board::GetNumSeedsInBank, &old_Board_GetNumSeedsInBank);
    homura::HookFunc(Board_GetSeedPacketPositionXAddr, &Board::GetSeedPacketPositionX, nullptr);
    homura::HookFunc(Board_AddCoinAddr, &Board::AddCoin, &old_Board_AddCoin);
    homura::HookFunc(Board_AddAGraveStoneAddr, &Board::AddAGraveStone, &old_Board_AddAGraveStone);
    homura::HookFunc(Board_TakeSunMoneyAddr, &Board::TakeSunMoney, &old_Board_TakeSunMoney);
    homura::HookFunc(Board_TakeDeathMoneyAddr, &Board::TakeDeathMoney, &old_Board_TakeDeathMoney);
    homura::HookFunc(Board_SpawnZombiesFromGravesAddr, &Board::SpawnZombiesFromGraves, nullptr);
    homura::HookFunc(Board_CanAddGraveStoneAtAddr, &Board::CanAddGraveStoneAt, nullptr);
    homura::HookFunc(Board_DrawLevelAddr, &Board::DrawLevel, &old_Board_DrawLevel);
    homura::HookFunc(Board_CanAddBobSledMPAddr, &Board::CanAddBobSledMP, nullptr);
    homura::HookFunc(Board_AddMPTargetAddr, &Board::AddMPTarget, nullptr);
    homura::HookFunc(Board_PlantsWonAddr, &Board::PlantsWon, &old_Board_PlantsWon);
    homura::HookFunc(Board_AddALadderAddr, &Board::AddALadder, nullptr);
    homura::HookFunc(Board_AddACraterAddr, &Board::AddACrater, nullptr);
    homura::HookFunc(Board_IsZombieTypePoolOnlyAddr, &Board::IsZombieTypePoolOnly, nullptr);


    homura::HookFunc(FixBoardAfterLoadAddr, &FixBoardAfterLoad, nullptr);
    homura::HookFunc(LawnSaveGameAddr, &LawnSaveGame, nullptr);
    homura::HookFunc(LawnLoadGameAddr, &LawnLoadGame, nullptr);
    homura::HookFunc(SyncDataArray_ProjectileAddr, &SyncDataArray<Projectile>, nullptr);


    homura::HookFunc(Challenge_UpdateAddr, &Challenge::Update, &old_Challenge_Update);
    homura::HookFunc(Challenge_IsMPSuddenDeathAddr, &Challenge::IsMPSuddenDeath, nullptr);
    homura::HookFunc(Challenge_GetSuddenDeathCountAddr, &Challenge::GetSuddenDeathCount, nullptr);
    homura::HookFunc(Challenge_ChallengeAddr, &Challenge::_constructor, &old_Challenge_Challenge);
    homura::HookFunc(Challenge_HeavyWeaponFireAddr, &Challenge::HeavyWeaponFire, &old_Challenge_HeavyWeaponFire);
    homura::HookFunc(Challenge_IZombieDrawPlantAddr, &Challenge::IZombieDrawPlant, nullptr);
    homura::HookFunc(Challenge_HeavyWeaponUpdateAddr, &Challenge::HeavyWeaponUpdate, &old_Challenge_HeavyWeaponUpdate);
    homura::HookFunc(Challenge_IZombieEatBrainAddr, &Challenge::IZombieEatBrain, nullptr);
    homura::HookFunc(Challenge_DrawArtChallengeAddr, &Challenge::DrawArtChallenge, nullptr);
    homura::HookFunc(Challenge_CanPlantAtAddr, &Challenge::CanPlantAt, nullptr);
    homura::HookFunc(Challenge_InitLevelAddr, &Challenge::InitLevel, &old_Challenge_InitLevel);
    homura::HookFunc(Challenge_InitZombieWavesAddr, &Challenge::InitZombieWaves, &old_Challenge_InitZombieWaves);
    homura::HookFunc(Challenge_TreeOfWisdomFertilizeAddr, &Challenge::TreeOfWisdomFertilize, &old_Challenge_TreeOfWisdomFertilize);
    homura::HookFunc(Challenge_LastStandUpdateAddr, &Challenge::LastStandUpdate, nullptr);
    homura::HookFunc(Challenge_DrawHeavyWeaponAddr, &Challenge::DrawHeavyWeapon, nullptr);
    homura::HookFunc(Challenge_UpdateZombieSpawningAddr, &Challenge::UpdateZombieSpawning, &old_Challenge_UpdateZombieSpawning);
    homura::HookFunc(Challenge_HeavyWeaponPacketClickedAddr, &Challenge::HeavyWeaponPacketClicked, &old_Challenge_HeavyWeaponPacketClicked);
    homura::HookFunc(Challenge_IZombieSeedTypeToZombieTypeAddr, &Challenge::IZombieSeedTypeToZombieType, nullptr);
    homura::HookFunc(Challenge_StartLevelAddr, &Challenge::StartLevel, &old_Challenge_StartLevel);
    homura::HookFunc(Challenge_DeleteAddr, &Challenge::_destructor, &old_Challenge_Delete);
    homura::HookFunc(Challenge_ScaryPotterOpenPotAddr, &Challenge::ScaryPotterOpenPot, &old_Challenge_ScaryPotterOpenPot);
    homura::HookFunc(Challenge_IZombieGetBrainTargetAddr, &Challenge::IZombieGetBrainTarget, &old_Challenge_IZombieGetBrainTarget);
    homura::HookFunc(Challenge_IZombieSquishBrainAddr, &Challenge::IZombieSquishBrain, &old_Challenge_IZombieSquishBrain);
    homura::HookFunc(Challenge_UpdateConveyorBeltAddr, &Challenge::UpdateConveyorBelt, &old_Challenge_UpdateConveyorBelt);
    homura::HookFunc(Challenge_MouseDownWhackAZombieAddr, &Challenge::MouseDownWhackAZombie, nullptr);
    homura::HookFunc(Challenge_DrawWeatherAddr, &Challenge::DrawWeather, nullptr);
    homura::HookFunc(Challenge_UpdateMPGraveStonesAddr, &Challenge::UpdateMPGraveStones, nullptr);
    homura::HookFunc(Challenge_IsMPResourceProducerAddr, &Challenge::IsMPResourceProducer, nullptr);
    homura::HookFunc(Challenge_ISMPSeedSuddenDeathDisabledAddr, &Challenge::ISMPSeedSuddenDeathDisabled, nullptr);
    homura::HookFunc(Challenge_DrawBackdropAddr, &Challenge::DrawBackdrop, &old_Challenge_DrawBackdrop);


    homura::HookFunc(ChallengeScreen_AddedToManagerAddr, &ChallengeScreen::AddedToManager, &old_ChallengeScreen_AddedToManager);
    homura::HookFunc(ChallengeScreen_RemovedFromManagerAddr, &ChallengeScreen::RemovedFromManager, &old_ChallengeScreen_RemovedFromManager);
    homura::HookFunc(ChallengeScreen_Delete2Addr, &ChallengeScreen::_destructor, &old_ChallengeScreen__destructor);
    homura::HookFunc(ChallengeScreen_UpdateAddr, &ChallengeScreen::Update, &old_ChallengeScreen_Update);
    homura::HookFunc(ChallengeScreen_ChallengeScreenAddr, &ChallengeScreen::_constructor, &old_ChallengeScreen_ChallengeScreen);
    homura::HookFunc(ChallengeScreen_DrawAddr, &ChallengeScreen::Draw, &old_ChallengeScreen_Draw);
    homura::HookFunc(ChallengeScreen_KeyDownAddr, &ChallengeScreen::KeyDown, &old_ChallengeScreen_KeyDown);
    homura::HookFunc(ChallengeScreen_ButtonDepressAddr, &ChallengeScreen::ButtonDepress, nullptr);
    homura::HookFunc(ChallengeScreen_UpdateButtonsAddr, &ChallengeScreen::UpdateButtons, nullptr);
    homura::HookFunc(ChallengeScreen_DrawButtonAddr, &ChallengeScreen::DrawButton, &old_ChallengeScreen_DrawButton);
    homura::HookFunc(GetChallengeDefinitionAddr, &GetChallengeDefinition, nullptr);


    homura::HookFunc(Coin_CoinInitializeAddr, &Coin::CoinInitialize, &old_Coin_CoinInitialize);
    homura::HookFunc(Coin_UpadteAddr, &Coin::Update, &old_Coin_Update);
    homura::HookFunc(Coin_GamepadCursorOverAddr, &Coin::GamepadCursorOver, &old_Coin_GamepadCursorOver);
    homura::HookFunc(Coin_MouseHitTestAddr, &Coin::MouseHitTest, &old_Coin_MouseHitTest);
    homura::HookFunc(Coin_UpdateFallAddr, &Coin::UpdateFall, &old_Coin_UpdateFall);
    homura::HookFunc(Coin_DrawAddr, &Coin::Draw, &old_Coin_Draw);
    homura::HookFunc(Coin_PlayCollectSoundAddr, &Coin::PlayCollectSound, nullptr);
    homura::HookFunc(Coin_ScoreCoinAddr, &Coin::ScoreCoin, nullptr);
    homura::HookFunc(Coin_UpdateCollectedAddr, &Coin::UpdateCollected, nullptr);
    homura::HookFunc(Coin_GetSunValueAddr, &Coin::GetSunValue, nullptr);
    homura::HookFunc(Coin_GetSunScaleAddr, &Coin::GetSunScale, nullptr);
    homura::HookFunc(Coin_GetColorAddr, &Coin::GetColor, nullptr);
    homura::HookFunc(Board_CountDeathBeingCollectedAddr, &Board::CountDeathBeingCollected, nullptr);


    homura::HookFunc(GamepadControls_ButtonDownFireCobcannonTestAddr, &GamepadControls::ButtonDownFireCobcannonTest, &old_GamepadControls_ButtonDownFireCobcannonTest);
    homura::HookFunc(GamepadControls_DrawAddr, &GamepadControls::Draw, &old_GamepadControls_Draw);
    homura::HookFunc(GamepadControls_GamepadControlsAddr, &GamepadControls::_constructor, &old_GamepadControls_GamepadControls);
    homura::HookFunc(GamepadControls_UpdateAddr, &GamepadControls::Update, &old_GamepadControls_Update);
    homura::HookFunc(GamepadControls_DrawPreviewAddr, &GamepadControls::DrawPreview, &old_GamepadControls_DrawPreview);
    homura::HookFunc(GamepadControls_UpdatePreviewReanimAddr, &GamepadControls::UpdatePreviewReanim, &old_GamepadControls_UpdatePreviewReanim);
    homura::HookFunc(GamepadControls_OnButtonDownAddr, &GamepadControls::OnButtonDown, &old_GamepadControls_OnButtonDown);
    homura::HookFunc(GamepadControls_UpdateStatesAddr, &GamepadControls::UpdateStates, nullptr);


    homura::HookFunc(GridItem__constructorAddr, &GridItem::_constructor, &old_GridItem_GridItem);
    homura::HookFunc(GridItem_GridItemDieAddr, &GridItem::GridItemDie, &old_GridItem_GridItemDie);
    homura::HookFunc(GridItem_DrawGridItemAddr, &GridItem::DrawGridItem, &old_GridItem_DrawGridItem);
    homura::HookFunc(GridItem_UpdateAddr, &GridItem::Update, &old_GridItem_Update);
    homura::HookFunc(GridItem_UpdateScaryPotAddr, &GridItem::UpdateScaryPot, &old_GridItem_UpdateScaryPot);
    homura::HookFunc(GridItem_UpdateBurialMoundAddr, &GridItem::UpdateBurialMound, nullptr);
    homura::HookFunc(GridItem_GetMoundUpgradeCostAddr, &GridItem::GetMoundUpgradeCost, nullptr);
    homura::HookFunc(GridItem_DrawStinkyAddr, &GridItem::DrawStinky, &old_GridItem_DrawStinky);
    homura::HookFunc(GridItem_DrawSquirrelAddr, &GridItem::DrawSquirrel, nullptr);
    homura::HookFunc(GridItem_DrawScaryPotAddr, &GridItem::DrawScaryPot, nullptr);
    homura::HookFunc(GridItem_DrawCraterAddr, &GridItem::DrawCrater, nullptr);
    homura::HookFunc(GridItem_DrawGraveStoneAddr, &GridItem::DrawGraveStone, nullptr);
    homura::HookFunc(GridItem_AddGraveStoneParticlesAddr, &GridItem::AddGraveStoneParticles, nullptr);
    //    homura::HookFunc(GridItem_DrawMPTargetAddr, &GridItem::DrawMPTarget, &old_GridItem_DrawMPTarget);
    homura::HookFunc(GridItem_TakeDamgaeAddr, &GridItem::TakeDamage, &old_GridItem_TakeDamage);


    homura::HookFunc(AlmanacDialog_AddedToManagerAddr, &AlmanacDialog::AddedToManager, &old_AlmanacDialog_AddedToManager);
    homura::HookFunc(AlmanacDialog_RemovedFromManagerAddr, &AlmanacDialog::RemovedFromManager, &old_AlmanacDialog_RemovedFromManager);
    homura::HookFunc(AlmanacDialog_AlmanacDialogAddr, &AlmanacDialog::_constructor, &old_AlmanacDialog_AlmanacDialog);
    homura::HookFunc(AlmanacDialog_SetPageAddr, &AlmanacDialog::SetPage, &old_AlmanacDialog_SetPage);
    homura::HookFunc(AlmanacDialog_MouseDownAddr, &AlmanacDialog::MouseDown, nullptr);
    homura::HookFunc(AlmanacDialog_MouseUpAddr, &AlmanacDialog::MouseUp, nullptr);
    homura::HookFunc(AlmanacDialog_ButtonDepressAddr, &AlmanacDialog::ButtonDepress, nullptr);
    homura::HookFunc(AlmanacDialog_Delete2Addr, &AlmanacDialog::_destructor, &old_AlmanacDialog_Delete2);
    homura::HookFunc(AlmanacDialog_DrawPlantsAddr, &AlmanacDialog::DrawPlants, &old_AlmanacDialog_DrawPlants);
    homura::HookFunc(AlmanacDialog_DrawZombiesAddr, &AlmanacDialog::DrawZombies, nullptr);
    homura::HookFunc(AlmanacDialog_SetupLayoutPlantsAddr, &AlmanacDialog::SetupLayoutPlants, &old_AlmanacDialog_SetupLayoutPlants);
    homura::HookFunc(AlmanacDialog_SetupLayoutZombiesAddr, &AlmanacDialog::SetupLayoutZombies, &old_AlmanacDialog_SetupLayoutZombies);


    homura::HookFunc(SeedChooserScreen_SeedChooserScreenAddr, &SeedChooserScreen::_constructor, nullptr);
    homura::HookFunc(SeedChooserScreen__destructorAddr, &SeedChooserScreen::_destructor, &old_SeedChooserScreen__destructor);
    homura::HookFunc(SeedChooserScreen_HasPacketAddr, &SeedChooserScreen::HasPacket, nullptr);
    homura::HookFunc(SeedChooserScreen_EnableStartButtonAddr, &SeedChooserScreen::EnableStartButton, &old_SeedChooserScreen_EnableStartButton);
    homura::HookFunc(SeedChooserScreen_RebuildHelpbarAddr, &SeedChooserScreen::RebuildHelpbar, &old_SeedChooserScreen_RebuildHelpbar);
    homura::HookFunc(SeedChooserScreen_GetZombieSeedTypeAddr, &SeedChooserScreen::GetZombieSeedType, nullptr);
    homura::HookFunc(SeedChooserScreen_ClickedSeedInChooserAddr, &SeedChooserScreen::ClickedSeedInChooser, nullptr);
    homura::HookFunc(SeedChooserScreen_CrazyDavePickSeedsAddr, &SeedChooserScreen::CrazyDavePickSeeds, nullptr);
    homura::HookFunc(SeedChooserScreen_OnStartButtonAddr, &SeedChooserScreen::OnStartButton, &old_SeedChooserScreen_OnStartButton);
    homura::HookFunc(SeedChooserScreen_UpdateAddr, &SeedChooserScreen::Update, nullptr);
    homura::HookFunc(SeedChooserScreen_UpdateCursorAddr, &SeedChooserScreen::UpdateCursor, nullptr);
    homura::HookFunc(SeedChooserScreen_CloseSeedChooserAddr, &SeedChooserScreen::CloseSeedChooser, nullptr);
    homura::HookFunc(SeedChooserScreen_UpdateImitaterButtonAddr, &SeedChooserScreen::UpdateImitaterButton, nullptr);
    homura::HookFunc(SeedChooserScreen_LandFlyingSeedAddr, &SeedChooserScreen::LandFlyingSeed, nullptr);
    homura::HookFunc(SeedChooserScreen_UpdateAfterPurchaseAddr, &SeedChooserScreen::UpdateAfterPurchase, nullptr);
    homura::HookFunc(SeedChooserScreen_PickRandomSeedsAddr, &SeedChooserScreen::PickRandomSeeds, nullptr);
    homura::HookFunc(SeedChooserScreen_SeedNotAllowedToPickAddr, &SeedChooserScreen::SeedNotAllowedToPick, &old_SeedChooserScreen_SeedNotAllowedToPick);
    homura::HookFunc(SeedChooserScreen_SeedNotRecommendedToPickAddr, &SeedChooserScreen::SeedNotRecommendedToPick, nullptr);
    homura::HookFunc(SeedChooserScreen_ClickedSeedInBankAddr, &SeedChooserScreen::ClickedSeedInBank, nullptr);
    homura::HookFunc(SeedChooserScreen_GameButtonDownAddr, &SeedChooserScreen::GameButtonDown, nullptr);
    homura::HookFunc(SeedChooserScreen_DrawPacketAddr, &SeedChooserScreen::DrawPacket, nullptr);
    homura::HookFunc(SeedChooserScreen_ButtonDepressAddr, &SeedChooserScreen::ButtonDepress, nullptr);
    homura::HookFunc(SeedChooserScreen_GetSeedPositionInBankAddr, &SeedChooserScreen::GetSeedPositionInBank, nullptr);
    homura::HookFunc(SeedChooserScreen_GetSeedPositionInChooserAddr, &SeedChooserScreen::GetSeedPositionInChooser, nullptr);
    homura::HookFunc(SeedChooserScreen_ShowToolTipAddr, &SeedChooserScreen::ShowToolTip, nullptr);
    homura::HookFunc(SeedChooserScreen_GetNextSeedInDirAddr, &SeedChooserScreen::GetNextSeedInDir, nullptr);
    homura::HookFunc(SeedChooserScreen_PickedPlantTypeAddr, &SeedChooserScreen::PickedPlantType, nullptr);
    homura::HookFunc(SeedChooserScreen_DrawAddr, &SeedChooserScreen::Draw, &old_SeedChooserScreen_Draw);
    homura::HookFunc(SeedChooserScreen_SeedHitTestAddr, &SeedChooserScreen::SeedHitTest, nullptr);
    homura::HookFunc(SeedChooserScreen_OnKeyDownAddr, &SeedChooserScreen::OnKeyDown, &old_SeedChooserScreen_OnKeyDown);
    homura::HookFunc(SeedChooserScreen_VSAutoPickResourceGenAddr, &SeedChooserScreen::VSAutoPickResourceGen, nullptr);
    homura::HookFunc(SeedChooserScreen_KeyDownAddr, &SeedChooserScreen::KeyDown, &old_SeedChooserScreen_KeyDown);
    homura::HookFunc(SeedChooserScreen_KeyUpAddr, &SeedChooserScreen::KeyUp, &old_SeedChooserScreen_KeyUp);


    homura::HookFunc(MainMenu_KeyDownAddr, &MainMenu::KeyDown, &old_MainMenu_KeyDown);
    homura::HookFunc(MainMenu_ButtonDepressAddr, &MainMenu::ButtonDepress, &old_MainMenu_ButtonDepress);
    homura::HookFunc(MainMenu_UpdateAddr, &MainMenu::Update, &old_MainMenu_Update);
    homura::HookFunc(MainMenu_SyncProfileAddr, &MainMenu::SyncProfile, &old_MainMenu_SyncProfile);
    homura::HookFunc(MainMenu_EnterAddr, &MainMenu::Enter, &old_MainMenu_Enter);
    homura::HookFunc(MainMenu_ExitAddr, &MainMenu::Exit, &old_MainMenu_Exit);
    homura::HookFunc(MainMenu_UpdateExitAddr, &MainMenu::UpdateExit, &old_MainMenu_UpdateExit);
    homura::HookFunc(MainMenu_OnExitAddr, &MainMenu::OnExit, &old_MainMenu_OnExit);
    // homura::HookFunc(MainMenu_SetSceneAddr, SetScene, &old_MainMenu_SetScene);
    homura::HookFunc(MainMenu_OnSceneAddr, &MainMenu::OnScene, &old_MainMenu_OnScene);
    homura::HookFunc(MainMenu_SyncButtonsAddr, &MainMenu::SyncButtons, &old_MainMenu_SyncButtons);
    homura::HookFunc(MainMenu_MainMenuAddr, &MainMenu::_constructor, &old_MainMenu_MainMenu);
    homura::HookFunc(MainMenu_UpdateCameraPositionAddr, &MainMenu::UpdateCameraPosition, &old_MainMenu_UpdateCameraPosition);
    homura::HookFunc(MainMenu_AddedToManagerAddr, &MainMenu::AddedToManager, &old_MainMenu_AddedToManager);
    homura::HookFunc(MainMenu_RemovedFromManagerAddr, &MainMenu::RemovedFromManager, &old_MainMenu_RemovedFromManager);
    homura::HookFunc(MainMenu_DrawOverlayAddr, &MainMenu::DrawOverlay, &old_MainMenu_DrawOverlay);
    homura::HookFunc(MainMenu_DrawFadeAddr, &MainMenu::DrawFade, &old_MainMenu_DrawFade);
    homura::HookFunc(MainMenu_Delete2Addr, &MainMenu::_destructor2, &old_MainMenu_Delete2);
    homura::HookFunc(MainMenu_DrawAddr, &MainMenu::Draw, &old_MainMenu_Draw);


    homura::HookFunc(StoreScreen_UpdateAddr, &StoreScreen::Update, &old_StoreScreen_Update);
    homura::HookFunc(StoreScreen_SetupPageAddr, &StoreScreen::SetupPage, &old_StoreScreen_SetupPage);
    homura::HookFunc(StoreScreen_IsPageShownAddr, &StoreScreen::IsPageShown, nullptr);
    homura::HookFunc(StoreScreen_ButtonDepressAddr, &StoreScreen::ButtonDepress, &old_StoreScreen_ButtonDepress);
    homura::HookFunc(StoreScreen_AddedToManagerAddr, &StoreScreen::AddedToManager, &old_StoreScreen_AddedToManager);
    homura::HookFunc(StoreScreen_RemovedFromManagerAddr, &StoreScreen::RemovedFromManager, &old_StoreScreen_RemovedFromManager);
    homura::HookFunc(StoreScreen_PurchaseItemAddr, &StoreScreen::PurchaseItem, &old_StoreScreen_PurchaseItem);
    homura::HookFunc(StoreScreen_DrawAddr, &StoreScreen::Draw, &old_StoreScreen_Draw);
    homura::HookFunc(StoreScreen_DrawItemAddr, &StoreScreen::DrawItem, &old_StoreScreen_DrawItem);


    homura::HookFunc(Plant_UpdateAddr, &Plant::Update, &old_Plant_Update);
    homura::HookFunc(Plant_UpdateAbilitiesAddr, &Plant::UpdateAbilities, &old_Plant_UpdateAbilities);
    homura::HookFunc(Plant_SquishAddr, &Plant::Squish, nullptr);
    homura::HookFunc(Plant_AnimateAddr, &Plant::Animate, nullptr);
    homura::HookFunc(Plant_GetPlantRectAddr, &Plant::GetPlantRect, nullptr);
    homura::HookFunc(Plant_GetRefreshTimeAddr, &Plant::GetRefreshTime, &old_Plant_GetRefreshTime);
    homura::HookFunc(Plant_DoSpecialAddr, &Plant::DoSpecial, nullptr);
    homura::HookFunc(Plant_DrawAddr, &Plant::Draw, &old_Plant_Draw);
    homura::HookFunc(Plant_DrawShadowAddr, &Plant::DrawShadow, nullptr);
    homura::HookFunc(Plant_DrawSeedTypeAddr, &Plant::DrawSeedType, nullptr);
    homura::HookFunc(Plant_GetNameStringAddr, &Plant::GetNameString, nullptr);
    homura::HookFunc(Plant_GetToolTipAddr, &Plant::GetToolTip, nullptr);
    homura::HookFunc(Plant_IsUpgradeAddr, &Plant::IsUpgrade, &old_Plant_IsUpgrade);
    homura::HookFunc(Plant_PlantInitializeAddr, &Plant::PlantInitialize, &old_Plant_PlantInitialize);
    homura::HookFunc(Plant_IsNocturnalAddr, &Plant::IsNocturnal, nullptr);
    homura::HookFunc(Plant_SetSleepingAddr, &Plant::SetSleeping, &old_Plant_SetSleeping);
    homura::HookFunc(Plant_UpdateReanimColorAddr, &Plant::UpdateReanimColor, &old_Plant_UpdateReanimColor);
    homura::HookFunc(Plant_FindTargetZombieAddr, &Plant::FindTargetZombie, nullptr);
    homura::HookFunc(Plant_FindTargetGridItemAddr, &Plant::FindTargetGridItem, nullptr);
    homura::HookFunc(Plant_GetCostAddr, &Plant::GetCost, &old_Plant_GetCost);
    homura::HookFunc(Plant_DieAddr, &Plant::Die, nullptr);
    homura::HookFunc(GetPlantDefinitionAddr, &GetPlantDefinition, nullptr);
    homura::HookFunc(Plant_PlayBodyReanimAddr, &Plant::PlayBodyReanim, &old_Plant_PlayBodyReanim);
    homura::HookFunc(Plant_UpdateProductionPlantAddr, &Plant::UpdateProductionPlant, &old_Plant_UpdateProductionPlant);
    homura::HookFunc(Plant_UpdateShootingAddr, &Plant::UpdateShooting, nullptr);
    homura::HookFunc(Plant_FireAddr, &Plant::Fire, nullptr);
    homura::HookFunc(Plant_DoRowAreaDamageAddr, &Plant::DoRowAreaDamage, nullptr);
    //    homura::HookFunc(Plant_UpdateShootingAddr, &Plant::UpdateShooting, nullptr);
    homura::HookFunc(Plant_UpdateShooterAddr, &Plant::UpdateShooter, nullptr);
    homura::HookFunc(Plant_IceZombiesAddr, &Plant::IceZombies, nullptr);
    homura::HookFunc(Plant_FindTargetAndFireAddr, &Plant::FindTargetAndFire, &old_Plant_FindTargetAndFire);
    homura::HookFunc(Plant_UpdateChomperAddr, &Plant::UpdateChomper, nullptr);
    homura::HookFunc(Plant_UpdateGraveBusterAddr, &Plant::UpdateGraveBuster, nullptr);
    homura::HookFunc(Plant_UpdateMagnetShroomAddr, &Plant::UpdateMagnetShroom, nullptr);
    homura::HookFunc(Plant_UpdateSquashAddr, &Plant::UpdateSquash, nullptr);
    homura::HookFunc(Plant_CobCannonFireAddr, &Plant::CobCannonFire, nullptr);
    // homura::HookFunc(Plant_UpdateReanimAddr, Plant_UpdateReanim, &old_Plant_UpdateReanim);


    homura::HookFunc(Projectile_ProjectileInitializeAddr, &Projectile::ProjectileInitialize, &old_Projectile_ProjectileInitialize);
    homura::HookFunc(Projectile_ConvertToFireballAddr, &Projectile::ConvertToFireball, nullptr);
    homura::HookFunc(Projectile_ConvertToPeaAddr, &Projectile::ConvertToPea, &old_Projectile_ConvertToPea);
    homura::HookFunc(Projectile_UpdateAddr, &Projectile::Update, nullptr);
    homura::HookFunc(Projectile_UpdateMotionAddr, &Projectile::UpdateMotion, nullptr);
    homura::HookFunc(Projectile_UpdateNormalMotionAddr, &Projectile::UpdateNormalMotion, &old_Projectile_UpdateNormalMotion);
    homura::HookFunc(Projectile_UpdateLobMotionAddr, &Projectile::UpdateLobMotion, nullptr);
    homura::HookFunc(Projectile_DoImpactAddr, &Projectile::DoImpact, nullptr);
    homura::HookFunc(Projectile_DoImpactGridItemAddr, &Projectile::DoImpactGridItem, nullptr);
    homura::HookFunc(Projectile_DoSplashDamageAddr, &Projectile::DoSplashDamage, nullptr);
    homura::HookFunc(Projectile_CheckForCollisionAddr, &Projectile::CheckForCollision, nullptr);
    homura::HookFunc(Projectile_GetProjectileDefAddr, &Projectile::GetProjectileDef, nullptr);
    homura::HookFunc(Projectile_DrawAddr, &Projectile::Draw, &old_Projectile_Draw);
    homura::HookFunc(Projectile_DrawShadowAddr, &Projectile::DrawShadow, &old_Projectile_DrawShadow);
    homura::HookFunc(Projectile_FindCollisionTargetGridItemAddr, &Projectile::FindCollisionTargetGridItem, nullptr);
    homura::HookFunc(Projectile_FindCollisionTargetAddr, &Projectile::FindCollisionTarget, nullptr);


    homura::HookFunc(SeedPacket_UpdateAddr, &SeedPacket::Update, nullptr);
    homura::HookFunc(SeedPacket_UpdateSelectedAddr, &SeedPacket::UpdateSelected, &old_SeedPacket_UpdateSelected);
    homura::HookFunc(SeedPacket_DrawOverlayAddr, &SeedPacket::DrawOverlay, &old_SeedPacket_DrawOverlay);
    homura::HookFunc(SeedPacket_DrawAddr, &SeedPacket::Draw, &old_SeedPacket_Draw);
    homura::HookFunc(SeedPacket_FlashIfReadyAddr, &SeedPacket::FlashIfReady, &old_SeedPacket_FlashIfReady);
    homura::HookFunc(SeedPacket_SetPacketTypeAddr, &SeedPacket::SetPacketType, &old_SeedPacket_SetPacketType);
    //    homura::HookFunc(SeedPacket_MouseDownAddr, &SeedPacket::MouseDown, &old_SeedPacket_MouseDown);
    homura::HookFunc(SeedPacket_WasPlantedAddr, &SeedPacket::WasPlanted, &old_SeedPacket_WasPlanted);
    homura::HookFunc(SeedPacket_SlotMachineStartAddr, &SeedPacket::SlotMachineStart, nullptr);


    homura::HookFunc(ToolTipWidget_DrawAddr, &ToolTipWidget::Draw, &old_ToolTipWidget_Draw);
    homura::HookFunc(ToolTipWidget_CalculateSizeAddr, &ToolTipWidget::CalculateSize, nullptr);


    homura::HookFunc(Zombie_UpdateAddr, &Zombie::Update, nullptr);
    homura::HookFunc(Zombie_UpdateActionsAddr, &Zombie::UpdateActions, &old_Zombie_UpdateActions);
    homura::HookFunc(Zombie_UpdatePlayingAddr, &Zombie::UpdatePlaying, nullptr);
    homura::HookFunc(Zombie_UpdateYetiAddr, &Zombie::UpdateYeti, nullptr);
    homura::HookFunc(Zombie_UpdateZombieFlyerAddr, &Zombie::UpdateZombieFlyer, nullptr);
    homura::HookFunc(Zombie_UpdateZombieImpAddr, &Zombie::UpdateZombieImp, nullptr);
    homura::HookFunc(Zombie_UpdateZombieJackInTheBoxAddr, &Zombie::UpdateZombieJackInTheBox, nullptr);
    homura::HookFunc(Zombie_UpdateZombiePolevaulterAddr, &Zombie::UpdateZombiePolevaulter, nullptr);
    homura::HookFunc(Zombie_UpdateZombieDolphinRiderAddr, &Zombie::UpdateZombieDolphinRider, nullptr);
    homura::HookFunc(Zombie_UpdateZombieGargantuarAddr, &Zombie::UpdateZombieGargantuar, nullptr);
    homura::HookFunc(Zombie_UpdateZombiePeaHeadAddr, &Zombie::UpdateZombiePeaHead, nullptr);
    homura::HookFunc(Zombie_UpdateZombieGatlingHeadAddr, &Zombie::UpdateZombieGatlingHead, nullptr);
    homura::HookFunc(Zombie_UpdateZombieJalapenoHeadAddr, &Zombie::UpdateZombieJalapenoHead, nullptr);
    homura::HookFunc(Zombie_UpdateZombieSquashHeadAddr, &Zombie::UpdateZombieSquashHead, nullptr);
    homura::HookFunc(Zombie_UpdateZombieDancerAddr, &Zombie::UpdateZombieDancer, nullptr);
    homura::HookFunc(Zombie_UpdateZombieBobsledAddr, &Zombie::UpdateZombieBobsled, nullptr);
    homura::HookFunc(Zombie_UpdateZombieRiseFromGraveAddr, &Zombie::UpdateZombieRiseFromGrave, nullptr);
    homura::HookFunc(Zombie_GetDancerFrameAddr, &Zombie::GetDancerFrame, nullptr);
    homura::HookFunc(Zombie_RiseFromGraveAddr, &Zombie::RiseFromGrave, &old_Zombie_RiseFromGrave);
    homura::HookFunc(Zombie_EatPlantAddr, &Zombie::EatPlant, nullptr);
    homura::HookFunc(Zombie_EatZombieAddr, &Zombie::EatZombie, nullptr);
    homura::HookFunc(Zombie_StartEatingAddr, &Zombie::StartEating, nullptr);
    homura::HookFunc(Zombie_DetachShieldAddr, &Zombie::DetachShield, &old_Zombie_DetachShield);
    homura::HookFunc(Zombie_CheckForBoardEdgeAddr, &Zombie::CheckForBoardEdge, nullptr);
    homura::HookFunc(Zombie_DrawAddr, &Zombie::Draw, &old_Zombie_Draw);
    homura::HookFunc(Zombie_DrawShadowAddr, &Zombie::DrawShadow, nullptr);
    homura::HookFunc(Zombie_DrawBossPartAddr, &Zombie::DrawBossPart, &old_Zombie_DrawBossPart);
    homura::HookFunc(ZombieTypeCanGoInPoolAddr, &Zombie::ZombieTypeCanGoInPool, nullptr);
    homura::HookFunc(Zombie_BossSpawnAttackAddr, &Zombie::BossSpawnAttack, nullptr);
    homura::HookFunc(Zombie_DrawBungeeCordAddr, &Zombie::DrawBungeeCord, nullptr);
    homura::HookFunc(Zombie_IsTangleKelpTargetAddr, &Zombie::IsTangleKelpTarget, nullptr);
    homura::HookFunc(Zombie_IsTangleKelpTarget2Addr, &Zombie::IsTangleKelpTarget, nullptr);
    homura::HookFunc(Zombie_DrawReanimAddr, &Zombie::DrawReanim, nullptr);
    homura::HookFunc(Zombie_DropHeadAddr, &Zombie::DropHead, &old_Zombie_DropHead);
    homura::HookFunc(Zombie_DropHelmAddr, &Zombie::DropHelm, nullptr);
    homura::HookFunc(Zombie_DropShieldAddr, &Zombie::DropShield, nullptr);
    homura::HookFunc(Zombie_DropArmAddr, &Zombie::DropArm, &old_Zombie_DropArm);
    homura::HookFunc(Zombie_ZombieInitializeAddr, &Zombie::ZombieInitialize, &old_Zombie_ZombieInitialize);
    homura::HookFunc(Zombie_DieNoLootAddr, &Zombie::DieNoLoot, &old_Zombie_DieNoLoot);
    homura::HookFunc(Zombie_StopZombieSoundAddr, &Zombie::StopZombieSound, nullptr);
    homura::HookFunc(GetZombieDefinitionAddr, &GetZombieDefinition, nullptr);
    homura::HookFunc(Zombie_FindPlantTargetAddr, &Zombie::FindPlantTarget, nullptr);
    homura::HookFunc(Zombie_FindZombieTargetAddr, &Zombie::FindZombieTarget, nullptr);
    homura::HookFunc(Zombie_TakeDamageAddr, &Zombie::TakeDamage, nullptr);
    homura::HookFunc(Zombie_TakeHelmDamageAddr, &Zombie::TakeHelmDamage, &old_Zombie_TakeHelmDamage);
    homura::HookFunc(Zombie_TakeFlyingDamageAddr, &Zombie::TakeFlyingDamage, nullptr);
    homura::HookFunc(Zombie_TakeShieldDamageAddr, &Zombie::TakeShieldDamage, nullptr);
    homura::HookFunc(Zombie_AttachShieldAddr, &Zombie::AttachShield, nullptr);
    homura::HookFunc(Zombie_PlayZombieReanimAddr, &Zombie::PlayZombieReanim, &old_Zombie_PlayZombieReanim);
    homura::HookFunc(Zombie_StartWalkAnimAddr, &Zombie::StartWalkAnim, nullptr);
    homura::HookFunc(Zombie_ReanimShowPrefixAddr, &Zombie::ReanimShowPrefix, &old_Zombie_ReanimShowPrefix);
    homura::HookFunc(Zombie_ReanimShowTrackAddr, &Zombie::ReanimShowTrack, &old_Zombie_ReanimShowTrack);
    homura::HookFunc(Zombie_GetPosYBasedOnRowAddr, &Zombie::GetPosYBasedOnRow, &old_Zombie_GetPosYBasedOnRow);
    homura::HookFunc(Zombie_SetRowAddr, &Zombie::SetRow, &old_Zombie_SetRow);
    homura::HookFunc(Zombie_StartMindControlledAddr, &Zombie::StartMindControlled, nullptr);
    homura::HookFunc(Zombie_UpdateReanimAddr, &Zombie::UpdateReanim, &old_Zombie_UpdateReanim);
    homura::HookFunc(Zombie_GetBobsledPositionAddr, &Zombie::GetBobsledPosition, &old_Zombie_GetBobsledPosition);
    homura::HookFunc(Zombie_SquishAllInSquareAddr, &Zombie::SquishAllInSquare, &old_Zombie_SquishAllInSquare);
    homura::HookFunc(Zombie_StopEatingAddr, &Zombie::StopEating, nullptr);
    homura::HookFunc(Zombie_BungeeDropZombieAddr, &Zombie::BungeeDropZombie, nullptr);
    homura::HookFunc(Zombie_PickRandomSpeedAddr, &Zombie::PickRandomSpeed, nullptr);
    homura::HookFunc(Zombie_UpdateAnimSpeedAddr, &Zombie::UpdateAnimSpeed, nullptr);
    homura::HookFunc(Zombie_AnimateAddr, &Zombie::Animate, nullptr);
    homura::HookFunc(Zombie_UpdateZombieWalkingAddr, &Zombie::UpdateZombieWalking, nullptr);
    homura::HookFunc(Zombie_UpdateDamageStatesAddr, &Zombie::UpdateDamageStates, nullptr);
    homura::HookFunc(Zombie_PlayDeathAnimAddr, &Zombie::PlayDeathAnim, nullptr);
    homura::HookFunc(Zombie_DropLootAddr, &Zombie::DropLoot, &old_Zombie_DropLoot);
    homura::HookFunc(Zombie_ApplyBurnAddr, &Zombie::ApplyBurn, &old_Zombie_ApplyBurn);
    homura::HookFunc(Zombie_ApplyButterAddr, &Zombie::ApplyButter, nullptr);
    homura::HookFunc(Zombie_ApplyChillAddr, &Zombie::ApplyChill, nullptr);
    homura::HookFunc(Zombie_CheckIfPreyCaughtAddr, &Zombie::CheckIfPreyCaught, nullptr);
    homura::HookFunc(Zombie_CanTargetPlantAddr, &Zombie::CanTargetPlant, nullptr);
    homura::HookFunc(Zombie_HitIceTrapAddr, &Zombie::HitIceTrap, nullptr);
    homura::HookFunc(Zombie_HasYuckyFaceImageAddr, &Zombie::HasYuckyFaceImage, nullptr);
    homura::HookFunc(Zombie_UpdateYuckyFaceAddr, &Zombie::UpdateYuckyFace, &old_Zombie_UpdateYuckyFace);
    homura::HookFunc(Zombie_UpdateZombiePoolAddr, &Zombie::UpdateZombiePool, nullptr);
    homura::HookFunc(Zombie_SummonBackupDancersAddr, &Zombie::SummonBackupDancers, nullptr);
    homura::HookFunc(Zombie_UpdateZombieBungeeAddr, &Zombie::UpdateZombieBungee, nullptr);
    homura::HookFunc(Zombie_UpdateZombiePogoAddr, &Zombie::UpdateZombiePogo, nullptr);
    homura::HookFunc(Zombie_UpdateZombieCatapultAddr, &Zombie::UpdateZombieCatapult, nullptr);
    homura::HookFunc(Zombie_BungeeLandingAddr, &Zombie::BungeeLanding, nullptr);
    homura::HookFunc(Zombie_UpdateLadderAddr, &Zombie::UpdateLadder, nullptr);
    homura::HookFunc(Zombie_GetDrawPosAddr, &Zombie::GetDrawPos, nullptr);
    homura::HookFunc(Zombie_DrawIceTrapAddr, &Zombie::DrawIceTrap, nullptr);
    homura::HookFunc(Zombie_DrawButterAddr, &Zombie::DrawButter, nullptr);
    homura::HookFunc(Zombie_ZombieNotWalkingAddr, &Zombie::ZombieNotWalking, nullptr);
    homura::HookFunc(Zombie_MowDownAddr, &Zombie::MowDown, nullptr);
    homura::HookFunc(Zombie_UpdateDeathAddr, &Zombie::UpdateDeath, nullptr);
    homura::HookFunc(Zombie_WalkIntoHouseAddr, &Zombie::WalkIntoHouse, nullptr);


    homura::HookFunc(Sexy_Dialog_AddedToManagerWidgetManagerAddr, &Sexy::Dialog::AddedToManager, &old_Sexy_Dialog_AddedToManager);
    homura::HookFunc(Sexy_Dialog_RemovedFromManagerAddr, &Sexy::Dialog::RemovedFromManager, &old_Sexy_Dialog_RemovedFromManager);


    homura::HookFunc(SeedBank_DrawAddr, &SeedBank::Draw, &old_SeedBank_Draw);
    homura::HookFunc(SeedBank_MouseHitTestAddr, &SeedBank::MouseHitTest, nullptr);
    // homura::HookFunc(SeedBank_SeedBankAddr, &SeedBank::Create, &old_SeedBank_SeedBank);
    // homura::HookFunc(SeedBank_UpdateWidthAddr, &SeedBank::UpdateWidth, &old_SeedBank_UpdateWidth);
    homura::HookFunc(SeedBank_MoveAddr, &SeedBank::Move, nullptr);
    homura::HookFunc(SeedBank_AddSeedAddr, &SeedBank::AddSeed, &old_SeedBank_AddSeed);


    homura::HookFunc(AwardScreen_MouseDownAddr, &AwardScreen::MouseDown, &old_AwardScreen_MouseDown);
    homura::HookFunc(AwardScreen_MouseUpAddr, &AwardScreen::MouseUp, &old_AwardScreen_MouseUp);


    homura::HookFunc(VSSetupMenu_VSSetupMenuAddr, &VSSetupMenu::_constructor, &old_VSSetupMenu_Constructor);
    homura::HookFunc(VSSetupMenu_Delete2Addr, &VSSetupMenu::_destructor, &old_VSSetupMenu_Destructor);
    homura::HookFunc(VSSetupMenu_DrawAddr, &VSSetupMenu::Draw, &old_VSSetupMenu_Draw);
    homura::HookFunc(VSSetupMenu_DrawOverlayAddr, &VSSetupMenu::DrawOverlay, &old_VSSetupMenu_DrawOverlay);
    homura::HookFunc(VSSetupMenu_AddedToManagerAddr, &VSSetupMenu::AddedToManager, &old_VSSetupMenu_AddedToManager);
    homura::HookFunc(VSSetupMenu_CloseVSSetupAddr, &VSSetupMenu::CloseVSSetup, &old_VSSetupMenu_CloseVSSetup);
    homura::HookFunc(VSSetupMenu_UpdateAddr, &VSSetupMenu::Update, &old_VSSetupMenu_Update);
    homura::HookFunc(VSSetupMenu_KeyDownAddr, &VSSetupMenu::KeyDown, &old_VSSetupMenu_KeyDown);
    homura::HookFunc(VSSetupMenu_OnStateEnterAddr, &VSSetupMenu::OnStateEnter, &old_VSSetupMenu_OnStateEnter);
    // homura::HookFunc(VSSetupMenu_ButtonPressAddr, &VSSetupMenu::ButtonPress, &old_VSSetupMenu_ButtonPress);
    homura::HookFunc(VSSetupMenu_ButtonDepressAddr, &VSSetupMenu::ButtonDepress, &old_VSSetupMenu_ButtonDepress);
    homura::HookFunc(VSSetupMenu_PickRandomZombiesAddr, &VSSetupMenu::PickRandomZombies, nullptr);
    homura::HookFunc(VSSetupMenu_PickRandomPlantsAddr, &VSSetupMenu::PickRandomPlants, nullptr);


    homura::HookFunc(VSResultsMenu__constructorAddr, &VSResultsMenu::_constructor, &old_VSResultsMenu_Constructor);
    homura::HookFunc(VSResultsMenu__destructorAddr, &VSResultsMenu::_destructor, &old_VSResultsMenu_Destructor);
    homura::HookFunc(VSResultsMenu_AddedToManagerAddr, &VSResultsMenu::AddedToManager, &old_VSResultsMenu_AddedToManager);
    homura::HookFunc(VSResultsMenu_UpdateAddr, &VSResultsMenu::Update, &old_VSResultsMenu_Update);
    homura::HookFunc(VSResultsMenu_OnExitAddr, &VSResultsMenu::OnExit, nullptr);
    homura::HookFunc(VSResultsMenu_DrawAddr, &VSResultsMenu::Draw, &old_VSResultsMenu_Draw);
    homura::HookFunc(VSResultsMenu_DrawInfoBoxAddr, &VSResultsMenu::DrawInfoBox, &old_VSResultsMenu_DrawInfoBox);
    homura::HookFunc(VSResultsMenu_ButtonDepressAddr, &VSResultsMenu::ButtonDepress, nullptr);
    homura::HookFunc(VSResultsMenu_InitFromBoardAddr, &VSResultsMenu::InitFromBoard, nullptr);
    homura::HookFunc(VSResultsMenu_ClearPlayerRecordsAddr, &VSResultsMenu::ClearPlayerRecords, &old_VSResultsMenu_ClearPlayerRecords);


    homura::HookFunc(ImitaterDialog_ImitaterDialogAddr, &ImitaterDialog::_constructor, &old_ImitaterDialog_ImitaterDialog);
    homura::HookFunc(ImitaterDialog_MouseDownAddr, &ImitaterDialog::MouseDown, &old_ImitaterDialog_MouseDown);
    // homura::HookFunc(ImitaterDialog_OnKeyDownAddr, ImitaterDialog_OnKeyDown, &old_ImitaterDialog_OnKeyDown);
    homura::HookFunc(ImitaterDialog_KeyDownAddr, &ImitaterDialog::KeyDown, &old_ImitaterDialog_KeyDown);
    homura::HookFunc(ImitaterDialog_ShowToolTipAddr, &ImitaterDialog::ShowToolTip, &old_ImitaterDialog_ShowToolTip);


    homura::HookFunc(MailScreen_MailScreenAddr, &MailScreen::_constructor, &old_MailScreen_MailScreen);
    homura::HookFunc(MailScreen_AddedToManagerAddr, &MailScreen::AddedToManager, &old_MailScreen_AddedToManager);
    homura::HookFunc(MailScreen_RemovedFromManagerAddr, &MailScreen::RemovedFromManager, &old_MailScreen_RemovedFromManager);
    homura::HookFunc(MailScreen_Delete2Addr, &MailScreen::_destructor2, &old_MailScreen_Delete2);
    homura::HookFunc(Mailbox_GetNumUnseenMessagesAddr, &Mailbox::GetNumUnseenMessages, nullptr);


    homura::HookFunc(ZenGardenControls_UpdateAddr, &ZenGardenControls::Update, &old_ZenGardenControls_Update);
    homura::HookFunc(ZenGarden_DrawBackdropAddr, &ZenGarden::DrawBackdrop, &old_ZenGarden_DrawBackdrop);
    // homura::HookFunc(ZenGarden_MouseDownWithFeedingToolAddr, ZenGarden_MouseDownWithFeedingTool, &old_ZenGarden_MouseDownWithFeedingTool);
    // homura::HookFunc(ZenGarden_DrawPottedPlantAddr, ZenGarden_DrawPottedPlant, nullptr);

    // homura::HookFunc( Sexy_GamepadApp_CheckGamepadAddr, Sexy_GamepadApp_CheckGamepad,nullptr);
    // homura::HookFunc( Sexy_GamepadApp_HasGamepadAddr, Sexy_GamepadApp_HasGamepad,nullptr);


    homura::HookFunc(CutScene_ShowShovelAddr, &CutScene::ShowShovel, &old_CutScene_ShowShovel);
    homura::HookFunc(CutScene_UpdateAddr, &CutScene::Update, &old_CutScene_Update);
    homura::HookFunc(CutScene_PlaceLawnItemsAddr, &CutScene::PlaceLawnItems, &old_CutScene_PlaceLawnItems);
    homura::HookFunc(CutScene_PlaceStreetZombiesAddr, &CutScene::PlaceStreetZombies, nullptr);
    homura::HookFunc(CutScene_AddFlowerPotsAddr, &CutScene::AddFlowerPots, &old_CutScene_AddFlowerPots);
    homura::HookFunc(CutScene_LoadUpsellChallengeScreenAddr, &CutScene::LoadUpsellChallengeScreen, nullptr);
    homura::HookFunc(CutScene_EndSeedChooserAddr, &CutScene::EndSeedChooser, nullptr);
    homura::HookFunc(CutScene_ClearUpsellBoardAddr, &CutScene::ClearUpsellBoard, nullptr);


    homura::HookFunc(NewOptionsDialog_ButtonDepressAddr, &NewOptionsDialog::ButtonDepress, &old_NewOptionsDialog_ButtonDepress);
    homura::HookFunc(NewOptionsDialog_AddedToManagerAddr, &NewOptionsDialog::AddedToManager, &old_NewOptionsDialog_AddedToManager);
    homura::HookFunc(NewOptionsDialog_RemovedFromManagerAddr, &NewOptionsDialog::RemovedFromManager, &old_NewOptionsDialog_RemovedFromManager);

    homura::HookFunc(BaseGamepadControls_GetGamepadVelocityAddr, &BaseGamepadControls::GetGamepadVelocity, nullptr);

    homura::HookFunc(LookupFoleyAddr, &LookupFoley, nullptr);
    homura::HookFunc(TodFoley_RehookupSoundWithMusicVolumeAddr, &TodFoley::RehookupSoundWithMusicVolume, nullptr);
    homura::HookFunc(SoundSystemReleaseFinishedInstancesAddr, &SoundSystemReleaseFinishedInstances, nullptr);

    // homura::HookFunc(TodDrawStringWrappedHelperAddr, TodDrawStringWrappedHelper, &old_TodDrawStringWrappedHelper);
    homura::HookFunc(MessageWidget_ClearLabelAddr, &CustomMessageWidget::ClearLabel, &old_MessageWidget_ClearLabel);
    homura::HookFunc(MessageWidget_SetLabelAddr, &CustomMessageWidget::SetLabel, &old_MessageWidget_SetLabel);
    homura::HookFunc(MessageWidget_UpdateAddr, &CustomMessageWidget::Update, &old_MessageWidget_Update);
    homura::HookFunc(MessageWidget_DrawAddr, &CustomMessageWidget::Draw, &old_MessageWidget_Draw);

    homura::HookFunc(Sexy_ExtractLoadingSoundsResourcesAddr, &Sexy::ExtractLoadingSoundsResources, &old_Sexy_ExtractLoadingSoundsResources);
    homura::HookFunc(Sexy_EditWidget_ProcessKeyAddr, &Sexy::EditWidget::ProcessKey, &old_Sexy_EditWidget_ProcessKey);
    // homura::HookFunc(Sexy_ScrollbarWidget_MouseDownAddr, Sexy_ScrollbarWidget_MouseDown, nullptr);

    homura::HookFunc(CustomScrollbarWidget_RemoveScrollButtonsAddr, &Sexy::CustomScrollbarWidget::RemoveScrollButtons, nullptr);

    homura::HookFunc(CreditScreen_CreditScreenAddr, &CreditScreen::_constructor, &old_CreditScreen__constructor);
    homura::HookFunc(CreditScreen_Delete2Addr, &CreditScreen::_destructor, &old_CreditScreen__destructor);
    homura::HookFunc(CreditScreen_AddedToManagerAddr, &CreditScreen::AddedToManager, &old_CreditScreen_AddedToManager);
    homura::HookFunc(CreditScreen_RemovedFromManagerAddr, &CreditScreen::RemovedFromManager, &old_CreditScreen_RemovedFromManager);

    homura::HookFunc(HelpOptionsDialog_AddedToManagerAddr, &HelpOptionsDialog_AddedToManager, &old_HelpOptionsDialog_AddedToManager);
    homura::HookFunc(HelpOptionsDialog_RemovedFromManagerAddr, &HelpOptionsDialog_RemovedFromManager, &old_HelpOptionsDialog_RemovedFromManager);
    homura::HookFunc(HelpOptionsDialog_ButtonDepressAddr, &HelpOptionsDialog_ButtonDepress, &old_HelpOptionsDialog_ButtonDepress);
    homura::HookFunc(HelpOptionsDialog_HelpOptionsDialogAddr, &HelpOptionsDialog_HelpOptionsDialog, &old_HelpOptionsDialog_HelpOptionsDialog);
    homura::HookFunc(HelpOptionsDialog_ResizeAddr, &HelpOptionsDialog_Resize, &old_HelpOptionsDialog_Resize);

    homura::HookFunc(HelpTextScreen_AddedToManagerAddr, &HelpTextScreen::AddedToManager, &old_HelpTextScreen_AddedToManager);
    homura::HookFunc(HelpTextScreen_RemovedFromManagerAddr, &HelpTextScreen::RemovedFromManager, &old_HelpTextScreen_RemovedFromManager);
    homura::HookFunc(HelpTextScreen__constructorAddr, &HelpTextScreen::_constructor, &old_HelpTextScreen__constructor);
    homura::HookFunc(HelpTextScreen__destructorAddr, &HelpTextScreen::_destructor, &old_HelpTextScreen__destructor);
    homura::HookFunc(HelpTextScreen_UpdateAddr, &HelpTextScreen::Update, &old_HelpTextScreen_Update);

    homura::HookFunc(WaitForSecondPlayerDialog_WaitForSecondPlayerDialogAddr, &WaitForSecondPlayerDialog::_constructor, &old_WaitForSecondPlayerDialog_WaitForSecondPlayerDialog);
    homura::HookFunc(WaitForSecondPlayerDialog__destructorAddr, &WaitForSecondPlayerDialog::_destructor, &old_WaitForSecondPlayerDialog__destructorAddr);
    homura::HookFunc(WaitForSecondPlayerDialog__destructor2Addr, &WaitForSecondPlayerDialog::_destructor2, &old_WaitForSecondPlayerDialog__destructor2Addr);


    homura::HookFunc(Sexy_WidgetManager_MouseDownAddr, &Sexy::WidgetManager::MouseDown, &old_Sexy_WidgetManager_MouseDown);
    homura::HookFunc(Sexy_WidgetManager_MouseDragAddr, &Sexy::WidgetManager::MouseDrag, &old_Sexy_WidgetManager_MouseDrag);
    homura::HookFunc(Sexy_WidgetManager_MouseUpAddr, &Sexy::WidgetManager::MouseUp, &old_Sexy_WidgetManager_MouseUp);
    // homura::HookFunc(Sexy_WidgetManager_AxisMovedAddr, Sexy_WidgetManager_AxisMoved, nullptr);


    homura::HookFunc(LawnMower_UpdateAddr, &LawnMower::Update, &old_LawnMower_Update);
    homura::HookFunc(LawnMower_StartMowerAddr, &LawnMower::StartMower, &old_LawnMower_StartMower);
    homura::HookFunc(ConfirmBackToMainDialog_ButtonDepressAddr, &ConfirmBackToMainDialog_ButtonDepress, &old_ConfirmBackToMainDialog_ButtonDepress);
    homura::HookFunc(ConfirmBackToMainDialog_AddedToManagerAddr, &ConfirmBackToMainDialog_AddedToManager, &old_ConfirmBackToMainDialog_AddedToManager);
    homura::HookFunc(ConfirmBackToMainDialog_RemovedFromManagerAddr, &ConfirmBackToMainDialog_RemovedFromManager, &old_ConfirmBackToMainDialog_RemovedFromManager);
    // homura::HookFunc(FilterEffectDisposeForAppAddr, FilterEffectDisposeForApp, nullptr);
    // homura::HookFunc(FilterEffectGetImageAddr, FilterEffectGetImage, nullptr);
    homura::HookFunc(Reanimation_DrawTrackAddr, &Reanimation::DrawTrack, &old_Reanimation_DrawTrack);


    //    homura::HookFunc(ReanimatorCache_ReanimatorCacheInitializeAddr, &ReanimatorCache::ReanimatorCacheInitialize, nullptr);
    homura::HookFunc(ReanimatorCache_ReanimatorCacheDisposeAddr, &ReanimatorCache::ReanimatorCacheDispose, nullptr);
    homura::HookFunc(ReanimatorCache_DrawCachedPlantAddr, &ReanimatorCache::DrawCachedPlant, &old_ReanimatorCache_DrawCachedPlant);
    homura::HookFunc(ReanimatorCache_UpdateReanimationForVariationAddr, &ReanimatorCache::UpdateReanimationForVariation, &old_ReanimatorCache_UpdateReanimationForVariation);
    homura::HookFunc(ReanimatorCache_LoadCachedImagesAddr, &ReanimatorCache::LoadCachedImages, &old_ReanimatorCache_LoadCachedImages);
    homura::HookFunc(ReanimatorCache_MakeCachedZombieFrameAddr, &ReanimatorCache::MakeCachedZombieFrame, nullptr);
    //    homura::HookFunc(ReanimatorCache_DrawCachedZombieAddr, &ReanimatorCache::DrawCachedZombie, nullptr);


    homura::HookFunc(HelpBarWidget_HelpBarWidgetAddr, &HelpBarWidget::_constructor, &old_HelpBarWidget_HelpBarWidget);


    homura::HookFunc(DrawSeedTypeAddr, &DrawSeedType, nullptr);
    homura::HookFunc(DrawSeedPacketAddr, &DrawSeedPacket, nullptr);

    homura::HookFunc(Music_PlayMusicAddr, &Music::PlayMusic, nullptr);
    homura::HookFunc(Music_MusicUpdateAddr, &Music::MusicUpdate, nullptr);
    homura::HookFunc(Music_MusicTitleScreenInitAddr, &Music::MusicTitleScreenInit, nullptr);
    homura::HookFunc(Music_UpdateMusicBurstAddr, &Music::UpdateMusicBurst, &old_Music_UpdateMusicBurst);
    homura::HookFunc(Music2_Music2Addr, &Music2::_constructor, &old_Music2_Music2);

    homura::HookFunc(LawnPlayerInfo_AddCoinsAddr, &LawnPlayerInfo::AddCoins, nullptr);
    homura::HookFunc(MaskHelpWidget_UpdateAddr, &AchievementsWidget::Update, nullptr);
    homura::HookFunc(MaskHelpWidget_DrawAddr, &AchievementsWidget::Draw, nullptr);

    homura::HookFunc(DaveHelp__destructorAddr, &LeaderboardsWidget::_destructor, nullptr);
    homura::HookFunc(DaveHelp_UpdateAddr, &LeaderboardsWidget::Update, nullptr);
    homura::HookFunc(DaveHelp_DrawAddr, &LeaderboardsWidget::Draw, nullptr);
    homura::HookFunc(DaveHelp_DealClickAddr, &LeaderboardsWidget::DealClick, nullptr);

    homura::HookFunc(TrashBin_TrashBinAddr, &TrashBin::_constructor, &old_TrashBin_TrashBin);
    homura::HookFunc(Sexy_SexyAppBase_Is3DAcceleratedAddr, &LawnApp::Is3DAccelerated, nullptr);
    homura::HookFunc(Sexy_SexyAppBase_SexyAppBaseAddr, &Sexy::SexyAppBase::_constructor, &old_Sexy_SexyAppBase_SexyAppBase);
    homura::HookFunc(Sexy_SexyAppBase_EraseFileAddr, &Sexy::SexyAppBase::EraseFile, nullptr);

    homura::HookFunc(SettingsDialog_SettingsDialogAddr, &SettingsDialog::_constructor, &old_SettingsDialog__constructor);
    homura::HookFunc(SettingsDialog__destructorAddr, &SettingsDialog::_destructor, &old_SettingsDialog__destructor);
    homura::HookFunc(SettingsDialog_AddedToManagerAddr, &SettingsDialog::AddedToManager, &old_SettingsDialog_AddedToManager);
    homura::HookFunc(SettingsDialog_RemovedFromManagerAddr, &SettingsDialog::RemovedFromManager, &old_SettingsDialog_RemovedFromManager);
    homura::HookFunc(SettingsDialog_DrawAddr, &SettingsDialog::Draw, &old_SettingsDialog_Draw);
    homura::HookFunc(ReanimatorLoadDefinitionsAddr, &ReanimatorLoadDefinitions, &old_ReanimatorLoadDefinitions);
    homura::HookFunc(DefinitionGetCompiledFilePathFromXMLFilePathAddr, &DefinitionGetCompiledFilePathFromXMLFilePath, &old_DefinitionGetCompiledFilePathFromXMLFilePath);
    homura::HookFunc(SaveGameContext_SyncReanimationDefAddr, &SaveGameContext::SyncReanimationDef, nullptr);
    homura::HookFunc(SyncReanimationAddr, &SyncReanimation, nullptr);
    homura::HookFunc(PoolEffect_PoolEffectDrawAddr, &PoolEffect::PoolEffectDraw, nullptr);
    homura::HookFunc(Sexy_MemoryImage_ClearRectAddr, &Sexy::MemoryImage::ClearRect, nullptr);


    homura::HookFunc(TitleScreen_TitleScreenAddr, &TitleScreen::_constructor, &old_TitleScreen_TitleScreen);
    homura::HookFunc(TitleScreen_DrawAddr, &TitleScreen::Draw, &old_TitleScreen_Draw);
    homura::HookFunc(TitleScreen_UpdateAddr, &TitleScreen::Update, &old_TitleScreen_Update);
    homura::HookFunc(TitleScreen_SwitchStateAddr, &TitleScreen::SwitchState, nullptr);
    homura::HookFunc(TitleScreen__destructorAddr, &TitleScreen::_destructor, nullptr);
    homura::HookFunc(TitleScreen_ButtonPressAddr, &TitleScreen::ButtonPress, nullptr);
    homura::HookFunc(TitleScreen_VideoCompletedAddr, &TitleScreen::VideoCompleted, nullptr);
}

void InitVTableHookFunction() {
    homura::HookVirtualFunc(vTableForCursorObjectAddr, 4, &CursorObject::BeginDraw, &old_CursorObject_BeginDraw);
    homura::HookVirtualFunc(vTableForCursorObjectAddr, 5, &CursorObject::EndDraw, &old_CursorObject_EndDraw);

    homura::HookVirtualFunc(vTableForBoardAddr, 31, &Board::AddedToManager, &old_Board_AddedToManager);
    homura::HookVirtualFunc(vTableForBoardAddr, 77, &Board::MouseMove, &old_Board_MouseMove);
    homura::HookVirtualFunc(vTableForBoardAddr, 78, &Board::MouseDown, &old_Board_MouseDown);
    homura::HookVirtualFunc(vTableForBoardAddr, 81, &Board::MouseUp, &old_Board_MouseUp);
    homura::HookVirtualFunc(vTableForBoardAddr, 83, &Board::MouseDrag, &old_Board_MouseDrag);
    homura::HookVirtualFunc(vTableForBoardAddr, 133, &Board::ButtonDepress, &old_Board_ButtonDepress);

    homura::HookVirtualFunc(vTableForStoreScreenAddr, 78, &StoreScreen::MouseDown, &old_StoreScreen_MouseDown);
    homura::HookVirtualFunc(vTableForStoreScreenAddr, 81, &StoreScreen::MouseUp, &old_StoreScreen_MouseUp);
    // VTableHookFunction(vTableForStoreScreenAddr, 83, (void *) StoreScreen_MouseDrag,(void **) &old_StoreScreen_MouseDrag);

    homura::HookVirtualFunc(vTableForMailScreenAddr, 78, &MailScreen::MouseDown, &old_MailScreen_MouseDown);
    homura::HookVirtualFunc(vTableForMailScreenAddr, 81, &MailScreen::MouseUp, &old_MailScreen_MouseUp);
    homura::HookVirtualFunc(vTableForMailScreenAddr, 83, &MailScreen::MouseDrag, &old_MailScreen_MouseDrag);
    homura::HookVirtualFunc(vTableForMailScreenAddr, 140, &MailScreen::ButtonPress, &old_MailScreen_ButtonPress);
    homura::HookVirtualFunc(vTableForMailScreenAddr, 141, &MailScreen::ButtonDepress, &old_MailScreen_ButtonDepress);

    homura::HookVirtualFunc(vTableForChallengeScreenAddr, 78, &ChallengeScreen::MouseDown, &old_ChallengeScreen_MouseDown);
    homura::HookVirtualFunc(vTableForChallengeScreenAddr, 81, &ChallengeScreen::MouseUp, &old_ChallengeScreen_MouseUp);
    homura::HookVirtualFunc(vTableForChallengeScreenAddr, 83, &ChallengeScreen::MouseDrag, &old_ChallengeScreen_MouseDrag);
    homura::HookVirtualFunc(vTableForChallengeScreenAddr, 130, &ChallengeScreen::ButtonPress, nullptr);

    // homura::HookVirtualFunc(vTableForVSResultsMenuAddr, 78, &VSResultsMenu::MouseDown,nullptr);
    // homura::HookVirtualFunc(vTableForVSResultsMenuAddr, 81, &VSResultsMenu::MouseUp,nullptr);
    // homura::HookVirtualFunc(vTableForVSResultsMenuAddr, 83, &VSResultsMenu::MouseDrag,nullptr);
    homura::HookVirtualFunc(vTableForVSResultsMenuAddr, 32, &VSResultsMenu::RemovedFromManager, &old_VSResultsMenu_RemovedFromManager);


    homura::HookVirtualFunc(vTableForVSSetupMenuAddr, 78, &VSSetupMenu::MouseDown, nullptr);
    homura::HookVirtualFunc(vTableForVSSetupMenuAddr, 81, &VSSetupMenu::MouseUp, nullptr);
    homura::HookVirtualFunc(vTableForVSSetupMenuAddr, 83, &VSSetupMenu::MouseDrag, nullptr);

    homura::HookVirtualFunc(vTableForSeedChooserScreenAddr, 31, &SeedChooserScreen::AddedToManager, &old_SeedChooserScreen_AddedToManager);
    homura::HookVirtualFunc(vTableForSeedChooserScreenAddr, 32, &SeedChooserScreen::RemovedFromManager, &old_SeedChooserScreen_RemovedFromManager);
    homura::HookVirtualFunc(vTableForSeedChooserScreenAddr, 77, &SeedChooserScreen::MouseMove, &old_SeedChooserScreen_MouseMove);
    homura::HookVirtualFunc(vTableForSeedChooserScreenAddr, 78, &SeedChooserScreen::MouseDown, &old_SeedChooserScreen_MouseDown);
    homura::HookVirtualFunc(vTableForSeedChooserScreenAddr, 81, &SeedChooserScreen::MouseUp, &old_SeedChooserScreen_MouseUp);
    homura::HookVirtualFunc(vTableForSeedChooserScreenAddr, 83, &SeedChooserScreen::MouseDrag, &old_SeedChooserScreen_MouseDrag);
    homura::HookVirtualFunc(vTableForSeedChooserScreenAddr, 135, &SeedChooserScreen::ButtonPress, nullptr);


    homura::HookVirtualFunc(vTableForHelpTextScreenAddr, 38, &HelpTextScreen::Draw, &old_HelpTextScreen_Draw);
    homura::HookVirtualFunc(vTableForHelpTextScreenAddr, 78, &HelpTextScreen::MouseDown, &old_HelpTextScreen_MouseDown);
    // VTableHookFunction(vTableForHelpTextScreenAddr, 81, (void *) HelpTextScreen_MouseUp,(void **) &old_HelpTextScreen_MouseUp);
    // VTableHookFunction(vTableForHelpTextScreenAddr, 83, (void *) HelpTextScreen_MouseDrag,(void **) &old_HelpTextScreen_MouseDrag);
    homura::HookVirtualFunc(vTableForHelpTextScreenAddr, 136, &HelpTextScreen::ButtonDepress, &old_HelpTextScreen_ButtonDepress);

    homura::HookVirtualFunc(vTableForAlmanacDialogAddr, 33, &AlmanacDialog::Update, &old_AlmanacDialog_Update);
    homura::HookVirtualFunc(vTableForAlmanacDialogAddr, 81, &AlmanacDialog::MouseUp, nullptr);
    homura::HookVirtualFunc(vTableForAlmanacDialogAddr, 83, &AlmanacDialog::MouseDrag, &old_AlmanacDialog_MouseDrag);

    homura::HookVirtualFunc(vTableForHouseChooserDialogAddr, 73, &HouseChooserDialog::KeyDown, &old_HouseChooserDialog_KeyDown);
    homura::HookVirtualFunc(vTableForHouseChooserDialogAddr, 78, &HouseChooserDialog::MouseDown, &old_HouseChooserDialog_MouseDown);

    homura::HookVirtualFunc(vTableForSeedPacketAddr, 4, &SeedPacket::BeginDraw, &old_SeedPacket_BeginDraw);
    homura::HookVirtualFunc(vTableForSeedPacketAddr, 5, &SeedPacket::EndDraw, &old_SeedPacket_EndDraw);

    homura::HookVirtualFunc(vTableForSeedBankAddr, 4, &SeedBank::BeginDraw, &old_SeedBank_BeginDraw);
    homura::HookVirtualFunc(vTableForSeedBankAddr, 5, &SeedBank::EndDraw, &old_SeedBank_EndDraw);

    homura::HookVirtualFunc(vTableForGraphicsAddr, 4, &Sexy::Graphics::PushTransform, &old_Sexy_Graphics_PushTransform);
    homura::HookVirtualFunc(vTableForGraphicsAddr, 5, &Sexy::Graphics::PopTransform, &old_Sexy_Graphics_PopTransform);


    homura::HookVirtualFunc(vTableForImageAddr, 37, &Sexy::Image::PushTransform, &old_Sexy_Image_PushTransform);
    homura::HookVirtualFunc(vTableForImageAddr, 38, &Sexy::Image::PopTransform, &old_Sexy_Image_PopTransform);

    homura::HookVirtualFunc(vTableForGLImageAddr, 37, &Sexy::GLImage::PushTransform, &old_Sexy_GLImage_PushTransform);
    homura::HookVirtualFunc(vTableForGLImageAddr, 38, &Sexy::GLImage::PopTransform, &old_Sexy_GLImage_PopTransform);

    homura::HookVirtualFunc(vTableForMemoryImageAddr, 37, &Sexy::MemoryImage::PushTransform, &old_Sexy_MemoryImage_PushTransform);
    homura::HookVirtualFunc(vTableForMemoryImageAddr, 38, &Sexy::MemoryImage::PopTransform, &old_Sexy_MemoryImage_PopTransform);


    homura::HookVirtualFunc(vTableForMusic2Addr, 7, &Music2::StopAllMusic, &old_Music2_StopAllMusic);
    homura::HookVirtualFunc(vTableForMusic2Addr, 8, &Music2::StartGameMusic, &old_Music2_StartGameMusic);
    homura::HookVirtualFunc(vTableForMusic2Addr, 11, &Music2::GameMusicPause, &old_Music2_GameMusicPause);
    // VTableHookFunction(vTableForMusic2Addr, 12, (void *) Music2_UpdateMusicBurst,(void **) &old_Music2_UpdateMusicBurst);
    // VTableHookFunction(vTableForMusic2Addr, 13, (void *) Music2_StartBurst,(void **) &old_Music2_StartBurst);
    homura::HookVirtualFunc(vTableForMusic2Addr, 17, &Music2::FadeOut, &old_Music2_FadeOut);


    // VTableHookFunction(vTableForMaskHelpWidgetAddr, 71, (void *) MaskHelpWidget_KeyDown,nullptr);
    homura::HookVirtualFunc(vTableForMaskHelpWidgetAddr, 78, &AchievementsWidget::MouseDown, nullptr);
    homura::HookVirtualFunc(vTableForMaskHelpWidgetAddr, 81, &AchievementsWidget::MouseUp, nullptr);
    homura::HookVirtualFunc(vTableForMaskHelpWidgetAddr, 83, &AchievementsWidget::MouseDrag, nullptr);


    homura::HookVirtualFunc(vTableForDaveHelpAddr, 31, &LeaderboardsWidget::AddedToManager, nullptr);
    homura::HookVirtualFunc(vTableForDaveHelpAddr, 32, &LeaderboardsWidget::RemovedFromManager, nullptr);
    homura::HookVirtualFunc(vTableForDaveHelpAddr, 78, &LeaderboardsWidget::MouseDown, nullptr);
    homura::HookVirtualFunc(vTableForDaveHelpAddr, 81, &LeaderboardsWidget::MouseUp, nullptr);
    homura::HookVirtualFunc(vTableForDaveHelpAddr, 83, &LeaderboardsWidget::MouseDrag, nullptr);
    homura::HookVirtualFunc(vTableForDaveHelpAddr, 73, &LeaderboardsWidget::KeyDown, nullptr);

    homura::HookVirtualFunc(vTableForTestMenuWidgetAddr, 2, &ZombatarWidget::_destructor, nullptr);
    homura::HookVirtualFunc(vTableForTestMenuWidgetAddr, 3, &ZombatarWidget::_destructor2, nullptr);
    homura::HookVirtualFunc(vTableForTestMenuWidgetAddr, 31, &ZombatarWidget::AddedToManager, nullptr);
    homura::HookVirtualFunc(vTableForTestMenuWidgetAddr, 32, &ZombatarWidget::RemovedFromManager, nullptr);
    homura::HookVirtualFunc(vTableForTestMenuWidgetAddr, 33, &ZombatarWidget::Update, nullptr);
    homura::HookVirtualFunc(vTableForTestMenuWidgetAddr, 38, &ZombatarWidget::Draw, nullptr);
    homura::HookVirtualFunc(vTableForTestMenuWidgetAddr, 78, &ZombatarWidget::MouseDown, nullptr);
    homura::HookVirtualFunc(vTableForTestMenuWidgetAddr, 81, &ZombatarWidget::MouseUp, nullptr);
    homura::HookVirtualFunc(vTableForTestMenuWidgetAddr, 83, &ZombatarWidget::MouseDrag, nullptr);
    homura::HookVirtualFunc(vTableForTestMenuWidgetAddr, 73, &ZombatarWidget::KeyDown, nullptr);


    homura::HookVirtualFunc(vTableForTrashBinAddr, 38, &TrashBin::Draw, nullptr);

    homura::HookVirtualFunc(vTableForConfirmBackToMainDialogAddr, 83, &ConfirmBackToMainDialog_MouseDrag, &old_ConfirmBackToMainDialog_MouseDrag);

    homura::HookVirtualFunc(vTableForSettingsDialogAddr, 153, &SettingsDialog::CheckboxChecked, nullptr);

    homura::HookVirtualFunc(vTableForCreditScreenAddr, 133, &CreditScreen::ButtonDepress, nullptr);

    homura::HookVirtualFunc(vTableForMainMenuAddr, 139, &MainMenu::ButtonPress, nullptr);


    homura::HookVirtualFunc(vTableForWaitForSecondPlayerDialogAddr, 142, &WaitForSecondPlayerDialog::ButtonDepress_Thunk, &old_WaitForSecondPlayerDialog_ButtonDepress);
    homura::HookVirtualFunc(vTableForWaitForSecondPlayerDialogAddr, 33, &WaitForSecondPlayerDialog::Update, nullptr);
    homura::HookVirtualFunc(vTableForWaitForSecondPlayerDialogAddr, 38, &WaitForSecondPlayerDialog::Draw, &old_WaitForSecondPlayerDialog_Draw);
    homura::HookVirtualFunc(vTableForWaitForSecondPlayerDialogAddr, 52, &WaitForSecondPlayerDialog::Resize, nullptr);
    homura::HookVirtualFunc(vTableForWaitForSecondPlayerDialogAddr, 78, &WaitForSecondPlayerDialog::MouseDown, nullptr);
}

void InitOpenSL() {
    homura::HookFunc(Native_AudioOutput_setupAddr, &Native::AudioOutput::setup, &old_Native_AudioOutput_setup);
    homura::HookFunc(Native_AudioOutput_shutdownAddr, &Native::AudioOutput::shutdown, &old_Native_AudioOutput_shutdown);
    // homura::HookFunc(Native_AudioOutput_writeAddr, Native_AudioOutput_write, &old_Native_AudioOutput_write);
    homura::HookFunc(j_AGAudioWriteAddr, &AudioWrite, nullptr);
}

void InitIntroVideo() {

    //     homura::HookFunc(j_AGVideoOpenAddr, AGVideoOpen, nullptr);
    //     homura::HookFunc(j_AGVideoShowAddr, AGVideoShow, nullptr);
    //     homura::HookFunc(j_AGVideoEnableAddr, AGVideoEnable, nullptr);
    //     homura::HookFunc(j_AGVideoIsPlayingAddr, AGVideoIsPlaying, nullptr);
    //     homura::HookFunc(j_AGVideoPlayAddr, AGVideoPlay, nullptr);
    //     homura::HookFunc(j_AGVideoPauseAddr, AGVideoPause, nullptr);
    //     homura::HookFunc(j_AGVideoResumeAddr, AGVideoResume, nullptr);

    //    constexpr auto &libGameMain = "libGameMain.so";
    //    homura::HookPltFunc(libGameMain, AGVideoOpenOffset, AGVideoOpen, nullptr);
    //    homura::HookPltFunc(libGameMain, AGVideoShowOffset, AGVideoShow, nullptr);
    //    homura::HookPltFunc(libGameMain, AGVideoEnableOffset, AGVideoEnable, nullptr);
    //    homura::HookPltFunc(libGameMain, AGVideoIsPlayingOffset, AGVideoIsPlaying, nullptr);
    //    homura::HookPltFunc(libGameMain, AGVideoPlayOffset, AGVideoPlay, nullptr);
}
