/******************************************/
/* Track editing: attribute flags editing */
/******************************************/

#include "fctsys.h"
#include "class_drawpanel.h"
#include "gr_basic.h"
#include "wxPcbStruct.h"

#include "pcbnew.h"
#include "protos.h"

#include "class_track.h"
#include "class_board.h"


/* Attribute change for 1 track segment.
 *  Attributes are
 *  TRACK_LOCKED       protection against global delete
 *  TRACK_AR           AutoRouted segment
 */
void PCB_EDIT_FRAME::Attribut_Segment( TRACK* track, wxDC* DC, bool Flag_On )
{
    if( track == NULL )
        return;

    OnModify();
    DrawPanel->CrossHairOff( DC );   // Erase cursor shape
    track->SetState( TRACK_LOCKED, Flag_On );
    track->Draw( DrawPanel, DC, GR_OR | GR_SURBRILL );
    DrawPanel->CrossHairOn( DC );    // Display cursor shape
    track->DisplayInfo( this );
}


/* Attribute change for an entire track */
void PCB_EDIT_FRAME::Attribut_Track( TRACK* track, wxDC* DC, bool Flag_On )
{
    TRACK* Track;
    int    nb_segm;

    if( (track == NULL ) || (track->Type() == PCB_ZONE_T) )
        return;

    DrawPanel->CrossHairOff( DC );   // Erase cursor shape
    Track = GetBoard()->MarkTrace( track, &nb_segm, NULL, NULL, true );
    DrawTraces( DrawPanel, DC, Track, nb_segm, GR_OR | GR_SURBRILL );

    for( ; (Track != NULL) && (nb_segm > 0); nb_segm-- )
    {
        Track->SetState( TRACK_LOCKED, Flag_On );
        Track->SetState( BUSY, OFF );
        Track = Track->Next();
    }

    DrawPanel->CrossHairOn( DC );    // Display cursor shape

    OnModify();
}


/* Modify the flag TRACK_LOCKED according to Flag_On value,
 *  for all the segments related to net_code.
 *  if net_code < 0 all the segments are modified.
 */
void PCB_EDIT_FRAME::Attribut_net( wxDC* DC, int net_code, bool Flag_On )
{
    TRACK* Track = GetBoard()->m_Track;

    /* search the first segment for the selected net_code */
    if( net_code >= 0 )
    {
        for( ; Track != NULL; Track = Track->Next() )
        {
            if( net_code == Track->GetNet() )
                break;
        }
    }

    DrawPanel->CrossHairOff( DC );     // Erase cursor shape

    while( Track )                  /* Flag change */
    {
        if( (net_code >= 0 ) && (net_code != Track->GetNet()) )
            break;

        OnModify();
        Track->SetState( TRACK_LOCKED, Flag_On );
        Track->Draw( DrawPanel, DC, GR_OR | GR_SURBRILL );
        Track = Track->Next();
    }

    DrawPanel->CrossHairOn( DC );    // Display cursor shape
    OnModify();
}
