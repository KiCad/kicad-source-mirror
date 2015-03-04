/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2014 KiCad Developers, see CHANGELOG.TXT for contributors.
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

#include <dialog_color_config.h>


#define ID_COLOR_SETUP  1800

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
    { _( "No Connect Symbol" ), LAYER_NOCONNECT },
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
    { _( "Erc warning" ),       LAYER_ERC_WARN },
    { _( "Erc error" ),         LAYER_ERC_ERR },
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


DIALOG_COLOR_CONFIG::DIALOG_COLOR_CONFIG( EDA_DRAW_FRAME* aParent ) :
    DIALOG_SHIM( aParent, wxID_ANY,  _( "EESchema Colors" ),
              wxDefaultPosition, wxDefaultSize,
              wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER )
{
    m_parent = aParent;
    SetExtraStyle( wxWS_EX_BLOCK_EVENTS );
    Init();
    CreateControls();

    GetSizer()->SetSizeHints( this );
}


DIALOG_COLOR_CONFIG::~DIALOG_COLOR_CONFIG()
{
}


void DIALOG_COLOR_CONFIG::Init()
{
    m_outerBoxSizer  = NULL;
    m_mainBoxSizer   = NULL;
    m_columnBoxSizer = NULL;
    m_rowBoxSizer    = NULL;
    m_bitmapButton = NULL;
    m_SelBgColor = NULL;
    m_line = NULL;
    m_stdDialogButtonSizer = NULL;
}


void DIALOG_COLOR_CONFIG::CreateControls()
{
    wxButton*       button;
    wxStaticText*   label;
    int             buttonId = 1800;

    BUTTONINDEX* groups = buttonGroups;

    m_outerBoxSizer = new wxBoxSizer( wxVERTICAL );
    SetSizer( m_outerBoxSizer );

    m_mainBoxSizer = new wxBoxSizer( wxHORIZONTAL );
    m_outerBoxSizer->Add( m_mainBoxSizer, 1, wxGROW | wxLEFT | wxRIGHT, 5 );

    while( groups->m_Buttons != NULL )
    {
        COLORBUTTON* buttons = groups->m_Buttons;

        m_columnBoxSizer = new wxBoxSizer( wxVERTICAL );
        m_mainBoxSizer->Add( m_columnBoxSizer, 1, wxALIGN_TOP | wxLEFT | wxTOP, 5 );
        m_rowBoxSizer = new wxBoxSizer( wxHORIZONTAL );
        m_columnBoxSizer->Add( m_rowBoxSizer, 0, wxGROW | wxLEFT | wxRIGHT | wxBOTTOM, 5 );

        // Add a text string to identify the column of color select buttons.
        label = new wxStaticText( this, wxID_ANY, groups->m_Name );

        // Make the column label font bold.
        wxFont font( label->GetFont() );
        font.SetWeight( wxFONTWEIGHT_BOLD );
        label->SetFont( font );

        m_rowBoxSizer->Add( label, 1, wxALIGN_CENTER_VERTICAL | wxALL, 5 );

        while( buttons->m_Layer >= 0 )
        {
            m_rowBoxSizer = new wxBoxSizer( wxHORIZONTAL );
            m_columnBoxSizer->Add( m_rowBoxSizer, 0, wxGROW | wxALL, 0 );

            wxMemoryDC iconDC;
            wxBitmap   bitmap( BUTT_SIZE_X, BUTT_SIZE_Y );

            iconDC.SelectObject( bitmap );

            EDA_COLOR_T color = GetLayerColor( LAYERSCH_ID( buttons->m_Layer ) );
            currentColors[ buttons->m_Layer ] = color;

            iconDC.SetPen( *wxBLACK_PEN );

            wxBrush brush;
            ColorSetBrush( &brush, color );

#if wxCHECK_VERSION( 3, 0, 0 )
            brush.SetStyle( wxBRUSHSTYLE_SOLID );
#else
            brush.SetStyle( wxSOLID );
#endif

            iconDC.SetBrush( brush );
            iconDC.DrawRectangle( 0, 0, BUTT_SIZE_X, BUTT_SIZE_Y );

            m_bitmapButton = new wxBitmapButton( this, buttonId, bitmap, wxDefaultPosition,
                                               wxSize( BUTT_SIZE_X+8, BUTT_SIZE_Y+6 ) );
            m_bitmapButton->SetClientData( (void*) buttons );

            m_rowBoxSizer->Add( m_bitmapButton, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxBOTTOM, 5 );

            label = new wxStaticText( this, wxID_ANY, wxGetTranslation( buttons->m_Name ) );
            m_rowBoxSizer->Add( label, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxBOTTOM, 5 );
            buttonId += 1;
            buttons++;
        }

        groups++;
    }

    Connect( 1800, buttonId - 1, wxEVT_COMMAND_BUTTON_CLICKED,
             wxCommandEventHandler( DIALOG_COLOR_CONFIG::SetColor ) );

    // Add a spacer to improve appearance.
    m_columnBoxSizer->AddSpacer( 5 );

    wxArrayString m_SelBgColorStrings;
    m_SelBgColorStrings.Add( _( "White" ) );
    m_SelBgColorStrings.Add( _( "Black" ) );
    m_SelBgColor = new wxRadioBox( this, wxID_ANY, _( "Background Color" ),
                                   wxDefaultPosition, wxDefaultSize,
                                   m_SelBgColorStrings, 1, wxRA_SPECIFY_COLS );
    m_SelBgColor->SetSelection( ( m_parent->GetDrawBgColor() == BLACK ) ? 1 : 0 );
    m_columnBoxSizer->Add( m_SelBgColor, 1, wxGROW | wxRIGHT | wxTOP | wxBOTTOM, 5 );

    currentColors[ LAYER_BACKGROUND ] =  m_parent->GetDrawBgColor();


    // Provide a line to separate all of the controls added so far from the
    // "OK", "Cancel", and "Apply" buttons (which will be added after that
    // line).
    m_line = new wxStaticLine( this, -1, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    m_outerBoxSizer->Add( m_line, 0, wxGROW | wxALL, 5 );

    // Provide a StdDialogButtonSizer to accommodate the OK, Cancel, and Apply
    // buttons; using that type of sizer results in those buttons being
    // automatically located in positions appropriate for each (OS) version of
    // KiCad.
    m_stdDialogButtonSizer = new wxStdDialogButtonSizer;
    m_outerBoxSizer->Add( m_stdDialogButtonSizer, 0, wxGROW | wxALL, 10 );

    button = new wxButton( this, wxID_OK, _( "OK" ), wxDefaultPosition, wxDefaultSize, 0 );
    m_stdDialogButtonSizer->AddButton( button );

    button = new wxButton( this, wxID_CANCEL, _( "Cancel" ), wxDefaultPosition, wxDefaultSize, 0 );
    m_stdDialogButtonSizer->AddButton( button );

    button->SetFocus();

    button = new wxButton( this, wxID_APPLY, _( "Apply" ), wxDefaultPosition, wxDefaultSize, 0 );
    m_stdDialogButtonSizer->AddButton( button );

    Connect( wxID_OK, wxEVT_COMMAND_BUTTON_CLICKED,
             wxCommandEventHandler( DIALOG_COLOR_CONFIG::OnOkClick ) );
    Connect( wxID_CANCEL, wxEVT_COMMAND_BUTTON_CLICKED,
             wxCommandEventHandler( DIALOG_COLOR_CONFIG::OnCancelClick ) );
    Connect( wxID_APPLY, wxEVT_COMMAND_BUTTON_CLICKED,
             wxCommandEventHandler( DIALOG_COLOR_CONFIG::OnApplyClick ) );

    m_stdDialogButtonSizer->Realize();

    // Dialog now needs to be resized, but the associated command is found elsewhere.
}


void DIALOG_COLOR_CONFIG::SetColor( wxCommandEvent& event )
{
    wxBitmapButton* button = (wxBitmapButton*) event.GetEventObject();

    wxCHECK_RET( button != NULL, wxT( "Color button event object is NULL." ) );

    COLORBUTTON* colorButton = (COLORBUTTON*) button->GetClientData();

    wxCHECK_RET( colorButton != NULL, wxT( "Client data not set for color button." ) );

    EDA_COLOR_T color = DisplayColorFrame( this, colorButton->m_Layer );

    if( color < 0 || currentColors[ colorButton->m_Layer ] == color )
        return;

    currentColors[ colorButton->m_Layer ] = color;

    wxMemoryDC iconDC;

    wxBitmap bitmap = button->GetBitmapLabel();
    iconDC.SelectObject( bitmap );
    iconDC.SetPen( *wxBLACK_PEN );

    wxBrush  brush;

    ColorSetBrush( &brush, color);

#if wxCHECK_VERSION( 3, 0, 0 )
    brush.SetStyle( wxBRUSHSTYLE_SOLID );
#else
    brush.SetStyle( wxSOLID );
#endif

    iconDC.SetBrush( brush );
    iconDC.DrawRectangle( 0, 0, BUTT_SIZE_X, BUTT_SIZE_Y );
    button->SetBitmapLabel( bitmap );
    button->Refresh();

    Refresh( false );
}


bool DIALOG_COLOR_CONFIG::UpdateColorsSettings()
{
    // Update color of background
    EDA_COLOR_T bgcolor = WHITE;

    if( m_SelBgColor->GetSelection() > 0 )
        bgcolor =  BLACK;

    m_parent->SetDrawBgColor( bgcolor );
    currentColors[ LAYER_BACKGROUND ] = bgcolor;

    bool warning = false;

    for( LAYERSCH_ID clyr = LAYER_WIRE; clyr < LAYERSCH_ID_COUNT; ++clyr )
    {
        SetLayerColor( currentColors[ clyr ], clyr );

        if(  bgcolor == GetLayerColor( clyr ) && clyr != LAYER_BACKGROUND )
            warning = true;
    }

    m_parent->SetGridColor( GetLayerColor( LAYER_GRID ) );

    if( bgcolor == GetLayerColor( LAYER_GRID ) )
        warning = true;

    return warning;
}


void DIALOG_COLOR_CONFIG::OnOkClick( wxCommandEvent& event )
{
    bool warning = UpdateColorsSettings();

    // Prompt the user if an item has the same color as the background
    // because this item cannot be seen:
    if( warning )
        wxMessageBox( _("Warning:\nSome items have the same color as the background\n"
                        "and they will not be seen on screen") );

    m_parent->GetCanvas()->Refresh();

    EndModal( 1 );
}


void DIALOG_COLOR_CONFIG::OnCancelClick( wxCommandEvent& event )
{
    EndModal( -1 );
}


void DIALOG_COLOR_CONFIG::OnApplyClick( wxCommandEvent& event )
{
    UpdateColorsSettings();
    m_parent->GetCanvas()->Refresh();
}
