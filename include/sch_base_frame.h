#ifndef SCH_BASE_FRAME_H_
#define SCH_BASE_FRAME_H_
/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 KiCad Developers, see change_log.txt for contributors.
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
public:
    SCH_BASE_FRAME( KIWAY* aKiway, wxWindow* aParent,
                    FRAME_T aWindowType,
                    const wxString& aTitle,
                    const wxPoint& aPosition, const wxSize& aSize,
                    long aStyle, const wxString & aFrameName );

    SCH_SCREEN* GetScreen() const;                              // overload EDA_DRAW_FRAME

    void SetPageSettings( const PAGE_INFO& aPageSettings );     // overload EDA_DRAW_FRAME
    const PAGE_INFO& GetPageSettings () const;                  // overload EDA_DRAW_FRAME
    const wxSize GetPageSizeIU() const;                         // overload EDA_DRAW_FRAME

    const wxPoint& GetAuxOrigin() const;                        // overload EDA_DRAW_FRAME
    void SetAuxOrigin( const wxPoint& aPosition );              // overload EDA_DRAW_FRAME

    const wxPoint& GetGridOrigin() const                        // overload EDA_DRAW_FRAME
    {
        static wxPoint zero;
        return zero;
    }
    void SetGridOrigin( const wxPoint& aPoint )   {}            // overload EDA_DRAW_FRAME

    const TITLE_BLOCK& GetTitleBlock() const;                   // overload EDA_DRAW_FRAME
    void SetTitleBlock( const TITLE_BLOCK& aTitleBlock );       // overload EDA_DRAW_FRAME

    void UpdateStatusBar();                                     // overload EDA_DRAW_FRAME

    /**
     * Function SelectComponentFromLib
     * Calls the library viewer to select component to import into schematic.
     * if the library viewer is currently running, it is closed and reopened
     * in modal mode.
     * @param aLibname           the lib name or an empty string.
     *                              if aLibname is empty, the full list of libraries is used
     * @param aHistoryList       list of previously loaded components
     * @param aHistoryLastUnit   remembering last unit in last component.
     * @param aUseLibBrowser     bool to call the library viewer to select the component
     * @param aUnit              a pointer to int to return the selected unit (if any)
     * @param aConvert           a pointer to int to return the selected De Morgan shape (if any)
     *
     * @return the component name
     */
    wxString SelectComponentFromLibrary( const wxString& aLibname,
                                         wxArrayString&  aHistoryList,
                                         int&            aHistoryLastUnit,
                                         bool            aUseLibBrowser,
                                         int*            aUnit,
                                         int*            aConvert );

protected:

    /**
     * Function SelectComponentFromLibBrowser
     * Calls the library viewer to select component to import into schematic.
     * if the library viewer is currently running, it is closed and reopened
     * in modal mode.
     * @param aPreslectedAlias Preselected component alias. NULL if none.
     * @param aUnit            Pointer to Unit-number. Input is the pre-selected unit, output
     *                         is the finally selected unit by the user. Can be NULL.
     * @param aConvert         Pointer to deMorgan conversion. Input is what is pre-selected,
     *                         output is the finally selected deMorgan type by the user.
     * @return the component name
     */
    wxString SelectComponentFromLibBrowser( LIB_ALIAS* aPreselectedAlias,
                                            int* aUnit, int* aConvert );

    /**
     * Function OnOpenLibraryViewer
     * Open the library viewer only to browse library contents.
     * If the viewed is already opened from this, raise the viewer
     * If the viewed is already opened from an other window, close it and reopen
     */
    void OnOpenLibraryViewer( wxCommandEvent& event );
};

#endif // SCH_BASE_FRAME_H_
