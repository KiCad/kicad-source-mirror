/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2004-2020 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <kiway.h>
#include <pgm_base.h>
#include <sch_draw_panel.h>
#include <confirm.h>
#include <kicad_string.h>
#include <gestfich.h>
#include <lib_pin.h>
#include <lib_edit_frame.h>
#include <class_libentry.h>
#include <wildcards_and_files_ext.h>
#include <sch_legacy_plugin.h>
#include <properties.h>
#include <view/view.h>
#include <tool/tool_manager.h>
#include <tools/ee_selection_tool.h>

void LIB_EDIT_FRAME::LoadOneSymbol()
{
    EE_SELECTION_TOOL* selTool = m_toolManager->GetTool<EE_SELECTION_TOOL>();

    // Exit if no library entry is selected or a command is in progress.
    if( !m_my_part || !EE_CONDITIONS::Idle( selTool->GetSelection() ) )
        return;

    PROJECT&        prj = Prj();
    SEARCH_STACK*   search = prj.SchSearchS();

    wxString default_path = prj.GetRString( PROJECT::SCH_LIB_PATH );

    if( !default_path )
        default_path = search->LastVisitedPath();

    wxFileDialog dlg( this, _( "Import Symbol" ), default_path,
                      wxEmptyString, SchematicSymbolFileWildcard(),
                      wxFD_OPEN | wxFD_FILE_MUST_EXIST );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

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

    LIB_PART* alias = nullptr;

    try
    {
        alias = pi->LoadSymbol( filename, symbols[0] );
    }
    catch( const IO_ERROR& )
    {
        return;
    }

    wxCHECK_RET( alias, "Invalid symbol." );

    SaveCopyInUndoList( m_my_part );

    LIB_PART* first = alias;
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

        newItem->SetParent( m_my_part );
        m_my_part->AddDrawItem( newItem );
        item.ClearSelected();
    }

    m_my_part->RemoveDuplicateDrawItems();

    OnModify();
}


void LIB_EDIT_FRAME::SaveOneSymbol()
{
    wxCHECK( m_my_part, /* void */ );

    // Export the current part as a symbol (.sym file)
    // this is the current part without its aliases and doc file
    // because a .sym file is used to import graphics in a part being edited
    if( m_my_part->GetDrawItems().empty() )
        return;

    PROJECT&        prj = Prj();
    SEARCH_STACK*   search = prj.SchSearchS();

    wxString default_path = prj.GetRString( PROJECT::SCH_LIB_PATH );

    if( !default_path )
        default_path = search->LastVisitedPath();

    wxFileDialog dlg( this, _( "Export Symbol" ), default_path,
                      m_my_part->GetName() + "." + SchematicSymbolFileExtension,
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
        wxRemove( fn.GetFullPath() );

    SetStatusText( wxString::Format( _( "Saving symbol in \"%s\"" ), fn.GetPath() ) );

    SCH_PLUGIN::SCH_PLUGIN_RELEASER plugin( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_LEGACY ) );

    try
    {
        PROPERTIES nodoc_props;     // Doc file is useless for a .sym file
        nodoc_props[ SCH_LEGACY_PLUGIN::PropNoDocFile ] = "";
        plugin->CreateSymbolLib( fn.GetFullPath(), &nodoc_props );

        // The part gets flattened by the LIB_PART copy constructor.
        LIB_PART* saved_part = new LIB_PART( *m_my_part );
        plugin->SaveSymbol( fn.GetFullPath(), saved_part, &nodoc_props );
    }
    catch( const IO_ERROR& ioe )
    {
        wxString msg = wxString::Format( _( "An error occurred saving symbol file \"%s\"" ),
                                         fn.GetFullPath() );
        DisplayErrorMessage( this, msg, ioe.What() );
    }
}


void LIB_EDIT_FRAME::DisplayCmpDoc()
{
    EDA_DRAW_FRAME::ClearMsgPanel();

    if( !m_my_part )
        return;

    wxString msg = m_my_part->GetName();

    AppendMsgPanel( _( "Name" ), msg, BLUE, 8 );

    if( m_my_part->IsAlias() )
    {
        PART_SPTR parent = m_my_part->GetParent().lock();

        msg = parent ? parent->GetName() : _( "Undefined!" );
        AppendMsgPanel( _( "Parent" ), msg, BROWN, 8 );
    }

    static wxChar UnitLetter[] = wxT( "?ABCDEFGHIJKLMNOPQRSTUVWXYZ" );
    msg = UnitLetter[m_unit];

    AppendMsgPanel( _( "Unit" ), msg, BROWN, 8 );

    if( m_convert > 1 )
        msg = _( "Convert" );
    else
        msg = _( "Normal" );

    AppendMsgPanel( _( "Body" ), msg, GREEN, 8 );

    if( m_my_part->IsPower() )
        msg = _( "Power Symbol" );
    else
        msg = _( "Symbol" );

    AppendMsgPanel( _( "Type" ), msg, MAGENTA, 8 );
    AppendMsgPanel( _( "Description" ), m_my_part->GetDescription(), CYAN, 8 );
    AppendMsgPanel( _( "Key words" ), m_my_part->GetKeyWords(), DARKDARKGRAY );
    AppendMsgPanel( _( "Datasheet" ), m_my_part->GetDatasheetField().GetText(), DARKDARKGRAY );
}
