/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022-2024 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Wayne Stambaugh <stambaughw@gmail.com>
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <wx/log.h>
#include <base_units.h>
#include <build_version.h>
#include <sch_shape.h>
#include <lib_symbol.h>
#include <sch_textbox.h>
#include <locale_io.h>
#include <macros.h>
#include <richio.h>
#include "sch_io_kicad_sexpr_lib_cache.h"
#include "sch_io_kicad_sexpr_common.h"
#include "sch_io_kicad_sexpr_parser.h"
#include <string_utils.h>
#include <trace_helpers.h>


SCH_IO_KICAD_SEXPR_LIB_CACHE::SCH_IO_KICAD_SEXPR_LIB_CACHE( const wxString& aFullPathAndFileName ) :
    SCH_IO_LIB_CACHE( aFullPathAndFileName )
{
    m_fileFormatVersionAtLoad = 0;
}


SCH_IO_KICAD_SEXPR_LIB_CACHE::~SCH_IO_KICAD_SEXPR_LIB_CACHE()
{
}


void SCH_IO_KICAD_SEXPR_LIB_CACHE::Load()
{
    if( !m_libFileName.FileExists() )
    {
        THROW_IO_ERROR( wxString::Format( _( "Library file '%s' not found." ),
                                          m_libFileName.GetFullPath() ) );
    }

    wxCHECK_RET( m_libFileName.IsAbsolute(),
                 wxString::Format( "Cannot use relative file paths in sexpr plugin to "
                                   "open library '%s'.", m_libFileName.GetFullPath() ) );

    // The current locale must use period as the decimal point.
    // Yes, we did this earlier, but it's sadly not thread-safe.
    LOCALE_IO toggle;

    wxLogTrace( traceSchLegacyPlugin, "Loading sexpr symbol library file '%s'",
                m_libFileName.GetFullPath() );

    FILE_LINE_READER reader( m_libFileName.GetFullPath() );

    SCH_IO_KICAD_SEXPR_PARSER parser( &reader );

    parser.ParseLib( m_symbols );
    IncrementModifyHash();

    // Remember the file modification time of library file when the cache snapshot was made,
    // so that in a networked environment we will reload the cache as needed.
    m_fileModTime = GetLibModificationTime();
    SetFileFormatVersionAtLoad( parser.GetParsedRequiredVersion() );
}


void SCH_IO_KICAD_SEXPR_LIB_CACHE::Save( const std::optional<bool>& aOpt )
{
    if( !m_isModified )
        return;

    LOCALE_IO   toggle;     // toggles on, then off, the C locale.

    // Write through symlinks, don't replace them.
    wxFileName fn = GetRealFile();

    auto formatter = std::make_unique<PRETTIFIED_FILE_OUTPUTFORMATTER>( fn.GetFullPath() );

    formatter->Print( 0, "(kicad_symbol_lib (version %d) (generator \"kicad_symbol_editor\") "
                      "(generator_version \"%s\")\n",
                      SEXPR_SYMBOL_LIB_FILE_VERSION, GetMajorMinorVersion().c_str().AsChar() );

    std::vector<LIB_SYMBOL*> orderedSymbols;

    for( const auto& [ name, symbol ] : m_symbols )
    {
        if( symbol )
            orderedSymbols.push_back( symbol );
    }

    // Library must be ordered by inheritance depth.
    std::sort( orderedSymbols.begin(), orderedSymbols.end(),
               []( const LIB_SYMBOL* aLhs, const LIB_SYMBOL* aRhs )
               {
                   unsigned int lhDepth = aLhs->GetInheritanceDepth();
                   unsigned int rhDepth = aRhs->GetInheritanceDepth();

                   if( lhDepth == rhDepth )
                       return aLhs->GetName() < aRhs->GetName();

                   return lhDepth < rhDepth;
               } );

    for( LIB_SYMBOL* symbol : orderedSymbols )
        SaveSymbol( symbol, *formatter.get(), 1 );

    formatter->Print( 0, ")\n" );

    formatter.reset();

    m_fileModTime = fn.GetModificationTime();
    m_isModified = false;
}


void SCH_IO_KICAD_SEXPR_LIB_CACHE::SaveSymbol( LIB_SYMBOL* aSymbol, OUTPUTFORMATTER& aFormatter,
                                               int aNestLevel, const wxString& aLibName )
{
    wxCHECK_RET( aSymbol, "Invalid LIB_SYMBOL pointer." );

    // The current locale must use period as the decimal point.
    wxCHECK2( wxLocale::GetInfo( wxLOCALE_DECIMAL_POINT, wxLOCALE_CAT_NUMBER ) == ".",
              LOCALE_IO toggle );

    int nextFreeFieldId = MANDATORY_FIELDS;
    std::vector<SCH_FIELD*> fields;
    std::string name = aFormatter.Quotew( aSymbol->GetLibId().GetLibItemName().wx_str() );
    std::string unitName = aSymbol->GetLibId().GetLibItemName();

    if( !aLibName.IsEmpty() )
    {
        name = aFormatter.Quotew( aLibName );

        LIB_ID unitId;

        wxCHECK2( unitId.Parse( aLibName ) < 0, /* do nothing */ );

        unitName = unitId.GetLibItemName();
    }

    if( aSymbol->IsRoot() )
    {
        aFormatter.Print( aNestLevel, "(symbol %s", name.c_str() );

        if( aSymbol->IsPower() )
            aFormatter.Print( 0, " (power)" );

        // TODO: add uuid token here.

        // TODO: add anchor position token here.

        if( !aSymbol->GetShowPinNumbers() )
            aFormatter.Print( 0, " (pin_numbers hide)" );

        if( aSymbol->GetPinNameOffset() != schIUScale.MilsToIU( DEFAULT_PIN_NAME_OFFSET )
          || !aSymbol->GetShowPinNames() )
        {
            aFormatter.Print( 0, " (pin_names" );

            if( aSymbol->GetPinNameOffset() != schIUScale.MilsToIU( DEFAULT_PIN_NAME_OFFSET ) )
            {
                aFormatter.Print( 0, " (offset %s)",
                                  EDA_UNIT_UTILS::FormatInternalUnits( schIUScale,
                                                                       aSymbol->GetPinNameOffset() ).c_str() );
            }

            if( !aSymbol->GetShowPinNames() )
                aFormatter.Print( 0, " hide" );

            aFormatter.Print( 0, ")" );
        }

        aFormatter.Print( 0, " (exclude_from_sim %s)",
                          ( aSymbol->GetExcludedFromSim() ) ? "yes" : "no" );
        aFormatter.Print( 0, " (in_bom %s)", ( aSymbol->GetExcludedFromBOM() ) ? "no" : "yes" );
        aFormatter.Print( 0, " (on_board %s)", ( aSymbol->GetExcludedFromBoard() ) ? "no" : "yes" );

        // TODO: add atomic token here.

        // TODO: add required token here."

        aFormatter.Print( 0, "\n" );

        aSymbol->GetFields( fields );

        for( SCH_FIELD* field : fields )
            saveField( field, aFormatter, aNestLevel + 1 );

        nextFreeFieldId = aSymbol->GetNextAvailableFieldId();

        // @todo At some point in the future the lock status (all units interchangeable) should
        // be set deterministically.  For now a custom lock property is used to preserve the
        // locked flag state.
        if( aSymbol->UnitsLocked() )
        {
            SCH_FIELD locked( nullptr, nextFreeFieldId, "ki_locked" );
            saveField( &locked, aFormatter, aNestLevel + 1 );
            nextFreeFieldId += 1;
        }

        saveDcmInfoAsFields( aSymbol, aFormatter, nextFreeFieldId, aNestLevel );

        // Save the draw items grouped by units.
        std::vector<LIB_SYMBOL_UNIT> units = aSymbol->GetUnitDrawItems();
        std::sort( units.begin(), units.end(),
                   []( const LIB_SYMBOL_UNIT& a, const LIB_SYMBOL_UNIT& b )
                   {
                        if( a.m_unit == b.m_unit )
                            return a.m_bodyStyle < b.m_bodyStyle;

                        return a.m_unit < b.m_unit;
                   } );

        for( const LIB_SYMBOL_UNIT& unit : units )
        {
            // Add quotes and escape chars like ") to the UTF8 unitName string
            name = aFormatter.Quotes( unitName );
            name.pop_back();    // Remove last char: the quote ending the string.

            aFormatter.Print( aNestLevel + 1, "(symbol %s_%d_%d\"\n",
                              name.c_str(),
                              unit.m_unit,
                              unit.m_bodyStyle );

            // if the unit has a display name, write that
            if( aSymbol->HasUnitDisplayName( unit.m_unit ) )
            {
                name = aSymbol->GetUnitDisplayName( unit.m_unit );
                aFormatter.Print( aNestLevel + 2, "(unit_name %s)\n",
                                  aFormatter.Quotes( name ).c_str() );
            }
            // Enforce item ordering
            auto cmp =
                    []( const SCH_ITEM* a, const SCH_ITEM* b )
                    {
                        return *a < *b;
                    };

            std::multiset<SCH_ITEM*, decltype( cmp )> save_map( cmp );

            for( SCH_ITEM* item : unit.m_items )
                save_map.insert( item );

            for( SCH_ITEM* item : save_map )
                saveSymbolDrawItem( item, aFormatter, aNestLevel + 2 );

            aFormatter.Print( aNestLevel + 1, ")\n" );
        }
    }
    else
    {
        std::shared_ptr<LIB_SYMBOL> parent = aSymbol->GetParent().lock();

        wxASSERT( parent );

        aFormatter.Print( aNestLevel, "(symbol %s (extends %s)\n",
                          name.c_str(),
                          aFormatter.Quotew( parent->GetName() ).c_str() );

        aSymbol->GetFields( fields );

        for( SCH_FIELD* field : fields )
            saveField( field, aFormatter, aNestLevel + 1 );

        nextFreeFieldId = aSymbol->GetNextAvailableFieldId();

        saveDcmInfoAsFields( aSymbol, aFormatter, nextFreeFieldId, aNestLevel );
    }

    aFormatter.Print( aNestLevel, ")\n" );
}


void SCH_IO_KICAD_SEXPR_LIB_CACHE::saveDcmInfoAsFields( LIB_SYMBOL* aSymbol,
                                                        OUTPUTFORMATTER& aFormatter,
                                                        int& aNextFreeFieldId, int aNestLevel )
{
    wxCHECK_RET( aSymbol, "Invalid LIB_SYMBOL pointer." );

    if( !aSymbol->GetKeyWords().IsEmpty() )
    {
        SCH_FIELD keywords( nullptr, aNextFreeFieldId, wxString( "ki_keywords" ) );
        keywords.SetVisible( false );
        keywords.SetText( aSymbol->GetKeyWords() );
        saveField( &keywords, aFormatter, aNestLevel + 1 );
        aNextFreeFieldId += 1;
    }

    wxArrayString fpFilters = aSymbol->GetFPFilters();

    if( !fpFilters.IsEmpty() )
    {
        wxString tmp;

        for( const wxString& filter : fpFilters )
        {
            // Spaces are not handled in fp filter names so escape spaces if any
            wxString curr_filter = EscapeString( filter, ESCAPE_CONTEXT::CTX_NO_SPACE );

            if( tmp.IsEmpty() )
                tmp = curr_filter;
            else
                tmp += " " + curr_filter;
        }

        SCH_FIELD description( nullptr, aNextFreeFieldId, wxString( "ki_fp_filters" ) );
        description.SetVisible( false );
        description.SetText( tmp );
        saveField( &description, aFormatter, aNestLevel + 1 );
        aNextFreeFieldId += 1;
    }
}


void SCH_IO_KICAD_SEXPR_LIB_CACHE::saveSymbolDrawItem( SCH_ITEM* aItem, OUTPUTFORMATTER& aFormatter,
                                                       int aNestLevel )
{
    wxCHECK_RET( aItem, "Invalid SCH_ITEM pointer." );

    switch( aItem->Type() )
    {
    case SCH_SHAPE_T:
    {
        SCH_SHAPE*    shape = static_cast<SCH_SHAPE*>( aItem );
        STROKE_PARAMS stroke = shape->GetStroke();
        FILL_T        fillMode = shape->GetFillMode();
        COLOR4D       fillColor = shape->GetFillColor();
        bool          isPrivate = shape->IsPrivate();

        switch( shape->GetShape() )
        {
        case SHAPE_T::ARC:
            formatArc( &aFormatter, aNestLevel, shape, isPrivate, stroke, fillMode, fillColor, true );
            break;

        case SHAPE_T::CIRCLE:
            formatCircle( &aFormatter, aNestLevel, shape, isPrivate, stroke, fillMode, fillColor, true );
            break;

        case SHAPE_T::RECTANGLE:
            formatRect( &aFormatter, aNestLevel, shape, isPrivate, stroke, fillMode, fillColor, true );
            break;

        case SHAPE_T::BEZIER:
            formatBezier(&aFormatter, aNestLevel, shape, isPrivate, stroke, fillMode, fillColor, true );
            break;

        case SHAPE_T::POLY:
            formatPoly( &aFormatter, aNestLevel, shape, isPrivate, stroke, fillMode, fillColor, true );
            break;

        default:
            UNIMPLEMENTED_FOR( shape->SHAPE_T_asString() );
        }

        break;
    }

    case SCH_PIN_T:
        savePin( static_cast<SCH_PIN*>( aItem ), aFormatter, aNestLevel );
        break;

    case SCH_TEXT_T:
        saveText( static_cast<SCH_TEXT*>( aItem ), aFormatter, aNestLevel );
        break;

    case SCH_TEXTBOX_T:
        saveTextBox( static_cast<SCH_TEXTBOX*>( aItem ), aFormatter, aNestLevel );
        break;

    default:
        UNIMPLEMENTED_FOR( aItem->GetClass() );
    }
}


void SCH_IO_KICAD_SEXPR_LIB_CACHE::saveField( SCH_FIELD* aField, OUTPUTFORMATTER& aFormatter,
                                              int aNestLevel )
{
    wxCHECK_RET( aField && aField->Type() == SCH_FIELD_T, "Invalid SCH_FIELD object." );

    wxString fieldName = aField->GetName();

    if( aField->IsMandatory() )
        fieldName = GetCanonicalFieldName( aField->GetId() );

    aFormatter.Print( aNestLevel, "(property %s %s (at %s %s %g)",
                      aFormatter.Quotew( fieldName ).c_str(),
                      aFormatter.Quotew( aField->GetText() ).c_str(),
                      EDA_UNIT_UTILS::FormatInternalUnits( schIUScale,
                                                           aField->GetPosition().x ).c_str(),
                      EDA_UNIT_UTILS::FormatInternalUnits( schIUScale,
                                                           -aField->GetPosition().y ).c_str(),
                      aField->GetTextAngle().AsDegrees() );

    if( aField->IsNameShown() )
        aFormatter.Print( 0, " (show_name)" );

    if( !aField->CanAutoplace() )
        aFormatter.Print( 0, " (do_not_autoplace)" );

    aFormatter.Print( 0, "\n" );
    aField->Format( &aFormatter, aNestLevel, 0 );
    aFormatter.Print( aNestLevel, ")\n" );
}


void SCH_IO_KICAD_SEXPR_LIB_CACHE::savePin( SCH_PIN* aPin, OUTPUTFORMATTER& aFormatter,
                                            int aNestLevel )
{
    wxCHECK_RET( aPin && aPin->Type() == SCH_PIN_T, "Invalid SCH_PIN object." );

    aPin->ClearFlags( IS_CHANGED );

    aFormatter.Print( aNestLevel, "(pin %s %s (at %s %s %s) (length %s)",
                      getPinElectricalTypeToken( aPin->GetType() ),
                      getPinShapeToken( aPin->GetShape() ),
                      EDA_UNIT_UTILS::FormatInternalUnits( schIUScale,
                                                           aPin->GetPosition().x ).c_str(),
                      EDA_UNIT_UTILS::FormatInternalUnits( schIUScale,
                                                           -aPin->GetPosition().y ).c_str(),
                      EDA_UNIT_UTILS::FormatAngle( getPinAngle( aPin->GetOrientation() ) ).c_str(),
                      EDA_UNIT_UTILS::FormatInternalUnits( schIUScale,
                                                           aPin->GetLength() ).c_str() );

    if( !aPin->IsVisible() )
        aFormatter.Print( 0, " hide\n" );
    else
        aFormatter.Print( 0, "\n" );

    // This follows the EDA_TEXT effects formatting for future expansion.
    aFormatter.Print( aNestLevel + 1, "(name %s (effects (font (size %s %s))))\n",
                      aFormatter.Quotew( aPin->GetName() ).c_str(),
                      EDA_UNIT_UTILS::FormatInternalUnits( schIUScale,
                                                           aPin->GetNameTextSize() ).c_str(),
                      EDA_UNIT_UTILS::FormatInternalUnits( schIUScale,
                                                           aPin->GetNameTextSize() ).c_str() );

    aFormatter.Print( aNestLevel + 1, "(number %s (effects (font (size %s %s))))\n",
                      aFormatter.Quotew( aPin->GetNumber() ).c_str(),
                      EDA_UNIT_UTILS::FormatInternalUnits( schIUScale,
                                                           aPin->GetNumberTextSize() ).c_str(),
                      EDA_UNIT_UTILS::FormatInternalUnits( schIUScale,
                                                           aPin->GetNumberTextSize() ).c_str() );


    for( const std::pair<const wxString, SCH_PIN::ALT>& alt : aPin->GetAlternates() )
    {
        aFormatter.Print( aNestLevel + 1, "(alternate %s %s %s)\n",
                          aFormatter.Quotew( alt.second.m_Name ).c_str(),
                          getPinElectricalTypeToken( alt.second.m_Type ),
                          getPinShapeToken( alt.second.m_Shape ) );
    }

    aFormatter.Print( aNestLevel, ")\n" );
}


void SCH_IO_KICAD_SEXPR_LIB_CACHE::saveText( SCH_TEXT* aText, OUTPUTFORMATTER& aFormatter,
                                             int aNestLevel )
{
    wxCHECK_RET( aText && aText->Type() == SCH_TEXT_T, "Invalid SCH_TEXT object." );

    aFormatter.Print( aNestLevel, "(text%s %s (at %s %s %g)\n",
                      aText->IsPrivate() ? " private" : "",
                      aFormatter.Quotew( aText->GetText() ).c_str(),
                      EDA_UNIT_UTILS::FormatInternalUnits( schIUScale,
                                                           aText->GetPosition().x ).c_str(),
                      EDA_UNIT_UTILS::FormatInternalUnits( schIUScale,
                                                           -aText->GetPosition().y ).c_str(),
                      (double) aText->GetTextAngle().AsTenthsOfADegree() );

    aText->EDA_TEXT::Format( &aFormatter, aNestLevel, 0 );
    aFormatter.Print( aNestLevel, ")\n" );
}


void SCH_IO_KICAD_SEXPR_LIB_CACHE::saveTextBox( SCH_TEXTBOX* aTextBox, OUTPUTFORMATTER& aFormatter,
                                                int aNestLevel )
{
    wxCHECK_RET( aTextBox && aTextBox->Type() == SCH_TEXTBOX_T, "Invalid SCH_TEXTBOX object." );

    aFormatter.Print( aNestLevel, "(text_box%s %s\n",
                      aTextBox->IsPrivate() ? " private" : "",
                      aFormatter.Quotew( aTextBox->GetText() ).c_str() );

    VECTOR2I pos = aTextBox->GetStart();
    VECTOR2I size = aTextBox->GetEnd() - pos;

    aFormatter.Print( aNestLevel + 1, "(at %s %s %s) (size %s %s) (margins %s %s %s %s)\n",
                      EDA_UNIT_UTILS::FormatInternalUnits( schIUScale, pos.x ).c_str(),
                      EDA_UNIT_UTILS::FormatInternalUnits( schIUScale, -pos.y ).c_str(),
                      EDA_UNIT_UTILS::FormatAngle( aTextBox->GetTextAngle() ).c_str(),
                      EDA_UNIT_UTILS::FormatInternalUnits( schIUScale, size.x ).c_str(),
                      EDA_UNIT_UTILS::FormatInternalUnits( schIUScale, -size.y ).c_str(),
                      EDA_UNIT_UTILS::FormatInternalUnits( schIUScale, aTextBox->GetMarginLeft() ).c_str(),
                      EDA_UNIT_UTILS::FormatInternalUnits( schIUScale, aTextBox->GetMarginTop() ).c_str(),
                      EDA_UNIT_UTILS::FormatInternalUnits( schIUScale, aTextBox->GetMarginRight() ).c_str(),
                      EDA_UNIT_UTILS::FormatInternalUnits( schIUScale, aTextBox->GetMarginBottom() ).c_str() );

    aTextBox->GetStroke().Format( &aFormatter, schIUScale, aNestLevel + 1 );
    aFormatter.Print( 0, "\n" );

    formatFill( &aFormatter, aNestLevel + 1, aTextBox->GetFillMode(), aTextBox->GetFillColor() );
    aFormatter.Print( 0, "\n" );

    aTextBox->EDA_TEXT::Format( &aFormatter, aNestLevel, 0 );
    aFormatter.Print( aNestLevel, ")\n" );
}


void SCH_IO_KICAD_SEXPR_LIB_CACHE::DeleteSymbol( const wxString& aSymbolName )
{
    LIB_SYMBOL_MAP::iterator it = m_symbols.find( aSymbolName );

    if( it == m_symbols.end() )
        THROW_IO_ERROR( wxString::Format( _( "library %s does not contain a symbol named %s" ),
                                          m_libFileName.GetFullName(), aSymbolName ) );

    LIB_SYMBOL* symbol = it->second;

    if( symbol->IsRoot() )
    {
        LIB_SYMBOL* rootSymbol = symbol;

        // Remove the root symbol and all its children.
        m_symbols.erase( it );

        LIB_SYMBOL_MAP::iterator it1 = m_symbols.begin();

        while( it1 != m_symbols.end() )
        {
            if( it1->second->IsAlias()
              && it1->second->GetParent().lock() == rootSymbol->SharedPtr() )
            {
                delete it1->second;
                it1 = m_symbols.erase( it1 );
            }
            else
            {
                it1++;
            }
        }

        delete rootSymbol;
    }
    else
    {
        // Just remove the alias.
        m_symbols.erase( it );
        delete symbol;
    }

    IncrementModifyHash();
    m_isModified = true;
}
