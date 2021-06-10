/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Alexander Shuklin <Jasuramme@gmail.com>
 * Copyright (C) 2004-2021 KiCad Developers, see AUTHORS.txt for contributors.
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


#include <backannotate.h>
#include <boost/property_tree/ptree.hpp>
#include <confirm.h>
#include <dsnlexer.h>
#include <ptree.h>
#include <reporter.h>
#include <sch_edit_frame.h>
#include <sch_sheet_path.h>
#include <schematic.h>
#include <kicad_string.h>
#include <kiface_i.h>
#include <wildcards_and_files_ext.h>
#include <connection_graph.h>
#include <wx/log.h>


BACK_ANNOTATE::BACK_ANNOTATE( SCH_EDIT_FRAME* aFrame, REPORTER& aReporter, bool aRelinkFootprints,
                              bool aProcessFootprints, bool aProcessValues,
                              bool aProcessReferences, bool aProcessNetNames, bool aDryRun ) :
        m_reporter( aReporter ),
        m_matchByReference( aRelinkFootprints ),
        m_processFootprints( aProcessFootprints ),
        m_processValues( aProcessValues ),
        m_processReferences( aProcessReferences ),
        m_processNetNames( aProcessNetNames ),
        m_dryRun( aDryRun ),
        m_frame( aFrame ),
        m_changesCount( 0 ),
        m_appendUndo( false )
{
}


BACK_ANNOTATE::~BACK_ANNOTATE()
{
}


bool BACK_ANNOTATE::BackAnnotateSymbols( const std::string& aNetlist )
{
    m_changesCount = 0;
    m_appendUndo = false;
    wxString msg;

    if( !m_matchByReference && !m_processValues && !m_processFootprints && !m_processReferences
        && !m_processNetNames )
    {
        m_reporter.ReportTail( _( "Select at least one property to back annotate." ),
                               RPT_SEVERITY_ERROR );
        return false;
    }

    getPcbModulesFromString( aNetlist );

    SCH_SHEET_LIST sheets = m_frame->Schematic().GetSheets();
    sheets.GetSymbols( m_refs, false );
    sheets.GetMultiUnitSymbols( m_multiUnitsRefs );

    getChangeList();
    checkForUnusedSymbols();

    applyChangelist();

    return true;
}


bool BACK_ANNOTATE::FetchNetlistFromPCB( std::string& aNetlist )
{
    if( Kiface().IsSingle() )
    {
        DisplayErrorMessage( m_frame, _( "Cannot fetch PCB netlist because eeschema is opened "
                                         "in stand-alone mode.\n"
                                         "You must launch the KiCad project manager and create "
                                         "a project." ) );
        return false;
    }

    KIWAY_PLAYER* frame = m_frame->Kiway().Player( FRAME_PCB_EDITOR, false );

    if( !frame )
    {
        wxFileName fn( m_frame->Prj().GetProjectFullName() );
        fn.SetExt( PcbFileExtension );

        frame = m_frame->Kiway().Player( FRAME_PCB_EDITOR, true );
        frame->OpenProjectFiles( std::vector<wxString>( 1, fn.GetFullPath() ) );
    }

    m_frame->Kiway().ExpressMail( FRAME_PCB_EDITOR, MAIL_PCB_GET_NETLIST, aNetlist );
    return true;
}


void BACK_ANNOTATE::PushNewLinksToPCB()
{
    std::string nullPayload;

    m_frame->Kiway().ExpressMail( FRAME_PCB_EDITOR, MAIL_PCB_UPDATE_LINKS, nullPayload );
}


void BACK_ANNOTATE::getPcbModulesFromString( const std::string& aPayload )
{
    auto getStr = []( const PTREE& pt ) -> wxString
                  {
                      return UTF8( pt.front().first );
                  };

    DSNLEXER lexer( aPayload, FROM_UTF8( __func__ ) );
    PTREE    doc;

    // NOTE: KiCad's PTREE scanner constructs a property *name* tree, not a property tree.
    // Every token in the s-expr is stored as a property name; the property's value is then
    // either the nested s-exprs or an empty PTREE; there are *no* literal property values.

    Scan( &doc, &lexer );

    PTREE&   tree = doc.get_child( "pcb_netlist" );
    wxString msg;
    m_pcbFootprints.clear();

    for( const std::pair<const std::string, PTREE>& item : tree )
    {
        wxString path, value, footprint;
        std::map<wxString, wxString> pinNetMap;
        wxASSERT( item.first == "ref" );
        wxString ref = getStr( item.second );

        try
        {
            if( m_matchByReference )
                path = ref;
            else
                path = getStr( item.second.get_child( "timestamp" ) );

            if( path == "" )
            {
                msg.Printf( _( "Footprint '%s' has no assigned symbol." ), ref );
                m_reporter.ReportHead( msg, RPT_SEVERITY_WARNING );
                continue;
            }

            footprint = getStr( item.second.get_child( "fpid" ) );
            value     = getStr( item.second.get_child( "value" ) );

            boost::optional<const PTREE&> nets = item.second.get_child_optional( "nets" );

            if( nets )
            {
                for( const std::pair<const std::string, PTREE>& pin_net : nets.get() )
                {
                    wxASSERT( pin_net.first == "pin_net" );
                    wxString pinNumber = UTF8( pin_net.second.front().first );
                    wxString netName = UTF8( pin_net.second.back().first );
                    pinNetMap[ pinNumber ] = netName;
                }
            }
        }
        catch( ... )
        {
            wxLogWarning( "Cannot parse PCB netlist for back-annotation." );
        }

        // Use lower_bound for not to iterate over map twice
        auto nearestItem = m_pcbFootprints.lower_bound( path );

        if( nearestItem != m_pcbFootprints.end() && nearestItem->first == path )
        {
            // Module with this path already exists - generate error
            msg.Printf( _( "Footprints '%s' and '%s' linked to same symbol." ),
                        nearestItem->second->m_ref,
                        ref );
            m_reporter.ReportHead( msg, RPT_SEVERITY_ERROR );
        }
        else
        {
            // Add footprint to the map
            auto data = std::make_shared<PCB_FP_DATA>( ref, footprint, value, pinNetMap );
            m_pcbFootprints.insert( nearestItem, std::make_pair( path, data ) );
        }
    }
}


void BACK_ANNOTATE::getChangeList()
{
    for( std::pair<const wxString, std::shared_ptr<PCB_FP_DATA>>& fpData : m_pcbFootprints )
    {
        const wxString& pcbPath = fpData.first;
        auto&           pcbData = fpData.second;
        int             refIndex;
        bool            foundInMultiunit = false;

        for( std::pair<const wxString, SCH_REFERENCE_LIST>& item : m_multiUnitsRefs )
        {
            SCH_REFERENCE_LIST& refList = item.second;

            if( m_matchByReference )
                refIndex = refList.FindRef( pcbPath );
            else
                refIndex = refList.FindRefByPath( pcbPath );

            if( refIndex >= 0 )
            {
                // If footprint linked to multi unit symbol, we add all symbol's units to
                // the change list
                foundInMultiunit = true;

                for( size_t i = 0; i < refList.GetCount(); ++i )
                {
                    refList[ i ].GetSymbol()->ClearFlags(SKIP_STRUCT );
                    m_changelist.emplace_back( CHANGELIST_ITEM( refList[i], pcbData ) );
                }

                break;
            }
        }

        if( foundInMultiunit )
            continue;

        if( m_matchByReference )
            refIndex = m_refs.FindRef( pcbPath );
        else
            refIndex = m_refs.FindRefByPath( pcbPath );

        if( refIndex >= 0 )
        {
            m_refs[ refIndex ].GetSymbol()->ClearFlags( SKIP_STRUCT );
            m_changelist.emplace_back( CHANGELIST_ITEM( m_refs[refIndex], pcbData ) );
        }
        else
        {
            // Haven't found linked symbol in multiunits or common refs. Generate error
            wxString msg = wxString::Format( _( "Cannot find symbol for footprint '%s'." ),
                                             pcbData->m_ref );
            m_reporter.ReportTail( msg, RPT_SEVERITY_ERROR );
        }
    }
}

void BACK_ANNOTATE::checkForUnusedSymbols()
{
    m_refs.SortByTimeStamp();

    std::sort( m_changelist.begin(), m_changelist.end(),
               []( const CHANGELIST_ITEM& a, const CHANGELIST_ITEM& b )
               {
                   return SCH_REFERENCE_LIST::sortByTimeStamp( a.first, b.first );
               } );

    size_t i = 0;

    for( const std::pair<SCH_REFERENCE, std::shared_ptr<PCB_FP_DATA>>& item : m_changelist )
    {
        // Refs and changelist are both sorted by paths, so we just go over m_refs and
        // generate errors before we will find m_refs member to which item linked
        while( i < m_refs.GetCount() && m_refs[i].GetPath() != item.first.GetPath() )
        {
            const SCH_REFERENCE& ref = m_refs[i];

            if( ref.GetSymbol()->GetIncludeOnBoard() )
            {
                wxString msg = wxString::Format( _( "Footprint '%s' is not present on PCB. "
                                                    "Corresponding symbols in schematic must be "
                                                    "manually deleted (if desired)." ),
                                                 m_refs[i].GetFullRef() );
                m_reporter.ReportTail( msg, RPT_SEVERITY_WARNING );
            }

            ++i;
        }

        ++i;
    }

    if( m_matchByReference && !m_frame->ReadyToNetlist( _( "Re-linking footprints requires a fully "
                                                           "annotated schematic." ) ) )
    {
        m_reporter.ReportTail( _( "Footprint re-linking cancelled by user." ), RPT_SEVERITY_ERROR );
    }
}


void BACK_ANNOTATE::applyChangelist()
{
    wxString msg;

    // Apply changes from change list
    for( CHANGELIST_ITEM& item : m_changelist )
    {
        SCH_REFERENCE& ref = item.first;
        PCB_FP_DATA&   fpData = *item.second;
        SCH_SYMBOL*    symbol = ref.GetSymbol();
        SCH_SCREEN*    screen = ref.GetSheetPath().LastScreen();
        wxString       oldFootprint = ref.GetFootprint();
        wxString       oldValue = ref.GetValue();
        bool           skip = ( ref.GetSymbol()->GetFlags() & SKIP_STRUCT ) > 0;

        if( m_processReferences && ref.GetRef() != fpData.m_ref && !skip )
        {
            ++m_changesCount;
            msg.Printf( _( "Change '%s' reference designator to '%s'." ),
                        ref.GetFullRef(),
                        fpData.m_ref );

            if( !m_dryRun )
            {
                m_frame->SaveCopyInUndoList( screen, symbol, UNDO_REDO::CHANGED, m_appendUndo );
                m_appendUndo = true;
                symbol->SetRef( &ref.GetSheetPath(), fpData.m_ref );
            }

            m_reporter.ReportHead( msg, RPT_SEVERITY_ACTION );
        }

        if( m_processFootprints && oldFootprint != fpData.m_footprint && !skip )
        {
            ++m_changesCount;
            msg.Printf( _( "Change %s footprint assignment from '%s' to '%s'." ),
                        ref.GetFullRef(),
                        oldFootprint,
                        fpData.m_footprint );

            if( !m_dryRun )
            {
                m_frame->SaveCopyInUndoList( screen, symbol, UNDO_REDO::CHANGED, m_appendUndo );
                m_appendUndo = true;
                symbol->SetFootprint( &ref.GetSheetPath(), fpData.m_footprint );
            }

            m_reporter.ReportHead( msg, RPT_SEVERITY_ACTION );
        }

        if( m_processValues && oldValue != fpData.m_value && !skip )
        {
            ++m_changesCount;
            msg.Printf( _( "Change %s value from '%s' to '%s'." ),
                        ref.GetFullRef(),
                        oldValue,
                        fpData.m_value );

            if( !m_dryRun )
            {
                m_frame->SaveCopyInUndoList( screen, symbol, UNDO_REDO::CHANGED, m_appendUndo );
                m_appendUndo = true;
                symbol->SetValue( &ref.GetSheetPath(), fpData.m_value );
            }

            m_reporter.ReportHead( msg, RPT_SEVERITY_ACTION );
        }

        if( m_processNetNames )
        {
            for( const std::pair<const wxString, wxString>& entry : fpData.m_pinMap )
            {
                const wxString& pinNumber = entry.first;
                const wxString& shortNetName = entry.second;
                SCH_PIN*        pin = symbol->GetPin( pinNumber );

                if( !pin )
                {
                    msg.Printf( _( "Cannot find %s pin '%s'." ),
                                ref.GetFullRef(),
                                pinNumber );
                    m_reporter.ReportHead( msg, RPT_SEVERITY_ERROR );

                    continue;
                }

                SCH_CONNECTION* connection = pin->Connection( &ref.GetSheetPath() );

                if( connection && connection->Name( true ) != shortNetName )
                {
                    processNetNameChange( ref.GetFullRef(), pin, connection,
                                          connection->Name( true ), shortNetName );
                }
            }
        }
    }

    if( !m_dryRun )
    {
        m_frame->RecalculateConnections( NO_CLEANUP );
        m_frame->UpdateNetHighlightStatus();
    }

    m_reporter.ReportHead( msg, RPT_SEVERITY_INFO );
}


static LABEL_SPIN_STYLE orientLabel( SCH_PIN* aPin )
{
    LABEL_SPIN_STYLE spin = LABEL_SPIN_STYLE::RIGHT;

    // Initial orientation from the pin
    switch( aPin->GetLibPin()->GetOrientation() )
    {
    case PIN_UP:    spin = LABEL_SPIN_STYLE::BOTTOM; break;
    case PIN_DOWN:  spin = LABEL_SPIN_STYLE::UP;     break;
    case PIN_LEFT:  spin = LABEL_SPIN_STYLE::RIGHT;  break;
    case PIN_RIGHT: spin = LABEL_SPIN_STYLE::LEFT;   break;
    }

    // Reorient based on the actual symbol orientation now
    struct ORIENT
    {
        int flag;
        int n_rots;
        int mirror_x;
        int mirror_y;
    }
    orientations[] =
    {
        { SYM_ORIENT_0,                  0, 0, 0 },
        { SYM_ORIENT_90,                 1, 0, 0 },
        { SYM_ORIENT_180,                2, 0, 0 },
        { SYM_ORIENT_270,                3, 0, 0 },
        { SYM_MIRROR_X + SYM_ORIENT_0,   0, 1, 0 },
        { SYM_MIRROR_X + SYM_ORIENT_90,  1, 1, 0 },
        { SYM_MIRROR_Y,                  0, 0, 1 },
        { SYM_MIRROR_X + SYM_ORIENT_270, 3, 1, 0 },
        { SYM_MIRROR_Y + SYM_ORIENT_0,   0, 0, 1 },
        { SYM_MIRROR_Y + SYM_ORIENT_90,  1, 0, 1 },
        { SYM_MIRROR_Y + SYM_ORIENT_180, 2, 0, 1 },
        { SYM_MIRROR_Y + SYM_ORIENT_270, 3, 0, 1 }
    };

    ORIENT o = orientations[ 0 ];

    SCH_SYMBOL* parentSymbol = aPin->GetParentSymbol();

    if( !parentSymbol )
        return spin;

    int symbolOrientation = parentSymbol->GetOrientation();

    for( auto& i : orientations )
    {
        if( i.flag == symbolOrientation )
        {
            o = i;
            break;
        }
    }

    for( int i = 0; i < o.n_rots; i++ )
        spin = spin.RotateCCW();

    if( o.mirror_x )
        spin = spin.MirrorX();

    if( o.mirror_y )
        spin = spin.MirrorY();

    return spin;
}


void addConnections( SCH_ITEM* aItem, const SCH_SHEET_PATH& aSheetPath,
                     std::set<SCH_ITEM*>& connectedItems )
{
    if( connectedItems.insert( aItem ).second )
    {
        for( SCH_ITEM* connectedItem : aItem->ConnectedItems( aSheetPath ) )
            addConnections( connectedItem, aSheetPath, connectedItems );
    }
}


void BACK_ANNOTATE::processNetNameChange( const wxString& aRef, SCH_PIN* aPin,
                                          const SCH_CONNECTION* aConnection,
                                          const wxString& aOldName, const wxString& aNewName )
{
    wxString msg;

    // Find a physically-connected driver.  We can't use the SCH_CONNECTION's m_driver because
    // it has already been resolved by merging subgraphs with the same label, etc., and our
    // name change may cause that resolution to change.

    std::set<SCH_ITEM*>           connectedItems;
    SCH_ITEM*                     driver = nullptr;
    CONNECTION_SUBGRAPH::PRIORITY driverPriority = CONNECTION_SUBGRAPH::PRIORITY::NONE;

    addConnections( aPin, aConnection->Sheet(), connectedItems );

    for( SCH_ITEM* item : connectedItems )
    {
        CONNECTION_SUBGRAPH::PRIORITY priority = CONNECTION_SUBGRAPH::GetDriverPriority( item );

        if( priority > driverPriority )
        {
            driver = item;
            driverPriority = priority;
        }
    }

    switch( driver->Type() )
    {
    case SCH_LABEL_T:
    case SCH_GLOBAL_LABEL_T:
    case SCH_HIER_LABEL_T:
    case SCH_SHEET_PIN_T:
        ++m_changesCount;

        msg.Printf( _( "Change %s pin %s net label from '%s' to '%s'." ),
                    aRef,
                    aPin->GetNumber(),
                    aOldName,
                    aNewName );

        if( !m_dryRun )
        {
            SCH_SCREEN* screen = aConnection->Sheet().LastScreen();

            m_frame->SaveCopyInUndoList( screen, driver, UNDO_REDO::CHANGED, m_appendUndo );
            m_appendUndo = true;
            static_cast<SCH_TEXT*>( driver )->SetText( aNewName );
        }

        m_reporter.ReportHead( msg, RPT_SEVERITY_ACTION );
        break;

    case SCH_PIN_T:
    {
        SCH_PIN*         schPin = static_cast<SCH_PIN*>( driver );
        LABEL_SPIN_STYLE spin   = orientLabel( schPin );

        if( schPin->IsPowerConnection() )
        {
            msg.Printf( _( "Net %s cannot be changed to '%s' because it is driven by a power "
                           "pin." ), aOldName, aNewName );

            m_reporter.ReportHead( msg, RPT_SEVERITY_ERROR );
            break;
        }

        ++m_changesCount;
        msg.Printf( _( "Add label '%s' to %s pin %s net." ), aNewName, aRef, aPin->GetNumber() );

        if( !m_dryRun )
        {
            SCHEMATIC_SETTINGS& settings = m_frame->Schematic().Settings();
            SCH_LABEL* label = new SCH_LABEL( driver->GetPosition(), aNewName );
            label->SetParent( &m_frame->Schematic() );
            label->SetTextSize( wxSize( settings.m_DefaultTextSize, settings.m_DefaultTextSize ) );
            label->SetLabelSpinStyle( spin );
            label->SetFlags( IS_NEW );

            SCH_SCREEN* screen = aConnection->Sheet().LastScreen();
            m_frame->AddItemToScreenAndUndoList( screen, label, m_appendUndo );
            m_appendUndo = true;
        }

        m_reporter.ReportHead( msg, RPT_SEVERITY_ACTION );
    }
        break;

    default:
        break;
    }
}
