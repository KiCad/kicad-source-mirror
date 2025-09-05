/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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
 * along with this program.  If not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <algorithm>
#include <wx/grid.h>
#include <sch_edit_frame.h>
#include <schematic.h>
#include <sch_symbol.h>
#include <sch_pin.h>
#include <sch_connection.h>
#include <connection_graph.h>
#include <dialog_link_components.h>

DIALOG_LINK_COMPONENTS::DIALOG_LINK_COMPONENTS( SCH_EDIT_FRAME* aParent,
                                                const std::vector<SCH_SYMBOL*>& aSources,
                                                std::vector<SCH_SYMBOL*> aTargets ) :
        DIALOG_LINK_COMPONENTS_BASE( aParent ),
        m_frame( aParent ),
        m_initialSources( aSources )
{
    std::set<SCH_SYMBOL*> srcSet( aSources.begin(), aSources.end() );
    for( SCH_SYMBOL* sym : aTargets )
    {
        if( !srcSet.count( sym ) )
            m_initialTargets.push_back( sym );
    }

    TransferDataToWindow();

    for( size_t i = 0; i < m_filteredSources.size(); ++i )
    {
        if( std::find( m_initialSources.begin(), m_initialSources.end(), m_filteredSources[i] )
                != m_initialSources.end() )
        {
            m_listSources->SetSelection( i );
        }
    }

    for( size_t i = 0; i < m_filteredTargets.size(); ++i )
    {
        if( std::find( m_initialTargets.begin(), m_initialTargets.end(), m_filteredTargets[i] )
                != m_initialTargets.end() )
        {
            m_listTargets->SetSelection( i );
        }
    }

    wxCommandEvent ev;
    OnSrcSelect( ev );
    OnTargetSelect( ev );
    updateGrid();
}


bool DIALOG_LINK_COMPONENTS::TransferDataToWindow()
{
    m_allSources.clear();
    m_allTargets.clear();
    m_allNets.clear();

    SCH_REFERENCE_LIST refs;
    m_frame->Schematic().Hierarchy().GetSymbols( refs );
    for( const SCH_REFERENCE& ref : refs )
    {
        SCH_SYMBOL* sym = ref.GetSymbol();
        if( sym )
        {
            m_allSources.push_back( sym );
            m_allTargets.push_back( sym );
        }
    }

    CONNECTION_GRAPH* graph = m_frame->Schematic().ConnectionGraph();
    if( graph )
    {
        std::set<wxString> nets;
        for( const auto& netEntry : graph->GetNetMap() )
        {
            for( CONNECTION_SUBGRAPH* sub : netEntry.second )
            {
                wxString name = sub->GetNetName();
                if( !name.IsEmpty() )
                    nets.insert( name );
            }
        }
        m_allNets.assign( nets.begin(), nets.end() );
    }

    if( m_gridKiLinks->GetNumberRows() )
        m_gridKiLinks->DeleteRows( 0, m_gridKiLinks->GetNumberRows() );

    if( m_gridKiLinks->GetNumberCols() < 5 )
        m_gridKiLinks->AppendCols( 5 - m_gridKiLinks->GetNumberCols() );

    m_gridKiLinks->SetColLabelValue( 0, wxEmptyString );
    m_gridKiLinks->SetColLabelValue( 1, _( "Name" ) );
    m_gridKiLinks->SetColLabelValue( 2, _( "Connections" ) );
    m_gridKiLinks->SetColLabelValue( 3, _( "Source" ) );
    m_gridKiLinks->SetColLabelValue( 4, _( "Target" ) );

    wxGridCellAttr* attrBool = new wxGridCellAttr;
    attrBool->SetRenderer( new wxGridCellBoolRenderer );
    attrBool->SetEditor( new wxGridCellBoolEditor );
    m_gridKiLinks->SetColAttr( 0, attrBool );

    wxGridCellAttr* attrRO = new wxGridCellAttr;
    attrRO->SetReadOnly();
    m_gridKiLinks->SetColAttr( 2, attrRO );

    wxGridCellAttr* attrRO2 = new wxGridCellAttr;
    attrRO2->SetReadOnly();
    m_gridKiLinks->SetColAttr( 3, attrRO2 );

    wxGridCellAttr* attrRO3 = new wxGridCellAttr;
    attrRO3->SetReadOnly();
    m_gridKiLinks->SetColAttr( 4, attrRO3 );

    updateSourceList();
    updateTargetList();
    updateNetList();
    updateGrid();

    return true;
}


bool DIALOG_LINK_COMPONENTS::TransferDataFromWindow()
{
    for( int row = 0; row < (int) m_linkRows.size(); ++row )
    {
        m_linkRows[row].name = m_gridKiLinks->GetCellValue( row, 1 );
        m_linkRows[row].selected = m_gridKiLinks->GetCellValue( row, 0 ) == wxS( "1" );
    }
    return true;
}


void DIALOG_LINK_COMPONENTS::OnSrcFilter( wxCommandEvent& )
{
    updateSourceList();
    updateTargetList();
    updateNetList();
    updateGrid();
}


void DIALOG_LINK_COMPONENTS::OnTargetFilter( wxCommandEvent& )
{
    updateTargetList();
    updateSourceList();
    updateNetList();
    updateGrid();
}


void DIALOG_LINK_COMPONENTS::OnNetFilter( wxCommandEvent& )
{
    updateNetList();
    updateSourceList();
    updateTargetList();
    updateGrid();
}


void DIALOG_LINK_COMPONENTS::OnSrcSelect( wxCommandEvent& )
{
    updateTargetList();
    updateNetList();
    updateGrid();
}


void DIALOG_LINK_COMPONENTS::OnTargetSelect( wxCommandEvent& )
{
    updateSourceList();
    updateNetList();
    updateGrid();
}


void DIALOG_LINK_COMPONENTS::OnNetSelect( wxCommandEvent& )
{
    updateSourceList();
    updateTargetList();
    updateGrid();
}


void DIALOG_LINK_COMPONENTS::OnOK( wxCommandEvent& aEvent )
{
    if( TransferDataFromWindow() )
        EndModal( wxID_OK );
}


void DIALOG_LINK_COMPONENTS::updateSourceList()
{
    m_listSources->Clear();
    m_filteredSources.clear();

    wxString filter = m_filterSource->GetValue().Lower();
    std::set<SCH_SYMBOL*> base( m_allSources.begin(), m_allSources.end() );

    wxArrayInt netSel;
    m_srcNets->GetSelections( netSel );
    if( netSel.Count() )
    {
        std::set<wxString> nets;
        for( int idx : netSel )
            nets.insert( m_filteredNets[idx] );

        std::set<SCH_SYMBOL*> allowed;
        for( SCH_SYMBOL* sym : base )
        {
            auto snets = getNets( sym );
            for( const wxString& n : nets )
            {
                if( snets.count( n ) )
                {
                    allowed.insert( sym );
                    break;
                }
            }
        }
        base = allowed;
    }

    wxArrayInt tgtSel;
    m_listTargets->GetSelections( tgtSel );
    if( tgtSel.Count() )
    {
        std::set<SCH_SYMBOL*> allowed;
        for( int idx : tgtSel )
        {
            SCH_SYMBOL* tgt = m_filteredTargets[idx];
            for( SCH_SYMBOL* src : base )
            {
                if( connected( src, tgt ) )
                    allowed.insert( src );
            }
        }
        base = allowed;
    }

    for( SCH_SYMBOL* sym : base )
    {
        wxString name = sym->GetRef( &m_frame->GetCurrentSheet() );
        if( filter.IsEmpty() || name.Lower().Find( filter ) != wxNOT_FOUND )
        {
            m_listSources->Append( name );
            m_filteredSources.push_back( sym );
        }
    }
}


void DIALOG_LINK_COMPONENTS::updateTargetList()
{
    m_listTargets->Clear();
    m_filteredTargets.clear();

    wxString filter = m_filterDest->GetValue().Lower();
    std::set<SCH_SYMBOL*> base( m_allTargets.begin(), m_allTargets.end() );

    wxArrayInt srcSel;
    m_listSources->GetSelections( srcSel );
    if( srcSel.Count() )
    {
        std::set<SCH_SYMBOL*> allowed;
        for( int idx : srcSel )
        {
            SCH_SYMBOL* src = m_filteredSources[idx];
            for( SCH_SYMBOL* dst : base )
            {
                if( connected( src, dst ) && src != dst )
                    allowed.insert( dst );
            }
        }
        base = allowed;
    }

    for( SCH_SYMBOL* sym : base )
    {
        wxString name = sym->GetRef( &m_frame->GetCurrentSheet() );
        if( filter.IsEmpty() || name.Lower().Find( filter ) != wxNOT_FOUND )
        {
            m_listTargets->Append( name );
            m_filteredTargets.push_back( sym );
        }
    }
}


void DIALOG_LINK_COMPONENTS::updateNetList()
{
    m_srcNets->Clear();
    m_filteredNets.clear();

    wxString filter = m_filterNets->GetValue().Lower();

    std::set<wxString> nets;
    if( m_filteredSources.empty() )
    {
        nets.insert( m_allNets.begin(), m_allNets.end() );
    }
    else
    {
        for( SCH_SYMBOL* sym : m_filteredSources )
        {
            auto snets = getNets( sym );
            nets.insert( snets.begin(), snets.end() );
        }
    }

    for( const wxString& n : nets )
    {
        if( filter.IsEmpty() || n.Lower().Find( filter ) != wxNOT_FOUND )
        {
            m_srcNets->Append( n );
            m_filteredNets.push_back( n );
        }
    }
}


void DIALOG_LINK_COMPONENTS::updateGrid()
{
    if( m_gridKiLinks->GetNumberRows() )
        m_gridKiLinks->DeleteRows( 0, m_gridKiLinks->GetNumberRows() );

    m_linkRows.clear();

    wxArrayInt srcSel;
    wxArrayInt tgtSel;
    m_listSources->GetSelections( srcSel );
    m_listTargets->GetSelections( tgtSel );

    for( int si = 0; si < srcSel.Count(); ++si )
    {
        SCH_SYMBOL* src = m_filteredSources[srcSel[si]];
        wxString srcName = src->GetRef( &m_frame->GetCurrentSheet() );
        for( int ti = 0; ti < tgtSel.Count(); ++ti )
        {
            SCH_SYMBOL* dst = m_filteredTargets[tgtSel[ti]];
            if( !connected( src, dst ) )
                continue;

            KILINK_ROW row;
            row.source = src;
            row.target = dst;
            row.connections = connectionCount( src, dst );
            row.name = wxEmptyString;
            row.selected = false;
            row.existing = false;

            CONNECTION_GRAPH* graph = m_frame->Schematic().ConnectionGraph();
            if( graph )
            {
                if( auto name = graph->GetKiLinkName( src->m_Uuid, dst->m_Uuid ) )
                {
                    row.name = *name;
                    row.selected = true;
                    row.existing = true;
                }
            }

            int r = m_gridKiLinks->GetNumberRows();
            m_gridKiLinks->AppendRows( 1 );
            m_gridKiLinks->SetCellValue( r, 0, row.selected ? wxS( "1" ) : wxS( "0" ) );
            m_gridKiLinks->SetCellValue( r, 1, row.name );
            m_gridKiLinks->SetCellValue( r, 2, wxString::Format( wxS( "%d" ), row.connections ) );
            m_gridKiLinks->SetCellValue( r, 3, srcName );
            m_gridKiLinks->SetCellValue( r, 4, dst->GetRef( &m_frame->GetCurrentSheet() ) );

            m_linkRows.push_back( row );
        }
    }
}


std::set<wxString> DIALOG_LINK_COMPONENTS::getNets( SCH_SYMBOL* aSymbol ) const
{
    std::set<wxString> nets;
    if( !aSymbol )
        return nets;

    for( SCH_PIN* pin : aSymbol->GetPins() )
    {
        if( pin && pin->Connection() )
            nets.insert( pin->Connection()->Name() );
    }
    return nets;
}


bool DIALOG_LINK_COMPONENTS::connected( SCH_SYMBOL* aSrc, SCH_SYMBOL* aDst ) const
{
    if( !aSrc || !aDst || aSrc == aDst )
        return false;

    for( SCH_PIN* pa : aSrc->GetPins() )
    {
        if( !pa->Connection() )
            continue;
        for( SCH_PIN* pb : aDst->GetPins() )
        {
            if( pb->Connection() && pa->Connection()->Name() == pb->Connection()->Name() )
                return true;
        }
    }

    CONNECTION_GRAPH* graph = m_frame->Schematic().ConnectionGraph();
    if( graph )
    {
        for( SCH_PIN* pa : aSrc->GetPins() )
        {
            for( SCH_PIN* pb : aDst->GetPins() )
            {
                if( graph->FindPotentialSignalBetweenPins( pa, pb ) )
                    return true;
            }
        }
    }

    return false;
}


int DIALOG_LINK_COMPONENTS::connectionCount( SCH_SYMBOL* aSrc, SCH_SYMBOL* aDst ) const
{
    if( !aSrc || !aDst )
        return 0;

    CONNECTION_GRAPH* graph = m_frame->Schematic().ConnectionGraph();
    if( !graph )
        return 0;

    std::set<wxString> consideredNets;

    wxArrayInt netSel;
    m_srcNets->GetSelections( netSel );
    if( netSel.Count() )
    {
        for( int idx : netSel )
            consideredNets.insert( m_filteredNets[idx] );
    }
    else
    {
        for( SCH_PIN* pin : aSrc->GetPins() )
        {
            if( pin->Connection() )
                consideredNets.insert( pin->Connection()->Name() );
        }
    }

    int best = 0;

    for( SCH_PIN* pa : aSrc->GetPins() )
    {
        if( !pa->Connection() )
            continue;

        wxString srcNet = pa->Connection()->Name();
        if( !consideredNets.count( srcNet ) )
            continue;

        for( SCH_PIN* pb : aDst->GetPins() )
        {
            if( !pb->Connection() )
                continue;

            if( srcNet == pb->Connection()->Name() )
            {
                if( best == 0 || best > 1 )
                    best = 1;
                continue;
            }

            SCH_NETCHAIN* sig = graph->FindPotentialSignalBetweenPins( pa, pb );
            if( sig && sig->GetNets().contains( srcNet ) )
            {
                int count = sig->GetNets().size();
                if( best == 0 || count < best )
                    best = count;
            }
        }
    }

    return best;
}

