/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Alexander Shuklin <Jasuramme@gmail.com>
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


#include <backannotate.h>
#include <boost/property_tree/ptree.hpp>
#include <confirm.h>
#include <dsnlexer.h>
#include <ptree.h>
#include <reporter.h>
#include <sch_edit_frame.h>
#include <sch_sheet_path.h>
#include <schematic.h>
#include <kiface_i.h>
#include <wildcards_and_files_ext.h>

BACK_ANNOTATE::BACK_ANNOTATE( SCH_EDIT_FRAME* aFrame, REPORTER& aReporter,
                              bool aProcessFootprints, bool aProcessValues,
                              bool aProcessReferences, bool aProcessNetNames,
                              bool aIgnoreOtherProjects, bool aDryRun ) :
        m_reporter( aReporter ),
        m_processFootprints( aProcessFootprints ),
        m_processValues( aProcessValues ),
        m_processReferences( aProcessReferences ),
        m_processNetNames( aProcessNetNames ),
        m_ignoreOtherProjects( aIgnoreOtherProjects ),
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

    if( !m_processValues && !m_processFootprints && !m_processReferences && !m_processNetNames )
    {
        m_reporter.ReportTail( _( "Select at least one property to back annotate." ),
                               RPT_SEVERITY_ERROR );
        return false;
    }

    getPcbModulesFromString( aNetlist );

    SCH_SHEET_LIST sheets = m_frame->Schematic().GetSheets();
    sheets.GetComponents( m_refs, false );
    sheets.GetMultiUnitComponents( m_multiUnitsRefs );

    getChangeList();
    checkForUnusedSymbols();
    checkSharedSchematicErrors();

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
    m_pcbModules.clear();

    for( const std::pair<const std::string, PTREE>& item : tree )
    {
        wxString path, value, footprint;
        std::map<wxString, wxString> pinNetMap;
        wxASSERT( item.first == "ref" );
        wxString ref = getStr( item.second );

        try
        {
            path = getStr( item.second.get_child( "timestamp" ) );

            if( path == "" )
            {
                msg.Printf( _( "Footprint \"%s\" has no symbol associated." ), ref );
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
            wxLogWarning( "Cannot parse PCB netlist for back-annotation" );
        }

        // Use lower_bound for not to iterate over map twice
        auto nearestItem = m_pcbModules.lower_bound( path );

        if( nearestItem != m_pcbModules.end() && nearestItem->first == path )
        {
            // Module with this path already exists - generate error
            msg.Printf( _( "Pcb footprints \"%s\" and \"%s\" linked to same symbol" ),
                        nearestItem->second->m_ref, ref );
            m_reporter.ReportHead( msg, RPT_SEVERITY_ERROR );
        }
        else
        {
            // Add module to the map
            auto data = std::make_shared<PCB_MODULE_DATA>( ref, footprint, value, pinNetMap );
            m_pcbModules.insert( nearestItem, std::make_pair( path, data ) );
        }
    }
}


void BACK_ANNOTATE::getChangeList()
{
    for( std::pair<const wxString, std::shared_ptr<PCB_MODULE_DATA>>& module : m_pcbModules )
    {
        const wxString& pcbPath = module.first;
        auto&           pcbData = module.second;
        bool            foundInMultiunit = false;

        for( auto& item : m_multiUnitsRefs )
        {
            SCH_REFERENCE_LIST& refList = item.second;

            if( refList.FindRefByPath( pcbPath ) >= 0 )
            {

                // If module linked to multi unit symbol, we add all symbol's units to
                // the change list
                foundInMultiunit = true;

                for( size_t i = 0; i < refList.GetCount(); ++i )
                {
                    refList[i].GetComp()->ClearFlags( SKIP_STRUCT );
                    m_changelist.emplace_back( CHANGELIST_ITEM( refList[i], pcbData ) );
                }

                break;
            }
        }

        if( foundInMultiunit )
            continue;

        int refIndex = m_refs.FindRefByPath( pcbPath );

        if( refIndex >= 0 )
        {
            m_refs[refIndex].GetComp()->ClearFlags( SKIP_STRUCT );
            m_changelist.emplace_back( CHANGELIST_ITEM( m_refs[refIndex], pcbData ) );
        }
        else
        {
            // Haven't found linked symbol in multiunits or common refs. Generate error
            wxString msg;
            msg.Printf( _( "Cannot find symbol for \"%s\" footprint" ), pcbData->m_ref );
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

    for( auto& item : m_changelist )
    {
        // Refs and changelist are both sorted by paths, so we just go over m_refs and
        // generate errors before we will find m_refs member to which item linked
        while( i < m_refs.GetCount() && m_refs[i].GetPath() != item.first.GetPath() )
        {
            wxString msg;
            msg.Printf( _( "Cannot find footprint for \"%s\" symbol" ), m_refs[i++].GetFullRef() );
            m_reporter.ReportTail( msg, RPT_SEVERITY_ERROR );
        }

        ++i;
    }
}


bool BACK_ANNOTATE::checkReuseViolation( PCB_MODULE_DATA& aFirst, PCB_MODULE_DATA& aSecond )
{
    if( m_processFootprints && aFirst.m_footprint != aSecond.m_footprint )
        return false;

    if( m_processValues && aFirst.m_value != aSecond.m_value )
        return false;

    return true;
}

void BACK_ANNOTATE::checkSharedSchematicErrors()
{
    std::sort( m_changelist.begin(), m_changelist.end(),
               []( CHANGELIST_ITEM& a, CHANGELIST_ITEM& b )
               {
                   return a.first.GetComp() > b.first.GetComp();
               } );

    // We don't check that if no footprints or values updating
    if( !m_processFootprints && !m_processValues )
        return;

    // We will count how many times every component used in our changelist
    // Component in this case is SCH_COMPONENT which can be used by more than one symbol
    int usageCount = 1;

    for( auto it = m_changelist.begin(); it != m_changelist.end(); ++it )
    {
        int compUsage = it->first.GetComp()->GetInstanceReferences().size();

        if( compUsage == 1 )
            continue;

        // If that's not the last reference in list and references share same component
        if( ( it + 1 ) != m_changelist.end() && it->first.GetComp() == ( it + 1 )->first.GetComp() )
        {
            ++usageCount;

            if( !checkReuseViolation( *it->second, *( it + 1 )->second ) )
            {
                // Refs share same component but have different values or footprints
                it->first.GetComp()->SetFlags( SKIP_STRUCT );

                wxString msg;
                msg.Printf( _( "\"%s\" and \"%s\" use the same schematic symbol.\n"
                               "They cannot have different footprints or values." ),
                            ( it + 1 )->second->m_ref,
                            it->second->m_ref );
                m_reporter.ReportTail( msg, RPT_SEVERITY_ERROR );
            }
        }
        else
        {
            /* Next ref uses different component, so we count all components number for current
            one. We compare that number to stored in the component itself. If that differs, it
            means that this particular component is reused in some other project. */
            if( !m_ignoreOtherProjects && compUsage > usageCount )
            {
                SCH_COMPONENT*  comp = it->first.GetComp();
                PCB_MODULE_DATA tmp{ "",
                                     comp->GetField( FOOTPRINT )->GetText(),
                                     comp->GetField( VALUE )->GetText(),
                                     {} };

                if( !checkReuseViolation( tmp, *it->second ) )
                {
                    it->first.GetComp()->SetFlags( SKIP_STRUCT );

                    wxString msg;
                    msg.Printf( _( "Unable to change \"%s\" footprint or value because associated"
                                   " symbol is reused in the another project" ),
                                it->second->m_ref );
                    m_reporter.ReportTail( msg, RPT_SEVERITY_ERROR );
                }
            }
            usageCount = 1;
        }
    }
}


void BACK_ANNOTATE::applyChangelist()
{
    std::set<wxString> handledNetChanges;
    wxString           msg;

    // Apply changes from change list
    for( CHANGELIST_ITEM& item : m_changelist )
    {
        SCH_REFERENCE&   ref = item.first;
        PCB_MODULE_DATA& module = *item.second;
        SCH_COMPONENT*   comp = ref.GetComp();
        SCH_SCREEN*      screen = ref.GetSheetPath().LastScreen();
        wxString         oldFootprint = comp->GetField( FOOTPRINT )->GetText();
        wxString         oldValue = comp->GetField( VALUE )->GetText();
        bool             skip = ( ref.GetComp()->GetFlags() & SKIP_STRUCT ) > 0;

        if( m_processReferences && ref.GetRef() != module.m_ref && !skip )
        {
            ++m_changesCount;
            msg.Printf( _( "Change \"%s\" reference designator to \"%s\"." ),
                        ref.GetFullRef(),
                        module.m_ref );

            if( !m_dryRun )
            {
                m_frame->SaveCopyInUndoList( screen, comp, UR_CHANGED, m_appendUndo );
                m_appendUndo = true;
                comp->SetRef( &ref.GetSheetPath(), module.m_ref );
            }

            m_reporter.ReportHead( msg, RPT_SEVERITY_ACTION );
        }

        if( m_processFootprints && oldFootprint != module.m_footprint && !skip )
        {
            ++m_changesCount;
            msg.Printf( _( "Change %s footprint from \"%s\" to \"%s\"." ),
                        ref.GetFullRef(),
                        comp->GetField( FOOTPRINT )->GetText(),
                        module.m_footprint );

            if( !m_dryRun )
            {
                m_frame->SaveCopyInUndoList( screen, comp, UR_CHANGED, m_appendUndo );
                m_appendUndo = true;
                ref.GetComp()->GetField( FOOTPRINT )->SetText( module.m_footprint );
            }

            m_reporter.ReportHead( msg, RPT_SEVERITY_ACTION );
        }

        if( m_processValues && oldValue != module.m_value && !skip )
        {
            ++m_changesCount;
            msg.Printf( _( "Change %s value from \"%s\" to \"%s\"." ),
                        ref.GetFullRef(),
                        comp->GetField( VALUE )->GetText(),
                        module.m_value );

            if( !m_dryRun )
            {
                m_frame->SaveCopyInUndoList( screen, comp, UR_CHANGED, m_appendUndo );
                m_appendUndo = true;
                comp->GetField( VALUE )->SetText( module.m_value );
            }

            m_reporter.ReportHead( msg, RPT_SEVERITY_ACTION );
        }

        if( m_processNetNames )
        {
            for( const std::pair<const wxString, wxString>& entry : module.m_pinMap )
            {
                const wxString& pinNumber = entry.first;
                const wxString& shortNetName = entry.second;
                LIB_PIN*        pin = comp->GetPin( pinNumber );
                SCH_CONNECTION* conn = comp->GetConnectionForPin( pin, ref.GetSheetPath() );

                wxString key = shortNetName + ref.GetSheetPath().PathAsString();

                if( handledNetChanges.count( key ) )
                    continue;
                else
                    handledNetChanges.insert( key );

                if( conn && conn->Name( true ) != shortNetName )
                    processNetNameChange( conn, conn->Name( true ), shortNetName );
            }
        }
    }

    m_reporter.ReportHead( msg, RPT_SEVERITY_INFO );
}


void BACK_ANNOTATE::processNetNameChange( SCH_CONNECTION* aConn, const wxString& aOldName,
                                          const wxString& aNewName )
{
    wxString  msg;
    SCH_ITEM* driver = aConn->Driver();

    auto editMatchingLabels =
            [this]( SCH_SCREEN* aScreen, KICAD_T aType, const wxString& oldName,
                    const wxString& newName )
            {
                for( SCH_ITEM* schItem : aScreen->Items().OfType( aType ) )
                {
                    SCH_TEXT* label = static_cast<SCH_TEXT*>( schItem );

                    if( EscapeString( label->GetShownText(), CTX_NETNAME ) == oldName )
                    {
                        m_frame->SaveCopyInUndoList( aScreen, label, UR_CHANGED, m_appendUndo );
                        m_appendUndo = true;
                        static_cast<SCH_TEXT*>( label )->SetText( newName );
                    }
                }
            };

    switch( driver->Type() )
    {
    case SCH_LABEL_T:
        ++m_changesCount;
        msg.Printf( _( "Change \"%s\" labels to \"%s\"." ), aOldName, aNewName );

        if( !m_dryRun )
        {
            SCH_SCREEN* screen = aConn->Sheet().LastScreen();

            for( SCH_ITEM* label : screen->Items().OfType( SCH_LABEL_T ) )
            {
                SCH_CONNECTION* conn = label->Connection( aConn->Sheet() );

                if( conn && conn->Driver() == driver )
                {
                    m_frame->SaveCopyInUndoList( screen, label, UR_CHANGED, m_appendUndo );
                    m_appendUndo = true;
                    static_cast<SCH_TEXT*>( label )->SetText( aNewName );
                }
            }
        }

        m_reporter.ReportHead( msg, RPT_SEVERITY_ACTION );
        break;

    case SCH_GLOBAL_LABEL_T:
        ++m_changesCount;
        msg.Printf( _( "Change \"%s\" global labels to \"%s\"." ), aOldName, aNewName );

        if( !m_dryRun )
        {
            for( const SCH_SHEET_PATH& sheet : m_frame->Schematic().GetSheets() )
                editMatchingLabels( sheet.LastScreen(), SCH_GLOBAL_LABEL_T, aOldName, aNewName );
        }

        m_reporter.ReportHead( msg, RPT_SEVERITY_ACTION );
        break;

    case SCH_HIER_LABEL_T:
        ++m_changesCount;
        msg.Printf( _( "Change \"%s\" hierarchical label to \"%s\"." ), aOldName, aNewName );

        if( !m_dryRun )
        {
            SCH_SCREEN* screen = aConn->Sheet().LastScreen();
            editMatchingLabels( screen, SCH_HIER_LABEL_T, aOldName, aNewName );

            SCH_SHEET* sheet = dynamic_cast<SCH_SHEET*>( driver->GetParent() );
            screen = sheet->GetScreen();

            for( SCH_SHEET_PIN* pin : sheet->GetPins() )
            {
                if( EscapeString( pin->GetShownText(), CTX_NETNAME ) == aOldName )
                {
                    m_frame->SaveCopyInUndoList( screen, pin, UR_CHANGED, m_appendUndo );
                    m_appendUndo = true;
                    static_cast<SCH_TEXT*>( pin )->SetText( aNewName );
                }
            }
        }

        m_reporter.ReportHead( msg, RPT_SEVERITY_ACTION );
        break;

    case SCH_SHEET_PIN_T:
        ++m_changesCount;
        msg.Printf( _( "Change \"%s\" hierarchical label to \"%s\"." ), aOldName, aNewName );

        if( !m_dryRun )
        {
            SCH_SCREEN* screen = aConn->Sheet().LastScreen();
            m_frame->SaveCopyInUndoList( screen, driver, UR_CHANGED, m_appendUndo );
            m_appendUndo = true;
            static_cast<SCH_TEXT*>( driver )->SetText( aNewName );

            SCH_SHEET* sheet = static_cast<SCH_SHEET_PIN*>( driver )->GetParent();
            screen = sheet->GetScreen();
            editMatchingLabels( screen, SCH_HIER_LABEL_T, aOldName, aNewName );
        }

        m_reporter.ReportHead( msg, RPT_SEVERITY_ACTION );
        break;

    case LIB_PIN_T:
    {
        LIB_PIN*         pin = dynamic_cast<LIB_PIN*>( driver );
        LABEL_SPIN_STYLE spin = LABEL_SPIN_STYLE::RIGHT;

        if( pin->IsPowerConnection() )
        {
            msg.Printf( _( "Net \"%s\" cannot be changed to \"%s\" because it "
                           "is driven by a power pin." ),
                        aOldName,
                        aNewName );

            m_reporter.ReportHead( msg, RPT_SEVERITY_ERROR );
            break;
        }

        switch( pin->GetOrientation() )
        {
        case PIN_UP:    spin = LABEL_SPIN_STYLE::UP;     break;
        case PIN_DOWN:  spin = LABEL_SPIN_STYLE::BOTTOM; break;
        case PIN_LEFT:  spin = LABEL_SPIN_STYLE::LEFT;   break;
        case PIN_RIGHT: spin = LABEL_SPIN_STYLE::RIGHT;  break;
        }

        ++m_changesCount;
        msg.Printf( _( "Add label \"%s\" to net \"%s\"." ), aNewName, aOldName );

        if( !m_dryRun )
        {
            SCHEMATIC_SETTINGS& settings = m_frame->Schematic().Settings();
            SCH_LABEL* label = new SCH_LABEL( driver->GetPosition(), aNewName );
            label->SetParent( &m_frame->Schematic() );
            label->SetTextSize( wxSize( settings.m_DefaultTextSize,
                                        settings.m_DefaultTextSize ) );
            label->SetLabelSpinStyle( spin );
            label->SetFlags( IS_NEW );

            SCH_SCREEN* screen = aConn->Sheet().LastScreen();
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