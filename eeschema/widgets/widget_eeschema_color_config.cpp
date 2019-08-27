/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2016-2018 KiCad Developers, see AUTHORS.txt for contributors.
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

/* Set up colors to draw items in Eeschema
 */

#include <fctsys.h>
#include <eda_draw_frame.h>
#include <sch_draw_panel.h>
#include <sch_view.h>
#include <general.h>

#include "widget_eeschema_color_config.h"
#include <dialogs/dialog_color_picker.h>

// Width and height of every (color-displaying / bitmap) button in dialog units
const wxSize BUTT_SIZE( 10, 6 );
const wxSize BUTT_BORDER( 4, 4 );

/********************/
/* Layer menu list. */
/********************/

struct COLORBUTTON
{
    wxString        m_Name;
    int             m_Layer;
};

struct BUTTONINDEX
{
    wxString        m_Name;
    COLORBUTTON*    m_Buttons;
};

static COLORBUTTON generalColorButtons[] = {
    { _( "Wire" ),              LAYER_WIRE },
    { _( "Bus" ),               LAYER_BUS },
    { _( "Junction" ),          LAYER_JUNCTION },
    { _( "Label" ),             LAYER_LOCLABEL },
    { _( "Global label" ),      LAYER_GLOBLABEL },
    { _( "Net name" ),          LAYER_NETNAM },
    { _( "Notes" ),             LAYER_NOTES },
    { _( "No connect symbol" ), LAYER_NOCONNECT },
    { wxT( "" ), -1 }                           // Sentinel marking end of list.
};

static COLORBUTTON componentColorButtons[] = {
    { _( "Body outline" ),      LAYER_DEVICE },
    { _( "Body background" ),   LAYER_DEVICE_BACKGROUND },
    { _( "Pin" ),               LAYER_PIN },
    { _( "Pin number" ),        LAYER_PINNUM },
    { _( "Pin name" ),          LAYER_PINNAM },
    { _( "Reference" ),         LAYER_REFERENCEPART },
    { _( "Value" ),             LAYER_VALUEPART },
    { _( "Fields" ),            LAYER_FIELDS },
    { wxT( "" ), -1 }                           // Sentinel marking end of list.
};

static COLORBUTTON sheetColorButtons[] = {
    { _( "Sheet" ),             LAYER_SHEET },
    { _( "Sheet file name" ),   LAYER_SHEETFILENAME },
    { _( "Sheet name" ),        LAYER_SHEETNAME },
    { _( "Sheet label" ),       LAYER_SHEETLABEL },
    { _( "Hierarchical label" ),LAYER_HIERLABEL },
    { wxT( "" ), -1 }                           // Sentinel marking end of list.
};

static COLORBUTTON miscColorButtons[] = {
    { _( "ERC warning" ),         LAYER_ERC_WARN },
    { _( "ERC error" ),           LAYER_ERC_ERR },
    { _( "Brightened" ),          LAYER_BRIGHTENED },
    { _( "Hidden items" ),        LAYER_HIDDEN },
    { _( "Worksheet" ),           LAYER_WORKSHEET },
    { _( "Cursor" ),              LAYER_SCHEMATIC_CURSOR },
    { _( "Grid" ),                LAYER_SCHEMATIC_GRID },
    { _( "Background" ),          LAYER_SCHEMATIC_BACKGROUND },
    { _( "Selection Highlight" ), LAYER_SELECTION_SHADOWS },
    { wxT( "" ), -1 }                           // Sentinel marking end of list.
};


static BUTTONINDEX buttonGroups[] = {
    { _( "General" ),           generalColorButtons },
    { _( "Component" ),         componentColorButtons },
    { _( "Sheet" ),             sheetColorButtons },
    { _( "Miscellaneous" ),     miscColorButtons },
    { wxT( "" ), NULL }
};

static COLORBUTTON bgColorButton = { "", LAYER_SCHEMATIC_BACKGROUND };

static COLOR4D currentColors[ LAYER_ID_COUNT ];


WIDGET_EESCHEMA_COLOR_CONFIG::WIDGET_EESCHEMA_COLOR_CONFIG( wxWindow* aParent, EDA_DRAW_FRAME* aDrawFrame ) :
    wxPanel( aParent ), m_drawFrame( aDrawFrame )
{
    m_butt_size_pix = ConvertDialogToPixels( BUTT_SIZE );
    m_butt_border_pix = ConvertDialogToPixels( BUTT_BORDER );

    CreateControls();
}


void WIDGET_EESCHEMA_COLOR_CONFIG::CreateControls()
{
    wxStaticText*   label;
    int             buttonId = 1800;

    m_mainBoxSizer = new wxBoxSizer( wxHORIZONTAL );
    SetSizer( m_mainBoxSizer );

    BUTTONINDEX* groups = buttonGroups;
    wxBoxSizer* columnBoxSizer = NULL;

    while( groups->m_Buttons != NULL )
    {
        COLORBUTTON* buttons = groups->m_Buttons;

        columnBoxSizer = new wxBoxSizer( wxVERTICAL );
        m_mainBoxSizer->Add( columnBoxSizer, 1, wxALIGN_TOP | wxLEFT, 5 );
        wxBoxSizer* rowBoxSizer = new wxBoxSizer( wxHORIZONTAL );
        columnBoxSizer->Add( rowBoxSizer, 0, wxGROW | wxLEFT | wxRIGHT | wxBOTTOM, 5 );

        // Add a text string to identify the column of color select buttons.
        label = new wxStaticText( this, wxID_ANY, groups->m_Name );

        // Make the column label font bold.
        wxFont font( label->GetFont() );
        font.SetWeight( wxFONTWEIGHT_BOLD );
        label->SetFont( font );

        rowBoxSizer->Add( label, 1, wxALIGN_CENTER_VERTICAL | wxALL, 5 );

        while( buttons->m_Layer >= 0 )
        {
            rowBoxSizer = new wxBoxSizer( wxHORIZONTAL );
            columnBoxSizer->Add( rowBoxSizer, 0, wxGROW | wxALL, 0 );

            COLOR4D color = GetLayerColor( SCH_LAYER_ID( buttons->m_Layer ) );
            currentColors[ buttons->m_Layer ] = color;

            wxMemoryDC iconDC;
            wxBitmap   bitmap( m_butt_size_pix );

            iconDC.SelectObject( bitmap );
            iconDC.SetPen( *wxBLACK_PEN );

            wxBrush brush;
            brush.SetColour( color.ToColour() );
            brush.SetStyle( wxBRUSHSTYLE_SOLID );
            iconDC.SetBrush( brush );
            iconDC.DrawRectangle( 0, 0, m_butt_size_pix.x, m_butt_size_pix.y );

            wxBitmapButton* bitmapButton = new wxBitmapButton(
                                    this, buttonId, bitmap, wxDefaultPosition,
                                    m_butt_size_pix + m_butt_border_pix + wxSize( 1, 1 ) );
            bitmapButton->SetClientData( (void*) buttons );

            rowBoxSizer->Add( bitmapButton, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxBOTTOM, 5 );

            label = new wxStaticText( this, wxID_ANY, wxGetTranslation( buttons->m_Name ) );
            rowBoxSizer->Add( label, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxBOTTOM, 5 );
            buttonId += 1;
            buttons++;
        }

        groups++;
    }

    Connect( 1800, buttonId, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( WIDGET_EESCHEMA_COLOR_CONFIG::SetColor ) );

    // Dialog now needs to be resized, but the associated command is found elsewhere.
}


void WIDGET_EESCHEMA_COLOR_CONFIG::SetColor( wxCommandEvent& event )
{
    wxBitmapButton* button = (wxBitmapButton*) event.GetEventObject();

    wxCHECK_RET( button != NULL, wxT( "Color button event object is NULL." ) );

    COLORBUTTON* colorButton = (COLORBUTTON*) button->GetClientData();

    wxCHECK_RET( colorButton != NULL, wxT( "Client data not set for color button." ) );
    COLOR4D oldColor = currentColors[ colorButton->m_Layer ];
    COLOR4D newColor = COLOR4D::UNSPECIFIED;
    DIALOG_COLOR_PICKER dialog( this, oldColor, false );

    if( dialog.ShowModal() == wxID_OK )
    {
        newColor = dialog.GetColor();
    }

    if( newColor == COLOR4D::UNSPECIFIED || oldColor == newColor )
        return;

    currentColors[ colorButton->m_Layer ] = newColor;

    wxMemoryDC iconDC;

    wxBitmap bitmap = button->GetBitmapLabel();
    iconDC.SelectObject( bitmap );
    iconDC.SetPen( *wxBLACK_PEN );

    wxBrush  brush;
    brush.SetColour( newColor.ToColour() );
    brush.SetStyle( wxBRUSHSTYLE_SOLID );

    iconDC.SetBrush( brush );
    iconDC.DrawRectangle( 0, 0, m_butt_size_pix.x, m_butt_size_pix.y );
    button->SetBitmapLabel( bitmap );
    button->Refresh();

    Refresh( false );
}


bool WIDGET_EESCHEMA_COLOR_CONFIG::TransferDataFromControl()
{
    // Check for color conflicts with background color to give user a chance to bail
    // out before making changes.

    COLOR4D bgcolor = currentColors[ LAYER_SCHEMATIC_BACKGROUND ];

    for( SCH_LAYER_ID clyr = SCH_LAYER_ID_START; clyr < SCH_LAYER_ID_END; ++clyr )
    {
        if( bgcolor == currentColors[ clyr ] && clyr != LAYER_SCHEMATIC_BACKGROUND )
        {
            wxString msg = _( "Some items have the same color as the background\n"
                              "and they will not be seen on the screen.  Are you\n"
                              "sure you want to use these colors?" );

            if( wxMessageBox( msg,  _( "Warning" ), wxYES_NO | wxICON_QUESTION, this ) == wxNO )
                return false;

            break;
        }
    }

    for( SCH_LAYER_ID clyr = SCH_LAYER_ID_START; clyr < SCH_LAYER_ID_END; ++clyr )
        SetLayerColor( currentColors[ clyr ], clyr );

    SetLayerColor( currentColors[ LAYER_WORKSHEET ], (SCH_LAYER_ID) LAYER_WORKSHEET );

    return true;
}


PANEL_EESCHEMA_COLOR_CONFIG::PANEL_EESCHEMA_COLOR_CONFIG( EDA_DRAW_FRAME* aFrame,
                                                          wxWindow* aParent ) :
    wxPanel( aParent )
{
    auto sizer = new wxBoxSizer( wxVERTICAL );
    SetSizer( sizer );

    m_colorConfig = new WIDGET_EESCHEMA_COLOR_CONFIG( this, aFrame );
    sizer->Add( m_colorConfig, 1, wxEXPAND | wxLEFT | wxRIGHT, 5 );
}


bool PANEL_EESCHEMA_COLOR_CONFIG::TransferDataToWindow()
{
    return true;
}


bool PANEL_EESCHEMA_COLOR_CONFIG::TransferDataFromWindow()
{
    return m_colorConfig->TransferDataFromControl();
}
