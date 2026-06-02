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

#ifndef TEST_DIPTRACE_SCH_IMPORT_FIXTURE_H
#define TEST_DIPTRACE_SCH_IMPORT_FIXTURE_H

/**
 * @file test_diptrace_sch_import_fixture.h
 * Shared fixture, reporter and helpers for the DipTrace schematic import test suite.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <eeschema/sch_io/diptrace/sch_io_diptrace.h>
#include <eeschema/sch_io/diptrace/diptrace_sch_parser.h>

#include <connection_graph.h>
#include <eeschema_helpers.h>
#include <sch_connection.h>
#include <sch_label.h>
#include <sch_line.h>
#include <sch_pin.h>
#include <sch_screen.h>
#include <sch_shape.h>
#include <sch_sheet_path.h>
#include <sch_symbol.h>
#include <schematic.h>
#include <settings/settings_manager.h>

#include <lib_symbol.h>

#include <climits>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <set>
#include <sstream>
#include <vector>

#include <reporter.h>

#include <wx/filefn.h>
#include <wx/ffile.h>
#include <wx/xml/xml.h>


/// Reporter that records the severity of every message so a test can assert that an import ran
/// without warnings or errors.
class DIPTRACE_COUNTING_REPORTER : public REPORTER
{
public:
    REPORTER& Report( const wxString& aText, SEVERITY aSeverity = RPT_SEVERITY_UNDEFINED ) override
    {
        m_messages.emplace_back( aSeverity, aText );
        return *this;
    }

    bool HasMessage() const override { return !m_messages.empty(); }

    int CountOfSeverity( int aSeverityMask ) const
    {
        int n = 0;

        for( const auto& [sev, text] : m_messages )
        {
            if( sev & aSeverityMask )
                n++;
        }

        return n;
    }

    std::string MessagesOfSeverity( int aSeverityMask ) const
    {
        std::ostringstream out;

        for( const auto& [sev, text] : m_messages )
        {
            if( !( sev & aSeverityMask ) )
                continue;

            if( out.tellp() > 0 )
                out << "; ";

            out << text.ToStdString();
        }

        return out.str();
    }

    void Clear() override { m_messages.clear(); }

    std::vector<std::pair<SEVERITY, wxString>> m_messages;
};


struct DIPTRACE_SCH_IMPORT_FIXTURE
{
    DIPTRACE_SCH_IMPORT_FIXTURE() :
            m_schematic( new SCHEMATIC( nullptr ) )
    {
        m_manager.LoadProject( "" );
        m_schematic->SetProject( &m_manager.Prj() );
        m_schematic->CurrentSheet().clear();
        m_schematic->CurrentSheet().push_back( &m_schematic->Root() );
    }

    SCH_IO_DIPTRACE            m_plugin;
    std::unique_ptr<SCHEMATIC> m_schematic;
    SETTINGS_MANAGER           m_manager;
    SCH_SHEET*                 m_loadedRoot = nullptr;
    DIPTRACE_COUNTING_REPORTER m_reporter;

    std::string GetTestDataDir() { return KI_TEST::GetEeschemaTestDataDir() + "plugins/diptrace/"; }

    std::string GetViewerExamplesDir()
    {
        const char* examplesEnv = std::getenv( "DIPTRACE_VIEWER_EXAMPLES_DIR" );

        return examplesEnv && *examplesEnv ? examplesEnv : "/home/seth/Downloads/DipTrace Viewer/Examples";
    }

    int CountItemsOfType( KICAD_T aType )
    {
        std::set<SCH_SCREEN*> seenScreens;
        int                   count = 0;

        for( const SCH_SHEET_PATH& sheetPath : m_schematic->BuildUnorderedSheetList() )
        {
            SCH_SCREEN* screen = sheetPath.LastScreen();

            if( !screen || !seenScreens.insert( screen ).second )
                continue;

            for( SCH_ITEM* item : screen->Items() )
            {
                if( item->Type() == aType )
                    count++;
            }
        }

        return count;
    }

    int CountImportedScreens()
    {
        std::set<SCH_SCREEN*> seenScreens;

        for( const SCH_SHEET_PATH& sheetPath : m_schematic->BuildUnorderedSheetList() )
        {
            SCH_SCREEN* screen = sheetPath.LastScreen();

            if( !screen || !seenScreens.insert( screen ).second )
                continue;
        }

        return static_cast<int>( seenScreens.size() );
    }

    // Symbol references are resolved per sheet path through instance data, so iterate the
    // hierarchy and match each symbol's path-resolved reference rather than its raw field text.
    int MaxPinCountForRefdes( const wxString& aRefdes )
    {
        int maxPins = -1;

        for( const SCH_SHEET_PATH& sheetPath : m_schematic->BuildUnorderedSheetList() )
        {
            SCH_SCREEN* screen = sheetPath.LastScreen();

            if( !screen )
                continue;

            for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
            {
                SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

                if( symbol->GetRef( &sheetPath, false ) == aRefdes )
                    maxPins = std::max( maxPins, static_cast<int>( symbol->GetPins().size() ) );
            }
        }

        return maxPins;
    }

    wxString GetFootprintForRefdes( const wxString& aRefdes )
    {
        for( const SCH_SHEET_PATH& sheetPath : m_schematic->BuildUnorderedSheetList() )
        {
            SCH_SCREEN* screen = sheetPath.LastScreen();

            if( !screen )
                continue;

            for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
            {
                SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

                if( symbol->GetRef( &sheetPath, false ) == aRefdes )
                {
                    SCH_FIELD* fpField = symbol->GetField( FIELD_T::FOOTPRINT );
                    return fpField ? fpField->GetText() : wxString();
                }
            }
        }

        return wxString();
    }

    /// Return the DipTrace sheet name carrying the symbol with the given reference, or empty.
    wxString GetSheetNameForRefdes( const wxString& aRefdes )
    {
        for( const SCH_SHEET_PATH& sheetPath : m_schematic->BuildUnorderedSheetList() )
        {
            SCH_SCREEN* screen = sheetPath.LastScreen();

            if( !screen )
                continue;

            for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
            {
                SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

                if( symbol->GetRef( &sheetPath, false ) == aRefdes )
                    return sheetPath.Last() ? sheetPath.Last()->GetName() : wxString();
            }
        }

        return wxString();
    }

    /// Count the pin orientations of the first symbol matching the reference designator, read from
    /// its embedded library symbol so the placed orientation is what is inspected.
    void PinOrientationCounts( const wxString& aRefdes, int& aLeft, int& aRight, int& aUp, int& aDown )
    {
        aLeft = aRight = aUp = aDown = 0;

        for( const SCH_SHEET_PATH& sheetPath : m_schematic->BuildUnorderedSheetList() )
        {
            SCH_SCREEN* screen = sheetPath.LastScreen();

            if( !screen )
                continue;

            for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
            {
                SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

                if( symbol->GetRef( &sheetPath, false ) != aRefdes || !symbol->GetLibSymbolRef() )
                    continue;

                for( SCH_PIN* pin : symbol->GetLibSymbolRef()->GetPins() )
                {
                    switch( pin->GetOrientation() )
                    {
                    case PIN_ORIENTATION::PIN_LEFT: aLeft++; break;
                    case PIN_ORIENTATION::PIN_RIGHT: aRight++; break;
                    case PIN_ORIENTATION::PIN_UP: aUp++; break;
                    case PIN_ORIENTATION::PIN_DOWN: aDown++; break;
                    default: break;
                    }
                }

                return;
            }
        }
    }

    /// True when a global label whose text matches exists on the sheet with the given name. A net
    /// can have a port on several sheets, so membership is tested per sheet rather than returning a
    /// single owning sheet.
    bool HasLabelOnSheet( const wxString& aText, const wxString& aSheetName )
    {
        for( const SCH_SHEET_PATH& sheetPath : m_schematic->BuildUnorderedSheetList() )
        {
            if( !sheetPath.Last() || sheetPath.Last()->GetName() != aSheetName )
                continue;

            SCH_SCREEN* screen = sheetPath.LastScreen();

            if( !screen )
                continue;

            for( SCH_ITEM* item : screen->Items().OfType( SCH_GLOBAL_LABEL_T ) )
            {
                SCH_LABEL_BASE* label = static_cast<SCH_LABEL_BASE*>( item );

                if( label->GetText() == aText )
                    return true;
            }
        }

        return false;
    }

    /// True when any global label on the named sheet has text beginning with the given prefix. Used
    /// to assert the importer no longer fabricates internal-net labels ("Net 36" and the like) that
    /// DipTrace itself never draws.
    bool HasLabelStartingWithOnSheet( const wxString& aPrefix, const wxString& aSheetName )
    {
        for( const SCH_SHEET_PATH& sheetPath : m_schematic->BuildUnorderedSheetList() )
        {
            if( !sheetPath.Last() || sheetPath.Last()->GetName() != aSheetName )
                continue;

            SCH_SCREEN* screen = sheetPath.LastScreen();

            if( !screen )
                continue;

            for( SCH_ITEM* item : screen->Items().OfType( SCH_GLOBAL_LABEL_T ) )
            {
                SCH_LABEL_BASE* label = static_cast<SCH_LABEL_BASE*>( item );

                if( label->GetText().StartsWith( aPrefix ) )
                    return true;
            }
        }

        return false;
    }

    /// Count SCH_LINE items on the DipTrace sheet with the given name.
    int CountLinesOnSheet( const wxString& aSheetName )
    {
        int lines = 0;

        for( const SCH_SHEET_PATH& sheetPath : m_schematic->BuildUnorderedSheetList() )
        {
            if( !sheetPath.Last() || sheetPath.Last()->GetName() != aSheetName )
                continue;

            SCH_SCREEN* screen = sheetPath.LastScreen();

            if( !screen )
                continue;

            for( SCH_ITEM* item : screen->Items().OfType( SCH_LINE_T ) )
            {
                static_cast<void>( item );
                lines++;
            }
        }

        return lines;
    }

    /// Distance between the reference and value field positions of the first symbol matching the
    /// reference designator, or -1 when the symbol or a field is missing. Used to confirm the two
    /// fields are not stacked on the symbol origin.
    int RefValueFieldSeparation( const wxString& aRefdes )
    {
        for( const SCH_SHEET_PATH& sheetPath : m_schematic->BuildUnorderedSheetList() )
        {
            SCH_SCREEN* screen = sheetPath.LastScreen();

            if( !screen )
                continue;

            for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
            {
                SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

                if( symbol->GetRef( &sheetPath, false ) != aRefdes )
                    continue;

                SCH_FIELD* refField = symbol->GetField( FIELD_T::REFERENCE );
                SCH_FIELD* valField = symbol->GetField( FIELD_T::VALUE );

                if( !refField || !valField )
                    return -1;

                return ( refField->GetPosition() - valField->GetPosition() ).EuclideanNorm();
            }
        }

        return -1;
    }

    /// The text of a named field on the first symbol matching the reference designator, or an empty
    /// string when the symbol or field is absent.
    wxString GetFieldValueForRefdes( const wxString& aRefdes, const wxString& aFieldName )
    {
        for( const SCH_SHEET_PATH& sheetPath : m_schematic->BuildUnorderedSheetList() )
        {
            SCH_SCREEN* screen = sheetPath.LastScreen();

            if( !screen )
                continue;

            for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
            {
                SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

                if( symbol->GetRef( &sheetPath, false ) != aRefdes )
                    continue;

                const SCH_FIELD* field = symbol->GetField( aFieldName );
                return field ? field->GetText() : wxString();
            }
        }

        return wxString();
    }

    /// The value-field text angle in degrees for the first symbol matching the reference designator,
    /// or -1 when the symbol is absent.
    double ValueFieldAngleDegrees( const wxString& aRefdes )
    {
        for( const SCH_SHEET_PATH& sheetPath : m_schematic->BuildUnorderedSheetList() )
        {
            SCH_SCREEN* screen = sheetPath.LastScreen();

            if( !screen )
                continue;

            for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
            {
                SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

                if( symbol->GetRef( &sheetPath, false ) != aRefdes )
                    continue;

                const SCH_FIELD* field = symbol->GetField( FIELD_T::VALUE );
                return field ? field->GetTextAngle().AsDegrees() : -1;
            }
        }

        return -1;
    }

    /// Whether the embedded library symbol of the first matching reference shows its pin names.
    /// Returns -1 when the symbol is absent, otherwise 0 or 1.
    int ShowsPinNames( const wxString& aRefdes )
    {
        for( const SCH_SHEET_PATH& sheetPath : m_schematic->BuildUnorderedSheetList() )
        {
            SCH_SCREEN* screen = sheetPath.LastScreen();

            if( !screen )
                continue;

            for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
            {
                SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

                if( symbol->GetRef( &sheetPath, false ) != aRefdes || !symbol->GetLibSymbolRef() )
                    continue;

                return symbol->GetLibSymbolRef()->GetShowPinNames() ? 1 : 0;
            }
        }

        return -1;
    }

    /// Count the library-symbol graphic shapes of a given type for the first matching reference.
    int CountSymbolShapes( const wxString& aRefdes, SHAPE_T aShapeType )
    {
        for( const SCH_SHEET_PATH& sheetPath : m_schematic->BuildUnorderedSheetList() )
        {
            SCH_SCREEN* screen = sheetPath.LastScreen();

            if( !screen )
                continue;

            for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
            {
                SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

                if( symbol->GetRef( &sheetPath, false ) != aRefdes || !symbol->GetLibSymbolRef() )
                    continue;

                int count = 0;

                for( const SCH_ITEM& draw : symbol->GetLibSymbolRef()->GetDrawItems() )
                {
                    if( draw.Type() == SCH_SHAPE_T && static_cast<const SCH_SHAPE&>( draw ).GetShape() == aShapeType )
                    {
                        count++;
                    }
                }

                return count;
            }
        }

        return -1;
    }

    int CountSymbolsForRefdesOnSheet( const wxString& aRefdes, const wxString& aSheetName )
    {
        int count = 0;

        for( const SCH_SHEET_PATH& sheetPath : m_schematic->BuildUnorderedSheetList() )
        {
            if( !sheetPath.Last() || sheetPath.Last()->GetName() != aSheetName )
                continue;

            SCH_SCREEN* screen = sheetPath.LastScreen();

            if( !screen )
                continue;

            for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
            {
                SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

                if( symbol->GetRef( &sheetPath, false ) == aRefdes )
                    count++;
            }
        }

        return count;
    }

    static int DipTraceMmToSchIU( double aMm ) { return static_cast<int>( std::lround( aMm * 10000.0 ) ); }

    static VECTOR2I DipTraceXmlSheetPointToKiCad( double aXmm, double aYmm )
    {
        return VECTOR2I( DipTraceMmToSchIU( 210.0 + aXmm ), DipTraceMmToSchIU( 148.5 - aYmm ) );
    }

    int CountSheetGraphicLines( const wxString& aSheetName, const VECTOR2I& aStart, const VECTOR2I& aEnd, int aWidth )
    {
        int count = 0;

        for( const SCH_SHEET_PATH& sheetPath : m_schematic->BuildUnorderedSheetList() )
        {
            if( !sheetPath.Last() || sheetPath.Last()->GetName() != aSheetName )
                continue;

            SCH_SCREEN* screen = sheetPath.LastScreen();

            if( !screen )
                continue;

            for( SCH_ITEM* item : screen->Items().OfType( SCH_SHAPE_T ) )
            {
                SCH_SHAPE* shape = static_cast<SCH_SHAPE*>( item );

                if( shape->GetShape() != SHAPE_T::POLY || shape->GetStroke().GetWidth() != aWidth )
                    continue;

                const KIGFX::COLOR4D color = shape->GetStroke().GetColor();

                if( color.r > 0.01 || color.g > 0.01 || color.b < 0.99 )
                    continue;

                const SHAPE_POLY_SET& poly = shape->GetPolyShape();

                if( poly.TotalVertices() != 2 )
                    continue;

                VECTOR2I p0 = poly.CVertex( 0 );
                VECTOR2I p1 = poly.CVertex( 1 );

                if( ( p0 == aStart && p1 == aEnd ) || ( p0 == aEnd && p1 == aStart ) )
                    count++;
            }
        }

        return count;
    }

    int CountFilledPolysForRefdes( const wxString& aRefdes )
    {
        for( const SCH_SHEET_PATH& sheetPath : m_schematic->BuildUnorderedSheetList() )
        {
            SCH_SCREEN* screen = sheetPath.LastScreen();

            if( !screen )
                continue;

            for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
            {
                SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

                if( symbol->GetRef( &sheetPath, false ) != aRefdes || !symbol->GetLibSymbolRef() )
                    continue;

                int count = 0;

                for( const SCH_ITEM& draw : symbol->GetLibSymbolRef()->GetDrawItems() )
                {
                    if( draw.Type() == SCH_SHAPE_T && static_cast<const SCH_SHAPE&>( draw ).GetShape() == SHAPE_T::POLY
                        && static_cast<const SCH_SHAPE&>( draw ).GetFillMode() != FILL_T::NO_FILL )
                    {
                        count++;
                    }
                }

                return count;
            }
        }

        return -1;
    }

    int CountOpenPolysWithPointCountForRefdes( const wxString& aRefdes, int aPointCount )
    {
        for( const SCH_SHEET_PATH& sheetPath : m_schematic->BuildUnorderedSheetList() )
        {
            SCH_SCREEN* screen = sheetPath.LastScreen();

            if( !screen )
                continue;

            for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
            {
                SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

                if( symbol->GetRef( &sheetPath, false ) != aRefdes || !symbol->GetLibSymbolRef() )
                    continue;

                int count = 0;

                for( const SCH_ITEM& draw : symbol->GetLibSymbolRef()->GetDrawItems() )
                {
                    if( draw.Type() != SCH_SHAPE_T )
                        continue;

                    const SCH_SHAPE& shape = static_cast<const SCH_SHAPE&>( draw );

                    if( shape.GetShape() == SHAPE_T::POLY && shape.GetFillMode() == FILL_T::NO_FILL
                        && shape.GetPolyShape().TotalVertices() == aPointCount )
                    {
                        count++;
                    }
                }

                return count;
            }
        }

        return -1;
    }

    /// Return the units of all symbols matching the resolved reference on the named sheet.
    std::vector<int> UnitsForRefdesOnSheet( const wxString& aRefdes, const wxString& aSheetName )
    {
        std::vector<int> units;

        for( const SCH_SHEET_PATH& sheetPath : m_schematic->BuildUnorderedSheetList() )
        {
            if( !sheetPath.Last() || sheetPath.Last()->GetName() != aSheetName )
                continue;

            SCH_SCREEN* screen = sheetPath.LastScreen();

            if( !screen )
                continue;

            for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
            {
                SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

                if( symbol->GetRef( &sheetPath, false ) == aRefdes )
                    units.push_back( symbol->GetUnit() );
            }
        }

        std::sort( units.begin(), units.end() );
        return units;
    }

    /// Return the unit count of the first library symbol matching the resolved reference.
    int UnitCountForRefdes( const wxString& aRefdes )
    {
        for( const SCH_SHEET_PATH& sheetPath : m_schematic->BuildUnorderedSheetList() )
        {
            SCH_SCREEN* screen = sheetPath.LastScreen();

            if( !screen )
                continue;

            for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
            {
                SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

                if( symbol->GetRef( &sheetPath, false ) == aRefdes )
                    return symbol->GetUnitCount();
            }
        }

        return -1;
    }

    /// Return the placed symbol's resolved library pin count for a specific unit on a sheet.
    int LibPinCountForRefdesUnitOnSheet( const wxString& aRefdes, const wxString& aSheetName, int aUnit )
    {
        for( const SCH_SHEET_PATH& sheetPath : m_schematic->BuildUnorderedSheetList() )
        {
            if( !sheetPath.Last() || sheetPath.Last()->GetName() != aSheetName )
                continue;

            SCH_SCREEN* screen = sheetPath.LastScreen();

            if( !screen )
                continue;

            for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
            {
                SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

                if( symbol->GetRef( &sheetPath, false ) == aRefdes && symbol->GetUnit() == aUnit )
                    return static_cast<int>( symbol->GetLibPins().size() );
            }
        }

        return -1;
    }

    /// Return the library pin names for a specific placed unit on a sheet.
    std::set<wxString> LibPinNamesForRefdesUnitOnSheet( const wxString& aRefdes, const wxString& aSheetName, int aUnit )
    {
        for( const SCH_SHEET_PATH& sheetPath : m_schematic->BuildUnorderedSheetList() )
        {
            if( !sheetPath.Last() || sheetPath.Last()->GetName() != aSheetName )
                continue;

            SCH_SCREEN* screen = sheetPath.LastScreen();

            if( !screen )
                continue;

            for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
            {
                SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

                if( symbol->GetRef( &sheetPath, false ) != aRefdes || symbol->GetUnit() != aUnit )
                    continue;

                std::set<wxString> names;

                for( SCH_PIN* pin : symbol->GetLibPins() )
                    names.insert( pin->GetName() );

                return names;
            }
        }

        return {};
    }

    /// Count junctions on the sheet with the given name.
    int CountJunctionsOnSheet( const wxString& aSheetName )
    {
        int count = 0;

        for( const SCH_SHEET_PATH& sheetPath : m_schematic->BuildUnorderedSheetList() )
        {
            if( !sheetPath.Last() || sheetPath.Last()->GetName() != aSheetName )
                continue;

            SCH_SCREEN* screen = sheetPath.LastScreen();

            if( !screen )
                continue;

            for( SCH_ITEM* item : screen->Items().OfType( SCH_JUNCTION_T ) )
            {
                static_cast<void>( item );
                count++;
            }
        }

        return count;
    }

    /// The position of a field (by type) of the first symbol matching the reference, or a sentinel
    /// far-away point when absent.
    VECTOR2I FieldPosForRefdes( const wxString& aRefdes, FIELD_T aFieldType )
    {
        for( const SCH_SHEET_PATH& sheetPath : m_schematic->BuildUnorderedSheetList() )
        {
            SCH_SCREEN* screen = sheetPath.LastScreen();

            if( !screen )
                continue;

            for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
            {
                SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

                if( symbol->GetRef( &sheetPath, false ) != aRefdes )
                    continue;

                if( SCH_FIELD* field = symbol->GetField( aFieldType ) )
                    return field->GetPosition();
            }
        }

        return VECTOR2I( INT_MAX, INT_MAX );
    }

    /// The horizontal justification of a field (by type) of the first matching reference, as the
    /// raw enum int, or 99 when absent.
    int FieldHorizJustify( const wxString& aRefdes, FIELD_T aFieldType )
    {
        for( const SCH_SHEET_PATH& sheetPath : m_schematic->BuildUnorderedSheetList() )
        {
            SCH_SCREEN* screen = sheetPath.LastScreen();

            if( !screen )
                continue;

            for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
            {
                SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

                if( symbol->GetRef( &sheetPath, false ) != aRefdes )
                    continue;

                if( SCH_FIELD* field = symbol->GetField( aFieldType ) )
                    return static_cast<int>( field->GetHorizJustify() );
            }
        }

        return 99;
    }

    /// Count global labels whose text matches exactly on the sheet with the given name.
    int CountLabelsOnSheet( const wxString& aText, const wxString& aSheetName )
    {
        int count = 0;

        for( const SCH_SHEET_PATH& sheetPath : m_schematic->BuildUnorderedSheetList() )
        {
            if( !sheetPath.Last() || sheetPath.Last()->GetName() != aSheetName )
                continue;

            SCH_SCREEN* screen = sheetPath.LastScreen();

            if( !screen )
                continue;

            for( SCH_ITEM* item : screen->Items().OfType( SCH_GLOBAL_LABEL_T ) )
            {
                if( static_cast<SCH_LABEL_BASE*>( item )->GetText() == aText )
                    count++;
            }
        }

        return count;
    }

    /// True when a global label whose text starts with the prefix sits on a wire endpoint on the
    /// named sheet. A net-port label must connect to its net, so its anchor coincides with a wire
    /// end rather than floating at the symbol body.
    bool LabelConnectsToWireOnSheet( const wxString& aPrefix, const wxString& aSheetName )
    {
        for( const SCH_SHEET_PATH& sheetPath : m_schematic->BuildUnorderedSheetList() )
        {
            if( !sheetPath.Last() || sheetPath.Last()->GetName() != aSheetName )
                continue;

            SCH_SCREEN* screen = sheetPath.LastScreen();

            if( !screen )
                continue;

            for( SCH_ITEM* labelItem : screen->Items().OfType( SCH_GLOBAL_LABEL_T ) )
            {
                SCH_LABEL_BASE* label = static_cast<SCH_LABEL_BASE*>( labelItem );

                if( !label->GetText().StartsWith( aPrefix ) )
                    continue;

                VECTOR2I anchor = label->GetPosition();

                for( SCH_ITEM* lineItem : screen->Items().OfType( SCH_LINE_T ) )
                {
                    SCH_LINE* line = static_cast<SCH_LINE*>( lineItem );

                    if( line->GetStartPoint() == anchor || line->GetEndPoint() == anchor )
                        return true;
                }
            }
        }

        return false;
    }

    struct DCH_XML_COUNTS
    {
        int partCount = 0;
        int netPortCount = 0; ///< Parts with PartType="Net Port" (imported as global labels)
        int sheetCount = 0;
    };

    static bool XmlSubtreeContains( wxXmlNode* aNode, const wxString& aNeedle )
    {
        for( wxXmlNode* n = aNode; n; n = n->GetNext() )
        {
            if( n->GetType() == wxXML_TEXT_NODE && n->GetContent().Contains( aNeedle ) )
                return true;

            if( XmlSubtreeContains( n->GetChildren(), aNeedle ) )
                return true;
        }

        return false;
    }

    bool LoadDchXmlCounts( const std::string& aXmlPath, DCH_XML_COUNTS& aCounts )
    {
        if( !wxFileExists( aXmlPath ) )
            return false;

        wxXmlDocument doc;

        if( !doc.Load( aXmlPath ) )
            return false;

        wxXmlNode* root = doc.GetRoot();

        if( !root || root->GetName() != "Source" )
            return false;

        wxXmlNode* schematicNode = nullptr;

        for( wxXmlNode* child = root->GetChildren(); child; child = child->GetNext() )
        {
            if( child->GetType() == wxXML_ELEMENT_NODE && child->GetName() == "Schematic" )
            {
                schematicNode = child;
                break;
            }
        }

        if( !schematicNode )
            return false;

        int                     partCount = 0;
        int                     netPortCount = 0;
        int                     sheetCount = 0;
        std::vector<wxXmlNode*> stack = { schematicNode };

        while( !stack.empty() )
        {
            wxXmlNode* node = stack.back();
            stack.pop_back();

            if( !node || node->GetType() != wxXML_ELEMENT_NODE )
                continue;

            if( node->GetName() == "Part" )
            {
                partCount++;

                // Net-port placements carry an auto_net_ports library reference; the importer maps
                // these to global labels rather than symbols, so count them the same way here.
                if( XmlSubtreeContains( node->GetChildren(), wxT( "auto_net_ports" ) ) )
                    netPortCount++;
            }
            else if( node->GetName() == "Sheet" )
            {
                sheetCount++;
            }

            for( wxXmlNode* child = node->GetChildren(); child; child = child->GetNext() )
            {
                stack.push_back( child );
            }
        }

        aCounts.partCount = partCount;
        aCounts.netPortCount = netPortCount;
        aCounts.sheetCount = sheetCount;
        return true;
    }

    SCH_SHEET* LoadDipTraceSchematic( const std::string& aFilePath )
    {
        m_schematic.reset( new SCHEMATIC( nullptr ) );
        m_manager.LoadProject( "" );
        m_schematic->SetProject( &m_manager.Prj() );
        m_schematic->CurrentSheet().clear();
        m_schematic->CurrentSheet().push_back( &m_schematic->Root() );

        m_reporter.Clear();
        m_plugin.SetReporter( &m_reporter );
        m_loadedRoot = m_plugin.LoadSchematicFile( aFilePath, m_schematic.get() );
        return m_loadedRoot;
    }

    void RemoveGeneratedLibrary( const std::string& aFilePath )
    {
        wxFileName genLib( aFilePath );
        genLib.SetName( genLib.GetName() + wxT( "-diptrace-import" ) );
        genLib.SetExt( wxT( "kicad_sym" ) );

        if( genLib.FileExists() )
            wxRemoveFile( genLib.GetFullPath() );
    }
};

#endif // TEST_DIPTRACE_SCH_IMPORT_FIXTURE_H
