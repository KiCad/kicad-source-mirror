/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2004-2017 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file symbedit.cpp
 * @brief Functions to load and save individual symbols.
 */

#include <fctsys.h>
#include <kiway.h>
#include <pgm_base.h>
#include <class_drawpanel.h>
#include <confirm.h>
#include <kicad_string.h>
#include <gestfich.h>

#include <class_sch_screen.h>
#include <general.h>
#include <libeditframe.h>
#include <class_libentry.h>
#include <wildcards_and_files_ext.h>
#include <sch_legacy_plugin.h>
#include <properties.h>


void LIB_EDIT_FRAME::LoadOneSymbol()
{
    LIB_PART*       part = GetCurPart();

    // Exit if no library entry is selected or a command is in progress.
    if( !part || ( m_drawItem && m_drawItem->GetFlags() ) )
        return;

    PROJECT&        prj = Prj();
    SEARCH_STACK*   search = prj.SchSearchS();

    m_canvas->SetIgnoreMouseEvents( true );

    wxString default_path = prj.GetRString( PROJECT::SCH_LIB_PATH );

    if( !default_path )
        default_path = search->LastVisitedPath();

    wxFileDialog dlg( this, _( "Import Symbol" ), default_path,
                      wxEmptyString, SchematicSymbolFileWildcard(),
                      wxFD_OPEN | wxFD_FILE_MUST_EXIST );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    SetCrossHairPosition( wxPoint( 0, 0 ) );
    m_canvas->MoveCursorToCrossHair();
    m_canvas->SetIgnoreMouseEvents( false );

    wxString filename = dlg.GetPath();

    prj.SetRString( PROJECT::SCH_LIB_PATH, filename );

    wxArrayString symbols;
    SCH_PLUGIN::SCH_PLUGIN_RELEASER pi( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_LEGACY ) );

    wxString msg;

    try
    {
        pi->EnumerateSymbolLib( symbols, filename );
    }
    catch( const IO_ERROR& ioe )
    {
        msg.Printf( _( "Cannot import symbol library \"%s\"." ), filename );
        DisplayErrorMessage( this, msg, ioe.What() );
        return;
    }

    if( symbols.empty() )
    {
        msg.Printf( _( "Symbol library file \"%s\" is empty." ), filename );
        DisplayError( this,  msg );
        return;
    }

    if( symbols.GetCount() > 1 )
    {
        msg.Printf( _( "More than one symbol found in symbol file \"%s\"." ), filename );
        wxMessageBox( msg, _( "Warning" ), wxOK | wxICON_EXCLAMATION, this );
    }

    LIB_ALIAS* alias = nullptr;

    try
    {
        alias = pi->LoadSymbol( filename, symbols[0] );
    }
    catch( const IO_ERROR& ioe )
    {
        return;
    }

    wxCHECK_RET( alias && alias->GetPart(), "Invalid symbol." );

    LIB_PART* first = alias->GetPart();
    LIB_ITEMS_CONTAINER& drawList = first->GetDrawItems();

    for( LIB_ITEM& item : drawList )
    {
        if( item.Type() == LIB_FIELD_T )
            continue;

        if( item.GetUnit() )
            item.SetUnit( m_unit );

        if( item.GetConvert() )
            item.SetConvert( m_convert );

        item.SetFlags( IS_NEW | SELECTED );

        LIB_ITEM* newItem = (LIB_ITEM*) item.Clone();

        newItem->SetParent( part );
        part->AddDrawItem( newItem );
    }

    part->RemoveDuplicateDrawItems();
    part->ClearSelectedItems();

    OnModify();
    m_canvas->Refresh();
}


void LIB_EDIT_FRAME::SaveOneSymbol()
{
    // Export the current part as a symbol (.sym file)
    // this is the current part without its aliases and doc file
    // because a .sym file is used to import graphics in a part being edited
    LIB_PART*       part = GetCurPart();

    if( !part || part->GetDrawItems().empty() )
        return;

    PROJECT&        prj = Prj();
    SEARCH_STACK*   search = prj.SchSearchS();

    wxString default_path = prj.GetRString( PROJECT::SCH_LIB_PATH );

    if( !default_path )
        default_path = search->LastVisitedPath();

    wxFileDialog dlg( this, _( "Export Symbol" ), default_path,
                      part->GetName() + "." + SchematicSymbolFileExtension,
                      SchematicSymbolFileWildcard(),
                      wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    wxFileName fn = dlg.GetPath();

    /* The GTK file chooser doesn't return the file extension added to
     * file name so add it here. */
    if( fn.GetExt().IsEmpty() )
        fn.SetExt( SchematicSymbolFileExtension );

    prj.SetRString( PROJECT::SCH_LIB_PATH, fn.GetPath() );

    if( fn.FileExists() )
    {
        wxRemove( fn.GetFullPath() );
    }

    wxString        msg;
    msg.Printf( _( "Saving symbol in \"%s\"" ), fn.GetPath() );
    SetStatusText( msg );

    SCH_PLUGIN::SCH_PLUGIN_RELEASER pi( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_LEGACY ) );

    try
    {
        PROPERTIES nodoc_props;     // Doc file is useless for a .sym file
        nodoc_props[ SCH_LEGACY_PLUGIN::PropNoDocFile ] = "";
        pi->CreateSymbolLib( fn.GetFullPath(), &nodoc_props );

        LIB_PART* saved_part = new LIB_PART( *part );
        saved_part->RemoveAllAliases();     // useless in a .sym file
        pi->SaveSymbol( fn.GetFullPath(), saved_part, &nodoc_props );
    }
    catch( const IO_ERROR& ioe )
    {
        msg.Printf( _( "An error occurred attempting to save symbol file \"%s\"" ),
                    fn.GetFullPath() );
        DisplayErrorMessage( this, msg, ioe.What() );
    }
}


void LIB_EDIT_FRAME::PlaceAnchor()
{
    if( LIB_PART*      part = GetCurPart() )
    {
        const wxPoint& cross_hair = GetCrossHairPosition();

        wxPoint offset( -cross_hair.x, cross_hair.y );

        OnModify( );

        part->SetOffset( offset );

        // Redraw the symbol
        RedrawScreen( wxPoint( 0 , 0 ), true );
        m_canvas->Refresh();
    }
}
