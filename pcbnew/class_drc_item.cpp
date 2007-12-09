/*************************************************/
/* class_drc_item.cpp - DRC_ITEM class functions */
/*************************************************/
#include "fctsys.h"
#include "common.h"

#include "pcbnew.h"

wxString DRC_ITEM::GetErrorText() const
{
    switch( m_ErrorCode )
    {
//    case DRCE_:    not assigned yet
        
    case DRCE_UNCONNECTED_PADS:
        return wxString( _("Unconnected pads") );
    case DRCE_TRACK_NEAR_THROUGH_HOLE:
        return wxString( _("Track near thru-hole") ); 
    case DRCE_TRACK_NEAR_PAD:
        return wxString( _("Track near pad") );
    case DRCE_TRACK_NEAR_VIA:
        return wxString( _("Track near via") );
    case DRCE_VIA_NEAR_VIA:
        return wxString( _("Via near via") );
    case DRCE_VIA_NEAR_TRACK:
        return wxString( _("Via near track") );
    case DRCE_TRACK_ENDS1:
    case DRCE_TRACK_ENDS2:
    case DRCE_TRACK_ENDS3:
    case DRCE_TRACK_ENDS4:
    case DRCE_ENDS_PROBLEM1: 
    case DRCE_ENDS_PROBLEM2:
    case DRCE_ENDS_PROBLEM3:
    case DRCE_ENDS_PROBLEM4:
    case DRCE_ENDS_PROBLEM5:
        return wxString( _("Two track ends") );
    case DRCE_TRACK_UNKNOWN1:
        return wxString( _("This looks bad") );  ///< @todo check source code and change this comment
    case DRCE_TRACKS_CROSSING:
        return wxString( _("Tracks crossing") );
    case DRCE_PAD_NEAR_PAD1:
        return wxString( _("Pad near pad") );
    case DRCE_VIA_HOLE_BIGGER:
        return wxString( _("Via hole > diameter"));
        
    default:
        return wxString( wxT("PROGRAM BUG, PLEASE LEAVE THE ROOM.") );
    }
}


wxString DRC_ITEM::ShowCoord( const wxPoint& aPos )
{
    wxString temp;
    wxString ret;

    ret << wxT("@(") << valeur_param( aPos.x, temp ); 
    ret << wxT(",")  << valeur_param( aPos.y, temp );
    ret << wxT(")");

    return ret;
}

