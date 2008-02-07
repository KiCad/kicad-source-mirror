/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2007-2008 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2007 Kicad Developers, see change_log.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */


/*  This source is a complement to specctra.cpp and implements the import of
    a specctra session file (*.ses), and import of a specctra design file
    (*.dsn) file.  The specification for the grammar of the specctra dsn file
    used to develop this code is given here:
    http://www.autotraxeda.com/docs/SPECCTRA/SPECCTRA.pdf

    Also see the comments at the top of the specctra.cpp file itself.
*/


#include "specctra.h"
#include "common.h"             // IsOK() & EDA_FileSelector()



using namespace DSN;

void WinEDA_PcbFrame::ImportSpecctraDesign( wxCommandEvent& event )
{
    /* @todo write this someday

    if( !Clear_Pcb( true ) )
        return;
    */
}


void WinEDA_PcbFrame::ImportSpecctraSession( wxCommandEvent& event )
{
/*
    if( GetScreen()->IsModify() )
    {
        if( !IsOK( this, _( "Board Modified: Continue ?" ) ) )
            return;
    }
*/

    wxString sessionExt( wxT( ".ses" ) );
    wxString fileName = GetScreen()->m_FileName;
    wxString mask = wxT( "*" ) + sessionExt;

    ChangeFileNameExt( fileName, sessionExt );

    fileName = EDA_FileSelector( _( "Merge Specctra Session file:" ),
                          wxEmptyString,
                          fileName,
                          sessionExt,
                          mask,
                          this,
                          wxFD_OPEN,
                          FALSE );

    if( fileName == wxEmptyString )
        return;

    SPECCTRA_DB     db;

    setlocale( LC_NUMERIC, "C" );    // Switch the locale to standard C

    try
    {
        db.LoadSESSION( fileName );
        db.FromSESSION( m_Pcb );
    }
    catch( IOError ioe )
    {
        setlocale( LC_NUMERIC, "" );    // revert to the current locale
        DisplayError( this, ioe.errorText );
        return;
    }

    setlocale( LC_NUMERIC, "" );    // revert to the current locale

    m_SelTrackWidthBox_Changed = TRUE;
    m_SelViaSizeBox_Changed    = TRUE;

    GetScreen()->SetModify();
    m_Pcb->m_Status_Pcb = 0;

    Affiche_Message( wxString( _("Session file imported and merged OK.")) );

    DrawPanel->Refresh( TRUE );
}


namespace DSN {


static wxPoint mapPt( const POINT& aPoint, UNIT_RES* aResolution )
{
    wxPoint ret;

    double  resValue = aResolution->GetValue();

    double  factor;     // multiply this times units to get mils for Kicad.

    switch( aResolution->GetEngUnits() )
    {
    default:
    case T_inch:
        factor = 0.001;
        break;
    case T_mil:
        factor = 1.0;
        break;
    case T_cm:
        factor = 2.54/1000.0;
        break;
    case T_mm:
        factor = 25.4/1000.0;
        break;
    case T_um:
        factor = 25.4;
        break;
    }

    // the factor of 10.0 is used to convert mils to deci-mils, the units
    // used within Kicad.
    factor *= 10.0;

    ret.x = (int)  (factor * aPoint.x / resValue);
    ret.y = (int) -(factor * aPoint.y / resValue);  // negate y coord

    return ret;
}


// no UI code in this function, throw exception to report problems to the
// UI handler: void WinEDA_PcbFrame::ImportSpecctraSession( wxCommandEvent& event )

void SPECCTRA_DB::FromSESSION( BOARD* aBoard ) throw( IOError )
{
    //wxASSERT( session );

    if( !session )
        ThrowIOError( _("Session file is missing the \"session\" section") );

    if( !session->placement )
        ThrowIOError( _("Session file is missing the \"placement\" section") );

    if( !session->route )
        ThrowIOError( _("Session file is missing the \"routes\" section") );

    if( !session->route->library )
        ThrowIOError( _("Session file is missing the \"library_out\" section") );

    // delete all the old tracks and vias
    aBoard->m_Track->DeleteStructList();
    aBoard->m_Track = NULL;
    aBoard->m_NbSegmTrack = 0;

    aBoard->DeleteMARKERs();

    // Walk the PLACEMENT object's COMPONENTs list, and for each PLACE within
    // each COMPONENT, reposition and re-orient each component and put on
    // correct side of the board.
    COMPONENTS& components = session->placement->components;
    for( COMPONENTS::iterator comp=components.begin();  comp!=components.end();  ++comp )
    {
        PLACES& places = comp->places;
        for( unsigned i=0; i<places.size();  ++i )
        {
            PLACE* place = &places[i];  // '&' even though places[] holds a pointer!

            wxString reference = CONV_FROM_UTF8( place->component_id.c_str() );
            MODULE* module = aBoard->FindModuleByReference( reference );
            if( !module )
            {
                ThrowIOError(
                   _("Session file has 'reference' to non-existent component \"%s\""),
                   reference.GetData() );
            }

            if( !place->hasVertex )
                continue;

            UNIT_RES* resolution = place->GetUnits();
            wxASSERT( resolution );

            wxPoint newPos = mapPt( place->vertex, resolution );
            module->SetPosition( newPos );

            if( place->side == T_front )
            {
                // convert from degrees to tenths of degrees used in Kicad.
                int orientation = (int) (place->rotation * 10.0);

                if( module->GetLayer() != CMP_N )
                {
                    // module is on copper layer (back)
                    aBoard->Change_Side_Module( module, 0 );
                }
                module->SetOrientation( orientation );
            }
            else if( place->side == T_back )
            {
                int orientation = (place->rotation + 180.0) * 10.0;
                if( module->GetLayer() != COPPER_LAYER_N )
                {
                    // module is on component layer (front)
                    aBoard->Change_Side_Module( module, 0 );
                }
                module->SetOrientation( orientation );
            }
            else
            {
                // as I write this, the LEXER *is* catching this, so we should never see below:
                wxFAIL_MSG( wxT("DSN::LEXER did not catch an illegal side := 'back|front'") );
            }
        }
    }

    // Walk the NET_OUTs and create tracks and vias anew.
    NET_OUTS& net_outs = session->route->net_outs;
    for( NET_OUTS::iterator i=net_outs.begin();  i!=net_outs.end();  ++i )
    {
        // create a track or via and position it.


    }
}


} // namespace DSN

