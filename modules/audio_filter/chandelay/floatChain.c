/*****************************************************************************
 * floatChain.c: float double chained list/Queue  for the samples
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
#include <stdlib.h>
#include <stdio.h>
#include "floatChain.h"

/* Creates a new chained list */
CircularQueue* initCircularQueue( unsigned int i_size )
{
    CircularQueue* p_this = malloc( sizeof(CircularQueue) );
    if( p_this )
    {
        p_this->p_array = malloc( (i_size + 5) * sizeof(float) );
        if ( !p_this->p_array )
            return NULL;

        p_this->i_size = i_size;

        p_this->i_head = 0;
        p_this->i_tail = 0;
    }
    return p_this;
}

/* Destroyer */
void destroyCircularQueue( CircularQueue* p_this )
{ 
    free( p_this->p_array );
    free( p_this );
}

void resizeCircularQueue( CircularQueue* p_this, unsigned int i_nuSize )
{
    float* p_nuQueue = realloc( p_this->p_array, sizeof(float) * i_nuSize);
    if ( p_nuQueue )
    {
        p_this->p_array = p_nuQueue;
        p_this->i_head %= i_nuSize;
        p_this->i_tail %= i_nuSize;
        p_this->i_size = i_nuSize;
    }
    else
    {
        // ON TROUE UN MOYEM ELEGANT DE FAIRE UNE ERREUR
    }
}

void push( CircularQueue* p_this, float i_inp )
{
    p_this->p_array[p_this->i_head] = i_inp;

    p_this->i_head = ( p_this->i_head + 1 ) % ( p_this->i_size );
}

float pop( CircularQueue* p_this )
{
    float f_temp = p_this->p_array[p_this->i_tail];

    p_this->i_tail = ( p_this->i_tail + 1 ) % ( p_this->i_size );

    return f_temp;
}