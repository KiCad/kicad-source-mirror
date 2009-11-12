///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 16 2008)
// http://www.wxformbuilder.org/
///////////////////////////////////////////////////////////////////////////

#include "fctsys.h"
#include "common.h"
#include "class_drawpanel.h"

#include "confirm.h"
#include "pcbnew.h"
#include "wxPcbStruct.h"
#include "class_board_design_settings.h"

#include "pcbnew_id.h"

#include "dialog_layers_setup.h"

///////////////////////////////////////////////////////////////////////////

//==============================================================================
//
//           CHANGE HERE IF YOU WANT TO CHANGE THE DIALOG'S APEARANCE
//
//==============================================================================

// Define as 1 to show the layers in Pcbnew's original order
#define ORIGINAL_KICAD_LAYER_ORDER  0

// Define as 1 to show colored rows backgrounds in the list panel
#define PANEL_BACKGROUND_COLORED    1

// Setting this to 1 gives insteresting (and odd) results, some may like
#define CONTROL_BACKGROUND_COLORED  0

// Works best when LIST_CONTROLS_FLAGS includes wxALL
#define HIGHLIGHT_BACKGROUND_OTHER_THAN_CHECBOXES 0

// Uncommenting wxSIMPLE_BORDER will create a visibled grid
#define LIST_PANELS_STYLE           (wxTAB_TRAVERSAL/*|wxSIMPLE_BORDER*/)

// Uncommenting wxALL will add a margin between rows, but waste some visual space
#define LIST_CONTROLS_FLAGS         (wxRIGHT|wxLEFT|wxALIGN_CENTER_VERTICAL/*|wxALL*/)

//==============================================================================
// IDs for the dialog controls

enum
{
    ID_LAYERNAMES   = ( wxID_HIGHEST + 1 ),
    ID_CHECKBOXES   = ( ID_LAYERNAMES + NB_LAYERS ),
    ID_LAYERTYPES   = ( ID_CHECKBOXES + NB_LAYERS ),
};

//==============================================================================
// We want our dialog to remember its previous screen position

wxPoint DialogLayerSetup::m_DialogLastPosition( -1, -1 );

//==============================================================================
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

//==============================================================================
// Categories for coloring the rows backgrounds (0 means default dialog color).

static const int LayerCategories[NB_LAYERS] =
{  3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1, 1, 1, 1, 2, 2, 2, 2, 5, 5, 5, 5, 4 };

//==============================================================================
// Colors for the rows backgrounds in layer list

static const wxColour LayerCategoriesColors[5] =
{
    wxColour( 236, 233, 236 ),  // 1
    wxColour( 255, 252, 235 ),  // 2
    wxColour( 236, 253, 216 ),  // 3
    wxColour( 255, 253, 216 ),  // 4
    wxColour( 236, 233, 255 )   // 5
};

//==============================================================================
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

//==============================================================================
// Names for the presets

static const wxString m_PresetsChoiceChoices[] =
{
    _("All Layers On"),
    _("Single Side"),
    _("Single Side, SMD on Back"),
    _("Two Layers, Parts on Front"),
    _("Two Layers, Parts on Both Faces"),
    _("Four Layers, Parts on Front"),
    _("Four Layers, Parts on Both Faces")
};

#define PRESETS_CHOICE_N_CHOICES ((int)(sizeof m_PresetsChoiceChoices/sizeof m_PresetsChoiceChoices[0]))

//==============================================================================
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

//==============================================================================
// Options to show in the copper layer choice widget

static const wxString m_LayerNumberChoiceChoices[] =
{
    wxT("1"),
    wxT("2"),
    wxT("4"),
    wxT("6"),
    wxT("8"),
    wxT("10"),
    wxT("12"),
    wxT("14"),
    wxT("16")
};

#define LAYER_NUMBER_CHOICE_N_CHOICES ((int)(sizeof m_LayerNumberChoiceChoices/sizeof m_LayerNumberChoiceChoices[0]))

//==============================================================================
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

//==============================================================================
// Names for the types of copper layers

static const wxString m_LayerTypeChoiceChoices[] =
{
    wxT("Signal"),
    wxT("Power"),
    wxT("Mixed"),
    wxT("Jumper")
};

#define LAYER_TYPE_CHOICE_N_CHOICES ( sizeof m_LayerTypeChoiceChoices / sizeof m_LayerTypeChoiceChoices[0] )

//==============================================================================
// Helper function, only needed if we are goint to color the rows

#if PANEL_BACKGROUND_COLORED

static wxColour GetRowColor( int Layer )
{
    if( ! IsValidLayerIndex( Layer )  || LayerCategories[Layer] == 0 )
        return wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE );
    else
        return LayerCategoriesColors[LayerCategories[Layer]-1];
}

#endif // PANEL_BACKGROUND_COLORED

//==============================================================================

static wxString GetCategoryName( int Layer )
{
    if( ! IsValidLayerIndex( Layer ))
        return wxEmptyString;

    int Category = LayerCategories[Layer];

    if( Category < 0 || Category >= (int)( sizeof LayerCategoriesNames / sizeof LayerCategoriesNames[0] ))
        return wxEmptyString;
    else
        return LayerCategoriesNames[Category];
}

//==============================================================================
// We will get the names for all the layers here, it will ease things if in the
// future the non-copper layers names can be changed.

wxString DialogLayerSetup::GetLayerName( int Layer )
{
    return m_Pcb->GetLayerName( Layer );
/*
    if( Layer < 0 || Layer >= NB_LAYERS )
        return wxEmptyString;
    else if( Layer < NB_COPPER_LAYERS )
        return m_Pcb->GetLayerName( Layer );
    else
        return LayerNames[Layer];
*/
}

//==============================================================================

void DialogLayerSetup::SetLayerName( int Layer, wxString Name )
{
    m_Pcb->SetLayerName( Layer, Name );
}

//==============================================================================

int DialogLayerSetup::GetLayerType( int Layer )
{
    return m_Pcb->GetLayerType( Layer );
}

//==============================================================================
void DialogLayerSetup::SetLayerType( int Layer, LAYER_T Type )
{
    m_Pcb->SetLayerType( Layer, Type );
}

//==============================================================================
// The layer mask for non-copper layers is obtained from the new
// EDA_BoardDesignSettings::m_EnabledLayers, but for compatibility, the mask
// for the copper-layers is obtained from g_DesignSettings::m_CopperLayerCount

// Hopefully in the future we may unify them, perhaps saving only the mask and
// at load time calculating the number, or dropping the number completely if
// possible.

int DialogLayerSetup::GetLayersMask()
{
    int Aux = m_Pcb->m_BoardSettings->GetCopperLayerCount();

    // Here we transform the number of copper layers in an index to the table
    // CopperMasks, and also do some consistency check.
    if( Aux <= 1 )
        Aux = 0;
    else if( Aux > 16 )
        Aux = 8;
    else
        Aux /= 2;

    return CopperMasks[Aux] | ( m_Pcb->GetEnabledLayers() & ALL_NO_CU_LAYERS );
}

//==============================================================================
// This function translates from the dialog's layer order to Kicad's layer order.

static int GetLayerNumber( int Row )
{
    return LayerOrder[Row];
}

//==============================================================================
DialogLayerSetup::DialogLayerSetup( WinEDA_PcbFrame* parent, const wxPoint& pos, wxWindowID id, const wxString& title, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
    m_Parent    = parent;
    m_Pcb       = m_Parent->GetBoard();

    if( pos == wxDefaultPosition )
        SetPosition( m_DialogLastPosition );

    this->SetSizeHints( wxDefaultSize, wxDefaultSize );

    wxBoxSizer* m_MainSizer;
    m_MainSizer = new wxBoxSizer( wxVERTICAL );

    m_MainPanel = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxRAISED_BORDER|wxTAB_TRAVERSAL );
    wxBoxSizer* m_MainPanelSizer;
    m_MainPanelSizer = new wxBoxSizer( wxVERTICAL );

    wxBoxSizer* m_ChoicesSizer;
    m_ChoicesSizer = new wxBoxSizer( wxHORIZONTAL );

    wxBoxSizer* m_PresetsSizer;
    m_PresetsSizer = new wxBoxSizer( wxVERTICAL );

    m_PresetsCaption = new wxStaticText( m_MainPanel, wxID_ANY, wxT("Presets"), wxDefaultPosition, wxDefaultSize, 0 );
    m_PresetsCaption->Wrap( -1 );
    m_PresetsSizer->Add( m_PresetsCaption, 0, wxLEFT|wxRIGHT|wxTOP, 5 );

    m_PresetsChoice = new wxChoice( m_MainPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, PRESETS_CHOICE_N_CHOICES, m_PresetsChoiceChoices, 0 );
    m_PresetsChoice->SetSelection( 0 );
    m_PresetsChoice->SetToolTip( wxT("Choose the type which better describes your board") );

    m_PresetsSizer->Add( m_PresetsChoice, 0, wxBOTTOM|wxRIGHT|wxLEFT|wxEXPAND, 5 );

    m_ChoicesSizer->Add( m_PresetsSizer, 1, wxEXPAND, 5 );

    wxBoxSizer* m_LayerNumberSizer;
    m_LayerNumberSizer = new wxBoxSizer( wxVERTICAL );

    m_LayerNumberCaption = new wxStaticText( m_MainPanel, wxID_ANY, wxT("Copper Layers"), wxDefaultPosition, wxDefaultSize, 0 );
    m_LayerNumberCaption->Wrap( -1 );
    m_LayerNumberSizer->Add( m_LayerNumberCaption, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

    m_LayerNumberChoice = new wxChoice( m_MainPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, LAYER_NUMBER_CHOICE_N_CHOICES, m_LayerNumberChoiceChoices, 0 );
    m_LayerNumberChoice->SetSelection( 8 );
    m_LayerNumberChoice->SetToolTip( wxT("Choose how many copper layers you need") );

    m_LayerNumberSizer->Add( m_LayerNumberChoice, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

    m_ChoicesSizer->Add( m_LayerNumberSizer, 0, wxEXPAND, 5 );

    m_MainPanelSizer->Add( m_ChoicesSizer, 0, wxEXPAND, 5 );

    m_Separator1 = new wxStaticLine( m_MainPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    m_MainPanelSizer->Add( m_Separator1, 0, wxEXPAND | wxALL, 5 );

    m_LayersCaptionText = new wxStaticText( m_MainPanel, wxID_ANY, wxT("Layers"), wxDefaultPosition, wxDefaultSize, 0 );
    m_LayersCaptionText->Wrap( -1 );
    m_MainPanelSizer->Add( m_LayersCaptionText, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

    m_LayersPanel = new wxPanel( m_MainPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER|wxTAB_TRAVERSAL );
    wxBoxSizer* m_LayersPanelSizer;
    m_LayersPanelSizer = new wxBoxSizer( wxVERTICAL );

    wxBoxSizer* m_LayerCaptionsSizer;
    m_LayerCaptionsSizer = new wxBoxSizer( wxHORIZONTAL );

    wxBoxSizer* m_LayerNameCaptionSizer;
    m_LayerNameCaptionSizer = new wxBoxSizer( wxVERTICAL );

    m_LayerNameCaptionText = new wxStaticText( m_LayersPanel, wxID_ANY, wxT("Name"), wxDefaultPosition, wxDefaultSize, 0 );
    m_LayerNameCaptionText->Wrap( -1 );
    m_LayerNameCaptionText->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 92, false, wxEmptyString ) );

    m_LayerNameCaptionSizer->Add( m_LayerNameCaptionText, 0, wxALL | wxALIGN_CENTER_HORIZONTAL, 5 );

    m_LayerCaptionsSizer->Add( m_LayerNameCaptionSizer, 0, wxEXPAND, 5 );

    wxBoxSizer* m_LayerEnabledCaptionSizer;
    m_LayerEnabledCaptionSizer = new wxBoxSizer( wxVERTICAL );

    m_LayerEnabledCaptionText = new wxStaticText( m_LayersPanel, wxID_ANY, wxT("Enabled"), wxDefaultPosition, wxDefaultSize, 0 );
    m_LayerEnabledCaptionText->Wrap( -1 );
    m_LayerEnabledCaptionText->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 92, false, wxEmptyString ) );

    m_LayerEnabledCaptionSizer->Add( m_LayerEnabledCaptionText, 0, wxALL | wxALIGN_CENTER_HORIZONTAL, 5 );

    m_LayerCaptionsSizer->Add( m_LayerEnabledCaptionSizer, 0, wxEXPAND, 5 );

    wxBoxSizer* m_LayerTypeCaptionSizer;
    m_LayerTypeCaptionSizer = new wxBoxSizer( wxVERTICAL );

    m_LayerTypeCaptionText = new wxStaticText( m_LayersPanel, wxID_ANY, wxT("Type"), wxDefaultPosition, wxDefaultSize, 0 );
    m_LayerTypeCaptionText->Wrap( -1 );
    m_LayerTypeCaptionText->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 92, false, wxEmptyString ) );

    m_LayerTypeCaptionSizer->Add( m_LayerTypeCaptionText, 0, wxALL | wxALIGN_CENTER_HORIZONTAL, 5 );

    m_LayerCaptionsSizer->Add( m_LayerTypeCaptionSizer, 0, wxEXPAND, 5 );

    m_LayersPanelSizer->Add( m_LayerCaptionsSizer, 0, wxEXPAND, 5 );

    m_LayerListScroller = new wxScrolledWindow( m_LayersPanel, wxID_ANY, wxDefaultPosition, wxSize( -1, 300 )/* wxDefaultSize */, wxALWAYS_SHOW_SB|wxVSCROLL );
    m_LayerListScroller->SetMinSize( wxSize( -1, 300 ) );

    wxBoxSizer* bSizer18;
    bSizer18 = new wxBoxSizer( wxHORIZONTAL );

    wxBoxSizer* m_LayerListSizer;
    m_LayerListSizer = new wxBoxSizer( wxHORIZONTAL );

    //--------------------------------------------------------------------------
    // Create the controls inside the scrolled window

    wxGridSizer* m_LayerNameListSizer;
    m_LayerNameListSizer = new wxGridSizer( 32, 1, 0, 0 );

    for( int Row = 0; Row < NB_LAYERS; Row++ )
    {
        int Layer = GetLayerNumber( Row );

        //----------------------------------------------------------------------
        // Create a panel and sizer for each layer name

        m_LayerNamePanel[Layer] = new wxPanel( m_LayerListScroller, wxID_ANY, wxDefaultPosition, wxDefaultSize, LIST_PANELS_STYLE );

#if PANEL_BACKGROUND_COLORED
        m_LayerNamePanel[Layer]->SetBackgroundColour( GetRowColor( Layer ));
#endif // PANEL_BACKGROUND_COLORED

        wxBoxSizer* m_LayerNameSizer = new wxBoxSizer( wxHORIZONTAL );

        if( Layer >= NB_COPPER_LAYERS )
        {
            //----------------------------------------------------------------------
            // The non-copper layer names canot be changed, we use just a static text

            m_LayerNameStaticText[Layer-NB_COPPER_LAYERS] = new wxStaticText( m_LayerNamePanel[Layer], ID_LAYERNAMES + Layer, GetLayerName( Layer ), wxDefaultPosition, wxDefaultSize, 0 );
            m_LayerNameStaticText[Layer-NB_COPPER_LAYERS]->Wrap( -1 );
            m_LayerNameStaticText[Layer-NB_COPPER_LAYERS]->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 90, false, wxEmptyString ) );

            m_LayerNameSizer->Add( m_LayerNameStaticText[Layer-NB_COPPER_LAYERS], 1, LIST_CONTROLS_FLAGS, 5 );

            m_LayerNamePanel[Layer]->SetSizer( m_LayerNameSizer );
            m_LayerNamePanel[Layer]->Layout();
            m_LayerNameSizer->Fit( m_LayerNamePanel[Layer] );
            m_LayerNameListSizer->Add( m_LayerNamePanel[Layer], 0, wxEXPAND, 5 );

        }
        else
        {
            //----------------------------------------------------------------------
            // The copper layer names can be changed, we need a text control

            m_LayerNameTextCtrl[Layer] = new wxTextCtrl( m_LayerNamePanel[Layer], ID_LAYERNAMES + Layer, GetLayerName( Layer ), wxDefaultPosition, wxDefaultSize, 0 /*|wxNO_BORDER*/ );
            m_LayerNameTextCtrl[Layer]->SetMaxLength( 20 );

#if CONTROL_BACKGROUND_COLORED
            m_LayerNameTextCtrl[Layer]->SetBackgroundColour( GetRowColor( Layer ));
#endif // CONTROL_BACKGROUND_COLORED

            m_LayerNameTextCtrl[Layer]->SetToolTip( wxT("Edit Copper Layer Name") );

            m_LayerNameSizer->Add( m_LayerNameTextCtrl[Layer], 1, LIST_CONTROLS_FLAGS, 5 );

            m_LayerNamePanel[Layer]->SetSizer( m_LayerNameSizer );
            m_LayerNamePanel[Layer]->Layout();
            m_LayerNameSizer->Fit( m_LayerNamePanel[Layer] );
            m_LayerNameListSizer->Add( m_LayerNamePanel[Layer], 0, wxEXPAND, 5 );
        }
    }

    m_LayerListSizer->Add( m_LayerNameListSizer, 0, wxEXPAND, 5 );

    //--------------------------------------------------------------------------
    // Create the controls inside the scrolled window

    wxGridSizer* m_LayerEnabledListSizer;
    m_LayerEnabledListSizer = new wxGridSizer( 32, 1, 0, 0 );

    for( int Row = 0; Row < NB_LAYERS; Row++ )
    {
        int Layer = GetLayerNumber( Row );

        //----------------------------------------------------------------------
        // Create a panel and sizer for each layer enable check-box

        m_LayerEnabledPanel[Layer] = new wxPanel( m_LayerListScroller, wxID_ANY, wxDefaultPosition, wxDefaultSize, LIST_PANELS_STYLE );

#if PANEL_BACKGROUND_COLORED
        m_LayerEnabledPanel[Layer]->SetBackgroundColour( GetRowColor( Layer ));
#endif // PANEL_BACKGROUND_COLORED

        wxBoxSizer* m_LayerEnabledSizer;
        m_LayerEnabledSizer = new wxBoxSizer( wxVERTICAL );

        wxBoxSizer* m_LayerEnabledSizerInt;
        m_LayerEnabledSizerInt = new wxBoxSizer( wxHORIZONTAL );

        m_LayerEnabledCheckBox[Layer] = new wxCheckBox( m_LayerEnabledPanel[Layer], ID_CHECKBOXES + Layer, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );

        m_LayerEnabledCheckBox[Layer]->SetForegroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHT ) );
        m_LayerEnabledCheckBox[Layer]->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_3DDKSHADOW ) );
        m_LayerEnabledCheckBox[Layer]->SetToolTip( wxT("Enable/Disable Layer") );

        m_LayerEnabledSizerInt->Add( m_LayerEnabledCheckBox[Layer], 0, LIST_CONTROLS_FLAGS, 5 );
        m_LayerEnabledSizer->Add( m_LayerEnabledSizerInt, 1, wxALIGN_CENTER_HORIZONTAL, 5 );

        m_LayerEnabledPanel[Layer]->SetSizer( m_LayerEnabledSizer );
        m_LayerEnabledPanel[Layer]->Layout();
        m_LayerEnabledSizer->Fit( m_LayerEnabledPanel[Layer] );
        m_LayerEnabledListSizer->Add( m_LayerEnabledPanel[Layer], 1, wxEXPAND|wxALIGN_CENTER_HORIZONTAL, 5 );
    }

    m_LayerListSizer->Add( m_LayerEnabledListSizer, 0, wxEXPAND, 5 );

    //--------------------------------------------------------------------------
    // Create the controls inside the scrolled window

    wxGridSizer* m_LayerTypeListSizer;
    m_LayerTypeListSizer = new wxGridSizer( 32, 1, 0, 0 );

    for( int Row = 0; Row < NB_LAYERS; Row++ )
    {
        int Layer = GetLayerNumber( Row );

        //----------------------------------------------------------------------
        // Create a panel and sizer for each layer enable chack-box

        m_LayerTypePanel[Layer] = new wxPanel( m_LayerListScroller, wxID_ANY, wxDefaultPosition, wxDefaultSize, LIST_PANELS_STYLE );

#if PANEL_BACKGROUND_COLORED
        m_LayerTypePanel[Layer]->SetBackgroundColour( GetRowColor( Layer ));
#endif // PANEL_BACKGROUND_COLORED

        wxBoxSizer* m_LayerTypeSizer;
        m_LayerTypeSizer = new wxBoxSizer( wxHORIZONTAL );

        if( Layer >= NB_COPPER_LAYERS )
        {
            //----------------------------------------------------------------------
            // The non-copper layer types canot be changed, we use just a static text

            m_LayerTypeStaticText[Layer-NB_COPPER_LAYERS] = new wxStaticText( m_LayerTypePanel[Layer], ID_LAYERTYPES + Layer, GetCategoryName( Layer ), wxDefaultPosition, wxDefaultSize, 0 );
            m_LayerTypeStaticText[Layer-NB_COPPER_LAYERS]->Wrap( -1 );
            m_LayerTypeStaticText[Layer-NB_COPPER_LAYERS]->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 90, false, wxEmptyString ) );

            m_LayerTypeSizer->Add( m_LayerTypeStaticText[Layer-NB_COPPER_LAYERS], 1, LIST_CONTROLS_FLAGS, 5 );

            m_LayerTypePanel[Layer]->SetSizer( m_LayerTypeSizer );
            m_LayerTypePanel[Layer]->Layout();
            m_LayerTypeSizer->Fit( m_LayerTypePanel[Layer] );
            m_LayerTypeListSizer->Add( m_LayerTypePanel[Layer], 0, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );
        }
        else
        {
            //----------------------------------------------------------------------
            // The copper layer names can be changed, we need a choice control

            m_LayerTypeChoice[Layer] = new wxChoice( m_LayerTypePanel[Layer], ID_LAYERTYPES + Layer, wxDefaultPosition, wxDefaultSize, LAYER_TYPE_CHOICE_N_CHOICES, m_LayerTypeChoiceChoices, 0 );
            m_LayerTypeChoice[Layer]->SetSelection( GetLayerType( Layer ));
            m_LayerTypeChoice[Layer]->SetToolTip( wxT("Choose Copper Layer Type") );

#if CONTROL_BACKGROUND_COLORED
            m_LayerTypeChoice[Layer]->SetBackgroundColour( GetRowColor( Layer ));
#endif // CONTROL_BACKGROUND_COLORED

            m_LayerTypeSizer->Add( m_LayerTypeChoice[Layer], 1, LIST_CONTROLS_FLAGS, 5 );

            m_LayerTypePanel[Layer]->SetSizer( m_LayerTypeSizer );
            m_LayerTypePanel[Layer]->Layout();
            m_LayerTypeSizer->Fit( m_LayerTypePanel[Layer] );
            m_LayerTypeListSizer->Add( m_LayerTypePanel[Layer], 1, wxEXPAND|wxALIGN_CENTER_VERTICAL, 5 );
        }
    }

    m_LayerListSizer->Add( m_LayerTypeListSizer, 0, wxEXPAND, 5 );

    //--------------------------------------------------------------------------
    // Adjust the width of the columns and column captions
    int w;

    w   = max( m_LayerNameListSizer->CalcMin().x, m_LayerNameCaptionSizer->CalcMin().x );
    m_LayerNameListSizer->SetMinSize( w, -1 );
    m_LayerNameCaptionSizer->SetMinSize( w, -1 );

    w   = max( m_LayerEnabledListSizer->CalcMin().x, m_LayerEnabledCaptionSizer->CalcMin().x );
    m_LayerEnabledListSizer->SetMinSize( w, -1 );
    m_LayerEnabledCaptionSizer->SetMinSize( w, -1 );

    w   = max( m_LayerTypeListSizer->CalcMin().x, m_LayerTypeCaptionSizer->CalcMin().x );
    m_LayerTypeListSizer->SetMinSize( w, -1 );
    m_LayerTypeCaptionSizer->SetMinSize( w, -1 );


    //--------------------------------------------------------------------------
    // Make the scroll step be exactly one row

    wxSize s = m_LayerNamePanel[0]->GetSize();
    m_LayerListScroller->SetScrollRate( 0, s.y );

    //--------------------------------------------------------------------------

    bSizer18->Add( m_LayerListSizer, 1, 0, 5 );

    m_LayerListScroller->SetSizer( bSizer18 );

    m_LayerListScroller->Layout();
    bSizer18->Fit( m_LayerListScroller );
    m_LayersPanelSizer->Add( m_LayerListScroller, 1, wxEXPAND, 5 );

    m_LayersPanel->SetSizer( m_LayersPanelSizer );
    m_LayersPanel->Layout();
    m_LayersPanelSizer->Fit( m_LayersPanel );
    m_MainPanelSizer->Add( m_LayersPanel, 1, /*wxEXPAND|*/ wxBOTTOM|wxRIGHT|wxLEFT, 5 );

    m_Separator2 = new wxStaticLine( m_MainPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    m_MainPanelSizer->Add( m_Separator2, 0, wxEXPAND | wxALL, 5 );

    m_StdButtonsSizer = new wxStdDialogButtonSizer();
    m_StdButtonsSizerOK = new wxButton( m_MainPanel, wxID_OK );
    m_StdButtonsSizer->AddButton( m_StdButtonsSizerOK );
    m_StdButtonsSizerCancel = new wxButton( m_MainPanel, wxID_CANCEL );
    m_StdButtonsSizer->AddButton( m_StdButtonsSizerCancel );
    m_StdButtonsSizer->Realize();
    m_MainPanelSizer->Add( m_StdButtonsSizer, 0, wxALL|wxEXPAND, 5 );

    m_MainPanel->SetSizer( m_MainPanelSizer );
    m_MainPanel->Layout();
    m_MainPanelSizer->Fit( m_MainPanel );
    m_MainSizer->Add( m_MainPanel, 1, wxEXPAND | wxALL, 5 );

    this->SetSizer( m_MainSizer );
    this->Layout();
    m_MainSizer->Fit( this );




    m_LayersMask    = GetLayersMask();

    UpdatePresetsChoice();
    UpdateCopperLayersChoice();
    UpdateCheckBoxes();

    // Connect Events
    m_StdButtonsSizerCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DialogLayerSetup::OnCancelClick ), NULL, this );
    m_StdButtonsSizerOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DialogLayerSetup::OnOKClick ), NULL, this );

    m_PresetsChoice->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DialogLayerSetup::OnPresetChoice ), NULL, this );
    m_LayerNumberChoice->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DialogLayerSetup::OnCopperLayersChoice ), NULL, this );

    for( int i = 0; i < NB_COPPER_LAYERS; i++ )
    {
        m_LayerTypeChoice[i]->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DialogLayerSetup::OnLayerTypeChoice ), NULL, this );
#if HIGHLIGHT_BACKGROUND_OTHER_THAN_CHECBOXES
        m_LayerTypeChoice[i]->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( DialogLayerSetup::OnLayerTypeKillFocus ), NULL, this );
        m_LayerTypeChoice[i]->Connect( wxEVT_SET_FOCUS, wxFocusEventHandler( DialogLayerSetup::OnLayerTypeSetFocus ), NULL, this );
        m_LayerNameTextCtrl[i]->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( DialogLayerSetup::OnLayerNameKillFocus ), NULL, this );
        m_LayerNameTextCtrl[i]->Connect( wxEVT_SET_FOCUS, wxFocusEventHandler( DialogLayerSetup::OnLayerNameSetFocus ), NULL, this );
#endif // HIGHLIGHT_BACKGROUND_OTHER_THAN_CHECBOXES
    }

    for( int i = 0; i < NB_LAYERS; i++ )
    {
        m_LayerEnabledCheckBox[i]->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DialogLayerSetup::OnLayerEnabledCheckBox ), NULL, this );
        m_LayerEnabledCheckBox[i]->Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( DialogLayerSetup::OnLayerEnabledKillFocus ), NULL, this );
        m_LayerEnabledCheckBox[i]->Connect( wxEVT_SET_FOCUS, wxFocusEventHandler( DialogLayerSetup::OnLayerEnabledSetFocus ), NULL, this );
    }

    Centre();
}

//==============================================================================

DialogLayerSetup::~DialogLayerSetup()
{
    // Disconnect Events
    m_StdButtonsSizerCancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DialogLayerSetup::OnCancelClick ), NULL, this );
    m_StdButtonsSizerOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DialogLayerSetup::OnOKClick ), NULL, this );

    m_PresetsChoice->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DialogLayerSetup::OnPresetChoice ), NULL, this );
    m_LayerNumberChoice->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DialogLayerSetup::OnCopperLayersChoice ), NULL, this );

    for( int i = 0; i < NB_COPPER_LAYERS; i++ )
    {
        m_LayerTypeChoice[i]->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DialogLayerSetup::OnLayerTypeChoice ), NULL, this );
#if HIGHLIGHT_BACKGROUND_OTHER_THAN_CHECBOXES
        m_LayerTypeChoice[i]->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( DialogLayerSetup::OnLayerTypeKillFocus ), NULL, this );
        m_LayerTypeChoice[i]->Disconnect( wxEVT_SET_FOCUS, wxFocusEventHandler( DialogLayerSetup::OnLayerTypeSetFocus ), NULL, this );
        m_LayerNameTextCtrl[i]->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( DialogLayerSetup::OnLayerNameKillFocus ), NULL, this );
        m_LayerNameTextCtrl[i]->Disconnect( wxEVT_SET_FOCUS, wxFocusEventHandler( DialogLayerSetup::OnLayerNameSetFocus ), NULL, this );
#endif // HIGHLIGHT_BACKGROUND_OTHER_THAN_CHECBOXES
    }

    for( int i = 0; i < NB_LAYERS; i++ )
    {
        m_LayerEnabledCheckBox[i]->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( DialogLayerSetup::OnLayerEnabledCheckBox ), NULL, this );
        m_LayerEnabledCheckBox[i]->Disconnect( wxEVT_KILL_FOCUS, wxFocusEventHandler( DialogLayerSetup::OnLayerEnabledKillFocus ), NULL, this );
        m_LayerEnabledCheckBox[i]->Disconnect( wxEVT_SET_FOCUS, wxFocusEventHandler( DialogLayerSetup::OnLayerEnabledSetFocus ), NULL, this );
    }
}

//==============================================================================

void DialogLayerSetup::UpdateCheckBoxes()
{
    int i;

    for( i = 0; i < NB_COPPER_LAYERS; i++ )
    {
        m_LayerEnabledCheckBox[i]->Disable();

        if( m_LayersMask >> i & 0x00000001 )
        {
            m_LayerEnabledCheckBox[i]->SetValue( true );
            m_LayerNameTextCtrl[i]->Enable();
            m_LayerTypeChoice[i]->Enable();
        }
        else
        {
            m_LayerEnabledCheckBox[i]->SetValue( false );
            m_LayerNameTextCtrl[i]->Disable();
            m_LayerTypeChoice[i]->Disable();
        }
        m_LayerEnabledCheckBox[i]->Refresh();
    }

    for( ; i < NB_LAYERS; i++ )
    {
        m_LayerEnabledCheckBox[i]->SetValue( m_LayersMask >> i & 0x00000001 );
        m_LayerEnabledCheckBox[i]->Refresh();
    }

    if( m_LayersMask & CUIVRE_LAYER )
    {
        m_LayerNameStaticText[SOLDERPASTE_N_CU-NB_COPPER_LAYERS]->Enable();
        m_LayerEnabledCheckBox[SOLDERPASTE_N_CU]->Enable();
        m_LayerTypeStaticText[SOLDERPASTE_N_CU-NB_COPPER_LAYERS]->Enable();

        m_LayerNameStaticText[SOLDERMASK_N_CU-NB_COPPER_LAYERS]->Enable();
        m_LayerEnabledCheckBox[SOLDERMASK_N_CU]->Enable();
        m_LayerTypeStaticText[SOLDERMASK_N_CU-NB_COPPER_LAYERS]->Enable();
    }
    else
    {
        m_LayerEnabledCheckBox[SOLDERPASTE_N_CU]->SetValue( false );
        m_LayerEnabledCheckBox[SOLDERMASK_N_CU]->SetValue( false );

        m_LayerNameStaticText[SOLDERPASTE_N_CU-NB_COPPER_LAYERS]->Disable();
        m_LayerEnabledCheckBox[SOLDERPASTE_N_CU]->Disable();
        m_LayerTypeStaticText[SOLDERPASTE_N_CU-NB_COPPER_LAYERS]->Disable();

        m_LayerNameStaticText[SOLDERMASK_N_CU-NB_COPPER_LAYERS]->Disable();
        m_LayerEnabledCheckBox[SOLDERMASK_N_CU]->Disable();
        m_LayerTypeStaticText[SOLDERMASK_N_CU-NB_COPPER_LAYERS]->Disable();
    }

    if( m_LayersMask & CMP_LAYER )
    {
        m_LayerNameStaticText[SOLDERPASTE_N_CMP-NB_COPPER_LAYERS]->Enable();
        m_LayerEnabledCheckBox[SOLDERPASTE_N_CMP]->Enable();
        m_LayerTypeStaticText[SOLDERPASTE_N_CMP-NB_COPPER_LAYERS]->Enable();

        m_LayerNameStaticText[SOLDERMASK_N_CMP-NB_COPPER_LAYERS]->Enable();
        m_LayerEnabledCheckBox[SOLDERMASK_N_CMP]->Enable();
        m_LayerTypeStaticText[SOLDERMASK_N_CMP-NB_COPPER_LAYERS]->Enable();
    }
    else
    {
        m_LayerEnabledCheckBox[SOLDERPASTE_N_CMP]->SetValue( false );
        m_LayerEnabledCheckBox[SOLDERMASK_N_CMP]->SetValue( false );

        m_LayerNameStaticText[SOLDERPASTE_N_CMP-NB_COPPER_LAYERS]->Disable();
        m_LayerEnabledCheckBox[SOLDERPASTE_N_CMP]->Disable();
        m_LayerTypeStaticText[SOLDERPASTE_N_CMP-NB_COPPER_LAYERS]->Disable();

        m_LayerNameStaticText[SOLDERMASK_N_CMP-NB_COPPER_LAYERS]->Disable();
        m_LayerEnabledCheckBox[SOLDERMASK_N_CMP]->Disable();
        m_LayerTypeStaticText[SOLDERMASK_N_CMP-NB_COPPER_LAYERS]->Disable();
    }
}

//==============================================================================

void DialogLayerSetup::UpdateCopperLayersChoice()
{
    int NumberOfCopperLayers = 0;

    // Count the copper layers
    for( int i = 0; i < NB_COPPER_LAYERS; i++ )
    {
        if( m_LayersMask >> i & 0x00000001 )
            NumberOfCopperLayers++;
    }

    m_LayerNumberChoice->SetSelection( NumberOfCopperLayers / 2 );
}

//==============================================================================

void DialogLayerSetup::UpdatePresetsChoice()
{
    int Selection = -1;

    for( int i = 0; i < NB_LAYERS; i++ )
    {
        if( m_LayersMask == Presets[i] )
        {
            Selection = i;
            break;
        }
    }

    m_PresetsChoice->SetSelection( Selection );
    m_PresetsChoice->Refresh();
}

//==============================================================================

void DialogLayerSetup::OnPresetChoice( wxCommandEvent& event )
{
    int Selection = m_PresetsChoice->GetSelection();

    if( Selection < 0 || Selection >= PRESETS_CHOICE_N_CHOICES )
        return;

    m_LayersMask = Presets[Selection];

    UpdateCopperLayersChoice();
    UpdateCheckBoxes();

//    event.Skip();
}

//==============================================================================

void DialogLayerSetup::OnCopperLayersChoice( wxCommandEvent& event )
{
    int Selection = m_LayerNumberChoice->GetSelection();

    if( Selection < 0 || Selection >= LAYER_NUMBER_CHOICE_N_CHOICES )
        return;

    m_LayersMask = ( m_LayersMask & ALL_NO_CU_LAYERS ) | CopperMasks[Selection];

    UpdatePresetsChoice();
    UpdateCheckBoxes();

//    event.Skip();
}

//==============================================================================

void DialogLayerSetup::OnLayerEnabledCheckBox( wxCommandEvent& event )
{
    int Id  = event.GetId();

    if( Id >= ID_CHECKBOXES && Id < ID_CHECKBOXES + NB_LAYERS )
    {
        int Index   = Id - ID_CHECKBOXES;
        if( m_LayerEnabledCheckBox[Index]->GetValue() )
            m_LayersMask |= 1 << Index;
        else
            m_LayersMask &= ~( 1 << Index );

        UpdatePresetsChoice();
        UpdateCheckBoxes();
    }

//    event.Skip();
}

//==============================================================================
// Checkboxes without captions don´t show when they have focus, we will change the color
// of the panel where they reside to allow navigating the dialog by keyboard

void DialogLayerSetup::OnLayerEnabledSetFocus( wxFocusEvent& event )
{
    int Id  = event.GetId();

    if( Id >= ID_CHECKBOXES && Id < ID_CHECKBOXES + NB_LAYERS )
    {
        int Index   = Id - ID_CHECKBOXES;
        m_LayerEnabledPanel[Index]->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHT ) );
        m_LayerEnabledPanel[Index]->Refresh();
    }

//    event.Skip();
}
//==============================================================================
void DialogLayerSetup::OnLayerEnabledKillFocus( wxFocusEvent& event )
{
    int Id  = event.GetId();

    if( Id >= ID_CHECKBOXES && Id < ID_CHECKBOXES + NB_LAYERS )
    {
        int Index   = Id - ID_CHECKBOXES;
#if PANEL_BACKGROUND_COLORED
        m_LayerEnabledPanel[Index]->SetBackgroundColour( GetRowColor( Index ));
#else // PANEL_BACKGROUND_COLORED
        m_LayerEnabledPanel[Index]->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE ));
#endif // PANEL_BACKGROUND_COLORED
        m_LayerEnabledPanel[Index]->Refresh();
    }

//    event.Skip();
}
//==============================================================================
void DialogLayerSetup::OnLayerNameSetFocus( wxFocusEvent& event )
{
    int Id  = event.GetId();

    if( Id >= ID_LAYERNAMES && Id < ID_LAYERNAMES + NB_COPPER_LAYERS )
    {
        int Index   = Id - ID_LAYERNAMES;
#if CONTROL_BACKGROUND_COLORED
        m_LayerNameTextCtrl[Index]->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );
#endif // CONTROL_BACKGROUND_COLORED
        m_LayerNamePanel[Index]->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHT ) );
        m_LayerNamePanel[Index]->Refresh();
    }

//    event.Skip();
}
//==============================================================================
void DialogLayerSetup::OnLayerNameKillFocus( wxFocusEvent& event )
{
    int Id  = event.GetId();

    if( Id >= ID_LAYERNAMES && Id < ID_LAYERNAMES + NB_COPPER_LAYERS )
    {
        int Index   = Id - ID_LAYERNAMES;
#if CONTROL_BACKGROUND_COLORED
        m_LayerNameTextCtrl[Index]->SetBackgroundColour( GetRowColor( Index ));
#endif // CONTROL_BACKGROUND_COLORED
#if PANEL_BACKGROUND_COLORED
        m_LayerNamePanel[Index]->SetBackgroundColour( GetRowColor( Index ));
#else // PANEL_BACKGROUND_COLORED
        m_LayerNamePanel[Index]->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE ));
#endif // PANEL_BACKGROUND_COLORED
        m_LayerNamePanel[Index]->Refresh();
    }

//    event.Skip();
}
//==============================================================================
void DialogLayerSetup::OnLayerTypeSetFocus( wxFocusEvent& event )
{
    int Id  = event.GetId();

    if( Id >= ID_LAYERTYPES && Id < ID_LAYERTYPES + NB_COPPER_LAYERS )
    {
        int Index   = Id - ID_LAYERTYPES;
#if CONTROL_BACKGROUND_COLORED
        m_LayerTypeChoice[Index]->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );
#endif // CONTROL_BACKGROUND_COLORED
        m_LayerTypePanel[Index]->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHT ) );
        m_LayerTypePanel[Index]->Refresh();
    }

//    event.Skip();
}
//==============================================================================
void DialogLayerSetup::OnLayerTypeKillFocus( wxFocusEvent& event )
{
    int Id  = event.GetId();

    if( Id >= ID_LAYERTYPES && Id < ID_LAYERTYPES + NB_COPPER_LAYERS )
    {
        int Index   = Id - ID_LAYERTYPES;
#if CONTROL_BACKGROUND_COLORED
        m_LayerTypeChoice[Index]->SetBackgroundColour( GetRowColor( Index ));
#endif // CONTROL_BACKGROUND_COLORED
#if PANEL_BACKGROUND_COLORED
        m_LayerTypePanel[Index]->SetBackgroundColour( GetRowColor( Index ));
#else // PANEL_BACKGROUND_COLORED
        m_LayerTypePanel[Index]->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE ));
#endif // PANEL_BACKGROUND_COLORED
        m_LayerTypePanel[Index]->Refresh();
    }

//    event.Skip();
}

//==============================================================================

void DialogLayerSetup::OnCancelClick( wxCommandEvent& event )
{
    // Save the dialog's position before finishing
    m_DialogLastPosition = this->GetPosition();
    event.Skip();
}

//==============================================================================

void DialogLayerSetup::OnOKClick( wxCommandEvent& event )
{
    int NumberOfCopperLayers = 0;

    // Count the copper layers
    for( int i = 0; i < NB_COPPER_LAYERS; i++ )
    {
        if( m_LayersMask >> i & 0x00000001 )
            NumberOfCopperLayers++;
    }

    m_Pcb->m_BoardSettings->SetCopperLayerCount( NumberOfCopperLayers );

    m_Pcb->SetEnabledLayers( m_LayersMask );
    /* Reset the layers visibility flag
     *  Because it could creates SERIOUS mistakes for the user,
     * if some layers are not visible after activating them ...
     */
    m_Pcb->SetVisibleLayers( m_LayersMask );

    for( int i = 0; i < NB_COPPER_LAYERS; i++ )
    {
        SetLayerName( i, m_LayerNameTextCtrl[i]->GetValue() );
        SetLayerType( i, (LAYER_T)m_LayerTypeChoice[i]->GetSelection() );
    }


    m_Parent->ReCreateLayerBox( NULL );

    // Save the dialog's position before finishing
    m_DialogLastPosition = this->GetPosition();

    event.Skip();
}

//==============================================================================
// Invoke the dialog.

void DisplayDialogLayerSetup( WinEDA_PcbFrame* parent )
{
    DialogLayerSetup* frame = new DialogLayerSetup( parent );
    frame->ShowModal();
    frame->Destroy();
}

//==============================================================================
