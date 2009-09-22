/////////////////////////////////////////////////////////////////////////////

// Name:        dialog_design_rules.cpp
// Author:      jean-pierre Charras
/////////////////////////////////////////////////////////////////////////////

/* functions relatives to the design rules editor
 */
#include "fctsys.h"
#include "common.h"
#include "class_drawpanel.h"

#include "confirm.h"
#include "pcbnew.h"
#include "wxPcbStruct.h"

#include "pcbnew_id.h"
#include "dialog_copper_layers_setup.h"
#include "wx/generic/gridctrl.h"


// Fields Positions on layer grid
#define LAYERS_GRID_ROUTABLE_POSITION 0
#define LAYERS_GRID_STATUS_POSITION   1
#define LAYERS_GRID_NAME_POSITION     2


/***********************************************************************************/
DIALOG_COPPER_LAYERS_SETUP::DIALOG_COPPER_LAYERS_SETUP( WinEDA_PcbFrame* parent ) :
    DIALOG_COPPER_LAYERS_SETUP_BASE( parent )
/***********************************************************************************/
{
    m_Parent = parent;

    Init();
    SetAutoLayout( true );
    GetSizer()->Fit( this );
    GetSizer()->SetSizeHints( this );
}


/********************************************************************/
void DIALOG_COPPER_LAYERS_SETUP::Init()
/********************************************************************/
{
    SetFocus();
    SetReturnCode( 0 );

    // Initialize the layers grid:
    m_ActivesLayersCount = g_DesignSettings.m_CopperLayerCount;
    m_Pcb = m_Parent->GetBoard();

    m_LayersCountSelection->SetSelection( m_ActivesLayersCount / 2 );

    // Initialize the Routable column
    SetRoutableLayerStatus();

    // Initialize the Status column (layers attribute)
    LAYER_T typelist[4] = { LT_SIGNAL, LT_POWER, LT_MIXED, LT_JUMPER };
    for( int ii = 0; ii < 4; ii++ )
    {
        m_LayersType[ii]     = typelist[ii];
        m_LayersTypeName[ii] = CONV_FROM_UTF8( LAYER::ShowType( typelist[ii] ) );
    }

    for( int ii = 0; ii <  m_gridLayersProperties->GetNumberRows(); ii++ )
    {
        m_gridLayersProperties->SetCellEditor( ii, LAYERS_GRID_STATUS_POSITION,
                                              new wxGridCellChoiceEditor( WXSIZEOF(
                                                                              m_LayersTypeName ),
                                                                          m_LayersTypeName ) );
        int select = LT_SIGNAL;
        for( int jj = 0; jj < 4; jj++ )
        {
            int layer = LAYER_CMP_N - ii;
            if( m_Pcb->GetLayerType( layer ) == m_LayersType[jj] )
            {
                select = m_LayersType[jj];
                break;
            }
        }

        m_gridLayersProperties->SetCellValue( ii, LAYERS_GRID_STATUS_POSITION,
                                              m_LayersTypeName[select] );
        m_gridLayersProperties->SetCellOverflow( ii, LAYERS_GRID_STATUS_POSITION, false );
    }

    // Initialize the Name column
    for( int ii = 0; ii <  m_gridLayersProperties->GetNumberRows(); ii++ )
    {
        wxString layer_name = m_Pcb->GetLayerName( LAYER_CMP_N - ii );
        m_gridLayersProperties->SetCellValue( ii, LAYERS_GRID_NAME_POSITION, layer_name );
    }
}


/* Initialize the Routable column, and the R/W property of some cells
 */
void DIALOG_COPPER_LAYERS_SETUP::SetRoutableLayerStatus()
{
    m_gridLayersProperties->SetColFormatBool( LAYERS_GRID_ROUTABLE_POSITION );
    for( int ii = 0; ii <  m_gridLayersProperties->GetNumberRows(); ii++ )
    {
        int      layer = LAYER_CMP_N - ii;
        wxString value = layer < (m_ActivesLayersCount - 1) ? wxT( "1" ) : wxT( "0" );
        if( m_ActivesLayersCount > 1 && layer == LAYER_CMP_N )
            value = wxT( "1" );
        if(  layer == COPPER_LAYER_N )
            value = wxT( "1" );
        m_gridLayersProperties->SetCellValue( ii, LAYERS_GRID_ROUTABLE_POSITION, value );
        m_gridLayersProperties->SetReadOnly( ii, LAYERS_GRID_ROUTABLE_POSITION );

        // Set to Read Only cell for non existing copper layers:
        m_gridLayersProperties->SetReadOnly( ii, LAYERS_GRID_STATUS_POSITION, value != wxT( "1" ) );
        m_gridLayersProperties->SetReadOnly( ii, LAYERS_GRID_NAME_POSITION, value != wxT( "1" ) );
    }
}



/*****************************************************************/
void DIALOG_COPPER_LAYERS_SETUP::OnCancelButtonClick( wxCommandEvent& event )
/*****************************************************************/
{
    EndModal( 0 );
}


/**************************************************************************/
void DIALOG_COPPER_LAYERS_SETUP::OnOkButtonClick( wxCommandEvent& event )
/**************************************************************************/
{
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
        wxString txt   = m_gridLayersProperties->GetCellValue( ii, LAYERS_GRID_STATUS_POSITION );
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

    EndModal( wxID_OK );
}


/**************************************************************************/
void DIALOG_COPPER_LAYERS_SETUP::OnLayerCountClick( wxCommandEvent& event )
/**************************************************************************/
{
    m_ActivesLayersCount = m_LayersCountSelection->GetSelection() * 2;
    if( m_ActivesLayersCount <= 0 )
        m_ActivesLayersCount = 1;

    // Reinit the routable layers status
    SetRoutableLayerStatus();
}


/* TestDataValidity
 * Performs a control of data validity
 * set the background of a bad cell in RED and display an info message
 * @return true if Ok, false if error
 */
bool DIALOG_COPPER_LAYERS_SETUP::TestDataValidity()
{
    bool success = true;
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
                            value.GetData() );
                m_MessagesList->AppendToPage( text );
                success = false;
            }
        }
    }

    return success;
}
