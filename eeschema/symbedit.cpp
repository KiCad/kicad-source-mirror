/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2008-2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2004-2011 KiCad Developers, see change_log.txt for contributors.
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
 * @brief Functions to load from and save to file component libraries and symbols.
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
#include <class_library.h>
#include <wildcards_and_files_ext.h>

#include <boost/foreach.hpp>


void LIB_EDIT_FRAME::LoadOneSymbol()
{
    LIB_COMPONENT* Component;
    wxString       msg, err;
    CMP_LIBRARY*   Lib;

    /* Exit if no library entry is selected or a command is in progress. */
    if( m_component == NULL || ( m_drawItem && m_drawItem->GetFlags() ) )
        return;

    PROJECT&        prj = Prj();
    SEARCH_STACK&   search = prj.SchSearchS();

    m_canvas->SetIgnoreMouseEvents( true );

    wxString default_path = prj.GetRString( PROJECT::SCH_LIB_PATH );
    if( !default_path )
        default_path = search.LastVisitedPath();

    wxFileDialog dlg( this, _( "Import Symbol Drawings" ), default_path,
                      wxEmptyString, SchematicSymbolFileWildcard,
                      wxFD_OPEN | wxFD_FILE_MUST_EXIST );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    SetCrossHairPosition( wxPoint( 0, 0 ) );
    m_canvas->MoveCursorToCrossHair();
    m_canvas->SetIgnoreMouseEvents( false );

    wxFileName fn = dlg.GetPath();

    prj.SetRString( PROJECT::SCH_LIB_PATH, fn.GetPath() );

    Lib = new CMP_LIBRARY( LIBRARY_TYPE_SYMBOL, fn );

    if( !Lib->Load( err ) )
    {
        msg.Printf( _( "Error '%s' occurred loading symbol library '%s'." ),
                    GetChars( err ), GetChars( fn.GetName() ) );
        DisplayError( this, msg );
        delete Lib;
        return;
    }

    if( Lib->IsEmpty() )
    {
        msg.Printf( _( "No components found in symbol library '%s'." ),
                    GetChars( fn.GetName() ) );
        delete Lib;
        return;
    }

    if( Lib->GetCount() > 1 )
    {
        msg.Printf( _( "More than one part in symbol file '%s'." ),
                    GetChars( fn.GetName() ) );
        wxMessageBox( msg, _( "Warning" ), wxOK | wxICON_EXCLAMATION, this );
    }

    Component = Lib->GetFirstEntry()->GetComponent();
    LIB_ITEMS& drawList = Component->GetDrawItemList();

    BOOST_FOREACH( LIB_ITEM& item, drawList )
    {
        if( item.Type() == LIB_FIELD_T )
            continue;

        if( item.GetUnit() )
            item.SetUnit( m_unit );

        if( item.GetConvert() )
            item.SetConvert( m_convert );

        item.SetFlags( IS_NEW | SELECTED );

        LIB_ITEM* newItem = (LIB_ITEM*) item.Clone();
        newItem->SetParent( m_component );
        m_component->AddDrawItem( newItem );
    }

    m_component->RemoveDuplicateDrawItems();
    m_component->ClearSelectedItems();

    OnModify();
    m_canvas->Refresh();

    delete Lib;
}


void LIB_EDIT_FRAME::SaveOneSymbol()
{
    wxString        msg;
    PROJECT&        prj = Prj();
    SEARCH_STACK&   search = prj.SchSearchS();

    if( m_component->GetDrawItemList().empty() )
        return;

    wxString default_path = prj.GetRString( PROJECT::SCH_LIB_PATH );
    if( !default_path )
        default_path = search.LastVisitedPath();

    wxFileDialog dlg( this, _( "Export Symbol Drawings" ), default_path,
                      m_component->GetName(), SchematicSymbolFileWildcard,
                      wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    wxFileName fn = dlg.GetPath();

    /* The GTK file chooser doesn't return the file extension added to
     * file name so add it here. */
    if( fn.GetExt().IsEmpty() )
        fn.SetExt( SchematicSymbolFileExtension );

    prj.SetRString( PROJECT::SCH_LIB_PATH, fn.GetPath() );

    msg.Printf( _( "Saving symbol in '%s'" ), GetChars( fn.GetPath() ) );
    SetStatusText( msg );

    wxString line;

    /* File header */
    line << wxT( LIBFILE_IDENT ) << wxT( " " ) << LIB_VERSION_MAJOR
         << wxT( "." ) << LIB_VERSION_MINOR << wxT( "  SYMBOL  " )
         << wxT( "Date: " ) << DateAndTime() << wxT( "\n" );

    /* Component name comment and definition. */
    line << wxT( "# SYMBOL " ) << m_component->GetName() << wxT( "\n#\nDEF " )
         << m_component->GetName() << wxT( " " );

    if( !m_component->GetReferenceField().GetText().IsEmpty() )
        line << m_component->GetReferenceField().GetText() << wxT( " " );
    else
        line << wxT( "~ " );

    line << 0 << wxT( " " ) << m_component->GetPinNameOffset() << wxT( " " );

    if( m_component->ShowPinNumbers() )
        line << wxT( "Y " );
    else
        line << wxT( "N " );

    if( m_component->ShowPinNames() )
        line << wxT( "Y " );
    else
        line << wxT( "N " );

    line << wxT( "1 0 N\n" );

    try
    {
        FILE_OUTPUTFORMATTER    formatter( fn.GetFullPath() );

        try
        {
            formatter.Print( 0, "%s", TO_UTF8( line ) );
            m_component->GetReferenceField().Save( formatter );
            m_component->GetValueField().Save( formatter );
            formatter.Print( 0, "DRAW\n" );

            LIB_ITEMS& drawList = m_component->GetDrawItemList();

            BOOST_FOREACH( LIB_ITEM& item, drawList )
            {
                if( item.Type() == LIB_FIELD_T )
                    continue;

                /* Don't save unused parts or alternate body styles. */
                if( m_unit && item.GetUnit() && ( item.GetUnit() != m_unit ) )
                    continue;

                if( m_convert && item.GetConvert() && ( item.GetConvert() != m_convert ) )
                    continue;

                item.Save( formatter );
            }

            formatter.Print( 0, "ENDDRAW\n" );
            formatter.Print( 0, "ENDDEF\n" );
        }
        catch( const IO_ERROR& ioe )
        {
            msg.Printf( _( "An error occurred attempting to save symbol file '%s'" ),
                        GetChars( fn.GetFullPath() ) );
            DisplayError( this, msg );
        }
    }
    catch( const IO_ERROR& ioe )
    {
        DisplayError( this, ioe.errorText );
        return;
    }
}


void LIB_EDIT_FRAME::PlaceAnchor()
{
    if( m_component == NULL )
        return;

    const wxPoint& cross_hair = GetCrossHairPosition();

    wxPoint offset( -cross_hair.x, cross_hair.y );

    OnModify( );

    m_component->SetOffset( offset );

    /* Redraw the symbol */
    RedrawScreen( wxPoint( 0 , 0 ), true );
    m_canvas->Refresh();
}
