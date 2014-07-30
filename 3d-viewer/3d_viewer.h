/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2011 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file 3d_viewer.h
 */

#ifndef __3D_VIEWER_H__
#define __3D_VIEWER_H__

#include <draw_frame.h>

#if !wxUSE_GLCANVAS
#error Please build wxWidgets with Opengl support (./configure --with-opengl)
#endif

#include <wx/glcanvas.h>
#include <3d_struct.h>

#define KISYS3DMOD "KISYS3DMOD"

class EDA_3D_CANVAS;
class PCB_BASE_FRAME;

#define KICAD_DEFAULT_3D_DRAWFRAME_STYLE    (wxDEFAULT_FRAME_STYLE | wxWANTS_CHARS)
#define LIB3D_PATH                          wxT( "packages3d" )


class EDA_3D_FRAME : public KIWAY_PLAYER
{
private:
    EDA_3D_CANVAS*  m_canvas;
    bool            m_reloadRequest;
    wxString        m_defaultFileName;  /// Filename to propose for screenshot

    /// Tracks whether to use Orthographic or Perspective projection
    bool            m_ortho;

public:
    EDA_3D_FRAME( KIWAY* aKiway, PCB_BASE_FRAME* aParent, const wxString& aTitle,
                  long style = KICAD_DEFAULT_3D_DRAWFRAME_STYLE );

    ~EDA_3D_FRAME()
    {
        m_auimgr.UnInit();
    };

    PCB_BASE_FRAME* Parent() const { return (PCB_BASE_FRAME*)GetParent(); }

    BOARD* GetBoard();

    /**
     * Function ReloadRequest
     * must be called when reloading data from Pcbnew is needed
     * mainly after edition of the board or footprint being displayed.
     * mainly for the module editor.
     */
    void ReloadRequest( )
    {
        m_reloadRequest = true;
    }

    /**
     * Function NewDisplay
     * Rebuild the display list.
     * must be called when 3D opengl data is modified
     * @param aGlList = the list to rebuild.
     * if 0 (default) all lists are rebuilt
     */
    void NewDisplay( int aGlList = 0 );

    void SetDefaultFileName(const wxString &aFn) { m_defaultFileName = aFn; }
    const wxString &GetDefaultFileName() const { return m_defaultFileName; }

    /// Toggles orthographic projection on and off
    void ToggleOrtho(){ m_ortho = !m_ortho ; Refresh(true);};

    /// Returns the orthographic projection flag
    bool ModeIsOrtho() { return m_ortho ;};

private:
    // Event handlers:
    void Exit3DFrame( wxCommandEvent& event );
    void OnCloseWindow( wxCloseEvent& Event );
    void Process_Special_Functions( wxCommandEvent& event );
    void On3DGridSelection( wxCommandEvent& event );
    void Process_Zoom( wxCommandEvent& event );
    void OnActivate( wxActivateEvent& event );
    void Install_3D_ViewOptionDialog( wxCommandEvent& event );

    // initialisation
    void CreateMenuBar();
    void SetMenuBarOptionsState();  // Set the state of toggle menus according
                                    // to the current display options
    void ReCreateMainToolbar();
    void SetToolbars();

    void LoadSettings( wxConfigBase* aCfg );    // overload virtual
    void SaveSettings( wxConfigBase* aCfg );    // overload virtual

    // Other functions
    void OnLeftClick( wxDC* DC, const wxPoint& MousePos );
    void OnRightClick( const wxPoint& MousePos, wxMenu* PopMenu );
    void OnKeyEvent( wxKeyEvent& event );
    double BestZoom();
    void RedrawActiveWindow( wxDC* DC, bool EraseBg );

    void Set3DBgColor();

    DECLARE_EVENT_TABLE()
};

#endif  /*  __3D_VIEWER_H__ */
