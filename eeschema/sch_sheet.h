/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2024 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef SCH_SHEEET_H
#define SCH_SHEEET_H

#include <sch_field.h>

class KIID_PATH;
class SCH_SCREEN;
class SCH_IO_KICAD_SEXPR_PARSER;
class SCH_SHEET_LIST;
class SCH_SHEET_PIN;
class SCH_SHEET_PATH;
class EDA_DRAW_FRAME;


#define MIN_SHEET_WIDTH  500    // Units are mils.
#define MIN_SHEET_HEIGHT 150    // Units are mils.


enum  SHEET_FIELD_TYPE
{
    SHEETNAME = 0,
    SHEETFILENAME,

    /// The first 2 are mandatory, and must be instantiated in SCH_SHEET
    SHEET_MANDATORY_FIELDS
};


/**
 * Sheet symbol placed in a schematic, and is the entry point for a sub schematic.
 */
class SCH_SHEET : public SCH_ITEM
{
public:
    SCH_SHEET( EDA_ITEM* aParent = nullptr, const VECTOR2I& aPos = VECTOR2I( 0, 0 ),
               VECTOR2I aSize = VECTOR2I( schIUScale.MilsToIU( MIN_SHEET_WIDTH ),
                                          schIUScale.MilsToIU( MIN_SHEET_HEIGHT ) ),
               FIELDS_AUTOPLACED aAutoplaceFields = FIELDS_AUTOPLACED_AUTO );

    /**
     * Copy \a aSheet into a new object.  All sheet pins are copied as is except and
     * the SCH_SHEET_PIN's #m_Parent pointers are set to the new copied parent object.
     */
    SCH_SHEET( const SCH_SHEET& aSheet );

    ~SCH_SHEET();

    static inline bool ClassOf( const EDA_ITEM* aItem )
    {
        return aItem && SCH_SHEET_T == aItem->Type();
    }

    wxString GetClass() const override
    {
        return wxT( "SCH_SHEET" );
    }

    /**
     * Return true for items which are moved with the anchor point at mouse cursor
     * and false for items moved with no reference to anchor.
     *
     * Usually return true for small items (labels, junctions) and false for
     * items which can be large (hierarchical sheets, symbols).
     *
     * @return false for a hierarchical sheet.
     */
    bool IsMovableFromAnchorPoint() const override { return false; }

    std::vector<SCH_FIELD>& GetFields() { return m_fields; }
    const std::vector<SCH_FIELD>& GetFields() const { return m_fields; }

    /**
     * Set multiple schematic fields.
     *
     * @param aFields are the fields to set in this symbol.
     */
    void SetFields( const std::vector<SCH_FIELD>& aFields );

    wxString GetShownName( bool aAllowExtraText ) const
    {
        return m_fields[SHEETNAME].GetShownText( aAllowExtraText );
    }
    wxString GetName() const { return m_fields[ SHEETNAME ].GetText(); }
    void SetName( const wxString& aName ) { m_fields[ SHEETNAME ].SetText( aName ); }

    SCH_SCREEN* GetScreen() const { return m_screen; }

    VECTOR2I GetSize() const { return m_size; }
    void     SetSize( const VECTOR2I& aSize ) { m_size = aSize; }

    int GetBorderWidth() const { return m_borderWidth; }
    void SetBorderWidth( int aWidth ) { m_borderWidth = aWidth; }

    KIGFX::COLOR4D GetBorderColor() const { return m_borderColor; }
    void SetBorderColor( KIGFX::COLOR4D aColor ) { m_borderColor = aColor; }

    KIGFX::COLOR4D GetBackgroundColor() const { return m_backgroundColor; }
    void SetBackgroundColor( KIGFX::COLOR4D aColor ) { m_backgroundColor = aColor; }

    /**
     * @return true if this sheet is the root sheet.
     */
    bool IsRootSheet() const;

    /**
     * Set the #SCH_SCREEN associated with this sheet to \a aScreen.
     *
     * The screen reference counting is performed by SetScreen.  If \a aScreen is not
     * the same as the current screen, the current screen reference count is decremented
     * and \a aScreen becomes the screen for the sheet.  If the current screen reference
     * count reaches zero, the current screen is deleted.  NULL is a valid value for
     * \a aScreen.
     *
     * @param aScreen The new screen to associate with the sheet.
     */
    void SetScreen( SCH_SCREEN* aScreen );

    /**
     * Return the number of times the associated screen for the sheet is being used.
     *
     * If no screen is associated with the sheet, then zero is returned.
     */
    int GetScreenCount() const;

    /**
     * Return the list of system text vars & fields for this sheet.
     */
    void GetContextualTextVars( wxArrayString* aVars ) const;

    /**
     * Resolve any references to system tokens supported by the sheet.
     *
     * @param aDepth is a counter to limit recursion and circular references.
     */
    bool ResolveTextVar( const SCH_SHEET_PATH* aPath, wxString* token, int aDepth = 0 ) const;

    void GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList ) override;

    /* there is no member for orientation in sch_sheet, to preserve file
     * format, we detect orientation based on pin edges
     */
    bool IsVerticalOrientation() const;

    void SetPositionIgnoringPins( const VECTOR2I& aPosition );

    /**
     * Add aSheetPin to the sheet.
     *
     * @note Once a sheet pin is added to the sheet, it is owned by the sheet.
     *       Do not delete the sheet pin object or you will likely get a segfault
     *       when the sheet is destroyed.
     *
     * @param aSheetPin The sheet pin item to add to the sheet.
     */
    void AddPin( SCH_SHEET_PIN* aSheetPin );

    std::vector<SCH_SHEET_PIN*>& GetPins() { return m_pins; }
    const std::vector<SCH_SHEET_PIN*>& GetPins() const { return m_pins; }

    /**
     * Remove \a aSheetPin from the sheet.
     *
     * @param aSheetPin The sheet pin item to remove from the sheet.
     */
    void RemovePin( const SCH_SHEET_PIN* aSheetPin );

    /**
     * Delete sheet label which do not have a corresponding hierarchical label.
     *
     * @note Make sure you save a copy of the sheet in the undo list before calling
     *       CleanupSheet() otherwise any unreferenced sheet labels will be lost.
     */
    void CleanupSheet();

    /**
     * Return the sheet pin item found at \a aPosition in the sheet.
     *
     * @param aPosition The position to check for a sheet pin.
     *
     * @return The sheet pin found at \a aPosition or NULL if no sheet pin is found.
     */
    SCH_SHEET_PIN* GetPin( const VECTOR2I& aPosition );

    /**
     * Checks if the sheet already has a sheet pin named \a aName.
     *
     * @param aName Name of the sheet pin to search for.
     * @return  True if sheet pin with \a aName is found, otherwise false.
     */
    bool HasPin( const wxString& aName ) const;

    bool HasPins() const { return !m_pins.empty(); }

    /**
     * Check all sheet labels against schematic for undefined hierarchical labels.
     *
     * @return True if there are any undefined labels.
     */
    bool HasUndefinedPins() const;

    /**
     * Return the minimum width of the sheet based on the widths of the sheet pin text.
     *
     * The minimum sheet width is determined by the width of the bounding box of each
     * hierarchical sheet pin.  If two pins are horizontally adjacent ( same Y position )
     * to each other, the sum of the bounding box widths is used.  If at some point in
     * the future sheet objects can be rotated or pins can be placed in the vertical
     * orientation, this function will need to be changed.
     *
     * @return The minimum width the sheet can be resized.
     */
    int GetMinWidth( bool aFromLeft ) const;

    /**
     * Return the minimum height that the sheet can be resized based on the sheet pin positions.
     *
     * The minimum width of a sheet is determined by the Y axis location of the bottom
     * most sheet pin.  If at some point in the future sheet objects can be rotated or
     * pins can be placed in the vertical orientation, this function will need to be
     * changed.
     *
     * @return The minimum height the sheet can be resized.
     */
    int GetMinHeight( bool aFromTop ) const;

    int GetPenWidth() const override;

    void Print( const RENDER_SETTINGS* aSettings, const VECTOR2I& aOffset ) override;

    /**
     * Return a bounding box for the sheet body but not the fields.
     */
    const BOX2I GetBodyBoundingBox() const;

    const BOX2I GetBoundingBox() const override;

    /**
     * Rotating around the boundingBox's center can cause walking when the sheetname or
     * filename is longer than the edge it's on.  Use this instead, which always returns
     * the center of the sheet itself.
     */
    VECTOR2I GetRotationCenter() const;

    void SwapData( SCH_ITEM* aItem ) override;

    /**
     * Count our own symbols, without the power symbols.
     */
    int SymbolCount() const;

    /**
     * Search the existing hierarchy for an instance of screen loaded from \a aFileName.
     *
     * @param aFilename The filename to find (MUST be absolute, and in wxPATH_NATIVE encoding).
     * @param aScreen A location to return a pointer to the screen (if found).
     * @return true if found, and a pointer to the screen
     */
    bool SearchHierarchy( const wxString& aFilename, SCH_SCREEN** aScreen );

    /**
     * Search the existing hierarchy for an instance of screen loaded from \a aFileName.
     *
     * Don't bother looking at the root sheet, it must be unique.  No other references to
     * its m_screen otherwise there would be loops in the hierarchy.
     *
     * @param[in] aScreen The SCH_SCREEN* screen that we search for.
     * @param[in] aList The SCH_SHEET_PATH* that must be used.
     * @return true if found.
     */
    bool LocatePathOfScreen( SCH_SCREEN* aScreen, SCH_SHEET_PATH* aList );

    /**
     * Count the number of sheets found in "this" sheet including all of the subsheets.
     *
     * @return the full count of sheets+subsheets contained by "this"
     */
    int CountSheets() const;

    /**
     * Return the filename corresponding to this sheet.
     *
     * @return a wxString containing the filename
     */
    wxString GetFileName() const
    {
        return m_fields[ SHEETFILENAME ].GetText();
    }

    // Set a new filename without changing anything else
    void SetFileName( const wxString& aFilename )
    {
        // Filenames are stored using unix notation
        wxString tmp = aFilename;
        tmp.Replace( wxT( "\\" ), wxT( "/" ) );
        m_fields[ SHEETFILENAME ].SetText( tmp );
    }

    // Geometric transforms (used in block operations):

    void Move( const VECTOR2I& aMoveVector ) override;
    void MirrorHorizontally( int aCenter ) override;
    void MirrorVertically( int aCenter ) override;
    void Rotate( const VECTOR2I& aCenter ) override;

    bool Matches( const EDA_SEARCH_DATA& aSearchData, void* aAuxData ) const override;

    bool IsReplaceable() const override { return true; }

    /**
     * Resize this sheet to aSize and adjust all of the labels accordingly.
     *
     * @param[in] aSize The new size for this sheet.
     */
    void Resize( const VECTOR2I& aSize );

    void AutoplaceFields( SCH_SCREEN* aScreen, bool aManual ) override;

    void GetEndPoints( std::vector <DANGLING_END_ITEM>& aItemList ) override;

    bool UpdateDanglingState( std::vector<DANGLING_END_ITEM>& aItemListByType,
                              std::vector<DANGLING_END_ITEM>& aItemListByPos,
                              const SCH_SHEET_PATH*           aPath = nullptr ) override;

    bool IsConnectable() const override { return true; }

    bool HasConnectivityChanges( const SCH_ITEM* aItem,
                                 const SCH_SHEET_PATH* aInstance = nullptr ) const override;

    bool CanConnect( const SCH_ITEM* aItem ) const override
    {
        return ( aItem->Type() == SCH_LINE_T && aItem->GetLayer() == LAYER_WIRE )
                || ( aItem->Type() == SCH_LINE_T && aItem->GetLayer() == LAYER_BUS )
                || ( aItem->Type() == SCH_NO_CONNECT_T )
                || ( aItem->Type() == SCH_SYMBOL_T );
    }

    std::vector<VECTOR2I> GetConnectionPoints() const override;

    INSPECT_RESULT Visit( INSPECTOR inspector, void* testData,
                          const std::vector<KICAD_T>& aScanTypes ) override;

    void RunOnChildren( const std::function<void( SCH_ITEM* )>& aFunction ) override;

    wxString GetItemDescription( UNITS_PROVIDER* aUnitsProvider ) const override;

    BITMAPS GetMenuImage() const override;

    SCH_SHEET& operator=( const SCH_ITEM& aSheet );

    bool operator <( const SCH_ITEM& aItem ) const override;

    void ViewGetLayers( int aLayers[], int& aCount ) const override;

    VECTOR2I GetPosition() const override { return m_pos; }
    void     SetPosition( const VECTOR2I& aPosition ) override;

    bool HitTest( const VECTOR2I& aPosition, int aAccuracy ) const override;
    bool HitTest( const BOX2I& aRect, bool aContained, int aAccuracy = 0 ) const override;

    void Plot( PLOTTER* aPlotter, bool aBackground,
               const SCH_PLOT_SETTINGS& aPlotSettings ) const override;

    EDA_ITEM* Clone() const override;

    /**
     * @return the list of #SCH_SHEET_INSTANCE objects for this sheet.
     */
    const std::vector<SCH_SHEET_INSTANCE>& GetInstances() const { return m_instances; }

    /**
     * Check to see if this sheet has a root sheet instance.
     *
     * @note It is possible for sheets to contain both sheet instances from other projects as
     *       well as a root sheet instance for this schematic.  This will happen with a
     *       shared schematic is opened by the schematic editor in stand alone model.
     *
     * @return true if the instance data contains a root sheet instance.
     */
    bool HasRootInstance() const;

    /**
     * Return the root sheet instance data.
     *
     * @note If no root sheet instance data exists, an assertion will be raise in debug builds
     *       and an empty instance will be returned.
     *
     * @return the root sheet instance data.
     */
    const SCH_SHEET_INSTANCE& GetRootInstance() const;

    void RemoveInstance( const KIID_PATH& aInstancePath );

    void AddInstance( const SCH_SHEET_INSTANCE& aInstance );

    /**
     * Check if the instance data of this sheet has any changes compared to \a aOther.
     *
     * @param aOther is the sheet to compare against.
     * @return true if there are page number changes with \a aOther otherwise false.
     */
    bool HasPageNumberChanges( const SCH_SHEET& aOther ) const;

    /**
     * Compares page numbers of schematic sheets.
     *
     * @return 0 if the page numbers are equal, -1 if aPageNumberA < aPageNumberB, 1 otherwise
     */
    static int ComparePageNum( const wxString& aPageNumberA, const wxString& aPageNumberB );

    double Similarity( const SCH_ITEM& aOther ) const override;

    bool operator==( const SCH_ITEM& aOther ) const override;

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const override;
#endif

    static const wxString GetDefaultFieldName( int aFieldNdx, bool aTranslated = true );

protected:
    friend SCH_SHEET_PATH;
    friend SCH_IO_KICAD_SEXPR_PARSER;

    void setInstances( const std::vector<SCH_SHEET_INSTANCE>& aInstances )
    {
        m_instances = aInstances;
    }

    /**
     * Add a new instance \a aSheetPath to the instance list.
     *
     * If \a aSheetPath  does not already exist, it is added to the list.  If already exists
     * in the list, do nothing.  Sheet instances allow for the sharing in complex hierarchies
     * which allows for per instance data such as page number for sheets to stored.
     *
     * @warning The #KIID_PATH object must be a full hierarchical path which means the sheet
     *          at index 0 must be the root sheet.
     *
     * @param[in] aInstance is the #SCH_SHEET_PATH of the sheet instance to the instance list.
     * @return false if the instance already exists, true if the instance was added.
     */
    bool addInstance( const KIID_PATH& aInstance );

    /**
     * Return the sheet page number for \a aInstance.
     *
     * @warning The #KIID_PATH object must be a full hierarchical path which means the sheet
     *          at index 0 must be the root sheet.
     *
     * @return the page number for the requested sheet instance.
     */
    wxString getPageNumber( const KIID_PATH& aInstance ) const;

    /**
     * Set the page number for the sheet instance \a aInstance.
     *
     * @warning The #KIID_PATH object must be a full hierarchical path which means the sheet
     *          at index 0 must be the root sheet.
     *
     * @param[in] aInstance is the hierarchical path of the sheet.
     * @param[in] aReference is the new page number for the sheet.
     */
    void setPageNumber( const KIID_PATH& aInstance, const wxString& aPageNumber );

    bool getInstance( SCH_SHEET_INSTANCE& aInstance, const KIID_PATH& aSheetPath,
                      bool aTestFromEnd = false ) const;

    /**
     * Renumber the sheet pins in the sheet.
     *
     * This method is used internally by SCH_SHEET to update the pin numbering
     * when the pin list changes.  Make sure you call this method any time a
     * sheet pin is added or removed.
     */
    void renumberPins();

    /**
     * Get the sheetpath of this sheet.  NB: REQUIRES that the current sheet hierarchy contains
     * the given sheet.
     */
    SCH_SHEET_PATH findSelf() const;

private:
    bool doIsConnected( const VECTOR2I& aPosition ) const override;

    friend class SCH_SHEET_PIN;
    friend class SCH_SHEET_LIST;

private:
    SCH_SCREEN*                 m_screen;       // Screen that contains the physical data for the
                                                // sheet.  In complex hierarchies multiple sheets
                                                // can share a common screen.

    std::vector<SCH_SHEET_PIN*> m_pins;         // The list of sheet connection points.
    std::vector<SCH_FIELD>      m_fields;

    VECTOR2I                    m_pos;          // The position of the sheet.
    VECTOR2I                    m_size;         // The size of the sheet.
    int                         m_borderWidth;
    KIGFX::COLOR4D              m_borderColor;
    KIGFX::COLOR4D              m_backgroundColor;

    std::vector<SCH_SHEET_INSTANCE> m_instances;
};


#endif // SCH_SHEEET_H
