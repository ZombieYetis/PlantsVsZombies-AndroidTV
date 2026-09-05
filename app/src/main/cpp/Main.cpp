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

#include "Homura/ExceptionUtils.h"
#include "Homura/JniUtils.h"
#include "Homura/Logger.h"
#include "PvZ/Cheat.h"
#include "PvZ/Formation.h"
#include "PvZ/GlobalVariable.h"
#include "PvZ/HookInit.h"
#include "PvZ/Lawn/Board/Board.h"
#include "PvZ/Lawn/Board/Challenge.h"
#include "PvZ/Lawn/Board/SeedPacket.h"
#include "PvZ/Lawn/Common/ConstEnums.h"
#include "PvZ/Lawn/GamepadControls.h"
#include "PvZ/Lawn/LawnApp.h"
#include "PvZ/Lawn/Widget/ReplayManageWidget.h"
#include "PvZ/Lawn/Widget/SeedChooserScreen.h"
#include "PvZ/Lawn/Widget/TitleScreen.h"
#include "PvZ/Lawn/Widget/VSSetupMenu.h"
#include "PvZ/PatchList.h"
#include "PvZ/Symbols.h"

#include <jni.h>

#include <cmath>

#include <numbers>

/**
 * @brief Homura 模块的初始化函数.
 *
 * Java 层已指定模块加载顺序: 先 libGameMain.so, 后 libHomura.so.<br/>
 * <br/>
 * constructor(102) -> call after some variables are initialized
 */
[[gnu::constructor(102)]] static void LibMain() {
    homura::RegisterExceptionHandler();
    homura::RegisterAccessViolationHandler();

    gLibGameMainBaseAddr = homura::GetLibBaseAddr("libGameMain.so");
    assert(gLibGameMainBaseAddr != 0);

    if (InitSymbols()) {
        CallHook();
    }
    ApplyPatches();
}

// jint JNI_OnLoad(JavaVM *vm, void *reserved) {
// return JNI_VERSION_1_6;
// }

extern "C" JNIEXPORT void JNICALL Java_com_transmension_mobile_EnhanceActivity_nativeDisableShop(JNIEnv *env, jclass clazz) {
    disableShop = true;
}

extern "C" JNIEXPORT void JNICALL Java_com_transmension_mobile_EnhanceActivity_nativeEnableManualCollect(JNIEnv *env, jclass clazz) {
    enableManualCollect = true;
}

extern "C" JNIEXPORT void JNICALL Java_com_transmension_mobile_EnhanceActivity_nativeSetHeavyWeaponAngle(JNIEnv *env, jclass clazz, jint i) {
    double radian = i * std::numbers::pi / 180;
    angle1 = float(std::cos(radian));
    angle2 = float(std::sin(radian));
}

extern "C" JNIEXPORT void JNICALL Java_com_transmension_mobile_EnhanceActivity_nativeEnableNewOptionsDialog(JNIEnv *env, jclass clazz) {
    enableNewOptionsDialog = true;
}

extern "C" JNIEXPORT void JNICALL Java_com_transmension_mobile_EnhanceActivity_nativeGaoJiPause(JNIEnv *env, jclass clazz, jboolean enable) {
    if (isMainMenu) {
        return;
    }
    requestPause = enable;
    LawnApp *lawnApp = gLawnApp;

    // if (lawnApp->mPlayerInfo != nullptr){
    // lawnApp->mPlayerInfo->mChallengeRecords[GameMode::ChallengeButteredPopcorn - 2] = 0;
    // lawnApp->mPlayerInfo->mChallengeRecords[GameMode::ChallengeHeavyWeapon - 2] = 0;
    // }
    if (enable) {
        lawnApp->PlaySample(Sexy::SOUND_PAUSE);
    } else {
        lawnApp->PlaySample(Sexy::SOUND_GRAVEBUTTON);
    }
}

extern "C" JNIEXPORT jboolean JNICALL Java_com_transmension_mobile_EnhanceActivity_nativeIsGaoJiPaused(JNIEnv *env, jclass clazz) {
    return requestPause;
}

extern "C" JNIEXPORT void JNICALL Java_com_transmension_mobile_EnhanceActivity_nativeHideCoverLayer(JNIEnv *env, jclass clazz) {
    hideCoverLayer = true;
}

extern "C" JNIEXPORT void JNICALL Java_com_transmension_mobile_EnhanceActivity_nativeShowCoolDown(JNIEnv *env, jclass clazz) {
    showCoolDown = true;
}

extern "C" JNIEXPORT void JNICALL Java_com_transmension_mobile_EnhanceActivity_nativeEnableNormalLevelMode(JNIEnv *env, jclass clazz) {
    normalLevel = true;
}

extern "C" JNIEXPORT void JNICALL Java_com_transmension_mobile_EnhanceActivity_nativeEnableImitater(JNIEnv *env, jclass clazz) {
    imitater = true;
}

extern "C" JNIEXPORT void JNICALL Java_com_transmension_mobile_EnhanceActivity_nativeSendSecondTouch(JNIEnv *env, jclass clazz, jint x, jint y, jint action) {
    if (gTcpConnected || gTcpClientSocket >= 0) {
        return;
    }
    Board *aBoard = (gLawnApp)->mBoard;
    if (aBoard == nullptr) {
        return;
    }
    switch (action) {
        case 0:
            aBoard->MouseDownSecond(x, y, 0);
            break;
        case 1:
            aBoard->MouseDragSecond(x, y);
            break;
        case 2:
            aBoard->MouseUpSecond(x, y, 0);
            break;
        default:
            break;
    }
}

extern "C" JNIEXPORT void JNICALL Java_com_transmension_mobile_EnhanceActivity_nativeEnableNewShovel(JNIEnv *env, jclass clazz) {
    useNewShovel = true;
}

extern "C" JNIEXPORT void JNICALL Java_com_transmension_mobile_EnhanceActivity_nativeDisableTrashBinZombie(JNIEnv *env, jclass clazz) {
    gZombieTrashBinDef.mPickWeight = 0;
}

extern "C" JNIEXPORT void JNICALL Java_com_transmension_mobile_EnhanceActivity_nativeShowHouse(JNIEnv *env, jclass clazz) {
    showHouse = true;
}

extern "C" JNIEXPORT void JNICALL Java_com_transmension_mobile_EnhanceActivity_nativeUseNewCobCannon(JNIEnv *env, jclass clazz) {
    useNewCobCannon = true;
}

extern "C" JNIEXPORT jboolean JNICALL Java_com_transmension_mobile_EnhanceActivity_nativeIsOnlineMode(JNIEnv *env, jclass clazz) {
    return IsOnlineModeActive();
}

extern "C" JNIEXPORT void JNICALL Java_com_android_support_Preferences_Changes(JNIEnv *env, jclass clazz, jobject con, jint featNum, jstring featName, jint value, jboolean boolean, jstring str) {
    switch (featNum) {
        case 1:
            infiniteSun = boolean; // 无限阳光
            break;
        case 2:
            seedPacketFastCoolDown = boolean; // 卡片无冷却
            break;
        case 3:
            abilityFastCoolDown = boolean; // 技能无冷却
            break;
        case 4:
            mushroomsNoSleep = boolean; // 蘑菇免唤醒
            break;
        case 5:
            requestPause = boolean; // 高级暂停
            break;
        case 6:
            noFog = boolean; // 清除迷雾
            break;
        case 7:
            BanDropCoin = boolean; // 禁止掉落阳光金币
            break;
        case 8:
            speedUpMode = value; // 设置游戏倍速
            break;
        case 9:
            hypnoAllZombies = boolean; // 魅惑所有僵尸
            break;
        case 10:
            freezeAllZombies = boolean; // 冻结所有僵尸
            break;
        case 11:
            startAllMowers = boolean; // 启动所有小推车
            break;
        case 21:
            showPlantHealth = boolean; // 所有植物显血
            break;
        case 22:
            showNutGarlicSpikeHealth = boolean; // 损伤点类植物显血
            break;
        case 23:
            showZombieBodyHealth = boolean; // 本体显血
            break;
        case 24:
            showHelmAndShieldHealth = boolean; // 防具显血
            break;
        case 25:
            showGargantuarHealth = boolean; // 巨人显血
            break;
        case 26:
            drawDebugText = boolean; // 显示出怪信息
            break;
        case 27:
            drawDebugRects = boolean; // 绘制碰撞箱
            break;
        case 41:
            doCheatCodeDialog = boolean; // 秘籍指令
            break;
        case 42:
            FreePlantAt = boolean; // 自由种植
            break;
        case 43:
            transparentVase = boolean; // 罐子透视
            break;
        case 44:
            zombieBloated = boolean; // 普僵啃坚果必过敏
            break;
        case 45:
            ZombieCanNotWon = boolean; // 无视僵尸进家
            break;
        case 46:
            boardEdgeAdjust = value * 10; // 进家线后退
            break;
        case 47:
            zombieSetScale = value; // 进家线后退
            break;
        case 48:
            maidCheats = value; // 女仆秘籍
            break;
        case 61:
            ColdPeaCanPassFireWood = boolean; // 寒冰豌豆无视火炬
            break;
        case 62:
            projectilePierce = boolean; // 子弹帧伤
            break;
        case 63:
            bulletSpinnerChosenNum = value - 1; // 手动选择子弹类型
            break;
        case 64:
            randomBullet = boolean; // 随机子弹类型
            break;
        case 65:
            isOnlyPeaUseable = boolean; // 仅对普通豌豆生效
            break;
        case 66:
            isOnlyTouchFireWood = boolean; // 豌豆穿过火炬后转变
            break;
        case 67:
            banCobCannon = boolean; // Ban玉米炮弹
            break;
        case 68:
            banStar = boolean; // Ban星星子弹
            break;
        case 81:
            doCheatDialog = boolean; // 关卡跳转器
            break;
        case 82:
            passNowLevel = boolean; // 过了这关
            break;
        case 83:
            daveNoPickSeeds = boolean; // 取消必选卡片
            break;
        case 84:
            endlessLastStand = boolean; // 坚不可摧无尽
            break;
        case 85:
            targetWavesToJump = value; // 目标轮数
            break;
        case 86:
            requestJumpSurvivalStage = boolean; // 跳轮
            break;
        case 87:
            stopSpawning = boolean; // 暂停刷怪
            break;
        case 88:
            banMower = boolean; // 不出小推车
            break;
        case 89:
            disableDeleteUserdata = boolean; // 禁止删档
            break;
        case 90:
            disableSaveUserdata = boolean; // 禁止存档
            break;
        case 101:
            PumpkinWithLadder = boolean; // 种下南瓜自动搭梯
            break;
        case 102:
            VSBackGround = value; // 更换场景
            break;
        case 103:
            gCheatPlacePlantType = (SeedType)(value >= 49 ? value + 1 : value - 1); // 植物类型
            break;
        case 104:
            gCheatPlaceZombieType = value <= 34 ? (ZombieType)(value - 1) : (ZombieType)(value + 1); // 僵尸类型
            break;
        case 105:
            gCheatPlaceColumn = value; // 横坐标
            break;
        case 106:
            gCheatPlaceRow = value; // 纵坐标
            break;
        case 107:
            gCheatIsPlaceImitaterPlant = boolean; // 模仿者植物
            break;
        case 108:
            gCheatPlacePlant = boolean; // 种植植物
            break;
        case 109:
            gCheatPlaceZombie = boolean; // 放置僵尸
            break;
        case 110:
            gCheatPlaceLadder = boolean; // 搭梯子(单格)
            break;
        case 111:
            recoverAllMowers = boolean; // 恢复所有小推车
            break;
        case 112:
            clearAllPlant = boolean; // 清除所有植物
            break;
        case 113:
            clearAllMowers = boolean; // 清除所有小推车
            break;
        case 114:
            gCheatPlaceGraveStone = boolean; // 冒墓碑
            break;
        case 115:
            clearAllZombies = boolean; // 清除所有僵尸
            break;
        case 116:
            clearAllGraves = boolean; // 清除所有墓碑
            break;
        case 121:
            formationId = value - 1; // 选择白天泳池阵型
            break;
        case 122:
            layChoseFormation = boolean; // 布置选择阵型
            break;
            // case 123:
            // break; // 复制阵型代码
        case 124:
            customFormation = homura::JStringToString(env, str); // 粘贴阵型代码
            break;
        case 125:
            layPastedFormation = boolean; // 布置粘贴阵型
            break;
        case 141:
            targetSeedBank = value; // 目标卡槽
            break;
        case 142:
            choiceSeedPacketIndex = value; // 卡片位置
            break;
        case 143:
            if (value <= 48) {
                choiceSeedType = SeedType(value - 1); // [豌豆射手, 模仿者)
            } else if (value <= 52) {
                choiceSeedType = SeedType(value + 1); // [爆炸坚果, NUM_SEED_TYPES)
            } else if (value <= 76) {
                choiceSeedType = SeedType(value + 8); // [墓碑, 气球僵尸]
            }
            break; // 卡片类型
        case 144:
            isImitaterSeed = boolean; // 模仿者植物卡片
            break;
        case 145:
            setSeedPacket = boolean; // 更换卡片
            break;
        case 200:
            gCheatZombiesToSpawn[ZombieType::ZOMBIE_NORMAL] = boolean; // 普通僵尸
            break;
        case 202:
            gCheatZombiesToSpawn[ZombieType::ZOMBIE_TRAFFIC_CONE] = boolean; // 路障僵尸
            break;
        case 203:
            gCheatZombiesToSpawn[ZombieType::ZOMBIE_POLEVAULTER] = boolean; // 撑杆僵尸
            break;
        case 204:
            gCheatZombiesToSpawn[ZombieType::ZOMBIE_PAIL] = boolean; // 铁桶僵尸
            break;
        case 205:
            gCheatZombiesToSpawn[ZombieType::ZOMBIE_NEWSPAPER] = boolean; // 报纸僵尸
            break;
        case 206:
            gCheatZombiesToSpawn[ZombieType::ZOMBIE_DOOR] = boolean; // 铁网门僵尸
            break;
        case 207:
            gCheatZombiesToSpawn[ZombieType::ZOMBIE_FOOTBALL] = boolean; // 橄榄球僵尸
            break;
        case 208:
            gCheatZombiesToSpawn[ZombieType::ZOMBIE_DANCER] = boolean; // 舞者僵尸
            break;
        case 211:
            gCheatZombiesToSpawn[ZombieType::ZOMBIE_SNORKEL] = boolean; // 潜水僵尸
            break;
        case 212:
            gCheatZombiesToSpawn[ZombieType::ZOMBIE_ZAMBONI] = boolean; // 雪橇车僵尸
            break;
        case 214:
            gCheatZombiesToSpawn[ZombieType::ZOMBIE_DOLPHIN_RIDER] = boolean; // 海豚骑士僵尸
            break;
        case 215:
            gCheatZombiesToSpawn[ZombieType::ZOMBIE_JACK_IN_THE_BOX] = boolean; // 小丑僵尸
            break;
        case 216:
            gCheatZombiesToSpawn[ZombieType::ZOMBIE_BALLOON] = boolean; // 气球僵尸
            break;
        case 217:
            gCheatZombiesToSpawn[ZombieType::ZOMBIE_DIGGER] = boolean; // 矿工僵尸
            break;
        case 218:
            gCheatZombiesToSpawn[ZombieType::ZOMBIE_POGO] = boolean; // 蹦蹦僵尸
            break;
        case 219:
            gCheatZombiesToSpawn[ZombieType::ZOMBIE_YETI] = boolean; // 僵尸雪人
            break;
        case 220:
            gCheatZombiesToSpawn[ZombieType::ZOMBIE_BUNGEE] = boolean; // 飞贼僵尸
            break;
        case 221:
            gCheatZombiesToSpawn[ZombieType::ZOMBIE_LADDER] = boolean; // 梯子僵尸
            break;
        case 222:
            gCheatZombiesToSpawn[ZombieType::ZOMBIE_CATAPULT] = boolean; // 投石车僵尸
            break;
        case 223:
            gCheatZombiesToSpawn[ZombieType::ZOMBIE_GARGANTUAR] = boolean; // 白眼巨人僵尸
            break;
        case 226:
            gCheatZombiesToSpawn[ZombieType::ZOMBIE_TRASHCAN] = boolean; // 垃圾桶僵尸
            break;
        case 227:
            gCheatZombiesToSpawn[ZombieType::ZOMBIE_PEA_HEAD] = boolean; // 豌豆射手僵尸
            break;
        case 228:
            gCheatZombiesToSpawn[ZombieType::ZOMBIE_WALLNUT_HEAD] = boolean; // 坚果僵尸
            break;
        case 229:
            gCheatZombiesToSpawn[ZombieType::ZOMBIE_JALAPENO_HEAD] = boolean; // 火爆辣椒僵尸
            break;
        case 230:
            gCheatZombiesToSpawn[ZombieType::ZOMBIE_GATLING_HEAD] = boolean; // 机枪射手僵尸
            break;
        case 231:
            gCheatZombiesToSpawn[ZombieType::ZOMBIE_SQUASH_HEAD] = boolean; // 窝瓜僵尸
            break;
        case 232:
            gCheatZombiesToSpawn[ZombieType::ZOMBIE_TALLNUT_HEAD] = boolean; // 高坚果僵尸
            break;
        case 233:
            gCheatZombiesToSpawn[ZombieType::ZOMBIE_REDEYE_GARGANTUAR] = boolean; // 红眼巨人僵尸
            break;
        case 234:
            gCheatZombiesToSpawn[ZombieType::ZOMBIE_GIGA_FOOTBALL] = boolean; // 全明星僵尸
            break;
        case 235:
            gCheatZombiesToSpawn[ZombieType::ZOMBIE_SUPER_FAN_IMP] = boolean; // 粉丝小鬼僵尸
            break;
        case 241:
            choiceSpawnMode = value; // 刷怪模式
            break;
        case 242:
            buttonSetSpawn = boolean; // 设置出怪
            break;
        default:
            break;
    }
}

extern "C" JNIEXPORT jobjectArray JNICALL Java_com_android_support_CkHomuraMenu_GetFeatureList(JNIEnv *env, jobject thiz) {
    const std::string language = homura::GetLocaleLanguage(env);
    const auto &featureList =                            //
        (language == "zh") ? cheat::zh_Hans::featureList //
        : (language == "vi") ? cheat::vi_VN::featureList //
                             : cheat::en_US::featureList;

    const jsize featuresCount = static_cast<jsize>(featureList.size());
    jobjectArray ret = env->NewObjectArray(featuresCount, env->FindClass("java/lang/String"), nullptr);
    for (jsize i = 0; i < featuresCount; ++i) {
        env->SetObjectArrayElement(ret, i, env->NewStringUTF(featureList[i]));
    }
    return ret;
}

extern "C" JNIEXPORT jobjectArray JNICALL Java_com_android_support_CkHomuraMenu_SettingsList(JNIEnv *env, jobject thiz) {
    const std::string language = homura::GetLocaleLanguage(env);
    const auto &settingsList =                            //
        (language == "zh") ? cheat::zh_Hans::settingsList //
        : (language == "vi") ? cheat::vi_VN::settingsList //
                             : cheat::en_US::settingsList;

    const jsize settingsCount = static_cast<jsize>(settingsList.size());
    jobjectArray ret = env->NewObjectArray(settingsCount, env->FindClass("java/lang/String"), nullptr);
    for (jsize i = 0; i < settingsCount; ++i) {
        env->SetObjectArrayElement(ret, i, env->NewStringUTF(settingsList[i]));
    }
    return ret;
}

extern "C" JNIEXPORT jstring JNICALL Java_com_android_support_CkHomuraMenu_GetCurrentFormation(JNIEnv *env, jobject thiz) {
    Board *aBoard = (gLawnApp)->mBoard;
    if (aBoard == nullptr) {
        return env->NewStringUTF("");
    }
    return env->NewStringUTF(formation::GenerateFormationStr(aBoard).c_str());
}

extern "C" JNIEXPORT void JNICALL Java_com_transmension_mobile_EnhanceActivity_native1PButtonDown(JNIEnv *env, jclass clazz, jint code) {
    Board *aBoard = (gLawnApp)->mBoard;
    if (aBoard) {
        gButtonDownP1 = true;
        gButtonCodeP1 = Sexy::GamepadButton(code);
    }
}

extern "C" JNIEXPORT void JNICALL Java_com_transmension_mobile_EnhanceActivity_native2PButtonDown(JNIEnv *env, jclass clazz, jint code) {
    Board *aBoard = (gLawnApp)->mBoard;
    if (aBoard) {
        gButtonDownP2 = true;
        gButtonCodeP2 = Sexy::GamepadButton(code);
    }
}


extern "C" JNIEXPORT void JNICALL Java_com_transmension_mobile_EnhanceActivity_nativeSwitchTwoPlayerMode(JNIEnv *env, jclass clazz, jboolean isOn) {
    isKeyboardTwoPlayerMode = doKeyboardTwoPlayerDialog = isOn;
    if (isOn) {
        return;
    }
    LawnApp *anApp = gLawnApp;
    Board *aBoard = anApp->mBoard;
    if (anApp->IsCoopMode() || anApp->mGameMode == GameMode::GAMEMODE_MP_VS) {
        return;
    }
    anApp->ClearSecondPlayer();
    if (aBoard) {
        aBoard->mGamepadControls[1]->mGamepadIndex = -1;
    }
    anApp->mSecondPlayerGamepadIndex = -1;
}

extern "C" JNIEXPORT jboolean JNICALL Java_com_transmension_mobile_EnhanceActivity_nativeIsInGame(JNIEnv *env, jclass clazz) {
    LawnApp *anApp = gLawnApp;
    Board *aBoard = anApp->mBoard;
    auto *aFocusWidget = anApp->mWidgetManager->mFocusWidget;
    if (aBoard && aFocusWidget == aBoard) {
        return true;
    }
    SeedChooserScreen *aSeedChooser = anApp->mSeedChooserScreen;
    if (anApp->IsCoopMode() && aSeedChooser && (aFocusWidget == aSeedChooser)) {
        return true;
    }
    if (anApp->IsVSMode() && anApp->mVSSetupMenu && (anApp->mVSSetupMenu->mState == VSSetupMenu::VS_SETUP_STATE_SIDES || anApp->mVSSetupMenu->mState == VSSetupMenu::VS_SETUP_STATE_CUSTOM_BATTLE)) {
        return true;
    }

    return false;
}


extern "C" JNIEXPORT void JNICALL Java_com_transmension_mobile_EnhanceActivity_nativeSendButtonEvent(JNIEnv *env, jclass clazz, jboolean isButtonDown, jint buttonCode) {
    bool aIsPlayer2 = buttonCode >= 256;
    bool aGamepad1Is2P = gGamepad1ToPlayerIndex == 1;
    Sexy::GamepadButton aButtonCode = Sexy::GamepadButton(aIsPlayer2 ? buttonCode - 256 : buttonCode);

    LawnApp *anApp = gLawnApp;
    Board *aBoard = anApp->mBoard;
    auto *aFocusWidget = anApp->mWidgetManager->mFocusWidget;

    if (!aBoard || aFocusWidget != aBoard) {
        SeedChooserScreen *aSeedChooser = anApp->mSeedChooserScreen;
        if (isButtonDown && anApp->IsCoopMode() && aSeedChooser && aFocusWidget == aSeedChooser) {
            gButtonDownSeedChooser = true;
            gButtonCode = aButtonCode;
            gGamePlayerIndex = aIsPlayer2 ? 1 : 0;
            return;
        }

        VSSetupMenu *aVSSetup = anApp->mVSSetupMenu;
        if (isButtonDown && anApp->IsVSMode() && aVSSetup && (aVSSetup->mState == VSSetupMenu::VS_SETUP_STATE_SIDES || aVSSetup->mState == VSSetupMenu::VS_SETUP_STATE_CUSTOM_BATTLE)) {
            gButtonDownVSSetup = true;
            gButtonCode = aButtonCode;
            gGamePlayerIndex = aIsPlayer2 ? 1 : 0;
            return;
        }
        return;
    }

    float &aX = aIsPlayer2 ? gGamepadP2VelX : gGamepadP1VelX;
    float &aY = aIsPlayer2 ? gGamepadP2VelY : gGamepadP1VelY;

    using Sexy::GamepadButton;
    if (isButtonDown) {
        switch (aButtonCode) {
            case GamepadButton::GAMEPAD_BUTTON_B:
                gKeyDown = true;
                gGamePlayerIndex = aIsPlayer2 ? 1 : 0;
                break;
            case GamepadButton::GAMEPAD_BUTTON_DPAD_UP:
                aY = -400;
                break;
            case GamepadButton::GAMEPAD_BUTTON_DPAD_DOWN:
                aY = 400;
                break;
            case GamepadButton::GAMEPAD_BUTTON_DPAD_LEFT:
                aX = -400;
                break;
            case GamepadButton::GAMEPAD_BUTTON_DPAD_RIGHT:
                aX = 400;
                break;
            default:
                gButtonDown = true;
                gButtonCode = aButtonCode;
                if (aGamepad1Is2P) {
                    gGamePlayerIndex = aIsPlayer2 ? 0 : 1;
                } else {
                    gGamePlayerIndex = aIsPlayer2 ? 1 : 0;
                }
                break;
        }
    } else {
        switch (aButtonCode) {
            case GamepadButton::GAMEPAD_BUTTON_DPAD_UP:
            case GamepadButton::GAMEPAD_BUTTON_DPAD_DOWN:
                aY = 0;
                break;
            case GamepadButton::GAMEPAD_BUTTON_DPAD_LEFT:
            case GamepadButton::GAMEPAD_BUTTON_DPAD_RIGHT:
                aX = 0;
                break;
            default:
                break;
        }
    }
}

extern "C" JNIEXPORT void JNICALL Java_com_transmension_mobile_EnhanceActivity_nativeAutoFixPosition(JNIEnv *env, jclass clazz) {
    positionAutoFix = true;
}

extern "C" JNIEXPORT void JNICALL Java_com_transmension_mobile_EnhanceActivity_nativeUseXboxMusics(JNIEnv *env, jclass clazz) {
    useXboxMusic = true;
}

extern "C" JNIEXPORT void JNICALL Java_com_transmension_mobile_EnhanceActivity_nativeSeedBankPin(JNIEnv *env, jclass clazz) {
    seedBankPin = true;
}
extern "C" JNIEXPORT void JNICALL Java_com_transmension_mobile_EnhanceActivity_nativeDynamicPreview(JNIEnv *env, jclass clazz) {
    dynamicPreview = true;
}
extern "C" JNIEXPORT void JNICALL Java_com_transmension_mobile_EnhanceActivity_nativeEnableOpenSL(JNIEnv *env, jclass clazz) {
    InitOpenSL();
}

extern "C" JNIEXPORT void JNICALL Java_com_transmension_mobile_EnhanceActivity_nativeJumpLogo(JNIEnv *env, jclass clazz) {
    jumpLogo = true;
}

extern "C" JNIEXPORT void JNICALL Java_com_transmension_mobile_EnhanceActivity_nativePlayVideo(JNIEnv *env, jclass clazz) {
    playVideo = true;
}

extern "C" JNIEXPORT void JNICALL Java_com_transmension_mobile_EnhanceActivity_nativeHeavyWeaponAccel(JNIEnv *env, jclass clazz) {
    heavyWeaponAccel = true;
}


extern "C" JNIEXPORT void JNICALL Java_com_transmension_mobile_NativeView_onTextInputNative2(JNIEnv *env, jobject thiz, jstring text) {
    std::string input = homura::JStringToString(env, text);
    if (input.empty()) {
        return;
    }
    gHasInputContent.wait(true); // 若旧输入未被消费则等待
    gInputString = std::move(input);
    gHasInputContent = true;
}

extern "C" JNIEXPORT void JNICALL Java_com_transmension_mobile_EnhanceActivity_nativeOnReplayImportFinished(JNIEnv *env, jclass clazz, jboolean success, jstring message) {
    const char *msg = message ? env->GetStringUTFChars(message, nullptr) : "";
    replayui::OnReplayImportFinished(success, msg);
    if (message) {
        env->ReleaseStringUTFChars(message, msg);
    }
}

extern "C" JNIEXPORT void JNICALL Java_com_transmension_mobile_EnhanceActivity_nativeOnReplayExportFinished(JNIEnv *env, jclass clazz, jboolean success, jstring message) {
    const char *msg = message ? env->GetStringUTFChars(message, nullptr) : "";
    replayui::OnReplayExportFinished(success, msg);
    if (message) {
        env->ReleaseStringUTFChars(message, msg);
    }
}
