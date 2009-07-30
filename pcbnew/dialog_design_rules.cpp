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

#include "id.h"
#include "dialog_design_rules.h"
#include "wx/generic/gridctrl.h"

// Fields Positions on layer grid
#define LAYERS_GRID_ROUTABLE_POSITION 0
#define LAYERS_GRID_STATUS_POSITION   1
#define LAYERS_GRID_NAME_POSITION     2

// Fields Positions on rules grid
#define RULE_GRID_TRACKSIZE_POSITION    0
#define RULE_GRID_VIASIZE_POSITION      1
#define RULE_GRID_CLEARANCE_POSITION    2
#define RULE_GRID_MINTRACKSIZE_POSITION 3
#define RULE_GRID_MINVIASIZE_POSITION   4

/***********************************************************************************/
DIALOG_DESIGN_RULES::DIALOG_DESIGN_RULES( WinEDA_PcbFrame* parent ) :
    DIALOG_DESIGN_RULES_BASE( parent )
/***********************************************************************************/
{
    m_Parent = parent;

    Init();
    SetAutoLayout( true );
    GetSizer()->Fit( this );
    GetSizer()->SetSizeHints( this );
}


/********************************************************************/
void DIALOG_DESIGN_RULES::Init()
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

    // Initialize the Rules List
    InitRulesList();

    /* Initialize the list of nets buffers
     *  (note the netcode 0 is not a real net, so it is not loaded)
     */
    for( unsigned ii = 1; ; ii++ )
    {
        NETINFO_ITEM* net = m_Pcb->FindNet( ii );
        if( net == NULL )
            break;
        m_StockNets.push_back( net );

        // search the index in rules list for this net
        int rules_idx = 0;
        for( int jj = 0; jj < m_gridNetClassesProperties->GetNumberRows(); jj++ )
        {
            if( m_gridNetClassesProperties->GetRowLabelValue( jj ).CmpNoCase( net->GetClassName() )
                == 0 )
            {
                rules_idx = jj;
                break;
            }
        }

        m_NetsLinkToClasses.push_back( rules_idx ); // All nets are set to default net class
    }

    InitializeRulesSelectionBoxes();
}


/** Function FillListBoxWithNetsNames
 * populates the aListBox with net names members of the aNetclassIndex net class
 * the "Client Data pointer" is used to store the index of nets in ne nets lists
 */
void DIALOG_DESIGN_RULES::FillListBoxWithNetsNames( wxListBox* aListBox, int aNetclassIndex )
{
    aListBox->Clear();
    unsigned idx = 0;
    for( unsigned ii = 0; ii < m_StockNets.size(); ii++ )
    {
        if( aNetclassIndex == m_NetsLinkToClasses[ii] )
        {
            aListBox->Append( m_StockNets[ii]->GetNetname() );

            // Store the index of this net
            // This is a trick to get an unsigned integer index from a pointer value.
            // Some compilers cannot accept to convert an unsigned to a pointer  without complains
            char * ptr = (char*)0 + ii;
            aListBox->SetClientData( idx, ptr );
            idx++;
        }
    }
}


/* Initialize the combno boxes by the list of existing net classes
 */
void DIALOG_DESIGN_RULES::InitializeRulesSelectionBoxes()
{
    m_CBoxRightSelection->Clear();
    m_CBoxLeftSelection->Clear();
    for( int ii = 0; ii < m_gridNetClassesProperties->GetNumberRows(); ii++ )
    {
        m_CBoxRightSelection->Append( m_gridNetClassesProperties->GetRowLabelValue( ii ) );
        m_CBoxLeftSelection->Append( m_gridNetClassesProperties->GetRowLabelValue( ii ) );
    }

    m_CBoxRightSelection->Select( 0 );
    m_CBoxLeftSelection->Select( 0 );
    m_buttonRightToLeft->Enable( false );
    m_buttonLeftToRight->Enable( false );;
    FillListBoxWithNetsNames( m_listBoxLeftNetSelect, m_CBoxLeftSelection->GetCurrentSelection() );
    FillListBoxWithNetsNames( m_listBoxRightNetSelect, m_CBoxRightSelection->GetCurrentSelection() );
}


/* Initialize the Routable column, and the R/W property of some cells
 */
void DIALOG_DESIGN_RULES::SetRoutableLayerStatus()
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


/* Initialize the rules list from board
 */
void DIALOG_DESIGN_RULES::InitRulesList()
{
    for( int ii = 0;  ; ii++ )
    {
        const NETCLASS* netclass = m_Pcb->m_NetClassesList.GetNetClass( ii );
        if( netclass == NULL )
            break;

        // Creates one entry if needed
        if( ii >= m_gridNetClassesProperties->GetNumberRows() )
            m_gridNetClassesProperties->AppendRows();

        // Init name
        m_gridNetClassesProperties->SetRowLabelValue( ii, netclass->m_Name );

        // Init data
        wxString msg;
        msg = ReturnStringFromValue( g_UnitMetric,
                                     netclass->m_NetParams.m_TracksWidth,
                                     m_Parent->m_InternalUnits, false );
        m_gridNetClassesProperties->SetCellValue( ii, RULE_GRID_TRACKSIZE_POSITION, msg );
        msg = ReturnStringFromValue( g_UnitMetric,
                                     netclass->m_NetParams.m_ViasSize,
                                     m_Parent->m_InternalUnits, false );
        m_gridNetClassesProperties->SetCellValue( ii, RULE_GRID_VIASIZE_POSITION, msg );
        msg = ReturnStringFromValue( g_UnitMetric,
                                     netclass->m_NetParams.m_Clearance,
                                     m_Parent->m_InternalUnits, false );
        m_gridNetClassesProperties->SetCellValue( ii, RULE_GRID_CLEARANCE_POSITION, msg );
        msg = ReturnStringFromValue( g_UnitMetric,
                                     netclass->m_NetParams.m_TracksMinWidth,
                                     m_Parent->m_InternalUnits, false );
        m_gridNetClassesProperties->SetCellValue( ii, RULE_GRID_MINTRACKSIZE_POSITION, msg );
        msg = ReturnStringFromValue( g_UnitMetric,
                                     netclass->m_NetParams.m_ViasMinSize,
                                     m_Parent->m_InternalUnits, false );
        m_gridNetClassesProperties->SetCellValue( ii, RULE_GRID_MINVIASIZE_POSITION, msg );
    }
}


/* Copy the rules list to board
 */
void DIALOG_DESIGN_RULES::CopyRulesListToBoard()
{
    m_Pcb->m_NetClassesList.ClearList();
    for( int ii = 0; ii < m_gridNetClassesProperties->GetNumberRows(); ii++ )
    {
        NETCLASS* netclass = new NETCLASS( m_Pcb,
                                          m_gridNetClassesProperties->GetRowLabelValue( ii ) );
        m_Pcb->m_NetClassesList.AddNetclass( netclass );

        // Init data
        netclass->m_NetParams.m_TracksWidth =
            ReturnValueFromString( g_UnitMetric,
                                   m_gridNetClassesProperties->GetCellValue( ii,
                                                                             RULE_GRID_TRACKSIZE_POSITION ),
                                   m_Parent->m_InternalUnits );

        netclass->m_NetParams.m_ViasSize =
            ReturnValueFromString( g_UnitMetric,
                                   m_gridNetClassesProperties->GetCellValue( ii,
                                                                             RULE_GRID_VIASIZE_POSITION ),
                                   m_Parent->m_InternalUnits );

        netclass->m_NetParams.m_Clearance =
            ReturnValueFromString( g_UnitMetric,
                                   m_gridNetClassesProperties->GetCellValue( ii,
                                                                             RULE_GRID_CLEARANCE_POSITION ),
                                   m_Parent->m_InternalUnits );

        netclass->m_NetParams.m_TracksMinWidth =
            ReturnValueFromString( g_UnitMetric,
                                   m_gridNetClassesProperties->GetCellValue( ii,
                                                                             RULE_GRID_MINTRACKSIZE_POSITION ),
                                   m_Parent->m_InternalUnits );

        netclass->m_NetParams.m_ViasMinSize =
            ReturnValueFromString( g_UnitMetric,
                                   m_gridNetClassesProperties->GetCellValue( ii,
                                                                             RULE_GRID_MINVIASIZE_POSITION ),
                                   m_Parent->m_InternalUnits );

        // Copy the list of nets associated to this netclass:
        for( unsigned idx = 0; idx < m_StockNets.size(); idx++ )
        {
            if( m_NetsLinkToClasses[idx] == ii )
                netclass->AddMember( m_StockNets[idx]->GetNetname() );
        }
    }

    m_Pcb->TransfertDesignRulesToNets();
}


/*****************************************************************/
void DIALOG_DESIGN_RULES::OnCancelButtonClick( wxCommandEvent& event )
/*****************************************************************/
{
    EndModal( 0 );
}


/**************************************************************************/
void DIALOG_DESIGN_RULES::OnOkButtonClick( wxCommandEvent& event )
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

    CopyRulesListToBoard();

    EndModal( wxID_OK );
}


/**************************************************************************/
void DIALOG_DESIGN_RULES::OnLayerCountClick( wxCommandEvent& event )
/**************************************************************************/
{
    m_ActivesLayersCount = m_LayersCountSelection->GetSelection() * 2;
    if( m_ActivesLayersCount <= 0 )
        m_ActivesLayersCount = 1;

    // Reinit the routable layers status
    SetRoutableLayerStatus();
}


/**************************************************************************/
void DIALOG_DESIGN_RULES::OnAddNetclassClick( wxCommandEvent& event )
/**************************************************************************/
{
    wxString class_name;

    if( Get_Message( _( "New Net Class Name:" ),
                     wxEmptyString,
                     class_name,
                     this ) )
        return;

    // The name must dot exists:
    for( int ii = 0; ii < m_gridNetClassesProperties->GetNumberRows(); ii++ )
    {
        wxString value;
        value = m_gridNetClassesProperties->GetRowLabelValue( ii );
        if( class_name.CmpNoCase( value ) == 0 )       // Already exists!
        {
            DisplayError( this, _( "This NetClass is already existing, cannot add it; Aborted" ) );
            return;
        }
    }

    m_gridNetClassesProperties->AppendRows();
    m_gridNetClassesProperties->SetRowLabelValue(
        m_gridNetClassesProperties->GetNumberRows() - 1,
        class_name );

    // Copy values of the previous class:
    int irow = m_gridNetClassesProperties->GetNumberRows() - 1;
    for( int icol = 0; icol < m_gridNetClassesProperties->GetNumberCols(); icol++ )
    {
        wxString value;
        value = m_gridNetClassesProperties->GetCellValue( irow - 1, icol );
        m_gridNetClassesProperties->SetCellValue( irow, icol, value );
    }

    InitializeRulesSelectionBoxes();
}


/**************************************************************************/
void DIALOG_DESIGN_RULES::OnRemoveNetclassClick( wxCommandEvent& event )
/**************************************************************************/
{
    wxArrayInt select = m_gridNetClassesProperties->GetSelectedRows();

    for( int ii = select.GetCount() - 1; ii >= 0; ii-- )
    {
        if( select[ii] != 0 )   // Do not remove the default class
        {
            m_gridNetClassesProperties->DeleteRows( select[ii] );

            // reset the net class to default for nets member of the removed net class
            for( unsigned jj = 0; jj< m_NetsLinkToClasses.size(); jj++ )
                if( m_NetsLinkToClasses[jj] == ii )
                    m_NetsLinkToClasses[jj] = 0;    // Reset to default net class

        }
    }

    InitializeRulesSelectionBoxes();
}


/*
 * Called on the left Choice Box selection
 */
void DIALOG_DESIGN_RULES::OnLeftCBSelection( wxCommandEvent& event )
{
    FillListBoxWithNetsNames( m_listBoxLeftNetSelect, m_CBoxLeftSelection->GetCurrentSelection() );
    if( m_CBoxLeftSelection->GetCurrentSelection() ==  m_CBoxRightSelection->GetCurrentSelection() )
    {
        m_buttonRightToLeft->Enable( false );
        m_buttonLeftToRight->Enable( false );
    }
    else
    {
        m_buttonRightToLeft->Enable( true );
        m_buttonLeftToRight->Enable( true );
    }
}


/*
 * Called on the Right Choice Box selection
 */
void DIALOG_DESIGN_RULES::OnRightCBSelection( wxCommandEvent& event )
{
    FillListBoxWithNetsNames( m_listBoxRightNetSelect, m_CBoxRightSelection->GetCurrentSelection() );
    if( m_CBoxLeftSelection->GetCurrentSelection() ==  m_CBoxRightSelection->GetCurrentSelection() )
    {
        m_buttonRightToLeft->Enable( false );
        m_buttonLeftToRight->Enable( false );;
    }
    else
    {
        m_buttonRightToLeft->Enable( true );
        m_buttonLeftToRight->Enable( true );
    }
}


/* Called on clicking the "<<<" or Copy Right to Left button:
 * Selected items are moved from the right list to the left list
 */

void DIALOG_DESIGN_RULES::OnRightToLeftCopyButton( wxCommandEvent& event )
{
    int idx_class = m_CBoxLeftSelection->GetCurrentSelection();

    if( idx_class == wxNOT_FOUND )
        return;
    for( unsigned ii = 0; ii < m_listBoxRightNetSelect->GetCount(); ii++ )
    {
        if( !m_listBoxRightNetSelect->IsSelected( ii ) )
            continue;

        // This is a trick to get an unsigned integer index from a pointer value.
        // Some compilers cannot accept to convert a pointer to an unsigned without complains
        char * ptr = (char*) m_listBoxRightNetSelect->GetClientData( ii );
        unsigned idx = ptr - (char*)0;
        m_NetsLinkToClasses[idx] = idx_class;
    }

    FillListBoxWithNetsNames( m_listBoxLeftNetSelect, m_CBoxLeftSelection->GetCurrentSelection() );
    FillListBoxWithNetsNames( m_listBoxRightNetSelect, m_CBoxRightSelection->GetCurrentSelection() );
}


/* Called on clicking the ">>>" or Copy Left to Right button:
 * Selected items are moved from the left list to the right list
 */
void DIALOG_DESIGN_RULES::OnLeftToRightCopyButton( wxCommandEvent& event )
{
    int idx_class = m_CBoxRightSelection->GetCurrentSelection();

    if( idx_class == wxNOT_FOUND )
        return;
    for( unsigned ii = 0; ii < m_listBoxLeftNetSelect->GetCount(); ii++ )
    {
        if( !m_listBoxLeftNetSelect->IsSelected( ii ) )
            continue;

        // This is a trick to get an unsigned integer index from a pointer value.
        // Some compilers cannot accept to convert a pointer to an unsigned without complains
        char * ptr = (char*) m_listBoxLeftNetSelect->GetClientData( ii );
        unsigned idx = ptr - (char*)0 ;
        m_NetsLinkToClasses[idx] = idx_class;
    }

    FillListBoxWithNetsNames( m_listBoxLeftNetSelect, m_CBoxLeftSelection->GetCurrentSelection() );
    FillListBoxWithNetsNames( m_listBoxRightNetSelect, m_CBoxRightSelection->GetCurrentSelection() );
}


/* Called on clicking the left "select all" button:
 * select alls items of the left netname list lisxt box
 */
void DIALOG_DESIGN_RULES::OnLeftSelectAllButton( wxCommandEvent& event )
{
    for( unsigned ii = 0; ii < m_listBoxLeftNetSelect->GetCount(); ii++ )
        m_listBoxLeftNetSelect->SetSelection( ii );
}


/* Called on clicking the right "select all" button:
 * select alls items of the right netname list lisxt box
 */
void DIALOG_DESIGN_RULES::OnRightSelectAllButton( wxCommandEvent& event )
{
    for( unsigned ii = 0; ii < m_listBoxRightNetSelect->GetCount(); ii++ )
        m_listBoxRightNetSelect->SetSelection( ii );
}


/* TestDataValidity
 * Performs a control of data validity
 * set the background of a bad cell in RED and display an info message
 * @return true if Ok, false if error
 */
bool DIALOG_DESIGN_RULES::TestDataValidity()
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

    int value;
    int minvalue;

    for( int ii = 0; ii < m_gridNetClassesProperties->GetNumberRows(); ii++ )
    {
        value = ReturnValueFromString( g_UnitMetric,
                                       m_gridNetClassesProperties->GetCellValue(
                                           ii, RULE_GRID_TRACKSIZE_POSITION ),
                                       m_Parent->m_InternalUnits );
        minvalue = ReturnValueFromString( g_UnitMetric,
                                          m_gridNetClassesProperties->GetCellValue( ii,
                                                                                    RULE_GRID_MINTRACKSIZE_POSITION ),
                                          m_Parent->m_InternalUnits );
        if( value < minvalue )
        {
            success = false;
            m_MessagesList->AppendToPage( _( "The <b>track</b> minimum size is bigger than the size<br>" ) );
        }

        value = ReturnValueFromString( g_UnitMetric,
                                       m_gridNetClassesProperties->GetCellValue(
                                           ii, RULE_GRID_VIASIZE_POSITION ),
                                       m_Parent->m_InternalUnits );
        minvalue = ReturnValueFromString( g_UnitMetric,
                                          m_gridNetClassesProperties->GetCellValue( ii,
                                                                                    RULE_GRID_MINVIASIZE_POSITION ),
                                          m_Parent->m_InternalUnits );
        if( value < minvalue )
        {
            success = false;
            m_MessagesList->AppendToPage( _( "The <b>via</b> minimum size is bigger than the size<br>" ) );
        }
    }

    return success;
}
