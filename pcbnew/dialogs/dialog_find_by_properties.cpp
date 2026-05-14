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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "dialog_find_by_properties.h"

#include <pcb_edit_frame.h>
#include <board.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_track.h>
#include <pcb_shape.h>
#include <pcb_text.h>
#include <zone.h>
#include <tool/tool_manager.h>
#include <tools/pcb_selection_tool.h>
#include <tools/pcb_actions.h>
#include <tool/actions.h>
#include <properties/property_mgr.h>
#include <properties/property.h>
#include <properties/pg_properties.h>
#include <scintilla_tricks.h>
#include <pcbexpr_evaluator.h>
#include <drc/drc_rule_condition.h>
#include <project.h>
#include <project/project_file.h>
#include <wx/grid.h>
#include <wx/stc/stc.h>
#include <wx/msgdlg.h>
#include <wx/regex.h>

#include <algorithm>


DIALOG_FIND_BY_PROPERTIES::DIALOG_FIND_BY_PROPERTIES( PCB_EDIT_FRAME* aParent ) :
        DIALOG_FIND_BY_PROPERTIES_BASE( aParent, wxID_ANY, _( "Find by Properties" ) ),
        m_frame( aParent ),
        m_board( nullptr ),
        m_scintillaTricks( nullptr )
{
    m_propertyGrid->SetColLabelValue( 0, _( "Property" ) );
    m_propertyGrid->SetColLabelValue( 1, _( "Value" ) );
    m_propertyGrid->SetColLabelValue( 2, _( "Match" ) );
    m_propertyGrid->SetRowLabelSize( 0 );
    m_propertyGrid->SetColLabelSize( wxGRID_AUTOSIZE );
    m_propertyGrid->EnableEditing( true );
    m_propertyGrid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_CENTER );
    m_propertyGrid->SetSelectionMode( wxGrid::wxGridSelectRows );

    m_scintillaTricks = new SCINTILLA_TRICKS(
            m_queryEditor, wxT( "()" ), true,
            [this]( wxKeyEvent& aEvent )
            {
                wxCommandEvent dummy;
                onSelectMatchingClick( dummy );
            },
            [this]( wxStyledTextEvent& aEvent )
            {
                onScintillaCharAdded( aEvent );
            } );

    m_queryEditor->SetMarginWidth( 0, 0 );
    m_queryEditor->SetMarginWidth( 1, 0 );
    m_queryEditor->SetMarginWidth( 2, 0 );

    m_board = m_frame->GetBoard();

    m_frame->Bind( EDA_EVT_BOARD_CHANGED, &DIALOG_FIND_BY_PROPERTIES::OnBoardChanged, this );

    m_propertyGrid->Bind( wxEVT_GRID_CELL_CHANGED, &DIALOG_FIND_BY_PROPERTIES::onGridCellChanged, this );
    m_propertyGrid->Bind( wxEVT_GRID_CELL_LEFT_CLICK, &DIALOG_FIND_BY_PROPERTIES::onGridCellClick, this );
    m_propertyGrid->Bind( wxEVT_SIZE, &DIALOG_FIND_BY_PROPERTIES::onGridSizeChanged, this );

    loadRecentQueries();
    rebuildPropertyGrid();

    SetMinSize( wxSize( 350, 400 ) );
    m_propertyGrid->SetMinSize( wxSize( 0, 200 ) );
    m_selectMatchingBtn->SetDefault();
    Center();
}


DIALOG_FIND_BY_PROPERTIES::~DIALOG_FIND_BY_PROPERTIES()
{
    m_frame->Unbind( EDA_EVT_BOARD_CHANGED, &DIALOG_FIND_BY_PROPERTIES::OnBoardChanged, this );

    delete m_scintillaTricks;
}


bool DIALOG_FIND_BY_PROPERTIES::Show( bool show )
{
    if( show )
        rebuildPropertyGrid();

    return DIALOG_FIND_BY_PROPERTIES_BASE::Show( show );
}


void DIALOG_FIND_BY_PROPERTIES::OnBoardChanged( wxCommandEvent& event )
{
    m_board = m_frame->GetBoard();
    event.Skip();
}


void DIALOG_FIND_BY_PROPERTIES::OnSelectionChanged()
{
    if( IsShown() && m_notebook->GetSelection() == 0 )
        rebuildPropertyGrid();
}


wxVariant DIALOG_FIND_BY_PROPERTIES::anyToVariant( const wxAny& aValue )
{
    if( aValue.CheckType<int>() )
        return wxVariant( aValue.As<int>() );
    else if( aValue.CheckType<long>() )
        return wxVariant( aValue.As<long>() );
    else if( aValue.CheckType<long long>() )
        // Use double to avoid truncation on platforms where long is 32-bit
        return wxVariant( static_cast<double>( aValue.As<long long>() ) );
    else if( aValue.CheckType<double>() )
        return wxVariant( aValue.As<double>() );
    else if( aValue.CheckType<bool>() )
        return wxVariant( aValue.As<bool>() );
    else if( aValue.CheckType<wxString>() )
        return wxVariant( aValue.As<wxString>() );

    wxString strVal;

    if( aValue.GetAs( &strVal ) )
        return wxVariant( strVal );

    return wxVariant();
}


wxVariant DIALOG_FIND_BY_PROPERTIES::getVariantAwareValue( EDA_ITEM* aItem, PROPERTY_BASE* aProperty )
{
    if( aItem->Type() == PCB_FOOTPRINT_T )
    {
        FOOTPRINT*      footprint = static_cast<FOOTPRINT*>( aItem );
        const wxString& propName = aProperty->Name();
        wxString        variantName;

        if( footprint->GetBoard() )
            variantName = footprint->GetBoard()->GetCurrentVariant();

        if( !variantName.IsEmpty() )
        {
            if( propName == _HKI( "Do not Populate" ) )
                return wxVariant( footprint->GetDNPForVariant( variantName ) );
            else if( propName == _HKI( "Exclude From Bill of Materials" ) )
                return wxVariant( footprint->GetExcludedFromBOMForVariant( variantName ) );
            else if( propName == _HKI( "Exclude From Position Files" ) )
                return wxVariant( footprint->GetExcludedFromPosFilesForVariant( variantName ) );
        }
    }

    wxAny anyValue = aItem->Get( aProperty );

    if( anyValue.IsNull() )
        return wxVariant();

    return anyToVariant( anyValue );
}


namespace
{
wxVariant getFootprintFieldValue( FOOTPRINT* aFootprint, const wxString& aFieldName )
{
    PCB_FIELD* field = aFootprint ? aFootprint->GetField( aFieldName ) : nullptr;

    if( !field )
        return wxVariant();

    wxString variantName;

    if( aFootprint->GetBoard() )
        variantName = aFootprint->GetBoard()->GetCurrentVariant();

    if( !variantName.IsEmpty() )
    {
        if( const FOOTPRINT_VARIANT* variant = aFootprint->GetVariant( variantName );
            variant && variant->HasFieldValue( aFieldName ) )
        {
            return wxVariant( aFootprint->GetFieldValueForVariant( variantName, aFieldName ) );
        }
    }

    return wxVariant( field->GetText() );
}


std::set<wxString> getCommonFootprintFieldNames( const PCB_SELECTION& aSelection )
{
    std::set<wxString> commonFieldNames;
    bool               firstFootprint = true;

    for( EDA_ITEM* item : aSelection )
    {
        if( item->Type() != PCB_FOOTPRINT_T )
            return {};

        FOOTPRINT*         footprint = static_cast<FOOTPRINT*>( item );
        std::set<wxString> fieldNames;

        for( PCB_FIELD* field : footprint->GetFields() )
        {
            if( field )
                fieldNames.insert( field->GetCanonicalName() );
        }

        if( firstFootprint )
        {
            commonFieldNames = std::move( fieldNames );
            firstFootprint = false;
        }
        else
        {
            std::erase_if( commonFieldNames,
                           [&]( const wxString& aName )
                           {
                               return !fieldNames.count( aName );
                           } );
        }
    }

    return commonFieldNames;
}


wxString formatFindByPropertiesDisplayValue( PCB_EDIT_FRAME* aFrame, PROPERTY_BASE* aProperty, const wxVariant& aValue )
{
    if( aValue.IsNull() )
        return wxEmptyString;

    if( !aProperty )
        return aValue.GetString();

    wxVariant     valueCopy = aValue;
    wxPGProperty* pgProperty = nullptr;

    if( aProperty->TypeHash() == TYPE_HASH( PCB_LAYER_ID ) )
    {
        wxASSERT( aProperty->HasChoices() );

        const wxPGChoices& canonicalLayers = aProperty->Choices();
        wxArrayString      boardLayerNames;
        wxArrayInt         boardLayerIDs;

        for( int ii = 0; ii < (int) canonicalLayers.GetCount(); ++ii )
        {
            int layer = canonicalLayers.GetValue( ii );

            boardLayerNames.push_back( aFrame->GetBoard()->GetLayerName( ToLAYER_ID( layer ) ) );
            boardLayerIDs.push_back( layer );
        }

        auto layerProp = new PGPROPERTY_COLORENUM( new wxPGChoices( boardLayerNames, boardLayerIDs ) );
        layerProp->SetLabel( wxGetTranslation( aProperty->Name() ) );
        layerProp->SetName( aProperty->Name() );
        layerProp->SetHelpString( wxGetTranslation( aProperty->Name() ) );
        layerProp->SetClientData( const_cast<PROPERTY_BASE*>( aProperty ) );
        pgProperty = layerProp;
    }
    else
    {
        pgProperty = PGPropertyFactory( aProperty, aFrame );
    }

    if( pgProperty )
    {
        wxString formatted = pgProperty->ValueToString( valueCopy, 0 );
        delete pgProperty;

        if( !formatted.IsEmpty() )
            return formatted;
    }

    return aValue.GetString();
}


wxString normalizeFormattedValueForExpression( PROPERTY_BASE* aProperty, const wxString& aValue )
{
    wxString normalized = aValue;

    switch( aProperty->Display() )
    {
    case PROPERTY_DISPLAY::PT_COORD:
    case PROPERTY_DISPLAY::PT_SIZE:
    case PROPERTY_DISPLAY::PT_TIME:
        normalized.Replace( wxT( " " ), wxEmptyString );
        normalized.Replace( wxT( "mils" ), wxT( "mil" ) );
        break;

    case PROPERTY_DISPLAY::PT_DEGREE:
    case PROPERTY_DISPLAY::PT_DECIDEGREE:
        normalized.Replace( wxT( "°" ), wxT( "deg" ) );
        normalized.Replace( wxT( " " ), wxEmptyString );
        break;

    default: break;
    }

    return normalized;
}


bool isExprIdentChar( wxChar aCh )
{
    return wxIsalnum( aCh ) || aCh == wxT( '_' );
}


std::set<wxString> getQueryableFootprintFieldNames( const std::vector<PROPERTY_ROW_DATA>& aRows )
{
    std::set<wxString> fieldNames = { _HKI( "Reference" ), _HKI( "Value" ), _HKI( "Datasheet" ),
                                      _HKI( "Description" ) };

    for( const PROPERTY_ROW_DATA& row : aRows )
    {
        if( row.property == nullptr )
            fieldNames.insert( row.propertyName );
    }

    return fieldNames;
}


bool matchAliasAt( const wxString& aExpression, size_t aPos, const wxString& aAlias )
{
    if( aPos + aAlias.length() > aExpression.length() )
        return false;

    if( aExpression.Mid( aPos, aAlias.length() ) != aAlias )
        return false;

    if( aPos > 0 && isExprIdentChar( aExpression[aPos - 1] ) )
        return false;

    size_t end = aPos + aAlias.length();

    if( end < aExpression.length() && isExprIdentChar( aExpression[end] ) )
        return false;

    return true;
}


wxString normalizeQueryFieldAliases( const wxString& aExpression, const std::vector<PROPERTY_ROW_DATA>& aRows )
{
    struct FIELD_ALIAS
    {
        wxString from;
        wxString to;
    };

    std::vector<FIELD_ALIAS> aliases;

    for( const wxString& fieldName : getQueryableFootprintFieldNames( aRows ) )
    {
        wxString exprName = fieldName;
        exprName.Replace( wxT( " " ), wxT( "_" ) );

        wxString escapedFieldName = fieldName;
        escapedFieldName.Replace( wxT( "'" ), wxT( "\\'" ) );

        aliases.push_back(
                { wxT( "A." ) + exprName, wxString::Format( wxT( "A.getField('%s')" ), escapedFieldName ) } );
    }

    std::sort( aliases.begin(), aliases.end(),
               []( const FIELD_ALIAS& aLhs, const FIELD_ALIAS& aRhs )
               {
                   return aLhs.from.length() > aRhs.from.length();
               } );

    wxString normalized;
    bool     inString = false;

    for( size_t i = 0; i < aExpression.length(); )
    {
        wxChar ch = aExpression[i];

        if( ch == wxT( '\'' ) && ( i == 0 || aExpression[i - 1] != wxT( '\\' ) ) )
        {
            inString = !inString;
            normalized += ch;
            ++i;
            continue;
        }

        if( !inString )
        {
            bool replaced = false;

            for( const FIELD_ALIAS& alias : aliases )
            {
                if( matchAliasAt( aExpression, i, alias.from ) )
                {
                    normalized += alias.to;
                    i += alias.from.length();
                    replaced = true;
                    break;
                }
            }

            if( replaced )
                continue;
        }

        normalized += ch;
        ++i;
    }

    return normalized;
}


bool queryUsesUnsupportedPairwiseSyntax( const wxString& aExpression )
{
    bool inString = false;

    for( size_t i = 0; i < aExpression.length(); ++i )
    {
        wxChar ch = aExpression[i];

        if( ch == wxT( '\'' ) && ( i == 0 || aExpression[i - 1] != wxT( '\\' ) ) )
        {
            inString = !inString;
            continue;
        }

        if( !inString && i + 2 <= aExpression.length() && aExpression.Mid( i, 2 ) == wxT( "B." )
            && ( i == 0 || !isExprIdentChar( aExpression[i - 1] ) ) )
        {
            return true;
        }
    }

    return false;
}


bool isVisibleInFindByProperties( const wxString& aName, const PROPERTY_BASE* aProperty )
{
    if( aProperty->IsHiddenFromPropertiesManager() && aName != wxS( "Type" ) )
        return false;

    if( aProperty->IsHiddenFromDesignEditors() )
        return false;

    return true;
}
} // namespace


void DIALOG_FIND_BY_PROPERTIES::rebuildPropertyGrid()
{
    PCB_SELECTION_TOOL*  selTool = m_frame->GetToolManager()->GetTool<PCB_SELECTION_TOOL>();
    const PCB_SELECTION& selection = selTool->GetSelection();

    if( m_propertyGrid->GetNumberRows() > 0 )
        m_propertyGrid->DeleteRows( 0, m_propertyGrid->GetNumberRows() );

    if( selection.Empty() )
    {
        m_statusLabel->SetLabel( _( "No items selected" ) );
        m_createQueryBtn->Enable( false );
        return;
    }

    std::map<wxString, PROPERTY_MATCH_MODE> savedModes;

    for( const PROPERTY_ROW_DATA& row : m_propertyRows )
    {
        if( row.matchMode != PROPERTY_MATCH_MODE::IGNORED )
            savedModes[row.propertyName] = row.matchMode;
    }

    m_propertyRows.clear();
    m_selectedTypes.clear();

    if( selection.Size() == 1 )
        m_statusLabel->SetLabel( selection.Front()->GetFriendlyName() );
    else
        m_statusLabel->SetLabel( wxString::Format( _( "%d objects selected" ), selection.Size() ) );

    for( EDA_ITEM* item : selection )
        m_selectedTypes.insert( TYPE_HASH( *item ) );

    PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();

    std::map<wxString, PROPERTY_BASE*> commonProps;
    const std::vector<PROPERTY_BASE*>& firstProps = propMgr.GetProperties( *m_selectedTypes.begin() );

    for( PROPERTY_BASE* prop : firstProps )
        commonProps.emplace( prop->Name(), prop );

    for( auto it = std::next( m_selectedTypes.begin() ); it != m_selectedTypes.end(); ++it )
    {
        for( auto propIt = commonProps.begin(); propIt != commonProps.end(); )
        {
            if( !propMgr.GetProperty( *it, propIt->first ) )
                propIt = commonProps.erase( propIt );
            else
                ++propIt;
        }
    }

    for( auto& [name, property] : commonProps )
    {
        if( !isVisibleInFindByProperties( name, property ) )
            continue;

        bool available = true;

        for( EDA_ITEM* item : selection )
        {
            if( !propMgr.IsAvailableFor( TYPE_HASH( *item ), property, item ) )
            {
                available = false;
                break;
            }
        }

        if( !available )
            continue;

        PROPERTY_ROW_DATA row;
        row.propertyName = name;
        row.property = property;
        row.matchMode = PROPERTY_MATCH_MODE::IGNORED;
        row.isMixed = false;

        bool first = true;
        bool different = false;

        for( EDA_ITEM* item : selection )
        {
            PROPERTY_BASE* itemProp = propMgr.GetProperty( TYPE_HASH( *item ), name );

            if( !itemProp )
            {
                available = false;
                break;
            }

            wxVariant value = getVariantAwareValue( item, itemProp );

            if( value.IsNull() )
            {
                available = false;
                break;
            }

            if( first )
            {
                row.rawValue = value;
                first = false;
            }
            else if( !different && !row.rawValue.IsNull() && value != row.rawValue )
            {
                different = true;
                row.rawValue.MakeNull();
            }
        }

        if( !available )
            continue;

        row.isMixed = different;
        row.displayValue = different ? wxString( wxT( "<...>" ) )
                                     : formatFindByPropertiesDisplayValue( m_frame, row.property, row.rawValue );

        m_propertyRows.push_back( row );
    }

    for( const wxString& fieldName : getCommonFootprintFieldNames( selection ) )
    {
        if( commonProps.count( fieldName ) )
            continue;

        PROPERTY_ROW_DATA row;
        row.propertyName = fieldName;
        row.property = nullptr;
        row.matchMode = PROPERTY_MATCH_MODE::IGNORED;
        row.isMixed = false;

        bool first = true;
        bool different = false;
        bool available = true;

        for( EDA_ITEM* item : selection )
        {
            wxVariant value = getFootprintFieldValue( static_cast<FOOTPRINT*>( item ), fieldName );

            if( value.IsNull() )
            {
                available = false;
                break;
            }

            if( first )
            {
                row.rawValue = value;
                first = false;
            }
            else if( !different && !row.rawValue.IsNull() && value != row.rawValue )
            {
                different = true;
                row.rawValue.MakeNull();
            }
        }

        if( !available )
            continue;

        row.isMixed = different;
        row.displayValue = different ? wxString( wxT( "<...>" ) )
                                     : formatFindByPropertiesDisplayValue( m_frame, row.property, row.rawValue );

        m_propertyRows.push_back( row );
    }

    m_propertyGrid->AppendRows( m_propertyRows.size() );

    for( int i = 0; i < (int) m_propertyRows.size(); i++ )
    {
        m_propertyGrid->SetCellValue( i, 0, wxGetTranslation( m_propertyRows[i].propertyName ) );
        m_propertyGrid->SetCellValue( i, 1, m_propertyRows[i].displayValue );

        m_propertyGrid->SetReadOnly( i, 0 );
        m_propertyGrid->SetReadOnly( i, 1 );

        if( m_propertyRows[i].isMixed )
        {
            m_propertyGrid->SetReadOnly( i, 2 );
            m_propertyGrid->SetCellValue( i, 2, wxEmptyString );
            m_propertyGrid->SetCellTextColour( i, 2, wxSystemSettings::GetColour( wxSYS_COLOUR_GRAYTEXT ) );
        }
        else
        {
            wxString choices[] = { _( "Ignored" ), _( "Matching" ), _( "Different" ) };
            m_propertyGrid->SetCellEditor( i, 2, new wxGridCellChoiceEditor( 3, choices ) );
            m_propertyGrid->SetCellRenderer( i, 2, new wxGridCellStringRenderer() );

            updateMatchModeCell( i );
        }
    }

    m_propertyGrid->AutoSizeColumns();
    m_propertyGrid->SetColSize( 2, m_propertyGrid->GetTextExtent( _( "Different" ) ).GetWidth() + 40 );

    int totalWidth = m_propertyGrid->GetClientSize().GetWidth();
    int col0Width = m_propertyGrid->GetColSize( 0 );
    int col2Width = m_propertyGrid->GetColSize( 2 );
    int col1Width = totalWidth - col0Width - col2Width;

    if( col1Width > m_propertyGrid->GetColSize( 1 ) )
        m_propertyGrid->SetColSize( 1, col1Width );

    bool anyActive = false;

    for( int i = 0; i < (int) m_propertyRows.size(); i++ )
    {
        auto it = savedModes.find( m_propertyRows[i].propertyName );

        if( it != savedModes.end() && !m_propertyRows[i].isMixed )
        {
            m_propertyRows[i].matchMode = it->second;
            updateMatchModeCell( i );
            anyActive = true;
        }
    }

    m_createQueryBtn->Enable( anyActive );
}


void DIALOG_FIND_BY_PROPERTIES::onGridCellClick( wxGridEvent& aEvent )
{
    if( aEvent.GetCol() == 2 && aEvent.GetRow() >= 0 && aEvent.GetRow() < (int) m_propertyRows.size()
        && !m_propertyRows[aEvent.GetRow()].isMixed )
    {
        m_propertyGrid->SetGridCursor( aEvent.GetRow(), aEvent.GetCol() );
        m_propertyGrid->EnableCellEditControl();
    }
    else
    {
        aEvent.Skip();
    }
}


void DIALOG_FIND_BY_PROPERTIES::onGridCellChanged( wxGridEvent& aEvent )
{
    int row = aEvent.GetRow();
    int col = aEvent.GetCol();

    if( col != 2 || row < 0 || row >= (int) m_propertyRows.size() )
    {
        aEvent.Skip();
        return;
    }

    wxString           val = m_propertyGrid->GetCellValue( row, 2 );
    PROPERTY_ROW_DATA& data = m_propertyRows[row];

    if( val == _( "Matching" ) )
        data.matchMode = PROPERTY_MATCH_MODE::MATCHING;
    else if( val == _( "Different" ) )
        data.matchMode = PROPERTY_MATCH_MODE::DIFFERENT;
    else
        data.matchMode = PROPERTY_MATCH_MODE::IGNORED;

    updateMatchModeCell( row );

    bool anyActive = false;

    for( const PROPERTY_ROW_DATA& r : m_propertyRows )
    {
        if( r.matchMode != PROPERTY_MATCH_MODE::IGNORED )
        {
            anyActive = true;
            break;
        }
    }

    m_createQueryBtn->Enable( anyActive );
}


void DIALOG_FIND_BY_PROPERTIES::onGridSizeChanged( wxSizeEvent& aEvent )
{
    int totalWidth = m_propertyGrid->GetClientSize().GetWidth();
    int col0Width = m_propertyGrid->GetColSize( 0 );
    int col2Width = m_propertyGrid->GetColSize( 2 );
    int col1Width = totalWidth - col0Width - col2Width;

    if( col1Width > 0 )
        m_propertyGrid->SetColSize( 1, col1Width );

    aEvent.Skip();
}


void DIALOG_FIND_BY_PROPERTIES::updateMatchModeCell( int aRow )
{
    const PROPERTY_ROW_DATA& data = m_propertyRows[aRow];

    switch( data.matchMode )
    {
    case PROPERTY_MATCH_MODE::IGNORED: m_propertyGrid->SetCellValue( aRow, 2, _( "Ignored" ) ); break;

    case PROPERTY_MATCH_MODE::MATCHING: m_propertyGrid->SetCellValue( aRow, 2, _( "Matching" ) ); break;

    case PROPERTY_MATCH_MODE::DIFFERENT: m_propertyGrid->SetCellValue( aRow, 2, _( "Different" ) ); break;
    }

    m_propertyGrid->ForceRefresh();
}


void DIALOG_FIND_BY_PROPERTIES::onSelectMatchingClick( wxCommandEvent& event )
{
    if( m_notebook->GetSelection() == 0 )
        selectMatchingFromProperties();
    else
        selectMatchingFromQuery();
}


std::vector<BOARD_ITEM*> DIALOG_FIND_BY_PROPERTIES::collectAllBoardItems()
{
    std::vector<BOARD_ITEM*> items;

    if( !m_board )
        return items;

    for( PCB_TRACK* track : m_board->Tracks() )
        items.push_back( track );

    for( FOOTPRINT* fp : m_board->Footprints() )
    {
        items.push_back( fp );

        for( PAD* pad : fp->Pads() )
            items.push_back( pad );

        for( PCB_FIELD* field : fp->GetFields() )
            items.push_back( field );

        for( BOARD_ITEM* gi : fp->GraphicalItems() )
            items.push_back( gi );

        for( ZONE* zone : fp->Zones() )
            items.push_back( zone );
    }

    for( BOARD_ITEM* item : m_board->Drawings() )
        items.push_back( item );

    for( ZONE* zone : m_board->Zones() )
        items.push_back( zone );

    return items;
}


bool DIALOG_FIND_BY_PROPERTIES::itemMatchesPropertyCriteria( BOARD_ITEM* aItem )
{
    if( m_selectedTypes.find( TYPE_HASH( *aItem ) ) == m_selectedTypes.end() )
        return false;

    PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();

    for( const PROPERTY_ROW_DATA& row : m_propertyRows )
    {
        if( row.matchMode == PROPERTY_MATCH_MODE::IGNORED )
            continue;

        if( row.isMixed )
            continue;

        wxVariant itemValue;

        if( row.property == nullptr )
        {
            if( aItem->Type() != PCB_FOOTPRINT_T )
                return false;

            itemValue = getFootprintFieldValue( static_cast<FOOTPRINT*>( aItem ), row.propertyName );
        }
        else
        {
            PROPERTY_BASE* prop = propMgr.GetProperty( TYPE_HASH( *aItem ), row.propertyName );

            if( !prop )
                return false;

            if( !propMgr.IsAvailableFor( TYPE_HASH( *aItem ), prop, aItem ) )
                return false;

            itemValue = getVariantAwareValue( aItem, prop );
        }

        if( itemValue.IsNull() )
            return false;

        bool valuesEqual = ( itemValue == row.rawValue );

        if( row.matchMode == PROPERTY_MATCH_MODE::MATCHING && !valuesEqual )
            return false;

        if( row.matchMode == PROPERTY_MATCH_MODE::DIFFERENT && valuesEqual )
            return false;
    }

    return true;
}


void DIALOG_FIND_BY_PROPERTIES::applyMatchResults( EDA_ITEMS& aMatchList, wxStaticText* aStatusLabel )
{
    TOOL_MANAGER* toolMgr = m_frame->GetToolManager();

    if( m_deselectNonMatching->IsChecked() )
        toolMgr->RunAction( ACTIONS::selectionClear );

    if( !aMatchList.empty() )
        toolMgr->RunAction<EDA_ITEMS*>( ACTIONS::selectItems, &aMatchList );

    if( m_zoomToFit->IsChecked() && !aMatchList.empty() )
        toolMgr->RunAction( ACTIONS::zoomFitSelection );

    m_frame->GetCanvas()->ForceRefresh();

    aStatusLabel->SetLabel( wxString::Format( _( "%zu items matched" ), aMatchList.size() ) );
}


void DIALOG_FIND_BY_PROPERTIES::selectMatchingFromProperties()
{
    std::vector<BOARD_ITEM*> allItems = collectAllBoardItems();
    EDA_ITEMS                matchList;

    for( BOARD_ITEM* item : allItems )
    {
        if( itemMatchesPropertyCriteria( item ) )
            matchList.push_back( item );
    }

    applyMatchResults( matchList, m_statusLabel );
}


void DIALOG_FIND_BY_PROPERTIES::selectMatchingFromQuery()
{
    wxString expression = m_queryEditor->GetText().Trim().Trim( false );

    if( expression.IsEmpty() )
        return;

    wxString normalizedExpression = normalizeQueryFieldAliases( expression, m_propertyRows );

    if( queryUsesUnsupportedPairwiseSyntax( normalizedExpression ) )
    {
        wxString error = _( "B. expressions are not supported." );

        m_queryStatusLabel->SetLabel( error );
        wxMessageBox( error, _( "Expression Error" ), wxOK | wxICON_ERROR, this );
        return;
    }

    PCBEXPR_COMPILER compiler( new PCBEXPR_UNIT_RESOLVER() );
    PCBEXPR_UCODE    ucode;
    PCBEXPR_CONTEXT  preflightContext( 0, F_Cu );

    wxString errors;

    compiler.SetErrorCallback(
            [&]( const wxString& aMessage, int aOffset )
            {
                errors += aMessage + wxT( "\n" );
            } );

    bool ok = compiler.Compile( normalizedExpression.ToUTF8().data(), &ucode, &preflightContext );

    if( !ok )
    {
        m_queryStatusLabel->SetLabel( _( "Syntax error in expression" ) );
        wxMessageBox( errors, _( "Expression Error" ), wxOK | wxICON_ERROR, this );
        return;
    }

    saveRecentQuery( expression );

    std::vector<BOARD_ITEM*> allItems = collectAllBoardItems();
    EDA_ITEMS                matchList;

    for( BOARD_ITEM* item : allItems )
    {
        bool matched = false;
        LSET itemLayers = item->GetLayerSet();

        for( PCB_LAYER_ID layer : itemLayers.Seq() )
        {
            PCBEXPR_CONTEXT ctx( 0, layer );
            ctx.SetItems( item, nullptr );

            LIBEVAL::VALUE* result = ucode.Run( &ctx );

            if( result && result->AsDouble() != 0.0 )
            {
                matched = true;
                break;
            }
        }

        if( matched )
            matchList.push_back( item );
    }

    applyMatchResults( matchList, m_queryStatusLabel );
}


wxString DIALOG_FIND_BY_PROPERTIES::propNameToExprField( const wxString& aPropName )
{
    wxString result = aPropName;
    result.Replace( wxT( " " ), wxT( "_" ) );
    return result;
}


wxString DIALOG_FIND_BY_PROPERTIES::formatValueForExpression( PROPERTY_BASE* aProp, const wxVariant& aValue )
{
    if( aValue.GetType() == wxT( "bool" ) )
        return aValue.GetBool() ? wxT( "true" ) : wxT( "false" );

    if( aProp && aProp->HasChoices() )
    {
        wxString val = formatFindByPropertiesDisplayValue( m_frame, aProp, aValue );

        val.Replace( wxT( "'" ), wxT( "\\'" ) );

        return wxString::Format( wxT( "'%s'" ), val );
    }

    if( aProp )
    {
        switch( aProp->Display() )
        {
        case PROPERTY_DISPLAY::PT_COORD:
        case PROPERTY_DISPLAY::PT_SIZE:
        case PROPERTY_DISPLAY::PT_DEGREE:
        case PROPERTY_DISPLAY::PT_DECIDEGREE:
        case PROPERTY_DISPLAY::PT_TIME:
            return normalizeFormattedValueForExpression( aProp,
                                                         formatFindByPropertiesDisplayValue( m_frame, aProp, aValue ) );

        default: break;
        }
    }

    if( aValue.GetType() == wxT( "double" ) || aValue.GetType() == wxT( "long" ) )
        return aValue.GetString();

    wxString val = aProp ? formatFindByPropertiesDisplayValue( m_frame, aProp, aValue ) : aValue.GetString();

    val.Replace( wxT( "'" ), wxT( "\\'" ) );

    return wxString::Format( wxT( "'%s'" ), val );
}


wxString DIALOG_FIND_BY_PROPERTIES::generateExpressionFromProperties()
{
    wxArrayString conditions;

    for( const PROPERTY_ROW_DATA& row : m_propertyRows )
    {
        if( row.matchMode == PROPERTY_MATCH_MODE::IGNORED )
            continue;

        if( row.isMixed )
            continue;

        wxString lhs;

        if( row.property == nullptr )
        {
            lhs = wxString::Format( wxT( "A.getField(%s)" ),
                                    formatValueForExpression( nullptr, wxVariant( row.propertyName ) ) );
        }
        else
        {
            lhs = wxString::Format( wxT( "A.%s" ), propNameToExprField( row.propertyName ) );
        }

        wxString value = formatValueForExpression( row.property, row.rawValue );
        wxString op = ( row.matchMode == PROPERTY_MATCH_MODE::MATCHING ) ? wxT( "==" ) : wxT( "!=" );

        conditions.Add( wxString::Format( wxT( "%s %s %s" ), lhs, op, value ) );
    }

    wxString result;

    for( size_t i = 0; i < conditions.size(); i++ )
    {
        if( i > 0 )
            result += wxT( " && " );

        result += conditions[i];
    }

    return result;
}


void DIALOG_FIND_BY_PROPERTIES::onCreateQueryClick( wxCommandEvent& event )
{
    wxString expr = generateExpressionFromProperties();

    if( !expr.IsEmpty() )
    {
        m_queryEditor->SetText( expr );
        m_notebook->SetSelection( 1 );
    }
}


void DIALOG_FIND_BY_PROPERTIES::onCheckSyntaxClick( wxCommandEvent& event )
{
    wxString expression = m_queryEditor->GetText().Trim().Trim( false );

    if( expression.IsEmpty() )
    {
        m_queryStatusLabel->SetLabel( _( "Expression is empty" ) );
        return;
    }

    wxString normalizedExpression = normalizeQueryFieldAliases( expression, m_propertyRows );

    if( queryUsesUnsupportedPairwiseSyntax( normalizedExpression ) )
    {
        m_queryStatusLabel->SetLabel( _( "B. expressions are not supported." ) );
        return;
    }

    PCBEXPR_COMPILER compiler( new PCBEXPR_UNIT_RESOLVER() );
    PCBEXPR_UCODE    ucode;
    PCBEXPR_CONTEXT  preflightContext( 0, F_Cu );
    wxString         errors;

    compiler.SetErrorCallback(
            [&]( const wxString& aMessage, int aOffset )
            {
                errors += aMessage + wxT( "\n" );
            } );

    bool ok = compiler.Compile( normalizedExpression.ToUTF8().data(), &ucode, &preflightContext );

    if( ok )
        m_queryStatusLabel->SetLabel( _( "Syntax OK" ) );
    else
        m_queryStatusLabel->SetLabel( _( "Syntax error: " ) + errors );
}


void DIALOG_FIND_BY_PROPERTIES::onRecentQuerySelected( wxCommandEvent& event )
{
    int sel = m_recentQueries->GetSelection();

    if( sel != wxNOT_FOUND )
        m_queryEditor->SetText( m_recentQueries->GetString( sel ) );
}


void DIALOG_FIND_BY_PROPERTIES::loadRecentQueries()
{
    m_recentQueries->Clear();

    PROJECT_FILE& prj = m_frame->Prj().GetProjectFile();

    for( const wxString& query : prj.m_FindByPropertiesQueries )
        m_recentQueries->Append( query );
}


void DIALOG_FIND_BY_PROPERTIES::saveRecentQuery( const wxString& aQuery )
{
    if( aQuery.IsEmpty() )
        return;

    PROJECT_FILE& prj = m_frame->Prj().GetProjectFile();
    auto&         queries = prj.m_FindByPropertiesQueries;

    // Remove duplicate if exists
    queries.erase( std::remove( queries.begin(), queries.end(), aQuery ), queries.end() );

    // Insert at front
    queries.insert( queries.begin(), aQuery );

    // Cap at 10
    if( queries.size() > 10 )
        queries.resize( 10 );

    loadRecentQueries();
    m_recentQueries->SetSelection( 0 );
}


void DIALOG_FIND_BY_PROPERTIES::OnCloseButtonClick( wxCommandEvent& event )
{
    Show( false );
}


void DIALOG_FIND_BY_PROPERTIES::onNotebookPageChanged( wxNotebookEvent& event )
{
    if( event.GetSelection() == 0 )
        rebuildPropertyGrid();

    event.Skip();
}


void DIALOG_FIND_BY_PROPERTIES::onScintillaCharAdded( wxStyledTextEvent& aEvent )
{
    int pos = m_queryEditor->GetCurrentPos();

    if( pos < 2 )
        return;

    wxString prev2 = m_queryEditor->GetTextRange( pos - 2, pos );

    if( prev2 == wxT( "A." ) )
    {
        PROPERTY_MANAGER&  propMgr = PROPERTY_MANAGER::Instance();
        wxArrayString      tokens;
        std::set<wxString> seen;

        // Gather property names from known board item types
        std::vector<TYPE_ID> types = { TYPE_HASH( PCB_TRACK ), TYPE_HASH( PCB_VIA ),   TYPE_HASH( PCB_ARC ),
                                       TYPE_HASH( PAD ),       TYPE_HASH( FOOTPRINT ), TYPE_HASH( ZONE ),
                                       TYPE_HASH( PCB_SHAPE ), TYPE_HASH( PCB_TEXT ) };

        for( TYPE_ID type : types )
        {
            for( PROPERTY_BASE* prop : propMgr.GetProperties( type ) )
            {
                wxString name = prop->Name();

                if( !isVisibleInFindByProperties( name, prop ) )
                    continue;

                name.Replace( wxT( " " ), wxT( "_" ) );

                if( seen.insert( name ).second )
                    tokens.Add( name );
            }
        }

        for( const wxString& fieldName : getQueryableFootprintFieldNames( m_propertyRows ) )
        {
            wxString exprName = fieldName;
            exprName.Replace( wxT( " " ), wxT( "_" ) );

            if( seen.insert( exprName ).second )
                tokens.Add( exprName );
        }

        tokens.Sort();
        m_scintillaTricks->DoAutocomplete( wxEmptyString, tokens );
    }
}
