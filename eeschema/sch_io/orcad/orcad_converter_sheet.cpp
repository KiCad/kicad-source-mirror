/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * Based on the dsn2kicad reference implementation and on OrCAD file format
 * documentation from the OpenOrCadParser project (MIT licensed).
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file orcad_converter_sheet.cpp
 * Sheet-level ORCAD_CONVERTER: page assembly, wires, buses, labels, junctions,
 * graphics, title blocks, page sizing, bitmaps.
 */

#include <sch_io/orcad/orcad_converter.h>
#include <sch_io/orcad/orcad_ole.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include <wx/buffer.h>
#include <wx/filename.h>
#include <wx/log.h>
#include <wx/translation.h>

#include <base_units.h>
#include <ki_exception.h>
#include <layer_ids.h>
#include <lib_symbol.h>
#include <math/util.h>
#include <page_info.h>
#include <progress_reporter.h>
#include <project.h>
#include <reference_image.h>
#include <reporter.h>
#include <schematic.h>
#include <sch_bitmap.h>
#include <sch_bus_entry.h>
#include <sch_junction.h>
#include <sch_label.h>
#include <sch_line.h>
#include <sch_no_connect.h>
#include <sch_screen.h>
#include <sch_shape.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <sch_sheet_pin.h>
#include <sch_text.h>
#include <string_utils.h>
#include <stroke_params.h>
#include <template_fieldnames.h>
#include <title_block.h>

#include <sch_io/orcad/orcad_stream.h>


namespace
{

/// OrCAD DBU (10 mil) -> millimetres.
constexpr double DBU_TO_MM = 0.254;



std::string trimmed( const std::string& aText )
{
    size_t begin = aText.find_first_not_of( " \t\r\n\f\v" );

    if( begin == std::string::npos )
        return std::string();

    size_t end = aText.find_last_not_of( " \t\r\n\f\v" );

    return aText.substr( begin, end - begin + 1 );
}


void pollProgress( PROGRESS_REPORTER* aReporter, const std::string& aPageName )
{
    if( !aReporter )
        return;

    aReporter->Report( wxString::Format( _( "Converting page '%s'..." ),
                                         FromOrcadString( aPageName ) ) );

    if( !aReporter->KeepRefreshing() )
        THROW_IO_ERROR( _( "Open canceled by user." ) );
}


SCH_SHAPE* makeSheetPoly( const std::vector<VECTOR2I>& aPoints, const ORCAD_PRIMITIVE& aPrimitive,
                          const KIGFX::COLOR4D& aColor, bool aCanFill = false )
{
    SCH_SHAPE* poly = new SCH_SHAPE( SHAPE_T::POLY, LAYER_NOTES );

    for( const VECTOR2I& pt : aPoints )
        poly->AddPoint( pt );

    poly->SetStroke( STROKE_PARAMS( OrcadLineWidthIu( aPrimitive.lineWidth ),
                                    OrcadLineStyle( aPrimitive.lineStyle ), aColor ) );

    // Page graphics have no symbol body color, so solid fill uses the foreground color.
    if( aCanFill && aPrimitive.fillStyle == 0 )
        poly->SetFillMode( FILL_T::FILLED_SHAPE );
    else
        poly->SetFillMode( aCanFill ? OrcadFillType( aPrimitive.fillStyle, aPrimitive.hatchStyle )
                                    : FILL_T::NO_FILL );

    return poly;
}


VECTOR2I dbuPointToIu( double aX, double aY )
{
    return VECTOR2I( KiROUND( aX * ORCAD_IU_PER_DBU ), KiROUND( aY * ORCAD_IU_PER_DBU ) );
}

} // namespace


KIGFX::COLOR4D OrcadColor( int aColorIndex )
{
    static constexpr uint8_t palette[][3] = {
        { 0, 0, 0 },       { 255, 255, 128 }, { 128, 255, 128 }, { 0, 255, 128 },   { 128, 255, 255 }, { 0, 128, 255 },
        { 255, 128, 192 }, { 255, 128, 255 }, { 255, 0, 0 },     { 255, 255, 0 },   { 128, 255, 0 },   { 0, 255, 64 },
        { 0, 255, 255 },   { 0, 128, 192 },   { 128, 128, 192 }, { 255, 0, 255 },   { 128, 64, 64 },   { 255, 128, 64 },
        { 0, 255, 0 },     { 0, 128, 128 },   { 0, 64, 128 },    { 128, 128, 255 }, { 128, 0, 64 },    { 255, 0, 128 },
        { 128, 0, 0 },     { 255, 128, 0 },   { 0, 128, 0 },     { 0, 128, 64 },    { 0, 0, 255 },     { 0, 0, 160 },
        { 128, 0, 128 },   { 128, 0, 255 },   { 64, 0, 0 },      { 128, 64, 0 },    { 0, 64, 0 },      { 0, 64, 64 },
        { 0, 0, 128 },     { 0, 0, 64 },      { 64, 0, 64 },     { 64, 0, 128 },    { 0, 0, 0 },       { 128, 128, 0 },
        { 128, 128, 64 },  { 128, 128, 128 }, { 64, 128, 128 },  { 192, 192, 192 }, { 64, 0, 64 },     { 255, 255, 255 }
    };

    if( aColorIndex < 1 || aColorIndex >= static_cast<int>( std::size( palette ) ) )
        return KIGFX::COLOR4D::UNSPECIFIED;

    return KIGFX::COLOR4D( palette[aColorIndex][0] / 255.0, palette[aColorIndex][1] / 255.0,
                           palette[aColorIndex][2] / 255.0, 1.0 );
}


ORCAD_CONVERTER::ORCAD_CONVERTER( ORCAD_DESIGN& aDesign, SCHEMATIC* aSchematic, REPORTER* aReporter,
                                  PROGRESS_REPORTER* aProgressReporter ) :
        m_design( aDesign ),
        m_schematic( aSchematic ),
        m_reporter( aReporter ),
        m_progressReporter( aProgressReporter ),
        m_rootSheet( nullptr ),
        m_powerCount( 0 ),
        m_fontBaselineDbu( 0 )
{
}


ORCAD_CONVERTER::~ORCAD_CONVERTER() = default;


void ORCAD_CONVERTER::warn( const wxString& aMsg )
{
    if( m_reporter )
        m_reporter->Report( aMsg, RPT_SEVERITY_WARNING );
}


void ORCAD_CONVERTER::note( const wxString& aMsg )
{
    if( m_reporter )
        m_reporter->Report( aMsg, RPT_SEVERITY_INFO );
}


/// Return a numeric page prefix (or -1); strip only the "N - title" convention.
int OrcadPageOrder( wxString& aName )
{
    size_t   digitStart = 0;
    wxString upper = aName.Upper();

    for( const wxString& prefix : { wxS( "PAGE" ), wxS( "SCH" ), wxS( "PAG" ) } )
    {
        if( upper.StartsWith( prefix )
            && ( aName.length() == prefix.length() || wxIsdigit( aName[prefix.length()] )
                 || wxIsspace( aName[prefix.length()] ) || aName[prefix.length()] == '_' ) )
        {
            digitStart = prefix.length();

            while( digitStart < aName.length()
                   && ( wxIsspace( aName[digitStart] ) || aName[digitStart] == '_' ) )
            {
                digitStart++;
            }

            break;
        }
    }

    size_t digitEnd = digitStart;

    while( digitEnd < aName.length() && wxIsdigit( aName[digitEnd] ) )
        digitEnd++;

    long order = 0;

    if( digitEnd == digitStart
        || !aName.Mid( digitStart, digitEnd - digitStart ).ToLong( &order ) )
    {
        return -1;
    }

    size_t separator = digitEnd;

    while( separator < aName.length() && wxIsspace( aName[separator] ) )
        separator++;

    if( separator == aName.length()
        || ( aName[separator] != '-' && aName[separator] != '.' && aName[separator] != ':' ) )
        return -1;

    if( aName[separator] == '-' )
    {
        wxString rest = aName.Mid( separator + 1 );
        rest.Trim( false );
        aName = rest;
    }

    return static_cast<int>( order );
}


SCH_SHEET* ORCAD_CONVERTER::Convert( SCH_SHEET* aRootSheet )
{
    m_rootSheet = aRootSheet;

    prepareSymbols();

    SCH_SCREEN* rootScreen = aRootSheet->GetScreen();

    SCH_SHEET_PATH rootPath;
    rootPath.push_back( aRootSheet );
    rootPath.SetPageNumber( wxS( "1" ) );

    std::function<bool( const ORCAD_RAW_PAGE&, const ORCAD_OCC_SCOPE& )> canBuildHierarchy =
            [&]( const ORCAD_RAW_PAGE& aPage, const ORCAD_OCC_SCOPE& aScope )
    {
        if( aPage.blocks.size() != aScope.blocks.size() )
            return false;

        std::set<uint32_t> matchedBlocks;

        for( const ORCAD_OCC_BLOCK& occurrence : aScope.blocks )
        {
            auto drawn = std::find_if( aPage.blocks.begin(), aPage.blocks.end(),
                    [&]( const ORCAD_DRAWN_INSTANCE& aBlock )
                    {
                        return aBlock.dbId == occurrence.targetDbId;
                    } );

            std::string key = occurrence.childFolder;
            std::transform( key.begin(), key.end(), key.begin(),
                            []( unsigned char c ) { return static_cast<char>( std::tolower( c ) ); } );

            auto pages = m_design.childFolderPages.find( key );

            if( !matchedBlocks.insert( occurrence.targetDbId ).second
                || drawn == aPage.blocks.end() || pages == m_design.childFolderPages.end()
                || pages->second.size() != 1 || !canBuildHierarchy( pages->second.front(), occurrence.scope ) )
            {
                return false;
            }
        }

        return true;
    };

    bool nativeHierarchy = m_design.pages.size() == 1 && !m_design.occurrenceRoot.blocks.empty()
                           && canBuildHierarchy( m_design.pages.front(), m_design.occurrenceRoot );

    if( nativeHierarchy )
    {
        ORCAD_RAW_PAGE& rootPage = m_design.pages.front();

        pollProgress( m_progressReporter, rootPage.name );
        m_currentOccRefs = &m_design.occurrenceRoot.partRefs;
        applyPageSettings( rootPage, rootScreen );
        convertPage( rootPage, rootScreen, rootPath );

        int pageIndex = 1;

        std::function<void( ORCAD_RAW_PAGE&, const ORCAD_OCC_SCOPE&, SCH_SHEET*,
                            const SCH_SHEET_PATH& )> placeChildren =
                [&]( ORCAD_RAW_PAGE& aParentPage, const ORCAD_OCC_SCOPE& aScope,
                     SCH_SHEET* aParentSheet, const SCH_SHEET_PATH& aParentPath )
        {
            std::vector<const ORCAD_OCC_BLOCK*> occurrences;

            for( const ORCAD_OCC_BLOCK& occurrence : aScope.blocks )
                occurrences.push_back( &occurrence );

            std::stable_sort( occurrences.begin(), occurrences.end(),
                    []( const ORCAD_OCC_BLOCK* a, const ORCAD_OCC_BLOCK* b )
                    {
                        wxString aName = FromOrcadString( a->childFolder );
                        wxString bName = FromOrcadString( b->childFolder );
                        int      aOrder = OrcadPageOrder( aName );
                        int      bOrder = OrcadPageOrder( bName );
                        int      aKey = aOrder >= 0 ? aOrder : std::numeric_limits<int>::max();
                        int      bKey = bOrder >= 0 ? bOrder : std::numeric_limits<int>::max();
                        return aKey < bKey;
                    } );

            for( const ORCAD_OCC_BLOCK* occurrencePtr : occurrences )
            {
                const ORCAD_OCC_BLOCK& occurrence = *occurrencePtr;
                auto drawn = std::find_if( aParentPage.blocks.begin(), aParentPage.blocks.end(),
                        [&]( const ORCAD_DRAWN_INSTANCE& aBlock )
                        {
                            return aBlock.dbId == occurrence.targetDbId;
                        } );

                std::string key = occurrence.childFolder;
                std::transform( key.begin(), key.end(), key.begin(),
                                []( unsigned char c ) { return static_cast<char>( std::tolower( c ) ); } );

                ORCAD_RAW_PAGE& childPage = m_design.childFolderPages.at( key ).front();
                SCH_SCREEN*     childScreen = new SCH_SCREEN( m_schematic );
                SCH_SHEET*      childSheet = new SCH_SHEET( aParentSheet,
                                                            OrcadDbuToIu( drawn->x1, drawn->y1 ),
                                                            OrcadDbuToIu( drawn->w, drawn->h ) );
                wxString        sheetName = FromOrcadString( drawn->reference );

                if( sheetName.IsEmpty() )
                    sheetName = FromOrcadString( childPage.name );

                wxString base = sheetName;

                for( int suffix = 2; !m_usedSheetNames.insert( sheetName.Lower() ).second; ++suffix )
                    sheetName = wxString::Format( wxS( "%s (%d)" ), base, suffix );

                wxString fileName = MakePageFileName( ++pageIndex, childPage.name );
                childSheet->GetField( FIELD_T::SHEET_NAME )->SetText( sheetName );
                childSheet->GetField( FIELD_T::SHEET_FILENAME )->SetText( fileName );
                childSheet->SetScreen( childScreen );
                childScreen->SetFileName( m_schematic->Project().GetProjectPath() + fileName );

                for( const ORCAD_BLOCK_PIN& sourcePin : drawn->pins )
                {
                    VECTOR2I       position = OrcadDbuToIu( sourcePin.x, sourcePin.y );
                    SCH_SHEET_PIN* pin = new SCH_SHEET_PIN( childSheet, position,
                                                           FromOrcadString( sourcePin.name ) );
                    std::array<std::pair<int, SHEET_SIDE>, 4> sides = {
                        std::pair{ std::abs( sourcePin.x - drawn->x1 ), SHEET_SIDE::LEFT },
                        std::pair{ std::abs( sourcePin.x - drawn->x1 - drawn->w ), SHEET_SIDE::RIGHT },
                        std::pair{ std::abs( sourcePin.y - drawn->y1 ), SHEET_SIDE::TOP },
                        std::pair{ std::abs( sourcePin.y - drawn->y1 - drawn->h ), SHEET_SIDE::BOTTOM }
                    };

                    pin->SetSide( std::min_element( sides.begin(), sides.end(),
                                          []( const auto& a, const auto& b )
                                          {
                                              return a.first < b.first;
                                          } )->second );

                    switch( sourcePin.portType )
                    {
                    case ORCAD_PORT_TYPE::INPUT:         pin->SetShape( LABEL_FLAG_SHAPE::L_INPUT ); break;
                    case ORCAD_PORT_TYPE::OUTPUT:        pin->SetShape( LABEL_FLAG_SHAPE::L_OUTPUT ); break;
                    case ORCAD_PORT_TYPE::BIDIRECTIONAL: pin->SetShape( LABEL_FLAG_SHAPE::L_BIDI ); break;
                    case ORCAD_PORT_TYPE::TRI_STATE:     pin->SetShape( LABEL_FLAG_SHAPE::L_TRISTATE ); break;
                    default:                            pin->SetShape( LABEL_FLAG_SHAPE::L_UNSPECIFIED ); break;
                    }

                    childSheet->AddPin( pin );
                }

                aParentSheet->GetScreen()->Append( childSheet );

                SCH_SHEET_PATH childPath = aParentPath;
                childPath.push_back( childSheet );
                childPath.SetPageNumber( wxString::Format( wxS( "%d" ), pageIndex ) );

                pollProgress( m_progressReporter, childPage.name );
                m_currentOccRefs = &occurrence.scope.partRefs;
                applyPageSettings( childPage, childScreen );
                convertPage( childPage, childScreen, childPath );
                placeChildren( childPage, occurrence.scope, childSheet, childPath );
            }
        };

        placeChildren( rootPage, m_design.occurrenceRoot, aRootSheet, rootPath );
        m_currentOccRefs = nullptr;
        return aRootSheet;
    }

    // Page list = root pages + each block occurrence's child pages, tagged w/ scope refs.
    // Child schematic reused N times yields N jobs, each w/ own designators.
    struct PAGE_JOB
    {
        ORCAD_RAW_PAGE*                        page;
        const std::map<uint32_t, std::string>* refs;
    };

    std::vector<PAGE_JOB> jobs;

    for( ORCAD_RAW_PAGE& page : m_design.pages )
        jobs.push_back( { &page, &m_design.occurrenceRoot.partRefs } );

    std::function<void( const ORCAD_OCC_SCOPE& )> expand =
            [&]( const ORCAD_OCC_SCOPE& aScope )
    {
        for( const ORCAD_OCC_BLOCK& block : aScope.blocks )
        {
            std::string key = block.childFolder;
            std::transform( key.begin(), key.end(), key.begin(),
                            []( unsigned char c ) { return static_cast<char>( std::tolower( c ) ); } );

            auto it = m_design.childFolderPages.find( key );

            if( it != m_design.childFolderPages.end() )
            {
                for( ORCAD_RAW_PAGE& childPage : it->second )
                    jobs.push_back( { &childPage, &block.scope.partRefs } );
            }

            expand( block.scope );
        }
    };

    expand( m_design.occurrenceRoot );

    if( jobs.size() == 1 )
    {
        PAGE_JOB& job = jobs[0];

        pollProgress( m_progressReporter, job.page->name );
        m_currentOccRefs = job.refs;
        applyPageSettings( *job.page, rootScreen );
        convertPage( *job.page, rootScreen, rootPath );
        m_currentOccRefs = nullptr;
    }
    else
    {
        // Each page = flat top-level sheet (no stitching root); order root pages by
        // leading "N - " prefix, also stripped for title.
        struct SHEET_JOB
        {
            PAGE_JOB job;
            wxString name;
            int      order;
        };

        std::vector<SHEET_JOB> sheetJobs;

        for( size_t i = 0; i < jobs.size(); ++i )
        {
            wxString name = FromOrcadString( jobs[i].page->name );
            int      order = i < m_design.pages.size() ? OrcadPageOrder( name ) : -1;

            if( name.IsEmpty() )
                name = wxString::Format( wxS( "PAGE%zu" ), i + 1 );

            sheetJobs.push_back( { jobs[i], name, order } );
        }

        std::stable_sort( sheetJobs.begin(), sheetJobs.end(),
                          []( const SHEET_JOB& a, const SHEET_JOB& b )
                          {
                              int ka = a.order >= 0 ? a.order : std::numeric_limits<int>::max();
                              int kb = b.order >= 0 ? b.order : std::numeric_limits<int>::max();
                              return ka < kb;
                          } );

        std::vector<SCH_SHEET*> topSheets;

        for( size_t i = 0; i < sheetJobs.size(); ++i )
        {
            SHEET_JOB& sj = sheetJobs[i];

            pollProgress( m_progressReporter, sj.job.page->name );

            SCH_SHEET*  sheet;
            SCH_SCREEN* screen;

            if( i == 0 )
            {
                // Reuse sheet the loader created for first page
                sheet = aRootSheet;
                screen = rootScreen;
            }
            else
            {
                screen = new SCH_SCREEN( m_schematic );
                sheet = new SCH_SHEET( m_schematic );
                sheet->SetScreen( screen );
                const_cast<KIID&>( sheet->m_Uuid ) = screen->GetUuid();
            }

            wxString base = sj.name;

            for( int suffix = 2; !m_usedSheetNames.insert( sj.name.Lower() ).second; ++suffix )
                sj.name = wxString::Format( wxS( "%s (%d)" ), base, suffix );

            wxString fileName = MakePageFileName( static_cast<int>( i + 1 ), sj.job.page->name );

            sheet->GetField( FIELD_T::SHEET_NAME )->SetText( sj.name );
            sheet->GetField( FIELD_T::SHEET_FILENAME )->SetText( fileName );
            screen->SetFileName( m_schematic->Project().GetProjectPath() + fileName );

            SCH_SHEET_PATH pagePath;
            pagePath.push_back( sheet );
            pagePath.SetPageNumber( wxString::Format( wxS( "%zu" ), i + 1 ) );

            m_currentOccRefs = sj.job.refs;
            applyPageSettings( *sj.job.page, screen );
            convertPage( *sj.job.page, screen, pagePath );
            m_currentOccRefs = nullptr;

            topSheets.push_back( sheet );
        }

        m_schematic->SetTopLevelSheets( topSheets );
    }

    return aRootSheet;
}


void ORCAD_CONVERTER::convertPage( ORCAD_RAW_PAGE& aPage, SCH_SCREEN* aScreen,
                                   const SCH_SHEET_PATH& aSheetPath )
{
    applyTitleBlock( aPage, aScreen );
    buildNetLookup( aPage );

    placeJunctions( aPage, aScreen );
    placeNoConnects( aPage, aScreen );
    placeBusEntries( aPage, aScreen );
    placeWires( aPage, aScreen );
    placeOffpageConnectors( aPage, aScreen );
    placePorts( aPage, aScreen, aSheetPath.size() > 1 );
    placeGraphics( aPage, aScreen );

    for( const ORCAD_PLACED_INSTANCE& instance : aPage.instances )
        placeInstance( aPage, instance, aScreen, aSheetPath );

    for( const ORCAD_GRAPHIC_INST& global : aPage.globals )
        placePowerSymbol( aPage, global, powerNet( aPage, global ), aScreen, aSheetPath );
}


wxString ORCAD_CONVERTER::MakePageFileName( int aPageIndex, const std::string& aPageName )
{
    wxString fileName = wxString::Format( wxS( "P%02d_" ), aPageIndex )
                        + SanitizeFileName( aPageName ) + wxS( ".kicad_sch" );

    ReplaceIllegalFileNameChars( fileName, '_' );

    return fileName;
}


wxString ORCAD_CONVERTER::SanitizeFileName( const std::string& aName )
{
    const wxString illegal( wxS( "<>:\"/\\|?*" ) );
    wxString       in = FromOrcadString( aName );
    wxString       out;

    for( wxUniChar c : in )
    {
        if( c.GetValue() < 0x20 || illegal.Find( c ) != wxNOT_FOUND )
            out += '_';
        else
            out += c;
    }

    while( !out.IsEmpty() && ( out.Last() == ' ' || out.Last() == '.' ) )
        out.RemoveLast();

    while( !out.IsEmpty() && ( out.GetChar( 0 ) == ' ' || out.GetChar( 0 ) == '.' ) )
        out.Remove( 0, 1 );

    if( out.IsEmpty() )
        out = wxS( "unnamed" );

    return out;
}


void ORCAD_CONVERTER::applyPageSettings( ORCAD_RAW_PAGE& aPage, SCH_SCREEN* aScreen )
{
    BOX2I extent = pageExtentDbu( aPage );

    // Shift content clear of frame, round up to 10-DBU grid to keep points on grid.
    int dx = std::max( 0, MARGIN_L_DBU - extent.GetLeft() );
    int dy = std::max( 0, MARGIN_T_DBU - extent.GetTop() );

    dx = ( dx + 9 ) / 10 * 10;
    dy = ( dy + 9 ) / 10 * 10;

    int maxX = extent.GetRight();
    int maxY = extent.GetBottom();

    if( dx || dy )
    {
        offsetPage( aPage, dx, dy );
        maxX += dx;
        maxY += dy;
    }

    // Nominal paper from stored page size; mils, or micrometres when metric.
    double k = aPage.isMetric ? 0.001 : 0.0254;
    double nominalWmm = aPage.width * k;
    double nominalHmm = aPage.height * k;

    // Needed paper = shifted content extent plus right/bottom margins.
    double neededWmm = ( maxX + MARGIN_R_DBU ) * DBU_TO_MM;
    double neededHmm = ( maxY + MARGIN_B_DBU ) * DBU_TO_MM;

    int paperWmm = static_cast<int>( std::ceil( std::max( nominalWmm, neededWmm ) ) );
    int paperHmm = static_cast<int>( std::ceil( std::max( nominalHmm, neededHmm ) ) );

    PAGE_INFO pageInfo;
    PAGE_INFO::SetCustomWidthMils( paperWmm * 1000.0 / 25.4 );
    PAGE_INFO::SetCustomHeightMils( paperHmm * 1000.0 / 25.4 );
    pageInfo.SetType( PAGE_SIZE_TYPE::User );

    aScreen->SetPageSettings( pageInfo );
}


BOX2I ORCAD_CONVERTER::pageExtentDbu( const ORCAD_RAW_PAGE& aPage )
{
    bool any = false;
    int  minX = 0;
    int  minY = 0;
    int  maxX = 0;
    int  maxY = 0;

    auto add = [&]( int aX, int aY )
    {
        if( !any )
        {
            minX = maxX = aX;
            minY = maxY = aY;
            any = true;
        }
        else
        {
            minX = std::min( minX, aX );
            minY = std::min( minY, aY );
            maxX = std::max( maxX, aX );
            maxY = std::max( maxY, aY );
        }
    };

    for( const ORCAD_WIRE& wire : aPage.wires )
    {
        add( wire.x1, wire.y1 );
        add( wire.x2, wire.y2 );
    }

    for( const ORCAD_PLACED_INSTANCE& instance : aPage.instances )
    {
        add( instance.bbox.x1, instance.bbox.y1 );
        add( instance.bbox.x2, instance.bbox.y2 );

        for( const ORCAD_PIN_INST& pin : instance.pins )
            add( pin.x, pin.y );
    }

    for( const std::vector<ORCAD_GRAPHIC_INST>* list :
         { &aPage.globals, &aPage.offpage, &aPage.ports, &aPage.ercObjects } )
    {
        for( const ORCAD_GRAPHIC_INST& inst : *list )
        {
            add( inst.x, inst.y );
            add( inst.bbox.x1, inst.bbox.y1 );
            add( inst.bbox.x2, inst.bbox.y2 );
        }
    }

    // Free graphics outer bbox = anchor-relative junk; only nested primitives carry real coords.
    for( const ORCAD_GRAPHIC_INST& gfx : aPage.graphics )
    {
        if( !gfx.nested )
            continue;

        for( const ORCAD_PRIMITIVE& prim : gfx.nested->primitives )
        {
            if( !prim.points.empty() )
            {
                for( const ORCAD_POINT& pt : prim.points )
                    add( pt.x, pt.y );
            }
            else
            {
                add( prim.x1, prim.y1 );
                add( prim.x2, prim.y2 );
            }
        }
    }

    for( const ORCAD_BUS_ENTRY& entry : aPage.busEntries )
    {
        add( entry.x1, entry.y1 );
        add( entry.x2, entry.y2 );
    }

    for( const ORCAD_DRAWN_INSTANCE& block : aPage.blocks )
    {
        add( block.x1, block.y1 );
        add( block.x1 + block.w, block.y1 + block.h );

        for( const ORCAD_BLOCK_PIN& pin : block.pins )
            add( pin.x, pin.y );
    }

    if( !any )
        return BOX2I( VECTOR2I( 0, 0 ), VECTOR2I( 3800, 2700 ) );

    return BOX2I( VECTOR2I( minX, minY ), VECTOR2I( maxX - minX, maxY - minY ) );
}


void ORCAD_CONVERTER::offsetPage( ORCAD_RAW_PAGE& aPage, int aDx, int aDy )
{
    for( ORCAD_WIRE& wire : aPage.wires )
    {
        wire.x1 += aDx;
        wire.y1 += aDy;
        wire.x2 += aDx;
        wire.y2 += aDy;

        for( ORCAD_ALIAS& alias : wire.aliases )
        {
            alias.x += aDx;
            alias.y += aDy;
        }
    }

    for( ORCAD_PLACED_INSTANCE& instance : aPage.instances )
    {
        instance.x += aDx;
        instance.y += aDy;
        instance.bbox.x1 += aDx;
        instance.bbox.y1 += aDy;
        instance.bbox.x2 += aDx;
        instance.bbox.y2 += aDy;

        for( ORCAD_PIN_INST& pin : instance.pins )
        {
            pin.x += aDx;
            pin.y += aDy;
        }
    }

    auto shiftGraphic = [&]( ORCAD_GRAPHIC_INST& aInst )
    {
        aInst.x += aDx;
        aInst.y += aDy;
        aInst.bbox.x1 += aDx;
        aInst.bbox.y1 += aDy;
        aInst.bbox.x2 += aDx;
        aInst.bbox.y2 += aDy;

        if( !aInst.nested )
            return;

        for( ORCAD_PRIMITIVE& prim : aInst.nested->primitives )
        {
            prim.x1 += aDx;
            prim.y1 += aDy;
            prim.x2 += aDx;
            prim.y2 += aDy;

            for( ORCAD_POINT& pt : prim.points )
            {
                pt.x += aDx;
                pt.y += aDy;
            }

            if( prim.start )
            {
                prim.start->x += aDx;
                prim.start->y += aDy;
            }

            if( prim.end )
            {
                prim.end->x += aDx;
                prim.end->y += aDy;
            }
        }
    };

    for( std::vector<ORCAD_GRAPHIC_INST>* list : { &aPage.globals, &aPage.offpage, &aPage.ports,
                                                   &aPage.ercObjects, &aPage.graphics } )
    {
        for( ORCAD_GRAPHIC_INST& inst : *list )
            shiftGraphic( inst );
    }

    for( ORCAD_BUS_ENTRY& entry : aPage.busEntries )
    {
        entry.x1 += aDx;
        entry.y1 += aDy;
        entry.x2 += aDx;
        entry.y2 += aDy;
    }

    for( ORCAD_DRAWN_INSTANCE& block : aPage.blocks )
    {
        block.x1 += aDx;
        block.y1 += aDy;

        for( ORCAD_BLOCK_PIN& pin : block.pins )
        {
            pin.x += aDx;
            pin.y += aDy;
        }
    }
}


void ORCAD_CONVERTER::applyTitleBlock( const ORCAD_RAW_PAGE& aPage, SCH_SCREEN* aScreen )
{
    for( const ORCAD_GRAPHIC_INST& tbInst : aPage.titleBlocks )
    {
        auto symbolIt = m_design.symbols.find( tbInst.name );

        if( symbolIt != m_design.symbols.end() )
        {
            placeDefinitionVectors( symbolIt->second, tbInst.bbox.x1, tbInst.bbox.y1,
                                    OrcadOrientOf( tbInst.rotation, tbInst.mirror ), aScreen );
            placeDefinitionImages( symbolIt->second, tbInst.bbox.x1, tbInst.bbox.y1,
                                   OrcadOrientOf( tbInst.rotation, tbInst.mirror ), aScreen );
        }

        if( tbInst.props.empty() )
            continue;

        auto get = [&]( const char* aKey ) -> wxString
        {
            auto it = tbInst.props.find( aKey );

            return it != tbInst.props.end() ? FromOrcadString( it->second ) : wxString();
        };

        TITLE_BLOCK titleBlock;

        titleBlock.SetTitle( get( "Title" ) );

        wxString date = get( "Page Modify Date" );

        if( date.IsEmpty() )
            date = get( "Doc Date" );

        titleBlock.SetDate( date );
        titleBlock.SetRevision( get( "RevCode" ) );
        titleBlock.SetCompany( get( "OrgName" ) );
        titleBlock.SetComment( 0, get( "Doc" ) );
        titleBlock.SetComment( 1, get( "OrgAddr1" ) );
        titleBlock.SetComment( 2, get( "OrgAddr2" ) );

        aScreen->SetTitleBlock( titleBlock );
        return;
    }
}


void ORCAD_CONVERTER::placeDefinitionVectors( const ORCAD_SYMBOL_DEF& aDefinition, int aBaseX, int aBaseY, int aOrient,
                                              SCH_SCREEN* aScreen )
{
    ORCAD_BBOX bbox = aDefinition.bbox.value_or( ORCAD_BBOX() );
    int        width = bbox.x2 - bbox.x1;
    int        height = bbox.y2 - bbox.y1;

    auto transform = [&]( int aX, int aY )
    {
        return OrcadTransformPoint( aOrient, width, height, aBaseX, aBaseY, aX, aY );
    };

    ORCAD_GRAPHIC_INST graphic;
    graphic.rotation = aOrient & 3;
    graphic.color = aDefinition.color;
    graphic.nested = std::make_unique<ORCAD_SYMBOL_DEF>();

    std::function<void( const std::vector<ORCAD_PRIMITIVE>&, int, int )> appendVectors =
            [&]( const std::vector<ORCAD_PRIMITIVE>& aPrimitives, int aOffsetX, int aOffsetY )
    {
        for( const ORCAD_PRIMITIVE& source : aPrimitives )
        {
            if( source.kind == ORCAD_PRIM_KIND::GROUP )
            {
                appendVectors( source.children, aOffsetX + source.x1, aOffsetY + source.y1 );
                continue;
            }

            if( source.kind == ORCAD_PRIM_KIND::IMAGE )
                continue;

            ORCAD_PRIMITIVE primitive = source;

            auto transformBox = [&]
            {
                std::array<VECTOR2I, 4> corners = { transform( aOffsetX + source.x1, aOffsetY + source.y1 ),
                                                    transform( aOffsetX + source.x2, aOffsetY + source.y1 ),
                                                    transform( aOffsetX + source.x2, aOffsetY + source.y2 ),
                                                    transform( aOffsetX + source.x1, aOffsetY + source.y2 ) };

                primitive.x1 = primitive.x2 = corners[0].x;
                primitive.y1 = primitive.y2 = corners[0].y;

                for( const VECTOR2I& corner : corners )
                {
                    primitive.x1 = std::min( primitive.x1, corner.x );
                    primitive.y1 = std::min( primitive.y1, corner.y );
                    primitive.x2 = std::max( primitive.x2, corner.x );
                    primitive.y2 = std::max( primitive.y2, corner.y );
                }
            };

            if( source.kind == ORCAD_PRIM_KIND::LINE )
            {
                VECTOR2I p1 = transform( aOffsetX + source.x1, aOffsetY + source.y1 );
                VECTOR2I p2 = transform( aOffsetX + source.x2, aOffsetY + source.y2 );
                primitive.x1 = p1.x;
                primitive.y1 = p1.y;
                primitive.x2 = p2.x;
                primitive.y2 = p2.y;
            }
            else
            {
                transformBox();
            }

            for( ORCAD_POINT& point : primitive.points )
            {
                VECTOR2I transformed = transform( aOffsetX + point.x, aOffsetY + point.y );
                point.x = transformed.x;
                point.y = transformed.y;
            }

            if( primitive.start )
            {
                VECTOR2I transformed = transform( aOffsetX + primitive.start->x, aOffsetY + primitive.start->y );
                primitive.start = ORCAD_POINT{ transformed.x, transformed.y };
            }

            if( primitive.end )
            {
                VECTOR2I transformed = transform( aOffsetX + primitive.end->x, aOffsetY + primitive.end->y );
                primitive.end = ORCAD_POINT{ transformed.x, transformed.y };
            }

            graphic.nested->primitives.push_back( std::move( primitive ) );
        }
    };

    appendVectors( aDefinition.primitives, 0, 0 );

    ORCAD_RAW_PAGE page;
    page.graphics.push_back( std::move( graphic ) );
    placeGraphics( page, aScreen );
}


void ORCAD_CONVERTER::placeDefinitionImages( const ORCAD_SYMBOL_DEF& aDefinition, int aBaseX, int aBaseY, int aOrient,
                                             SCH_SCREEN* aScreen )
{
    ORCAD_BBOX bbox = aDefinition.bbox.value_or( ORCAD_BBOX() );
    int        width = bbox.x2 - bbox.x1;
    int        height = bbox.y2 - bbox.y1;

    std::function<void( const std::vector<ORCAD_PRIMITIVE>&, int, int )> placeImages =
            [&]( const std::vector<ORCAD_PRIMITIVE>& aPrimitives, int aOffsetX, int aOffsetY )
    {
        for( const ORCAD_PRIMITIVE& primitive : aPrimitives )
        {
            if( primitive.kind == ORCAD_PRIM_KIND::GROUP )
            {
                placeImages( primitive.children, aOffsetX + primitive.x1, aOffsetY + primitive.y1 );
                continue;
            }

            if( primitive.kind != ORCAD_PRIM_KIND::IMAGE )
                continue;

            VECTOR2I        center = OrcadTransformPoint( aOrient, width, height, aBaseX, aBaseY,
                                                          aOffsetX + ( primitive.x1 + primitive.x2 ) / 2,
                                                          aOffsetY + ( primitive.y1 + primitive.y2 ) / 2 );
            int             imageWidth = std::abs( primitive.x2 - primitive.x1 );
            int             imageHeight = std::abs( primitive.y2 - primitive.y1 );
            ORCAD_PRIMITIVE image = primitive;
            image.x1 = center.x - imageWidth / 2;
            image.y1 = center.y - imageHeight / 2;
            image.x2 = image.x1 + imageWidth;
            image.y2 = image.y1 + imageHeight;
            placeBitmap( image, aScreen, aOrient );
        }
    };

    placeImages( aDefinition.primitives, 0, 0 );
}


void ORCAD_CONVERTER::buildNetLookup( const ORCAD_RAW_PAGE& aPage )
{
    m_wireEndpoints.clear();

    for( const ORCAD_WIRE& wire : aPage.wires )
    {
        m_wireEndpoints[{ wire.x1, wire.y1 }].push_back( &wire );
        m_wireEndpoints[{ wire.x2, wire.y2 }].push_back( &wire );
    }
}


std::string ORCAD_CONVERTER::netAt( const ORCAD_RAW_PAGE& aPage, int aX, int aY ) const
{
    auto endIt = m_wireEndpoints.find( { aX, aY } );

    const std::vector<const ORCAD_WIRE*>* endWires =
            endIt != m_wireEndpoints.end() ? &endIt->second : nullptr;

    if( endWires )
    {
        for( const ORCAD_WIRE* wire : *endWires )
        {
            auto netIt = aPage.netmap.find( wire->id );

            if( netIt != aPage.netmap.end() )
                return netIt->second;
        }
    }

    // Also try wires passing through point.
    for( const ORCAD_WIRE& wire : aPage.wires )
    {
        if( onSegment( aX, aY, wire ) )
        {
            auto netIt = aPage.netmap.find( wire.id );

            if( netIt != aPage.netmap.end() )
                return netIt->second;
        }
    }

    if( endWires )
    {
        for( const ORCAD_WIRE* wire : *endWires )
        {
            for( const ORCAD_ALIAS& alias : wire->aliases )
                return alias.name;
        }
    }

    return std::string();
}


std::string ORCAD_CONVERTER::powerNet( const ORCAD_RAW_PAGE& aPage,
                                       const ORCAD_GRAPHIC_INST& aInst ) const
{
    // Capture's Name property is the effective net name, including renamed stock power symbols.
    auto nameIt = aInst.props.find( "Name" );

    if( nameIt != aInst.props.end() && !trimmed( nameIt->second ).empty() )
        return trimmed( nameIt->second );

    if( !trimmed( aInst.logicalName ).empty() )
        return trimmed( aInst.logicalName );

    VECTOR2I    pin = graphicPinPos( aInst );
    std::string net = netAt( aPage, pin.x, pin.y );

    if( net.empty() )
    {
        // No wire touches the pin (direct pin-to-pin placement): the record's resolved
        // logical name is the net; the symbol shape name (e.g. a "VCC_BAR" graphic
        // shared by differently-named rails) is only a last resort.
        net = aInst.logicalName.empty() ? aInst.name : aInst.logicalName;
    }

    return trimmed( net );
}


VECTOR2I ORCAD_CONVERTER::graphicPinPos( const ORCAD_GRAPHIC_INST& aInst ) const
{
    auto symIt = m_design.symbols.find( aInst.name );

    if( symIt == m_design.symbols.end() || symIt->second.pins.empty() )
        return VECTOR2I( aInst.x, aInst.y );

    const ORCAD_SYMBOL_DEF& sym = symIt->second;

    int baseX = std::min( aInst.bbox.x1, aInst.bbox.x2 );
    int baseY = std::min( aInst.bbox.y1, aInst.bbox.y2 );

    ORCAD_BBOX symBox = sym.bbox.value_or( ORCAD_BBOX() );

    int width = symBox.x2 - symBox.x1;
    int height = symBox.y2 - symBox.y1;
    int orient = OrcadOrientOf( aInst.rotation, aInst.mirror );

    const ORCAD_SYMBOL_PIN& pin = sym.pins[0];

    return OrcadTransformPoint( orient, width, height, baseX, baseY, pin.hotptX, pin.hotptY );
}


std::vector<ORCAD_CONVERTER::OFFPAGE_NET>
ORCAD_CONVERTER::offpageNets( const ORCAD_RAW_PAGE& aPage ) const
{
    std::vector<OFFPAGE_NET> out;

    for( size_t i = 0; i < aPage.offpage.size(); ++i )
    {
        const ORCAD_GRAPHIC_INST& conn = aPage.offpage[i];
        VECTOR2I                  pin = graphicPinPos( conn );

        OFFPAGE_NET entry;
        entry.index = static_cast<int>( i );

        // Connector name = cross-page net identity; prefer over local wire net which
        // OrCAD may name differently (CAM0_IO1 on net GPIO19). Global label still binds wire.
        entry.net = trimmed( conn.logicalName );

        if( entry.net.empty() )
            entry.net = trimmed( netAt( aPage, pin.x, pin.y ) );

        entry.x = pin.x;
        entry.y = pin.y;

        out.push_back( entry );
    }

    return out;
}


std::vector<VECTOR2I> ORCAD_CONVERTER::computeJunctions( const ORCAD_RAW_PAGE& aPage ) const
{
    std::set<std::pair<int, int>> pinPts;

    for( const ORCAD_PLACED_INSTANCE& instance : aPage.instances )
    {
        for( const ORCAD_PIN_INST& pin : instance.pins )
            pinPts.insert( { pin.x, pin.y } );
    }

    for( const ORCAD_DRAWN_INSTANCE& block : aPage.blocks )
    {
        for( const ORCAD_BLOCK_PIN& pin : block.pins )
            pinPts.insert( { pin.x, pin.y } );
    }

    std::map<std::pair<int, int>, int> ends;

    for( const ORCAD_WIRE& wire : aPage.wires )
    {
        ends[{ wire.x1, wire.y1 }]++;
        ends[{ wire.x2, wire.y2 }]++;
    }

    std::set<std::pair<int, int>> candidates = pinPts;

    for( const std::pair<const std::pair<int, int>, int>& end : ends )
        candidates.insert( end.first );

    std::vector<VECTOR2I> out;

    for( const std::pair<int, int>& pt : candidates )
    {
        auto endIt = ends.find( pt );
        int  endCount = endIt != ends.end() ? endIt->second : 0;
        int  through = 0;

        for( const ORCAD_WIRE& wire : aPage.wires )
        {
            if( onSegment( pt.first, pt.second, wire ) )
                through++;
        }

        int pinCount = pinPts.count( pt ) ? 1 : 0;
        int score = endCount + 2 * through + pinCount;

        // Junction needed when 3+ contributions meet and at least one terminates there.
        if( score >= 3 && endCount + pinCount >= 1 && endCount + through >= 2 )
            out.emplace_back( pt.first, pt.second );
    }

    return out;
}


void ORCAD_CONVERTER::placeJunctions( const ORCAD_RAW_PAGE& aPage, SCH_SCREEN* aScreen )
{
    for( const VECTOR2I& pt : computeJunctions( aPage ) )
        aScreen->Append( new SCH_JUNCTION( OrcadDbuToIu( pt.x, pt.y ) ) );
}


void ORCAD_CONVERTER::placeWires( const ORCAD_RAW_PAGE& aPage, SCH_SCREEN* aScreen )
{
    for( const ORCAD_WIRE& wire : aPage.wires )
    {
        SCH_LINE* line = new SCH_LINE( OrcadDbuToIu( wire.x1, wire.y1 ),
                                       wire.isBus ? LAYER_BUS : LAYER_WIRE );
        line->SetEndPoint( OrcadDbuToIu( wire.x2, wire.y2 ) );

        line->SetLineWidth( OrcadLineWidthIu( wire.lineWidth ) );
        line->SetLineStyle( OrcadLineStyle( wire.lineStyle ) );

        line->SetLineColor( OrcadColor( wire.color ) );

        aScreen->Append( line );

        for( const ORCAD_ALIAS& alias : wire.aliases )
        {
            wxString text = FromOrcadString( trimmed( alias.name ) );

            if( text.IsEmpty() )
                continue;

            // Alias stores free display position; snap anchor onto wire so label binds
            // electrically.
            VECTOR2I anchor = snapToWire( alias.x, alias.y, wire );

            SCH_LABEL* label = new SCH_LABEL( OrcadDbuToIu( anchor.x, anchor.y ), text );

            // Quadrants 2/3 fold to 0/90 so text reads upright.
            int quadrant = alias.rotation & 3;

            label->SetSpinStyle( ( quadrant == 1 || quadrant == 3 ) ? SPIN_STYLE::UP
                                                                    : SPIN_STYLE::RIGHT );

            int size = textSizeIU( alias.fontIdx );
            label->SetTextSize( VECTOR2I( size, size ) );
            applyFont( label, alias.fontIdx );
            label->SetTextColor( OrcadColor( alias.color ) );

            aScreen->Append( label );
        }
    }
}


void ORCAD_CONVERTER::placeBusEntries( const ORCAD_RAW_PAGE& aPage, SCH_SCREEN* aScreen )
{
    for( const ORCAD_BUS_ENTRY& busEntry : aPage.busEntries )
    {
        SCH_BUS_WIRE_ENTRY* entry = new SCH_BUS_WIRE_ENTRY( OrcadDbuToIu( busEntry.x1,
                                                                          busEntry.y1 ) );
        entry->SetSize( VECTOR2I( ( busEntry.x2 - busEntry.x1 ) * ORCAD_IU_PER_DBU,
                                  ( busEntry.y2 - busEntry.y1 ) * ORCAD_IU_PER_DBU ) );
        entry->SetStroke( STROKE_PARAMS( 0, LINE_STYLE::DEFAULT, OrcadColor( busEntry.color ) ) );
        aScreen->Append( entry );
    }
}


void ORCAD_CONVERTER::placeNoConnects( const ORCAD_RAW_PAGE& aPage, SCH_SCREEN* aScreen )
{
    for( const ORCAD_GRAPHIC_INST& erc : aPage.ercObjects )
        aScreen->Append( new SCH_NO_CONNECT( OrcadDbuToIu( erc.x, erc.y ) ) );
}


void ORCAD_CONVERTER::placeOffpageConnectors( const ORCAD_RAW_PAGE& aPage, SCH_SCREEN* aScreen )
{
    for( const OFFPAGE_NET& offpage : offpageNets( aPage ) )
    {
        if( offpage.net.empty() )
        {
            note( wxString::Format( _( "Page '%s': the off-page connector at (%d, %d) is "
                                       "unconnected in the source design; skipped." ),
                                    FromOrcadString( aPage.name ), offpage.x, offpage.y ) );
            continue;
        }

        SCH_GLOBALLABEL* label = new SCH_GLOBALLABEL( OrcadDbuToIu( offpage.x, offpage.y ),
                                                      FromOrcadString( offpage.net ) );
        label->SetShape( LABEL_FLAG_SHAPE::L_BIDI );
        label->SetTextColor( OrcadColor( aPage.offpage[offpage.index].color ) );

        // Point label away from attached wire; vertical wire gives up/down, horizontal left/right.
        SPIN_STYLE spin = SPIN_STYLE::RIGHT;
        auto       endIt = m_wireEndpoints.find( { offpage.x, offpage.y } );

        if( endIt != m_wireEndpoints.end() && !endIt->second.empty() )
        {
            const ORCAD_WIRE* wire = endIt->second.front();
            int64_t           dx = (int64_t) wire->x1 + wire->x2 - 2LL * offpage.x;
            int64_t           dy = (int64_t) wire->y1 + wire->y2 - 2LL * offpage.y;

            if( std::abs( dx ) >= std::abs( dy ) )
                spin = dx > 0 ? SPIN_STYLE::LEFT : SPIN_STYLE::RIGHT;
            else
                spin = dy > 0 ? SPIN_STYLE::UP : SPIN_STYLE::BOTTOM;
        }

        label->SetSpinStyle( spin );

        if( SCH_FIELD* refs = label->GetField( FIELD_T::INTERSHEET_REFS ) )
            refs->SetVisible( false );

        aScreen->Append( label );
    }
}


void ORCAD_CONVERTER::placePorts( const ORCAD_RAW_PAGE& aPage, SCH_SCREEN* aScreen,
                                 bool aHierarchical )
{
    for( const ORCAD_GRAPHIC_INST& port : aPage.ports )
    {
        std::string net = trimmed( port.logicalName.empty() ? port.name : port.logicalName );

        if( net.empty() )
            continue;

        VECTOR2I pos = graphicPinPos( port );

        // Symbol name encodes arrow direction, e.g. "PORTLEFT-L".
        wxString         symbolName = FromOrcadString( port.name ).Upper();
        LABEL_FLAG_SHAPE shape = LABEL_FLAG_SHAPE::L_BIDI;

        if( symbolName.Contains( wxS( "LEFT" ) ) )
            shape = LABEL_FLAG_SHAPE::L_INPUT;
        else if( symbolName.Contains( wxS( "RIGHT" ) ) )
            shape = LABEL_FLAG_SHAPE::L_OUTPUT;

        SCH_LABEL_BASE* label;

        if( aHierarchical )
            label = new SCH_HIERLABEL( OrcadDbuToIu( pos.x, pos.y ), FromOrcadString( net ) );
        else
            label = new SCH_GLOBALLABEL( OrcadDbuToIu( pos.x, pos.y ), FromOrcadString( net ) );

        label->SetShape( shape );
        label->SetSpinStyle( SPIN_STYLE::RIGHT );
        label->SetTextColor( OrcadColor( port.color ) );

        if( SCH_GLOBALLABEL* global = dynamic_cast<SCH_GLOBALLABEL*>( label ) )
        {
            if( SCH_FIELD* refs = global->GetField( FIELD_T::INTERSHEET_REFS ) )
                refs->SetVisible( false );
        }

        aScreen->Append( label );
    }
}


void ORCAD_CONVERTER::placeGraphics( const ORCAD_RAW_PAGE& aPage, SCH_SCREEN* aScreen )
{
    for( const ORCAD_GRAPHIC_INST& gfx : aPage.graphics )
    {
        if( !gfx.nested )
            continue;

        KIGFX::COLOR4D                                          graphicColor = OrcadColor( gfx.color );
        std::function<void( const ORCAD_PRIMITIVE&, int, int )> placePrimitive =
                [&]( const ORCAD_PRIMITIVE& aSource, int aOffsetX, int aOffsetY )
        {
            if( aSource.kind == ORCAD_PRIM_KIND::GROUP )
            {
                for( const ORCAD_PRIMITIVE& child : aSource.children )
                    placePrimitive( child, aOffsetX + aSource.x1, aOffsetY + aSource.y1 );

                return;
            }

            ORCAD_PRIMITIVE prim = aSource;
            prim.x1 += aOffsetX;
            prim.y1 += aOffsetY;
            prim.x2 += aOffsetX;
            prim.y2 += aOffsetY;

            for( ORCAD_POINT& point : prim.points )
            {
                point.x += aOffsetX;
                point.y += aOffsetY;
            }

            if( prim.start )
            {
                prim.start->x += aOffsetX;
                prim.start->y += aOffsetY;
            }

            if( prim.end )
            {
                prim.end->x += aOffsetX;
                prim.end->y += aOffsetY;
            }

            switch( prim.kind )
            {
            case ORCAD_PRIM_KIND::GROUP: break;

            case ORCAD_PRIM_KIND::IMAGE: placeBitmap( prim, aScreen ); break;

            case ORCAD_PRIM_KIND::TEXT:
            {
                wxString content = FromOrcadString( prim.text );

                if( content.IsEmpty() )
                    break;

                SCH_TEXT* text = new SCH_TEXT( OrcadDbuToIu( prim.x1, prim.y1 ), content );

                // Comment text anchors at top-left of bbox.
                text->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
                text->SetVertJustify( GR_TEXT_V_ALIGN_TOP );
                text->SetMultilineAllowed( true );

                int size = textSizeIU( prim.fontIdx );
                text->SetTextSize( VECTOR2I( size, size ) );
                applyFont( text, prim.fontIdx );
                text->SetTextColor( graphicColor );

                // Quadrants 2/3 fold to horizontal so text reads upright.
                if( ( gfx.rotation & 3 ) == 1 )
                    text->SetTextAngle( ANGLE_VERTICAL );

                aScreen->Append( text );
                break;
            }

            case ORCAD_PRIM_KIND::LINE:
                aScreen->Append( makeSheetPoly( { OrcadDbuToIu( prim.x1, prim.y1 ), OrcadDbuToIu( prim.x2, prim.y2 ) },
                                                prim, graphicColor ) );
                break;

            case ORCAD_PRIM_KIND::RECT:
                aScreen->Append( makeSheetPoly( { OrcadDbuToIu( prim.x1, prim.y1 ), OrcadDbuToIu( prim.x2, prim.y1 ),
                                                  OrcadDbuToIu( prim.x2, prim.y2 ), OrcadDbuToIu( prim.x1, prim.y2 ),
                                                  OrcadDbuToIu( prim.x1, prim.y1 ) },
                                                prim, graphicColor, true ) );
                break;

            case ORCAD_PRIM_KIND::POLYLINE:
            case ORCAD_PRIM_KIND::POLYGON:
            {
                if( prim.points.size() < 2 )
                    break;

                std::vector<VECTOR2I> pts;

                for( const ORCAD_POINT& pt : prim.points )
                    pts.push_back( OrcadDbuToIu( pt.x, pt.y ) );

                if( prim.kind == ORCAD_PRIM_KIND::POLYGON )
                    pts.push_back( pts.front() );

                aScreen->Append( makeSheetPoly( pts, prim, graphicColor, prim.kind == ORCAD_PRIM_KIND::POLYGON ) );
                break;
            }

            case ORCAD_PRIM_KIND::BEZIER:
            {
                if( prim.points.size() < 4 || ( prim.points.size() - 1 ) % 3 != 0 )
                {
                    if( prim.points.size() >= 2 )
                    {
                        std::vector<VECTOR2I> pts;

                        for( const ORCAD_POINT& pt : prim.points )
                            pts.push_back( OrcadDbuToIu( pt.x, pt.y ) );

                        aScreen->Append( makeSheetPoly( pts, prim, graphicColor ) );
                    }

                    break;
                }

                for( size_t i = 0; i + 3 < prim.points.size(); i += 3 )
                {
                    SCH_SHAPE* shape = new SCH_SHAPE( SHAPE_T::BEZIER, LAYER_NOTES );
                    shape->SetPosition( OrcadDbuToIu( prim.points[i].x, prim.points[i].y ) );
                    shape->SetBezierC1( OrcadDbuToIu( prim.points[i + 1].x, prim.points[i + 1].y ) );
                    shape->SetBezierC2( OrcadDbuToIu( prim.points[i + 2].x, prim.points[i + 2].y ) );
                    shape->SetEnd( OrcadDbuToIu( prim.points[i + 3].x, prim.points[i + 3].y ) );

                    shape->SetStroke( STROKE_PARAMS( OrcadLineWidthIu( prim.lineWidth ),
                                                    OrcadLineStyle( prim.lineStyle ), graphicColor ) );
                    shape->SetFillMode( FILL_T::NO_FILL );
                    aScreen->Append( shape );
                }

                break;
            }

            case ORCAD_PRIM_KIND::ARC:
            case ORCAD_PRIM_KIND::ELLIPSE:
            {
                double cx = ( prim.x1 + prim.x2 ) / 2.0;
                double cy = ( prim.y1 + prim.y2 ) / 2.0;
                double rx = std::abs( prim.x2 - prim.x1 ) / 2.0;
                double ry = std::abs( prim.y2 - prim.y1 ) / 2.0;

                if( rx == 0.0 )
                    rx = 0.01;

                if( ry == 0.0 )
                    ry = 0.01;

                double a0 = 0.0;
                double a1 = 2.0 * M_PI;

                if( prim.kind == ORCAD_PRIM_KIND::ARC && prim.start && prim.end )
                {
                    a0 = std::atan2( ( prim.start->y - cy ) / ry, ( prim.start->x - cx ) / rx );
                    a1 = std::atan2( ( prim.end->y - cy ) / ry, ( prim.end->x - cx ) / rx );

                    // Arcs run CCW in screen (Y-down) coords.
                    if( a1 >= a0 )
                        a1 -= 2.0 * M_PI;
                }

                int steps = std::max( 8, static_cast<int>( std::abs( a1 - a0 )
                                                           / ( M_PI / 16.0 ) ) );

                std::vector<VECTOR2I> pts;

                for( int k = 0; k <= steps; ++k )
                {
                    double a = a0 + ( a1 - a0 ) * k / steps;
                    pts.push_back( dbuPointToIu( cx + rx * std::cos( a ),
                                                 cy + ry * std::sin( a ) ) );
                }

                aScreen->Append( makeSheetPoly( pts, prim, graphicColor, prim.kind == ORCAD_PRIM_KIND::ELLIPSE ) );
                break;
            }
            }
        };

        for( const ORCAD_PRIMITIVE& sourcePrimitive : gfx.nested->primitives )
            placePrimitive( sourcePrimitive, 0, 0 );
    }
}


void ORCAD_CONVERTER::placeBitmap( const ORCAD_PRIMITIVE& aPrim, SCH_SCREEN* aScreen, int aOrient )
{
    int widthDbu = std::abs( aPrim.x2 - aPrim.x1 );
    int heightDbu = std::abs( aPrim.y2 - aPrim.y1 );

    if( widthDbu < 2 || heightDbu < 2 || aPrim.data.empty() )
        return;

    wxMemoryBuffer bmpData;

    VECTOR2I center = dbuPointToIu( ( aPrim.x1 + aPrim.x2 ) / 2.0, ( aPrim.y1 + aPrim.y2 ) / 2.0 );

    std::unique_ptr<SCH_BITMAP> bitmap = std::make_unique<SCH_BITMAP>( center );
    REFERENCE_IMAGE&            refImage = bitmap->GetReferenceImage();

    bool readOk = false;

    if( MakeBmpFromDib( aPrim.data, bmpData ) )
    {
        wxLogNull noLog;
        readOk = refImage.ReadImageFile( bmpData );
    }
    else
    {
        ORCAD_OLE_PREVIEW preview = OrcadExtractOlePreview( aPrim.data );

        if( preview.type == ORCAD_OLE_PREVIEW_TYPE::BMP )
        {
            bmpData.AppendData( preview.data.data(), preview.data.size() );
            wxLogNull noLog;
            readOk = refImage.ReadImageFile( bmpData );
        }
        else if( preview.type == ORCAD_OLE_PREVIEW_TYPE::DIB && MakeBmpFromDib( preview.data, bmpData ) )
        {
            wxLogNull noLog;
            readOk = refImage.ReadImageFile( bmpData );
        }
        else if( preview.type == ORCAD_OLE_PREVIEW_TYPE::WMF )
        {
            wxImage image;
            int     maxWidth = std::clamp( widthDbu * 2, 1, 4096 );
            int     maxHeight = std::clamp( heightDbu * 2, 1, 4096 );

            if( OrcadRenderWmf( preview.data, maxWidth, maxHeight, image ) )
                readOk = refImage.SetImage( image );
        }
    }

    if( !readOk )
    {
        warn( _( "An embedded picture could not be decoded and was skipped." ) );
        return;
    }

    VECTOR2I nativeSize = refImage.GetSize();

    if( nativeSize.x > 0 && nativeSize.y > 0 )
    {
        double scaleX = static_cast<double>( widthDbu * ORCAD_IU_PER_DBU ) / nativeSize.x;
        double scaleY = static_cast<double>( heightDbu * ORCAD_IU_PER_DBU ) / nativeSize.y;

        refImage.SetImageScale( std::min( scaleX, scaleY ) );
    }

    const ORCAD_ORIENT_ENTRY& orientation = ORCAD_ORIENT_TABLE[aOrient & 7];

    if( orientation.mirror == 'x' )
        bitmap->MirrorVertically( center.y );
    else if( orientation.mirror == 'y' )
        bitmap->MirrorHorizontally( center.x );

    for( int angle = 0; angle < orientation.angle; angle += 90 )
        bitmap->Rotate( center, true );

    aScreen->Append( bitmap.release() );
}


bool ORCAD_CONVERTER::MakeBmpFromDib( const std::vector<uint8_t>& aDib, wxMemoryBuffer& aOut )
{
    // DIB = BITMAPINFOHEADER + optional palette + pixels; prepend 14-byte BITMAPFILEHEADER
    // with palette-aware pixel offset for loadable .BMP.
    if( aDib.size() < 40 )
        return false;

    auto readU16 = [&aDib]( size_t aOffset ) -> uint32_t
    {
        return static_cast<uint32_t>( aDib[aOffset] )
               | ( static_cast<uint32_t>( aDib[aOffset + 1] ) << 8 );
    };

    auto readU32 = [&aDib]( size_t aOffset ) -> uint32_t
    {
        return static_cast<uint32_t>( aDib[aOffset] )
               | ( static_cast<uint32_t>( aDib[aOffset + 1] ) << 8 )
               | ( static_cast<uint32_t>( aDib[aOffset + 2] ) << 16 )
               | ( static_cast<uint32_t>( aDib[aOffset + 3] ) << 24 );
    };

    uint32_t biSize = readU32( 0 );

    // Header size outside BITMAPINFOHEADER..BITMAPV5HEADER = not DIB (rejects OLE embeds).
    if( biSize < 40 || biSize > 200 )
        return false;

    uint32_t bitCount = readU16( 14 );
    uint32_t compression = readU32( 16 );
    uint32_t clrUsed = readU32( 32 );
    uint32_t paletteEntries = clrUsed ? clrUsed : ( bitCount <= 8 ? ( 1u << bitCount ) : 0 );
    uint32_t pixelOffset = 14 + biSize + paletteEntries * 4;

    if( compression == 3 )  // BI_BITFIELDS, three DWORD channel masks follow header
        pixelOffset += 12;

    uint32_t fileSize = 14 + static_cast<uint32_t>( aDib.size() );

    uint8_t header[14];
    header[0] = 'B';
    header[1] = 'M';
    header[2] = static_cast<uint8_t>( fileSize );
    header[3] = static_cast<uint8_t>( fileSize >> 8 );
    header[4] = static_cast<uint8_t>( fileSize >> 16 );
    header[5] = static_cast<uint8_t>( fileSize >> 24 );
    header[6] = 0;
    header[7] = 0;
    header[8] = 0;
    header[9] = 0;
    header[10] = static_cast<uint8_t>( pixelOffset );
    header[11] = static_cast<uint8_t>( pixelOffset >> 8 );
    header[12] = static_cast<uint8_t>( pixelOffset >> 16 );
    header[13] = static_cast<uint8_t>( pixelOffset >> 24 );

    aOut.AppendData( header, sizeof( header ) );
    aOut.AppendData( aDib.data(), aDib.size() );

    return true;
}


VECTOR2I ORCAD_CONVERTER::snapToWire( int aX, int aY, const ORCAD_WIRE& aWire )
{
    if( aWire.x1 == aWire.x2 )  // vertical
    {
        return VECTOR2I( aWire.x1, std::clamp( aY, std::min( aWire.y1, aWire.y2 ),
                                               std::max( aWire.y1, aWire.y2 ) ) );
    }

    if( aWire.y1 == aWire.y2 )  // horizontal
    {
        return VECTOR2I( std::clamp( aX, std::min( aWire.x1, aWire.x2 ),
                                     std::max( aWire.x1, aWire.x2 ) ),
                         aWire.y1 );
    }

    // diagonal, project onto segment
    double dx = aWire.x2 - aWire.x1;
    double dy = aWire.y2 - aWire.y1;
    double t = ( ( aX - aWire.x1 ) * dx + ( aY - aWire.y1 ) * dy ) / ( dx * dx + dy * dy );

    t = std::clamp( t, 0.0, 1.0 );

    return VECTOR2I( KiROUND( aWire.x1 + t * dx ), KiROUND( aWire.y1 + t * dy ) );
}


bool ORCAD_CONVERTER::onSegment( int aX, int aY, const ORCAD_WIRE& aWire )
{
    if( ( aX == aWire.x1 && aY == aWire.y1 ) || ( aX == aWire.x2 && aY == aWire.y2 ) )
        return false;

    if( aWire.x1 == aWire.x2 && aWire.x1 == aX )
        return std::min( aWire.y1, aWire.y2 ) < aY && aY < std::max( aWire.y1, aWire.y2 );

    if( aWire.y1 == aWire.y2 && aWire.y1 == aY )
        return std::min( aWire.x1, aWire.x2 ) < aX && aX < std::max( aWire.x1, aWire.x2 );

    return false;
}
