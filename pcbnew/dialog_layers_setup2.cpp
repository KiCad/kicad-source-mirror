/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 1992-2009 Isaac Marino Bavaresco, isaacbavaresco@yahoo.com.br
 * Copyright (C) 2009 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2009 Kicad Developers, see change_log.txt for contributors.
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


/* functions relatives to the design rules editor
 */
#include "fctsys.h"
#include "common.h"
#include "class_drawpanel.h"

#include "confirm.h"
#include "pcbnew.h"
#include "wxPcbStruct.h"

#include "pcbnew_id.h"
#include "dialog_layers_setup2.h"
#include "wx/generic/gridctrl.h"

// Fields Positions on layer grid
#define LAYERS_GRID_NAME_POSITION       0
#define LAYERS_GRID_ENABLED_POSITION    1
#define LAYERS_GRID_TYPE_POSITION       2



// Define as 1 to show the layers in Pcbnew's original order
#define ORIGINAL_KICAD_LAYER_ORDER  0

// IDs for the dialog controls
enum
{
    ID_LAYERNAMES   = ( wxID_HIGHEST + 1 ),
    ID_CHECKBOXES   = ( ID_LAYERNAMES + NB_LAYERS ),
    ID_LAYERTYPES   = ( ID_CHECKBOXES + NB_LAYERS ),
};


// We want our dialog to remember its previous screen position
wxPoint DIALOG_LAYERS_SETUP::s_LastPos( -1, -1 );
wxSize  DIALOG_LAYERS_SETUP::s_LastSize;


// Layer order on the list panel

#if ORIGINAL_KICAD_LAYER_ORDER

// Kicad's original order

static const int LayerOrder[NB_LAYERS] =
{  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
  16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28 };

#else

// Real board order

static const int LayerOrder[NB_LAYERS] =
{ 17, 19, 21, 23, 15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,
   3,  2,  1,  0, 22, 20, 18, 16, 28, 27, 26, 25, 24 };

#endif

// This function translates from the dialog's layer order to Kicad's layer order.

static int GetlayerNumber( int Row )
{
    return LayerOrder[Row];
}

// Categories for coloring the rows backgrounds (0 means default dialog color).
static const int LayerCategories[NB_LAYERS] =
{  3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1, 1, 1, 1, 2, 2, 2, 2, 5, 5, 5, 5, 4 };

 // Descriptive types for non-copper layers
static const wxString LayerCategoriesNames[NB_LAYERS] =
{
    _( "Unknown" ),                     // Not used
    _( "Off-board, manufacturing" ),    // 1
    _( "On-board, non-copper" ),        // 2
    _( "On-board, copper" ),            // 3
    _( "Board contour" ),               // 4
    _( "Auxiliary" )                    // 5
};

// Names for the presets
static const wxString PresetsChoiceChoices[] =
{
    _("All Layers On"),
    _("Single Side"),
    _("Single Side, SMD on Back"),
    _("Two Layers, Parts on Front"),
    _("Two Layers, Parts on Both Faces"),
    _("Four Layers, Parts on Front"),
    _("Four Layers, Parts on Both Faces")
};

#define PRESETS_CHOICE_N_CHOICES DIM(m_PresetsChoiceChoices)

// Bit masks (for all layers) for each defined preset
static const int Presets[] =
{
    ALL_LAYERS,                                                                     // 0x1fffffff
    SILKSCREEN_LAYER_CMP | CUIVRE_LAYER | SOLDERMASK_LAYER_CU | EDGE_LAYER,         // 0x10600001
    SILKSCREEN_LAYER_CMP | CUIVRE_LAYER | SOLDERMASK_LAYER_CU |
        ADHESIVE_LAYER_CU | EDGE_LAYER,                                             // 0x10610001
    SILKSCREEN_LAYER_CMP | SOLDERMASK_LAYER_CMP | CMP_LAYER | CUIVRE_LAYER |
        SOLDERMASK_LAYER_CU | EDGE_LAYER,                                           // 0x10e08001
    SILKSCREEN_LAYER_CMP | SOLDERMASK_LAYER_CMP | CMP_LAYER | CUIVRE_LAYER |
        SOLDERMASK_LAYER_CU | SILKSCREEN_LAYER_CU | EDGE_LAYER,                     // 0x10f08001
    SILKSCREEN_LAYER_CMP | SOLDERMASK_LAYER_CMP | CMP_LAYER | LAYER_3 |
        LAYER_2 | CUIVRE_LAYER | SOLDERMASK_LAYER_CU | EDGE_LAYER,                  // 0x10e08007
    SILKSCREEN_LAYER_CMP | SOLDERMASK_LAYER_CMP | CMP_LAYER | LAYER_3 | LAYER_2 |
        CUIVRE_LAYER | SOLDERMASK_LAYER_CU | SILKSCREEN_LAYER_CU | EDGE_LAYER       // 0x10f08007
};


// Bit masks for copper layers, one for each option in the copper layer choice widget
static const int CopperMasks[] =
{
    0x00000001,
    0x00008001,
    0x00008007,
    0x0000801f,
    0x0000807f,
    0x000081ff,
    0x000087ff,
    0x00009fff,
    0x0000ffff
};


// Names for the types of copper layers

static const wxString LayerTypeChoiceChoices[] =
{
    wxT("Signal"),
    wxT("Power"),
    wxT("Mixed"),
    wxT("Jumper")
};
#define LAYER_TYPE_CHOICE_N_CHOICES  DIM(LayerTypeChoiceChoices)


/***********************************************************************************/
DIALOG_LAYERS_SETUP::DIALOG_LAYERS_SETUP( WinEDA_PcbFrame* parent ) :
    DIALOG_LAYERS_SETUP_BASE2( parent )
/***********************************************************************************/
{
    m_Parent    = parent;
    m_Pcb       = m_Parent->GetBoard();

    Init();

    SetAutoLayout( true );
}


#define WX_COMMON_FLAGS   (wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT)


/********************************************************************/
void DIALOG_LAYERS_SETUP::Init()
/********************************************************************/
{
    // delete the junk controls, which were put in only for WYSIWYG design formatting purposes.
    delete m_junkStaticText;
    delete m_junkCheckBox;
    delete m_junkChoice;

    // We need a sizer to organize the controls inside the wxScrolledWindow.
    // We must adjust manually the widths of the column captions and controls.

    int col1Width;
    {
        // We create a wxStaticText just to query its size.
        // The layer names are restricted to 20 characters, so 20 "W" is the widest possible text,
        // plus 1 "W" to compensate for the wxTextCtrl border.
        wxStaticText *text = new wxStaticText( this, wxID_ANY, wxT( "WWWWWWWWWWWWWWWWWWWWW" ));
        col1Width = text->GetSize().x;
        // It is important to delete the temporary text, or else it will appear in the top
        // left corner of the dialog.
        delete text;
    }

    // The second column will have the width of its caption, because the check boxes are
    // narrower than the caption.
    int col2Width = m_LayerEnabledCaption->GetSize().x;

    // The third column width will be the widest of all of its controls.
    // Inside the loop we will update col3Width if some control is wider.
    int col3Width = m_LayerTypeCaption->GetSize().x;

    // Iterate for every layer.
    for( int i = 0; i < NB_LAYERS; i++ )
    {
        // Obtain the layer number which lies in this row.
        int layerNumber = GetlayerNumber( i );

        // Here we create the control for the first column (layer name).

        if( layerNumber < NB_COPPER_LAYERS )
        {
            // For the copper layers we need a wxTextCtrl.

            m_textCtrl1[layerNumber] = new wxTextCtrl( m_LayersListPanel, ID_LAYERNAMES+i, m_Pcb->GetLayerName( layerNumber ), wxDefaultPosition, wxDefaultSize, 0 );
            m_textCtrl1[layerNumber]->SetMaxLength( 20 );
            m_textCtrl1[layerNumber]->SetMinSize( wxSize( col1Width, -1 ));
            m_LayerListFlexGridSizer->Add( m_textCtrl1[layerNumber], 1, WX_COMMON_FLAGS | wxEXPAND, 5 );
        }
        else
        {
            // For the non-copper layers we need a wxStaticText.
            wxStaticText*   layerName;

            layerName = new wxStaticText( m_LayersListPanel, wxID_ANY, m_Pcb->GetLayerName( layerNumber ), wxDefaultPosition, wxDefaultSize, 0 );
            layerName->Wrap( -1 );
            layerName->SetMinSize( wxSize( col1Width, -1 ));
            m_LayerListFlexGridSizer->Add( layerName, 1, WX_COMMON_FLAGS, 5 );

        }

        // Here we create the control for the second column (layer enabled), a wxCheckBox.

        m_checkBox[layerNumber] = new wxCheckBox( m_LayersListPanel, ID_CHECKBOXES+i, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
        m_checkBox[layerNumber]->SetMinSize( wxSize( col2Width, -1 ));
        m_LayerListFlexGridSizer->Add( m_checkBox[layerNumber], 1, WX_COMMON_FLAGS | wxALIGN_CENTER_HORIZONTAL, 5 );

        // Here we create the control for the third column (layer type).

        if( layerNumber < NB_COPPER_LAYERS )
        {
            // For the copper layers we need a wxChoice.

            m_choice5[layerNumber] = new wxChoice( m_LayersListPanel, ID_LAYERTYPES+i, wxDefaultPosition, wxDefaultSize, LAYER_TYPE_CHOICE_N_CHOICES, LayerTypeChoiceChoices, 0 );
            m_choice5[layerNumber]->SetSelection( 0 );
            m_LayerListFlexGridSizer->Add( m_choice5[layerNumber], 1, WX_COMMON_FLAGS, 5 );
            col3Width = max( col3Width, m_choice5[layerNumber]->GetBestSize().x );
        }
        else
        {
            // For the non-copper layers we need a wxStaticText.
            wxStaticText*   staticText2;

            staticText2 = new wxStaticText( m_LayersListPanel, wxID_ANY, LayerCategoriesNames[LayerCategories[layerNumber]], wxDefaultPosition, wxDefaultSize, 0 );
            staticText2->Wrap( -1 );
            m_LayerListFlexGridSizer->Add( staticText2, 1, WX_COMMON_FLAGS, 5 );
            col3Width = max( col3Width, staticText2->GetBestSize().x );
        }
    }

    // @todo overload a layout function so we can reposition the column titles,
    // which should probably not go in a sizer of their own so that we do not have
    // to fight to position them, Dick.  Will work this out next.


    // Adjust the vertical scroll rate so our list scrolls always one full line each time.
//    m_LayersListPanel->SetScrollRate( 0, m_textCtrl1[0]->GetSize().y );
}


/* Initialize the Routable column, and the R/W property of some cells
 */
void DIALOG_LAYERS_SETUP::SetRoutableLayerStatus()
{
/*
    m_gridLayersProperties->SetColFormatBool( LAYERS_GRID_ENABLED_POSITION );
    for( int ii = 0; ii <  m_gridLayersProperties->GetNumberRows(); ii++ )
    {
        int      layer = LAYER_CMP_N - ii;
        wxString value = layer < (m_ActivesLayersCount - 1) ? wxT( "1" ) : wxT( "0" );
        if( m_ActivesLayersCount > 1 && layer == LAYER_CMP_N )
            value = wxT( "1" );
        if(  layer == COPPER_LAYER_N )
            value = wxT( "1" );
        m_gridLayersProperties->SetCellValue( ii, LAYERS_GRID_ENABLED_POSITION, value );
        m_gridLayersProperties->SetReadOnly( ii, LAYERS_GRID_ENABLED_POSITION );

        // Set to Read Only cell for non existing copper layers:
        m_gridLayersProperties->SetReadOnly( ii, LAYERS_GRID_TYPE_POSITION, value != wxT( "1" ) );
        m_gridLayersProperties->SetReadOnly( ii, LAYERS_GRID_NAME_POSITION, value != wxT( "1" ) );
    }
*/
}



/*****************************************************************/
void DIALOG_LAYERS_SETUP::OnCancelButtonClick( wxCommandEvent& event )
/*****************************************************************/
{
    EndModal( 0 );
}


/**************************************************************************/
void DIALOG_LAYERS_SETUP::OnOkButtonClick( wxCommandEvent& event )
/**************************************************************************/
{
/*
    if( !TestDataValidity() )
    {
        DisplayError( this, _( "Errors detected, Abort" ) );
        return;
    }

    g_DesignSettings.m_CopperLayerCount = m_ActivesLayersCount;

    // Initialize the new layer name
    for( int ii = 0; ii <  m_gridLayersProperties->GetNumberRows(); ii++ )
    {
        wxString layer_name = m_gridLayersProperties->GetCellValue( ii, LAYERS_GRID_NAME_POSITION );
        if( layer_name != m_Pcb->GetLayerName( LAYER_CMP_N - ii ) )
        {
            m_Pcb->SetLayerName( LAYER_CMP_N - ii, layer_name );
        }
    }

    // Initialize the layer type
    for( int ii = 0; ii <  m_gridLayersProperties->GetNumberRows(); ii++ )
    {
        wxString txt   = m_gridLayersProperties->GetCellValue( ii, LAYERS_GRID_TYPE_POSITION );
        int      layer = LAYER_CMP_N - ii;
        for( int jj = 0; jj < 3; jj++ )
        {
            if( m_LayersTypeName[jj] == txt )
            {
                m_Pcb->SetLayerType( layer, m_LayersType[jj] );
                break;
            }
        }
    }
    m_Parent->ReCreateLayerBox( NULL );
*/
    EndModal( wxID_OK );
}


/**************************************************************************/
void DIALOG_LAYERS_SETUP::OnLayerCountClick( wxCommandEvent& event )
/**************************************************************************/
{
/*
    m_ActivesLayersCount = m_LayersCountSelection->GetSelection() * 2;
    if( m_ActivesLayersCount <= 0 )
        m_ActivesLayersCount = 1;

    // Reinit the routable layers status
    SetRoutableLayerStatus();
*/
}


/* TestDataValidity
 * Performs a control of data validity
 * set the background of a bad cell in RED and display an info message
 * @return true if Ok, false if error
 */
bool DIALOG_LAYERS_SETUP::TestDataValidity()
{
    bool success = true;
/*
    m_MessagesList->SetPage(wxEmptyString); // Clear message list

    //  Test duplicate layers names
    for( int ii = 0; ii < m_gridLayersProperties->GetNumberRows() - 1; ii++ )
    {
        wxString value = m_gridLayersProperties->GetCellValue( ii, LAYERS_GRID_NAME_POSITION );
        for( int jj = ii+1; jj < m_gridLayersProperties->GetNumberRows(); jj++ )
        {
            wxString othervalue = m_gridLayersProperties->GetCellValue( ii,
                                                                        LAYERS_GRID_NAME_POSITION );
            othervalue = m_gridLayersProperties->GetCellValue( jj, LAYERS_GRID_NAME_POSITION );
            if( value.CmpNoCase( othervalue ) == 0 )   // Already exists!
            {
                wxString text;
                text.Printf( _(
                                "<small>This layer name <b>%s</b> is already existing<br>" ),
                            GetChars( value ) );
                m_MessagesList->AppendToPage( text );
                success = false;
            }
        }
    }
*/
    return success;
}
//==============================================================================
// Invoke the dialog.

void DisplayDialogLayerSetup( WinEDA_PcbFrame* parent )
{
    DIALOG_LAYERS_SETUP* frame = new DIALOG_LAYERS_SETUP( parent );
    frame->ShowModal();
    frame->Destroy();
}

//==============================================================================
