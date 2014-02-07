/**
 * @file pl_editor.cpp
 * @brief page layout editor main file.
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * @author Jean-Pierre Charras, jp.charras at wanadoo.fr
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

#include <fctsys.h>
#include <appl_wxstruct.h>
#include <confirm.h>
#include <gestfich.h>
#include <worksheet_shape_builder.h>
#include <pl_editor_frame.h>
#include <hotkeys.h>

#include <build_version.h>

#include <wx/file.h>
#include <wx/snglinst.h>

extern EDA_COLOR_T g_DrawBgColor;


IMPLEMENT_APP( EDA_APP )

/* MacOSX: Needed for file association
 * http://wiki.wxwidgets.org/WxMac-specific_topics
 */
void EDA_APP::MacOpenFile( const wxString& aFileName )
{
    PL_EDITOR_FRAME*    frame = ((PL_EDITOR_FRAME*)GetTopWindow());
    wxFileName          filename = aFileName;

    if( !filename.FileExists() )
        return;

    frame->LoadPageLayoutDescrFile( aFileName );
}


bool EDA_APP::OnInit()
{
    wxFileName          fn;

    InitEDA_Appl( wxT( "pl_editor" ), APP_PL_EDITOR_T );

    if( m_Checker && m_Checker->IsAnotherRunning() )
    {
        if( !IsOK( NULL, _( "pl_editor is already running. Continue?" ) ) )
            return false;
    }

    g_UserUnit = MILLIMETRES;
    g_DrawBgColor = WHITE;
    g_ShowPageLimits = true;

    // read current setup and reopen last directory if no filename to open in
    // command line
    bool reopenLastUsedDirectory = argc == 1;
    GetSettings( reopenLastUsedDirectory );

    // Must be called before creating the main frame in order to
    // display the real hotkeys in menus or tool tips
    ReadHotkeyConfig( wxT("PlEditorFrame"), s_PlEditor_Hokeys_Descr );

    PL_EDITOR_FRAME * frame = new PL_EDITOR_FRAME( NULL, wxT( "PlEditorFrame" ), wxPoint( 0, 0 ), wxSize( 600, 400 ) );

    // frame title:
    frame->SetTitle( GetTitle() + wxT( " " ) + GetBuildVersion() );

    SetTopWindow( frame );
    frame->Show( true );
    frame->Zoom_Automatique( true );        // Zoom fit in frame
    frame->GetScreen()->m_FirstRedraw = false;


    bool descrLoaded = false;
    if( argc > 1 )
    {
        fn = argv[1];

        if( fn.IsOk() )
        {
            bool success = frame->LoadPageLayoutDescrFile( fn.GetFullPath() );
            if( !success )
            {
                wxString msg;
                msg.Printf( _("Error when loading file <%s>"),
                            fn.GetFullPath().GetData() );
                wxMessageBox( msg );
            }
            else
            {
                descrLoaded = true;
                frame->OnNewPageLayout();
            }
        }
    }

    if( !descrLoaded )
    {
        WORKSHEET_LAYOUT::GetTheInstance().SetPageLayout();
        frame->OnNewPageLayout();
    }

    return true;
}
