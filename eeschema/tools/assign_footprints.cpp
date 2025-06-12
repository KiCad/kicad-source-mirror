/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <confirm.h>
#include <string_utils.h>
#include <kiface_base.h>
#include <sch_edit_frame.h>
#include <wildcards_and_files_ext.h>
#include <sch_commit.h>
#include <sch_sheet_path.h>
#include <sch_symbol.h>
#include <sch_reference_list.h>
#include <schematic.h>
#include <dsnlexer.h>
#include <ptree.h>
#include <boost/property_tree/ptree.hpp>
#include <tools/sch_editor_control.h>

#include <wx/choicdlg.h>
#include <wx/filedlg.h>


void SCH_EDITOR_CONTROL::AssignFootprints( const std::string& aChangedSetOfReferences )
{
    // Build a flat list of symbols in schematic:
    SCH_REFERENCE_LIST refs;
    SCH_COMMIT         commit( m_frame );
    bool               isChanged = false;

    m_frame->Schematic().Hierarchy().GetSymbols( refs, false );

    DSNLEXER lexer( aChangedSetOfReferences, From_UTF8( __func__ ) );
    PTREE    doc;

    try
    {
        Scan( &doc, &lexer );

        CPTREE& back_anno = doc.get_child( "cvpcb_netlist" );
        wxString footprint;

        for( PTREE::const_iterator ref = back_anno.begin();  ref != back_anno.end();  ++ref )
        {
            wxASSERT( ref->first == "ref" );

            wxString reference = UTF8( ref->second.front().first );

            // Ensure the "fpid" node contains a footprint name, and get it if exists
            if( ref->second.get_child( "fpid" ).size() )
            {
                wxString tmp = UTF8( ref->second.get_child( "fpid" ).front().first );
                footprint = tmp;
            }
            else
                footprint.Empty();

            // Search the symbol in the flat list
            for( unsigned ii = 0;  ii < refs.GetCount();  ++ii )
            {
                if( reference == refs[ii].GetRef() )
                {
                    // We have found a candidate.
                    // Note: it can be not unique (multiple parts per package)
                    // So we *do not* stop the search here
                    SCH_SYMBOL*  symbol = refs[ ii ].GetSymbol();

                    // For backwards-compatibility CvPcb currently updates all instances of a
                    // symbol (even though it lists these instances separately).
                    wxString        oldfp = refs[ii].GetFootprint();
                    SCH_FIELD*      footprintField = symbol->GetField( FIELD_T::FOOTPRINT );

                    if( oldfp.IsEmpty() && footprintField->IsVisible() )
                        footprintField->SetVisible( false );

                    if( oldfp != footprint )
                    {
                        isChanged = true;
                        SCH_SCREEN* screen = refs[ii].GetSheetPath().LastScreen();

                        commit.Modify( symbol, screen, RECURSE_MODE::NO_RECURSE );
                        footprintField->SetText( footprint );
                    }
                }
            }
        }
    }
    catch( const PTREE_ERROR& ex )
    {
        // remap the exception to something the caller is likely to understand.
        THROW_IO_ERROR( ex.what() );
    }

    if( isChanged )
    {
        m_frame->SyncView();
        commit.Push( wxS( "Assign Footprints" ) );
    }
}


bool SCH_EDITOR_CONTROL::processCmpToFootprintLinkFile( const wxString& aFullFilename,
                                                        bool aForceVisibilityState,
                                                        bool aVisibilityState )
{
    // Build a flat list of symbols in schematic:
    SCH_REFERENCE_LIST referencesList;
    m_frame->Schematic().Hierarchy().GetSymbols( referencesList, false );

    FILE* cmpFile = wxFopen( aFullFilename, wxT( "rt" ) );

    if( cmpFile == nullptr )
        return false;

    // cmpFileReader dtor will close cmpFile
    FILE_LINE_READER cmpFileReader( cmpFile, aFullFilename );

    // Now, for each symbol found in file,
    // replace footprint field value by the new value:
    wxString reference;
    wxString footprint;
    wxString buffer;
    wxString value;

    while( cmpFileReader.ReadLine() )
    {
        buffer = From_UTF8( cmpFileReader.Line() );

        if( !buffer.StartsWith( wxT( "BeginCmp" ) ) )
            continue;

        // Begin symbol description.
        reference.Empty();
        footprint.Empty();

        while( cmpFileReader.ReadLine() )
        {
            buffer = From_UTF8( cmpFileReader.Line() );

            if( buffer.StartsWith( wxT( "EndCmp" ) ) )
                break;

            // store string value, stored between '=' and ';' delimiters.
            value = buffer.AfterFirst( '=' );
            value = value.BeforeLast( ';' );
            value.Trim(true);
            value.Trim(false);

            if( buffer.StartsWith( wxT( "Reference" ) ) )
                reference = value;
            else if( buffer.StartsWith( wxT( "IdModule" ) ) )
                footprint = value;
        }

        // A block is read: initialize the footprint field of the corresponding symbol
        // if the footprint name is not empty
        if( reference.IsEmpty() )
            continue;

        // Search the symbol in the flat list
        for( unsigned ii = 0;  ii < referencesList.GetCount();  ii++ )
        {
            if( reference == referencesList[ii].GetRef() )
            {
                // We have found a candidate.
                // Note: it can be not unique (multiple units per part)
                // So we *do not* stop the search here
                SCH_SYMBOL* symbol = referencesList[ ii ].GetSymbol();

                symbol->SetFootprintFieldText( footprint );

                if( aForceVisibilityState )
                    symbol->GetField( FIELD_T::FOOTPRINT )->SetVisible( aVisibilityState );
            }
        }
    }

    return true;
}


int SCH_EDITOR_CONTROL::ImportFPAssignments( const TOOL_EVENT& aEvent )
{
    wxString path = wxPathOnly( m_frame->Prj().GetProjectFullName() );

    wxFileDialog dlg( m_frame, _( "Load Symbol Footprint Link File" ),
                      path, wxEmptyString,
                      FILEEXT::FootprintAssignmentFileWildcard(),
                      wxFD_OPEN | wxFD_FILE_MUST_EXIST );

    if( dlg.ShowModal() == wxID_CANCEL )
        return 0;

    wxString filename = dlg.GetPath();

    wxArrayString choices;
    choices.Add( _( "Keep existing footprint field visibility" ) );
    choices.Add( _( "Show all footprint fields" ) );
    choices.Add( _( "Hide all footprint fields" ) );

    wxSingleChoiceDialog choiceDlg( m_frame, _( "Select the footprint field visibility setting." ),
                                    _( "Change Visibility" ), choices );

    if( choiceDlg.ShowModal() == wxID_CANCEL )
        return 0;

    bool forceVisibility = (choiceDlg.GetSelection() != 0 );
    bool visibilityState = (choiceDlg.GetSelection() == 1 );

    if( !processCmpToFootprintLinkFile( filename, forceVisibility, visibilityState ) )
    {
        wxString msg = wxString::Format( _( "Failed to open symbol-footprint link file '%s'." ),
                                         filename.GetData() );

        DisplayError( m_frame, msg );
        return 0;
    }

    m_frame->SyncView();
    m_frame->GetCanvas()->Refresh();
    m_frame->OnModify();
    return 0;
}
