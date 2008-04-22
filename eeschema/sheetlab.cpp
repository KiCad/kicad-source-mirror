/****************************************************************/
/*	sheetlab.cpp  module pour creation /editin des Sheet labels */
/****************************************************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"

#include "protos.h"


/* Routines Locales */
static void     ExitPinSheet( WinEDA_DrawPanel* Panel, wxDC* DC );
static void     Move_PinSheet( WinEDA_DrawPanel* panel, wxDC* DC, bool erase );

/* Variables locales */
static int CurrentTypeLabel = NET_INPUT;
static wxSize   NetSheetTextSize( DEFAULT_SIZE_TEXT, DEFAULT_SIZE_TEXT );

/****************************************/
/* class WinEDA_PinSheetPropertiesFrame */
/****************************************/

#define NBSHAPES 5
static wxString shape_list[NBSHAPES] =
{
    wxT( "Input" ), wxT( "Output" ), wxT( "Bidi" ), wxT( "TriState" ), wxT( "Passive" )
};


/*****************************************************/
class WinEDA_PinSheetPropertiesFrame : public wxDialog
/*****************************************************/
{
private:

    WinEDA_SchematicFrame*  m_Parent;
    Hierarchical_PIN_Sheet_Struct*   m_CurrentPinSheet;
    wxRadioBox*             m_PinSheetType;
    wxRadioBox*             m_PinSheetShape;
    WinEDA_GraphicTextCtrl* m_TextWin;

public:

    // Constructor and destructor
    WinEDA_PinSheetPropertiesFrame( WinEDA_SchematicFrame* parent,
                                   Hierarchical_PIN_Sheet_Struct* curr_pinsheet,
                                   const wxPoint& framepos = wxPoint( -1, -1 ) );
    ~WinEDA_PinSheetPropertiesFrame() { };

private:
    void    OnOkClick( wxCommandEvent& event );
    void    OnCancelClick( wxCommandEvent& event );

    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE( WinEDA_PinSheetPropertiesFrame, wxDialog )
EVT_BUTTON( wxID_OK, WinEDA_PinSheetPropertiesFrame::OnOkClick )
EVT_BUTTON( wxID_CANCEL, WinEDA_PinSheetPropertiesFrame::OnCancelClick )
END_EVENT_TABLE()


/**********************************************************************************/
WinEDA_PinSheetPropertiesFrame::WinEDA_PinSheetPropertiesFrame(
    WinEDA_SchematicFrame* parent,
    Hierarchical_PIN_Sheet_Struct*  curr_pinsheet,
    const wxPoint&         framepos ) :
    wxDialog( parent, -1, _( "PinSheet Properties:" ), framepos, wxSize( 340, 220 ),
              DIALOG_STYLE )
/**********************************************************************************/
{
    wxPoint   pos;
    wxString  number;
    wxButton* Button;

    m_Parent = parent;
    Centre();

    wxBoxSizer* MainBoxSizer = new wxBoxSizer( wxHORIZONTAL );
    SetSizer( MainBoxSizer );
    wxBoxSizer* LeftBoxSizer  = new wxBoxSizer( wxVERTICAL );
    wxBoxSizer* RightBoxSizer = new wxBoxSizer( wxVERTICAL );
    MainBoxSizer->Add( LeftBoxSizer, 0, wxGROW | wxALL, 5 );
    MainBoxSizer->Add( RightBoxSizer, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5 );

    m_CurrentPinSheet = curr_pinsheet;

    /* Creation des boutons de commande */
    Button = new wxButton( this, wxID_OK, _( "OK" ) );
    Button->SetForegroundColour( *wxRED );
    RightBoxSizer->Add( Button, 0, wxGROW | wxALL, 5 );

    Button = new wxButton( this, wxID_CANCEL, _( "Cancel" ) );
    Button->SetForegroundColour( *wxBLUE );
    RightBoxSizer->Add( Button, 0, wxGROW | wxALL, 5 );

    m_TextWin = new WinEDA_GraphicTextCtrl( this, _( "Text:" ),
                                            m_CurrentPinSheet->m_Text, m_CurrentPinSheet->m_Size.x,
                                            g_UnitMetric, LeftBoxSizer, 200 );

    // Selection de la forme :
    m_PinSheetShape = new wxRadioBox( this, -1, _( "PinSheet Shape:" ),
                                      wxDefaultPosition, wxSize( -1, -1 ),
                                      NBSHAPES, shape_list, 1 );
    m_PinSheetShape->SetSelection( m_CurrentPinSheet->m_Shape );
    LeftBoxSizer->Add( m_PinSheetShape, 0, wxGROW | wxALL, 5 );

    GetSizer()->Fit( this );
    GetSizer()->SetSizeHints( this );
}


/************************************************************************/
void WinEDA_PinSheetPropertiesFrame::OnCancelClick( wxCommandEvent& WXUNUSED (event) )
/************************************************************************/
{
    EndModal( -1 );
}


/***********************************************************************************/
void WinEDA_PinSheetPropertiesFrame::OnOkClick( wxCommandEvent& event )
/***********************************************************************************/
{
    m_CurrentPinSheet->m_Text   = m_TextWin->GetText();
    m_CurrentPinSheet->m_Size.x = m_CurrentPinSheet->m_Size.y = m_TextWin->GetTextSize();

    m_CurrentPinSheet->m_Shape = m_PinSheetShape->GetSelection();
    EndModal( 0 );
}


/*****************************************************************/
static void ExitPinSheet( WinEDA_DrawPanel* Panel, wxDC* DC )
/*****************************************************************/

/*  Routine de sortie du Menu d'Edition Des NETS (Labels) SHEET
 */
{
    Hierarchical_PIN_Sheet_Struct* SheetLabel = (Hierarchical_PIN_Sheet_Struct*)
                                       Panel->GetScreen()->GetCurItem();

    if( SheetLabel == NULL )
        return;

    if( SheetLabel->m_Flags & IS_NEW )
    {     /* Nouveau Placement en cours, on l'efface */
        RedrawOneStruct( Panel, DC, SheetLabel, g_XorMode );
        SAFE_DELETE( SheetLabel );
    }
    else
    {
        RedrawOneStruct( Panel, DC, SheetLabel, GR_DEFAULT_DRAWMODE );
        SheetLabel->m_Flags = 0;
    }

    Panel->GetScreen()->SetCurItem( NULL );
    Panel->ManageCurseur = NULL;
    Panel->ForceCloseManageCurseur = NULL;
}


/* Cette routine place un nouveau NetSheet ou place un ancien en cours
 *  de deplacement
 *  Si le NetSheet est nouveau, il est pointe par NewSheetLabel
 */
void Hierarchical_PIN_Sheet_Struct::Place( WinEDA_SchematicFrame* frame, wxDC* DC )
{
    DrawSheetStruct* Sheet = (DrawSheetStruct*) m_Parent;

    if( m_Flags & IS_NEW )  /* ajout a la liste des structures */
    {
        if( Sheet->m_Label == NULL )
            Sheet->m_Label = this;
        else
        {
            Hierarchical_PIN_Sheet_Struct* pinsheet = Sheet->m_Label;
            while( pinsheet )
            {
                if( pinsheet->Pnext == NULL )
                {
                    pinsheet->Pnext = this;
                    break;
                }
                pinsheet = (Hierarchical_PIN_Sheet_Struct*) pinsheet->Pnext;
            }
        }
    }

    m_Flags = 0;
    m_Pos.x = Sheet->m_Pos.x;
    m_Edge  = 0;
    if( frame->GetScreen()->m_Curseur.x > ( Sheet->m_Pos.x + (Sheet->m_Size.x / 2) ) )
    {
        m_Edge  = 1;
        m_Pos.x = Sheet->m_Pos.x + Sheet->m_Size.x;
    }

    m_Pos.y = frame->GetScreen()->m_Curseur.y;
    if( m_Pos.y < Sheet->m_Pos.y )
        m_Pos.y = Sheet->m_Pos.y;
    if( m_Pos.y > (Sheet->m_Pos.y + Sheet->m_Size.y) )
        m_Pos.y = Sheet->m_Pos.y + Sheet->m_Size.y;

    RedrawOneStruct( frame->DrawPanel, DC, Sheet, GR_DEFAULT_DRAWMODE );

    frame->DrawPanel->ManageCurseur = NULL;
    frame->DrawPanel->ForceCloseManageCurseur = NULL;
}


/*******************************************************************************/
void WinEDA_SchematicFrame::StartMove_PinSheet( Hierarchical_PIN_Sheet_Struct* SheetLabel,
                                                wxDC*                 DC )
/*******************************************************************************/
/* Initialise un deplacement de NetSheet */
{
    NetSheetTextSize     = SheetLabel->m_Size;
    CurrentTypeLabel     = SheetLabel->m_Shape;
    SheetLabel->m_Flags |= IS_MOVED;

    DrawPanel->ManageCurseur = Move_PinSheet;
    DrawPanel->ForceCloseManageCurseur = ExitPinSheet;
    DrawPanel->ManageCurseur( DrawPanel, DC, TRUE );
}


/**********************************************************************/
/* Routine de deplacement du  NetSheet actif selon la position souris */
/**********************************************************************/

static void Move_PinSheet( WinEDA_DrawPanel* panel, wxDC* DC, bool erase )
{
    Hierarchical_PIN_Sheet_Struct* SheetLabel = (Hierarchical_PIN_Sheet_Struct*)
                                       panel->GetScreen()->GetCurItem();

    if( SheetLabel == NULL )
        return;

    DrawSheetStruct* Sheet = (DrawSheetStruct*) SheetLabel->m_Parent;

    if( Sheet == NULL )
        return;
    if( erase )
        RedrawOneStruct( panel, DC, SheetLabel, g_XorMode );

    SheetLabel->m_Edge  = 0;
    SheetLabel->m_Pos.x = Sheet->m_Pos.x;
    if( panel->GetScreen()->m_Curseur.x > ( Sheet->m_Pos.x + (Sheet->m_Size.x / 2) ) )
    {
        SheetLabel->m_Edge  = 1;
        SheetLabel->m_Pos.x = Sheet->m_Pos.x + Sheet->m_Size.x;
    }

    SheetLabel->m_Pos.y = panel->GetScreen()->m_Curseur.y;
    if( SheetLabel->m_Pos.y < Sheet->m_Pos.y )
        SheetLabel->m_Pos.y = Sheet->m_Pos.y;
    if( SheetLabel->m_Pos.y > (Sheet->m_Pos.y + Sheet->m_Size.y) )
        SheetLabel->m_Pos.y = Sheet->m_Pos.y + Sheet->m_Size.y;

    RedrawOneStruct( panel, DC, SheetLabel, g_XorMode );
}


/***************************************************************************/
void WinEDA_SchematicFrame::Edit_PinSheet( Hierarchical_PIN_Sheet_Struct* SheetLabel,
                                           wxDC*                 DC )
/***************************************************************************/
/* Modification du texte d'un net sheet */
{
    if( SheetLabel == NULL )
        return;

    RedrawOneStruct( DrawPanel, DC, SheetLabel, g_XorMode );

    WinEDA_PinSheetPropertiesFrame* frame =
        new WinEDA_PinSheetPropertiesFrame( this, SheetLabel );

    frame->ShowModal(); frame->Destroy();

    RedrawOneStruct( DrawPanel, DC, SheetLabel, GR_DEFAULT_DRAWMODE );
}


/***************************************************************/
Hierarchical_PIN_Sheet_Struct* WinEDA_SchematicFrame::Create_PinSheet(
    DrawSheetStruct* Sheet, wxDC* DC )
/**************************************************************/

/* Addition d'un nouveau PinSheet sur la feuille selectionnee, a l'endroit
 *  pointe par la souris
 */
{
    wxString Line, Text;
    Hierarchical_PIN_Sheet_Struct* NewSheetLabel;

    switch( CurrentTypeLabel )
    {
    default:
        CurrentTypeLabel = NET_INPUT;

    case NET_INPUT:
        Text = wxT( "Pin Input: " );
        break;

    case NET_OUTPUT:
        Text = wxT( "Pin Output: " );
        break;

    case NET_BIDI:
        Text = wxT( "Pin BiDi: " );
        break;

    case NET_TRISTATE:
        Text = wxT( "Pin TriStat: " );
        break;

    case NET_UNSPECIFIED:
        Text = wxT( "Pin Unspec.: " );
        break;
    }

    Get_Message( Text, Line, this );
    if( Line.IsEmpty() )
        return NULL;

    GetScreen()->SetModify();

    /* Creation en memoire */
    NewSheetLabel = new Hierarchical_PIN_Sheet_Struct( Sheet, wxPoint( 0, 0 ), Line );
    NewSheetLabel->m_Flags = IS_NEW;
    NewSheetLabel->m_Size  = NetSheetTextSize;
    NewSheetLabel->m_Shape = CurrentTypeLabel;

    GetScreen()->SetCurItem( NewSheetLabel );

    DrawPanel->ManageCurseur = Move_PinSheet;
    DrawPanel->ForceCloseManageCurseur = ExitPinSheet;
    DrawPanel->ManageCurseur( DrawPanel, DC, TRUE );

    return NewSheetLabel;
}


/*****************************************************************************/
Hierarchical_PIN_Sheet_Struct* WinEDA_SchematicFrame::Import_PinSheet( DrawSheetStruct* Sheet, wxDC* DC )
/*****************************************************************************/

/* Permet de creer automatiquement les Sheet Labels a partir des Labels Globaux
 *  de la feuille de sous hierarchie correspondante
 */
{
    EDA_BaseStruct*     	DrawStruct;
    Hierarchical_PIN_Sheet_Struct* 	NewSheetLabel, * SheetLabel = NULL;
    SCH_HIERLABEL* 	HLabel = NULL;

    if(!Sheet->m_AssociatedScreen) return NULL;
    DrawStruct = Sheet->m_AssociatedScreen->EEDrawList;
    HLabel = NULL;
    for( ; DrawStruct != NULL; DrawStruct = DrawStruct->Pnext )
    {
        if( DrawStruct->Type() != TYPE_SCH_HIERLABEL )
            continue;
        HLabel = (SCH_HIERLABEL*) DrawStruct;

        /* Ici un G-Label a ete trouve: y a t-il un SheetLabel correspondant */
        SheetLabel = Sheet->m_Label;
        for( ; SheetLabel != NULL; SheetLabel = (Hierarchical_PIN_Sheet_Struct*) SheetLabel->Pnext )
        {
            if( SheetLabel->m_Text.CmpNoCase( HLabel->m_Text ) == 0 )
            {
                break;
            }
        }

        /* Ici si SheetLabel == NULL le G-Label n'a pas de SheetLabel corresp */
        if( SheetLabel == NULL )
            break;
    }

    if( (HLabel == NULL ) || SheetLabel )
    {
        DisplayError( this, _( "No New Hierarchal Label found" ), 10 );
        return NULL;
    }

    /* Ici H-Label n'a pas de SheetLabel corresp, on va le creer */

    GetScreen()->SetModify();
    /* Creation en memoire */
    NewSheetLabel = new Hierarchical_PIN_Sheet_Struct( Sheet, wxPoint( 0, 0 ), HLabel->m_Text );
    NewSheetLabel->m_Flags = IS_NEW;
    NewSheetLabel->m_Size  = NetSheetTextSize;
    CurrentTypeLabel = NewSheetLabel->m_Shape = HLabel->m_Shape;

    GetScreen()->SetCurItem( NewSheetLabel );
    DrawPanel->ManageCurseur = Move_PinSheet;
    DrawPanel->ForceCloseManageCurseur = ExitPinSheet;
    Move_PinSheet( DrawPanel, DC, FALSE );

    return NewSheetLabel;
}


/**************************************************************/
void WinEDA_SchematicFrame::DeleteSheetLabel( wxDC* DC,
              Hierarchical_PIN_Sheet_Struct* SheetLabelToDel )
/**************************************************************/

/*
 *  Routine de suppression de 1 Structure type (Hierarchical_PIN_Sheet_Struct.
 *  Cette Structure ne peut etre mise en pile "undelete" car il ne serait pas
 *  possible de la ratacher a la 'DrawSheetStruct' d'origine
 *  si DC != NULL, effacement a l'ecran du dessin
 */
{
    if( DC )
        RedrawOneStruct( DrawPanel, DC, SheetLabelToDel, g_XorMode );

    DrawSheetStruct* parent = (DrawSheetStruct*) SheetLabelToDel->m_Parent;

    wxASSERT( parent );
    wxASSERT( parent->Type() == DRAW_SHEET_STRUCT_TYPE );

#if 0 && defined(DEBUG)
    std::cout << "\n\nbefore deleting:\n" << std::flush;
    parent->Show( 0, std::cout );
    std::cout << "\n\n\n" << std::flush;
#endif

    Hierarchical_PIN_Sheet_Struct* label = parent->m_Label;

    Hierarchical_PIN_Sheet_Struct** pprev = &parent->m_Label;

    while( label )
    {
        if( label == SheetLabelToDel )
        {
            *pprev = label->Next();
            break;
        }

        pprev = (Hierarchical_PIN_Sheet_Struct**) &label->Pnext;
        label = label->Next();
    }

    delete SheetLabelToDel;

#if 0 && defined(DEBUG)
    std::cout << "\n\nafter deleting:\n" << std::flush;
    parent->Show( 0, std::cout );
    std::cout << "~after deleting\n\n" << std::flush;
#endif
}
