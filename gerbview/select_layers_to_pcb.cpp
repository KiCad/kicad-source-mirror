/*******************************************************/
/* Dialog frame to choose gerber layers and pcb layers */
/*******************************************************/

/* select_layers_to_pcb.cpp */

#include "fctsys.h"
#include "common.h"
#include "gerbview.h"
#include "class_board_design_settings.h"

#include "protos.h"

#include "wx/statline.h"

#define LAYER_UNSELECTED NB_LAYERS

static int    ButtonTable[32];       // Indexes buttons to Gerber layers
static int    LayerLookUpTable[32];  // Indexes Gerber layers to PCB file layers
wxStaticText* layer_list[32];        // Indexes text strings to buttons

enum swap_layer_id {
    ID_WINEDA_SWAPLAYERFRAME = 1800,
    ID_BUTTON_0,
    ID_TEXT_0 = ID_BUTTON_0 + 32
};


class WinEDA_SwapLayerFrame : public wxDialog
{
private:
    WinEDA_GerberFrame*     m_Parent;
    wxBoxSizer*             OuterBoxSizer;
    wxBoxSizer*             MainBoxSizer;
    wxFlexGridSizer*        FlexColumnBoxSizer;
    wxStaticText*           label;
    wxButton*               Button;
    wxStaticText*           text;
    wxStaticLine*           Line;
    wxStdDialogButtonSizer* StdDialogButtonSizer;

public:

    WinEDA_SwapLayerFrame( WinEDA_GerberFrame* parent );
    ~WinEDA_SwapLayerFrame() {};

private:
    void OnSelectLayer( wxCommandEvent& event );
    void OnOkClick( wxCommandEvent& event );
    void OnCancelClick( wxCommandEvent& event );

    DECLARE_EVENT_TABLE()
};


BEGIN_EVENT_TABLE( WinEDA_SwapLayerFrame, wxDialog )
    EVT_COMMAND_RANGE( ID_BUTTON_0, ID_BUTTON_0 + 31,
                       wxEVT_COMMAND_BUTTON_CLICKED,
                       WinEDA_SwapLayerFrame::OnSelectLayer )
    EVT_BUTTON( wxID_OK, WinEDA_SwapLayerFrame::OnOkClick )
    EVT_BUTTON( wxID_CANCEL, WinEDA_SwapLayerFrame::OnCancelClick )
END_EVENT_TABLE()


/* Install a dialog frame to choose the equivalence
 * between gerber layers and pcbnew layers
 * return the "lookup table" if ok, or NULL
 */
int* InstallDialogLayerPairChoice( WinEDA_GerberFrame * parent )
{
    WinEDA_SwapLayerFrame* frame = new WinEDA_SwapLayerFrame( parent );

    int ii = frame->ShowModal();

    frame->Destroy();
    if( ii >= 0 )
        return LayerLookUpTable;
    else
        return NULL;
}


WinEDA_SwapLayerFrame::WinEDA_SwapLayerFrame( WinEDA_GerberFrame* parent ) :
    wxDialog( parent, -1, _( "Layer selection:" ), wxPoint( -1, -1 ),
              wxDefaultSize, wxDEFAULT_DIALOG_STYLE | MAYBE_RESIZE_BORDER )
{
    OuterBoxSizer = NULL;
    MainBoxSizer  = NULL;
    FlexColumnBoxSizer = NULL;
    label  = NULL;
    Button = NULL;
    text   = NULL;
    Line   = NULL;
    StdDialogButtonSizer = NULL;

    m_Parent = parent;

    int      item_ID, ii, nb_items;
    wxString msg;
    wxSize   goodSize;

    // Experimentation has shown that buttons in the Windows version can be 20
    // pixels wide and 20 pixels high, but that they need to be 26 pixels wide
    // and 26 pixels high in the Linux version. (And although the dimensions
    // of those buttons could be set to 26 pixels wide and 26 pixels high in
    // both of those versions, that would result in a dialog box which would
    // be excessively high in the Windows version.)
#ifdef __WINDOWS__
    int w = 20;
    int h = 20;
#else
    int w = 26;
    int h = 26;
#endif

    // As currently implemented, the dimensions of the buttons in the Mac
    // version are also 26 pixels wide and 26 pixels high. If appropriate,
    // the above code should be modified as required in the event that those
    // buttons should be some other size in that version.

    // Compute a reasonable number of copper layers
    int pcb_layer_number = 0;
    for( ii = 0; ii < 32; ii++ )
    {
        if( g_GERBER_List[ii] != NULL )
            pcb_layer_number++;

        // Specify the default value for each member of these arrays.
        ButtonTable[ii] = -1;
        LayerLookUpTable[ii] = LAYER_UNSELECTED;
    }
    g_DesignSettings.SetCopperLayerCount(pcb_layer_number);

    pcb_layer_number = 0;
    for( nb_items = 0, ii = 0; ii < 32; ii++ )
    {
        if( g_GERBER_List[ii] == NULL )
            continue;

        if( (pcb_layer_number == g_DesignSettings.GetCopperLayerCount() - 1)
           && (g_DesignSettings.GetCopperLayerCount() > 1) )
            pcb_layer_number = CMP_N;

        ButtonTable[nb_items] = ii;
        LayerLookUpTable[ii]  = pcb_layer_number;
        nb_items++;
        pcb_layer_number++;
    }

    OuterBoxSizer = new wxBoxSizer( wxVERTICAL );
    SetSizer( OuterBoxSizer );

    MainBoxSizer = new wxBoxSizer( wxHORIZONTAL );
    OuterBoxSizer->Add( MainBoxSizer, 1, wxGROW | wxLEFT | wxRIGHT | wxTOP, 5 );

    for( ii = 0; ii < nb_items; ii++ )
    {
        // If more than 16 Gerber layers are used, provide a vertical line to
        // separate the two FlexGrid sizers
        if( (nb_items > 16) && (ii == 16) )
        {
            Line = new wxStaticLine( this, -1, wxDefaultPosition, wxDefaultSize,
                                     wxLI_VERTICAL );
            MainBoxSizer->Add( Line, 0, wxGROW | wxLEFT | wxRIGHT, 5 );
        }

        // Provide a separate FlexGrid sizer for every sixteen sets of controls
        if( ii % 16 == 0 )
        {
            // Each Gerber layer has an associated static text string (to
            // identify that layer), a button (for invoking a child dialog
            // box to change which pcbnew layer that the Gerber layer is
            // mapped to), and a second static text string (to depict which
            // pcbnew layer that the Gerber layer has been mapped to). Each
            // of those items are placed into the left hand column, middle
            // column, and right hand column (respectively) of the Flexgrid
            // sizer, and the color of the second text string is set to
            // fuchsia or blue (to respectively indicate whether the Gerber
            // layer has been mapped to a pcbnew layer or is not being
            // exported at all).  (Experimentation has shown that if a text
            // control is used to depict which pcbnew layer that each Gerber
            // layer is mapped to (instead of a static text string), then
            // those controls do not behave in a fully satisfactory manner
            // in the Linux version. Even when the read-only attribute is
            // specified for all of those controls, they can still be selected
            // when the arrow keys or Tab key is used to step through all of
            // the controls within the dialog box, and directives to set the
            // foreground color of the text of each such control to blue (to
            // indicate that the text is of a read-only nature) are disregarded.
            // Specify a FlexGrid sizer with an appropriate number of rows
            // and three columns.  If nb_items < 16, then the number of rows
            // is nb_items; otherwise, the number of rows is 16 (with two
            // separate columns of controls being used if nb_items > 16).

            if( nb_items < 16 )
                FlexColumnBoxSizer = new wxFlexGridSizer( nb_items, 4, 0, 0 );
            else
                FlexColumnBoxSizer = new wxFlexGridSizer( 16, 4, 0, 0 );

            // Specify that all of the rows can be expanded.
            for( int jj = 0; jj < MIN( nb_items, 16 ); jj++ )
            {
                FlexColumnBoxSizer->AddGrowableRow( jj );
            }

            // Specify that (just) the right-hand column can be expanded.
            FlexColumnBoxSizer->AddGrowableCol( 2 );

            MainBoxSizer->Add( FlexColumnBoxSizer, 1, wxGROW | wxTOP, 5 );
        }

        // Provide a text string to identify the Gerber layer
        msg = _( "Layer " );
        msg << ButtonTable[ii] + 1;

        label = new wxStaticText( this, wxID_STATIC, msg, wxDefaultPosition,
                                  wxDefaultSize, wxALIGN_RIGHT );
        FlexColumnBoxSizer->Add( label, 0,
                                 wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL |
                                 wxALL, 5 );

        /* Add file name and extension without path. */
        wxFileName fn( g_GERBER_List[ii]->m_FileName );
        label = new wxStaticText( this, wxID_STATIC, fn.GetFullName(),
                                  wxDefaultPosition, wxDefaultSize );
        FlexColumnBoxSizer->Add( label, 0,
                                 wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL |
                                 wxALL, 5 );

        // Provide a button for this layer (which will invoke a child dialog box)
        item_ID = ID_BUTTON_0 + ii;

        Button = new wxButton( this, item_ID, wxT( "..." ),
                               wxDefaultPosition, wxSize( w, h ), 0 );

        FlexColumnBoxSizer->Add( Button, 0,
                                 wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL |
                                 wxALL, 5 );

        // Provide another text string to specify which pcbnew layer that this
        // Gerber layer is initially mapped to, and set the initial text to
        // specify the appropriate pcbnew layer, and set the foreground color
        // of the text to fuchsia (to indicate that the layer is being exported).
        item_ID = ID_TEXT_0 + ii;

        // When the first of these text strings is being added, determine what
        // size is necessary to to be able to display any possible string
        // without it being truncated. Then specify that size as the minimum
        // size for all of these text strings. (If this minimum size is not
        // determined in this fashion, then it is possible for the display of
        // one or more of these strings to be truncated after different pcbnew
        // layers are selected.)
        if( ii == 0 )
        {
            msg = _( "Do not export" );
            text = new wxStaticText( this, item_ID, msg, wxDefaultPosition,
                                     wxDefaultSize, 0 );
            goodSize = text->GetSize();

            for( int jj = 0; jj < NB_LAYERS; jj++ )
            {
                text->SetLabel( ReturnPcbLayerName( jj ) );
                if( goodSize.x < text->GetSize().x )
                    goodSize.x = text->GetSize().x;
            }

            msg = ReturnPcbLayerName( LayerLookUpTable[ButtonTable[ii]] );
            text->SetLabel( msg );
        }
        else
        {
            msg  = ReturnPcbLayerName( LayerLookUpTable[ButtonTable[ii]] );
            text = new wxStaticText( this, item_ID, msg, wxDefaultPosition,
                                     wxDefaultSize, 0 );
        }
        text->SetMinSize( goodSize );
        FlexColumnBoxSizer->Add( text, 1,
                                 wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxALL,
                                 5 );

        layer_list[ii] = text;
    }

    // If required, provide spacers to occupy otherwise blank cells within the
    // second FlexGrid sizer. (As it incorporates three columns, three spacers
    // are thus required for each otherwise unused row.)
    if( 16 < nb_items && nb_items < 32 )
    {
        for( ii = 4 * nb_items; ii < 96; ii++ )
        {
            FlexColumnBoxSizer->Add( 5, h, 0,
                                     wxALIGN_CENTER_HORIZONTAL |
                                     wxALIGN_CENTER_VERTICAL | wxLEFT |
                                     wxRIGHT | wxBOTTOM, 5 );
        }
    }

    // Provide a line to separate the controls which have been provided so far
    // from the OK and Cancel buttons (which will be provided after this line)
    Line = new wxStaticLine( this, -1, wxDefaultPosition, wxDefaultSize,
                             wxLI_HORIZONTAL );
    OuterBoxSizer->Add( Line, 0, wxGROW | wxLEFT | wxRIGHT | wxTOP, 5 );

    // Provide a StdDialogButtonSizer to accommodate the OK and Cancel buttons;
    // using that type of sizer results in those buttons being automatically
    // located in positions appropriate for each (OS) version of KiCad.
    StdDialogButtonSizer = new wxStdDialogButtonSizer;
    OuterBoxSizer->Add( StdDialogButtonSizer, 0, wxGROW | wxALL, 10 );

    Button = new wxButton( this, wxID_OK, _( "&OK" ), wxDefaultPosition,
                           wxDefaultSize, 0 );
    StdDialogButtonSizer->AddButton( Button );

    Button = new wxButton( this, wxID_CANCEL, _( "&Cancel" ),
                           wxDefaultPosition, wxDefaultSize, 0 );
    StdDialogButtonSizer->AddButton( Button );
    StdDialogButtonSizer->Realize();

    // Resize the dialog
    if( GetSizer() )
    {
        GetSizer()->SetSizeHints( this );
    }
}


void WinEDA_SwapLayerFrame::OnSelectLayer( wxCommandEvent& event )
{
    int ii, jj;

    ii = event.GetId();

    if( ii < ID_BUTTON_0 || ii >= ID_BUTTON_0 + 32 )
        return;

    ii = event.GetId() - ID_BUTTON_0;

    jj = LayerLookUpTable[ButtonTable[ii]];
    if( ( jj < 0 ) || ( jj > LAYER_UNSELECTED ) )
        jj = 0; // (Defaults to "Copper" layer.)
    jj = m_Parent->SelectLayer( jj, -1, -1, true );

    if( ( jj < 0 ) || ( jj > LAYER_UNSELECTED ) )
        return;

    if( jj != LayerLookUpTable[ButtonTable[ii]] )
    {
        LayerLookUpTable[ButtonTable[ii]] = jj;
        if( jj == LAYER_UNSELECTED )
        {
            layer_list[ii]->SetLabel( _( "Do not export" ) );

            // Change the text color to blue (to highlight
            // that this layer is *not* being exported)
            layer_list[ii]->SetForegroundColour( *wxBLUE );
        }
        else
        {
            layer_list[ii]->SetLabel( ReturnPcbLayerName( jj ) );

            // Change the text color to fuchsia (to highlight
            // that this layer *is* being exported)
            layer_list[ii]->SetForegroundColour( wxColour( 255, 0, 128 ) );
        }
    }
}


void WinEDA_SwapLayerFrame::OnCancelClick( wxCommandEvent& event )
{
    EndModal( -1 );
}


void WinEDA_SwapLayerFrame::OnOkClick( wxCommandEvent& event )
{
    int  ii;
    bool AsCmpLayer = false;

    /* Compute the number of copper layers
     * this is the max layer number + 1 (if some internal layers exist)
     */
    int layers_count = 1;
    for( ii = 0; ii < 32; ii++ )
    {
        if( LayerLookUpTable[ii] == CMP_N )
            AsCmpLayer = true;
        else
        {
            if( LayerLookUpTable[ii] >= LAST_COPPER_LAYER )
                continue; // not a copper layer
            if( LayerLookUpTable[ii] >= layers_count )
                layers_count++;
        }
    }

    if( AsCmpLayer )
        layers_count++;
    if( layers_count > NB_COPPER_LAYERS ) // should not occur.
        layers_count = NB_COPPER_LAYERS;

    g_DesignSettings.SetCopperLayerCount( layers_count );

    EndModal( 1 );
}
