/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2015-2017 KiCad Developers, see change_log.txt for contributors.
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

#include <base_units.h>
#include <kiway.h>
#include <class_drawpanel.h>
#include <confirm.h>

#include <class_library.h>
#include <eeschema_id.h>
#include <libeditframe.h>
#include <viewlib_frame.h>
#include <sch_base_frame.h>
#include <symbol_lib_table.h>
#include "dialogs/dialog_sym_lib_table.h"



LIB_ALIAS* SchGetLibAlias( const LIB_ID& aLibId, SYMBOL_LIB_TABLE* aLibTable, PART_LIB* aCacheLib,
                           wxWindow* aParent, bool aShowErrorMsg )
{
    // wxCHECK_MSG( aLibId.IsValid(), NULL, "LIB_ID is not valid." );
    wxCHECK_MSG( aLibTable, NULL, "Invalid symbol library table." );

    LIB_ALIAS* alias = NULL;

    try
    {
        alias = aLibTable->LoadSymbol( aLibId );

        if( !alias && aCacheLib )
            alias = aCacheLib->FindAlias( aLibId );
    }
    catch( const IO_ERROR& ioe )
    {
        if( aShowErrorMsg )
        {
            wxString msg;

            msg.Printf( _( "Could not load symbol '%s' from library '%s'." ),
                        aLibId.GetLibItemName().wx_str(), aLibId.GetLibNickname().wx_str() );
            DisplayErrorMessage( aParent, msg, ioe.What() );
        }
    }

    return alias;
}


LIB_PART* SchGetLibPart( const LIB_ID& aLibId, SYMBOL_LIB_TABLE* aLibTable, PART_LIB* aCacheLib,
                         wxWindow* aParent, bool aShowErrorMsg )
{
    LIB_ALIAS* alias = SchGetLibAlias( aLibId, aLibTable, aCacheLib, aParent, aShowErrorMsg );

    return ( alias ) ? alias->GetPart() : NULL;
}


// Sttaic members:

SCH_BASE_FRAME::SCH_BASE_FRAME( KIWAY* aKiway, wxWindow* aParent,
        FRAME_T aWindowType, const wxString& aTitle,
        const wxPoint& aPosition, const wxSize& aSize, long aStyle,
        const wxString& aFrameName ) :
    EDA_DRAW_FRAME( aKiway, aParent, aWindowType, aTitle, aPosition,
            aSize, aStyle, aFrameName )
{
    m_zoomLevelCoeff = 11.0;    // Adjusted to roughly displays zoom level = 1
                                // when the screen shows a 1:1 image
                                // obviously depends on the monitor,
                                // but this is an acceptable value
    m_repeatStep = wxPoint( DEFAULT_REPEAT_OFFSET_X, DEFAULT_REPEAT_OFFSET_Y );
    m_repeatDeltaLabel = DEFAULT_REPEAT_LABEL_INC;
}



SCH_BASE_FRAME::~SCH_BASE_FRAME()
{
}


void SCH_BASE_FRAME::OnOpenLibraryViewer( wxCommandEvent& event )
{
    LIB_VIEW_FRAME* viewlibFrame = (LIB_VIEW_FRAME*) Kiway().Player( FRAME_SCH_VIEWER, true );

    viewlibFrame->PushPreferences( m_canvas );

    // On Windows, Raise() does not bring the window on screen, when iconized
    if( viewlibFrame->IsIconized() )
        viewlibFrame->Iconize( false );

    viewlibFrame->Show( true );
    viewlibFrame->Raise();
}


// Virtual from EDA_DRAW_FRAME
COLOR4D SCH_BASE_FRAME::GetDrawBgColor() const
{
    return GetLayerColor( LAYER_SCHEMATIC_BACKGROUND );
}


void SCH_BASE_FRAME::SetDrawBgColor( COLOR4D aColor )
{
    m_drawBgColor= aColor;
    SetLayerColor( aColor, LAYER_SCHEMATIC_BACKGROUND );
}


SCH_SCREEN* SCH_BASE_FRAME::GetScreen() const
{
    return (SCH_SCREEN*) EDA_DRAW_FRAME::GetScreen();
}


const wxString SCH_BASE_FRAME::GetZoomLevelIndicator() const
{
    return EDA_DRAW_FRAME::GetZoomLevelIndicator();
}


void SCH_BASE_FRAME::SetPageSettings( const PAGE_INFO& aPageSettings )
{
    GetScreen()->SetPageSettings( aPageSettings );
}


const PAGE_INFO& SCH_BASE_FRAME::GetPageSettings () const
{
    return GetScreen()->GetPageSettings();
}


const wxSize SCH_BASE_FRAME::GetPageSizeIU() const
{
    // GetSizeIU is compile time dependent:
    return GetScreen()->GetPageSettings().GetSizeIU();
}


const wxPoint& SCH_BASE_FRAME::GetAuxOrigin() const
{
    wxASSERT( GetScreen() );
    return GetScreen()->GetAuxOrigin();
}


void SCH_BASE_FRAME::SetAuxOrigin( const wxPoint& aPosition )
{
    wxASSERT( GetScreen() );
    GetScreen()->SetAuxOrigin( aPosition );
}


const TITLE_BLOCK& SCH_BASE_FRAME::GetTitleBlock() const
{
    wxASSERT( GetScreen() );
    return GetScreen()->GetTitleBlock();
}


void SCH_BASE_FRAME::SetTitleBlock( const TITLE_BLOCK& aTitleBlock )
{
    wxASSERT( GetScreen() );
    GetScreen()->SetTitleBlock( aTitleBlock );
}


void SCH_BASE_FRAME::UpdateStatusBar()
{
    wxString        line;
    int             dx, dy;
    BASE_SCREEN*    screen = GetScreen();

    if( !screen )
        return;

    EDA_DRAW_FRAME::UpdateStatusBar();

    // Display absolute coordinates:
    double dXpos = To_User_Unit( g_UserUnit, GetCrossHairPosition().x );
    double dYpos = To_User_Unit( g_UserUnit, GetCrossHairPosition().y );

    if ( g_UserUnit == MILLIMETRES )
    {
        dXpos = RoundTo0( dXpos, 100.0 );
        dYpos = RoundTo0( dYpos, 100.0 );
    }

    wxString absformatter;
    wxString locformatter;

    switch( g_UserUnit )
    {
    case INCHES:
        absformatter = wxT( "X %.3f  Y %.3f" );
        locformatter = wxT( "dx %.3f  dy %.3f  dist %.3f" );
        break;

    case MILLIMETRES:
        absformatter = wxT( "X %.2f  Y %.2f" );
        locformatter = wxT( "dx %.2f  dy %.2f  dist %.2f" );
        break;

    case UNSCALED_UNITS:
        absformatter = wxT( "X %f  Y %f" );
        locformatter = wxT( "dx %f  dy %f  dist %f" );
        break;

    case DEGREES:
        wxASSERT( false );
        break;
    }

    line.Printf( absformatter, dXpos, dYpos );
    SetStatusText( line, 2 );

    // Display relative coordinates:
    dx = GetCrossHairPosition().x - screen->m_O_Curseur.x;
    dy = GetCrossHairPosition().y - screen->m_O_Curseur.y;

    dXpos = To_User_Unit( g_UserUnit, dx );
    dYpos = To_User_Unit( g_UserUnit, dy );

    if( g_UserUnit == MILLIMETRES )
    {
        dXpos = RoundTo0( dXpos, 100.0 );
        dYpos = RoundTo0( dYpos, 100.0 );
    }

    // We already decided the formatter above
    line.Printf( locformatter, dXpos, dYpos, hypot( dXpos, dYpos ) );
    SetStatusText( line, 3 );

    // refresh units display
    DisplayUnitsMsg();
}


void SCH_BASE_FRAME::OnEditSymbolLibTable( wxCommandEvent& aEvent )
{
    DIALOG_SYMBOL_LIB_TABLE dlg( this, &SYMBOL_LIB_TABLE::GetGlobalLibTable(),
                                 Prj().SchSymbolLibTable() );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    saveSymbolLibTables( true, true );

    LIB_EDIT_FRAME* editor = (LIB_EDIT_FRAME*) Kiway().Player( FRAME_SCH_LIB_EDITOR, false );

    if( this == editor )
    {
        // There may be no parent window so use KIWAY message to refresh the schematic editor
        // in case any symbols have changed.
        Kiway().ExpressMail( FRAME_SCH, MAIL_SCH_REFRESH, std::string( "" ), this );
    }

    LIB_VIEW_FRAME* viewer = (LIB_VIEW_FRAME*) Kiway().Player( FRAME_SCH_VIEWER, false );

    if( viewer )
        viewer->ReCreateListLib();
}


LIB_ALIAS* SCH_BASE_FRAME::GetLibAlias( const LIB_ID& aLibId, bool aUseCacheLib,
                                        bool aShowErrorMsg )
{
    // wxCHECK_MSG( aLibId.IsValid(), NULL, "LIB_ID is not valid." );

    PART_LIB* cache = ( aUseCacheLib ) ? Prj().SchLibs()->GetCacheLibrary() : NULL;

    return SchGetLibAlias( aLibId, Prj().SchSymbolLibTable(), cache, this, aShowErrorMsg );
}


LIB_PART* SCH_BASE_FRAME::GetLibPart( const LIB_ID& aLibId, bool aUseCacheLib, bool aShowErrorMsg )
{
    // wxCHECK_MSG( aLibId.IsValid(), NULL, "LIB_ID is not valid." );

    PART_LIB* cache = ( aUseCacheLib ) ? Prj().SchLibs()->GetCacheLibrary() : NULL;

    return SchGetLibPart( aLibId, Prj().SchSymbolLibTable(), cache, this, aShowErrorMsg );
}


bool SCH_BASE_FRAME::saveSymbolLibTables( bool aGlobal, bool aProject )
{
    bool success = true;

    if( aGlobal )
    {
        try
        {
            FILE_OUTPUTFORMATTER sf( SYMBOL_LIB_TABLE::GetGlobalTableFileName() );

            SYMBOL_LIB_TABLE::GetGlobalLibTable().Format( &sf, 0 );
        }
        catch( const IO_ERROR& ioe )
        {
            success = false;
            wxString msg = wxString::Format( _( "Error occurred saving the global symbol library "
                                                "table:\n\n%s" ),
                                            GetChars( ioe.What().GetData() ) );
            wxMessageBox( msg, _( "File Save Error" ), wxOK | wxICON_ERROR );
        }
    }

    if( aProject && !Prj().GetProjectName().IsEmpty() )
    {
        wxFileName fn( Prj().GetProjectPath(), SYMBOL_LIB_TABLE::GetSymbolLibTableFileName() );

        try
        {
            Prj().SchSymbolLibTable()->Save( fn.GetFullPath() );
        }
        catch( const IO_ERROR& ioe )
        {
            success = false;
            wxString msg = wxString::Format( _( "Error occurred saving project specific "
                                                "symbol library table:\n\n%s" ),
                                             GetChars( ioe.What() ) );
            wxMessageBox( msg, _( "File Save Error" ), wxOK | wxICON_ERROR );
        }
    }

    return success;
}
