/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jon Evans <jon@craftyjon.com>
 * Copyright (C) 2017-2020 KiCad Developers, see AUTHORS.txt for contributors.
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <eda_item.h>
#include <bitmaps.h>
#include <class_draw_panel_gal.h>
#include <dialogs/dialog_layers_select_to_pcb.h>
#include <gestfich.h>
#include <gerber_file_image.h>
#include <gerbview_id.h>
#include "gerbview_inspection_tool.h"
#include "gerbview_actions.h"
#include <painter.h>
#include <preview_items/ruler_item.h>
#include <preview_items/selection_area.h>
#include <tool/tool_event.h>
#include <tool/tool_manager.h>
#include <view/view.h>
#include <view/view_controls.h>
#include <view/view_group.h>
#include <wx/msgdlg.h>
#include <wx/textdlg.h>
#include <wx/choicdlg.h>


GERBVIEW_INSPECTION_TOOL::GERBVIEW_INSPECTION_TOOL() :
        TOOL_INTERACTIVE( "gerbview.Inspection" ),
        m_frame( nullptr )
{
}


GERBVIEW_INSPECTION_TOOL::~GERBVIEW_INSPECTION_TOOL()
{
}


bool GERBVIEW_INSPECTION_TOOL::Init()
{
    return true;
}


void GERBVIEW_INSPECTION_TOOL::Reset( RESET_REASON aReason )
{
    m_frame = getEditFrame<GERBVIEW_FRAME>();
}


int GERBVIEW_INSPECTION_TOOL::ShowDCodes( const TOOL_EVENT& aEvent )
{
    int             ii, jj;
    wxString        Line;
    wxArrayString   list;
    int             curr_layer = m_frame->GetActiveLayer();

    double   scale = 1.0;
    wxString units;

    switch( m_frame->GetUserUnits() )
    {
        case EDA_UNITS::MILLIMETRES:
            scale = IU_PER_MM;
            units = "mm";
            break;

        case EDA_UNITS::INCHES:
            scale = IU_PER_MILS * 1000;
            units = "in";
            break;

        case EDA_UNITS::MILS:
            scale = IU_PER_MILS;
            units = "mil";
            break;

        default:
            wxASSERT_MSG( false, "Invalid units" );
    }

    for( unsigned int layer = 0; layer < m_frame->ImagesMaxCount(); ++layer )
    {
        GERBER_FILE_IMAGE* gerber = m_frame->GetGbrImage( layer );

        if( !gerber )
            continue;

        if( gerber->GetDcodesCount() == 0 )
            continue;

        if( curr_layer == static_cast<int>( layer ) )
            Line.Printf( wxT( "*** Active layer (%2.2d) ***" ), layer + 1 );
        else
            Line.Printf( wxT( "*** layer %2.2d  ***" ), layer + 1 );

        list.Add( Line );

        for( ii = 0, jj = 1; ii < TOOLS_MAX_COUNT; ii++ )
        {
            D_CODE* pt_D_code = gerber->GetDCODE( ii + FIRST_DCODE );

            if( pt_D_code == NULL )
                continue;

            if( !pt_D_code->m_InUse && !pt_D_code->m_Defined )
                continue;

            Line.Printf( wxT( "tool %2.2d:   D%2.2d   V %.4f %s  H %.4f %s   %s  attribute '%s'" ),
                         jj,
                         pt_D_code->m_Num_Dcode,
                         pt_D_code->m_Size.y / scale, units,
                         pt_D_code->m_Size.x / scale, units,
                         D_CODE::ShowApertureType( pt_D_code->m_Shape ),
                         pt_D_code->m_AperFunction.IsEmpty()? wxT( "none" ) : pt_D_code->m_AperFunction
                         );

            if( !pt_D_code->m_Defined )
                Line += wxT( " (not defined)" );

            if( pt_D_code->m_InUse )
                Line += wxT( " (in use)" );

            list.Add( Line );
            jj++;
        }
    }

    wxSingleChoiceDialog dlg( m_frame, wxEmptyString, _( "D Codes" ), list, (void**) NULL,
                              wxCHOICEDLG_STYLE & ~wxCANCEL );

    dlg.ShowModal();

    return 0;
}


int GERBVIEW_INSPECTION_TOOL::ShowSource( const TOOL_EVENT& aEvent )
{
    int                layer        = m_frame->GetActiveLayer();
    GERBER_FILE_IMAGE* gerber_layer = m_frame->GetGbrImage( layer );

    if( gerber_layer )
    {
        wxString editorname = Pgm().GetEditorName();

        if( !editorname.IsEmpty() )
        {
            wxFileName fn( gerber_layer->m_FileName );

            // Call the editor only if the Gerber/drill source file is available.
            // This is not always the case, because it can be a temporary file
            // if it comes from a zip archive.
            if( !fn.FileExists() )
            {
                wxString msg;
                msg.Printf( _( "Source file \"%s\" is not available" ), fn.GetFullPath() );
                wxMessageBox( msg );
            }
            else
            {
                ExecuteFile( m_frame, editorname, QuoteFullPath( fn ) );
            }
        }
        else
        {
            wxMessageBox( _( "No editor defined. Please select one" ) );
        }
    }
    else
    {
        wxString msg;
        msg.Printf( _( "No file loaded on the active layer %d" ), layer + 1 );
        wxMessageBox( msg );
    }

    return 0;
}


int GERBVIEW_INSPECTION_TOOL::MeasureTool( const TOOL_EVENT& aEvent )
{
    KIGFX::VIEW&          view = *getView();
    KIGFX::VIEW_CONTROLS& controls = *getViewControls();
    KIGFX::VC_SETTINGS    previous_settings = controls.GetSettings();

    std::string tool = aEvent.GetCommandStr().get();
    m_frame->PushTool( tool );
    Activate();

    KIGFX::PREVIEW::TWO_POINT_GEOMETRY_MANAGER twoPtMgr;

    EDA_UNITS                  units = m_frame->GetUserUnits();
    KIGFX::PREVIEW::RULER_ITEM ruler( twoPtMgr, units );

    view.Add( &ruler );
    view.SetVisible( &ruler, false );

    bool originSet = false;

    controls.ShowCursor( true );

    auto setCursor =
            [&]()
            {
                m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::MEASURE );
            };

    // Set initial cursor
    setCursor();

    while( TOOL_EVENT* evt = Wait() )
    {
        setCursor();
        const VECTOR2I cursorPos = controls.GetCursorPosition();

        auto clearRuler =
                [&] ()
                {
                    view.SetVisible( &ruler, false );
                    controls.SetAutoPan( false );
                    controls.CaptureCursor( false );
                    originSet = false;
                };

        if( evt->IsCancelInteractive() )
        {
            if( originSet )
            {
                clearRuler();
            }
            else
            {
                m_frame->PopTool( tool );
                break;
            }
        }
        else if( evt->IsActivate() )
        {
            if( originSet )
                clearRuler();

            if( evt->IsMoveTool() )
            {
                // leave ourselves on the stack so we come back after the move
                break;
            }
            else
            {
                m_frame->PopTool( tool );
                break;
            }
        }
        // click or drag starts
        else if( !originSet && ( evt->IsDrag( BUT_LEFT ) || evt->IsClick( BUT_LEFT ) ) )
        {
            twoPtMgr.SetOrigin( cursorPos );
            twoPtMgr.SetEnd( cursorPos );

            controls.CaptureCursor( true );
            controls.SetAutoPan( true );

            originSet = true;
        }
        // second click or mouse up after drag ends
        else if( originSet && ( evt->IsClick( BUT_LEFT ) || evt->IsMouseUp( BUT_LEFT ) ) )
        {
            originSet = false;

            controls.SetAutoPan( false );
            controls.CaptureCursor( false );
        }
        // move or drag when origin set updates rules
        else if( originSet && ( evt->IsMotion() || evt->IsDrag( BUT_LEFT ) ) )
        {
            twoPtMgr.SetAngleSnap( evt->Modifier( MD_SHIFT ) );
            twoPtMgr.SetEnd( cursorPos );

            view.SetVisible( &ruler, true );
            view.Update( &ruler, KIGFX::GEOMETRY );
        }
        else if( evt->IsAction( &ACTIONS::updateUnits ) )
        {
            if( m_frame->GetUserUnits() != units )
            {
                units = m_frame->GetUserUnits();
                ruler.SwitchUnits( units );
                view.Update( &ruler, KIGFX::GEOMETRY );
            }
            evt->SetPassEvent();
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            m_menu.ShowContextMenu( m_frame->GetCurrentSelection() );
        }
        else
        {
            evt->SetPassEvent();
        }
    }

    view.SetVisible( &ruler, false );
    view.Remove( &ruler );

    controls.ApplySettings( previous_settings );
    m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::ARROW );
    return 0;
}


void GERBVIEW_INSPECTION_TOOL::setTransitions()
{
    Go( &GERBVIEW_INSPECTION_TOOL::ShowSource,     GERBVIEW_ACTIONS::showSource.MakeEvent() );
    Go( &GERBVIEW_INSPECTION_TOOL::ShowDCodes,     GERBVIEW_ACTIONS::showDCodes.MakeEvent() );
    Go( &GERBVIEW_INSPECTION_TOOL::MeasureTool,    ACTIONS::measureTool.MakeEvent() );
}
