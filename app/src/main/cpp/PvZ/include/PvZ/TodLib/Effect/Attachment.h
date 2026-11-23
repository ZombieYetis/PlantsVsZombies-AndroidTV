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

#ifndef PVZ_SEXYAPPFRAMEWORK_EFFECT_ATTACHMENT_H
#define PVZ_SEXYAPPFRAMEWORK_EFFECT_ATTACHMENT_H

#include "PvZ/Lawn/Common/ConstEnums.h"
#include "PvZ/SexyAppFramework/Misc/SexyMatrix.h"
#include "PvZ/Symbols.h"
#include "PvZ/TodLib/Common/DataArray.h"

class AttachEffect {
public:
    ReanimationID mEffectID;       // 0
    EffectType mEffectType;        // 1
    Sexy::SexyTransform2D mOffset; // 2 ~ 10
    bool mDontDrawIfParentHidden;  // 44
    bool mDontPropogateColor;      // 45
}; // 大小12个整数

class Attachment {
public:
    void **vTable;                                           // 0
    homura::Storage<std::vector<SyncBlockInfo>> mSyncBlocks; // 1 ~ 3
    AttachEffect mEffectArray[16];                           // 4 ~ 195
    int mNumEffects;                                         // 196
    bool mDead;                                              //
    bool mActive;
    bool mUsesClipping;
}; // 大小198个整数

inline AttachEffect *AttachReanim(AttachmentID &theAttachmentID, Reanimation *theReanimation, float theOffsetX, float theOffsetY) {
    return reinterpret_cast<AttachEffect *(*)(AttachmentID &, Reanimation *, float, float)>(AttachReanimAddr)(theAttachmentID, theReanimation, theOffsetX, theOffsetY);
}

inline void AttachmentUpdateAndSetMatrix(AttachmentID &theAttachmentID, const Sexy::SexyTransform2D &theMatrix) {
    reinterpret_cast<void (*)(AttachmentID &, const Sexy::SexyTransform2D &)>(AttachmentUpdateAndSetMatrixAddr)(theAttachmentID, theMatrix);
}

inline void AttachmentDraw(AttachmentID theAttachmentID, Sexy::Graphics *g, bool theParentHidden) {
    reinterpret_cast<void (*)(AttachmentID, Sexy::Graphics *, bool)>(AttachmentDrawAddr)(theAttachmentID, g, theParentHidden);
}

inline void AttachmentDetach(AttachmentID &theAttachmentID) {
    reinterpret_cast<void (*)(AttachmentID &)>(AttachmentDetachAddr)(theAttachmentID);
}

inline void AttachmentDie(AttachmentID &theAttachmentID) {
    reinterpret_cast<void (*)(AttachmentID &)>(AttachmentDieAddr)(theAttachmentID);
}

inline void AttachParticle(AttachmentID &theAttachmentID, TodParticleSystem *theParticleSystem, float theOffsetX, float theOffsetY) {
    reinterpret_cast<void (*)(AttachmentID &, TodParticleSystem *, float, float)>(AttachParticleAddr)(theAttachmentID, theParticleSystem, theOffsetX, theOffsetY);
}

inline void AttachmentDetachCrossFadeParticleType(AttachmentID &theAttachmentID, ParticleEffect theParticleEffect, const char *theCrossFadeName) {
    reinterpret_cast<void (*)(AttachmentID &, ParticleEffect, const char *)>(AttachmentDetachCrossFadeParticleTypeAddr)(theAttachmentID, theParticleEffect, theCrossFadeName);
}

inline void AttachmentReanimTypeDie(AttachmentID &theAttachmentID, ReanimationType theReanimationType) {
    reinterpret_cast<void (*)(AttachmentID &, ReanimationType)>(AttachmentReanimTypeDieAddr)(theAttachmentID, theReanimationType);
}

inline Reanimation *FindReanimAttachment(AttachmentID &theAttachmentID) {
    return reinterpret_cast<Reanimation *(*)(AttachmentID &)>(FindReanimAttachmentAddr)(theAttachmentID);
}

inline void AttachmentUpdateAndMove(AttachmentID &theAttachmentID, float theX, float theY) {
    reinterpret_cast<void (*)(AttachmentID &, float, float)>(AttachmentUpdateAndMoveAddr)(theAttachmentID, theX, theY);
}

class AttachmentHolder {
public:
    DataArray<Attachment> mAttachments;

    AttachmentHolder() = delete;
    ~AttachmentHolder() = delete;
};

#endif // PVZ_SEXYAPPFRAMEWORK_EFFECT_ATTACHMENT_H
