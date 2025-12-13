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

#include <board_design_settings.h>
#include <charconv>
#include <layer_ids.h>
#include <lset.h>
#include <string_utils.h>
#include <pcb_plot_params.h>
#include <pcb_plot_params_parser.h>
#include <plotters/plotter.h>
#include <io/kicad/kicad_io_utils.h>
#include <settings/color_settings.h>
#include <lseq.h>


#define PLOT_LINEWIDTH_DEFAULT    ( DEFAULT_TEXT_WIDTH * IU_PER_MM )

#define SVG_PRECISION_MIN         3U
#define SVG_PRECISION_MAX         6U
#define SVG_PRECISION_DEFAULT     4


// default trailing digits in Gerber coordinates, when units are mm
// This is also the max usable precision (i.e. internal Pcbnew Units)
static const int gbrDefaultPrecision = 6;


using namespace PCBPLOTPARAMS_T;


static const char* getTokenName( T aTok )
{
    return PCB_PLOT_PARAMS_LEXER::TokenName( aTok );
}


PCB_PLOT_PARAMS::PCB_PLOT_PARAMS()
{
    m_useGerberProtelExtensions  = false;
    m_gerberDisableApertMacros   = false;
    m_useGerberX2format          = true;
    m_includeGerberNetlistInfo   = true;
    m_createGerberJobFile        = true;
    m_gerberPrecision            = gbrDefaultPrecision;
    m_dashedLineDashRatio        = 12.0;   // From ISO 128-2
    m_dashedLineGapRatio         = 3.0;    // From ISO 128-2

    // we used 0.1mils for SVG step before, but nm precision is more accurate, so we use nm
    m_svgPrecision               = SVG_PRECISION_DEFAULT;
    m_svgFitPageToBoard          = false;
    m_plotDrawingSheet           = false;
    m_DXFPlotMode                = FILLED;
    m_DXFPolygonMode             = true;
    m_DXFUnits                   = DXF_UNITS::INCH;
    m_useAuxOrigin               = false;
    m_negative                   = false;
    m_A4Output                   = false;
    m_plotReference              = true;
    m_plotValue                  = true;
    m_plotFPText                 = true;
    m_sketchPadsOnFabLayers      = false;
    m_hideDNPFPsOnFabLayers      = false;
    m_sketchDNPFPsOnFabLayers    = true;
    m_crossoutDNPFPsOnFabLayers  = true;
    m_plotPadNumbers             = false;
    m_subtractMaskFromSilk       = false;
    m_format                     = PLOT_FORMAT::GERBER;
    m_mirror                     = false;
    m_drillMarks                 = DRILL_MARKS::SMALL_DRILL_SHAPE;
    m_autoScale                  = false;
    m_scale                      = 1.0;
    m_scaleSelection             = 1;
    m_fineScaleAdjustX           = 1.0;
    m_fineScaleAdjustY           = 1.0;
    m_widthAdjust                = 0.;
    m_textMode                   = PLOT_TEXT_MODE::DEFAULT;
    m_outputDirectory.clear();
    m_layerSelection             = LSET( { F_SilkS, B_SilkS, F_Mask, B_Mask, F_Paste, B_Paste, Edge_Cuts } )
                                         | LSET::AllCuMask();

    m_PDFFrontFPPropertyPopups   = true;
    m_PDFBackFPPropertyPopups    = true;
    m_PDFMetadata                = true;
    m_PDFSingle                  = false;
    m_PDFBackgroundColor         = COLOR4D::UNSPECIFIED;

    // This parameter controls if the NPTH pads will be plotted or not
    // it is a "local" parameter
    m_skipNPTH_Pads                 = false;

    // line width to plot items in outline mode.
    m_sketchPadLineWidth         = pcbIUScale.mmToIU( 0.1 );

    m_default_colors             = std::make_shared<COLOR_SETTINGS>();
    m_colors                     = m_default_colors.get();

    m_blackAndWhite              = true;

    m_DXFExportAsMultiLayeredFile = false;
}


void PCB_PLOT_PARAMS::SetGerberPrecision( int aPrecision )
{
    // Currently Gerber files use mm.
    // accepted precision is only 6 (max value, this is the resolution of Pcbnew)
    // or 5, min value for professional boards, when 6 creates problems
    // to board makers.

    m_gerberPrecision = aPrecision == gbrDefaultPrecision-1 ? gbrDefaultPrecision-1 :
                                      gbrDefaultPrecision;
}


void PCB_PLOT_PARAMS::SetSvgPrecision( unsigned aPrecision )
{
    m_svgPrecision = std::clamp( aPrecision, SVG_PRECISION_MIN, SVG_PRECISION_MAX );
}


void PCB_PLOT_PARAMS::Format( OUTPUTFORMATTER* aFormatter ) const
{
    aFormatter->Print( "(pcbplotparams" );

    aFormatter->Print( "(layerselection 0x%s)", m_layerSelection.FmtHex().c_str() );

    LSET commonLayers;

    for( PCB_LAYER_ID commonLayer : m_plotOnAllLayersSequence )
        commonLayers.set( commonLayer );

    aFormatter->Print( "(plot_on_all_layers_selection 0x%s)", commonLayers.FmtHex().c_str() );

    KICAD_FORMAT::FormatBool( aFormatter, "disableapertmacros", m_gerberDisableApertMacros );
    KICAD_FORMAT::FormatBool( aFormatter, "usegerberextensions", m_useGerberProtelExtensions );
    KICAD_FORMAT::FormatBool( aFormatter, "usegerberattributes", GetUseGerberX2format() );
    KICAD_FORMAT::FormatBool( aFormatter, "usegerberadvancedattributes", GetIncludeGerberNetlistInfo() );
    KICAD_FORMAT::FormatBool( aFormatter, "creategerberjobfile", GetCreateGerberJobFile() );

    // save this option only if it is not the default value,
    // to avoid incompatibility with older Pcbnew version
    if( m_gerberPrecision != gbrDefaultPrecision )
        aFormatter->Print( "(gerberprecision %d)", m_gerberPrecision );

    aFormatter->Print( "(dashed_line_dash_ratio %s)", FormatDouble2Str( GetDashedLineDashRatio() ).c_str() );
    aFormatter->Print( "(dashed_line_gap_ratio %s)", FormatDouble2Str( GetDashedLineGapRatio() ).c_str() );

    // SVG options
    aFormatter->Print( "(svgprecision %d)", m_svgPrecision );

    KICAD_FORMAT::FormatBool( aFormatter, "plotframeref", m_plotDrawingSheet );
    aFormatter->Print( "(mode %d)", GetDXFPlotMode() == SKETCH ? 2 : 1 );
    KICAD_FORMAT::FormatBool( aFormatter, "useauxorigin", m_useAuxOrigin );

    // PDF options
    KICAD_FORMAT::FormatBool( aFormatter, getTokenName( T_pdf_front_fp_property_popups ),
                              m_PDFFrontFPPropertyPopups );
    KICAD_FORMAT::FormatBool( aFormatter, getTokenName( T_pdf_back_fp_property_popups ),
                              m_PDFBackFPPropertyPopups );
    KICAD_FORMAT::FormatBool( aFormatter, getTokenName( T_pdf_metadata ), m_PDFMetadata );
    KICAD_FORMAT::FormatBool( aFormatter, getTokenName( T_pdf_single_document ), m_PDFSingle );

    // DXF options
    KICAD_FORMAT::FormatBool( aFormatter, getTokenName( T_dxfpolygonmode ), m_DXFPolygonMode );
    KICAD_FORMAT::FormatBool( aFormatter, getTokenName( T_dxfimperialunits ),
                              m_DXFUnits == DXF_UNITS::INCH );
    KICAD_FORMAT::FormatBool( aFormatter, getTokenName( T_dxfusepcbnewfont ),
                              m_textMode != PLOT_TEXT_MODE::NATIVE );

    KICAD_FORMAT::FormatBool( aFormatter, getTokenName( T_psnegative ), m_negative );
    KICAD_FORMAT::FormatBool( aFormatter, getTokenName( T_psa4output ), m_A4Output );

    KICAD_FORMAT::FormatBool( aFormatter, getTokenName( T_plot_black_and_white ), m_blackAndWhite );

    KICAD_FORMAT::FormatBool( aFormatter, "sketchpadsonfab", m_sketchPadsOnFabLayers );
    KICAD_FORMAT::FormatBool( aFormatter, "plotpadnumbers", m_plotPadNumbers );
    KICAD_FORMAT::FormatBool( aFormatter, "hidednponfab", m_hideDNPFPsOnFabLayers );
    KICAD_FORMAT::FormatBool( aFormatter, "sketchdnponfab", m_sketchDNPFPsOnFabLayers );
    KICAD_FORMAT::FormatBool( aFormatter, "crossoutdnponfab", m_crossoutDNPFPsOnFabLayers );
    KICAD_FORMAT::FormatBool( aFormatter, "subtractmaskfromsilk", m_subtractMaskFromSilk );
    aFormatter->Print( "(outputformat %d)", static_cast<int>( m_format ) );
    KICAD_FORMAT::FormatBool( aFormatter, "mirror", m_mirror );
    aFormatter->Print( "(drillshape %d)", (int)m_drillMarks );
    aFormatter->Print( "(scaleselection %d)", m_scaleSelection );
    aFormatter->Print( "(outputdirectory %s)", aFormatter->Quotew( m_outputDirectory ).c_str() );
    aFormatter->Print( ")" );
}


void PCB_PLOT_PARAMS::Parse( PCB_PLOT_PARAMS_PARSER* aParser )
{
    aParser->Parse( this );
}


bool PCB_PLOT_PARAMS::IsSameAs( const PCB_PLOT_PARAMS &aPcbPlotParams ) const
{
    if( m_layerSelection != aPcbPlotParams.m_layerSelection )
        return false;

    if( m_plotOnAllLayersSequence != aPcbPlotParams.m_plotOnAllLayersSequence )
        return false;

    if( m_useGerberProtelExtensions != aPcbPlotParams.m_useGerberProtelExtensions )
        return false;

    if( m_gerberDisableApertMacros != aPcbPlotParams.m_gerberDisableApertMacros )
        return false;

    if( m_useGerberX2format != aPcbPlotParams.m_useGerberX2format )
        return false;

    if( m_includeGerberNetlistInfo != aPcbPlotParams.m_includeGerberNetlistInfo )
        return false;

    if( m_createGerberJobFile != aPcbPlotParams.m_createGerberJobFile )
        return false;

    if( m_gerberPrecision != aPcbPlotParams.m_gerberPrecision )
        return false;

    if( m_dashedLineDashRatio != aPcbPlotParams.m_dashedLineDashRatio )
        return false;

    if( m_dashedLineGapRatio != aPcbPlotParams.m_dashedLineGapRatio )
        return false;

    if( m_plotDrawingSheet != aPcbPlotParams.m_plotDrawingSheet )
        return false;

    if( m_DXFPlotMode != aPcbPlotParams.m_DXFPlotMode )
        return false;

    if( m_DXFPolygonMode != aPcbPlotParams.m_DXFPolygonMode )
        return false;

    if( m_DXFUnits != aPcbPlotParams.m_DXFUnits )
        return false;

    if( m_svgPrecision != aPcbPlotParams.m_svgPrecision )
        return false;

    if( m_useAuxOrigin != aPcbPlotParams.m_useAuxOrigin )
        return false;

    if( m_negative != aPcbPlotParams.m_negative )
        return false;

    if( m_PDFFrontFPPropertyPopups != aPcbPlotParams.m_PDFFrontFPPropertyPopups )
        return false;

    if( m_PDFBackFPPropertyPopups != aPcbPlotParams.m_PDFBackFPPropertyPopups )
        return false;

    if( m_PDFMetadata != aPcbPlotParams.m_PDFMetadata )
        return false;

    if( m_A4Output != aPcbPlotParams.m_A4Output )
        return false;

    if( m_plotReference != aPcbPlotParams.m_plotReference )
        return false;

    if( m_plotValue != aPcbPlotParams.m_plotValue )
        return false;

    if( m_plotFPText != aPcbPlotParams.m_plotFPText )
        return false;

    if( m_sketchPadsOnFabLayers != aPcbPlotParams.m_sketchPadsOnFabLayers )
        return false;

    if( m_plotPadNumbers != aPcbPlotParams.m_plotPadNumbers )
        return false;

    if( m_hideDNPFPsOnFabLayers != aPcbPlotParams.m_hideDNPFPsOnFabLayers )
        return false;

    if( m_sketchDNPFPsOnFabLayers != aPcbPlotParams.m_sketchDNPFPsOnFabLayers )
        return false;

    if( m_crossoutDNPFPsOnFabLayers != aPcbPlotParams.m_crossoutDNPFPsOnFabLayers )
        return false;

    if( m_subtractMaskFromSilk != aPcbPlotParams.m_subtractMaskFromSilk )
        return false;

    if( m_format != aPcbPlotParams.m_format )
        return false;

    if( m_mirror != aPcbPlotParams.m_mirror )
        return false;

    if( m_drillMarks != aPcbPlotParams.m_drillMarks )
        return false;

    if( m_scaleSelection != aPcbPlotParams.m_scaleSelection )
        return false;

    if( m_autoScale != aPcbPlotParams.m_autoScale )
        return false;

    if( m_scale != aPcbPlotParams.m_scale )
        return false;

    if( m_fineScaleAdjustX != aPcbPlotParams.m_fineScaleAdjustX )
        return false;

    if( m_fineScaleAdjustY != aPcbPlotParams.m_fineScaleAdjustY )
        return false;

    if( m_widthAdjust != aPcbPlotParams.m_widthAdjust )
        return false;

    if( m_textMode != aPcbPlotParams.m_textMode )
        return false;

    if( m_blackAndWhite != aPcbPlotParams.m_blackAndWhite )
        return false;

    if( !m_outputDirectory.IsSameAs( aPcbPlotParams.m_outputDirectory ) )
        return false;

    if( m_DXFExportAsMultiLayeredFile != aPcbPlotParams.m_DXFExportAsMultiLayeredFile )
        return false;

    return true;
}


PCB_PLOT_PARAMS_PARSER::PCB_PLOT_PARAMS_PARSER( LINE_READER* aReader, int aBoardFileVersion ) :
    PCB_PLOT_PARAMS_LEXER( aReader ),
    m_boardFileVersion( aBoardFileVersion )
{
}


PCB_PLOT_PARAMS_PARSER::PCB_PLOT_PARAMS_PARSER( char* aLine, const wxString& aSource ) :
    PCB_PLOT_PARAMS_LEXER( aLine, aSource ),
    m_boardFileVersion( 0 )
{
}


/**
 * These are the layer IDs from before 5e0abadb23425765e164f49ee2f893e94ddb97fc,
 * and are needed for mapping old PCB files to the new layer numbering.
 */
enum LEGACY_PCB_LAYER_ID: int
{
    LEGACY_UNDEFINED_LAYER = -1,
    LEGACY_UNSELECTED_LAYER = -2,

    LEGACY_F_Cu = 0,
    LEGACY_In1_Cu,
    LEGACY_In2_Cu,
    LEGACY_In3_Cu,
    LEGACY_In4_Cu,
    LEGACY_In5_Cu,
    LEGACY_In6_Cu,
    LEGACY_In7_Cu,
    LEGACY_In8_Cu,
    LEGACY_In9_Cu,
    LEGACY_In10_Cu,
    LEGACY_In11_Cu,
    LEGACY_In12_Cu,
    LEGACY_In13_Cu,
    LEGACY_In14_Cu,
    LEGACY_In15_Cu,
    LEGACY_In16_Cu,
    LEGACY_In17_Cu,
    LEGACY_In18_Cu,
    LEGACY_In19_Cu,
    LEGACY_In20_Cu,
    LEGACY_In21_Cu,
    LEGACY_In22_Cu,
    LEGACY_In23_Cu,
    LEGACY_In24_Cu,
    LEGACY_In25_Cu,
    LEGACY_In26_Cu,
    LEGACY_In27_Cu,
    LEGACY_In28_Cu,
    LEGACY_In29_Cu,
    LEGACY_In30_Cu,
    LEGACY_B_Cu,           // 31

    LEGACY_B_Adhes,
    LEGACY_F_Adhes,

    LEGACY_B_Paste,
    LEGACY_F_Paste,

    LEGACY_B_SilkS,
    LEGACY_F_SilkS,

    LEGACY_B_Mask,
    LEGACY_F_Mask,         // 39

    LEGACY_Dwgs_User,
    LEGACY_Cmts_User,
    LEGACY_Eco1_User,
    LEGACY_Eco2_User,
    LEGACY_Edge_Cuts,
    LEGACY_Margin,         // 45

    LEGACY_B_CrtYd,
    LEGACY_F_CrtYd,

    LEGACY_B_Fab,
    LEGACY_F_Fab,          // 49

    // User definable layers.
    LEGACY_User_1,
    LEGACY_User_2,
    LEGACY_User_3,
    LEGACY_User_4,
    LEGACY_User_5,
    LEGACY_User_6,
    LEGACY_User_7,
    LEGACY_User_8,
    LEGACY_User_9,

    LEGACY_Rescue,         // 59

    // Four reserved layers (60 - 63) for future expansion within the 64 bit integer limit.

    LEGACY_PCB_LAYER_ID_COUNT
};

/*
 * Mapping to translate a legacy layer ID into the new PCB layer IDs.
 */
static const std::map<LEGACY_PCB_LAYER_ID, PCB_LAYER_ID> s_legacyLayerIdMap{
    {LEGACY_F_Cu,      F_Cu},
    {LEGACY_B_Cu,      B_Cu},
    {LEGACY_In1_Cu,    In1_Cu},
    {LEGACY_In2_Cu,    In2_Cu},
    {LEGACY_In3_Cu,    In3_Cu},
    {LEGACY_In4_Cu,    In4_Cu},
    {LEGACY_In5_Cu,    In5_Cu},
    {LEGACY_In6_Cu,    In6_Cu},
    {LEGACY_In7_Cu,    In7_Cu},
    {LEGACY_In8_Cu,    In8_Cu},
    {LEGACY_In9_Cu,    In9_Cu},
    {LEGACY_In10_Cu,   In10_Cu},
    {LEGACY_In11_Cu,   In11_Cu},
    {LEGACY_In12_Cu,   In12_Cu},
    {LEGACY_In13_Cu,   In13_Cu},
    {LEGACY_In14_Cu,   In14_Cu},
    {LEGACY_In15_Cu,   In15_Cu},
    {LEGACY_In16_Cu,   In16_Cu},
    {LEGACY_In17_Cu,   In17_Cu},
    {LEGACY_In18_Cu,   In18_Cu},
    {LEGACY_In19_Cu,   In19_Cu},
    {LEGACY_In20_Cu,   In20_Cu},
    {LEGACY_In21_Cu,   In21_Cu},
    {LEGACY_In22_Cu,   In22_Cu},
    {LEGACY_In23_Cu,   In23_Cu},
    {LEGACY_In24_Cu,   In24_Cu},
    {LEGACY_In25_Cu,   In25_Cu},
    {LEGACY_In26_Cu,   In26_Cu},
    {LEGACY_In27_Cu,   In27_Cu},
    {LEGACY_In28_Cu,   In28_Cu},
    {LEGACY_In29_Cu,   In29_Cu},
    {LEGACY_In30_Cu,   In30_Cu},
    {LEGACY_F_Mask,    F_Mask},
    {LEGACY_B_Mask,    B_Mask},
    {LEGACY_F_SilkS,   F_SilkS},
    {LEGACY_B_SilkS,   B_SilkS},
    {LEGACY_F_Adhes,   F_Adhes},
    {LEGACY_B_Adhes,   B_Adhes},
    {LEGACY_F_Paste,   F_Paste},
    {LEGACY_B_Paste,   B_Paste},
    {LEGACY_Dwgs_User, Dwgs_User},
    {LEGACY_Cmts_User, Cmts_User},
    {LEGACY_Eco1_User, Eco1_User},
    {LEGACY_Eco2_User, Eco2_User},
    {LEGACY_Edge_Cuts, Edge_Cuts},
    {LEGACY_Margin,    Margin},
    {LEGACY_B_CrtYd,   B_CrtYd},
    {LEGACY_F_CrtYd,   F_CrtYd},
    {LEGACY_B_Fab,     B_Fab},
    {LEGACY_F_Fab,     F_Fab},
    {LEGACY_User_1,    User_1},
    {LEGACY_User_2,    User_2},
    {LEGACY_User_3,    User_3},
    {LEGACY_User_4,    User_4},
    {LEGACY_User_5,    User_5},
    {LEGACY_User_6,    User_6},
    {LEGACY_User_7,    User_7},
    {LEGACY_User_8,    User_8},
    {LEGACY_User_9,    User_9},
    {LEGACY_Rescue,    Rescue},
};


LSET remapLegacyLayerLSET( const BASE_SET& aLegacyLSET )
{
    LSET newLayers;

    for( const auto& [legacyLayer, newLayer] : s_legacyLayerIdMap )
        newLayers[newLayer] = aLegacyLSET[legacyLayer];

    return newLayers;
}


void PCB_PLOT_PARAMS_PARSER::Parse( PCB_PLOT_PARAMS* aPcbPlotParams )
{
    T   token;

    while( ( token = NextTok() ) != T_RIGHT )
    {
        if( token == T_EOF)
            Unexpected( T_EOF );

        if( token == T_LEFT )
            token = NextTok();

        if( token == T_pcbplotparams )
            continue;

        bool skip_right = false;

        switch( token )
        {
        case T_layerselection:
        {
            token = NeedSYMBOLorNUMBER();

            const std::string& cur = CurStr();

            if( token == T_NUMBER ) // pretty 3 format had legacy Cu stack.
            {
                //  It's not possible to convert a legacy Cu layer number to a new Cu layer
                //  number without knowing the number or total Cu layers in the legacy board.
                //  We do not have that information here, so simply set all layers ON.  User
                //  can turn them off in the UI.
                aPcbPlotParams->m_layerSelection = LSET( { F_SilkS, B_SilkS } ) | LSET::AllCuMask();
            }
            else if( cur.find_first_of( "0x" ) == 0 ) // pretty ver. 4.
            {
                // The layers were renumbered in 5e0abadb23425765e164f49ee2f893e94ddb97fc, but there wasn't
                // a board file version change with it, so this value is the one immediately after that happened.
                if( m_boardFileVersion < 20240819 )
                {
                    BASE_SET legacyLSET( LEGACY_PCB_LAYER_ID_COUNT );

                    // skip the leading 2 0x bytes.
                    legacyLSET.ParseHex( cur.c_str() + 2, cur.size() - 2 );
                    aPcbPlotParams->SetLayerSelection( remapLegacyLayerLSET( legacyLSET ) );
                }
                else
                {
                    // skip the leading 2 0x bytes.
                    aPcbPlotParams->m_layerSelection.ParseHex( cur.c_str() + 2, cur.size() - 2 );
                }
            }
            else
            {
                Expecting( "integer or hex layerSelection" );
            }

            break;
        }

        case T_plot_on_all_layers_selection:
        {
            token = NeedSYMBOLorNUMBER();

            const std::string& cur = CurStr();

            if( cur.find_first_of( "0x" ) == 0 )
            {
                LSET layers;

                // The layers were renumbered in 5e0abadb23425765e164f49ee2f893e94ddb97fc, but
                // there wasn't a board file version change with it, so this value is the one
                // immediately after that happened.
                if( m_boardFileVersion < 20240819 )
                {
                    BASE_SET legacyLSET( LEGACY_PCB_LAYER_ID_COUNT );

                    // skip the leading 2 0x bytes.
                    legacyLSET.ParseHex( cur.c_str() + 2, cur.size() - 2 );

                    layers = remapLegacyLayerLSET( legacyLSET );
                }
                else
                {
                    // skip the leading 2 0x bytes.
                    layers.ParseHex( cur.c_str() + 2, cur.size() - 2 );
                }

                aPcbPlotParams->SetPlotOnAllLayersSequence( layers.SeqStackupForPlotting() );
            }
            else
            {
                Expecting( "hex plot_on_all_layers_selection" );
            }

            break;
        }

        case T_disableapertmacros:
            aPcbPlotParams->m_gerberDisableApertMacros = parseBool();
            break;

        case T_usegerberextensions:
            aPcbPlotParams->m_useGerberProtelExtensions = parseBool();
            break;

        case T_usegerberattributes:
            aPcbPlotParams->m_useGerberX2format = parseBool();
            break;

        case T_usegerberadvancedattributes:
            aPcbPlotParams->m_includeGerberNetlistInfo = parseBool();
            break;

        case T_creategerberjobfile:
            aPcbPlotParams->m_createGerberJobFile = parseBool();
            break;

        case T_gerberprecision:
            aPcbPlotParams->m_gerberPrecision = parseInt( gbrDefaultPrecision - 1,
                                                          gbrDefaultPrecision);
            break;

        case T_dashed_line_dash_ratio:
            aPcbPlotParams->m_dashedLineDashRatio = parseDouble();
            break;

        case T_dashed_line_gap_ratio:
            aPcbPlotParams->m_dashedLineGapRatio = parseDouble();
            break;

        case T_svgprecision:
            aPcbPlotParams->m_svgPrecision = parseInt( SVG_PRECISION_MIN, SVG_PRECISION_MAX );
            break;

        case T_svguseinch:
            parseBool();    // Unused. For compatibility
            break;

        case T_psa4output:
            aPcbPlotParams->m_A4Output = parseBool();
            break;

        case T_excludeedgelayer:
            if( !parseBool() )
                aPcbPlotParams->m_plotOnAllLayersSequence.push_back( Edge_Cuts );

            break;

        case T_plotframeref:
            aPcbPlotParams->m_plotDrawingSheet = parseBool();
            break;

        case T_viasonmask:
            aPcbPlotParams->m_plotViaOnMaskLayer = parseBool();
            break;

        case T_useauxorigin:
            aPcbPlotParams->m_useAuxOrigin = parseBool();
            break;

        case T_mode:
        case T_hpglpennumber:
        case T_hpglpenspeed:
        case T_hpglpenoverlay:
            // HPGL is no longer supported
            parseInt( std::numeric_limits<int>::min(), std::numeric_limits<int>::max() );
            break;

        case T_pdf_front_fp_property_popups:
            aPcbPlotParams->m_PDFFrontFPPropertyPopups = parseBool();
            break;

        case T_pdf_back_fp_property_popups:
            aPcbPlotParams->m_PDFBackFPPropertyPopups = parseBool();
            break;

        case T_pdf_metadata:
            aPcbPlotParams->m_PDFMetadata = parseBool();
            break;

        case T_pdf_single_document:
            aPcbPlotParams->m_PDFSingle = parseBool();
            break;

        case T_dxfpolygonmode:
            aPcbPlotParams->m_DXFPolygonMode = parseBool();
            break;

        case T_dxfimperialunits:
            aPcbPlotParams->m_DXFUnits = parseBool() ? DXF_UNITS::INCH : DXF_UNITS::MM;
            break;

        case T_dxfusepcbnewfont:
            aPcbPlotParams->m_textMode = parseBool() ? PLOT_TEXT_MODE::DEFAULT
                                                     : PLOT_TEXT_MODE::NATIVE;
            break;

        case T_pscolor:
            NeedSYMBOL(); // This actually was never used...
            break;

        case T_psnegative:
            aPcbPlotParams->m_negative = parseBool();
            break;

        case T_plot_black_and_white:
            aPcbPlotParams->m_blackAndWhite = parseBool();
            break;

        case T_plotinvisibletext:   // legacy token; no longer supported
            parseBool();
            break;

        case T_sketchpadsonfab:
            aPcbPlotParams->m_sketchPadsOnFabLayers= parseBool();
            break;

        case T_plotpadnumbers:
            aPcbPlotParams->m_plotPadNumbers = parseBool();
            break;

        case T_hidednponfab:
            aPcbPlotParams->m_hideDNPFPsOnFabLayers = parseBool();
            break;

        case T_sketchdnponfab:
            aPcbPlotParams->m_sketchDNPFPsOnFabLayers = parseBool();
            break;

        case T_crossoutdnponfab:
            aPcbPlotParams->m_crossoutDNPFPsOnFabLayers = parseBool();
            break;

        case T_subtractmaskfromsilk:
            aPcbPlotParams->m_subtractMaskFromSilk = parseBool();
            break;

        case T_outputformat:
            aPcbPlotParams->m_format = static_cast<PLOT_FORMAT>(
                    parseInt( static_cast<int>( PLOT_FORMAT::FIRST_FORMAT ),
                             static_cast<int>( PLOT_FORMAT::LAST_FORMAT ) ) );
            break;

        case T_mirror:
            aPcbPlotParams->m_mirror = parseBool();
            break;

        case T_drillshape:
            aPcbPlotParams->m_drillMarks = static_cast<DRILL_MARKS> ( parseInt( 0, 2 ) );
            break;

        case T_scaleselection:
            aPcbPlotParams->m_scaleSelection = parseInt( 0, 4 );
            break;

        case T_outputdirectory:
            NeedSYMBOLorNUMBER();   // a dir name can be like a number
            aPcbPlotParams->m_outputDirectory = From_UTF8( CurText() );
            break;

        default:
            skipCurrent();      // skip unknown or outdated plot parameter
            skip_right = true;  // the closing right token is already read.
            break;
        }

        if( ! skip_right )
            NeedRIGHT();
    }
}


bool PCB_PLOT_PARAMS_PARSER::parseBool()
{
    T token = NeedSYMBOL();

    switch( token )
    {
    case T_false:
    case T_no:
        return false;

    case T_true:
    case T_yes:
        return true;

    default:
        Expecting( "true, false, yes, or no" );
        return false;
    }
}


int PCB_PLOT_PARAMS_PARSER::parseInt( int aMin, int aMax )
{
    T token = NextTok();

    if( token != T_NUMBER )
        Expecting( T_NUMBER );

    int val = atoi( CurText() );

    if( val < aMin )
        val = aMin;
    else if( val > aMax )
        val = aMax;

    return val;
}


double PCB_PLOT_PARAMS_PARSER::parseDouble()
{
    T token = NextTok();

    if( token != T_NUMBER )
        Expecting( T_NUMBER );

    return DSNLEXER::parseDouble();
}


void PCB_PLOT_PARAMS_PARSER::skipCurrent()
{
    int curr_level = 0;
    T token;

    while( ( token = NextTok() ) != T_EOF )
    {
        if( token == T_LEFT )
            curr_level--;

        if( token == T_RIGHT )
        {
            curr_level++;

            if( curr_level > 0 )
                return;
        }
    }
}
