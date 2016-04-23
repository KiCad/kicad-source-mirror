/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2016 KiCad Developers, see CHANGELOG.TXT for contributors.
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

/* Set up color Layers for Eeschema
 */

#include <fctsys.h>
#include <draw_frame.h>
#include <class_drawpanel.h>

#include <general.h>

#include "widget_eeschema_color_config.h"

// See selcolor.cpp:
extern EDA_COLOR_T DisplayColorFrame( wxWindow* aParent, EDA_COLOR_T aOldColor );

// Specify the width and height of every (color-displaying / bitmap) button
const int BUTT_SIZE_X = 16;
const int BUTT_SIZE_Y = 16;


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
    { _( "Body" ),              LAYER_DEVICE },
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
    { _( "ERC warning" ),       LAYER_ERC_WARN },
    { _( "ERC error" ),         LAYER_ERC_ERR },
    { _( "Grid" ),              LAYER_GRID },
    { wxT( "" ), -1 }                           // Sentinel marking end of list.
};


static BUTTONINDEX buttonGroups[] = {
    { _( "General" ),           generalColorButtons },
    { _( "Component" ),         componentColorButtons },
    { _( "Sheet" ),             sheetColorButtons },
    { _( "Miscellaneous" ),     miscColorButtons },
    { wxT( "" ), NULL }
};


static EDA_COLOR_T currentColors[ LAYERSCH_ID_COUNT ];


WIDGET_EESCHEMA_COLOR_CONFIG::WIDGET_EESCHEMA_COLOR_CONFIG( wxWindow* aParent, EDA_DRAW_FRAME* aDrawFrame ) :
    wxPanel( aParent ), m_drawFrame( aDrawFrame )
{
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
        m_mainBoxSizer->Add( columnBoxSizer, 1, wxALIGN_TOP | wxLEFT | wxTOP, 5 );
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

            wxMemoryDC iconDC;
            wxBitmap   bitmap( BUTT_SIZE_X, BUTT_SIZE_Y );

            iconDC.SelectObject( bitmap );

            EDA_COLOR_T color = GetLayerColor( LAYERSCH_ID( buttons->m_Layer ) );
            currentColors[ buttons->m_Layer ] = color;

            iconDC.SetPen( *wxBLACK_PEN );

            wxBrush brush;
            ColorSetBrush( &brush, color );
            brush.SetStyle( wxBRUSHSTYLE_SOLID );
            iconDC.SetBrush( brush );
            iconDC.DrawRectangle( 0, 0, BUTT_SIZE_X, BUTT_SIZE_Y );

            wxBitmapButton* bitmapButton = new wxBitmapButton(
                                    this, buttonId, bitmap, wxDefaultPosition,
                                    wxSize( BUTT_SIZE_X+8, BUTT_SIZE_Y+6 ) );
            bitmapButton->SetClientData( (void*) buttons );

            rowBoxSizer->Add( bitmapButton, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxBOTTOM, 5 );

            label = new wxStaticText( this, wxID_ANY, wxGetTranslation( buttons->m_Name ) );
            rowBoxSizer->Add( label, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxBOTTOM, 5 );
            buttonId += 1;
            buttons++;
        }

        groups++;
    }

    Connect( 1800, buttonId - 1, wxEVT_COMMAND_BUTTON_CLICKED,
             wxCommandEventHandler( WIDGET_EESCHEMA_COLOR_CONFIG::SetColor ) );

    wxArrayString selBgColorStrings;
    selBgColorStrings.Add( _( "White" ) );
    selBgColorStrings.Add( _( "Black" ) );
    m_SelBgColor = new wxRadioBox( this, wxID_ANY, _( "Background Color" ),
                                   wxDefaultPosition, wxDefaultSize,
                                   selBgColorStrings, 1, wxRA_SPECIFY_COLS );
    m_SelBgColor->SetSelection( ( GetDrawFrame()->GetDrawBgColor() == BLACK ) ? 1 : 0 );

    if( columnBoxSizer )
    {
        // Add a spacer to improve appearance.
        columnBoxSizer->AddSpacer( 5 );
        columnBoxSizer->Add( m_SelBgColor, 1, wxGROW | wxRIGHT | wxTOP | wxBOTTOM, 5 );
    }

    currentColors[ LAYER_BACKGROUND ] =  GetDrawFrame()->GetDrawBgColor();

    // Dialog now needs to be resized, but the associated command is found elsewhere.
}


void WIDGET_EESCHEMA_COLOR_CONFIG::SetColor( wxCommandEvent& event )
{
    wxBitmapButton* button = (wxBitmapButton*) event.GetEventObject();

    wxCHECK_RET( button != NULL, wxT( "Color button event object is NULL." ) );

    COLORBUTTON* colorButton = (COLORBUTTON*) button->GetClientData();

    wxCHECK_RET( colorButton != NULL, wxT( "Client data not set for color button." ) );

    EDA_COLOR_T color = DisplayColorFrame( this, currentColors[colorButton->m_Layer] );

    if( color < 0 || currentColors[ colorButton->m_Layer ] == color )
        return;

    currentColors[ colorButton->m_Layer ] = color;

    wxMemoryDC iconDC;

    wxBitmap bitmap = button->GetBitmapLabel();
    iconDC.SelectObject( bitmap );
    iconDC.SetPen( *wxBLACK_PEN );

    wxBrush  brush;

    ColorSetBrush( &brush, color);

    brush.SetStyle( wxBRUSHSTYLE_SOLID );

    iconDC.SetBrush( brush );
    iconDC.DrawRectangle( 0, 0, BUTT_SIZE_X, BUTT_SIZE_Y );
    button->SetBitmapLabel( bitmap );
    button->Refresh();

    Refresh( false );
}


bool WIDGET_EESCHEMA_COLOR_CONFIG::TransferDataFromControl()
{
    bool warning = false;

    // Check for color conflicts with background color to give user a chance to bail
    // out before making changes.

    EDA_COLOR_T bgcolor = WHITE;

    if( m_SelBgColor->GetSelection() > 0 )
        bgcolor =  BLACK;

    for( LAYERSCH_ID clyr = LAYER_WIRE; clyr < LAYERSCH_ID_COUNT; ++clyr )
    {
        if( bgcolor == currentColors[ clyr ] && clyr != LAYER_BACKGROUND )
        {
            warning = true;
            break;
        }
    }

    // Prompt the user if an item has the same color as the background
    // because this item cannot be seen:
    if( warning )
    {
        if( wxMessageBox( _( "Some items have the same color as the background\n"
                             "and they will not be seen on the screen.  Are you\n"
                             "sure you want to use these colors?" ),
                          _( "Warning" ),
                          wxYES_NO | wxICON_QUESTION, this ) == wxNO )
            return false;
    }

    // Update color of background
    GetDrawFrame()->SetDrawBgColor( bgcolor );
    currentColors[ LAYER_BACKGROUND ] = bgcolor;


    for( LAYERSCH_ID clyr = LAYER_WIRE; clyr < LAYERSCH_ID_COUNT; ++clyr )
    {
        SetLayerColor( currentColors[ clyr ], clyr );
    }

    GetDrawFrame()->SetGridColor( GetLayerColor( LAYER_GRID ) );
    GetDrawFrame()->GetCanvas()->Refresh();

    return true;
}


void WIDGET_EESCHEMA_COLOR_CONFIG::InstallOnPanel( wxPanel* aPanel )
{
    wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );

    sizer->Add( this, 1, wxALL | wxEXPAND, 0 );
    aPanel->SetSizer( sizer );
}
