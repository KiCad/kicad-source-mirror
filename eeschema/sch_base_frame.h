#ifndef SCH_BASE_FRAME_H_
#define SCH_BASE_FRAME_H_
/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2015-2017 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <draw_frame.h>
#include <class_sch_screen.h>

class PAGE_INFO;
class TITLE_BLOCK;
class LIB_VIEW_FRAME;
class LIB_EDIT_FRAME;
class LIB_ALIAS;
class PART_LIB;
class SCHLIB_FILTER;

/**
 * Class SCH_BASE_FRAME
 * is a shim class between EDA_DRAW_FRAME and several derived classes:
 * LIB_EDIT_FRAME, LIB_VIEW_FRAME, and SCH_EDIT_FRAME, and it brings in a
 * common way of handling the provided virtual functions for the derived classes.
 * <p>
 * The motivation here is to switch onto GetScreen() for the underlying data model.
 *
 * @author Dick Hollenbeck
 */
class SCH_BASE_FRAME : public EDA_DRAW_FRAME
{
protected:
    wxPoint  m_repeatStep;          ///< the increment value of the position of an item
                                    ///< when it is repeated
    int      m_repeatDeltaLabel;    ///< the increment value of labels like bus members
                                    ///< when they are repeated


public:
    SCH_BASE_FRAME( KIWAY* aKiway, wxWindow* aParent,
                    FRAME_T aWindowType,
                    const wxString& aTitle,
                    const wxPoint& aPosition, const wxSize& aSize,
                    long aStyle, const wxString & aFrameName );

    SCH_SCREEN* GetScreen() const override;

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

    // Virtual from EDA_DRAW_FRAME
    // the background color of the draw canvas:
    COLOR4D GetDrawBgColor() const override;
    void SetDrawBgColor( COLOR4D aColor) override;

    const TITLE_BLOCK& GetTitleBlock() const override;
    void SetTitleBlock( const TITLE_BLOCK& aTitleBlock ) override;

    void UpdateStatusBar() override;


    struct COMPONENT_SELECTION
    {
        wxString    Name;
        int         Unit;
        int         Convert;

        std::vector<std::pair<int, wxString>>   Fields;

        COMPONENT_SELECTION():
            Name(""),
            Unit(1),
            Convert(1)
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
     * @param aAllowFields       whether to allow field editing in the dialog
     *
     * @return the selected component
     */
    COMPONENT_SELECTION SelectComponentFromLibrary(
            const SCHLIB_FILTER*                aFilter,
            std::vector<COMPONENT_SELECTION>&   aHistoryList,
            bool                                aUseLibBrowser,
            int                                 aUnit,
            int                                 aConvert,
            const wxString& aHighlight = wxEmptyString,
            bool                                aAllowFields = true );

protected:

    /**
     * Function SelectComponentFromLibBrowser
     * Calls the library viewer to select component to import into schematic.
     * if the library viewer is currently running, it is closed and reopened
     * in modal mode.
     * @param aFilter is a filter to pass the allowed library names
     *          and/or some other filter
     * @param aPreselectedAlias Preselected component alias. NULL if none.
     * @param aUnit             preselected unit
     * @param aConvert          preselected deMorgan conversion
     * @return the selected component
     */
    COMPONENT_SELECTION SelectComponentFromLibBrowser(
            const SCHLIB_FILTER* aFilter,
            LIB_ALIAS* aPreselectedAlias,
            int aUnit, int aConvert );

    /**
     * Function OnOpenLibraryViewer
     * Open the library viewer only to browse library contents.
     * If the viewed is already opened from this, raise the viewer
     * If the viewed is already opened from an other window, close it and reopen
     */
    void OnOpenLibraryViewer( wxCommandEvent& event );

    /**
     * Function DisplayComponentsNamesInLib
     * Select a component from the list of components in a library
     *
     * @param aLibrary = a reference to the library to explore
     *                If NULL the user will be prompted tp chose a library
     * @param aBuffer = a wxString to put the selected component name
     *
     * @return true if a component is selected
     *         false on cancel
     */
    bool DisplayListComponentsInLib( PART_LIB*  aLibrary, wxString&  aBuffer,
                                     wxString&  aPreviousChoice );

    /**
     * Function SelectLibraryFromList
     * displays a list of current loaded libraries, and allows the user to select
     * a library
     * This list is sorted, with the library cache always at end of the list
     * @return a reference to the selected library, or NULL
     */
    PART_LIB* SelectLibraryFromList();

    /**
     * Function SelectPartNameToLoad
     * Select a part name from the list of components (parts) found in a library.
     *
     * @param aLibrary = a reference to the library to explore
     *                If NULL the user will be prompted tp chose a library
     * @param aBufName = a wxString to put the selected component name
     *
     * @return true if a component is selected
     *         false on cancel
     */
    bool SelectPartNameToLoad( PART_LIB* aLibrary, wxString& aBufName );
};

#endif // SCH_BASE_FRAME_H_
