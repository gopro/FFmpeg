/*
 * Android MediaCodec Surface functions
 *
 * Copyright (c) 2016 Matthieu Bouron <matthieu.bouron stupeflix.com>
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <android/native_window.h>
#include <jni.h>

#include "libavutil/mem.h"
#include "ffjni.h"
#include "mediacodec_surface.h"

static int is_surface_valid(JNIEnv *env, void *surface, void *log_ctx)
{   
    if (!env)
        return 0;
    jclass surface_class = (*env)->FindClass(env, "android/view/Surface");
    if (!surface_class) {
        ff_jni_exception_check(env, 1, log_ctx);
        av_log(log_ctx, AV_LOG_WARNING, "Could not find android/view/Surface class\n");
        return 1; /* assume valid if we cannot check */
    }
    jmethodID is_valid_id = (*env)->GetMethodID(env, surface_class, "isValid", "()Z");
    if (!is_valid_id) {
        (*env)->DeleteLocalRef(env, surface_class);
        av_log(log_ctx, AV_LOG_WARNING, "Could not find Surface.isValid() method\n");
        return 1; /* assume valid if we cannot check */
    }
    jboolean valid = (*env)->CallBooleanMethod(env, (jobject)surface, is_valid_id);
    (*env)->DeleteLocalRef(env, surface_class);
    ff_jni_exception_check(env, 1, log_ctx);
    return valid;
}

FFANativeWindow *ff_mediacodec_surface_ref(void *surface, void *native_window, void *log_ctx)
{
    FFANativeWindow *ret;

    ret = av_mallocz(sizeof(*ret));
    if (!ret)
        return NULL;

    if (surface) {
        JNIEnv *env = NULL;

        env = ff_jni_get_env(log_ctx);
        if (env) {
            if (!is_surface_valid(env, surface, log_ctx)) {
                av_log(log_ctx, AV_LOG_ERROR,
                       "Surface %p is not valid (native peer released?)\n", surface);
                av_freep(&ret);
                return NULL;
            }
            ret->surface = (*env)->NewGlobalRef(env, surface);
        }
    }

    if (native_window) {
        ANativeWindow_acquire(native_window);
        ret->native_window = native_window;
    }

    if (!ret->surface && !ret->native_window) {
        av_log(log_ctx, AV_LOG_ERROR, "Both surface and native_window are NULL\n");
        av_freep(&ret);
    }

    return ret;
}

int ff_mediacodec_surface_unref(FFANativeWindow *window, void *log_ctx)
{
    if (!window)
        return 0;

    if (window->surface) {
        JNIEnv *env = NULL;

        env = ff_jni_get_env(log_ctx);
        if (env)
            (*env)->DeleteGlobalRef(env, window->surface);
    }

    if (window->native_window)
        ANativeWindow_release(window->native_window);

    av_free(window);

    return 0;
}
