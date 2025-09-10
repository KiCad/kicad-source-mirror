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

#include "sch_tool_utils.h"

#include <sch_text.h>
#include <sch_field.h>
#include <sch_pin.h>
#include <sch_reference_list.h>
#include <sch_symbol.h>
#include <sch_table.h>
#include <sch_tablecell.h>
#include <sch_textbox.h>
#include <schematic.h>

#include <wx/arrstr.h>

wxString GetSchItemAsText( const SCH_ITEM& aItem )
{
    switch( aItem.Type() )
    {
    case SCH_TEXT_T:
    case SCH_LABEL_T:
    case SCH_HIER_LABEL_T:
    case SCH_GLOBAL_LABEL_T:
    case SCH_DIRECTIVE_LABEL_T:
    case SCH_SHEET_PIN_T:
    {
        const SCH_TEXT& text = static_cast<const SCH_TEXT&>( aItem );
        return text.GetShownText( true );
    }
    case SCH_FIELD_T:
    {
        // Goes via EDA_TEXT
        const SCH_FIELD& field = static_cast<const SCH_FIELD&>( aItem );
        return field.GetShownText( true );
    }
    case SCH_TEXTBOX_T:
    case SCH_TABLECELL_T:
    {
        // Also EDA_TEXT
        const SCH_TEXTBOX& textbox = static_cast<const SCH_TEXTBOX&>( aItem );
        return textbox.GetShownText( true );
    }
    case SCH_PIN_T:
    {
        // This is a choice - probably the name makes more sense than the number
        // (or should it be name/number?)
        const SCH_PIN& pin = static_cast<const SCH_PIN&>( aItem );
        return pin.GetShownName();
    }
    case SCH_TABLE_T:
    {
        // A simple tabbed list of the cells seems like a place to start here
        const SCH_TABLE& table = static_cast<const SCH_TABLE&>( aItem );
        wxString         s;

        for( int row = 0; row < table.GetRowCount(); ++row )
        {
            for( int col = 0; col < table.GetColCount(); ++col )
            {
                const SCH_TABLECELL* cell = table.GetCell( row, col );
                s << cell->GetShownText( true );

                if( col < table.GetColCount() - 1 )
                {
                    s << '\t';
                }
            }

            if( row < table.GetRowCount() - 1 )
            {
                s << '\n';
            }
        }
        return s;
    }
    default:
    {
        break;
    }
    }

    return wxEmptyString;
};


wxString GetSelectedItemsAsText( const SELECTION& aSel )
{
    wxArrayString itemTexts;

    for( EDA_ITEM* item : aSel )
    {
        if( item->IsSCH_ITEM() )
        {
            const SCH_ITEM& schItem = static_cast<const SCH_ITEM&>( *item );
            wxString        itemText = GetSchItemAsText( schItem );

            itemText.Trim( false ).Trim( true );

            if( !itemText.IsEmpty() )
            {
                itemTexts.Add( std::move( itemText ) );
            }
        }
    }

    return wxJoin( itemTexts, '\n', '\0' );
}


std::set<int> GetUnplacedUnitsForSymbol( const SCH_SYMBOL& aSym )
{
    SCHEMATIC const* schematic = aSym.Schematic();

    if( !schematic )
        return {};

    const wxString symRefDes = aSym.GetRef( &schematic->CurrentSheet(), false );

    // Get a list of all references in the schematic
    SCH_SHEET_LIST     hierarchy = schematic->Hierarchy();
    SCH_REFERENCE_LIST existingRefs;
    hierarchy.GetSymbols( existingRefs );

    std::set<int> missingUnits;

    for( int unit = 1; unit <= aSym.GetUnitCount(); ++unit )
        missingUnits.insert( unit );

    for( const SCH_REFERENCE& ref : existingRefs )
    {
        if( symRefDes == ref.GetRef() )
            missingUnits.erase( ref.GetUnit() );
    }

    return missingUnits;
}


std::optional<SCH_REFERENCE> FindSymbolByRefAndUnit( const SCHEMATIC& aSchematic,
                                                     const wxString& aRef, int aUnit )
{
    SCH_REFERENCE_LIST refs;
    aSchematic.Hierarchy().GetSymbols( refs );

    for( const SCH_REFERENCE& ref : refs )
    {
        if( ref.GetRef() == aRef && ref.GetUnit() == aUnit )
        {
            return ref;
        }
    }

    return std::nullopt;
}


std::vector<SCH_SYMBOL*> GetSameSymbolMultiUnitSelection( const SELECTION& aSel )
{
    std::vector<SCH_SYMBOL*> result;

    if( aSel.GetSize() < 2 || !aSel.OnlyContains( { SCH_SYMBOL_T } ) )
        return result;

    wxString rootRef;
    LIB_ID   rootLibId;
    bool     haveRootLibId = false;
    size_t   rootPinCount = 0;
    bool     haveRootPinCount = false;

    // Preserve selection order for cyclical swaps A->B->C
    std::vector<EDA_ITEM*> itemsInOrder = aSel.GetItemsSortedBySelectionOrder();

    for( EDA_ITEM* it : itemsInOrder )
    {
        SCH_SYMBOL* sym = dynamic_cast<SCH_SYMBOL*>( it );

        if( !sym || !sym->GetLibSymbolRef() || sym->GetLibSymbolRef()->GetUnitCount() < 2 )
            return {};

        const SCH_SHEET_PATH& sheet = sym->Schematic()->CurrentSheet();

        // Get unit-less reference
        wxString ref = sym->GetRef( &sheet, false );

        if( rootRef.IsEmpty() )
            rootRef = ref;

        if( ref != rootRef )
            return {};

        // Make sure the user isn't selecting units that are misreferenced such that
        // they have U1A and U1B that are actually from different library symbols.
        const LIB_ID& libId = sym->GetLibId();

        if( !haveRootLibId )
        {
            rootLibId = libId;
            haveRootLibId = true;
        }

        if( libId != rootLibId )
            return {};

        // Ensure same pin count across selected units
        size_t pinCount = sym->GetPins( &sheet ).size();

        if( !haveRootPinCount )
        {
            rootPinCount = pinCount;
            haveRootPinCount = true;
        }

        if( pinCount != rootPinCount )
            return {};

        result.push_back( sym );
    }

    if( result.size() < 2 )
        return {};

    return result;
}
