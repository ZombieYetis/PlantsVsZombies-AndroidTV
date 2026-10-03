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

#ifndef PVZ_LAWN_SYSTEM_SAVE_GAME_H
#define PVZ_LAWN_SYSTEM_SAVE_GAME_H

#include "PvZ/Lawn/Common/SyncObject.h"
#include "PvZ/STL/string.h"
#include "PvZ/SexyAppFramework/Buffer.h"
#include "PvZ/Symbols.h"
#include "PvZ/TodLib/Common/DataArray.h"
#include "PvZ/TodLib/Effect/Reanimator.h"

class Board;
class LawnApp;
struct BaseEvent;

#define SAVE_FILE_MAGIC_NUMBER 0xFEEDDEAD
#define SAVE_FILE_VERSION 8
#define SAVE_FILE_DATE 0

struct SaveFileHeader {
    unsigned int mMagicNumber;
    unsigned int mBuildVersion;
    unsigned int mBuildDate;
};

class SaveGameContext {
public:
    Sexy::Buffer mBuffer;
    bool mFailed;
    bool mReading;

    SaveGameContext() = default;

    void SyncInt(int &theInt) {
        reinterpret_cast<void (*)(SaveGameContext *, int &)>(SaveGameContext_SyncIntAddr)(this, theInt);
    }
    void SyncBytes(void *theDest, int theReadSize) {
        reinterpret_cast<void (*)(SaveGameContext *, void *, int)>(SaveGameContext_SyncBytesAddr)(this, theDest, theReadSize);
    }
    void SyncUint(unsigned int &theUint) {
        reinterpret_cast<void (*)(SaveGameContext *, unsigned int &)>(SaveGameContext_SyncUintAddr)(this, theUint);
    }
    void SyncImage(Sexy::Image *&theImage) {
        reinterpret_cast<void (*)(SaveGameContext *, Sexy::Image *&)>(SaveGameContext_SyncImageAddr)(this, theImage);
    }

    void SyncReanimationDef(ReanimatorDefinition *&theDefinition);
};

inline void SyncBoard(SaveGameContext *theContext, Board *theBoard) {
    reinterpret_cast<void (*)(SaveGameContext *, Board *)>(SyncBoardAddr)(theContext, theBoard);
}

template <typename T>
void SyncDataArray(DataArray<T> &theArray, SaveGameContext *theContext) {
    theContext->SyncUint(theArray.mFreeListHead);
    theContext->SyncUint(theArray.mMaxUsedCount);
    theContext->SyncUint(theArray.mSize);

    for (auto i = 0u; i < theArray.mMaxUsedCount; ++i) {
        auto *anItem = &theArray.mBlock[i];
        theContext->SyncBytes(&anItem->mID, sizeof(anItem->mID));

        bool itemExists = (anItem->mID >> DATA_ARRAY_KEY_SHIFT) != 0u;
        if (!itemExists) {
            continue;
        }

        if (theContext->mReading) {
            new (&anItem->mItem) T();
        }
        std::vector<SyncBlockInfo> &aSyncBlocks = *anItem->mItem.mSyncBlocks;
        for (auto &aBlock : aSyncBlocks) {
            theContext->SyncBytes(aBlock.mAddress, aBlock.mSize);
        }
    }
}

bool LawnSaveGame_Original(Board *theBoard, const pvzstl::string &theFilePath);
bool LawnLoadGame_Original(Board *theBoard, SaveGameContext *theContext);
void FixBoardAfterLoad(Board *board);
bool LawnSaveGame(Board *theBoard, const pvzstl::string &theFilePath);
bool LawnLoadGame(Board *theBoard, SaveGameContext *theContext);
inline void GetSavedGameName(const pvzstl::string &name, GameMode theGameMode, int theProfileId, int theId) {
    reinterpret_cast<void (*)(const pvzstl::string &, GameMode, int, int)>(GetSavedGameNameAddr)(name, theGameMode, theProfileId, theId);
}
void SyncReanimation(Board *theBoard, Reanimation *theReanimation, SaveGameContext &theContext);

namespace netplay {
// State is external to LawnApp/Board to preserve the native object layouts.
bool StartCoopEndlessLoad(LawnApp *app);
bool HandleSaveGameEvent(LawnApp *app, const BaseEvent *event, bool fromHost);
void UpdateSaveGameTransfer(LawnApp *app);
void ResetSaveGameTransfer() noexcept;
bool IsSynchronizingSaveGame() noexcept;
bool IsApplyingOnlineSaveGame() noexcept;
} // namespace netplay

#endif // PVZ_LAWN_SYSTEM_SAVE_GAME_H
