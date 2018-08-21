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

/* Creates a new chain */
Chain* newChain(float f_inp)
{
    Chain* p_this = malloc( sizeof( Chain ));
    if ( p_this )
        p_this->f_value = f_inp;
    return p_this;
}
/* Creates a new chained list */
Chain* initFloatChain()
{
    Chain* p_head = newChain( 0.f );
    if ( p_head )
        vlc_list_init( &p_head->p_nodes );
    return p_head;
}
/* Destroyer */
void destroyFloatChain( Chain* p_head )
{
    while( !vlc_list_is_empty( &p_head->p_nodes ) )
        specificDeleter( vlc_list_entry( p_head->p_nodes.next, Chain, p_nodes ) );

    free(p_head);
}

void push( Chain* p_head, float i_inp )
{
    Chain* p_nuChain = newChain(i_inp);
    vlc_list_prepend( &p_nuChain->p_nodes, &p_head->p_nodes );
}

void pushBack( Chain* p_head, float i_inp )
{
    Chain* p_nuChain = newChain(i_inp);
    vlc_list_append( &p_nuChain->p_nodes, &p_head->p_nodes );
}

float specificDeleter( Chain* p_toRemove )
{
    float f_ret = p_toRemove->f_value;
    vlc_list_remove( &p_toRemove->p_nodes );
    free( p_toRemove );
    p_toRemove = NULL;
    return f_ret;
}

float pop( Chain* p_head )
{
    struct vlc_list* p_toRemove = p_head->p_nodes.next;
    return specificDeleter( vlc_list_entry( p_toRemove, struct Chain, p_nodes ) );
}

float popBack( Chain* p_head  )
{
    return specificDeleter( vlc_list_entry( p_head->p_nodes.prev, struct Chain, p_nodes ) );
}