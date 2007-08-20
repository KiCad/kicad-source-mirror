/***************************************************/
/* EESchema:										*/
/* Edition des textes sur Composants en Schematique */
/****************************************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"

#include "wx/checklst.h"

#include "protos.h"


/* Fonctions exportees */

/* Fonctions locales */
static void AbortMoveCmpField( WinEDA_DrawFrame* frame, wxDC* DC );
static void MoveCmpField( WinEDA_DrawPanel* panel, wxDC* DC, bool erase );

/* variables locales */
static PartTextStruct* CurrentField;
static int             Multiflag;
static int             TextFieldSize = DEFAULT_SIZE_TEXT;
static wxPoint         OldPos;

/* Classe de la frame des propriétés d'un composant en librairie */

enum id_cmpedit {
    ID_SCHEDIT_NOTEBOOK = 3200,
    ID_PANEL_BASIC,
    ID_PANEL_REFERENCE,
    ID_PANEL_VALUE,
    ID_PANEL_FIELD1,
    ID_PANEL_FIELD2,
    ID_PANEL_FIELD3,
    ID_PANEL_FIELD4,
    ID_PANEL_FIELD5,
    ID_PANEL_FIELD6,
    ID_PANEL_FIELD7,
    ID_PANEL_FIELD8,
    ID_PANEL_MODULEPCB,
    ID_PANEL_SUBSCHEMATIC,
    ID_CLOSE_CMP_PROPERTIES,
    ID_ACCEPT_CMP_PROPERTIES,
    ID_RESTORE_CMP_DEFAULTS
};


/************************************/
/* class WinEDA_PartPropertiesFrame */
/************************************/

class WinEDA_ComponentPropertiesFrame : public wxDialog
{
private:

    WinEDA_SchematicFrame*  m_Parent;
    EDA_SchComponentStruct* m_Cmp;
    EDA_LibComponentStruct* m_LibEntry;

    wxCheckBox*             m_ConvertButt;
    wxRadioBox*             m_SelectUnit;
    wxRadioBox*             m_MirrorUnit;
    wxRadioBox*             m_OrientUnit;
    wxNotebook*             m_NoteBook;
    WinEDA_EnterText*       m_RefInLib;
    wxPanel*                m_PanelBasic;

    wxPanel*                m_PanelField[NUMBER_OF_FIELDS];

    wxCheckBox*             ShowFieldText[NUMBER_OF_FIELDS];
    wxCheckBox*             VorientFieldText[NUMBER_OF_FIELDS];

    WinEDA_GraphicTextCtrl* FieldTextCtrl[NUMBER_OF_FIELDS];
    WinEDA_PositionCtrl*    FieldPosition[NUMBER_OF_FIELDS];
    int FieldFlags[NUMBER_OF_FIELDS];
    int FieldOrient[NUMBER_OF_FIELDS];

public:

    // Constructor and destructor
    WinEDA_ComponentPropertiesFrame( WinEDA_SchematicFrame* parent, wxPoint& pos,
                                     EDA_SchComponentStruct* cmp );
    ~WinEDA_ComponentPropertiesFrame( void )
    {
    }


private:
    void    BuildPanelBasic( void );
    void    ComponentPropertiesAccept( wxCommandEvent& event );
    void    SetInitCmp( wxCommandEvent& event );
    void    OnQuit( wxCommandEvent& event );

    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE( WinEDA_ComponentPropertiesFrame, wxDialog )
EVT_BUTTON( ID_ACCEPT_CMP_PROPERTIES, WinEDA_ComponentPropertiesFrame::ComponentPropertiesAccept )
EVT_BUTTON( ID_CLOSE_CMP_PROPERTIES, WinEDA_ComponentPropertiesFrame::OnQuit )
EVT_BUTTON( ID_RESTORE_CMP_DEFAULTS, WinEDA_ComponentPropertiesFrame::SetInitCmp )
END_EVENT_TABLE()


/**********************************************************************/
void InstallCmpeditFrame( WinEDA_SchematicFrame* parent, wxPoint& pos,
                          EDA_SchComponentStruct* cmp )
/*********************************************************************/

/* Create the dialog box for the current component edition
 */
{
    parent->DrawPanel->m_IgnoreMouseEvents = TRUE;
    if( cmp->m_StructType != DRAW_LIB_ITEM_STRUCT_TYPE )
    {
        DisplayError( parent,
                     wxT( "InstallCmpeditFrame() error: This struct is not a component" ) );
    }
    else
    {
        WinEDA_ComponentPropertiesFrame* frame =
            new WinEDA_ComponentPropertiesFrame( parent, pos, cmp );
        frame->ShowModal(); frame->Destroy();
    }
    parent->DrawPanel->m_IgnoreMouseEvents = FALSE;
}


#define XSIZE 370
#define YSIZE 300
/***********************************************************************************/
WinEDA_ComponentPropertiesFrame::WinEDA_ComponentPropertiesFrame(
    WinEDA_SchematicFrame* parent, wxPoint& framepos, EDA_SchComponentStruct* cmp ) :
    wxDialog( parent, -1, _( "Component properties" ), framepos,
              wxSize( XSIZE, YSIZE ), DIALOG_STYLE )
/***********************************************************************************/
{
    wxPoint pos, postmp;
    wxLayoutConstraints* c;
    wxPoint cmp_pos;
    int     ii;

    Centre();
    m_Parent   = parent;
    m_Cmp      = cmp;
    cmp_pos    = m_Cmp->m_Pos;
    m_LibEntry = FindLibPart( m_Cmp->m_ChipName.GetData(), wxEmptyString, FIND_ROOT );

    if( m_LibEntry == NULL )
    {
        SetTitle( _( "Component properties (Not found in lib)" ) );
    }

    SetAutoLayout( TRUE );

    m_NoteBook = new wxNotebook( this, ID_SCHEDIT_NOTEBOOK,
                                wxDefaultPosition, wxSize( XSIZE - 6, YSIZE - 70 ) );
    c = new wxLayoutConstraints;
    c->left.SameAs( this, wxLeft, 4 );
    c->right.SameAs( this, wxRight, 4 );
    c->top.SameAs( this, wxTop, 4 );
    c->bottom.SameAs( this, wxBottom, 40 );
    m_NoteBook->SetConstraints( c );
    m_NoteBook->SetAutoLayout( TRUE );

    /* Creation des boutons de commande */
    pos.x = 40; pos.y = YSIZE - 60;
    wxButton* GButton = new wxButton( this, ID_CLOSE_CMP_PROPERTIES,
                                      _( "Close" ), pos );
    GButton->SetForegroundColour( *wxBLUE );
    c = new wxLayoutConstraints;
    c->left.SameAs( this, wxLeft, 20 );
    c->height.AsIs();
    c->width.AsIs();
    c->bottom.SameAs( this, wxBottom, 5 );
    GButton->SetConstraints( c );

    pos.x += GButton->GetDefaultSize().x + 10;
    wxButton* MButton = new wxButton( this, ID_RESTORE_CMP_DEFAULTS,
                                      _( "Defaults" ), pos );
    MButton->SetForegroundColour( *wxRED );
    c = new wxLayoutConstraints;
    c->left.SameAs( GButton, wxRight, 20 );
    c->height.AsIs();
    c->width.AsIs();
    c->bottom.SameAs( this, wxBottom, 5 );
    MButton->SetConstraints( c );

    pos.x += MButton->GetDefaultSize().x + 10;
    wxButton* Button = new wxButton( this, ID_ACCEPT_CMP_PROPERTIES,
                                     _( "Ok" ), pos );
    Button->SetForegroundColour( *wxBLUE );
    c = new wxLayoutConstraints;
    c->left.SameAs( MButton, wxRight, 20 );
    c->height.AsIs();
    c->width.AsIs();
    c->bottom.SameAs( this, wxBottom, 5 );
    Button->SetConstraints( c );

    // Add panel Basic
    BuildPanelBasic();
    m_NoteBook->AddPage( m_PanelBasic, _( "Options" ), TRUE );

    // Add panel Fields
    for( ii = 0; ii < NUMBER_OF_FIELDS; ii++ )
    {
        m_PanelField[ii] = new wxPanel( m_NoteBook, ID_PANEL_REFERENCE + ii );
        c = new wxLayoutConstraints;
        c->left.SameAs( m_NoteBook, wxLeft );
        c->right.SameAs( m_NoteBook, wxRight );
        c->bottom.SameAs( m_NoteBook, wxBottom );
        m_PanelField[ii]->SetConstraints( c );
        m_NoteBook->AddPage( m_PanelField[ii],
                             DrawPartStruct::ReturnFieldName( ii ), FALSE );

        pos.x = 10; pos.y = 20;
        ShowFieldText[ii] = new wxCheckBox( m_PanelField[ii], -1,
                                            _( "Show Text" ), pos );
        if( (m_Cmp->m_Field[ii].m_Attributs & TEXT_NO_VISIBLE ) == 0 )
            ShowFieldText[ii]->SetValue( TRUE );

        pos.x += 150;
        VorientFieldText[ii] = new wxCheckBox( m_PanelField[ii], -1,
                                               _( "Vertical" ), pos );
        if( m_Cmp->m_Field[ii].m_Orient )
            VorientFieldText[ii]->SetValue( TRUE );

        pos.x = 10; pos.y += 50;

        wxPoint field_pos;
        FieldTextCtrl[ii] = new WinEDA_GraphicTextCtrl( m_PanelField[ii],
                                                        DrawPartStruct::ReturnFieldName( ii ),
                                                        m_Cmp->m_Field[ii].m_Text,
                                                        m_Cmp->m_Field[ii].m_Size.x,
                                                        g_UnitMetric,
                                                        pos, 200, TRUE );
        field_pos.x = m_Cmp->m_Field[ii].m_Pos.x - cmp_pos.x;
        field_pos.y = m_Cmp->m_Field[ii].m_Pos.y - cmp_pos.y;
        if( m_Cmp->m_Field[ii].m_Text == wxEmptyString )  // Field non initialisé, set pos a 0,0)
            field_pos = wxPoint( 0, 0 );
        FieldPosition[ii] = new WinEDA_PositionCtrl( m_PanelField[ii], _( "Pos" ),
                                                     field_pos,
                                                     g_UnitMetric,
                                                     wxPoint( pos.x + 150, pos.y + 32 ),
                                                     m_Parent->m_InternalUnits );
    }

    if( m_LibEntry && m_LibEntry->m_Options == ENTRY_POWER )
        FieldTextCtrl[VALUE]->Enable( FALSE );
}


/************************************************************************/
void WinEDA_ComponentPropertiesFrame::OnQuit( wxCommandEvent& WXUNUSED (event) )
/************************************************************************/
{
    // true is to force the frame to close
    Close( true );
}


/**********************************************************/
void WinEDA_ComponentPropertiesFrame::BuildPanelBasic( void )
/**********************************************************/

/* create the basic panel for component properties editing
 */
{
    wxPoint pos, postmp;
    int     ii, jj;

    m_PanelBasic = new wxPanel( m_NoteBook, ID_PANEL_BASIC );
    wxLayoutConstraints* c = new wxLayoutConstraints;
    c->left.SameAs( m_NoteBook, wxLeft );
    c->right.SameAs( m_NoteBook, wxRight );
    c->bottom.SameAs( m_NoteBook, wxBottom );
    m_PanelBasic->SetConstraints( c );

    pos.x = 5; pos.y = 15;

#define NB_MAX_UNIT 16
    int      nb_units = m_LibEntry ? MAX( m_LibEntry->m_UnitCount, 1 ) : 0;
    wxString list_units[NB_MAX_UNIT];

    if( nb_units > 16 )
        nb_units = 16;
    for( ii = 0; ii < NB_MAX_UNIT; ii++ )
        list_units[ii] << _( "Unit" ) << (ii + 1);

    m_SelectUnit = new wxRadioBox( m_PanelBasic, -1, _( "Unit:" ),
                                   pos, wxSize( -1, -1 ),
                                   (nb_units < 8 ) ? 8 : nb_units, list_units, 1 );
    for( ii = nb_units; ii < 8; ii++ )
    {
        m_SelectUnit->Enable( ii, FALSE );    // Disable non existant units
    }

    m_SelectUnit->SetSelection( m_Cmp->m_Multi - 1 );

    m_SelectUnit->GetSize( &ii, &jj );
    pos.x += ii + 5; postmp = pos;

    wxString list_orient[4] = { wxT( "0" ), wxT( "+90" ), wxT( "180" ), wxT( "-90" ) };
    pos.x += 45; pos.y = 15;
    m_OrientUnit = new wxRadioBox( m_PanelBasic, -1, _( "Orient:" ),
                                   pos, wxSize( -1, -1 ), 4, list_orient, 1 );
    ii = m_Cmp->GetRotationMiroir() & ~(CMP_MIROIR_X | CMP_MIROIR_Y);

    if( ii == CMP_ORIENT_90 )
        m_OrientUnit->SetSelection( 1 );
    else if( ii == CMP_ORIENT_180 )
        m_OrientUnit->SetSelection( 2 );
    else if( ii == CMP_ORIENT_270 )
        m_OrientUnit->SetSelection( 3 );

    m_OrientUnit->GetSize( &ii, &jj );
    pos.x += ii + 30;
    wxString list_mirror[3] = { _( "Normal" ), _( "Mirror --" ), _( "Mirror |" ) };
    m_MirrorUnit = new wxRadioBox( m_PanelBasic, -1, _( "Mirror:" ),
                                   pos, wxSize( -1, -1 ), 3, list_mirror, 1 );
    ii = m_Cmp->GetRotationMiroir() & (CMP_MIROIR_X | CMP_MIROIR_Y);
    if( ii == CMP_MIROIR_X )
        m_MirrorUnit->SetSelection( 1 );
    else if( ii == CMP_MIROIR_Y )
        m_MirrorUnit->SetSelection( 2 );

    // Positionnement de la selection normal/convert
    m_OrientUnit->GetSize( &ii, &jj );
    pos    = postmp;
    pos.y += jj + 10;
    m_ConvertButt = new wxCheckBox( m_PanelBasic, -1, _( "Convert" ), pos );
    if( m_Cmp->m_Convert > 1 )
        m_ConvertButt->SetValue( TRUE );

    if( (m_LibEntry == NULL) || LookForConvertPart( m_LibEntry ) <= 1 )
    {
        m_ConvertButt->Enable( FALSE );
    }

    // Show the "Parts Locked" option:
    if( m_LibEntry && m_LibEntry->m_UnitSelectionLocked )
    {
        new wxStaticText( m_PanelBasic, -1, _( "Parts are locked" ),
                         wxPoint( m_MirrorUnit->GetRect().x, pos.y ) );
    }

    // Positionnement de la reference en librairie
    m_ConvertButt->GetSize( &ii, &jj );
    pos.y     += jj + 20;
    m_RefInLib = new WinEDA_EnterText( m_PanelBasic, _( "Chip Name:" ),
                                      m_Cmp->m_ChipName,
                                      pos, wxSize( XSIZE - pos.x - 30, -1 ) );
}


/***********************************************************************************/
void WinEDA_ComponentPropertiesFrame::ComponentPropertiesAccept( wxCommandEvent& event )
/***********************************************************************************/

/* Update the new parameters for the current edited component
 */
{
    wxPoint    cmp_pos = m_Cmp->m_Pos;
    wxClientDC dc( m_Parent->DrawPanel );
    wxString   newname;

    m_Parent->DrawPanel->PrepareGraphicContext( &dc );

    RedrawOneStruct( m_Parent->DrawPanel, &dc, m_Cmp, g_XorMode );

    newname = m_RefInLib->GetValue();
    newname.MakeUpper();
    newname.Replace( wxT( " " ), wxT( "_" ) );

    if( newname.IsEmpty() )
        DisplayError( this, _( "No Component Name!" ) );
    else if( newname.CmpNoCase( m_Cmp->m_ChipName ) )
    {
        if( FindLibPart( newname.GetData(), wxEmptyString, FIND_ALIAS ) == NULL )
        {
            wxString msg;
            msg.Printf( _( "Component [%s] not found!" ), newname.GetData() );
            DisplayError( this, msg );
        }
        else    // Changement de composant!
        {
            m_Cmp->m_ChipName = newname;
        }
    }

    // Mise a jour de la representation:
    if( m_ConvertButt->IsEnabled() )
        (m_ConvertButt->GetValue() == TRUE) ?
        m_Cmp->m_Convert = 2 : m_Cmp->m_Convert = 1;

    //Mise a jour de la selection de l'élément dans le boitier
    if( m_Cmp->m_Multi )
        m_Cmp->m_Multi = m_SelectUnit->GetSelection() + 1;

    //Mise a jour de l'orientation:
    switch( m_OrientUnit->GetSelection() )
    {
    case 0:
        m_Cmp->SetRotationMiroir( CMP_ORIENT_0 );
        break;

    case 1:
        m_Cmp->SetRotationMiroir( CMP_ORIENT_90 );
        break;

    case 2:
        m_Cmp->SetRotationMiroir( CMP_ORIENT_180 );
        break;

    case 3:
        m_Cmp->SetRotationMiroir( CMP_ORIENT_270 );
        break;
    }

    switch( m_MirrorUnit->GetSelection() )
    {
    case 0:
        break;

    case 1:
        m_Cmp->SetRotationMiroir( CMP_MIROIR_X );
        break;

    case 2:
        m_Cmp->SetRotationMiroir( CMP_MIROIR_Y );
        break;
    }


    // Mise a jour des textes
    for( int ii = REFERENCE; ii < NUMBER_OF_FIELDS; ii++ )
    {
        if( ii == REFERENCE )   // la reference ne peut etre vide
        {
            if( !FieldTextCtrl[ii]->GetText().IsEmpty() )
                m_Cmp->m_Field[ii].m_Text = FieldTextCtrl[ii]->GetText();
        }
        else if( ii == VALUE )  // la valeur ne peut etre vide et ne peut etre change sur un POWER
        {
            EDA_LibComponentStruct* Entry = FindLibPart( m_Cmp->m_ChipName.GetData(
                                                             ), wxEmptyString, FIND_ROOT );
            if( Entry && (Entry->m_Options == ENTRY_POWER) )
                m_Cmp->m_Field[ii].m_Text = m_Cmp->m_ChipName;
            else if( !FieldTextCtrl[ii]->GetText().IsEmpty() )
            {
                m_Cmp->m_Field[ii].m_Text = FieldTextCtrl[ii]->GetText();
            }
        }
        else
            m_Cmp->m_Field[ii].m_Text = FieldTextCtrl[ii]->GetText();

        m_Cmp->m_Field[ii].m_Size.x     =
            m_Cmp->m_Field[ii].m_Size.y = FieldTextCtrl[ii]->GetTextSize();
        if( ShowFieldText[ii]->GetValue() )
            m_Cmp->m_Field[ii].m_Attributs &= ~TEXT_NO_VISIBLE;
        else
            m_Cmp->m_Field[ii].m_Attributs |= TEXT_NO_VISIBLE;
        m_Cmp->m_Field[ii].m_Orient = VorientFieldText[ii]->GetValue() ? 1 : 0;
        m_Cmp->m_Field[ii].m_Pos    = FieldPosition[ii]->GetValue();
        m_Cmp->m_Field[ii].m_Pos.x += cmp_pos.x;
        m_Cmp->m_Field[ii].m_Pos.y += cmp_pos.y;
    }

    m_Parent->m_CurrentScreen->SetModify();

    RedrawOneStruct( m_Parent->DrawPanel, &dc, m_Cmp, GR_DEFAULT_DRAWMODE );
    m_Parent->TestDanglingEnds( m_Parent->m_CurrentScreen->EEDrawList, &dc );

    Close();
}


/************************************************************************************/
void WinEDA_SchematicFrame::StartMoveCmpField( PartTextStruct* Field, wxDC* DC )
/************************************************************************************/

/* Prepare le deplacement du texte en cours d'edition
 */
{
    EDA_LibComponentStruct* Entry;

    CurrentField = Field;
    if( Field == NULL )
        return;

    if( Field->m_Text == wxEmptyString )
    {
        DisplayError( this, _( "No Field to move" ), 10 );
        return;
    }

    OldPos    = Field->m_Pos;
    Multiflag = 0;
    if( Field->m_FieldId == REFERENCE )
    {
        Entry = FindLibPart( ( (EDA_SchComponentStruct*) Field->m_Parent )->m_ChipName.GetData(),
                            wxEmptyString, FIND_ROOT );
        if( Entry  != NULL )
        {
            if( Entry->m_UnitCount > 1 )
                Multiflag = 1;
        }
    }

    DrawPanel->ForceCloseManageCurseur = AbortMoveCmpField;
    DrawPanel->ManageCurseur = MoveCmpField;
    Field->m_Flags = IS_MOVED;
}


/**********************************************************************************/
void WinEDA_SchematicFrame::EditCmpFieldText( PartTextStruct* Field, wxDC* DC )
/**********************************************************************************/
/* Routine de changement du texte selectionne */
{
    int FieldNumber, flag;
    EDA_LibComponentStruct* Entry;

    if( Field == NULL )
    {
        DisplayError( this, _( "No Field To Edit" ), 10 );
        return;
    }

    FieldNumber = Field->m_FieldId;
    if( FieldNumber == VALUE )
    {
        Entry = FindLibPart( ( (EDA_SchComponentStruct*) Field->m_Parent )->m_ChipName.GetData(),
                            wxEmptyString, FIND_ROOT );
        if( Entry && (Entry->m_Options == ENTRY_POWER) )
        {
            DisplayInfo( this,
                        _(
                            "Part is a POWER, value cannot be modified!\nYou must create a new power" )
                         );
            return;
        }
    }

    flag = 0;
    if( FieldNumber == REFERENCE )
    {
        Entry = FindLibPart( ( (EDA_SchComponentStruct*) Field->m_Parent )->m_ChipName.GetData(),
                            wxEmptyString, FIND_ROOT );
        if( Entry != NULL )
        {
            if( Entry->m_UnitCount > 1 )
                flag = 1;
        }
    }


    wxString newtext = Field->m_Text;
    Get_Message( DrawPartStruct::ReturnFieldName( FieldNumber ), newtext, this );

    DrawTextField( DrawPanel, DC, Field, flag, g_XorMode );

    if( !newtext.IsEmpty() )
    {
        if( Field->m_Text.IsEmpty() )
        {
            Field->m_Pos    = ( (EDA_SchComponentStruct*) Field->m_Parent )->m_Pos;
            Field->m_Size.x = Field->m_Size.y = TextFieldSize;
        }
        Field->m_Text = newtext;
    }
    else    /* Nouveau texte NULL */
    {
        if( FieldNumber == REFERENCE )
        {
            DisplayError( this, _( "Reference needed !, No change" ) );
        }
        else if( FieldNumber == VALUE )
        {
            DisplayError( this, _( "Value needed !, No change" ) );
        }
        else
        {
            Field->m_Text = wxT( "~" );
        }
    }

    DrawTextField( DrawPanel, DC, Field, flag, g_XorMode );
    ( (EDA_SchComponentStruct*) Field->m_Parent )->Display_Infos( this );
    m_CurrentScreen->SetModify();
}


/************************************************************************/
static void MoveCmpField( WinEDA_DrawPanel* panel, wxDC* DC, bool erase )
/************************************************************************/

/* Routine de deplacement d'un texte type Field.
 *  Celle routine est normalement attachee au deplacement du curseur
 */
{
#define TRF ( (EDA_SchComponentStruct*) CurrentField->m_Parent )->m_Transform
    wxPoint pos;
    int     x1, y1;
    int     FieldNumber;

    if( CurrentField == NULL )
        return;

    FieldNumber = CurrentField->m_FieldId;

    /* Effacement: */
    if( erase )
        DrawTextField( panel, DC, CurrentField, Multiflag, g_XorMode );

    pos = ( (EDA_SchComponentStruct*) CurrentField->m_Parent )->m_Pos;

    /* Les positions sont caculees par la matrice TRANSPOSEE de la matrice
     *  de rotation-miroir */
    x1 = panel->GetScreen()->m_Curseur.x - pos.x;
    y1 = panel->GetScreen()->m_Curseur.y - pos.y;
    CurrentField->m_Pos.x = pos.x + TRF[0][0] * x1 + TRF[1][0] * y1;
    CurrentField->m_Pos.y = pos.y + TRF[0][1] * x1 + TRF[1][1] * y1;

    DrawTextField( panel, DC, CurrentField, Multiflag, g_XorMode );
}


/******************************************************************/
static void AbortMoveCmpField( WinEDA_DrawFrame* frame, wxDC* DC )
/******************************************************************/
{
    frame->DrawPanel->ForceCloseManageCurseur = NULL;
    frame->DrawPanel->ManageCurseur = NULL;
    if( CurrentField )
    {
        DrawTextField( frame->DrawPanel, DC, CurrentField, Multiflag, g_XorMode );
        CurrentField->m_Flags = 0;
        CurrentField->m_Pos   = OldPos;
        DrawTextField( frame->DrawPanel, DC, CurrentField, Multiflag, GR_DEFAULT_DRAWMODE );
    }
    CurrentField = NULL;
}


/*********************************************************************************/
void WinEDA_SchematicFrame::RotateCmpField( PartTextStruct* Field, wxDC* DC )
/*********************************************************************************/
{
    int FieldNumber, flag;
    EDA_LibComponentStruct* Entry;

    if( Field == NULL )
        return;
    if( Field->m_Text == wxEmptyString )
        return;

    FieldNumber = Field->m_FieldId;
    flag = 0;
    if( FieldNumber == REFERENCE )
    {
        Entry = FindLibPart( ( (EDA_SchComponentStruct*) Field->m_Parent )->m_ChipName.GetData(),
                            wxEmptyString, FIND_ROOT );
        if( Entry != NULL )
        {
            if( Entry->m_UnitCount > 1 )
                flag = 1;
        }
    }

    DrawTextField( DrawPanel, DC, Field, flag, g_XorMode );

    if( Field->m_Orient == TEXT_ORIENT_HORIZ )
        Field->m_Orient = TEXT_ORIENT_VERT;
    else
        Field->m_Orient = TEXT_ORIENT_HORIZ;
    DrawTextField( DrawPanel, DC, Field, flag, g_XorMode );

    GetScreen()->SetModify();
}


/***************************************************************/
void PartTextStruct::Place( WinEDA_DrawFrame* frame, wxDC* DC )
/***************************************************************/
{
    int FieldNumber, flag;
    EDA_LibComponentStruct* Entry;

    frame->DrawPanel->ManageCurseur = NULL;
    frame->DrawPanel->ForceCloseManageCurseur = NULL;

    FieldNumber = m_FieldId;
    flag = 0;
    if( FieldNumber == REFERENCE )
    {
        Entry = FindLibPart( ( (EDA_SchComponentStruct*) m_Parent )->m_ChipName.GetData(),
                            wxEmptyString, FIND_ROOT );
        if( Entry != NULL )
        {
            if( Entry->m_UnitCount > 1 )
                flag = 1;
        }
    }

    DrawTextField( frame->DrawPanel, DC, this, flag, GR_DEFAULT_DRAWMODE );
    m_Flags = 0;
    frame->GetScreen()->SetCurItem( NULL );
    frame->GetScreen()->SetModify();
    CurrentField = NULL;
}


/**************************************************************************************************/
void WinEDA_SchematicFrame::EditComponentReference( EDA_SchComponentStruct* DrawLibItem, wxDC* DC )
/**************************************************************************************************/
/* Edit the component text reference*/
{
    wxString msg;
    EDA_LibComponentStruct* Entry;
    int      flag = 0;

    if( DrawLibItem == NULL )
        return;

    Entry = FindLibPart( DrawLibItem->m_ChipName.GetData(), wxEmptyString, FIND_ROOT );
    if( Entry == NULL )
        return;

    if( Entry->m_UnitCount > 1 )
        flag = 1;

    PartTextStruct* TextField = &DrawLibItem->m_Field[REFERENCE];

    msg = TextField->m_Text;
    Get_Message( _( "Reference" ), msg, this );

    if( !msg.IsEmpty() ) // New text entered
    {
        DrawTextField( DrawPanel, DC, &DrawLibItem->m_Field[REFERENCE], flag, g_XorMode );
        TextField->m_Text = msg;
        DrawTextField( DrawPanel, DC, &DrawLibItem->m_Field[REFERENCE], flag,
                       DrawLibItem->m_Flags ? g_XorMode : GR_DEFAULT_DRAWMODE );
        GetScreen()->SetModify();
    }
    DrawLibItem->Display_Infos( this );
}


/*****************************************************************************************/
void WinEDA_SchematicFrame::EditComponentValue( EDA_SchComponentStruct* DrawLibItem, wxDC* DC )
/*****************************************************************************************/
/* Routine de changement du texte selectionne */
{
    wxString msg;
    EDA_LibComponentStruct* Entry;
    int      flag = 0;

    if( DrawLibItem == NULL )
        return;

    Entry = FindLibPart( DrawLibItem->m_ChipName.GetData(), wxEmptyString, FIND_ROOT );
    if( Entry == NULL )
        return;
    if( Entry->m_UnitCount > 1 )
        flag = 1;

    PartTextStruct* TextField = &DrawLibItem->m_Field[VALUE];

    msg = TextField->m_Text;
    Get_Message( _( "Value" ), msg, this );

    if( !msg.IsEmpty() )
    {
        DrawTextField( DrawPanel, DC, &DrawLibItem->m_Field[VALUE], flag, g_XorMode );
        TextField->m_Text = msg;
        DrawTextField( DrawPanel, DC, &DrawLibItem->m_Field[VALUE], flag,
                       DrawLibItem->m_Flags ? g_XorMode : GR_DEFAULT_DRAWMODE );
        m_CurrentScreen->SetModify();
    }

    DrawLibItem->Display_Infos( this );
}


/*****************************************************************************/
void WinEDA_ComponentPropertiesFrame::SetInitCmp( wxCommandEvent& event )
/*****************************************************************************/

/* Replace le composant en position normale, dimensions et positions
 *  fields comme definies en librairie
 */
{
    EDA_LibComponentStruct* Entry;

    if( m_Cmp == NULL )
        return;

    Entry = FindLibPart( m_Cmp->m_ChipName.GetData(), wxEmptyString, FIND_ROOT );

    if( Entry == NULL )
        return;

    wxClientDC dc( m_Parent->DrawPanel );
    m_Parent->DrawPanel->PrepareGraphicContext( &dc );

    RedrawOneStruct( m_Parent->DrawPanel, &dc, m_Cmp, g_XorMode );

    /* Mise aux valeurs par defaut des champs et orientation */
    m_Cmp->m_Field[REFERENCE].m_Pos.x =
        Entry->m_Prefix.m_Pos.x + m_Cmp->m_Pos.x;
    m_Cmp->m_Field[REFERENCE].m_Pos.y =
        Entry->m_Prefix.m_Pos.y + m_Cmp->m_Pos.y;
    m_Cmp->m_Field[REFERENCE].m_Orient   = Entry->m_Prefix.m_Orient;
    m_Cmp->m_Field[REFERENCE].m_Size     = Entry->m_Prefix.m_Size;
    m_Cmp->m_Field[REFERENCE].m_HJustify = Entry->m_Prefix.m_HJustify;
    m_Cmp->m_Field[REFERENCE].m_VJustify = Entry->m_Prefix.m_VJustify;

    m_Cmp->m_Field[VALUE].m_Pos.x =
        Entry->m_Name.m_Pos.x + m_Cmp->m_Pos.x;
    m_Cmp->m_Field[VALUE].m_Pos.y =
        Entry->m_Name.m_Pos.y + m_Cmp->m_Pos.y;
    m_Cmp->m_Field[VALUE].m_Orient   = Entry->m_Name.m_Orient;
    m_Cmp->m_Field[VALUE].m_Size     = Entry->m_Name.m_Size;
    m_Cmp->m_Field[VALUE].m_HJustify = Entry->m_Name.m_HJustify;
    m_Cmp->m_Field[VALUE].m_VJustify = Entry->m_Name.m_VJustify;

    m_Cmp->SetRotationMiroir( CMP_NORMAL );

    m_Parent->m_CurrentScreen->SetModify();

    RedrawOneStruct( m_Parent->DrawPanel, &dc, m_Cmp, GR_DEFAULT_DRAWMODE );
    Close();
}
