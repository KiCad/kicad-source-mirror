/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
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
    SCH_SHEET( EDA_ITEM* aParent = nullptr, const wxPoint& pos = wxPoint( 0, 0 ) );

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
    void SetFields( const std::vector<SCH_FIELD>& aFields )
    {
        m_fields = aFields;     // vector copying, length is changed possibly
    }

    wxString GetName() const { return m_fields[ SHEETNAME ].GetText(); }

    SCH_SCREEN* GetScreen() const { return m_screen; }

    wxSize GetSize() const { return m_size; }
    void SetSize( const wxSize& aSize ) { m_size = aSize; }

    int GetBorderWidth() const { return m_borderWidth; }
    void SetBorderWidth( int aWidth ) { m_borderWidth = aWidth; }

    KIGFX::COLOR4D GetBorderColor() const { return m_borderColor; }
    void SetBorderColor( KIGFX::COLOR4D aColor ) { m_borderColor = aColor; }

    KIGFX::COLOR4D GetBackgroundColor() const { return m_backgroundColor; }
    void SetBackgroundColor( KIGFX::COLOR4D aColor ) { m_backgroundColor = aColor; }

    /**
     * Test this sheet to see if the default stroke is used to draw the outline.
     *
     * The default stroke is defined as follows:
     * * The outline width is the default line width or 0.
     * * The outline style is set to #PLOT_DASH_TYPE::DEFAULT or #PLOT_DASH_TYPE::SOLID.
     * * The outline color is set to #COLOR4D::UNSPECIFIED.
     *
     * @return True if the outline stroke meets the default criteria.
     */
    bool UsesDefaultStroke() const;

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
    bool ResolveTextVar( wxString* token, int aDepth = 0 ) const;

    void GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList ) override;

    /* there is no member for orientation in sch_sheet, to preserve file
     * format, we detect orientation based on pin edges
     */
    bool IsVerticalOrientation() const;

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

    const std::vector<SCH_SHEET_PIN*>& GetPins() const
    {
        return m_pins;
    }

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
    SCH_SHEET_PIN* GetPin( const wxPoint& aPosition );

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

    void Print( const RENDER_SETTINGS* aSettings, const wxPoint& aOffset ) override;

    /**
     * Return a bounding box for the sheet body but not the fields.
     */
    const EDA_RECT GetBodyBoundingBox() const;

    const EDA_RECT GetBoundingBox() const override;

    /**
     * Rotating around the boundingBox's center can cause walking when the sheetname or
     * filename is longer than the edge it's on.  Use this instead, which always returns
     * the center of the sheet itself.
     */
    wxPoint GetRotationCenter() const;

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
    void SetFileName( wxString aFilename )
    {
        // Filenames are stored using unix notation
        aFilename.Replace( wxT("\\"), wxT("/") );
        m_fields[ SHEETFILENAME ].SetText( aFilename );
    }

    // Geometric transforms (used in block operations):

    void Move( const wxPoint& aMoveVector ) override;
    void MirrorHorizontally( int aCenter ) override;
    void MirrorVertically( int aCenter ) override;
    void Rotate( const wxPoint& aCenter ) override;

    bool Matches( const wxFindReplaceData& aSearchData, void* aAuxData ) const override;

    bool IsReplaceable() const override { return true; }

    /**
     * Resize this sheet to aSize and adjust all of the labels accordingly.
     *
     * @param[in] aSize The new size for this sheet.
     */
    void Resize( const wxSize& aSize );

    void AutoplaceFields( SCH_SCREEN* aScreen, bool aManual ) override;

    void GetEndPoints( std::vector <DANGLING_END_ITEM>& aItemList ) override;

    bool UpdateDanglingState( std::vector<DANGLING_END_ITEM>& aItemList,
                              const SCH_SHEET_PATH* aPath = nullptr ) override;

    bool IsConnectable() const override { return true; }

    bool CanConnect( const SCH_ITEM* aItem ) const override
    {
        return ( aItem->Type() == SCH_LINE_T && aItem->GetLayer() == LAYER_WIRE )
                || ( aItem->Type() == SCH_LINE_T && aItem->GetLayer() == LAYER_BUS )
                || ( aItem->Type() == SCH_NO_CONNECT_T )
                || ( aItem->Type() == SCH_SYMBOL_T );
    }

    std::vector<wxPoint> GetConnectionPoints() const override;

    SEARCH_RESULT Visit( INSPECTOR inspector, void* testData, const KICAD_T scanTypes[] ) override;

    void RunOnChildren( const std::function<void( SCH_ITEM* )>& aFunction ) override;

    wxString GetSelectMenuText( EDA_UNITS aUnits ) const override;

    BITMAPS GetMenuImage() const override;

    SCH_SHEET& operator=( const SCH_ITEM& aSheet );

    bool operator <( const SCH_ITEM& aItem ) const override;

    void ViewGetLayers( int aLayers[], int& aCount ) const override;

    wxPoint GetPosition() const override { return m_pos; }
    void SetPosition( const wxPoint& aPosition ) override;

    bool HitTest( const wxPoint& aPosition, int aAccuracy ) const override;
    bool HitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy = 0 ) const override;

    void Plot( PLOTTER* aPlotter ) const override;

    EDA_ITEM* Clone() const override;

    /**
     * @return the list of #SCH_SHEET_INSTANCE objects for this sheet.
     */
    const std::vector<SCH_SHEET_INSTANCE> GetInstances() const;

    /**
     * Add a new instance \a aSheetPath to the instance list.
     *
     * If \a aSheetPath  does not already exist, it is added to the list.  If already exists
     * in the list, do nothing.  Sheet instances allow for the sharing in complex hierarchies
     * which allows for per instance data such as page number for sheets to stored.
     *
     * @param[in] aInstance is the #KIID_PATH of the sheet instance to the instance list.
     * @return false if the instance already exists, true if the instance was added.
     */
    bool AddInstance( const KIID_PATH& aInstance );

    /**
     * Return the sheet page number for \a aInstance.
     *
     * @return the page number for the requested sheet instance.
     */
    wxString GetPageNumber( const SCH_SHEET_PATH& aInstance ) const;

    /**
     * Set the page number for the sheet instance \a aInstance.
     *
     * @param[in] aInstance is the hierarchical path of the sheet.
     * @param[in] aReference is the new page number for the sheet.
     */
    void SetPageNumber( const SCH_SHEET_PATH& aInstance, const wxString& aPageNumber );

    /**
     * Compares page numbers of schematic sheets.
     *
     * @return 0 if the page numbers are equal, -1 if aPageNumberA < aPageNumberB, 1 otherwise
     */
    static int ComparePageNum( const wxString& aPageNumberA, const wxString aPageNumberB );

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const override;
#endif

    static const wxString GetDefaultFieldName( int aFieldNdx );

protected:
    /**
     * Renumber the sheet pins in the sheet.
     *
     * This method is used internally by SCH_SHEET to update the pin numbering
     * when the pin list changes.  Make sure you call this method any time a
     * sheet pin is added or removed.
     */
    void renumberPins();

private:
    bool doIsConnected( const wxPoint& aPosition ) const override;

    friend class SCH_SHEET_PIN;

    SCH_SCREEN*   m_screen;     // Screen that contains the physical data for the sheet.  In
                                // complex hierarchies multiple sheets can share a common screen.

    std::vector<SCH_SHEET_PIN*> m_pins;               // The list of sheet connection points.
    std::vector<SCH_FIELD>      m_fields;

    wxPoint                     m_pos;                // The position of the sheet.
    wxSize                      m_size;               // The size of the sheet.
    int                         m_borderWidth;
    KIGFX::COLOR4D              m_borderColor;
    KIGFX::COLOR4D              m_backgroundColor;

    std::vector<SCH_SHEET_INSTANCE> m_instances;
};


#endif // SCH_SHEEET_H
