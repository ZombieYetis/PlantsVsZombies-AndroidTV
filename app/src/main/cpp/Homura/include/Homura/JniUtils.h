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

#ifndef HOMURA_JNIUTILS_H
#define HOMURA_JNIUTILS_H

#include <jni.h>

#include <string>

namespace homura {

[[nodiscard]] inline std::string JStringToString(JNIEnv *env, jstring jstr) {
    const char *s = env->GetStringUTFChars(jstr, nullptr);
    std::string result = s;
    env->ReleaseStringUTFChars(jstr, s);
    return result;
}

namespace detail {
    [[nodiscard]] inline std::string GetLocaleField(JNIEnv *env, const char *methodName) {
        jclass localeClass = env->FindClass("java/util/Locale");

        jmethodID getDefault = env->GetStaticMethodID(localeClass, "getDefault", "()Ljava/util/Locale;");
        jobject localeObj = env->CallStaticObjectMethod(localeClass, getDefault);

        jmethodID getField = env->GetMethodID(localeClass, methodName, "()Ljava/lang/String;");
        jstring fieldJStr = (jstring)env->CallObjectMethod(localeObj, getField);

        std::string field = JStringToString(env, fieldJStr);
        env->DeleteLocalRef(fieldJStr);
        return field;
    }
} // namespace detail

[[nodiscard]] inline std::string GetLocaleLanguage(JNIEnv *env) {
    return detail::GetLocaleField(env, "getLanguage");
}

[[nodiscard]] inline std::string GetLocaleScript(JNIEnv *env) {
    return detail::GetLocaleField(env, "getScript");
}

[[nodiscard]] inline std::string GetLocaleCountry(JNIEnv *env) {
    return detail::GetLocaleField(env, "getCountry");
}

} // namespace homura

#endif // HOMURA_JNIUTILS_H
