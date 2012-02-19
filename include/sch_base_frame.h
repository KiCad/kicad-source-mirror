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

#include <wxstruct.h>
#include <class_sch_screen.h>

class PAGE_INFO;
class TITLE_BLOCK;
class LIB_VIEW_FRAME;
class LIB_EDIT_FRAME;


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
    LIB_VIEW_FRAME*       m_ViewlibFrame;       // A library viewer, common to SCH_EDIT_FRAME
                                                // and LIB_EDIT_FRAME.
                                                // has no meaning to LIB_VIEW_FRAME
    LIB_EDIT_FRAME*       m_LibeditFrame;       // the library editor.
                                                // Is used mainly in SCH_EDIT_FRAME
                                                // but is defined here because some functions
                                                // need to know if a library editor or a libray
                                                // viewer is currently opened

public:
    SCH_BASE_FRAME( wxWindow* aParent,
                    id_drawframe aWindowType,
                    const wxString& aTitle,
                    const wxPoint& aPosition, const wxSize& aSize,
                    long aStyle = KICAD_DEFAULT_DRAWFRAME_STYLE );

    SCH_SCREEN* GetScreen() const;                              // overload EDA_DRAW_FRAME

    void SetLibraryViewerWindow( LIB_VIEW_FRAME* aFrame ) { m_ViewlibFrame = aFrame; }

    void SetPageSettings( const PAGE_INFO& aPageSettings );     // overload EDA_DRAW_FRAME
    const PAGE_INFO& GetPageSettings () const;                  // overload EDA_DRAW_FRAME
    const wxSize GetPageSizeIU() const;                         // overload EDA_DRAW_FRAME

    const wxPoint& GetOriginAxisPosition() const;               // overload EDA_DRAW_FRAME
    void SetOriginAxisPosition( const wxPoint& aPosition );     // overload EDA_DRAW_FRAME

    const TITLE_BLOCK& GetTitleBlock() const;                   // overload EDA_DRAW_FRAME
    void SetTitleBlock( const TITLE_BLOCK& aTitleBlock );       // overload EDA_DRAW_FRAME

protected:

    /**
     * Function SelectComponentFromLibBrowser
     * Calls the library viewer to select component to import into schematic.
     * if the library viewer is currently running, it is closed and reopened
     * in modal mode.
     * @return the component name
     */
    wxString SelectComponentFromLibBrowser( void );

    /**
     * Function SelectComponentFromLib
     * Calls the library viewer to select component to import into schematic.
     * if the library viewer is currently running, it is closed and reopened
     * in modal mode.
     * @param aLibname = the lib name or an empty string.
     *     if aLibname is empty, the full list of libraries is used
     * @param aHistoryList = list of previously loaded components
     * @param aUseLibBrowser = bool to call the library viewer to select the component
     * @param aUnit = a point to int to return the selected unit (if any)
     * @param aConvert = a point to int to return the selected De Morgan shape (if any)
     *
     * @return the component name
     */
    wxString SelectComponentFromLibrary( const wxString& aLibname,
                                         wxArrayString&  aHistoryList,
                                         bool            aUseLibBrowser,
                                         int*            aUnit,
                                         int*            aConvert );

    /**
     * Function OnOpenLibraryViewer
     * Open the library viewer only to browse library contents.
     * If the viewed is already opened from this, raise the viewer
     * If the viewed is already opened from an other window, close it and reopen
     */
    void OnOpenLibraryViewer( wxCommandEvent& event );
};

#endif // SCH_BASE_FRAME_H_
