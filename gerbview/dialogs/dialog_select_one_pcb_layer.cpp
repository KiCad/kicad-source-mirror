/**
 * @file dialog_select_one_pcb_layer.cpp
 * @brief Set up a dialog to choose a PCB Layer.
 */

#include <fctsys.h>
#include <gerbview_frame.h>
#include <select_layers_to_pcb.h>

#define NB_PCB_LAYERS LAYER_ID_COUNT
#define FIRST_COPPER_LAYER 0
#define LAST_COPPER_LAYER 31

// Exported function
const wxString GetPCBDefaultLayerName( int aLayerId );


enum layer_sel_id {
    ID_LAYER_SELECT_TOP = 1800,
    ID_LAYER_SELECT_BOTTOM,
    ID_LAYER_SELECT
};


class SELECT_LAYER_DIALOG : public wxDialog
{
private:
    wxRadioBox*     m_layerList;
    std::vector <int> m_layerId;

public:
    // Constructor and destructor
    SELECT_LAYER_DIALOG( GERBVIEW_FRAME* parent, int aDefaultLayer,
                         int aCopperLayerCount, bool aShowDeselectOption );
    ~SELECT_LAYER_DIALOG() { };

private:
    void OnLayerSelected( wxCommandEvent& event );
    void OnCancelClick( wxCommandEvent& event );

    DECLARE_EVENT_TABLE()
};


BEGIN_EVENT_TABLE( SELECT_LAYER_DIALOG, wxDialog )
    EVT_BUTTON( wxID_OK, SELECT_LAYER_DIALOG::OnLayerSelected )
    EVT_BUTTON( wxID_CANCEL, SELECT_LAYER_DIALOG::OnCancelClick )
    EVT_RADIOBOX( ID_LAYER_SELECT, SELECT_LAYER_DIALOG::OnLayerSelected )
END_EVENT_TABLE()


/** Install the dialog box for layer selection
 * @param aDefaultLayer = Preselection (GERBER_DRAWLAYERS_COUNT for "(Deselect)" layer)
 * @param aCopperLayerCount = number of copper layers
 * @param aShowDeselectOption = display a "(Deselect)" radiobutton (when set to true)
 * @return new layer value (GERBER_DRAWLAYERS_COUNT when "(Deselect)" radiobutton selected),
 *                         or -1 if canceled
 *
 * Providing the option to also display a "(Deselect)" radiobutton makes the
 *  GerbView's "Export to Pcbnew" command) more "user friendly",
 * by permitting any layer to be "deselected" immediately after its
 * corresponding radiobutton has been clicked on. (It would otherwise be
 * necessary to first cancel the "Select Layer:" dialog box (invoked after a
 * different radiobutton is clicked on) prior to then clicking on the "Deselect"
 * button provided within the "Layer selection:" dialog box).
 */
int GERBVIEW_FRAME::SelectPCBLayer( int aDefaultLayer, int aCopperLayerCount,
                                          bool aShowDeselectOption )
{
    SELECT_LAYER_DIALOG* frame = new SELECT_LAYER_DIALOG( this, aDefaultLayer,
                                                          aCopperLayerCount,
                                                          aShowDeselectOption );

    int layer = frame->ShowModal();
    frame->Destroy();
    return layer;
}


/*
 * The "OK" and "Cancel" buttons are positioned (in a horizontal line)
 * beneath the "Layer" radiobox, unless that contains only one column of
 * radiobuttons, in which case they are positioned (in a vertical line)
 * to the right of that radiobox.
 */
SELECT_LAYER_DIALOG::SELECT_LAYER_DIALOG( GERBVIEW_FRAME* parent,
                                          int aDefaultLayer, int aCopperLayerCount,
                                          bool aShowDeselectOption ) :
    wxDialog( parent, -1, _( "Select Layer:" ), wxPoint( -1, -1 ),
              wxSize( 470, 250 ),
              wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER )
{
    wxButton* button;
    int ii;
    wxArrayString  layerList;
    int layerSelect = -1;

    // Build the layer list; first build copper layers list
    int layerCount = 0;

    for( ii = FIRST_COPPER_LAYER; ii <= LAST_COPPER_LAYER; ++ii )
    {
        if( ii == FIRST_COPPER_LAYER || ii == LAST_COPPER_LAYER || ii < aCopperLayerCount-1 )
        {
            layerList.Add( GetPCBDefaultLayerName( ii ) );

            if( ii == aDefaultLayer )
                layerSelect = layerCount;

            m_layerId.push_back( ii );
            layerCount++;
        }
    }

    // Build the layer list; build non copper layers list
    for( ; ii < NB_PCB_LAYERS; ++ii )
    {
        layerList.Add( GetPCBDefaultLayerName( ii ) );

        if( ii == aDefaultLayer )
            layerSelect = layerCount;

        m_layerId.push_back( ii );
        layerCount++;
    }

    // When appropriate, also provide a "(Deselect)" radiobutton
    if( aShowDeselectOption )
    {
        layerList.Add( _( "Do not export" ) );

        if( UNSELECTED_LAYER == aDefaultLayer )
            layerSelect = layerCount;

        m_layerId.push_back( UNSELECTED_LAYER );
        layerCount++;
    }

    m_layerList = new wxRadioBox( this, ID_LAYER_SELECT, _( "Layer" ),
                                  wxPoint( -1, -1 ), wxSize( -1, -1 ),
                                  layerList,
                                  (layerCount < 8) ? layerCount : 8,
                                  wxRA_SPECIFY_ROWS );

    if( layerSelect >= 0 )
        m_layerList->SetSelection( layerSelect );

    wxBoxSizer* FrameBoxSizer = new wxBoxSizer( wxHORIZONTAL );
    SetSizer( FrameBoxSizer );
    FrameBoxSizer->Add( m_layerList, 0, wxALIGN_TOP | wxALL, 5 );
    wxBoxSizer* ButtonBoxSizer = new wxBoxSizer( wxVERTICAL );
    FrameBoxSizer->Add( ButtonBoxSizer, 0, wxALIGN_BOTTOM | wxALL, 0 );

    button = new wxButton( this, wxID_OK, _( "OK" ) );
    button->SetDefault();
    ButtonBoxSizer->Add( button, 0, wxGROW | wxALL, 5 );

    button = new wxButton( this, wxID_CANCEL, _( "Cancel" ) );
    ButtonBoxSizer->Add( button, 0, wxGROW | wxALL, 5 );

    GetSizer()->SetSizeHints( this );

    Center();
}


void SELECT_LAYER_DIALOG::OnLayerSelected( wxCommandEvent& event )
{
    int ii = m_layerId[m_layerList->GetSelection()];

    EndModal( ii );
}


void SELECT_LAYER_DIALOG::OnCancelClick( wxCommandEvent& event )
{
    EndModal( -1 );
}

// This function is a duplicate of
// const wxChar* LSET::Name( LAYER_ID aLayerId )
// However it avoids a dependency to Pcbnew code.
const wxString GetPCBDefaultLayerName( int aLayerId )
{
    const wxChar* txt;

    // using a switch to explicitly show the mapping more clearly
    switch( aLayerId )
    {
    case F_Cu:              txt = wxT( "F.Cu" );            break;
    case In1_Cu:            txt = wxT( "In1.Cu" );          break;
    case In2_Cu:            txt = wxT( "In2.Cu" );          break;
    case In3_Cu:            txt = wxT( "In3.Cu" );          break;
    case In4_Cu:            txt = wxT( "In4.Cu" );          break;
    case In5_Cu:            txt = wxT( "In5.Cu" );          break;
    case In6_Cu:            txt = wxT( "In6.Cu" );          break;
    case In7_Cu:            txt = wxT( "In7.Cu" );          break;
    case In8_Cu:            txt = wxT( "In8.Cu" );          break;
    case In9_Cu:            txt = wxT( "In9.Cu" );          break;
    case In10_Cu:           txt = wxT( "In10.Cu" );         break;
    case In11_Cu:           txt = wxT( "In11.Cu" );         break;
    case In12_Cu:           txt = wxT( "In12.Cu" );         break;
    case In13_Cu:           txt = wxT( "In13.Cu" );         break;
    case In14_Cu:           txt = wxT( "In14.Cu" );         break;
    case In15_Cu:           txt = wxT( "In15.Cu" );         break;
    case In16_Cu:           txt = wxT( "In16.Cu" );         break;
    case In17_Cu:           txt = wxT( "In17.Cu" );         break;
    case In18_Cu:           txt = wxT( "In18.Cu" );         break;
    case In19_Cu:           txt = wxT( "In19.Cu" );         break;
    case In20_Cu:           txt = wxT( "In20.Cu" );         break;
    case In21_Cu:           txt = wxT( "In21.Cu" );         break;
    case In22_Cu:           txt = wxT( "In22.Cu" );         break;
    case In23_Cu:           txt = wxT( "In23.Cu" );         break;
    case In24_Cu:           txt = wxT( "In24.Cu" );         break;
    case In25_Cu:           txt = wxT( "In25.Cu" );         break;
    case In26_Cu:           txt = wxT( "In26.Cu" );         break;
    case In27_Cu:           txt = wxT( "In27.Cu" );         break;
    case In28_Cu:           txt = wxT( "In28.Cu" );         break;
    case In29_Cu:           txt = wxT( "In29.Cu" );         break;
    case In30_Cu:           txt = wxT( "In30.Cu" );         break;
    case B_Cu:              txt = wxT( "B.Cu" );            break;

    // Technicals
    case B_Adhes:           txt = wxT( "B.Adhes" );         break;
    case F_Adhes:           txt = wxT( "F.Adhes" );         break;
    case B_Paste:           txt = wxT( "B.Paste" );         break;
    case F_Paste:           txt = wxT( "F.Paste" );         break;
    case B_SilkS:           txt = wxT( "B.SilkS" );         break;
    case F_SilkS:           txt = wxT( "F.SilkS" );         break;
    case B_Mask:            txt = wxT( "B.Mask" );          break;
    case F_Mask:            txt = wxT( "F.Mask" );          break;

    // Users
    case Dwgs_User:         txt = wxT( "Dwgs.User" );       break;
    case Cmts_User:         txt = wxT( "Cmts.User" );       break;
    case Eco1_User:         txt = wxT( "Eco1.User" );       break;
    case Eco2_User:         txt = wxT( "Eco2.User" );       break;
    case Edge_Cuts:         txt = wxT( "Edge.Cuts" );       break;
    case Margin:            txt = wxT( "Margin" );          break;

    // Footprint
    case F_CrtYd:           txt = wxT( "F.CrtYd" );         break;
    case B_CrtYd:           txt = wxT( "B.CrtYd" );         break;
    case F_Fab:             txt = wxT( "F.Fab" );           break;
    case B_Fab:             txt = wxT( "B.Fab" );           break;

    default:
        wxASSERT_MSG( 0, wxT( "aLayerId out of range" ) );
                            txt = wxT( "BAD INDEX!" );      break;
    }

    return wxString( txt );
}
