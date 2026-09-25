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
#include <functional>
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

#include <sch_io/orcad/orcad_orient.h>
#include <sch_io/orcad/orcad_records.h>

class LIB_SYMBOL;
class SCH_LINE;
class EDA_TEXT;
class PROGRESS_REPORTER;
class REPORTER;
class SCHEMATIC;
class SCH_FIELD;
class SCH_ITEM;
class SCH_LABEL_BASE;
class SCH_LABEL;
class SCH_SCREEN;
class SCH_SHEET;
class SCH_SHEET_PIN;
class SCH_SHEET_PATH;
class SCH_SYMBOL;
class SCH_TEXT;
class SPIN_STYLE;
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


/** Map the point members of a primitive: points, arc start and arc end. The box corners are left alone. */
void OrcadTransformPrimitivePoints( ORCAD_PRIMITIVE&                                        aPrim,
                                    const std::function<ORCAD_POINT( const ORCAD_POINT& )>& aMap );

/** Translate a primitive, box included. Group children are relative to the group and need no shift. */
void OrcadOffsetPrimitive( ORCAD_PRIMITIVE& aPrim, int aDx, int aDy );

/** Visit every non-group primitive in drawing order with the accumulated offset of its enclosing groups. */
void OrcadForEachLeafPrimitive( const std::vector<ORCAD_PRIMITIVE>&                            aPrimitives,
                                const std::function<void( const ORCAD_PRIMITIVE&, int, int )>& aVisit );


/** Undo the orientation matrix of OrcadTransformPoint. Its determinant is +/-1, so the division is exact. */
inline ORCAD_POINT OrcadInverseOrient( int aOrient, int aX, int aY )
{
    const ORCAD_ORIENT_ENTRY& e = ORCAD_ORIENT_TABLE[aOrient & 7];
    int                       det = e.a * e.d - e.b * e.c;

    return { ( e.d * aX - e.b * aY ) / det, ( -e.c * aX + e.a * aY ) / det };
}


class ORCAD_CONVERTER
{
public:
    /**
     * @param aDesign parsed design; held by reference and MUTATED during
     *                conversion (page-content shift, placeholder symbol
     *                insertion), so it must outlive the converter.
     * @param aSchematic is KiCad schematic to load into.
     * @param aReporter is an optional #REPORTER object to write status information.
     * @param aProgressReporter is an optional progress dialog to show conversion progress.
     *
     * [orcad_converter_sheet.cpp]
     */
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
        const ORCAD_SYMBOL_DEF*  convert = nullptr; ///< DeMorgan view sharing this unit's pin map
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

    /** Occurrence context of the page being converted. A reused child schematic converts once per scope. */
    struct PAGE_SCOPE
    {
        const ORCAD_OCC_SCOPE*             occ = nullptr; ///< null when the page has no occurrence
        std::string                        flatNetSuffix;
        bool                               namedFlatNets = false;
        bool                               generatedFlatNets = false;
        std::map<std::string, std::string> unconnectedInterfaceNetNames;
        std::map<std::string, std::string> interfaceNetAliases;
        std::map<std::string, std::string> occurrenceNetAliases;
        std::set<std::string>              connectorInterfaceNetAliases;
        std::map<std::string, size_t>      electricalKeyCounts;
    };

    /** Lookups for the page and scope being converted, rebuilt by buildNetLookup(). */
    struct PAGE_NET_INDEX
    {
        const ORCAD_RAW_PAGE*                                          page = nullptr;
        std::map<std::pair<int, int>, std::vector<const ORCAD_WIRE*>> wireEndpoints;
        std::map<const ORCAD_PIN_INST*, std::set<uint32_t>>           pinNets;
        std::vector<VECTOR2I>                                          junctions;

        /** Scope occurrence nets owned by a page net through a wire or part object id. */
        std::map<uint32_t, std::set<const std::string*>> occurrenceNamesByNetId;

        /** powerNet() depends on the scope, so the memo lives only as long as this index. */
        mutable std::map<const ORCAD_GRAPHIC_INST*, std::string> powerNets;
    };

    /** Placements of a power symbol name and the power nets they resolved to. */
    struct POWER_ALIAS_EVIDENCE
    {
        size_t                                                placements = 0;
        std::map<std::string, std::pair<std::string, size_t>> targets;
    };

    /** Page counters and rules threaded through a folder hierarchy walk. The defaults are the folder rules. A
     *  design whose every block opens a single-page child keeps the rules it was first imported with. */
    struct FOLDER_WALK
    {
        int    pageIndex = 1;
        size_t sourcePageIndex = 0;
        size_t sourcePageCount = 0;
        bool   numberSourcePages = true;    ///< fill title-block page numbers from the walk order
        bool   occurrenceAliases = true;    ///< learn power and child occurrence aliases before placing pages
        bool   interfaceAliases = false;    ///< alias a child's interface nets to the parent occurrence net
        bool   nestedFlatSuffix = true;     ///< suffix by block path; else only in simple repeated leaf designs
        bool   folderSheetNames = true;     ///< name an unreferenced block after its folder, else its page
        bool   rootSharedFolderPage = true; ///< convert the root page as a shared folder page
    };

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

    /** Case-folded canonicalGlobalNetName() for use as a map key. */
    std::string canonicalGlobalNetKey( const std::string& aName ) const;

    std::string effectiveInterfaceNetName( const std::string& aName ) const;

    void        enterScope( PAGE_SCOPE aScope );
    std::string occurrenceBaseNetName( const std::string& aName ) const;
    std::string occurrenceElectricalNetName( uint32_t aOccurrenceId, const std::string& aName ) const;
    bool        isPowerNetName( const std::string& aName ) const;
    bool        isOffpageNetName( const std::string& aName ) const;


    void prepareSymbols();

    /** One unit per package device; a Convert view becomes the unit's DeMorgan body. */
    LIB_ENTRY buildPackageEntry( const ORCAD_PACKAGE& aPackage ) const;

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


    /** Per-pin choices kicadSymbolFor() makes before emitting a pin. */
    struct PIN_EMIT
    {
        wxString    number;
        int         unit = 1;
        bool        power = false;
        std::string nameOverride;
        bool        nameVisible = false;
        bool        showPinNumbers = false;
        bool        numberVisible = true;
        bool        hidden = false;
        bool        explicitNet = false;
    };

    void addSymbolPin( LIB_SYMBOL* aSymbol, const ORCAD_SYMBOL_PIN& aPin, const BOX2I& aBodyBox, PIN_EMIT aEmit );

    /** aBase, or aBase_pinsN for the first N >= 2 whose existing entry is absent or passes aReusable. */
    std::string uniqueLibName( const std::string&                             aBase,
                               const std::function<bool( const LIB_ENTRY& )>& aReusable ) const;

    /** New placement of a library symbol, oriented by an OrCAD orientation code. */
    SCH_SYMBOL* instantiateSymbol( const LIB_SYMBOL& aLibSymbol, const std::string& aLibName, int aUnit, int aOrient,
                                   const VECTOR2I& aPos, const SCH_SHEET_PATH& aSheetPath ) const;


    void placeInstance( ORCAD_RAW_PAGE& aPage, const ORCAD_PLACED_INSTANCE& aInst, SCH_SCREEN* aScreen,
                        const SCH_SHEET_PATH& aSheetPath );

    /** Record each CIS variant's installed state and property overrides as KiCad symbol variants.
     *  aValue is the part value before a not-installed part shows "NI". */
    void applyCisVariants( SCH_SYMBOL* aSymbol, const ORCAD_PLACED_INSTANCE& aInst,
                           const SCH_SHEET_PATH& aSheetPath, const std::string& aValue );

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

    /** Capture's display-property origin sits on the font baseline; return the KiCad text origin. */
    VECTOR2I displayPropPosition( const ORCAD_DISPLAY_PROP& aDisplay, const VECTOR2I& aAnchorIu ) const;

    /** Style a field from a display property. aSymbolFlips compensates the parent symbol transform. */
    void applyDisplayProp( SCH_FIELD& aField, const ORCAD_DISPLAY_PROP& aDisplay, const VECTOR2I& aAnchorIu,
                           bool aSymbolFlips, bool aApplyVisibility = true ) const;
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

    /** -- hierarchy [orcad_converter_sheet.cpp] ----------------------------------------- */

    /** Collect the occurrence facts every hierarchy strategy reads. */
    void prepareOccurrenceNets();

    /** A scope for aOcc carrying the occurrence aliases known so far. */
    PAGE_SCOPE occurrenceScope( const ORCAD_OCC_SCOPE& aOcc ) const;

    /** No-connect block pins whose names are wired elsewhere become per-occurrence nets. */
    std::map<std::string, std::string> unconnectedInterfaceNetNames( const ORCAD_DRAWN_INSTANCE& aBlock,
                                                                     const std::string& aFlatNetSuffix ) const;

    /** The child sheet for a hierarchical block, with its pins, appended to aParentSheet's screen. */
    SCH_SHEET* createBlockSheet( SCH_SHEET* aParentSheet, const ORCAD_RAW_PAGE& aParentPage,
                                 const ORCAD_DRAWN_INSTANCE& aBlock, const ORCAD_OCC_BLOCK& aOccurrence,
                                 const wxString& aName, const std::string& aFilePageName, int aPageIndex );

    /** Top-level sheets named by aNamesAndFiles. With aReuseRoot the first is the root sheet and the list
     *  replaces the top level; otherwise the sheets are appended. Returns only the listed sheets. */
    std::vector<SCH_SHEET*> createTopLevelSheets( const std::vector<std::pair<wxString, wxString>>& aNamesAndFiles,
                                                  bool aReuseRoot, bool aOrdinalSheetUuids );

    void recordPowerAlias( std::map<std::string, POWER_ALIAS_EVIDENCE>& aCandidates, const std::string& aSourceName,
                           const std::string& aElectricalName ) const;

    /** Rename a power symbol to the net most of its placements resolve to. */
    void acceptPowerAliases( const std::map<std::string, POWER_ALIAS_EVIDENCE>& aCandidates );

    /** One root page whose every block opens a single-page child. */
    bool canBuildNativeHierarchy( const ORCAD_RAW_PAGE& aPage, const ORCAD_OCC_SCOPE& aScope ) const;

    /** Every occurrence block is drawn exactly once on its parent folder's pages. */
    bool canBuildFolderHierarchy( const std::vector<ORCAD_RAW_PAGE>& aPages, const ORCAD_OCC_SCOPE& aScope ) const;
    void convertFolderHierarchy( const SCH_SHEET_PATH& aRootPath, bool aNative );
    void addChildOccurrenceAliases( const ORCAD_RAW_PAGE& aParentPage, const ORCAD_OCC_SCOPE& aParentScope,
                                    const ORCAD_DRAWN_INSTANCE& aDrawn, const ORCAD_OCC_SCOPE& aChildScope );
    void collectFolderPowerAliases( std::map<std::string, POWER_ALIAS_EVIDENCE>& aCandidates,
                                    std::vector<ORCAD_RAW_PAGE>& aPages, const ORCAD_OCC_SCOPE& aScope );
    void placeFolder( FOLDER_WALK& aWalk, std::vector<ORCAD_RAW_PAGE>& aPages, const ORCAD_OCC_SCOPE& aScope,
                      SCH_SHEET* aFolderSheet, const SCH_SHEET_PATH& aFolderPath,
                      const std::string& aOccurrenceSuffix, bool aRepeatedFolder,
                      const std::map<std::string, std::string>& aUnconnectedInterfaceNetNames,
                      std::map<std::string, std::string>        aInterfaceNetAliases );

    /** Every page, each child occurrence expanded, becomes a top-level sheet. */
    void convertFlatPages( const SCH_SHEET_PATH& aRootPath );

    /** Visit root pages, then child-folder pages, then unreferenced folder pages if asked. */
    void forEachDesignPage( const std::function<void( const ORCAD_RAW_PAGE& )>& aVisit,
                            bool aUnreferenced = false ) const;

    /** aBase, or "aBase (N)" for the first N >= 2 not already used by a sibling sheet. */
    wxString uniqueSheetName( const wxString& aBase );

    /** Occurrence nets in aScopeNets that a block pin on aPage reaches by name or by wire object id.
     *  aSourceKeys receives the pin's own name keys and those of the wires under it. */
    std::set<const std::string*> pinOccurrenceTargets( const ORCAD_RAW_PAGE& aPage, const ORCAD_BLOCK_PIN& aPin,
                                                       const std::map<uint32_t, std::string>& aScopeNets,
                                                       bool aSkipGenerated,
                                                       std::set<std::string>* aSourceKeys = nullptr ) const;

    /** The hierarchy path ending at a top-level sheet, numbered aPageNumber. */
    SCH_SHEET_PATH topLevelPath( SCH_SHEET* aSheet, size_t aPageNumber ) const;

    KIID deterministicUuid( const std::string& aRole, size_t aOrdinal ) const;

    /** Name-based UUIDs for a symbol's raw pins, derived from aRole and each pin's number, name and position. */
    void assignPinUuids( SCH_SYMBOL* aSymbol, const std::string& aRole ) const;

    /** A new screen with the next deterministic screen UUID, recorded as created by this import. */
    SCH_SCREEN* newScreen();

    /** Give items created after page conversion, such as moved labels and cleanup junctions, UUIDs derived from
     *  their type and position. Only screens this import created are touched. */
    void assignRemainingUuids();
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

    /** Rebuild m_netIndex for a page in the current scope. */
    void buildNetLookup( const ORCAD_RAW_PAGE& aPage );

    /** Page nets a placed pin joins through its net id, its wire object id or a wire under it. */
    const std::set<uint32_t>& pinNetIds( const ORCAD_PIN_INST& aPin ) const;

    /** Prefer endpoint nets, then intersecting wires, then endpoint aliases. */
    std::string netAt( const ORCAD_RAW_PAGE& aPage, int aX, int aY ) const;


    /** Memoized resolvePowerNet() for the indexed page. */
    std::string powerNet( const ORCAD_RAW_PAGE& aPage, const ORCAD_GRAPHIC_INST& aInst ) const;
    std::string resolvePowerNet( const ORCAD_RAW_PAGE& aPage, const ORCAD_GRAPHIC_INST& aInst ) const;

    /** The cached variant a page graphic uses: its Source Library first, then its placed bounds, else the first. */
    const ORCAD_SYMBOL_DEF* graphicDefinition( const ORCAD_GRAPHIC_INST& aInst ) const;

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

    /** Point an interface label at aPosDbu away from the wire that ends there. */
    SPIN_STYLE spinAwayFromWire( const VECTOR2I& aPosDbu ) const;

    /** Style a port or off-page label from its source graphic, draw its stored IREF text and add it.
     *  Returns the Name display property, or nullptr. */
    const ORCAD_DISPLAY_PROP* finishInterfaceLabel( const ORCAD_GRAPHIC_INST& aGraphic, SCH_LABEL_BASE* aLabel,
                                                    const std::vector<SEG>& aSourceWires, SCH_SCREEN* aScreen );


    void placeGraphics( const ORCAD_RAW_PAGE& aPage, SCH_SCREEN* aScreen );

    /** Skip undecodable images with a warning. */
    void placeBitmap( const ORCAD_PRIMITIVE& aPrim, SCH_SCREEN* aScreen, int aOrient = 0,
                      bool* aUsedEmbeddedEmf = nullptr );

    void placeDefinitionImages( const ORCAD_SYMBOL_DEF& aDefinition, int aBaseX, int aBaseY, int aOrient,
                                SCH_SCREEN* aScreen );

    void placeDefinitionVectors( const ORCAD_SYMBOL_DEF& aDefinition, int aBaseX, int aBaseY, int aOrient,
                                 SCH_SCREEN* aScreen, double aTextScaleX = 1.0, double aTextScaleY = 1.0,
                                 bool aUseGenericTextBaseline = false );

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
    std::map<std::string, size_t>                  m_occurrenceFolderCounts;
    bool                                           m_simpleRepeatedLeafDesign = false;

    /** Occurrence net aliases shared by every scope, then those a parent block adds per child scope. */
    std::map<std::string, std::string>                                                   m_baseOccurrenceNetAliases;
    std::map<const std::map<uint32_t, std::string>*, std::map<std::string, std::string>> m_occurrenceAliasesByScope;
    int                                            m_powerCount;      ///< "#PWR%04d" counter
    int                                            m_fontBaselineDbu; ///< dominant text height; 0 = none
    size_t                                         m_pageOrdinal = 0;
    size_t                                         m_screenOrdinal = 1;
    SCH_SCREEN*                                    m_pageItemScreen = nullptr;
    std::vector<SCH_ITEM*>                         m_pageItems;
    std::set<SCH_SCREEN*>                          m_importedScreens;
    std::set<const SCH_ITEM*>                      m_preExistingItems; ///< already on the root screen

    PAGE_SCOPE                                                m_scope;
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
    std::vector<std::pair<SCH_LABEL_BASE*, std::pair<SCH_SCREEN*, uint32_t>>> m_labelSourceNets;
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

    PAGE_NET_INDEX m_netIndex;
};

#endif // ORCAD_CONVERTER_H_
