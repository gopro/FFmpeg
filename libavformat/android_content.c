/*
 * Copyright (c) 2015-2016 Matthieu Bouron <matthieu.bouron stupeflix.com>
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

#include <pthread.h>

#include "android_content.h"

static void *android_app_ctx;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void av_android_content_set_app_ctx(void *ctx)
{
    pthread_mutex_lock(&lock);
    android_app_ctx = ctx;
    pthread_mutex_unlock(&lock);
}

void *av_android_content_get_app_ctx(void)
{
    void *ctx;

    pthread_mutex_lock(&lock);
    ctx = android_app_ctx;
    pthread_mutex_unlock(&lock);

    return ctx;
}
