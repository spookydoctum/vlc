/*****************************************************************************
 * va_surface_internal.h: libavcodec Generic Video Acceleration helpers
 *****************************************************************************
 * Copyright (C) 2009 Geoffroy Couprie
 * Copyright (C) 2009 Laurent Aimar
 * Copyright (C) 2015 Steve Lhomme
 *
 * Authors: Geoffroy Couprie <geal@videolan.org>
 *          Laurent Aimar <fenrir _AT_ videolan _DOT_ org>
 *          Steve Lhomme <robux4@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#ifndef AVCODEC_VA_SURFACE_INTERNAL_H
#define AVCODEC_VA_SURFACE_INTERNAL_H

#include "va_surface.h"

#include <libavcodec/avcodec.h>
#include "va.h"

#define MAX_SURFACE_COUNT (64)
typedef struct
{
    int          codec_id;
    int          width;
    int          height;

    /* */
    unsigned     surface_count;
    int          surface_width;
    int          surface_height;

    vlc_va_surface_t    *surface[MAX_SURFACE_COUNT];
    D3D_DecoderSurface  *hw_surface[MAX_SURFACE_COUNT];

    int (*pf_create_device)(vlc_va_t *);
    void (*pf_destroy_device)(vlc_va_t *);

    int (*pf_create_device_manager)(vlc_va_t *);
    void (*pf_destroy_device_manager)(vlc_va_t *);

    int (*pf_create_video_service)(vlc_va_t *);
    void (*pf_destroy_video_service)(vlc_va_t *);

    /**
     * Create the DirectX surfaces in hw_surface and the decoder in decoder
     */
    int (*pf_create_decoder_surfaces)(vlc_va_t *, int codec_id,
                                      const video_format_t *fmt,
                                      unsigned surface_count);
    /**
     * Destroy resources allocated with the surfaces and the associated decoder
     */
    void (*pf_destroy_surfaces)(vlc_va_t *);
    /**
     * Set the avcodec hw context after the decoder is created
     */
    void (*pf_setup_avcodec_ctx)(vlc_va_t *);

    /**
     * Create a new context for the surface being acquired
     */
    picture_context_t* (*pf_new_surface_context)(vlc_va_t *, vlc_va_surface_t *, D3D_DecoderSurface *);

} va_pool_t;

int va_pool_Open(vlc_va_t *, va_pool_t *, AVCodecContext *);
void va_pool_Close(vlc_va_t *va, va_pool_t *);
int va_pool_Setup(vlc_va_t *, va_pool_t *, AVCodecContext *, unsigned count, int alignment);
int va_pool_Get(vlc_va_t *, picture_t *, va_pool_t *);
void va_surface_AddRef(vlc_va_surface_t *surface);
void va_surface_Release(vlc_va_surface_t *surface);

#endif /* AVCODEC_VA_SURFACE_INTERNAL_H */