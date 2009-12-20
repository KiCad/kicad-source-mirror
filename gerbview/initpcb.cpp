/****************************************/
/******* initpcb.cpp ********************/
/****************************************/


#include "fctsys.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"

#include "gerbview.h"
#include "protos.h"


bool WinEDA_GerberFrame::Clear_Pcb( bool query )
{
    int layer;

    if( GetBoard() == NULL )
        return FALSE;

    if( query )
    {
        if( GetBoard()->m_Drawings || GetBoard()->m_Track
            || GetBoard()->m_Zone )
        {
            if( !IsOK( this, _( "Current data will be lost?" ) ) )
                return FALSE;
        }
    }

    GetBoard()->m_Drawings.DeleteAll();

    GetBoard()->m_Track.DeleteAll();

    GetBoard()->m_Zone.DeleteAll();

    for( layer = 0; layer < 32; layer++ )
    {
        if( g_GERBER_List[layer] )
            g_GERBER_List[layer]->InitToolTable();
    }

    GetBoard()->m_BoundaryBox.SetOrigin( 0, 0 );
    GetBoard()->m_BoundaryBox.SetSize( 0, 0 );
    GetBoard()->m_Status_Pcb  = 0;
    GetBoard()->m_NbNodes     = 0;
    GetBoard()->m_NbNoconnect = 0;

    SetBaseScreen( ActiveScreen = ScreenPcb );
    GetScreen()->Init();

    return TRUE;
}


void WinEDA_GerberFrame::Erase_Current_Layer( bool query )
{
    int      layer = GetScreen()->m_Active_Layer;
    wxString msg;

    msg.Printf( _( "Delete layer %d?" ), layer + 1 );
    if( query && !IsOK( this, msg ) )
        return;

    /* Delete tracks (spots and lines) */
    TRACK* PtNext;
    for( TRACK* pt_segm = GetBoard()->m_Track;
         pt_segm != NULL;
         pt_segm = (TRACK*) PtNext )
    {
        PtNext = pt_segm->Next();
        if( pt_segm->GetLayer() != layer )
            continue;
        pt_segm->DeleteStructure();
    }

    /* Delete polygons */
    SEGZONE* Nextzone;
    for( SEGZONE* zone = GetBoard()->m_Zone; zone != NULL; zone = Nextzone )
    {
        Nextzone = zone->Next();
        if( zone->GetLayer() != layer )
            continue;
        zone->DeleteStructure();
    }

    ScreenPcb->SetModify();
    ScreenPcb->SetRefreshReq();
}
