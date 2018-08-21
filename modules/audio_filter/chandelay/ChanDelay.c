/*****************************************************************************
 * ChanDelay.c: Multichannel delay
 *****************************************************************************
 * Copyright © 2018 VLC authors and VideoLAN
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

/*TO DO : WE'RE USING METERS FOR DISTANCE, ADD YARDS/FEET/WHATEVER THE CORRECT UNIT IS FOR AMERICANS*/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
 
/* VLC core API headers */
#include <vlc_aout.h>
#include <vlc_common.h>
#include <vlc_filter.h>
#include <vlc_plugin.h>
#include <vlc_interface.h>

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "floatChain.h"

/* Forward declarations */
/* Main functions */
static int Open( vlc_object_t * );
static void Close( vlc_object_t * );
static block_t  *Process ( filter_t *, block_t * );

/*WHERE WE KEEP THE PARAMETERS*/
typedef struct
{
    bool b_distance; /*is distance mode on?*/
    bool b_fahrenheit; /*is the teperature in fahrenheit*/
    float f_temperature; 
    unsigned int i_nbChannel;
    int i_sampleRate;
    float* p_delayValues; /*the time we need to delay*/
    CircularQueue** pp_stack; /* Where we keep the delayed values*/
    vlc_mutex_t* p_lock; /*Mutex for the callbacks*/
} filter_sys_t;

/* We're using 500ms as the maximum delay time, as it can sync up speakers
 * About 125 meters apart (for a temperature of 35C).
 * Feel free to change that if you wish, the program will work the same*/
#define MAX_TIME 500

/* Module descriptor */
vlc_module_begin()
    set_shortname(N_( "ChanDelay" ))
    set_description(N_( "Allows to independently delay audio channels" ))
    set_capability( "audio filter", 0 )
    set_category( CAT_AUDIO );
    set_subcategory( SUBCAT_AUDIO_AFILTER )
    set_callbacks( Open, Close )

/* The callbacks, I don't believe there's a way to dynamically allocate those
 * so I'm assming the max number of channels is 8
 * If you want/need to change this you'll need to copy-paste and change NB_CALLBACKS*/
#define NB_CALLBACKS 8
    add_float_with_range( "delay-chan-1", 0,0,MAX_TIME, "Delay Channel 1",
        N_( "Time in milliseconds of the sound delay on this channel" ), true )
    add_float_with_range( "delay-chan-2", 0,0,MAX_TIME, "Delay Channel 2",
        N_( "Time in milliseconds of the sound delay on this channel" ), true )
    add_float_with_range( "delay-chan-3", 0,0,MAX_TIME, "Delay Channel 3",
        N_( "Time in milliseconds of the sound delay on this channel" ), true )
    add_float_with_range( "delay-chan-4", 0,0,MAX_TIME, "Delay Channel 4",
        N_( "Time in milliseconds of the sound delay on this channel" ), true )
    add_float_with_range( "delay-chan-5", 0,0,MAX_TIME, "Delay Channel 5",
        N_("Time in milliseconds of the sound delay on this channel") , true )
    add_float_with_range( "delay-chan-6", 0,0,MAX_TIME, "Delay Channel 6",
        N_( "Time in milliseconds of the sound delay on this channel" ), true )
    add_float_with_range( "delay-chan-7", 0,0,MAX_TIME, "Delay Channel 7",
        N_( "Time in milliseconds of the sound delay on this channel" ), true )
    add_float_with_range( "delay-chan-8", 0,0,MAX_TIME, "Delay Channel 8",
        N_( "Time in milliseconds of the sound delay on this channel" ), true )

    add_bool( "mode-distance", false, N_( "Use distance" ),
        N_( "If this is set the delay will be calculated from the values in Delay Channel"), true  )
    add_bool( "fahrenheit", false, N_( "Fahrenheit" ),
        N_( "If your temperature is in farenheit check this" ), true)
    add_float( "temperature", 20, "Temperature"
      , N_( "Only necessary if the module is in distance mode" ), true )


vlc_module_end ()

/* Helper functions 
 *
 *Self explanatory, gets us the number of samples in a specified time*/
static int milisecToNbSamp( int i_time, int i_sampleRate )
{
    return  i_time * ((float) i_sampleRate / 1000 );
}

/*Finds the time sound takes to cross a distance*/
static float distToMilisec( float f_dist, float f_temp, bool b_fah )
{
    float f_realTemp = f_temp;
    if (b_fah)
        f_realTemp = ( ( f_realTemp - 32 ) * ( 5 / 9 ) );
    f_realTemp += 273.15;

    return  ( f_dist / ( 20.75 * sqrt( f_realTemp )) ) * 1000;
}

/* Adds silence to the queue, that's what creates the delay */
static void addNullSamples( filter_t* p_this, int i_nbChannel, int nbSamp )
{
    if( nbSamp > 0 )
    {
        filter_sys_t* p_sys = p_this->p_sys;
        for( int i = 0; i < nbSamp; i++ )
            push( p_sys->pp_stack[i_nbChannel], 0.f );
    }
    msg_Dbg(p_this, "%d silent samples added to channel %d", nbSamp, i_nbChannel );
}

/* Allows to change the "size" of the delay on one channel*/
static void updateChannelBuf( filter_t* p_delay, int i_channelACT, float diffVal )
{
    filter_sys_t* p_sys = p_delay->p_sys;
    resizeCircularQueue( p_sys->pp_stack[i_channelACT]
                         , p_sys->pp_stack[i_channelACT]->i_size 
                           + milisecToNbSamp(diffVal, p_sys->i_sampleRate) );

    if ( diffVal > 0 ) /* If the value increased */
    {
        addNullSamples( p_delay /* We add silence */
                       , i_channelACT
                       , milisecToNbSamp(diffVal, p_sys->i_sampleRate));
    }

    else if ( diffVal < 0 ) /* If the value decreased */
    {
        int nbSamplesToPush = milisecToNbSamp(diffVal*-1, p_sys->i_sampleRate);
        for ( int smp = 0; smp < nbSamplesToPush; smp++ ) /* We take out as much */
            pop( p_sys->pp_stack[i_channelACT] );                   /* as needed */
    }
}

/*Mode Initializers 
 *
 *switcher for "distance mode"*/

static void distanceModeInit( filter_t* p_this )
{
    filter_sys_t* p_sys = p_this->p_sys; 

    /*Step 1, get the largest value*/
    float big = 0;
    for ( unsigned int i = 0; i < p_sys->i_nbChannel; i++ )
    {
        if ( p_sys->p_delayValues[i] > big )
            big = p_sys->p_delayValues[i];
    }

    /*Step 2, modify queue using new info*/
    for ( unsigned int i = 0; i < p_sys->i_nbChannel; i++)
    {

        float temp = distToMilisec( big - p_sys->p_delayValues[i]
                                     , p_sys->f_temperature
                                     , p_sys->b_fahrenheit );
        /*temp -=  p_sys->p_delayValues[i];*/
        updateChannelBuf( p_this, i, temp );
    }
}

/*switcher for "time mode"*/
static void msModeInit( filter_t* p_this )
{
    filter_sys_t* p_sys = p_this->p_sys; 

    char** s_nameVariable = malloc( NB_CALLBACKS * sizeof(char*) );
    float big = 0;
    for( unsigned int i = 0; i < p_sys->i_nbChannel; i++ )
    {
        char* name = malloc( (strlen("delay-chan-X") + 1) * sizeof(char) );
        sprintf( s_nameVariable[i], "delay-chan-%d", i+1 );
        
        p_sys->p_delayValues[i] = var_GetFloat( p_this, s_nameVariable[i] );

        if ( p_sys->p_delayValues[i] > big )
            big = p_sys->p_delayValues[i];

        free(name);
    }
    free( s_nameVariable );

    for ( unsigned int i = 0; i < p_sys->i_nbChannel; i++ )
    {
        float temp = distToMilisec( big - p_sys->p_delayValues[i]
                                     , p_sys->f_temperature
                                     , p_sys->b_fahrenheit );

        updateChannelBuf( p_this, i, p_sys->p_delayValues[i] - temp );
    }
}

/* Callbacks */
/* If the mode is changed */
static int updateMode( vlc_object_t *p_this
                       , char const *psz_var
                       , vlc_value_t oldval
                       , vlc_value_t newval
                       , void *p_data )
{
    VLC_UNUSED(psz_var);
    VLC_UNUSED(oldval);
    VLC_UNUSED(p_data);

    filter_t* p_delay = (filter_t *)p_this;
    filter_sys_t* p_sys = p_delay->p_sys;

    vlc_mutex_lock( p_sys->p_lock );
    p_sys->b_distance = newval.b_bool;

    if (p_sys->b_distance)
    {
        distanceModeInit( p_delay );
    }
    else
    {
        msModeInit( p_delay );
    }

    vlc_mutex_unlock( p_sys->p_lock );
    return VLC_SUCCESS;
}

/* Channel delay value Callback*/
static int updateChannelVal( vlc_object_t *p_this
                           , char const *psz_var
                           , vlc_value_t oldval
                           , vlc_value_t newval
                           , void *p_data)
{
    VLC_UNUSED(p_data);

    filter_t* p_delay = (filter_t *)p_this;
    filter_sys_t* p_sys = p_delay->p_sys;
    bool b_isDone = false;

    unsigned int i = 0;
    /*We first find out which parameter was changed*/
    while ( !(b_isDone) && i < (p_sys->i_nbChannel) )
    {
        if( psz_var[11] == (( char )( i + '0' ) ))
        {
            vlc_mutex_lock( p_sys->p_lock );

            float diffVal;
            if (p_sys->b_distance)
            {
                p_sys->p_delayValues[i] = newval.f_float;
                distanceModeInit( p_delay );
                return VLC_SUCCESS;
            }
            else
            {
                diffVal = newval.f_float - oldval.f_float;
            }
            updateChannelBuf( p_delay, i, diffVal );
            p_sys->p_delayValues[i] = newval.f_float;

            vlc_mutex_unlock( p_sys->p_lock );
            
            b_isDone = true;
        }
        i++;
    }
    return VLC_SUCCESS;
}



/* if the temperature has changed */
static int updateTemp( vlc_object_t *p_this
                       , char const *psz_var
                       , vlc_value_t oldval
                       , vlc_value_t newval
                       , void *p_data )
{
    VLC_UNUSED(psz_var);
    VLC_UNUSED(oldval);
    VLC_UNUSED(p_data);

    filter_t* p_delay = (filter_t *)p_this;
    filter_sys_t* p_sys = p_delay->p_sys;

    vlc_mutex_lock( p_sys->p_lock );
    p_sys->f_temperature = newval.f_float;

    if (p_sys->b_distance)
        distanceModeInit( p_delay );

    vlc_mutex_unlock( p_sys->p_lock );

    return VLC_SUCCESS;
}

//lol

/* If we switched to a fahrenheit temperature */
static int updateFah( vlc_object_t *p_this
                      , char const *psz_var
                      , vlc_value_t oldval
                      , vlc_value_t newval
                      , void *p_data )
{
    VLC_UNUSED(psz_var);
    VLC_UNUSED(oldval);
    VLC_UNUSED(p_data);

    filter_t* p_delay = (filter_t *)p_this;
    filter_sys_t* p_sys = p_delay->p_sys;

    vlc_mutex_lock( p_sys->p_lock );
    p_sys->b_fahrenheit = newval.b_bool;

    if (p_sys->b_distance)
        distanceModeInit( p_delay );

    vlc_mutex_unlock( p_sys->p_lock );

    return VLC_SUCCESS;
}

/* Main functions 
 *
 *CONSTRUCTOR FUNCTION*/
static int Open ( vlc_object_t * p_this )
{
    /*Init basic variables and this*/
    msg_Dbg(p_this , "Initializing...");
    filter_t* p_delay = ( filter_t* ) p_this;
    p_delay->p_sys = malloc( sizeof(filter_sys_t) );
    filter_sys_t* p_sys = p_delay->p_sys;

    if ( !p_sys )
        return VLC_EGENERIC;


    p_sys->i_nbChannel = aout_FormatNbChannels( &p_delay->fmt_in.audio );
    p_sys->i_sampleRate = p_delay->fmt_in.audio.i_rate;

    p_sys->p_lock = malloc( sizeof(vlc_mutex_t) );
    if ( !p_sys->p_lock )
    {
        free( p_sys );
        return VLC_EGENERIC;
    }
    vlc_mutex_init( p_sys->p_lock );

    p_sys->p_delayValues = malloc( NB_CALLBACKS * sizeof(float) );
    if ( !p_sys->p_delayValues )
    {
        free( p_sys->p_lock );
        free( p_sys );
        return VLC_EGENERIC;
    }

    /*Init Sample Buffer Initialisation*/
    p_sys->pp_stack = malloc( NB_CALLBACKS * sizeof( CircularQueue ) );
    if ( !p_sys->pp_stack )
    {
        free( p_sys->p_lock );
        free( p_sys->p_delayValues );
        free( p_sys );
        return VLC_EGENERIC;
    }
    for ( size_t i = 0; i < NB_CALLBACKS; i++ )
       p_sys->pp_stack[i] = initCircularQueue(1);
    

    /*We add the callbacks*/

    p_sys->b_distance = var_CreateGetBool( p_this, "mode-distance" );
    var_AddCallback( p_delay, "mode-distance", updateMode, p_sys );

    p_sys->b_fahrenheit = var_CreateGetBool( p_this, "fahrenheit" );
    var_AddCallback( p_delay, "fahrenheit", updateFah, p_sys );

    p_sys->f_temperature = var_CreateGetFloat( p_this, "temperature" );
    var_AddCallback( p_delay, "temperature", updateTemp, p_sys );

    /* Callbacks for the delay values and buffer initialization*/
    char* p_nameVariable[NB_CALLBACKS];
    for ( unsigned int i = 0 ; i < NB_CALLBACKS ; i++ )
    {
        if ( asprintf( &p_nameVariable[i], "delay-chan-%d", i + 1 ) == -1 )
            return VLC_EGENERIC;
        p_sys->p_delayValues[i] = var_CreateGetFloat( p_this
                                                      , p_nameVariable[i] );
        var_AddCallback( p_delay, p_nameVariable[i], updateChannelVal, p_sys );
        
        if ( !p_sys->b_distance )
        {
            updateChannelBuf(p_delay, i, p_sys->p_delayValues[i] );
        }
        free(p_nameVariable[i]);
    }

    if ( p_sys->b_distance )
        distanceModeInit( p_delay );

    /* Final few details */
    p_delay->pf_audio_filter = Process;
    p_delay->fmt_out.audio = p_delay->fmt_in.audio;
    
    msg_Dbg(p_this , "Successfull initialization");
    return VLC_SUCCESS;
}

/*Destrctor fuction*/
static void Close ( vlc_object_t* p_this )
{
    filter_t* p_delay = (filter_t *) p_this;
    filter_sys_t* p_sys = p_delay->p_sys;

    free( p_sys->p_delayValues );
    for( size_t i = 0; i < NB_CALLBACKS; i++ )
    {
        destroyCircularQueue( p_sys->pp_stack[i] );
    }
    free( p_sys->pp_stack );

    vlc_mutex_destroy( p_sys->p_lock );
    free( p_sys->p_lock );
    free( p_delay->p_sys );
}

/*Processing incoming audio*/
static block_t *Process ( filter_t * p_flt, block_t * p_in)
{ 
    filter_sys_t* p_sys = p_flt->p_sys;

    unsigned int i_channelACT = 0;
    float f_danglingValue = 0;
    int i_sizeFlot = sizeof(float);

    /*For each sample in the block...
    * 1. Add it to the Queue/Buffer
    * 2. Replace it's value by the first one in the Queue*/
    float *spl = (float *)p_in->p_buffer;
    for( unsigned int smp = (p_in->i_nb_samples * p_sys->i_nbChannel);
             smp > 0;
             smp-- )
    {
        /*1*/
        memcpy( (float*)& f_danglingValue, spl, i_sizeFlot );
        push( p_sys->pp_stack[i_channelACT], f_danglingValue ); 

        /*2*/
        f_danglingValue = pop( p_sys->pp_stack[i_channelACT] );
        memcpy( spl, & f_danglingValue, i_sizeFlot );

        i_channelACT++;
        if (i_channelACT >= p_sys->i_nbChannel)
            i_channelACT = 0;
        spl++;
    }

    return p_in;
}