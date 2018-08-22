/*****************************************************************************
 * floatChain.h: float double chained list/Queue  for the samples
 *****************************************************************************
 * Copyright © 2009-2011 VLC authors and VideoLAN
 * $Id$
 *
 * Authors: Elno Dal Bianco
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
#ifndef FLOAT_CHAIN_H
#define FLOAT_CHAIN_H 1

#include <vlc_es.h>

typedef struct CircularQueue
{
    unsigned int i_size;
    unsigned int i_head;
    unsigned int i_tail;
    float* p_array;
} CircularQueue;

CircularQueue* initCircularQueue( unsigned int );
void destroyCircularQueue( CircularQueue* );
void resizeCircularQueue( CircularQueue*, unsigned int );

void push( CircularQueue*, float );
float pop( CircularQueue* );

#endif