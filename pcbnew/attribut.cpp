/******************************************/
/* Track editing: attribute flags editing */
/******************************************/

#include "fctsys.h"
#include "common.h"
#include "class_drawpanel.h"

#include "pcbnew.h"
#include "wxPcbStruct.h"
#include "protos.h"


/* Attribute change for 1 track segment.
 *  Attributes are
 *  SEGM_FIXE       protection against global delete
 *  SEGM_AR         AutoRouted segment
 */
void WinEDA_PcbFrame::Attribut_Segment( TRACK* track, wxDC* DC, bool Flag_On )
{
    if( track == NULL )
        return;

    OnModify();
    DrawPanel->CursorOff( DC );   // Erase cursor shape
    track->SetState( SEGM_FIXE, Flag_On );
    track->Draw( DrawPanel, DC, GR_OR | GR_SURBRILL );
    DrawPanel->CursorOn( DC );    // Display cursor shape
    track->DisplayInfo( this );
}


/* Attribute change for an entire track */
void WinEDA_PcbFrame::Attribut_Track( TRACK* track, wxDC* DC, bool Flag_On )
{
    TRACK* Track;
    int    nb_segm;

    if( (track == NULL ) || (track->Type() == TYPE_ZONE) )
        return;

    DrawPanel->CursorOff( DC );   // Erase cursor shape
    Track = Marque_Une_Piste( GetBoard(), track, &nb_segm, NULL, true );
    Trace_Une_Piste( DrawPanel, DC, Track, nb_segm, GR_OR | GR_SURBRILL );

    for( ; (Track != NULL) && (nb_segm > 0); nb_segm-- )
    {
        Track->SetState( SEGM_FIXE, Flag_On );
        Track->SetState( BUSY, OFF );
        Track = Track->Next();
    }

    DrawPanel->CursorOn( DC );    // Display cursor shape

    OnModify();
}


/* Modify the flag SEGM_FIXE according to Flag_On value,
 *  for all the segments related to net_code.
 *  if net_code < 0 all the segments are modified.
 */
void WinEDA_PcbFrame::Attribut_net( wxDC* DC, int net_code, bool Flag_On )
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

    DrawPanel->CursorOff( DC );     // Erase cursor shape
    while( Track )                  /* Flag change */
    {
        if( (net_code >= 0 ) && (net_code != Track->GetNet()) )
            break;

        OnModify();
        Track->SetState( SEGM_FIXE, Flag_On );
        Track->Draw( DrawPanel, DC, GR_OR | GR_SURBRILL );
        Track = Track->Next();
    }

    DrawPanel->CursorOn( DC );    // Display cursor shape
    OnModify();
}
