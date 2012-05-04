/**
 * @file select_pcb_layer.cpp
 * @brief Set up a dialog to choose a PCB Layer.
 */

#include <fctsys.h>
#include <gerbview_frame.h>
#include <select_layers_to_pcb.h>

// Exported function
const wxString GetPCBDefaultLayerName( int aLayerNumber );


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
    int m_LayerId[NB_LAYERS + 1]; // One extra element for "(Deselect)" radiobutton

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
int GERBVIEW_FRAME::SelectPCBLayer( int aDefaultLayer, int aCopperLayerCount, bool aShowDeselectOption )
{
    int layer;
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
                                          int aDefaultLayer, int aCopperLayerCount,
                                          bool aShowDeselectOption ) :
    wxDialog( parent, -1, _( "Select Layer:" ), wxPoint( -1, -1 ),
              wxSize( 470, 250 ),
              DIALOG_STYLE )
{
    wxButton* Button;
    int       ii;
    wxString  LayerList[NB_LAYERS + 1]; // One extra element for "(Deselect)"
                                        // radiobutton
    int       LayerCount, LayerSelect = -1;

    m_Parent = parent;

    // Build the layer list; first build copper layers list
    LayerCount = 0;
    for( ii = 0; ii < BOARD_COPPER_LAYERS_MAX_COUNT; ii++ )
    {
        m_LayerId[ii] = 0;

        if( ii == 0 || ii == BOARD_COPPER_LAYERS_MAX_COUNT-1 || ii < aCopperLayerCount-1 )
        {
            LayerList[LayerCount] = GetPCBDefaultLayerName( ii );

            if( ii == aDefaultLayer )
                LayerSelect = LayerCount;

            m_LayerId[LayerCount] = ii;
            LayerCount++;
        }
    }
    // Build the layer list; build copper layers list
    for( ; ii < NB_LAYERS; ii++ )
    {
        m_LayerId[ii] = 0;

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

        if( NB_LAYERS == aDefaultLayer )
            LayerSelect = LayerCount;

        m_LayerId[LayerCount] = NB_LAYERS;
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

const wxString GetPCBDefaultLayerName( int aLayerNumber )
{
    const wxChar* txt;

    // These are only default layer names.  For Pcbnew the copper names
    // may be over-ridden in the BOARD (*.brd) file.

    // Use a switch to explicitly show the mapping more clearly
    switch( aLayerNumber )
    {
    case LAYER_N_FRONT:         txt = _( "Front" );         break;
    case LAYER_N_2:             txt = _( "Inner2" );        break;
    case LAYER_N_3:             txt = _( "Inner3" );        break;
    case LAYER_N_4:             txt = _( "Inner4" );        break;
    case LAYER_N_5:             txt = _( "Inner5" );        break;
    case LAYER_N_6:             txt = _( "Inner6" );        break;
    case LAYER_N_7:             txt = _( "Inner7" );        break;
    case LAYER_N_8:             txt = _( "Inner8" );        break;
    case LAYER_N_9:             txt = _( "Inner9" );        break;
    case LAYER_N_10:            txt = _( "Inner10" );       break;
    case LAYER_N_11:            txt = _( "Inner11" );       break;
    case LAYER_N_12:            txt = _( "Inner12" );       break;
    case LAYER_N_13:            txt = _( "Inner13" );       break;
    case LAYER_N_14:            txt = _( "Inner14" );       break;
    case LAYER_N_15:            txt = _( "Inner15" );       break;
    case LAYER_N_BACK:          txt = _( "Back" );          break;
    case ADHESIVE_N_BACK:       txt = _( "Adhes_Back" );    break;
    case ADHESIVE_N_FRONT:      txt = _( "Adhes_Front" );   break;
    case SOLDERPASTE_N_BACK:    txt = _( "SoldP_Back" );    break;
    case SOLDERPASTE_N_FRONT:   txt = _( "SoldP_Front" );   break;
    case SILKSCREEN_N_BACK:     txt = _( "SilkS_Back" );    break;
    case SILKSCREEN_N_FRONT:    txt = _( "SilkS_Front" );   break;
    case SOLDERMASK_N_BACK:     txt = _( "Mask_Back" );     break;
    case SOLDERMASK_N_FRONT:    txt = _( "Mask_Front" );    break;
    case DRAW_N:                txt = _( "Drawings" );      break;
    case COMMENT_N:             txt = _( "Comments" );      break;
    case ECO1_N:                txt = _( "Eco1" );          break;
    case ECO2_N:                txt = _( "Eco2" );          break;
    case EDGE_N:                txt = _( "PCB_Edges" );     break;
    default:                    txt = _( "BAD INDEX" );     break;
    }

    return wxString( txt );
}


