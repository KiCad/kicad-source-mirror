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

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "macros.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "kicad_string.h"
#include "gestfich.h"
#include "class_sch_screen.h"
#include "richio.h"

#include "general.h"
#include "protos.h"
#include "libeditframe.h"
#include "class_library.h"

#include <boost/foreach.hpp>
#include <wx/wfstream.h>


void LIB_EDIT_FRAME::LoadOneSymbol()
{
    LIB_COMPONENT* Component;
    wxString       msg, err;
    CMP_LIBRARY*   Lib;

    /* Exit if no library entry is selected or a command is in progress. */
    if( m_component == NULL || ( m_drawItem && m_drawItem->m_Flags ) )
        return;

    DrawPanel->m_IgnoreMouseEvents = true;

    wxString default_path = wxGetApp().ReturnLastVisitedLibraryPath();

    wxFileDialog dlg( this, _( "Import Symbol Drawings" ), default_path,
                      wxEmptyString, SymbolFileWildcard,
                      wxFD_OPEN | wxFD_FILE_MUST_EXIST );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    GetScreen()->SetCrossHairPosition( wxPoint( 0, 0 ) );
    DrawPanel->MoveCursorToCrossHair();
    DrawPanel->m_IgnoreMouseEvents = FALSE;

    wxFileName fn = dlg.GetPath();
    wxGetApp().SaveLastVisitedLibraryPath( fn.GetPath() );

    Lib = new CMP_LIBRARY( LIBRARY_TYPE_SYMBOL, fn );

    if( !Lib->Load( err ) )
    {
        msg.Printf( _( "Error <%s> occurred loading symbol library <%s>." ),
                    GetChars( err ), GetChars( fn.GetName() ) );
        DisplayError( this, msg );
        delete Lib;
        return;
    }

    if( Lib->IsEmpty() )
    {
        msg.Printf( _( "No components found in symbol library <%s>." ),
                    GetChars( fn.GetName() ) );
        delete Lib;
        return;
    }

    if( Lib->GetCount() > 1 )
    {
        msg.Printf( _( "More than one part in symbol file <%s>." ),
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

        item.m_Flags    = IS_NEW;
        item.m_Selected = IS_SELECTED;

        LIB_ITEM* newItem = (LIB_ITEM*) item.Clone();
        newItem->SetParent( m_component );
        m_component->AddDrawItem( newItem );
    }

    m_component->RemoveDuplicateDrawItems();
    m_component->ClearSelectedItems();

    OnModify( );
    DrawPanel->Refresh();

    delete Lib;
}


void LIB_EDIT_FRAME::SaveOneSymbol()
{
    wxString msg;

    if( m_component->GetDrawItemList().empty() )
        return;

    wxString default_path = wxGetApp().ReturnLastVisitedLibraryPath();

    wxFileDialog dlg( this, _( "Export Symbol Drawings" ), default_path,
                      m_component->GetName(), SymbolFileWildcard,
                      wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    wxFileName fn = dlg.GetPath();

    /* The GTK file chooser doesn't return the file extension added to
     * file name so add it here. */
    if( fn.GetExt().IsEmpty() )
        fn.SetExt( SymbolFileExtension );

    wxGetApp().SaveLastVisitedLibraryPath( fn.GetPath() );

    wxFile file( fn.GetFullPath(), wxFile::write );

    if( !file.IsOpened() )
    {
        msg.Printf( _( "Unable to create file <%s>" ), GetChars( fn.GetFullPath() ) );
        DisplayError( this, msg );
        return;
    }

    msg.Printf( _( "Saving symbol in [%s]" ), GetChars( fn.GetPath() ) );
    SetStatusText( msg );

    wxFileOutputStream os( file );
    STREAM_OUTPUTFORMATTER formatter( os );
    wxString line;

    /* File header */
    line << wxT( LIBFILE_IDENT ) << wxT( " " ) << LIB_VERSION_MAJOR
         << wxT( "." ) << LIB_VERSION_MINOR << wxT( "  SYMBOL  " )
         << wxT( "Date: " ) << DateAndTime() << wxT( "\n" );

    /* Component name comment and definition. */
    line << wxT( "# SYMBOL " ) << m_component->GetName() << wxT( "\n#\nDEF " )
         << m_component->GetName() << wxT( " " );

    if( !m_component->GetReferenceField().m_Text.IsEmpty() )
        line << m_component->GetReferenceField().m_Text << wxT( " " );
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
        formatter.Print( 0, TO_UTF8( line ) );
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
    catch( IO_ERROR ioe )
    {
        msg.Printf( _( "An error occurred attempting to save symbol file <%s>" ),
                    GetChars( fn.GetFullPath() ) );
        DisplayError( this, msg );
    }
}


void LIB_EDIT_FRAME::PlaceAnchor()
{
    if( m_component == NULL )
        return;

    wxPoint offset( -GetScreen()->GetCrossHairPosition().x, GetScreen()->GetCrossHairPosition().y );

    OnModify( );

    m_component->SetOffset( offset );

    /* Redraw the symbol */
    RedrawScreen( wxPoint( 0 , 0 ), true );
    DrawPanel->Refresh();
}
