/**
 * @file eeschema/tools/backannotate.cpp
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Alexander Shuklin <Jasuramme@gmail.com>
 * Copyright (C) 2004-2019 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <kiface_i.h>
#include <macros.h>
#include <ptree.h>
#include <reporter.h>
#include <sch_edit_frame.h>
#include <sch_sheet_path.h>
#include <schematic.h>
#include <utf8.h>


BACK_ANNOTATE::BACK_ANNOTATE( SCH_EDIT_FRAME* aFrame, SETTINGS aSettings ) :
        m_settings( aSettings ),
        m_frame( aFrame ),
        m_changesCount( 0 )
{
}


BACK_ANNOTATE::~BACK_ANNOTATE()
{
}


bool BACK_ANNOTATE::BackAnnotateSymbols( const std::string& aNetlist )
{
    m_changesCount = 0;
    wxString msg;

    if( !m_settings.processValues && !m_settings.processFootprints
            && !m_settings.processReferences )
    {
        m_settings.reporter.ReportTail(
                _( "Select at least one property to back annotate" ), RPT_SEVERITY_ERROR );
        return false;
    }

    int errors = getPcbModulesFromString( aNetlist );

    SCH_SHEET_LIST sheets = m_frame->Schematic().GetSheets();
    sheets.GetComponents( m_refs, false );
    sheets.GetMultiUnitComponents( m_multiUnitsRefs );

    errors += getChangeList();
    errors += checkForUnusedSymbols();
    errors += checkSharedSchematicErrors();

    if( errors > 0 )
        m_settings.dryRun = true;
    applyChangelist();

    if( !errors )
    {

        if( !m_settings.dryRun )
        {
            msg.Printf( _( "Schematic is back-annotated. %d changes applied." ), m_changesCount );
            m_settings.reporter.ReportTail( msg, RPT_SEVERITY_ACTION );
        }
        else
            m_settings.reporter.ReportTail(
                    _( "No errors during dry run. Ready to go." ), RPT_SEVERITY_ACTION );
    }
    else
    {
        msg.Printf( _( "Found %d errors. Fix them and run back annotation." ), errors );
        m_settings.reporter.ReportTail( msg, RPT_SEVERITY_ERROR );
    }

    return !errors;
}

bool BACK_ANNOTATE::FetchNetlistFromPCB( std::string& aNetlist )
{

    if( Kiface().IsSingle() )
    {
        DisplayErrorMessage(
                m_frame, _( "Cannot fetch PCB netlist because eeschema is opened in "
                            "stand-alone mode.\n"
                            "You must launch the KiCad project manager "
                            "and create a project." ) );
        return false;
    }

    KIWAY_PLAYER* player = m_frame->Kiway().Player( FRAME_PCB_EDITOR, false );

    if( !player )
    {
        DisplayErrorMessage(
                this->m_frame, _( "Please open Pcbnew and run back annotation again" ) );
        return false;
    }

    m_frame->Kiway().ExpressMail( FRAME_PCB_EDITOR, MAIL_PCB_GET_NETLIST, aNetlist );
    return true;
}


int BACK_ANNOTATE::getPcbModulesFromString( const std::string& aPayload )
{
    DSNLEXER lexer( aPayload, FROM_UTF8( __func__ ) );
    PTREE    doc;
    Scan( &doc, &lexer );
    CPTREE& tree = doc.get_child( "pcb_netlist" );
    wxString msg;

    int errors = 0;
    m_pcbModules.clear();
    for( auto& item : tree )
    {
        wxString path, value, footprint;
        wxASSERT( item.first == "ref" );
        wxString ref = UTF8( item.second.front().first );
        try
        {
            path = UTF8( item.second.get_child( "timestamp" ).front().first );

            if( path == "" )
            {
                msg.Printf( _( "Footprint \"%s\" has no symbol associated." ), ref );
                m_settings.reporter.ReportHead( msg, RPT_SEVERITY_WARNING );
                continue;
            }
            footprint = UTF8( item.second.get_child( "fpid" ).front().first );
            value     = UTF8( item.second.get_child( "value" ).front().first );
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
                    nearestItem->second->ref, ref );
            m_settings.reporter.ReportHead( msg, RPT_SEVERITY_ERROR );
            ++errors;
        }
        else
        {
            // Add module to the map
            PCB_MODULE_DATA data( ref, footprint, value );
            m_pcbModules.insert( nearestItem,
                    std::make_pair( path, std::make_shared<PCB_MODULE_DATA>( data ) ) );
        }
    }
    return errors;
}

int BACK_ANNOTATE::getChangeList()
{
    int errors = 0;

    for( auto& module : m_pcbModules )
    {
        auto& pcbPath = module.first;
        auto& pcbData = module.second;
        bool  foundInMultiunit = false;

        for( auto& item : m_multiUnitsRefs )
        {
            auto& refList = item.second;

            if( refList.FindRefByPath( pcbPath ) >= 0 )
            {

                // If module linked to multi unit symbol, we add all symbol's units to
                // the change list
                foundInMultiunit = true;
                for( size_t i = 0; i < refList.GetCount(); ++i )
                {
                    m_changelist.push_back( CHANGELIST_ITEM( refList[i], pcbData ) );
                }
                break;
            }
        }

        if( foundInMultiunit )
            continue;

        int refIndex = m_refs.FindRefByPath( pcbPath );

        if( refIndex >= 0 )
            m_changelist.push_back( CHANGELIST_ITEM( m_refs[refIndex], pcbData ) );
        else
        {
            // Haven't found linked symbol in multiunits or common refs. Generate error
            wxString msg;
            msg.Printf( _( "Cannot find symbol for \"%s\" footprint" ), pcbData->ref );
            ++errors;
            m_settings.reporter.ReportTail( msg, RPT_SEVERITY_ERROR );
        }
    }
    return errors;
}

int BACK_ANNOTATE::checkForUnusedSymbols()
{
    int errors = 0;

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
            ++errors;
            wxString msg;
            msg.Printf( _( "Cannot find footprint for \"%s\" symbol" ), m_refs[i++].GetFullRef() );
            m_settings.reporter.ReportTail( msg, RPT_SEVERITY_ERROR );
        }

        ++i;
    }
    return errors;
}


bool BACK_ANNOTATE::checkReuseViolation( PCB_MODULE_DATA& aFirst, PCB_MODULE_DATA& aSecond )
{

    if( m_settings.processFootprints && aFirst.footprint != aSecond.footprint )
        return false;

    if( m_settings.processValues && aFirst.value != aSecond.value )
        return false;
    return true;
}

wxString BACK_ANNOTATE::getTextFromField( const SCH_REFERENCE& aRef, const NumFieldType aField )
{
    return aRef.GetComp()->GetField( aField )->GetText();
}


int BACK_ANNOTATE::checkSharedSchematicErrors()
{

    std::sort( m_changelist.begin(), m_changelist.end(),
                    []( CHANGELIST_ITEM& a, CHANGELIST_ITEM& b ) {
                return a.first.GetComp() > b.first.GetComp();
            } );

    // We don't check that if no footprints or values updating
    if( !m_settings.processFootprints && !m_settings.processValues )
        return 0;
    int errors = 0;

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
                ++errors;
                wxString msg;
                msg.Printf( _( "\"%s\" and \"%s\" use the same schematic symbol.\n"
                               "They cannot have different footprints or values." ),
                           ( it + 1 )->second->ref, it->second->ref );
                m_settings.reporter.ReportTail( msg, RPT_SEVERITY_ERROR );
            }
        }
        else
        {
            /* Next ref uses different component, so we count all components number for current
            one. We compare that number to stored in the component itself. If that differs, it
            means that this particular component is reused in some another project. */
            if( !m_settings.ignoreOtherProjects && compUsage > usageCount )
            {
                PCB_MODULE_DATA tmp{ "", getTextFromField( it->first, FOOTPRINT ),
                    getTextFromField( it->first, VALUE ) };
                if( !checkReuseViolation( tmp, *it->second ) )
                {
                    wxString msg;
                    msg.Printf( _( "Unable to change \"%s\" footprint or value because associated"
                                   " symbol is reused in the another project" ),
                                it->second->ref );
                    m_settings.reporter.ReportTail( msg, RPT_SEVERITY_ERROR );
                    ++errors;
                }
            }
            usageCount = 1;
        }
    }
    return errors;
}


void BACK_ANNOTATE::applyChangelist()
{
    wxString msg;
    int      leftUnchanged = 0;

    // Apply changes from change list
    for( auto& item : m_changelist )
    {
        SCH_REFERENCE&   ref = item.first;
        PCB_MODULE_DATA& module = *item.second;
        wxString         oldFootprint = getTextFromField( ref, FOOTPRINT );
        wxString         oldValue = getTextFromField( ref, VALUE );
        int              changesCountBefore = m_changesCount;

        if( m_settings.processReferences && ref.GetRef() != module.ref )
        {
            ++m_changesCount;
            msg.Printf( _( "Change \"%s\" reference designator to \"%s\"." ), ref.GetFullRef(), module.ref );
            if( !m_settings.dryRun )
                ref.GetComp()->SetRef( &ref.GetSheetPath(), module.ref );
            m_settings.reporter.ReportHead( msg, RPT_SEVERITY_ACTION );
        }

        if( m_settings.processFootprints && oldFootprint != module.footprint )
        {
            ++m_changesCount;
            msg.Printf( _( "Change %s footprint from \"%s\" to \"%s\"." ), ref.GetFullRef(),
                    getTextFromField( ref, FOOTPRINT ), module.footprint );

            if( !m_settings.dryRun )
                ref.GetComp()->GetField( FOOTPRINT )->SetText( module.footprint );
            m_settings.reporter.ReportHead( msg, RPT_SEVERITY_ACTION );
        }

        if( m_settings.processValues && oldValue != module.value )
        {
            ++m_changesCount;
            msg.Printf( _( "Change \"%s\" value from \"%s\" to \"\"%s\"." ), ref.GetFullRef(),
                    getTextFromField( ref, VALUE ), module.value );
            if( !m_settings.dryRun )
                item.first.GetComp()->GetField( VALUE )->SetText( module.value );
            m_settings.reporter.ReportHead( msg, RPT_SEVERITY_ACTION );
        }

        if( changesCountBefore == m_changesCount )
            ++leftUnchanged;
    }
    msg.Printf( _( "%d symbols left unchanged" ), leftUnchanged );
    m_settings.reporter.ReportHead( msg, RPT_SEVERITY_INFO );
}
