/**
 * @file dialog_select_one_pcb_layer.cpp
 * @brief Set up a dialog to choose a PCB Layer.
 */

#include <fctsys.h>
#include <gerbview_frame.h>
#include <select_layers_to_pcb.h>

// Exported function
const wxString GetPCBDefaultLayerName( LAYER_NUM aLayerNumber );


enum layer_sel_id {
    ID_LAYER_SELECT_TOP = 1800,
    ID_LAYER_SELECT_BOTTOM,
    ID_LAYER_SELECT
};


class SELECT_LAYER_DIALOG : public wxDialog
{
private:
    GERBVIEW_FRAME* m_Parent;
    wxRadioBox*     m_LayerList;
    LAYER_NUM m_LayerId[int(NB_LAYERS) + 1]; // One extra element for "(Deselect)" radiobutton

public:
    // Constructor and destructor
    SELECT_LAYER_DIALOG( GERBVIEW_FRAME* parent, LAYER_NUM aDefaultLayer,
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
 * @param aDefaultLayer = Preselection (NB_LAYERS for "(Deselect)" layer)
 * @param aCopperLayerCount = number of copper layers
 * @param aShowDeselectOption = display a "(Deselect)" radiobutton (when set to true)
 * @return new layer value (NB_LAYERS when "(Deselect)" radiobutton selected),
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
LAYER_NUM GERBVIEW_FRAME::SelectPCBLayer( LAYER_NUM aDefaultLayer, int aCopperLayerCount,
                                          bool aShowDeselectOption )
{
    LAYER_NUM layer;
    SELECT_LAYER_DIALOG* frame = new SELECT_LAYER_DIALOG( this, aDefaultLayer,
                                                          aCopperLayerCount,
                                                          aShowDeselectOption );

    layer = frame->ShowModal();
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
                                          LAYER_NUM aDefaultLayer, int aCopperLayerCount,
                                          bool aShowDeselectOption ) :
    wxDialog( parent, -1, _( "Select Layer:" ), wxPoint( -1, -1 ),
              wxSize( 470, 250 ),
              wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER )
{
    wxButton* Button;
    LAYER_NUM ii;
    wxString  LayerList[NB_PCB_LAYERS + 1]; // One extra element for "(Deselect)"
                                        // radiobutton
    int       LayerCount, LayerSelect = -1;

    m_Parent = parent;

    // Build the layer list; first build copper layers list
    LayerCount = 0;
    for( ii = FIRST_COPPER_LAYER; ii < NB_COPPER_LAYERS; ++ii )
    {
        m_LayerId[ii] = FIRST_LAYER;

        if( ii == FIRST_COPPER_LAYER || ii == LAST_COPPER_LAYER || ii < aCopperLayerCount-1 )
        {
            LayerList[LayerCount] = GetPCBDefaultLayerName( ii );

            if( ii == aDefaultLayer )
                LayerSelect = LayerCount;

            m_LayerId[LayerCount] = ii;
            LayerCount++;
        }
    }
    // Build the layer list; build non copper layers list
    for( ; ii < NB_PCB_LAYERS; ++ii )
    {
        m_LayerId[ii] = FIRST_LAYER;

        LayerList[LayerCount] = GetPCBDefaultLayerName( ii );

        if( ii == aDefaultLayer )
            LayerSelect = LayerCount;

        m_LayerId[LayerCount] = ii;
        LayerCount++;
    }

    // When appropriate, also provide a "(Deselect)" radiobutton
    if( aShowDeselectOption )
    {
        LayerList[LayerCount] = _( "(Deselect)" );

        if( NB_PCB_LAYERS == aDefaultLayer )
            LayerSelect = LayerCount;

        m_LayerId[LayerCount] = NB_PCB_LAYERS;
        LayerCount++;
    }

    m_LayerList = new wxRadioBox( this, ID_LAYER_SELECT, _( "Layer" ),
                                  wxPoint( -1, -1 ), wxSize( -1, -1 ),
                                  LayerCount, LayerList,
                                  (LayerCount < 8) ? LayerCount : 8,
                                  wxRA_SPECIFY_ROWS );

    if( LayerSelect >= 0 )
        m_LayerList->SetSelection( LayerSelect );

    wxBoxSizer* FrameBoxSizer = new wxBoxSizer( wxHORIZONTAL );
    SetSizer( FrameBoxSizer );
    FrameBoxSizer->Add( m_LayerList, 0, wxALIGN_TOP | wxALL, 5 );
    wxBoxSizer* ButtonBoxSizer = new wxBoxSizer( wxVERTICAL );
    FrameBoxSizer->Add( ButtonBoxSizer, 0, wxALIGN_BOTTOM | wxALL, 0 );

    Button = new wxButton( this, wxID_OK, _( "OK" ) );
    Button->SetDefault();
    ButtonBoxSizer->Add( Button, 0, wxGROW | wxALL, 5 );

    Button = new wxButton( this, wxID_CANCEL, _( "Cancel" ) );
    ButtonBoxSizer->Add( Button, 0, wxGROW | wxALL, 5 );

    SetFocus();

    GetSizer()->SetSizeHints( this );

    Center();
}


void SELECT_LAYER_DIALOG::OnLayerSelected( wxCommandEvent& event )
{
    int ii = m_LayerId[m_LayerList->GetSelection()];

    EndModal( ii );
}


void SELECT_LAYER_DIALOG::OnCancelClick( wxCommandEvent& event )
{
    EndModal( -1 );
}

const wxString GetPCBDefaultLayerName( LAYER_NUM aLayerNumber )
{
    const wxChar* txt;

    // Use a switch to explicitly show the mapping more clearly
    switch( aLayerNumber )
    {
    case F_Cu:         txt = wxT( "F.Cu" );            break;
    case LAYER_N_2:             txt = wxT( "Inner1.Cu" );       break;
    case LAYER_N_3:             txt = wxT( "Inner2.Cu" );       break;
    case LAYER_N_4:             txt = wxT( "Inner3.Cu" );       break;
    case LAYER_N_5:             txt = wxT( "Inner4.Cu" );       break;
    case LAYER_N_6:             txt = wxT( "Inner5.Cu" );       break;
    case LAYER_N_7:             txt = wxT( "Inner6.Cu" );       break;
    case LAYER_N_8:             txt = wxT( "Inner7.Cu" );       break;
    case LAYER_N_9:             txt = wxT( "Inner8.Cu" );       break;
    case LAYER_N_10:            txt = wxT( "Inner9.Cu" );       break;
    case LAYER_N_11:            txt = wxT( "Inner10.Cu" );      break;
    case LAYER_N_12:            txt = wxT( "Inner11.Cu" );      break;
    case LAYER_N_13:            txt = wxT( "Inner12.Cu" );      break;
    case LAYER_N_14:            txt = wxT( "Inner13.Cu" );      break;
    case LAYER_N_15:            txt = wxT( "Inner14.Cu" );      break;
    case B_Cu:          txt = wxT( "B.Cu" );            break;
    case B_Adhes:       txt = wxT( "B.Adhes" );         break;
    case F_Adhes:      txt = wxT( "F.Adhes" );         break;
    case B_Paste:    txt = wxT( "B.Paste" );         break;
    case F_Paste:   txt = wxT( "F.Paste" );         break;
    case B_SilkS:     txt = wxT( "B.SilkS" );         break;
    case F_SilkS:    txt = wxT( "F.SilkS" );         break;
    case B_Mask:     txt = wxT( "B.Mask" );          break;
    case F_Mask:    txt = wxT( "F.Mask" );          break;
    case Dwgs_User:                txt = wxT( "Dwgs.User" );       break;
    case Cmts_User:             txt = wxT( "Cmts.User" );       break;
    case Eco1_User:                txt = wxT( "Eco1.User" );       break;
    case Eco2_User:                txt = wxT( "Eco2.User" );       break;
    case Edge_Cuts:                txt = wxT( "Edge.Cuts" );       break;
    default:                    txt = wxT( "BAD_INDEX" );       break;
    }

    return wxString( txt );
}
