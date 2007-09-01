/************************************************/
/* Module editor: Dialog box for editing module	*/
/*  properties and carateristics				*/
/* include in modedit.cpp						*/
/************************************************/

#include "dialog_edit_module.h"

/**************************************/
/* class WinEDA_ModulePropertiesFrame */
/**************************************/

BEGIN_EVENT_TABLE( WinEDA_ModulePropertiesFrame, wxDialog )
    EVT_BUTTON( ID_ACCEPT_MODULE_PROPERTIES,
                WinEDA_ModulePropertiesFrame::ModulePropertiesAccept )
    EVT_BUTTON( ID_CLOSE_MODULE_PROPERTIES, WinEDA_ModulePropertiesFrame::OnQuit )
    EVT_BUTTON( ID_MODULE_EDIT_ADD_TEXT, WinEDA_ModulePropertiesFrame::CreateTextModule )
    EVT_BUTTON( ID_MODULE_EDIT_EDIT_TEXT, WinEDA_ModulePropertiesFrame::EditOrDelTextModule )
    EVT_BUTTON( ID_MODULE_EDIT_DELETE_TEXT, WinEDA_ModulePropertiesFrame::EditOrDelTextModule )
    EVT_BUTTON( ID_MODULE_PROPERTIES_EXCHANGE, WinEDA_ModulePropertiesFrame::ExchangeModule )
    EVT_KICAD_CHOICEBOX( ID_MODULE_LISTBOX_SELECT, WinEDA_ModulePropertiesFrame::SelectTextListBox )
    EVT_RADIOBOX( ID_LISTBOX_ORIENT_SELECT, WinEDA_ModulePropertiesFrame::ModuleOrientEvent )
    EVT_BUTTON( ID_GOTO_MODULE_EDITOR, WinEDA_ModulePropertiesFrame::GotoModuleEditor )
END_EVENT_TABLE()

/**********************/
/* class Panel3D_Ctrl */
/**********************/
BEGIN_EVENT_TABLE( Panel3D_Ctrl, wxPanel )
    EVT_BUTTON( ID_BROWSE_3D_LIB, Panel3D_Ctrl::Browse3DLib )
    EVT_BUTTON( ID_ADD_3D_SHAPE, Panel3D_Ctrl::AddOrRemove3DShape )
    EVT_BUTTON( ID_REMOVE_3D_SHAPE, Panel3D_Ctrl::AddOrRemove3DShape )
END_EVENT_TABLE()


/**************************************************************************************/
WinEDA_ModulePropertiesFrame::WinEDA_ModulePropertiesFrame( WinEDA_BasePcbFrame* parent,
                                                            MODULE* Module, wxDC* DC,
                                                            const wxPoint& framepos ) :
    wxDialog( parent, -1, _( "Module properties" ), framepos, wxDefaultSize, DIALOG_STYLE )
/**************************************************************************************/
{
    wxString number;

    SetIcon( wxICON( icon_modedit ) );        // Give an icon

    m_Parent = parent;
    SetFont( *g_DialogFont );
    m_DC = DC;

    m_LayerCtrl   = NULL;
    m_OrientCtrl  = NULL;
    m_OrientValue = NULL;
    m_Doc = m_Keyword = NULL;

    m_CurrentModule     = Module;
    m_DeleteFieddButton = NULL;

    if( m_CurrentModule )
    {
    }

    CreateControls();

    GetSizer()->Fit( this );
    GetSizer()->SetSizeHints( this );
    Centre();
}


/*****************************************************/
void WinEDA_ModulePropertiesFrame::CreateControls()
/*****************************************************/
{
    wxPoint   pos;
    wxButton* Button;
    bool      FullOptions = FALSE;

    if( m_Parent->m_Ident == PCB_FRAME )
        FullOptions = TRUE;

    m_GeneralBoxSizer = new wxBoxSizer( wxVERTICAL );
    SetSizer( m_GeneralBoxSizer );

    m_NoteBook = new wxNotebook( this, ID_NOTEBOOK );
    m_NoteBook->SetFont( *g_DialogFont );
    m_GeneralBoxSizer->Add( m_NoteBook, 0, wxGROW | wxALL, 5 );

    // Add panels
    m_PanelProperties = new wxPanel( m_NoteBook, -1 );
    m_PanelProperties->SetFont( *g_DialogFont );
    m_PanelPropertiesBoxSizer = new wxBoxSizer( wxHORIZONTAL );
    m_PanelProperties->SetSizer( m_PanelPropertiesBoxSizer );
    BuildPanelModuleProperties( FullOptions );
    m_NoteBook->AddPage( m_PanelProperties, _( "Properties" ), TRUE );

    m_Panel3D = new Panel3D_Ctrl( this, m_NoteBook, -1,
                                  m_CurrentModule->m_3D_Drawings );
    m_NoteBook->AddPage( m_Panel3D, _( "3D settings" ), FALSE );

    /* creation des autres formes 3D */
    Panel3D_Ctrl*    panel3D = m_Panel3D, * nextpanel3D;
    Struct3D_Master* draw3D  = m_CurrentModule->m_3D_Drawings;
    draw3D = (Struct3D_Master*) draw3D->Pnext;
    for( ; draw3D != NULL; draw3D = (Struct3D_Master*) draw3D->Pnext )
    {
        nextpanel3D = new Panel3D_Ctrl( this, m_NoteBook, -1, draw3D );
        m_NoteBook->AddPage( nextpanel3D, _( "3D settings" ), FALSE );
        panel3D->m_Pnext     = nextpanel3D;
        nextpanel3D->m_Pback = panel3D;
        panel3D = nextpanel3D;
    }

    /* Creation des boutons de commande */
    wxBoxSizer*      ButtonsBoxSizer = new wxBoxSizer( wxHORIZONTAL );
    m_GeneralBoxSizer->Add( ButtonsBoxSizer, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 5 );

    Button = new wxButton( this, ID_ACCEPT_MODULE_PROPERTIES,
                          _( "Ok" ) );
    Button->SetForegroundColour( *wxRED );
    ButtonsBoxSizer->Add( Button, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5 );

    Button = new wxButton( this, ID_CLOSE_MODULE_PROPERTIES,
                          _( "Cancel" ) );
    Button->SetForegroundColour( *wxBLUE );
    ButtonsBoxSizer->Add( Button, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5 );
}


/***********************************************************/
void Panel3D_Ctrl::AddOrRemove3DShape( wxCommandEvent& event )
/***********************************************************/
{
    if( event.GetId() == ID_ADD_3D_SHAPE )
    {
        Panel3D_Ctrl* panel3D = new Panel3D_Ctrl( m_ParentFrame, m_Parent,
                                                  -1, NULL );
        m_Parent->InsertPage( m_Parent->GetSelection() + 1,
                              panel3D, _( "3D settings" ), TRUE );
        panel3D->m_Pback = this;
        panel3D->m_Pnext = m_Pnext;
        if( m_Pnext )
            m_Pnext->m_Pback = panel3D;
        m_Pnext = panel3D;
    }

    if( event.GetId() == ID_REMOVE_3D_SHAPE )
    {
        if( m_Pback )
        {
            m_Pback->m_Pnext = m_Pnext;
            if( m_Pnext )
                m_Pnext->m_Pback = m_Pback;
            m_Parent->DeletePage( m_Parent->GetSelection() );
            m_ParentFrame->Refresh( TRUE );
        }
    }
}


/***************************************************************************/
void WinEDA_ModulePropertiesFrame::BuildPanelModuleProperties( bool FullOptions )
/***************************************************************************/

/* creation du panel d'edition des proprietes du module
 */
{
    wxButton*     Button;
    wxStaticText* StaticText;
    wxBoxSizer*   PropLeftSizer;
    wxBoxSizer*   PropRightSizer;
    wxString      msg;

    /* Create a sizer for controls in the left column */
    PropLeftSizer = new wxBoxSizer( wxVERTICAL );
    m_PanelPropertiesBoxSizer->Add( PropLeftSizer, 0, wxGROW | wxALL, 5 );
    /* Create a sizer for controls in the right column */
    PropRightSizer = new wxBoxSizer( wxVERTICAL );
    m_PanelPropertiesBoxSizer->Add( PropRightSizer, 0, wxGROW | wxALL, 5 );

    if( FullOptions )  // Module is on a board
    {
        Button = new wxButton( m_PanelProperties, ID_MODULE_PROPERTIES_EXCHANGE,
                              _( "Change module(s)" ) );
        Button->SetForegroundColour( wxColor( 80, 40, 0 ) );
        PropRightSizer->Add( Button, 0, wxGROW | wxALL, 5 );
        Button = new wxButton( m_PanelProperties, ID_GOTO_MODULE_EDITOR,
                              _( "Goto Module Editor" ) );
        Button->SetForegroundColour( wxColor( 0, 128, 80 ) );
        PropRightSizer->Add( Button, 0, wxGROW | wxALL, 5 );
    }
    else        // Module is edited in libedit
    {
        StaticText = new wxStaticText( m_PanelProperties, wxID_STATIC, _(
                                           "Doc" ), wxDefaultPosition, wxDefaultSize, 0 );
        PropLeftSizer->Add( StaticText, 0, wxGROW | wxLEFT | wxRIGHT | wxTOP | wxADJUST_MINSIZE, 5 );
        m_Doc = new wxTextCtrl( m_PanelProperties, -1,
                                m_CurrentModule->m_Doc );
        PropLeftSizer->Add( m_Doc, 0, wxGROW | wxLEFT | wxRIGHT | wxBOTTOM, 5 );

        StaticText = new wxStaticText( m_PanelProperties, wxID_STATIC, _(
                                           "Keywords" ), wxDefaultPosition, wxDefaultSize, 0 );
        PropLeftSizer->Add( StaticText, 0, wxGROW | wxLEFT | wxRIGHT | wxTOP | wxADJUST_MINSIZE, 5 );
        m_Keyword = new wxTextCtrl( m_PanelProperties, -1,
                                    m_CurrentModule->m_KeyWord );
        PropLeftSizer->Add( m_Keyword, 0, wxGROW | wxLEFT | wxRIGHT | wxBOTTOM, 5 );
    }

    wxStaticBox* box = new wxStaticBox( m_PanelProperties, -1, _( "Fields:" ) );
    m_TextListBox = new WinEDAChoiceBox( m_PanelProperties, ID_MODULE_LISTBOX_SELECT );
    ReCreateFieldListBox();
    m_TextListBox->SetSelection( 0 );

    wxStaticBoxSizer* StaticBoxSizer = new wxStaticBoxSizer( box, wxVERTICAL );
    PropLeftSizer->Add( StaticBoxSizer, 0, wxALIGN_CENTER_HORIZONTAL | wxALL, 5 );
    StaticBoxSizer->Add( m_TextListBox, 0, wxGROW | wxALL, 5 );

    Button = new wxButton( m_PanelProperties, ID_MODULE_EDIT_ADD_TEXT,
                          _( "Add Field" ) );
    Button->SetForegroundColour( *wxBLACK );
    StaticBoxSizer->Add( Button, 0, wxGROW | wxLEFT | wxRIGHT | wxTOP, 5 );

    Button = new wxButton( m_PanelProperties, ID_MODULE_EDIT_EDIT_TEXT,
                          _( "Edit Field" ) );
    Button->SetForegroundColour( *wxBLACK );
    StaticBoxSizer->Add( Button, 0, wxGROW | wxLEFT | wxRIGHT, 5 );

    m_DeleteFieddButton = Button = new wxButton( m_PanelProperties, ID_MODULE_EDIT_DELETE_TEXT,
                                                _( "Delete Field" ) );
    m_DeleteFieddButton->SetForegroundColour( *wxBLACK );
    m_DeleteFieddButton->Enable( FALSE ); // Enable pour fields autres que ref et valeur
    StaticBoxSizer->Add( Button, 0, wxGROW | wxLEFT | wxRIGHT | wxBOTTOM, 5 );

    if( FullOptions )
    {
        wxString layer_list[2] = { _( "Component" ), _( "Copper" ) };
        m_LayerCtrl = new wxRadioBox( m_PanelProperties, -1, _( "Layer" ), wxDefaultPosition,
                                      wxSize( -1, -1 ), 2, layer_list, 1 );
        m_LayerCtrl->SetSelection( (m_CurrentModule->GetLayer() == CUIVRE_N) ? 1 : 0 );
        PropLeftSizer->Add( m_LayerCtrl, 0, wxGROW | wxALL, 5 );

        bool     select = FALSE;
        
        wxString orient_list[5] = {
            _( "Normal" ), wxT( "+ 90.0" ), wxT( "- 90.0" ), wxT( "180.0" ), _( "User" )
        };
        
        m_OrientCtrl = new wxRadioBox( m_PanelProperties, ID_LISTBOX_ORIENT_SELECT, _( "Orient" ),
                                       wxDefaultPosition, wxSize( -1, -1 ), 5, orient_list, 1 );
        PropLeftSizer->Add( m_OrientCtrl, 0, wxGROW | wxALL, 5 );

        switch( m_CurrentModule->m_Orient )
        {
        case 0:
            m_OrientCtrl->SetSelection( 0 );
            break;

        case 900:
        case -2700:
            m_OrientCtrl->SetSelection( 1 );
            break;

        case -900:
        case 2700:
            m_OrientCtrl->SetSelection( 2 );
            break;

        case -1800:
        case 1800:
            m_OrientCtrl->SetSelection( 3 );
            break;

        default:
            m_OrientCtrl->SetSelection( 4 );
            select = TRUE;
            break;
        }

        StaticText = new wxStaticText( m_PanelProperties,
                                       wxID_STATIC, _(
                                           "Orient (0.1 deg)" ), wxDefaultPosition, wxDefaultSize,
                                       0 );
        PropLeftSizer->Add( StaticText, 0, wxGROW | wxLEFT | wxRIGHT | wxTOP | wxADJUST_MINSIZE, 5 );
        msg << m_CurrentModule->m_Orient;
        m_OrientValue = new wxTextCtrl( m_PanelProperties, -1, msg );
        m_OrientValue->Enable( select );
        PropLeftSizer->Add( m_OrientValue, 0, wxGROW | wxLEFT | wxRIGHT | wxBOTTOM, 5 );
    }

    /* Controls on right side of the dialog */
    wxString attribut_list[3] = { _( "Normal" ), _( "Normal+Insert" ), _( "Virtual" ) };
    m_AttributsCtrl = new wxRadioBox( m_PanelProperties, -1, _( "Attributs" ), wxDefaultPosition,
                                      wxSize( -1, -1 ), 3, attribut_list, 1 );
#if wxCHECK_VERSION( 2, 8, 0 )
    m_AttributsCtrl->SetItemToolTip( 0, _( "Use this attribute for most non smd components" ) );
    m_AttributsCtrl->SetItemToolTip( 1,
         _("Use this attribute for smd components.\nOnly components with this option are put in the footprint position list file"));
    m_AttributsCtrl->SetItemToolTip( 2,
         _("Use this attribute for \"virtual\" components drawn on board (like a old ISA PC bus connector)" ));
#endif
    PropRightSizer->Add( m_AttributsCtrl, 0, wxGROW | wxALL, 5 );

    switch( m_CurrentModule->m_Attributs & 255 )
    {
    case 0:
        m_AttributsCtrl->SetSelection( 0 );
        break;

    case MOD_CMS:
        m_AttributsCtrl->SetSelection( 1 );
        break;

    case MOD_VIRTUAL:
        m_AttributsCtrl->SetSelection( 2 );
        break;

    default:
        m_AttributsCtrl->SetSelection( 0 );
        break;
    }


    wxString properties_list[2] = { _( "Free" ), _( "Locked" ) };
    m_AutoPlaceCtrl = new wxRadioBox( m_PanelProperties, -1, _(
                                          "Move and Auto Place" ), wxDefaultPosition,
                                      wxSize( -1, -1 ), 2, properties_list, 1 );
    m_AutoPlaceCtrl->SetSelection(
        (m_CurrentModule->m_ModuleStatus & MODULE_is_LOCKED) ? 1 : 0 );
#if wxCHECK_VERSION( 2, 8, 0 )
    m_AutoPlaceCtrl->SetItemToolTip( 0, _( "Enable hotkey move commands and Auto Placement" ) );
    m_AutoPlaceCtrl->SetItemToolTip( 1, _( "Disable hotkey move commands and Auto Placement" ) );
#endif
    PropRightSizer->Add( m_AutoPlaceCtrl, 0, wxGROW | wxALL, 5 );

    StaticText = new wxStaticText( m_PanelProperties, -1, _( "Rot 90" ) );
    PropRightSizer->Add( StaticText, 0, wxGROW | wxLEFT | wxRIGHT | wxTOP | wxADJUST_MINSIZE, 5 );
    m_CostRot90Ctrl = new wxSlider( m_PanelProperties, -1,
                                    m_CurrentModule->m_CntRot90, 0, 10, wxDefaultPosition,
                                    wxSize( 100, -1 ),
                                    wxSL_HORIZONTAL + wxSL_AUTOTICKS + wxSL_LABELS );
    PropRightSizer->Add( m_CostRot90Ctrl, 0, wxGROW | wxLEFT | wxRIGHT | wxBOTTOM, 5 );

    StaticText = new wxStaticText( m_PanelProperties, -1, _( "Rot 180" ) );
    PropRightSizer->Add( StaticText, 0, wxGROW | wxLEFT | wxRIGHT | wxTOP | wxADJUST_MINSIZE, 5 );
    m_CostRot180Ctrl = new wxSlider( m_PanelProperties,
                                     -1,
                                     m_CurrentModule->m_CntRot180,
                                     0,
                                     10,
                                     wxDefaultPosition,
                                     wxSize( 100, -1 ),
                                     wxSL_HORIZONTAL + wxSL_AUTOTICKS + wxSL_LABELS );
    PropRightSizer->Add( m_CostRot180Ctrl, 0, wxGROW | wxLEFT | wxRIGHT | wxBOTTOM, 5 );
}


/**************************************************************/
Panel3D_Ctrl::Panel3D_Ctrl( WinEDA_ModulePropertiesFrame* parentframe,
                            wxNotebook* parent,
                            int id, Struct3D_Master* struct3D ) :
    wxPanel( parent, id )
/**************************************************************/

/* create the dialog panel managing 3D shape infos
 */
{
    wxButton*   button;
    S3D_Vertex  dummy_vertex;
    wxBoxSizer* PropLeftSizer;
    wxBoxSizer* PropRightSizer;

    m_Pnext = m_Pback = NULL;

    m_Parent      = parent;
    m_ParentFrame = parentframe;
    SetFont( *g_DialogFont );
    wxBoxSizer*   Panel3DBoxSizer = new wxBoxSizer( wxVERTICAL );
    SetSizer( Panel3DBoxSizer );

    wxStaticText* StaticText = new wxStaticText( this, wxID_STATIC, _( "3D Shape Name" ),
                                                 wxDefaultPosition, wxDefaultSize, 0 );
    Panel3DBoxSizer->Add( StaticText, 0, wxGROW | wxALL | wxADJUST_MINSIZE, 5 );
    m_3D_ShapeName = new wxTextCtrl( this, -1, _T( "" ), wxDefaultPosition, wxDefaultSize, 0 );
    if( struct3D )
        m_3D_ShapeName->SetValue( struct3D->m_Shape3DName );
    Panel3DBoxSizer->Add( m_3D_ShapeName, 0, wxGROW | wxALL, 5 );

    wxBoxSizer* LowerBoxSizer = new wxBoxSizer( wxHORIZONTAL );
    Panel3DBoxSizer->Add( LowerBoxSizer, 0, wxGROW | wxALL, 5 );
    /* Create a sizer for controls in the left column */
    PropLeftSizer = new wxBoxSizer( wxVERTICAL );
    LowerBoxSizer->Add( PropLeftSizer, 0, wxGROW | wxALL, 5 );
    /* Create a sizer for controls in the right column */
    PropRightSizer = new wxBoxSizer( wxVERTICAL );
    LowerBoxSizer->Add( PropRightSizer, 0, wxGROW | wxALL, 5 );

    button = new wxButton( this, ID_BROWSE_3D_LIB, _( "Browse" ) );
    button->SetForegroundColour( *wxBLUE );
    PropRightSizer->Add( button, 0, wxGROW | wxLEFT | wxRIGHT, 5 );

    button = new wxButton( this, ID_ADD_3D_SHAPE, _( "Add 3D Shape" ) );
    button->SetForegroundColour( *wxRED );
    PropRightSizer->Add( button, 0, wxGROW | wxLEFT | wxRIGHT, 5 );

    if( (struct3D == NULL) || (struct3D->Pback != NULL) )
    {
        button = new wxButton( this, ID_REMOVE_3D_SHAPE, _( "Remove 3D Shape" ) );
        button->SetForegroundColour( *wxRED );
        PropRightSizer->Add( button, 0, wxGROW | wxLEFT | wxRIGHT, 5 );
    }

    wxBoxSizer* BoxSizer = new wxBoxSizer( wxVERTICAL );
    m_3D_Scale = new WinEDA_VertexCtrl( this, _( "Shape Scale:" ), BoxSizer,
                                        2, 1 );
    if( struct3D )
        m_3D_Scale->SetValue( struct3D->m_MatScale );
    PropLeftSizer->Add( BoxSizer, 0, wxGROW | wxALL, 5 );

    BoxSizer    = new wxBoxSizer( wxVERTICAL );
    m_3D_Offset = new WinEDA_VertexCtrl( this, _( "Shape Offset:" ), BoxSizer,
                                         2, 1 );
    if( struct3D )
        m_3D_Offset->SetValue( struct3D->m_MatPosition );
    else
        m_3D_Offset->SetValue( dummy_vertex );
    PropLeftSizer->Add( BoxSizer, 0, wxGROW | wxALL, 5 );

    BoxSizer      = new wxBoxSizer( wxVERTICAL );
    m_3D_Rotation = new WinEDA_VertexCtrl( this, _( "Shape Rotation:" ), BoxSizer,
                                           2, 1 );
    if( struct3D )
        m_3D_Rotation->SetValue( struct3D->m_MatRotation );
    else
        m_3D_Rotation->SetValue( dummy_vertex );
    PropLeftSizer->Add( BoxSizer, 0, wxGROW | wxALL, 5 );

    if( struct3D == NULL )
    {
        dummy_vertex.x = dummy_vertex.y = dummy_vertex.z = 1.0;
        m_3D_Scale->SetValue( dummy_vertex );
    }
}


/********************************/
Panel3D_Ctrl::~Panel3D_Ctrl()
/********************************/
{
    delete m_3D_ShapeName;
    delete m_3D_Scale;
    delete m_3D_Offset;
    delete m_3D_Rotation;
}


/***************************************************/
void Panel3D_Ctrl::Browse3DLib( wxCommandEvent& event )
/***************************************************/
{
    wxString fullfilename, shortfilename;
    wxString fullpath = g_RealLibDirBuffer;
    wxString mask = wxT( "*" );

    fullpath += LIB3D_PATH;
    mask += g_Shapes3DExtBuffer;
#ifdef __WINDOWS__
    fullpath.Replace( wxT( "/" ), wxT( "\\" ) );
#endif
    fullfilename = EDA_FileSelector( _( "3D Shape:" ),
                                     fullpath,              /* Chemin par defaut */
                                     wxEmptyString,         /* nom fichier par defaut */
                                     g_Shapes3DExtBuffer,   /* extension par defaut */
                                     mask,                  /* Masque d'affichage */
                                     this,
                                     wxFD_OPEN,
                                     TRUE
                                     );

    if( fullfilename == wxEmptyString )
        return;

    shortfilename = MakeReducedFileName( fullfilename,
                                         fullpath, wxEmptyString );
    m_3D_ShapeName->SetValue( shortfilename );
}


/**********************************************************************/
void WinEDA_ModulePropertiesFrame::OnQuit( wxCommandEvent& WXUNUSED (event) )
/**********************************************************************/
{
    Close( true );    // true is to force the frame to close
}


/******************************************************************************/
void WinEDA_ModulePropertiesFrame::ModulePropertiesAccept( wxCommandEvent& event )
/******************************************************************************/
{
    bool change_layer = FALSE;

    if( m_DC )
        m_Parent->DrawPanel->CursorOff( m_DC );

    if( m_DC )
        m_CurrentModule->Draw( m_Parent->DrawPanel, m_DC, wxPoint( 0, 0 ), GR_XOR );

    if( m_OrientValue )
    {
        long orient = 0; wxString msg = m_OrientValue->GetValue();
        msg.ToLong( &orient );
        if( m_CurrentModule->m_Orient !=  orient )
            m_Parent->Rotate_Module( m_DC, m_CurrentModule,
                                     orient, FALSE );
    }

    if( m_LayerCtrl )
    {
        if( m_LayerCtrl->GetSelection() == 0 )     // layer req = COMPONENT
        {
            if( m_CurrentModule->GetLayer() == CUIVRE_N )
                change_layer = TRUE;
        }
        else if( m_CurrentModule->GetLayer() == CMP_N )
            change_layer = TRUE;
    }

    if( change_layer )
    {
        m_Parent->Change_Side_Module( m_CurrentModule, m_DC );
    }

    if( m_AutoPlaceCtrl->GetSelection() == 1 )
        m_CurrentModule->m_ModuleStatus |= MODULE_is_LOCKED;
    else
        m_CurrentModule->m_ModuleStatus &= ~MODULE_is_LOCKED;

    switch( m_AttributsCtrl->GetSelection() )
    {
    case 0:
        m_CurrentModule->m_Attributs = 0;
        break;

    case 1:
        m_CurrentModule->m_Attributs = MOD_CMS;
        break;

    case 2:
        m_CurrentModule->m_Attributs = MOD_VIRTUAL;
        break;
    }

    m_CurrentModule->m_CntRot90  = m_CostRot90Ctrl->GetValue();
    m_CurrentModule->m_CntRot180 = m_CostRot180Ctrl->GetValue();
    if( m_Doc )
        m_CurrentModule->m_Doc = m_Doc->GetValue();
    if( m_Keyword )
        m_CurrentModule->m_KeyWord = m_Keyword->GetValue();

    /* Mise a jour des parametres 3D */
    Panel3D_Ctrl*    panel3D = m_Panel3D;
    Struct3D_Master* draw3D  = m_CurrentModule->m_3D_Drawings,
    * nextdraw3D;
    for( ; panel3D != NULL; panel3D = panel3D->m_Pnext )
    {
        draw3D->m_Shape3DName = panel3D->m_3D_ShapeName->GetValue();
        draw3D->m_MatScale    = panel3D->m_3D_Scale->GetValue();
        draw3D->m_MatRotation = panel3D->m_3D_Rotation->GetValue();
        draw3D->m_MatPosition = panel3D->m_3D_Offset->GetValue();
        if( ( draw3D->m_Shape3DName.IsEmpty() )
           && (draw3D != m_CurrentModule->m_3D_Drawings) )
            continue;
        if( (draw3D->Pnext == NULL) && panel3D->m_Pnext )
        {
            nextdraw3D = new Struct3D_Master( draw3D );
            nextdraw3D->Pback = draw3D;
            draw3D->Pnext = nextdraw3D;
        }
        draw3D = (Struct3D_Master*) draw3D->Pnext;
    }

    for( ; draw3D != NULL; draw3D = nextdraw3D )
    {
        nextdraw3D = (Struct3D_Master*) draw3D->Pnext;
        (draw3D->Pback)->Pnext = NULL;
        delete draw3D;
    }

    m_CurrentModule->Set_Rectangle_Encadrement();

    m_Parent->GetScreen()->SetModify();

    Close( TRUE );

    if( m_DC )
        m_CurrentModule->Draw( m_Parent->DrawPanel, m_DC, wxPoint( 0, 0 ), GR_OR );
    if( m_DC )
        m_Parent->DrawPanel->CursorOn( m_DC );
}


/************************************************************************/
void WinEDA_ModulePropertiesFrame::GotoModuleEditor( wxCommandEvent& event )
/************************************************************************/
{
    GoToEditor = TRUE;
    if( m_CurrentModule->m_TimeStamp == 0 )    // Module Editor needs a non null timestamp
    {
        m_CurrentModule->m_TimeStamp = GetTimeStamp();
        m_Parent->GetScreen()->SetModify();
    }

    Close( TRUE );
}


/**********************************************************************/
void WinEDA_ModulePropertiesFrame::ExchangeModule( wxCommandEvent& event )
/**********************************************************************/
{
    m_Parent->InstallExchangeModuleFrame( m_CurrentModule,
                                         m_DC, wxPoint( -1, -1 ) );

    // Attention: si il y a eu echange, m_CurrentModule a été delete!
    m_Parent->GetScreen()->SetCurItem( NULL );
    Close( TRUE );
}


/*************************************************************************/
void WinEDA_ModulePropertiesFrame::ModuleOrientEvent( wxCommandEvent& event )
/*************************************************************************/
{
    switch( m_OrientCtrl->GetSelection() )
    {
    case 0:
        m_OrientValue->Enable( FALSE );
        m_OrientValue->SetValue( wxT( "0" ) );
        break;

    case 1:
        m_OrientValue->Enable( FALSE );
        m_OrientValue->SetValue( wxT( "900" ) );
        break;

    case 2:
        m_OrientValue->Enable( FALSE );
        m_OrientValue->SetValue( wxT( "2700" ) );
        break;

    case 3:
        m_OrientValue->Enable( FALSE );
        m_OrientValue->SetValue( wxT( "1800" ) );
        break;

    default:
        m_OrientValue->Enable( FALSE );
        m_OrientValue->Enable( TRUE );
        break;
    }
}


/*************************************************************************/
void WinEDA_ModulePropertiesFrame::SelectTextListBox( wxCommandEvent& event )
/*************************************************************************/
{
    SetTextListButtons();
}


/*************************************************************************/
void WinEDA_ModulePropertiesFrame::SetTextListButtons()
/*************************************************************************/
{
    int choice = m_TextListBox->GetChoice();

    if( m_DeleteFieddButton == NULL )
        return;

    if( choice > 1 )   // Texte autre que ref ou valeur selectionne
    {
        m_DeleteFieddButton->Enable( TRUE );
    }
    else
        m_DeleteFieddButton->Enable( FALSE );
}


/***********************************************************/
void WinEDA_ModulePropertiesFrame::ReCreateFieldListBox()
/***********************************************************/
{
    m_TextListBox->Clear();

    m_TextListBox->Append( m_CurrentModule->m_Reference->m_Text );
    m_TextListBox->Append( m_CurrentModule->m_Value->m_Text );

    EDA_BaseStruct* item = m_CurrentModule->m_Drawings;
    while( item )
    {
        if( item->Type() == TYPETEXTEMODULE )
            m_TextListBox->Append( ( (TEXTE_MODULE*) item )->m_Text );
        item = item->Pnext;
    }

    SetTextListButtons();
}


/************************************************************************/
void WinEDA_ModulePropertiesFrame::CreateTextModule( wxCommandEvent& event )
/************************************************************************/

/* Cree un nouveau texte sur le module actif
 *  Le texte sera mis en fonction Move
 */
{
    TEXTE_MODULE* Text;

    /* Creation de la place en memoire : */
    Text = m_Parent->CreateTextModule( m_CurrentModule, m_DC );

    ReCreateFieldListBox();
    m_TextListBox->SetSelection( 2 );
    SetTextListButtons();
}


/****************************************************************************/
void WinEDA_ModulePropertiesFrame::EditOrDelTextModule( wxCommandEvent& event )
/****************************************************************************/
{
    int           TextType = m_TextListBox->GetChoice();
    TEXTE_MODULE* Text = NULL;

    if( TextType < 0 )
        return;                 //No selection


    if( m_DC )
        m_Parent->DrawPanel->CursorOff( m_DC );


    // Get a pointer on the field
    if( TextType == 0 )
        Text = m_CurrentModule->m_Reference;
    else if( TextType == 1 )
        Text = m_CurrentModule->m_Value;
    else // Search the field 2 or more, because field 0 and 1 are ref and value
    {
        EDA_BaseStruct* item = m_CurrentModule->m_Drawings;
        int             jj   = 2;
        while( item )
        {
            if( item->Type() == TYPETEXTEMODULE )
            {
                if( jj == TextType )   // Texte trouvé
                {
                    Text = (TEXTE_MODULE*) item;
                    break;
                }
            }
            item = item->Pnext; jj++;
        }
    }

    if( Text )
    {
        if( event.GetId() == ID_MODULE_EDIT_DELETE_TEXT )
        {
            if( TextType < 2 )     // Ref or Value cannot be deleted
            {
                DisplayError( this, _( "Reference or Value cannot be deleted" ) );
                goto out;
            }
            wxString Line;
            Line.Printf( _( "Delete [%s]" ), Text->m_Text.GetData() );
            if( !IsOK( this, Line ) )
                goto out;
            m_Parent->DeleteTextModule( Text, m_DC );
            ReCreateFieldListBox();
            m_TextListBox->SetSelection( 0 );
        }
        else    // Edition du champ
        {
            m_Parent->InstallTextModOptionsFrame( Text, m_DC, wxPoint( -1, -1 ) );
            ReCreateFieldListBox();
            m_TextListBox->SetSelection( TextType );
        }
    }
    else
        DisplayError( this,
                     wxT(
                         "WinEDA_ModulePropertiesFrame::EditOrDelTextModule() error: Field not found" )
                      );

out:
    if( m_DC )
        m_Parent->DrawPanel->CursorOn( m_DC );
    SetTextListButtons();
}
