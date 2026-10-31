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

#ifndef PVZ_SEXYAPPFRAMEWORK_EFFECT_REANIMATION_H
#define PVZ_SEXYAPPFRAMEWORK_EFFECT_REANIMATION_H

#include "Homura/TypeUtils.h"
#include "PvZ/Lawn/Common/SyncObject.h"
#include "PvZ/STL/string.h"
#include "PvZ/SexyAppFramework/Graphics/Color.h"
#include "PvZ/SexyAppFramework/Misc/SexyMatrix.h"
#include "PvZ/Symbols.h"
#include "PvZ/TodLib/Common/DataArray.h"
#include "PvZ/TodLib/Effect/FilterEffect.h"

class Reanimation;
class ReanimAtlas;
class AttacherInfo;
class AttachEffect;
class TodTriangleGroup;
class TodParticleSystem;
class ReanimatorTransform;
class ReanimatorDefinition;
class ReanimatorFrameTime;

namespace Sexy {
class Font;
class Graphics;
class MemoryImage;
class Image;
} // namespace Sexy

// ######################################################################################################################################################
// ############################################################### 以下为动画定义相关内容 ###############################################################
// ######################################################################################################################################################

class ReanimatorTrack {
public:
    int unk;
    char *mName;
    ReanimatorTransform *mTransforms;
    int mTransformCount;
    bool IsAttacher;
};

// ====================================================================================================
// ★ 【动画器定义】
// ----------------------------------------------------------------------------------------------------
// 用于描述一种动画类型与该动画的数据文件的文件名及标志之间的对应关系。
// ====================================================================================================
class ReanimatorDefinition {
public:
    ReanimatorTrack *mTracks; // 0
    int mTrackCount;          // 1
    float mFPS;               // 2
    int *mReanimAtlas;        // 3
}; // 大小4个整数

// ====================================================================================================
// ★ 【动画参数】
// ----------------------------------------------------------------------------------------------------
// 用于描述一种动画类型与该动画的数据文件的文件名及标志之间的对应关系。
// ====================================================================================================
class ReanimationParams {
public:
    ReanimationType mReanimationType;
    const char *mReanimFileName;
    int mReanimParamFlags;
};

inline void ReanimatorEnsureDefinitionLoaded(ReanimationType theReanimType, bool theIsPreloading) {
    reinterpret_cast<void (*)(ReanimationType, bool)>(ReanimatorEnsureDefinitionLoadedAddr)(theReanimType, theIsPreloading);
}

// ######################################################################################################################################################
// ############################################################## 以下正式开始动画相关声明 ##############################################################
// ######################################################################################################################################################

enum {
    RENDER_GROUP_HIDDEN = -1,
    RENDER_GROUP_NORMAL = 0,
};

class ReanimationHolder {
public:
    // struct ReanimationLists {
    // public:
    // Reanimation list[1024];
    // }; // 225280个整数
public:
    DataArray<Reanimation> mReanimations; // 0
    int unk1;                             // 1
    int mReanimationNum;                  // 2
    int unk2[3];                          // 3 ~ 5
    char *mName;                          // 6
};

// ====================================================================================================
// ★ 【动画器时间】
// ----------------------------------------------------------------------------------------------------
// 用于描述动画当前正在播放的时间位置。
// ====================================================================================================
class ReanimatorFrameTime {
public:
    float mFraction;         // 0
    int mAnimFrameBeforeInt; // 1
    int mAnimFrameAfterInt;  // 2
};

class ReanimatorTransform {
public:
    float mTransX;        // 0
    float mTransY;        // 1
    float mSkewX;         // 2
    float mSkewY;         // 3
    float mScaleX;        // 4
    float mScaleY;        // 5
    float mFrame;         // 6
    float mAlpha;         // 7
    Sexy::Image *mImage;  // 8
    Sexy::Image *mImage2; // 9
    int *mFont;           // 10
    const char *mText;    // 11
    // 大小12个整数

    ReanimatorTransform() {
        reinterpret_cast<void (*)(ReanimatorTransform *)>(ReanimatorTransform_ReanimatorTransformAddr)(this);
    }
};

class ReanimatorTrackInstance {
public:
    int mBlendCounter;                   // 0
    int mBlendTime;                      // 1
    ReanimatorTransform mBlendTransform; // 2 ~ 13
    float mShakeOverride;                // 14
    float mShakeX;                       // 15
    float mShakeY;                       // 16
    AttachmentID mAttachmentID;          // 17
    Sexy::Image *mImageOverride;         // 18
    int mRenderGroup;                    // 19
    Sexy::Color mTrackColor;             // 20 ~ 23
    bool mIgnoreClipRect;                // 96
    bool mTruncateDisappearingFrames;    // 97
    bool mIgnoreColorOverride;           // 98
    bool mIgnoreExtraAdditiveColor;      // 99
}; // 大小25个整数

class Reanimation : SyncObject {
public:
    ReanimationType mReanimationType;           // 4
    float mAnimTime;                            // 5
    float mAnimRate;                            // 6
    ReanimatorDefinition *mDefinition;          // 7
    ReanimLoopType mLoopType;                   // 8
    bool mDead;                                 // 36
    int mFrameStart;                            // 10
    int mFrameCount;                            // 11
    int mFrameBasePose;                         // 12
    Sexy::SexyTransform2D mOverlayMatrix;       // 13 ~ 21
    Sexy::Color mColorOverride;                 // 22 ~ 25
    ReanimatorTrackInstance *mTrackInstances;   // 26
    int mLoopCount;                             // 27
    ReanimationHolder *mReanimationHolder;      // 28
    bool mIsAttachment;                         // 116
    int mRenderOrder;                           // 30
    Sexy::Color mExtraAdditiveColor;            // 31 ~ 34
    bool mEnableExtraAdditiveDraw;              // 140
    Sexy::Color mExtraOverlayColor;             // 36 ~ 39
    bool mEnableExtraOverlayDraw;               // 160
    float mLastFrameTime;                       // 41
    FilterEffect mFilterEffect;                 // 42
    Sexy::Color mCustomFilterEffectColor;       // 43 ~ 46
    int unk2[4];                                // 47 ~ 50
    ReanimatorTransform *mReanimatorTransforms; // 51
    bool unkBool;                               // 208
    float unkFloatWithInitialValue_1;           // 53
    // 大小54个整数

    void SetPosition(float theX, float theY) {
        reinterpret_cast<void (*)(Reanimation *, float, float)>(Reanimation_SetPositionAddr)(this, theX, theY);
    }
    void OverrideScale(float theScaleX, float theScaleY) {
        reinterpret_cast<void (*)(Reanimation *, float theX, float theY)>(Reanimation_OverrideScaleAddr)(this, theScaleX, theScaleY);
    }
    void DrawRenderGroup(Sexy::Graphics *g, int theRenderGroup) {
        reinterpret_cast<void (*)(Reanimation *, Sexy::Graphics *, int)>(Reanimation_DrawRenderGroupAddr)(this, g, theRenderGroup);
    }
    void ReanimationInitializeType(float theX, float theY, ReanimationType theReanimType) {
        reinterpret_cast<void (*)(Reanimation *, float, float, ReanimationType)>(Reanimation_ReanimationInitializeTypeAddr)(this, theX, theY, theReanimType);
    }
    int FindTrackIndexById(const char *theTrackName) {
        return reinterpret_cast<int (*)(Reanimation *, const char *)>(Reanimation_FindTrackIndexByIdAddr)(this, theTrackName);
    };
    void GetCurrentTransform(int theTrackIndex, ReanimatorTransform *theTransformCurrent) {
        reinterpret_cast<void (*)(Reanimation *, int, ReanimatorTransform *)>(Reanimation_GetCurrentTransformAddr)(this, theTrackIndex, theTransformCurrent);
    }
    void PlayReanim(const char *theTrackName,
                    ReanimLoopType theLoopType,
                    int theBlendTime,
                    float theAnimRate) { // 倍率 = abs(theAnimRate)/mFps, 若动画为12帧, 即 theAnimRate 6 = *0.5倍速, 12 = *1, 以此类推. 0为不改变当前速率
        reinterpret_cast<void (*)(Reanimation *, const char *, ReanimLoopType, int, float)>(Reanimation_PlayReanimAddr)(this, theTrackName, theLoopType, theBlendTime, theAnimRate);
    }
    void SetAnimRate(float theAnimRate) {
        reinterpret_cast<void (*)(Reanimation *, float)>(Reanimation_SetAnimRateAddr)(this, theAnimRate);
    }
    bool IsAnimPlaying(const char *theTrackName) {
        return reinterpret_cast<bool (*)(Reanimation *, const char *)>(Reanimation_IsAnimPlayingAddr)(this, theTrackName);
    }
    void SetImageOverride(const char *theTrackName, Sexy::Image *theImage) {
        reinterpret_cast<void (*)(Reanimation *, const char *theTrackName, Sexy::Image *)>(Reanimation_SetImageOverrideAddr)(this, theTrackName, theImage);
    }
    Sexy::Image *GetImageOverride(const char *theTrackName) {
        return reinterpret_cast<Sexy::Image *(*)(Reanimation *, const char *theTrackName)>(Reanimation_GetImageOverrideAddr)(this, theTrackName);
    }
    int FindTrackIndex(const char *theTrackName) {
        return reinterpret_cast<int (*)(Reanimation *, const char *)>(Reanimation_FindTrackIndexAddr)(this, theTrackName);
    }
    void ShowOnlyTrack(const char *theTrackName) {
        reinterpret_cast<void (*)(Reanimation *, const char *)>(Reanimation_ShowOnlyTrackAddr)(this, theTrackName);
    }
    void ReanimationDie() {
        reinterpret_cast<void (*)(Reanimation *)>(Reanimation_ReanimationDieAddr)(this);
    }
    void SetFramesForLayer(const char *theTrackName) {
        reinterpret_cast<void (*)(Reanimation *, const char *)>(Reanimation_SetFramesForLayerAddr)(this, theTrackName);
    }
    bool TrackExists(const char *theTrackName) {
        return reinterpret_cast<bool (*)(Reanimation *, const char *)>(Reanimation_TrackExistsAddr)(this, theTrackName);
    }
    void AttachToAnotherReanimation(Reanimation *theAttachReanim, const char *theTrackName) {
        reinterpret_cast<bool (*)(Reanimation *, Reanimation *, const char *)>(Reanimation_AttachToAnotherReanimationAddr)(this, theAttachReanim, theTrackName);
    }
    void Update() {
        reinterpret_cast<void (*)(Reanimation *)>(Reanimation_UpdateAddr)(this);
    }
    static void MatrixFromTransform(const ReanimatorTransform &theTransform, Sexy::SexyMatrix3 &theMatrix) {
        reinterpret_cast<void (*)(const ReanimatorTransform &, Sexy::SexyMatrix3 &)>(Reanimation_MatrixFromTransformAddr)(theTransform, theMatrix);
    }
    void GetFrameTime(ReanimatorFrameTime *theFrameTime) {
        reinterpret_cast<void (*)(Reanimation *, ReanimatorFrameTime *)>(Reanimation_GetFrameTimeAddr)(this, theFrameTime);
    }
    void GetTrackMatrix(int theTrackIndex, Sexy::SexyTransform2D &theMatrix) {
        reinterpret_cast<void (*)(Reanimation *, int, Sexy::SexyTransform2D &)>(Reanimation_GetTrackMatrixAddr)(this, theTrackIndex, theMatrix);
    }
    void GetAttachmentOverlayMatrix(int theTrackIndex, Sexy::SexyTransform2D &theOverlayMatrix) {
        reinterpret_cast<void (*)(Reanimation *, int, Sexy::SexyTransform2D &)>(Reanimation_GetAttachmentOverlayMatrixAddr)(this, theTrackIndex, theOverlayMatrix);
    }
    void AssignRenderGroupToPrefix(const char *theTrackName, int theRenderGroup) {
        reinterpret_cast<void (*)(Reanimation *, const char *, int)>(Reanimation_AssignRenderGroupToPrefixAddr)(this, theTrackName, theRenderGroup);
    }
    void PropogateColorToAttachments() {
        reinterpret_cast<void (*)(Reanimation *)>(Reanimation_PropogateColorToAttachmentsAddr)(this);
    }
    void StartBlend(int theBlendTime) {
        reinterpret_cast<void (*)(Reanimation *, int)>(Reanimation_StartBlendAddr)(this, theBlendTime);
    }
    AttachEffect *AttachParticleToTrack(const char *theTrackName, TodParticleSystem *theParticleSystem, float thePosX, float thePosY) {
        return reinterpret_cast<AttachEffect *(*)(Reanimation *, const char *, TodParticleSystem *, float, float)>(Reanimation_AttachParticleToTrackAddr)(
            this, theTrackName, theParticleSystem, thePosX, thePosY);
    }
    void SetTruncateDisappearingFrames(const char *theTrackName, bool theTruncateDisappearingFrames) {
        reinterpret_cast<void (*)(Reanimation *, const char *, bool)>(Reanimation_SetTruncateDisappearingFramesAddr)(this, theTrackName, theTruncateDisappearingFrames);
    }
    void IgnoreClipRectForPrefix(const char *theTrackName, bool theIgnoreClipRect) {
        reinterpret_cast<void (*)(Reanimation *, const char *, bool)>(Reanimation_IgnoreClipRectForPrefixAddr)(this, theTrackName, theIgnoreClipRect);
    }
    float GetTrackVelocity(const char *theTrackName) {
        return reinterpret_cast<float (*)(Reanimation *, const char *)>(Reanimation_GetTrackVelocityAddr)(this, theTrackName);
    }
    bool IsTrackShowing(const char *theTrackName) {
        return reinterpret_cast<bool (*)(Reanimation *, const char *)>(Reanimation_IsTrackShowingAddr)(this, theTrackName);
    }

    Reanimation() {
        _constructor();
    }
    ~Reanimation() {
        _destructor();
    }

    void Draw(Sexy::Graphics *g);
    bool DrawTrack(Sexy::Graphics *g, int theTrackIndex, int theRenderGroup, TodTriangleGroup *theTriangleGroup);
    bool ShouldTriggerTimedEvent(float theEventTime) const;
    void AssignRenderGroupToTrack(const char *theTrackName, int theRenderGroup) const;
    ReanimatorTrackInstance *GetTrackInstanceByName(const char *theTrackName);
    int GetZombatarHatTrackIndex() const;
    int GetZombatarEyeWearTrackIndex() const;
    void SetImageOrigin(const char *theTrackName, Sexy::Image *theImage);
    void SetImageDefinition(const char *theTrackName, Sexy::Image *theImage);
    int HideTrack(const char *theTrackName, bool theIsHide);
    void HideTrackById(int theTrackIndex, bool theIsHide) const;
    void HideTrackByPrefix(const char *theTrackPrefix, bool theIsHide);
    void SetZombatarReanim();
    void SetZombatarHats(unsigned char theHats, unsigned char theColor);
    void SetZombatarHair(unsigned char theHair, unsigned char theColor);
    void SetZombatarFHair(unsigned char theFacialHair, unsigned char theColor);
    void SetZombatarAccessories(unsigned char theAccessories, unsigned char theColor);
    void SetZombatarEyeWear(unsigned char theEyeWear, unsigned char theColor);
    void SetZombatarTidBits(unsigned char theTidBits, unsigned char theColor);
    void GetZombatarTrackIndex(int *theIndexArray) const;

protected:
    void _constructor() {
        reinterpret_cast<void (*)(Reanimation *)>(Reanimation_ReanimationAddr)(this);
    }
    void _destructor() {
        reinterpret_cast<void (*)(Reanimation *)>(Reanimation_Delete2Addr)(this);
    }
};

/***************************************************************************************************************/


inline bool (*old_Reanimation_DrawTrack)(Reanimation *reanim, Sexy::Graphics *g, int theTrackIndex, int theRenderGroup, TodTriangleGroup *theTriangleGroup);

inline void (*old_ReanimatorLoadDefinitions)(ReanimationParams *theReanimationParamArray, int theReanimationParamArraySize);

inline pvzstl::string (*old_DefinitionGetCompiledFilePathFromXMLFilePath)(const pvzstl::string &defPathString);

void ReanimatorLoadDefinitions(ReanimationParams *theReanimationParamArray, int theReanimationParamArraySize);

pvzstl::string DefinitionGetCompiledFilePathFromXMLFilePath(const pvzstl::string &defPathString);

// 复用定义加载器的图片 ID 缓存；原接口的两个字符串参数按值传递。
inline bool DefinitionLoadImage(Sexy::Image **theImage, pvzstl::string theImageId, pvzstl::string theImagePath) {
    return reinterpret_cast<bool (*)(Sexy::Image **, pvzstl::string, pvzstl::string)>(DefinitionLoadImageAddr)(theImage, theImageId, theImagePath);
}

#endif // PVZ_SEXYAPPFRAMEWORK_EFFECT_REANIMATION_H
