/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jean-pierre.charras at wanadoo.fr
 * Copyright (C) 1992-2016 KiCad Developers, see AUTHORS.txt for contributors.
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

/* functions relatives to the dialog opened from the main menu :
    Preferences/display
*/

#include <fctsys.h>
#include <class_drawpanel.h>
#include <confirm.h>
#include <pcbnew.h>
#include <wxPcbStruct.h>
#include <pcbstruct.h>
#include <incremental_text_ctrl.h>
#include <config_map.h>

#include <pcbnew_id.h>

#include <dialog_display_options.h>
#include <dialog_display_options_base.h>

#include <class_draw_panel_gal.h>
#include <view/view.h>
#include <pcb_painter.h>
#include <gal/gal_display_options.h>

/*
 * Spin control parameters
 */
static const double gridThicknessMin = 0.5;
static const double gridThicknessMax = 10.0;
static const double gridThicknessStep = 0.5;

static const double gridMinSpacingMin = 5;
static const double gridMinSpacingMax = 200;
static const double gridMinSpacingStep = 5;


static const UTIL::CFG_MAP<KIGFX::GRID_STYLE> gridStyleSelectMap =
{
    { KIGFX::GRID_STYLE::DOTS,     0 },    // Default
    { KIGFX::GRID_STYLE::LINES,    1 },
};


static const UTIL::CFG_MAP<TRACE_CLEARANCE_DISPLAY_MODE_T> traceClearanceSelectMap =
{
    { SHOW_CLEARANCE_NEW_TRACKS_AND_VIA_AREAS,            2 },     // Default
    { DO_NOT_SHOW_CLEARANCE,                              0 },
    { SHOW_CLEARANCE_NEW_TRACKS,                          1 },
    { SHOW_CLEARANCE_NEW_AND_EDITED_TRACKS_AND_VIA_AREAS, 3 },
    { SHOW_CLEARANCE_ALWAYS,                              4 },
};


static const UTIL::CFG_MAP<KIGFX::OPENGL_ANTIALIASING_MODE> aaModeSelectMap =
{
    { KIGFX::OPENGL_ANTIALIASING_MODE::NONE,              0 },    // Default
    { KIGFX::OPENGL_ANTIALIASING_MODE::SUBSAMPLE_HIGH,    1 },
    { KIGFX::OPENGL_ANTIALIASING_MODE::SUBSAMPLE_ULTRA,   2 },
    { KIGFX::OPENGL_ANTIALIASING_MODE::SUPERSAMPLING_X2,  3 },
    { KIGFX::OPENGL_ANTIALIASING_MODE::SUPERSAMPLING_X4,  4 },
};


void PCB_EDIT_FRAME::InstallDisplayOptionsDialog( wxCommandEvent& aEvent )
{
    DIALOG_DISPLAY_OPTIONS dlg( this );
    dlg.ShowModal();
}


DIALOG_DISPLAY_OPTIONS::DIALOG_DISPLAY_OPTIONS( PCB_EDIT_FRAME* parent ) :
    DIALOG_DISPLAY_OPTIONS_BASE( parent )
{
    m_Parent = parent;

    // bind the spin button and text box
    m_gridSizeIncrementer = std::make_unique<SPIN_INCREMENTAL_TEXT_CTRL>(
                *m_gridLineWidthSpinBtn, *m_gridLineWidth );

    m_gridSizeIncrementer->SetStep( gridThicknessMin, gridThicknessMax,
                                    gridThicknessStep );
    m_gridSizeIncrementer->SetPrecision( 1 );

    m_gridMinSpacingIncrementer = std::make_unique<SPIN_INCREMENTAL_TEXT_CTRL>(
                *m_gridMinSpacingSpinBtn, *m_gridMinSpacing );

    m_gridMinSpacingIncrementer->SetStep( gridMinSpacingMin, gridMinSpacingMax,
                                          gridMinSpacingStep );
    m_gridMinSpacingIncrementer->SetPrecision( 0 ); // restrict to ints

    // load settings into controls
    init();

    m_sdbSizerOK->SetDefault();

    // Now all widgets have the size fixed, call FinishDialogSettings
    FinishDialogSettings();
}


void DIALOG_DISPLAY_OPTIONS::init()
{
    SetFocus();
    const DISPLAY_OPTIONS* displ_opts = (DISPLAY_OPTIONS*)m_Parent->GetDisplayOptions();
    const KIGFX::GAL_DISPLAY_OPTIONS& gal_opts = m_Parent->GetGalDisplayOptions();

    m_OptDisplayTracks->SetValue( displ_opts->m_DisplayPcbTrackFill == SKETCH );

    m_OptDisplayTracksClearance->SetSelection( UTIL::GetConfigForVal(
            traceClearanceSelectMap, displ_opts->m_ShowTrackClearanceMode ) );

    m_OptDisplayPads->SetValue( displ_opts->m_DisplayPadFill == SKETCH );
    m_OptDisplayVias->SetValue( displ_opts->m_DisplayViaFill == SKETCH );

    m_Show_Page_Limits->SetValue( m_Parent->ShowPageLimits() );

    m_OptDisplayModTexts->SetValue( displ_opts->m_DisplayModTextFill == SKETCH );
    m_OptDisplayModOutlines->SetValue( displ_opts->m_DisplayModEdgeFill == SKETCH );
    m_OptDisplayPadClearence->SetValue( displ_opts->m_DisplayPadIsol );
    m_OptDisplayPadNumber->SetValue( displ_opts->m_DisplayPadNum );
    m_OptDisplayPadNoConn->SetValue( m_Parent->IsElementVisible( PCB_VISIBLE( NO_CONNECTS_VISIBLE ) ) );
    m_OptDisplayDrawings->SetValue( displ_opts->m_DisplayDrawItemsFill == SKETCH );
    m_ShowNetNamesOption->SetSelection( displ_opts->m_DisplayNetNamesMode );

    m_choiceAntialiasing->SetSelection( UTIL::GetConfigForVal(
            aaModeSelectMap, gal_opts.gl_antialiasing_mode ) );

    m_gridStyle->SetSelection( UTIL::GetConfigForVal(
            gridStyleSelectMap, gal_opts.m_gridStyle ) );

    m_gridSizeIncrementer->SetValue( gal_opts.m_gridLineWidth );

    m_gridMinSpacingIncrementer->SetValue( gal_opts.m_gridMinSpacing );
}


void DIALOG_DISPLAY_OPTIONS::OnCancelClick( wxCommandEvent& event )
{
    EndModal( 0 );
}


/*
 * Update variables with new options
 */
void DIALOG_DISPLAY_OPTIONS::OnOkClick( wxCommandEvent& event )
{
    DISPLAY_OPTIONS* displ_opts = (DISPLAY_OPTIONS*)m_Parent->GetDisplayOptions();
    KIGFX::GAL_DISPLAY_OPTIONS& gal_opts = m_Parent->GetGalDisplayOptions();

    m_Parent->SetShowPageLimits( m_Show_Page_Limits->GetValue() );

    displ_opts->m_DisplayPcbTrackFill = not m_OptDisplayTracks->GetValue();

    displ_opts->m_ShowTrackClearanceMode = UTIL::GetValFromConfig(
            traceClearanceSelectMap, m_OptDisplayTracksClearance->GetSelection() );

    displ_opts->m_DisplayModTextFill = not m_OptDisplayModTexts->GetValue();
    displ_opts->m_DisplayModEdgeFill = not m_OptDisplayModOutlines->GetValue();

    displ_opts->m_DisplayPadFill = not m_OptDisplayPads->GetValue();
    displ_opts->m_DisplayViaFill = not m_OptDisplayVias->GetValue();

    displ_opts->m_DisplayPadIsol = m_OptDisplayPadClearence->GetValue();

    displ_opts->m_DisplayPadNum = m_OptDisplayPadNumber->GetValue();

    m_Parent->SetElementVisibility( PCB_VISIBLE( NO_CONNECTS_VISIBLE ),
                                    m_OptDisplayPadNoConn->GetValue() );

    displ_opts->m_DisplayDrawItemsFill = not m_OptDisplayDrawings->GetValue();
    displ_opts->m_DisplayNetNamesMode = m_ShowNetNamesOption->GetSelection();

    gal_opts.gl_antialiasing_mode = UTIL::GetValFromConfig(
            aaModeSelectMap, m_choiceAntialiasing->GetSelection() );

    gal_opts.m_gridStyle = UTIL::GetValFromConfig(
            gridStyleSelectMap, m_gridStyle->GetSelection() );

    gal_opts.m_gridLineWidth = m_gridSizeIncrementer->GetValue();

    gal_opts.m_gridMinSpacing = m_gridMinSpacingIncrementer->GetValue();

    gal_opts.NotifyChanged();

    // Apply changes to the GAL
    KIGFX::VIEW* view = m_Parent->GetGalCanvas()->GetView();
    KIGFX::PCB_PAINTER* painter = static_cast<KIGFX::PCB_PAINTER*>( view->GetPainter() );
    KIGFX::PCB_RENDER_SETTINGS* settings =
            static_cast<KIGFX::PCB_RENDER_SETTINGS*>( painter->GetSettings() );
    settings->LoadDisplayOptions( displ_opts );
    view->RecacheAllItems();
    view->MarkTargetDirty( KIGFX::TARGET_NONCACHED );

    m_Parent->GetCanvas()->Refresh();

    EndModal( 1 );
}
