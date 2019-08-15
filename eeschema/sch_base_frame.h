/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2015-2018 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef SCH_BASE_FRAME_H_
#define SCH_BASE_FRAME_H_

#include <lib_id.h>
#include <eda_draw_frame.h>

#include <sch_screen.h>
#include <sch_draw_panel.h>
#include "template_fieldnames.h"


namespace KIGFX
{
    class SCH_RENDER_SETTINGS;
}

class PAGE_INFO;
class TITLE_BLOCK;
class LIB_VIEW_FRAME;
class LIB_EDIT_FRAME;
class LIB_ALIAS;
class LIB_PART;
class PART_LIB;
class SCHLIB_FILTER;
class LIB_ID;
class SYMBOL_LIB_TABLE;
class SCH_DRAW_PANEL;

/**
 * Load symbol from symbol library table.
 *
 * Check the symbol library table for the part defined by \a aLibId and optionally
 * check the optional cache library.
 *
 * @param aLibId is the symbol library identifier to load.
 * @param aLibTable is the #SYMBOL_LIBRARY_TABLE to load the alias from.
 * @param aCacheLib is an optional cache library.
 * @param aParent is an optiona parent window when displaying an error message.
 * @param aShowErrorMessage set to true to show any error messages.
 *
 * @return The symbol found in the library or NULL if the symbol was not found.
 */
LIB_ALIAS* SchGetLibAlias( const LIB_ID& aLibId, SYMBOL_LIB_TABLE* aLibTable,
                           PART_LIB* aCacheLib = NULL, wxWindow* aParent = NULL,
                           bool aShowErrorMsg = false );

LIB_PART* SchGetLibPart( const LIB_ID& aLibId, SYMBOL_LIB_TABLE* aLibTable,
                         PART_LIB* aCacheLib = NULL, wxWindow* aParent = NULL,
                         bool aShowErrorMsg = false );


/**
 * A shim class between EDA_DRAW_FRAME and several derived classes:
 * LIB_EDIT_FRAME, LIB_VIEW_FRAME, and SCH_EDIT_FRAME, and it brings in a
 * common way of handling the provided virtual functions for the derived classes.
 *
 * The motivation here is to switch onto GetScreen() for the underlying data model.
 *
 * @author Dick Hollenbeck
 */
class SCH_BASE_FRAME : public EDA_DRAW_FRAME
{
protected:
    TEMPLATES m_templateFieldNames;
    wxPoint   m_repeatStep;                // the increment value of the position of an item
                                           // when it is repeated
    int       m_repeatDeltaLabel;          // the increment value of labels like bus members
                                           // when they are repeated
    bool      m_showPinElectricalTypeName;
    bool      m_dragActionIsMove;          // drag action defaults to move, otherwise it's drag

public:
    SCH_BASE_FRAME( KIWAY* aKiway, wxWindow* aParent,
                    FRAME_T aWindowType,
                    const wxString& aTitle,
                    const wxPoint& aPosition, const wxSize& aSize,
                    long aStyle, const wxString & aFrameName );

    virtual ~SCH_BASE_FRAME();

    void createCanvas();

    SCH_DRAW_PANEL* GetCanvas() const override;
    SCH_SCREEN* GetScreen() const override;
    void SetScreen( BASE_SCREEN* aScreen ) override;

    KIGFX::SCH_RENDER_SETTINGS* GetRenderSettings();

    /**
     * Allow some frames to show/hide hidden pins.  The default impl shows all pins.
     */
    virtual bool GetShowAllPins() const { return true; }

    /**
     * Allow some frames to show/hide pin electrical type names.
     */
    bool GetShowElectricalType() { return m_showPinElectricalTypeName; }
    void SetShowElectricalType( bool aShow ) { m_showPinElectricalTypeName = aShow; }

    /**
     * @return the increment value of the position of an item
     * for the repeat command
     */
    const wxPoint GetRepeatStep() const { return m_repeatStep; }

    /**
     * Sets the repeat step value for repeat command
     * @param aStep the increment value of the position of an item
     * for the repeat command
     */
    void SetRepeatStep( const wxPoint& aStep) { m_repeatStep = aStep; }

    /**
     * @return the increment value of labels like bus members
     * for the repeat command
     */
    int GetRepeatDeltaLabel() const { return m_repeatDeltaLabel; }

    /**
     * Sets the repeat delta label value for repeat command
     * @param aDelta the increment value of labels like bus members
     * for the repeat command
     */
    void SetRepeatDeltaLabel( int aDelta ) { m_repeatDeltaLabel = aDelta; }


    /**
     * Function GetZoomLevelIndicator
     * returns a human readable value which can be displayed as zoom
     * level indicator in dialogs.
     * Virtual from the base class
     */
    const wxString GetZoomLevelIndicator() const override;

    void SetDragActionIsMove( bool aValue ) { m_dragActionIsMove = aValue; }
    bool GetDragActionIsMove() const { return m_dragActionIsMove; }

    void SetPageSettings( const PAGE_INFO& aPageSettings ) override;
    const PAGE_INFO& GetPageSettings () const override;
    const wxSize GetPageSizeIU() const override;

    const wxPoint& GetAuxOrigin() const override;
    void SetAuxOrigin( const wxPoint& aPosition ) override;

    const wxPoint& GetGridOrigin() const override
    {
        static wxPoint zero;
        return zero;
    }
    void SetGridOrigin( const wxPoint& aPoint ) override {}

    void OnGridSettings( wxCommandEvent& aEvent ) override;

    const TITLE_BLOCK& GetTitleBlock() const override;
    void SetTitleBlock( const TITLE_BLOCK& aTitleBlock ) override;

    void UpdateStatusBar() override;


    struct COMPONENT_SELECTION
    {
        LIB_ID      LibId;
        int         Unit;
        int         Convert;

        std::vector<std::pair<int, wxString>>   Fields;

        COMPONENT_SELECTION():
            Unit( 1 ),
            Convert( 1 )
        {}
    };

    typedef std::vector<COMPONENT_SELECTION> HISTORY_LIST;

    /**
     * Function SelectComponentFromLib
     * Calls the library viewer to select component to import into schematic.
     * if the library viewer is currently running, it is closed and reopened
     * in modal mode.
     *
     * aAllowFields chooses whether or not features that permit the user to edit
     * fields (e.g. footprint selection) should be enabled. This should be false
     * when they would have no effect, for example loading a part into libedit.
     *
     * @param aFilter is a SCHLIB_FILTER filter to pass the allowed library names
     *  and/or the library name to load the component from and/or some other filter
     *          if NULL, no filtering.
     * @param aHistoryList       list of previously loaded components - will be edited
     * @param aUseLibBrowser     bool to call the library viewer to select the component
     * @param aUnit              preselected unit
     * @param aConvert           preselected De Morgan shape
     * @param aHighlight         name of component to highlight in the list.
     *                           highlights none if there isn't one by that name
     * @param aShowFootprints    whether to show footprints in the dialog
     * @param aAllowFields       whether to allow field editing in the dialog
     *
     * @return the selected component
     */
    COMPONENT_SELECTION SelectCompFromLibTree(
            const SCHLIB_FILTER* aFilter,
            std::vector<COMPONENT_SELECTION>& aHistoryList,
            bool aUseLibBrowser,
            int aUnit,
            int aConvert,
            bool aShowFootprints,
            const LIB_ID* aHighlight = nullptr,
            bool aAllowFields = true );

    /**
     * Return a template field names list for read only access.
     */
    const TEMPLATE_FIELDNAMES& GetTemplateFieldNames() const
    {
        return m_templateFieldNames.GetTemplateFieldNames();
    }

    /**
     * Search for \a aName in the the template field name list.
     *
     * @param aName A wxString object containing the field name to search for.
     * @return the template fieldname if found; NULL otherwise.
     */
    const TEMPLATE_FIELDNAME* GetTemplateFieldName( const wxString& aName ) const
    {
        return m_templateFieldNames.GetFieldName( aName );
    }

    /**
     * Load symbol from symbol library table.
     *
     * @param aLibId is the symbol library identifier to load.
     * @param aUseCacheLib set to true to fall back to cache library if symbol is not found in
     *                     symbol library table.
     * @param aShowErrorMessage set to true to show any error messages.
     * @return The symbol found in the library or NULL if the symbol was not found.
     */
    LIB_ALIAS* GetLibAlias( const LIB_ID& aLibId, bool aUseCacheLib = false,
                            bool aShowError = false );

    LIB_PART* GetLibPart( const LIB_ID& aLibId, bool aUseCacheLib = false,
                          bool aShowErrorMsg = false );

    /**
     * Function SelectComponentFromLibBrowser
     * Calls the library viewer to select component to import into schematic.
     * if the library viewer is currently running, it is closed and reopened
     * in modal mode.
     * @param aParent is the caller
     * @param aFilter is a filter to pass the allowed library names
     *          and/or some other filter
     * @param aPreselectedLibId Preselected component LIB_ID. Not valid if none selected.
     * @param aUnit             preselected unit
     * @param aConvert          preselected deMorgan conversion
     * @return the selected component
     */
    COMPONENT_SELECTION SelectComponentFromLibBrowser( wxTopLevelWindow* aParent,
                                                       const SCHLIB_FILTER* aFilter,
                                                       const LIB_ID& aPreselectedLibid,
                                                       int aUnit, int aConvert );

    virtual void RedrawScreen( const wxPoint& aCenterPoint, bool aWarpPointer );

    virtual void CenterScreen( const wxPoint& aCenterPoint, bool aWarpPointer );

    void HardRedraw() override;

    /**
     * Add an item to the screen (and view)
     * aScreen is the screen the item is located on, if not the current screen
     */
    void AddToScreen( EDA_ITEM* aItem, SCH_SCREEN* aScreen = nullptr );

    /**
     * Remove an item from the screen (and view)
     * aScreen is the screen the item is located on, if not the current screen
     */
    void RemoveFromScreen( EDA_ITEM* aItem, SCH_SCREEN* aScreen = nullptr );

    /**
     * Mark an item for refresh.
     */
    void RefreshItem( EDA_ITEM* aItem, bool isAddOrDelete = false );

    /**
     * Mark selected items for refresh.
     */
    void RefreshSelection();

    /**
     * Mark all items for refresh.
     */
    void SyncView();

    /**
     * Must be called after a model change in order to set the "modify" flag and
     * do other frame-specific processing.
     */
    virtual void OnModify() {}

protected:
    /**
     * Saves Symbol Library Tables to disk.
     *
     * @param aGlobal when true, the Global Table is saved.
     * @param aProject when true, the Project Table is saved.
     * @return True when all requested actions succeeded.
     */
    bool saveSymbolLibTables( bool aGlobal, bool aProject );

};

#endif // SCH_BASE_FRAME_H_
