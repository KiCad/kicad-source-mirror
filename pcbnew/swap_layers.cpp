/*******************************/
/* Dialog frame to swap layers */
/*******************************/

/* Fichier swap_layers */

#include "fctsys.h"
#include "common.h"
#include "pcbnew.h"
#include "protos.h"


#include "wx/statline.h"

/* Variables locales */
#define LAYER_NO_CHANGE NB_LAYERS
static int New_Layer[NB_LAYERS];
wxStaticText* layer_list[NB_LAYERS];

enum swap_layer_id {
    ID_WINEDA_SWAPLAYERFRAME = 1800,
    ID_BUTTON_0,
    ID_TEXT_0 = ID_BUTTON_0 + NB_LAYERS
};


/***********************************************/
/* classe pour la frame de selection de layers */
/***********************************************/

class WinEDA_SwapLayerFrame : public wxDialog
{
private:
    WinEDA_BasePcbFrame*    m_Parent;
    wxBoxSizer*             OuterBoxSizer;
    wxBoxSizer*             MainBoxSizer;
    wxFlexGridSizer*        FlexColumnBoxSizer;
    wxStaticText*           label;
    wxButton*               Button;
    wxStaticText*           text;
    wxStaticLine*           Line;
    wxStdDialogButtonSizer* StdDialogButtonSizer;

public:

    // Constructor and destructor
    WinEDA_SwapLayerFrame( WinEDA_BasePcbFrame * parent );
    ~WinEDA_SwapLayerFrame() { };

private:
    void    Sel_Layer( wxCommandEvent& event );
    void    OnOkClick( wxCommandEvent& event );
    void    OnCancelClick( wxCommandEvent& event );

    DECLARE_EVENT_TABLE()
};

/* Table des evenements pour WinEDA_SwapLayerFrame */
BEGIN_EVENT_TABLE( WinEDA_SwapLayerFrame, wxDialog )
    EVT_COMMAND_RANGE( ID_BUTTON_0, ID_BUTTON_0 + NB_LAYERS - 1,
                       wxEVT_COMMAND_BUTTON_CLICKED,
                       WinEDA_SwapLayerFrame::Sel_Layer )
    EVT_BUTTON( wxID_OK, WinEDA_SwapLayerFrame::OnOkClick )
    EVT_BUTTON( wxID_CANCEL, WinEDA_SwapLayerFrame::OnCancelClick )
END_EVENT_TABLE()


/*************************************************************************/
WinEDA_SwapLayerFrame::WinEDA_SwapLayerFrame( WinEDA_BasePcbFrame* parent ) :
    wxDialog( parent, -1, _( "Swap Layers:" ), wxPoint( -1, -1 ),
              wxDefaultSize, wxDEFAULT_DIALOG_STYLE|MAYBE_RESIZE_BORDER )
/*************************************************************************/
{
    BOARD*  board = parent->m_Pcb;

    OuterBoxSizer = NULL;
    MainBoxSizer = NULL;
    FlexColumnBoxSizer = NULL;
    label = NULL;
    Button = NULL;
    text = NULL;
    Line = NULL;
    StdDialogButtonSizer = NULL;

    m_Parent = parent;
    SetFont( *g_DialogFont );

    int item_ID;
    wxSize goodSize;

    // Experimentation has shown that buttons in the Windows version can be 20 pixels
    // wide and 20 pixels high, but that they need to be 26 pixels wide and 26 pixels
    // high in the Linux version. (And although the dimensions of those buttons could
    // be set to 26 pixels wide and 26 pixels high in both of those versions, that would
    // result in a dialog box which would be excessively high in the Windows version.)
#ifdef __WINDOWS__
    int w = 20;
    int h = 20;
#else
    int w = 26;
    int h = 26;
#endif
    // As currently implemented, the dimensions of the buttons in the Mac version are
    // also 26 pixels wide and 26 pixels high. If appropriate, the above code should be
    // modified as required in the event that those buttons should be some other size
    // in that version.

    OuterBoxSizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(OuterBoxSizer);

    MainBoxSizer = new wxBoxSizer(wxHORIZONTAL);
    OuterBoxSizer->Add(MainBoxSizer, 1, wxGROW|wxLEFT|wxRIGHT|wxTOP, 5);

    for( int ii = 0; ii < NB_LAYERS; ii++ )
    {
        // Provide a vertical line to separate the two FlexGrid sizers
        if( ii == 16 )
        {
            Line = new wxStaticLine( this, -1, wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL );
            MainBoxSizer->Add(Line, 0, wxGROW|wxLEFT|wxRIGHT, 5);
        }

        // Provide a separate FlexGrid sizer for every sixteen sets of controls
        if( ii % 16 == 0 )
        {
            // Each layer has an associated static text string (to identify that layer),
            // a button (for invoking a child dialog box to change which layer that the
            // layer is mapped to), and a second static text string (to depict which layer
            // that the layer has been mapped to). Each of those items are placed into
            // the left hand column, middle column, and right hand column (respectively)
            // of the Flexgrid sizer, and the color of the second text string is set to
            // fushia or blue (to respectively indicate whether the layer has been
            // swapped to another layer or is not being swapped at all).
            // (Experimentation has shown that if a text control is used to depict which
            // layer that each layer is mapped to (instead of a static text string), then
            // those controls do not behave in a fully satisfactory manner in the Linux
            // version. Even when the read-only attribute is specified for all of those
            // controls, they can still be selected when the arrow keys or Tab key is used
            // to step through all of the controls within the dialog box, and directives
            // to set the foreground color of the text of each such control to blue (to
            // indicate that the text is of a read-only nature) are disregarded.)

            // Specify a FlexGrid sizer with sixteen rows and three columns.
            FlexColumnBoxSizer = new wxFlexGridSizer(16, 3, 0, 0);

            // Specify that all of the rows can be expanded.
            for( int jj = 0; jj < 16; jj++ )
            {
                FlexColumnBoxSizer->AddGrowableRow(jj);
            }

            // Specify that (just) the right-hand column can be expanded.
            FlexColumnBoxSizer->AddGrowableCol(2);

            MainBoxSizer->Add(FlexColumnBoxSizer, 1, wxGROW|wxTOP, 5);
        }

        // Provide a text string to identify this layer (with trailing spaces within that string being purged)
        label = new wxStaticText( this, wxID_STATIC, board->GetLayerName( ii ),
                                 wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT );
        FlexColumnBoxSizer->Add(label, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxBOTTOM, 5);

        // Provide a button for this layer (which will invoke a child dialog box)
        item_ID = ID_BUTTON_0 + ii;

        Button = new wxButton( this, item_ID, wxT("..."), wxDefaultPosition, wxSize(w, h), 0 );
        FlexColumnBoxSizer->Add(Button, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxBOTTOM, 5);

        // Provide another text string to specify which layer that this layer is
        // mapped to, set the initial text to "No Change" (to indicate that this
        // layer is currently unmapped to any other layer), and set the foreground
        // color of the text to blue (which also indicates that the layer is
        // currently unmapped to any other layer).
        item_ID = ID_TEXT_0 + ii;

        // When the first of these text strings is being added, determine what size is necessary to
        // to be able to display any possible string without it being truncated. Then specify that
        // size as the minimum size for all of these text strings. (If this minimum size is not
        // determined in this fashion, then it is possible for the display of one or more of these
        // strings to be truncated after different layers are selected.)
        if( ii == 0 )
        {
            text = new wxStaticText( this, item_ID, board->GetLayerName( 0 ), wxDefaultPosition, wxDefaultSize, 0 );
            goodSize = text->GetSize();
            for( int jj = 1; jj < NB_LAYERS; jj++ )
            {
                text->SetLabel( board->GetLayerName( jj ) );
                if( goodSize.x < text->GetSize().x )
                    goodSize.x = text->GetSize().x;
            }
            text->SetLabel( _("No Change") );
            if( goodSize.x < text->GetSize().x )
                goodSize.x = text->GetSize().x;
        }
        else
            text = new wxStaticText( this, item_ID, _("No Change"), wxDefaultPosition, wxDefaultSize, 0 );

        text->SetMinSize( goodSize );
        text->SetForegroundColour( *wxBLUE );
        FlexColumnBoxSizer->Add(text, 1, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM, 5);

        layer_list[ii] = text;
    }

    // Provide spacers to occupy otherwise blank cells within the second FlexGrid sizer. (As it
    // incorporates three columns, three spacers are thus required for each otherwise unused row.)
    for( int ii = 3 * NB_LAYERS; ii < 96; ii++ )
    {
        FlexColumnBoxSizer->Add(5, h, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM, 5);
    }

    // Provide a line to separate the controls which have been provided so far
    // from the OK and Cancel buttons (which will be provided after this line)
    Line = new wxStaticLine( this, -1, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    OuterBoxSizer->Add(Line, 0, wxGROW|wxLEFT|wxRIGHT|wxTOP, 5);

    // Provide a StdDialogButtonSizer to accommodate the OK and Cancel buttons;
    // using that type of sizer results in those buttons being automatically
    // located in positions appropriate for each (OS) version of KiCad.
    StdDialogButtonSizer = new wxStdDialogButtonSizer;
    OuterBoxSizer->Add(StdDialogButtonSizer, 0, wxGROW|wxALL, 10);

    Button = new wxButton( this, wxID_OK, _("&OK"), wxDefaultPosition, wxDefaultSize, 0 );
    Button->SetForegroundColour( *wxRED );
    StdDialogButtonSizer->AddButton(Button);

    Button = new wxButton( this, wxID_CANCEL, _("&Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
    Button->SetForegroundColour( *wxBLUE );
    StdDialogButtonSizer->AddButton(Button);

    StdDialogButtonSizer->Realize();

    // Resize the dialog
    if( GetSizer() )
    {
        GetSizer()->SetSizeHints(this);
    }
}


/***************************************************************/
void WinEDA_SwapLayerFrame::Sel_Layer( wxCommandEvent& event )
/***************************************************************/
{
    int ii, jj;

    ii = event.GetId();

    if( ii < ID_BUTTON_0 || ii >= ID_BUTTON_0 + NB_LAYERS )
        return;

    ii = event.GetId() - ID_BUTTON_0;

    jj = New_Layer[ii];
    if( (jj < 0) || (jj > NB_LAYERS) )
        jj = LAYER_NO_CHANGE; // (Defaults to "No Change".)
    jj = m_Parent->SelectLayer( jj, -1, -1, true );

    if( (jj < 0) || (jj > NB_LAYERS) )
        return;

    // No change if the selected layer matches the layer being edited.
    // (Hence the only way to restore a layer to the "No Change"
    // state is by specifically deselecting it; any attempt
    // to select the same layer (instead) will be ignored.)
    if( jj == ii )
    {
        wxString msg;
        msg = _( "Deselect this layer to select the No Change state" );
        DisplayInfo( this, msg );
        return;
    }

    if( jj != New_Layer[ii] )
    {
        New_Layer[ii] = jj;
        if( jj >= LAYER_NO_CHANGE )
        {
            layer_list[ii]->SetLabel( _( "No Change" ) );
            // Change the text color to blue (to highlight
            // that this layer is *not* being swapped)
            layer_list[ii]->SetForegroundColour( *wxBLUE );
        }
        else
        {
            layer_list[ii]->SetLabel( m_Parent->m_Pcb->GetLayerName( jj ) );
            // Change the text color to fushia (to highlight
            // that this layer *is* being swapped)
            layer_list[ii]->SetForegroundColour( wxColour(255, 0, 128) );
        }
    }
}


/*********************************************************/
void WinEDA_SwapLayerFrame::OnCancelClick( wxCommandEvent& event )
/*********************************************************/
{
    EndModal( -1 );
}


/*********************************************************/
void WinEDA_SwapLayerFrame::OnOkClick( wxCommandEvent& event )
/*********************************************************/
{
    EndModal( 1 );
}


/********************************************************/
void WinEDA_PcbFrame::Swap_Layers( wxCommandEvent& event )
/********************************************************/
/* Swap layers */
{
    int             ii, jj;
    TRACK*          pt_segm;
    DRAWSEGMENT*    pt_drawsegm;
    EDA_BaseStruct* PtStruct;


    /* Init default values */
    for( ii = 0; ii < NB_LAYERS; ii++ )
        New_Layer[ii] = LAYER_NO_CHANGE;

    WinEDA_SwapLayerFrame* frame = new WinEDA_SwapLayerFrame( this );

    ii = frame->ShowModal();
    frame->Destroy();

    if( ii != 1 )
        return; // (Cancelled dialog box returns -1 instead)

    /* Modifications des pistes */
    pt_segm = (TRACK*) m_Pcb->m_Track;
    for( ; pt_segm != NULL; pt_segm = (TRACK*) pt_segm->Pnext )
    {
        m_CurrentScreen->SetModify();
        if( pt_segm->Type() == TYPEVIA )
        {
            SEGVIA* Via = (SEGVIA*) pt_segm;
            if( Via->Shape() == VIA_THROUGH )
                continue;
            int     top_layer, bottom_layer;
            Via->ReturnLayerPair( &top_layer, &bottom_layer );
            if(  New_Layer[bottom_layer] >= 0 && New_Layer[bottom_layer] < LAYER_NO_CHANGE )
                bottom_layer = New_Layer[bottom_layer];
            if( New_Layer[top_layer] >= 0 && New_Layer[top_layer] < LAYER_NO_CHANGE )
                top_layer = New_Layer[top_layer];
            Via->SetLayerPair( top_layer, bottom_layer );
        }
        else
        {
            jj = pt_segm->GetLayer();
            if( New_Layer[jj] >= 0 && New_Layer[jj] < LAYER_NO_CHANGE )
                pt_segm->SetLayer( New_Layer[jj] );
        }
    }

    /* Modifications des zones */
    pt_segm = (TRACK*) m_Pcb->m_Zone;
    for( ; pt_segm != NULL; pt_segm = (TRACK*) pt_segm->Pnext )
    {
        m_CurrentScreen->SetModify();
        jj = pt_segm->GetLayer();
        if( New_Layer[jj] >= 0 && New_Layer[jj] < LAYER_NO_CHANGE )
            pt_segm->SetLayer( New_Layer[jj] );
    }

    /* Modifications des autres segments */
    PtStruct = m_Pcb->m_Drawings;
    for( ; PtStruct != NULL; PtStruct = PtStruct->Pnext )
    {
        if( PtStruct->Type() == TYPEDRAWSEGMENT )
        {
            m_CurrentScreen->SetModify();
            pt_drawsegm = (DRAWSEGMENT*) PtStruct;
            jj = pt_drawsegm->GetLayer();
            if( New_Layer[jj] >= 0 && New_Layer[jj] < LAYER_NO_CHANGE )
                pt_drawsegm->SetLayer( New_Layer[jj] );
        }
    }

    DrawPanel->Refresh( TRUE );
}
