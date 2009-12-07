/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2009 Isaac Marino Bavaresco, isaacbavaresco@yahoo.com.br
 * Copyright (C) 2009 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2009 Kicad Developers, see change_log.txt for contributors.
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

#include "class_board_design_settings.h"


// We want our dialog to remember its previous screen position
wxPoint DIALOG_LAYERS_SETUP::s_LastPos( -1, -1 );
wxSize  DIALOG_LAYERS_SETUP::s_LastSize;


/*
// Layer order on the list panel

// This function translates from the dialog's layer order to Kicad's layer order.

static int GetLayerNumber( int aRow )
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
*/


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


// The sequence of the layer oriented rows in the scrollPanel:
static const short layerOrder[NB_LAYERS] = {
    ADHESIVE_N_CMP,
    SOLDERPASTE_N_CMP,
    SILKSCREEN_N_CMP,
    SOLDERMASK_N_CMP,
    LAYER_N_FRONT,
    LAYER_N_2,
    LAYER_N_3,
    LAYER_N_4,
    LAYER_N_5,
    LAYER_N_6,
    LAYER_N_7,
    LAYER_N_8,
    LAYER_N_9,
    LAYER_N_10,
    LAYER_N_11,
    LAYER_N_12,
    LAYER_N_13,
    LAYER_N_14,
    LAYER_N_15,
    LAYER_N_BACK,
    SOLDERMASK_N_CU,
    SILKSCREEN_N_CU,
    SOLDERPASTE_N_CU,
    ADHESIVE_N_CU,
    EDGE_N,
    ECO2_N,
    ECO1_N,
    COMMENT_N,
    DRAW_N,
};


struct IDs
{
    IDs( int aName, int aCheckBox, int aChoice )
    {
        name     = aName;
        checkbox = aCheckBox;
        choice   = aChoice;
    }

    short   name;
    short   checkbox;
    short   choice;
};


/**
 * Function getIDs
 * maps \a aLayerNumber to the wx IDs for that layer which are
 * the layer name control ID, checkbox control ID, and choice control ID
 */
static IDs getIDs( int aLayerNumber )
{
#define RET(x)    return IDs( x##NAME, x##CHECKBOX, x##CHOICE );

    switch( aLayerNumber )
    {
    case ADHESIVE_N_CMP:    RET( ID_ADHESFRONT );
    case SOLDERPASTE_N_CMP: RET( ID_SOLDPFRONT );
    case SILKSCREEN_N_CMP:  RET( ID_SILKSFRONT );
    case SOLDERMASK_N_CMP:  RET( ID_MASKFRONT );
    case LAYER_N_FRONT:     RET( ID_FRONT );
    case LAYER_N_2:         RET( ID_INNER2 );
    case LAYER_N_3:         RET( ID_INNER3 );
    case LAYER_N_4:         RET( ID_INNER4 );
    case LAYER_N_5:         RET( ID_INNER5 );
    case LAYER_N_6:         RET( ID_INNER6 );
    case LAYER_N_7:         RET( ID_INNER7 );
    case LAYER_N_8:         RET( ID_INNER8 );
    case LAYER_N_9:         RET( ID_INNER9 );
    case LAYER_N_10:        RET( ID_INNER10 );
    case LAYER_N_11:        RET( ID_INNER11 );
    case LAYER_N_12:        RET( ID_INNER12 );
    case LAYER_N_13:        RET( ID_INNER13 );
    case LAYER_N_14:        RET( ID_INNER14 );
    case LAYER_N_15:        RET( ID_INNER15 );
    case LAYER_N_BACK:      RET( ID_BACK );
    case SOLDERMASK_N_CU:   RET( ID_MASKBACK );
    case SILKSCREEN_N_CU:   RET( ID_SILKSBACK );
    case SOLDERPASTE_N_CU:  RET( ID_SOLDPBACK );
    case ADHESIVE_N_CU:     RET( ID_ADHESBACK );
    case EDGE_N:            RET( ID_PCBEDGES );
    case ECO2_N:            RET( ID_ECO2 );
    case ECO1_N:            RET( ID_ECO1 );
    case COMMENT_N:         RET( ID_COMMENTS );
    case DRAW_N:            RET( ID_DRAWINGS );
    default:
        // wxDEBUGMSG( "bad layer id" );
        return IDs( 0, 0, 0 );
    }

#undef RET
}


// Names for the types of copper layers

/*
static const wxString layerTypeChoiceChoices[] =
{
    // these may not be translated since they come from Specctra.
    wxT("signal"),
    wxT("power"),
    wxT("mixed"),
    wxT("jumper")
};
*/


/***********************************************************************************/
DIALOG_LAYERS_SETUP::DIALOG_LAYERS_SETUP( WinEDA_PcbFrame* parent ) :
    DIALOG_LAYERS_SETUP_BASE2( parent )
/***********************************************************************************/
{
    m_Parent    = parent;
    m_Pcb       = m_Parent->GetBoard();

    init();

    SetAutoLayout( true );
    Layout();

    Center();

    m_sdbSizer2OK->SetFocus();
}


bool DIALOG_LAYERS_SETUP::Show( bool show )
{
    bool ret;

    if( show )
    {
        if( s_LastPos.x != -1 )
        {
            SetSize( s_LastPos.x, s_LastPos.y, s_LastSize.x, s_LastSize.y, 0 );
        }
        ret = DIALOG_LAYERS_SETUP_BASE2::Show( show );
    }
    else
    {
        // Save the dialog's position before hiding
        s_LastPos  = GetPosition();
        s_LastSize = GetSize();

        ret = DIALOG_LAYERS_SETUP_BASE2::Show( show );
    }

    return ret;
}




void DIALOG_LAYERS_SETUP::showBoardLayerNames()
{
    // Establish all the board's layer names into the dialog presentation, by
    // obtaining them from BOARD::GetLayerName() which calls
    // BOARD::GetDefaultLayerName() for non-coppers.

    for( unsigned i=0;  i<DIM(layerOrder);  ++i )
    {
        int layer = layerOrder[i];

        int nameId = getIDs( layer ).name;

        wxControl*  ctl = (wxControl*) FindWindowById( nameId );

        wxASSERT( ctl );

        if( ctl )
        {
            wxString lname = m_Pcb->GetLayerName( layer );

            D(printf("layerName[%d]=%s\n", layer, CONV_TO_UTF8( lname ) );)

            if( ctl->IsKindOf( CLASSINFO(wxTextCtrl) ) )
                ((wxTextCtrl*)ctl)->SetValue( lname );     // wxTextCtrl
            else
                ctl->SetLabel( lname );     // wxStaticText
        }
    }
}


void DIALOG_LAYERS_SETUP::showSelectedLayerCheckBoxes()
{
    int enabledLayers = m_Pcb->GetEnabledLayers();

    for( unsigned i=0;  i<DIM(layerOrder);  ++i )
    {
        int layer = layerOrder[i];

        int checkBoxId = getIDs( layer ).checkbox;

        wxCheckBox*  ctl = (wxCheckBox*) FindWindowById( checkBoxId );

        wxASSERT( ctl );

        if( ctl )
            ctl->SetValue(  (1<<layer) & enabledLayers );
    }
}

void DIALOG_LAYERS_SETUP::showLayerTypes()
{
    for( int copperLayer =  FIRST_COPPER_LAYER;
             copperLayer <= LAST_COPPER_LAYER; ++copperLayer )
    {
        int choiceId = getIDs( copperLayer ).choice;

        wxChoice* ctl = (wxChoice*) FindWindowById( choiceId );

        wxASSERT( ctl );

        if( ctl )
            ctl->SetSelection( m_Pcb->GetLayerType( copperLayer ) );
    }
}


/********************************************************************/
void DIALOG_LAYERS_SETUP::init()
/********************************************************************/
{

    // temporary: set copper layer count without regard to enabled layer mask.
    static const int copperCount[] = {1,2,4,6,8,10,12,14,16};

    m_CopperLayersChoice->SetSelection(1);

    int boardsCopperCount = m_Pcb->GetCopperLayerCount();

    D(printf("boardsCopperCount=%d\n", boardsCopperCount );)

    for( unsigned i = 0;  i<sizeof(copperCount);  ++i )
    {
        if( boardsCopperCount == copperCount[i] )
        {
            m_CopperLayersChoice->SetSelection(i);
            break;
        }
    }

    showBoardLayerNames();

    showSelectedLayerCheckBoxes();

    showLayerTypes();

/* names only:
ID_ADHESFRONTNAME
ID_SOLDPFRONTNAME
ID_SILKSFRONTNAME
ID_MASKFRONTNAME
ID_FRONTNAME
ID_INNER2NAME
ID_INNER3NAME
ID_INNER4NAME
ID_INNER5NAME
ID_INNER6NAME
ID_INNER7NAME
ID_INNER8NAME
ID_INNER9NAME
ID_INNER10NAME
ID_INNER11NAME
ID_INNER12NAME
ID_INNER13NAME
ID_INNER14NAME
ID_INNER15NAME
ID_BACKNAME
ID_MASKBACKNAME
ID_SILKSBACKNAME
ID_SOLDPBACKNAME
ID_ADHESBACKNAME
ID_PCBEDGESNAME
ID_ECO2NAME
ID_ECO1NAME
ID_COMMENTSNAME
ID_DRAWINGSNAME



clean ids:

ID_ADHESFRONTNAME
ID_ADHESFRONTCHECKBOX
ID_SOLDPFRONTNAME
ID_SOLDPFRONTCHECKBOX
ID_SILKSFRONTNAME
ID_SILKSFRONTCHECKBOX
ID_MASKFRONTNAME
ID_MASKFRONTCHECKBOX
ID_FRONTNAME
ID_FRONTCHECKBOX
ID_FRONTCHOICE
ID_INNER2NAME
ID_INNER2CHECKBOX
ID_INNER2CHOICE
ID_INNER3NAME
ID_INNER3CHECKBOX
ID_INNER3CHOICE
ID_INNER4NAME
ID_INNER4CHECKBOX
ID_INNER4CHOICE
ID_INNER5NAME
ID_INNER5CHECKBOX
ID_INNER5CHOICE
ID_INNER6NAME
ID_INNER6CHECKBOX
ID_INNER6CHOICE
ID_INNER7NAME
ID_INNER7CHECKBOX
ID_INNER7CHOICE
ID_INNER8NAME
ID_INNER8CHECKBOX
ID_INNER8CHOICE
ID_INNER9NAME
ID_INNER9CHECKBOX
ID_INNER9CHOICE
ID_INNER10NAME
ID_INNER10CHECKBOX
ID_INNER10CHOICE
ID_INNER11NAME
ID_INNER11CHECKBOX
ID_INNER11CHOICE
ID_INNER12NAME
ID_INNER12CHECKBOX
ID_INNER12CHOICE
ID_INNER13NAME
ID_INNER13CHECKBOX
ID_INNER13CHOICE
ID_INNER14NAME
ID_INNER14CHECKBOX
ID_INNER14CHOICE
ID_INNER15NAME
ID_INNER15CHECKBOX
ID_INNER15CHOICE
ID_BACKNAME
ID_BACKCHECKBOX
ID_BACKCHOICE
ID_MASKBACKNAME
ID_SILKSBACKNAME
ID_SILKSBACKCHECKBOX
ID_SOLDPBACKNAME
ID_SOLDPBACKCHECKBOX
ID_ADHESBACKNAME
ID_ADHESBACKCHECKBOX
ID_PCBEDGESNAME
ID_PCBEDGESCHECKBOX
ID_ECO2NAME
ID_ECHO2CHECKBOX
ID_ECO1NAME
ID_ECO1CHECKBOX
ID_COMMENTSNAME
ID_COMMENTSCHECKBOX
ID_DRAWINGSNAME
ID_DRAWINGSCHECKBOX
*/



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
        int      layer = LAYER_N_FRONT - ii;
        wxString value = layer < (m_ActivesLayersCount - 1) ? wxT( "1" ) : wxT( "0" );
        if( m_ActivesLayersCount > 1 && layer == LAYER_N_FRONT )
            value = wxT( "1" );
        if(  layer == LAYER_N_BACK )
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

    // temporary code to set the copper layer count until the custom layer enabling is in place:

    int copperLayerCount = m_CopperLayersChoice->GetSelection() * 2;
    if( copperLayerCount <= 0 )
        copperLayerCount = 1;

    g_DesignSettings.SetCopperLayerCount( copperLayerCount );
    m_Parent->ReCreateLayerBox( NULL );


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
        if( layer_name != m_Pcb->GetLayerName( LAYER_N_FRONT - ii ) )
        {
            m_Pcb->SetLayerName( LAYER_N_FRONT - ii, layer_name );
        }
    }

    // Initialize the layer type
    for( int ii = 0; ii <  m_gridLayersProperties->GetNumberRows(); ii++ )
    {
        wxString txt   = m_gridLayersProperties->GetCellValue( ii, LAYERS_GRID_TYPE_POSITION );
        int      layer = LAYER_N_FRONT - ii;
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
    DIALOG_LAYERS_SETUP frame( parent );

    frame.ShowModal();
    frame.Destroy();
}

//==============================================================================
