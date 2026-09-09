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

/** Coordinates use 10-mil DBU with Y down. KiCad library objects also use Y down. */

#ifndef ORCAD_CONVERTER_H_
#define ORCAD_CONVERTER_H_

#include <cstdint>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include <wx/string.h>

#include <geometry/seg.h>
#include <math/box2.h>
#include <math/vector2d.h>
#include <gal/color4d.h>
#include <eda_shape.h>
#include <kiid.h>

#include <sch_io/orcad/orcad_records.h>

class LIB_SYMBOL;
class SCH_LINE;
class EDA_TEXT;
class PROGRESS_REPORTER;
class REPORTER;
class SCHEMATIC;
class SCH_ITEM;
class SCH_LABEL_BASE;
class SCH_LABEL;
class SCH_SCREEN;
class SCH_SHEET;
class SCH_SHEET_PIN;
class SCH_SHEET_PATH;
class SCH_SYMBOL;
class SCH_TEXT;
class wxMemoryBuffer;


/** Schematic internal units per OrCAD DBU: 10 mil * 254 IU/mil. */
inline constexpr int ORCAD_IU_PER_DBU = 2540;

KIGFX::COLOR4D OrcadColor( int aColorIndex );
int            OrcadLineWidthIu( int aWidth );
int            OrcadPageGraphicLineWidthIu( int aWidth );
LINE_STYLE     OrcadLineStyle( int aStyle );
std::pair<double, double> OrcadDashRatios( int aFormatVersionMajor );
FILL_T         OrcadFillType( int aFillStyle, int aHatchStyle );
int            OrcadHatchPitchIu( uint32_t aModifyTimestamp );
int            OrcadHatchLineWidthIu( uint32_t aModifyTimestamp );
std::vector<SEG> OrcadHatchLines( const EDA_SHAPE& aShape, int aHatchStyle, int aPitch );
int            OrcadTextBaselineOffset( int aTextSize );
int            OrcadDisplayType( const ORCAD_DISPLAY_PROP& aProp );
bool           OrcadDisplayPropVisible( const ORCAD_DISPLAY_PROP& aProp );
bool           OrcadDisplayPropShowsName( const ORCAD_DISPLAY_PROP& aProp );
bool           OrcadDisplayPropShowsValue( const ORCAD_DISPLAY_PROP& aProp );
int            OrcadDisplayFontId( const ORCAD_DISPLAY_PROP& aProp );
int            OrcadPageOrder( wxString& aName );
VECTOR2I       OrcadStretchedImageSize( int aWidth, int aHeight, int aBoxWidth, int aBoxHeight );
wxString       OrcadPinNameMarkup( const wxString& aName );

inline VECTOR2I OrcadDbuToIu( int aX, int aY )
{
    return VECTOR2I( aX * ORCAD_IU_PER_DBU, aY * ORCAD_IU_PER_DBU );
}


/** OrCAD rotates about the bounding box; KiCad rotates about the anchor.
 * Offset selectors use 0 for zero, 1 for width, and 2 for height. */
struct ORCAD_ORIENT_ENTRY
{
    int    angle;  ///< KiCad placement angle in degrees (0/90/180/270, CCW)
    char   mirror; ///< 0 = none, 'x' or 'y' = KiCad mirror axis
    int8_t txSel;  ///< X offset selector
    int8_t tySel;  ///< Y offset selector
    int8_t a;
    int8_t b;
    int8_t c;
    int8_t d;
};


/** Indexed by orientation bits: angle, mirror, offset selectors, then matrix coefficients. */
inline constexpr ORCAD_ORIENT_ENTRY ORCAD_ORIENT_TABLE[8] = {
    { 0, 0, 0, 0, 1, 0, 0, 1 },    { 90, 0, 0, 1, 0, 1, -1, 0 },    { 180, 0, 1, 2, -1, 0, 0, -1 },
    { 270, 0, 2, 0, 0, -1, 1, 0 }, { 0, 'y', 1, 0, -1, 0, 0, 1 },   { 90, 'x', 0, 0, 0, 1, 1, 0 },
    { 0, 'x', 0, 2, 1, 0, 0, -1 }, { 270, 'x', 2, 1, 0, -1, -1, 0 }
};


/** Compose the 3-bit orientation code from the rotation bits and mirror bit. */
inline int OrcadOrientOf( int aRotation, bool aMirror )
{
    return ( aRotation & 3 ) | ( aMirror ? 0x4 : 0 );
}


inline int OrcadOrientDim( int aSelector, int aWidth, int aHeight )
{
    return aSelector == 1 ? aWidth : aSelector == 2 ? aHeight : 0;
}


/** Bbox re-anchoring offset for the given orientation and body size (DBU). */
inline VECTOR2I OrcadOrientOffset( int aOrient, int aWidth, int aHeight )
{
    const ORCAD_ORIENT_ENTRY& e = ORCAD_ORIENT_TABLE[aOrient & 7];
    return VECTOR2I( OrcadOrientDim( e.txSel, aWidth, aHeight ), OrcadOrientDim( e.tySel, aWidth, aHeight ) );
}


/** Parts use the instance anchor as the base. Power symbols, ports, and connectors use the box minimum. */
inline VECTOR2I OrcadTransformPoint( int aOrient, int aWidth, int aHeight, int aBaseX, int aBaseY, int aPx, int aPy )
{
    const ORCAD_ORIENT_ENTRY& e = ORCAD_ORIENT_TABLE[aOrient & 7];
    VECTOR2I                  t = OrcadOrientOffset( aOrient, aWidth, aHeight );

    return VECTOR2I( aBaseX + t.x + e.a * aPx + e.b * aPy, aBaseY + t.y + e.c * aPx + e.d * aPy );
}


class ORCAD_CONVERTER
{
public:
    /** aDesign is mutated and must outlive the converter. */
    ORCAD_CONVERTER( ORCAD_DESIGN& aDesign, SCHEMATIC* aSchematic, REPORTER* aReporter,
                     PROGRESS_REPORTER* aProgressReporter = nullptr );

    /** Out-of-line: LIB_ENTRY holds LIB_SYMBOL by unique_ptr.  [orcad_converter_sheet.cpp] */
    ~ORCAD_CONVERTER();

    /** aRootSheet must have a screen and be registered on the schematic. Cancellation throws IO_ERROR. */
    SCH_SHEET* Convert( SCH_SHEET* aRootSheet );

    /** The caller owns the returned library symbols. */
    std::vector<LIB_SYMBOL*> BuildSymbolLibrary();

    /** Symbol library nickname used in every emitted LIB_ID. */
    static constexpr const char* LIB_NICK = "orcad_import";

private:
    /** -- shared bookkeeping types --------------------------------------------------- */

    /** One unit of an emitted multi-unit symbol. */
    struct UNIT_INFO
    {
        std::string              letter; ///< unit discriminator, e.g. "A", "-16", "B:Convert"
        const ORCAD_SYMBOL_DEF*  symbol = nullptr;
        std::vector<std::string> pinNumbers; ///< device pin-number map for this unit
        std::vector<bool>        pinNumberVisible; ///< nonblank source package pin numbers
        std::vector<bool>        pinIgnore;  ///< package pins suppressed for this unit
        std::vector<int>         pinOffsets; ///< hidden duplicate-pin electrical offsets, DBU
        std::vector<bool>        explicitPinNets; ///< placed pins whose source net overrides implicit power naming

        bool operator==( const UNIT_INFO& ) const = default;
    };

    /** One emitted KiCad lib symbol (possibly multi-unit). */
    struct LIB_ENTRY
    {
        std::string                 name;  ///< lib item name (no nickname)
        std::vector<UNIT_INFO>      units; ///< sorted by letter; unit numbers are 1-based indices
        bool                        isPower = false;
        std::string                 powerNet; ///< power symbols are keyed by NET name
        std::string                 refPrefix = "U";
        std::string                 footprint;
        std::unique_ptr<LIB_SYMBOL> kicadSymbol; ///< built lazily by kicadSymbolFor()
    };

    /** One resolved off-page connector: index into page.offpage, net, pin position. */
    struct OFFPAGE_NET
    {
        int         index = 0;
        std::string net;
        int         x = 0; ///< DBU
        int         y = 0;
    };

    /** (sourcePackage-or-pkgName, pkgName, variant index, unit discriminator) ->
     * (lib name, unit number). */
    using PKG_KEY = std::tuple<std::string, std::string, int, std::string>;

    struct PLACED_PACKAGE_UNIT
    {
        SCH_SYMBOL*    symbol;
        const void*    scope;
        wxString       reference;
        const PKG_KEY* sourceUnit;
        UNIT_INFO      unit;
    };

    /** -- constants (calibrated; do not change) ---------------------------------------- */

    /** Standard OrCAD pin length used for synthesized placeholder pins, DBU. */
    static constexpr int PIN_LEN_DBU = 10;

    /** Page margins in DBU leave room for the KiCad frame and title block. */
    static constexpr int MARGIN_L_DBU = 60;
    static constexpr int MARGIN_T_DBU = 60;
    static constexpr int MARGIN_R_DBU = 60;
    static constexpr int MARGIN_B_DBU = 100;

    /** -- reporting [orcad_converter_sheet.cpp] ------------------------------------------ */

    /** Conversion problem: REPORTER at RPT_SEVERITY_WARNING. */
    void warn( const wxString& aMsg );

    /** Fact about the source design (not a conversion problem): RPT_SEVERITY_INFO. */
    void note( const wxString& aMsg );

    /** Capture net names ignore case; KiCad net names do not. Use one spelling for each global net. */
    void prepareGlobalNetNames();

    /** Return the design-wide spelling selected by prepareGlobalNetNames(). */
    std::string canonicalGlobalNetName( const std::string& aName ) const;
    std::string effectiveInterfaceNetName( const std::string& aName ) const;
    std::string occurrenceElectricalNetName( uint32_t aOccurrenceId, const std::string& aName ) const;
    bool        isPowerNetName( const std::string& aName ) const;
    bool        isOffpageNetName( const std::string& aName ) const;


    void prepareSymbols();

    /** Use placed pin positions to keep uncached symbols connected. */
    ORCAD_SYMBOL_DEF synthesizeSymbol( const std::string&                               aPkgName,
                                       const std::vector<const ORCAD_PLACED_INSTANCE*>& aInstances ) const;

    /** The unit key includes a non-Normal view so DeMorgan graphics remain distinct. */
    std::string unitLetter( const ORCAD_PLACED_INSTANCE& aInst ) const;

    /** Match cached variants to placed pin positions; use the first entry if none matches. */
    std::pair<const ORCAD_SYMBOL_DEF*, int> pickVariant( const ORCAD_PLACED_INSTANCE& aInst ) const;

    /** Returns the registered library name and unit number. */
    std::pair<std::string, int> libForInstance( const ORCAD_PLACED_INSTANCE& aInst,
                                              const PKG_KEY** aSourceUnit = nullptr );
    void finalizeNativePowerPackages();

    /** Return the occurrence net id owning aName, which the table identifies by address. */
    std::optional<uint32_t> occurrenceNetIdFor( const std::string* aName ) const;

    /** Run the shared finalization sequence that ends every conversion entry point. */
    void finishConversion();

    /** Key power symbols by net name because users can rename their ports. */
    std::string powerLibFor( const std::string& aSymbolName, const std::string& aNetName );


    LIB_SYMBOL* kicadSymbolFor( const std::string& aLibName );


    void addSymbolPrimitive( LIB_SYMBOL* aSymbol, const ORCAD_PRIMITIVE& aPrim, int aUnit, int aColor,
                             int aOffsetX = 0, int aOffsetY = 0 );

    /** Polyline approximation of an arc primitive.  Helper of addSymbolPrimitive(). */
    void addSymbolArc( LIB_SYMBOL* aSymbol, const ORCAD_PRIMITIVE& aPrim, int aUnit, int aColor, int aOffsetX = 0,
                       int aOffsetY = 0 );


    void addSymbolPin( LIB_SYMBOL* aSymbol, const ORCAD_SYMBOL_PIN& aPin, const wxString& aNumber, int aUnit,
                       bool aPower, const std::string& aNameOverride, bool aNameVisible, bool aShowPinNumbers,
                       bool aNumberVisible, const BOX2I* aBodyBox, bool aHidden = false,
                       bool aExplicitNet = false );


    void placeInstance( ORCAD_RAW_PAGE& aPage, const ORCAD_PLACED_INSTANCE& aInst, SCH_SCREEN* aScreen,
                        const SCH_SHEET_PATH& aSheetPath );

    /** Use the occurrence reference when the instance has an unannotated template. */
    wxString resolveReference( const ORCAD_PLACED_INSTANCE& aInst ) const;


    void placePowerSymbol( ORCAD_RAW_PAGE& aPage, const ORCAD_GRAPHIC_INST& aInst, const std::string& aNet,
                           SCH_SCREEN* aScreen, const SCH_SHEET_PATH& aSheetPath );


    void placeSymbolFields( SCH_SYMBOL* aSymbol, const ORCAD_PLACED_INSTANCE& aInst, const ORCAD_SYMBOL_DEF& aDef,
                            int aOrient, const std::string& aValue, const std::string& aFootprint );

    /** The cache package backing a placement, or nullptr when the design has none. */
    const ORCAD_PACKAGE* packageFor( const ORCAD_PLACED_INSTANCE& aInst ) const;

    /** Empty package and placement values must not clear a library property. */
    std::map<std::string, std::string> effectiveProps( const ORCAD_PLACED_INSTANCE& aInst,
                                                       const ORCAD_SYMBOL_DEF&      aDef ) const;


    static int toKicadOrientation( int aOrient );

    void computeFontBaseline();

    int displayFontId( const ORCAD_DISPLAY_PROP& aProp ) const;
    bool displayUsesTemplateFont( const ORCAD_DISPLAY_PROP& aProp ) const;
    int wireAliasFontId( const ORCAD_ALIAS& aAlias ) const;
    int textBaselineOffset( int aTextSize, int aFontIdx, bool aTemplateFont = true ) const;

    /** Resolve a Design Template font ID to its 1-based LOGFONT index. */
    int resolveFontIndex( int aFontIdx ) const;

    /** |lfHeight| of a Design Template font ID; ID 0 selects the template default. */
    int fontHeightDbu( int aFontIdx, bool aTemplateFont = true ) const;


    int textSizeIU( int aFontIdx, bool aTemplateFont = true ) const;

    /** Preserve an explicit LOGFONT width while retaining natural font aspect when it is zero. */
    VECTOR2I textSize( int aFontIdx, bool aTemplateFont = true ) const;

    void applyFont( EDA_TEXT* aText, int aFontIdx, bool aTemplateFont = true ) const;
    void applyMultilineSpacing( SCH_TEXT* aText, int aFontIdx, bool aTemplateFont = true ) const;

    /** LIB_ID-safe symbol name: ':', '"', '/' and whitespace runs -> '_'; "SYM" if empty. */
    static std::string SymbolId( const std::string& aName );


    void convertPage( ORCAD_RAW_PAGE& aPage, SCH_SCREEN* aScreen, const SCH_SHEET_PATH& aSheetPath,
                      bool aContainerPage = false, bool aSharedFolderPage = false );
    void placeHierarchicalBlockFields( SCH_SHEET* aSheet, const ORCAD_DRAWN_INSTANCE& aBlock,
                                       const std::string& aChildFolder );
    void placeHierarchicalBlockPinFill( SCH_SCREEN* aScreen, SCH_SHEET_PIN* aPin );
    void appendNetIntent( SCH_SCREEN* aScreen, SCH_LABEL* aLabel, bool aExplicitName, uint32_t aNetId = 0 );
    void minimizeNetLabels();
    void finalizeNetNames();
    void recordNetNameMap();
    void convertUnreferencedPages();

    KIID deterministicUuid( const std::string& aRole, size_t aOrdinal ) const;
    void appendPageItem( SCH_SCREEN* aScreen, SCH_ITEM* aItem );
    void assignPageItemUuids( size_t aPageOrdinal );


    static wxString MakePageFileName( int aPageIndex, const std::string& aPageName );

    /** Shift content on the 10-DBU grid to preserve connectivity. */
    void applyPageSettings( ORCAD_RAW_PAGE& aPage, SCH_SCREEN* aScreen );
    void placePageFrame( const ORCAD_RAW_PAGE& aPage, SCH_SCREEN* aScreen );

    /** Free graphics use nested primitive bounds; their outer boxes do not describe page coordinates. */
    static BOX2I pageExtentDbu( const ORCAD_RAW_PAGE& aPage );

    /** aDx and aDy must be multiples of the 10-DBU grid. */
    static void offsetPage( ORCAD_RAW_PAGE& aPage, int aDx, int aDy );


    void applyTitleBlock( const ORCAD_RAW_PAGE& aPage, SCH_SCREEN* aScreen );

    /** Rebuild m_wireEndpoints for a page (both endpoints of every wire). */
    void buildNetLookup( const ORCAD_RAW_PAGE& aPage );

    /** Prefer endpoint nets, then intersecting wires, then endpoint aliases. */
    std::string netAt( const ORCAD_RAW_PAGE& aPage, int aX, int aY ) const;


    std::string powerNet( const ORCAD_RAW_PAGE& aPage, const ORCAD_GRAPHIC_INST& aInst ) const;

    /** Use the transformed cache pin position, or the instance anchor if no pin is available. */
    VECTOR2I graphicPinPos( const ORCAD_GRAPHIC_INST& aInst ) const;

    /** Graphic-symbol connection point, corrected to a nearby endpoint of its named wire. */
    VECTOR2I namedGraphicPinPos( const ORCAD_RAW_PAGE& aPage, const ORCAD_GRAPHIC_INST& aInst ) const;

    /** Power-symbol connection point, corrected to a nearby endpoint of its named wire. */
    VECTOR2I powerPinPos( const ORCAD_RAW_PAGE& aPage, const ORCAD_GRAPHIC_INST& aInst ) const;

    std::vector<int> placedStackedPinOffsets( const ORCAD_PLACED_INSTANCE& aInstance ) const;
    VECTOR2I placedPinElectricalPosition( const ORCAD_PLACED_INSTANCE& aInstance, size_t aPinIndex ) const;
    bool hasImplicitPowerPinName( const ORCAD_PLACED_INSTANCE& aInstance, size_t aPinIndex,
                                  const std::string& aNetName ) const;

    /** Resolve every off-page connector on the page to (index, net, pin position). */
    std::vector<OFFPAGE_NET> offpageNets( const ORCAD_RAW_PAGE& aPage ) const;

    /** Use both geometry and net IDs; a crossing alone does not create a junction. */
    std::vector<VECTOR2I> computeJunctions( const ORCAD_RAW_PAGE& aPage ) const;

    /** Append computed junctions as SCH_JUNCTION items. */
    void placeJunctions( const ORCAD_RAW_PAGE& aPage, SCH_SCREEN* aScreen );


    void placeWires( const ORCAD_RAW_PAGE& aPage, SCH_SCREEN* aScreen, bool aHierarchical,
                     bool aNestedHierarchy, bool aSharedFolderPage );

    /** SCH_BUS_WIRE_ENTRY per bus entry (position x1,y1; size x2-x1, y2-y1). */
    void placeBusEntries( const ORCAD_RAW_PAGE& aPage, SCH_SCREEN* aScreen );

    /** One displayed property attached to a placed page symbol, preserving source text geometry. */
    void placeGraphicDisplayText( const ORCAD_GRAPHIC_INST& aGraphic, const ORCAD_DISPLAY_PROP& aDisplay,
                                  const std::string& aText, SCH_SCREEN* aScreen );

    /** Keep computed intersheet references hidden; display the stored OrCAD IREF text. */
    void placeOffpageConnectors( const ORCAD_RAW_PAGE& aPage, SCH_SCREEN* aScreen,
                                 const SCH_SHEET_PATH& aSheetPath, bool aHierarchical );


    void placePorts( const ORCAD_RAW_PAGE& aPage, SCH_SCREEN* aScreen, bool aHierarchical );


    void placeGraphics( const ORCAD_RAW_PAGE& aPage, SCH_SCREEN* aScreen );

    /** Skip undecodable images with a warning. */
    void placeBitmap( const ORCAD_PRIMITIVE& aPrim, SCH_SCREEN* aScreen, int aOrient = 0,
                      bool* aUsedEmbeddedEmf = nullptr );

    void placeDefinitionImages( const ORCAD_SYMBOL_DEF& aDefinition, int aBaseX, int aBaseY, int aOrient,
                                SCH_SCREEN* aScreen );

    void placeDefinitionVectors( const ORCAD_SYMBOL_DEF& aDefinition, int aBaseX, int aBaseY, int aOrient,
                                 SCH_SCREEN* aScreen, double aTextScaleX = 1.0, double aTextScaleY = 1.0,
                                 bool aUseGenericTextBaseline = false,
                                 const std::string& aTextFaceOverride = {} );

    /** Closest point (DBU) on a wire segment to (aX, aY); exact for H/V wires. */
    static VECTOR2I snapToWire( int aX, int aY, const ORCAD_WIRE& aWire );

    /** True when the point lies strictly INSIDE (not at an endpoint of) an H/V wire. */
    static bool onSegment( int aX, int aY, const ORCAD_WIRE& aWire );


    static wxString SanitizeFileName( const std::string& aName );

private:
    ORCAD_DESIGN&      m_design;
    SCHEMATIC*         m_schematic;
    REPORTER*          m_reporter;
    PROGRESS_REPORTER* m_progressReporter;
    SCH_SHEET*         m_rootSheet;

    std::map<std::string, LIB_ENTRY>               m_libSymbols; ///< keyed by emitted lib name
    std::map<PKG_KEY, std::pair<std::string, int>> m_pkgToLib;
    std::map<PKG_KEY, std::pair<std::string, int>> m_preparedPkgToLib;
    std::map<std::string, std::vector<UNIT_INFO>> m_preparedLibUnits;
    std::vector<PLACED_PACKAGE_UNIT>             m_placedPackageUnits;
    std::set<std::string>                       m_nativePowerFamilies;
    std::map<std::string, std::string>             m_globalNetNames;
    std::map<std::string, std::string>             m_globalNetAliases;
    std::set<std::string>                          m_powerNetNames;
    std::set<std::string>                          m_offpageNetNames;
    std::set<std::string>                          m_connectedBlockInterfaceNames;
    std::map<std::string, size_t>                  m_occurrenceNetNameScopeCounts;
    std::map<std::string, size_t>                  m_occurrenceNetNameMinDepth;
    int                                            m_powerCount;      ///< "#PWR%04d" counter
    int                                            m_fontBaselineDbu; ///< dominant text height; 0 = none
    size_t                                         m_pageOrdinal = 0;
    size_t                                         m_screenOrdinal = 1;
    SCH_SCREEN*                                    m_pageItemScreen = nullptr;
    std::vector<SCH_ITEM*>                         m_pageItems;

    /** Per-page occurrence references distinguish repeated child schematics. Null means no occurrence tree. */
    const std::map<uint32_t, std::string>*                    m_currentOccRefs = nullptr;
    const std::map<uint32_t, std::string>*                    m_currentOccUnitRefs = nullptr;
    const std::map<uint32_t, std::map<std::string, std::string>>* m_currentOccProps = nullptr;
    const std::map<uint32_t, std::string>*                    m_currentOccNetNames = nullptr;
    std::string                                               m_currentFlatNetSuffix;
    bool                                                      m_scopeNamedFlatNets = false;
    bool                                                      m_scopeGeneratedFlatNets = false;
    std::map<std::string, std::string>                         m_currentUnconnectedInterfaceNetNames;
    std::map<std::string, std::string>                         m_currentInterfaceNetAliases;
    std::map<std::string, std::string>                         m_currentOccurrenceNetAliases;
    std::set<std::string>                                      m_currentConnectorInterfaceNetAliases;
    std::set<const ORCAD_PIN_INST*>                            m_currentImplicitPowerPins;
    std::map<std::string, std::map<std::string, std::string>> m_hierBusNamesByScreen;

    struct NET_LABEL_INTENT
    {
        SCH_SCREEN* screen;
        SCH_LABEL_BASE* label;
        bool explicitName;
    };

    struct INTERFACE_LABEL_SOURCE
    {
        SCH_SCREEN* screen;
        SCH_LABEL_BASE* label;
        std::vector<SCH_LINE*> wires;
    };

    void rememberInterfaceLabelSource( SCH_SCREEN* aScreen, SCH_LABEL_BASE* aLabel,
                                       const std::vector<SEG>& aSourceWires );

    std::vector<INTERFACE_LABEL_SOURCE> m_interfaceLabelSources;
    std::vector<NET_LABEL_INTENT> m_netLabelIntents;
    std::map<SCH_LABEL_BASE*, std::pair<SCH_SCREEN*, uint32_t>> m_labelSourceNets;
    std::map<SCH_SCREEN*, const ORCAD_RAW_PAGE*> m_sourcePages;
    std::map<std::pair<SCH_SCREEN*, uint32_t>, std::vector<SCH_ITEM*>> m_sourceNetItems;
    std::map<std::pair<SCH_SCREEN*, uint32_t>, std::set<std::string>> m_sourceNetNames;
    std::map<std::pair<SCH_SCREEN*, uint32_t>, std::string> m_sourceGeneratedNetNames;
    struct SOURCE_PIN_IDENTITY
    {
        wxString number;
        VECTOR2I position;
        std::optional<KIID> libraryPin;
        bool ignored = false;
    };

    std::map<SCH_SYMBOL*, std::vector<SOURCE_PIN_IDENTITY>> m_sourcePinIdentities;
    std::map<SCH_SYMBOL*, const ORCAD_PLACED_INSTANCE*> m_sourceInstances;
    std::map<SCH_SCREEN*, std::vector<wxString>> m_sourceOccurrences;
    std::map<std::tuple<SCH_SCREEN*, const ORCAD_PLACED_INSTANCE*, size_t>, std::pair<uint32_t, std::string>> m_wirelessNetNames;

    /** Lower-cased sheet names already emitted, to keep sibling sheet names unique. */
    std::set<wxString> m_usedSheetNames;

    /** Per-page wire lookup: endpoint -> wires ending there.  Rebuilt by buildNetLookup(). */
    std::map<std::pair<int, int>, std::vector<const ORCAD_WIRE*>> m_wireEndpoints;
};

#endif // ORCAD_CONVERTER_H_
