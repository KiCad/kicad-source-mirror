/***************************/
/*  EESchema - PinEdit.cpp */
/***************************/

#include "fctsys.h"

#include "program.h"
#include "libeditfrm.h"
#include "eeschema_id.h"
#include "class_libentry.h"

#include "pinedit-dialog.h"

static int CodeOrient[4] =
{
    PIN_RIGHT,
    PIN_LEFT,
    PIN_UP,
    PIN_DOWN
};

#define NBSHAPES 7
static wxString shape_list[NBSHAPES] =
{
    _( "line" ),   _( "invert" ),    _( "clock" ), _( "clock inv" ),
    _( "low in" ), _( "low clock" ), _( "low out" )
};

int             CodeShape[NBSHAPES] =
{
    NONE, INVERT, CLOCK, CLOCK | INVERT, LOWLEVEL_IN, LOWLEVEL_IN | CLOCK,
    LOWLEVEL_OUT
};


/* Routines locales */
static void CreateImagePins( LibDrawPin* Pin, int unit, int convert,
                             bool asDeMorgan );
static void AbortPinMove( WinEDA_DrawPanel* Panel, wxDC* DC );
static void DrawMovePin( WinEDA_DrawPanel* panel, wxDC* DC, bool erase );

/* Variables importees */

/* Variables locales */
static wxPoint OldPos, PinPreviousPos;
static int     LastPinType          = PIN_INPUT,
               LastPinOrient        = PIN_RIGHT,
               LastPinShape         = NONE,
               LastPinSize          = 300,
               LastPinNameSize      = 50,
               LastPinNumSize       = 50,
               LastPinCommonConvert = false,
               LastPinCommonUnit    = false,
               LastPinNoDraw        = false;


#include "pinedit-dialog.cpp"

/*************************************************************************/
void WinEDA_PinPropertiesFrame::PinPropertiesAccept( wxCommandEvent& event )
/*************************************************************************/

/* Met a jour les differents parametres pour le composant en cours d'�dition
 */
{
    wxString msg;

    LastPinType   = m_PinElectricalType->GetSelection();
    LastPinShape  = CodeShape[m_PinShape->GetSelection()];
    LastPinOrient = CodeOrient[m_PinOrient->GetSelection()];
    LastPinCommonConvert = m_CommonConvert->GetValue();
    LastPinCommonUnit    = m_CommonUnit->GetValue();
    LastPinNoDraw = m_NoDraw->GetValue();

    msg = m_PinSizeCtrl->GetValue();
    LastPinSize   = ReturnValueFromString( g_UnitMetric, msg,
                                           m_Parent->m_InternalUnits );

    msg = m_PinNameSizeCtrl->GetValue();
    LastPinNameSize = ReturnValueFromString( g_UnitMetric, msg,
                                             m_Parent->m_InternalUnits );

    msg = m_PinNumSizeCtrl->GetValue();
    LastPinNumSize = ReturnValueFromString( g_UnitMetric, msg,
                                            m_Parent->m_InternalUnits );

    LIB_DRAW_ITEM* item = m_Parent->GetDrawItem();

    if( item == NULL )
        return;

    if( !( item->m_Flags & IS_NEW ) )  // if IS_NEW, copy for undo is done before place
        m_Parent->SaveCopyInUndoList( item->GetParent() );

    SetPinName( m_PinNameCtrl->GetValue(), LastPinNameSize );
    msg = m_PinNumCtrl->GetValue();

    if( msg.IsEmpty() )
        msg = wxT( "~" );

    SetPinNum( msg, LastPinNumSize );
    NewSizePin( LastPinSize );
    SetPinShape( LastPinShape );
    SetPinType( LastPinType );
    SetPinOrientation( LastPinOrient );

    // Set all attributes (visibility, common to units and common to
    // convert options)
    SetPinAttributes( true, true, true );
    item->DisplayInfo( m_Parent );

    m_Parent->DrawPanel->Refresh();
}


/*
 * Called when installing the edit pin dialog frame
 * Set pins flags (.m_Flags pins member) to ensure a correctins edition:
 * If 2 or more pins are on the same location (and the same orientation) they
 * are all moved or resized.
 * This is usefull for components which have more than one part per package
 * In this case all parts can be edited at once.
 * Note: if the option "Edit Pin per Pin" (tool of the main toolbar) is
 * activated, only the current part is edited.
 */
void WinEDA_LibeditFrame::InitEditOnePin()
{
    LibDrawPin* Pin;
    LibDrawPin* CurrentPin = (LibDrawPin*) m_drawItem;

    if( m_component == NULL || CurrentPin == NULL
        || m_drawItem->Type() != COMPONENT_PIN_DRAW_TYPE )
        return;

    for( Pin = m_component->GetNextPin(); Pin != NULL;
         Pin = m_component->GetNextPin( Pin ) )
    {
        if( Pin == CurrentPin )
            continue;
        if( ( Pin->m_Pos == CurrentPin->m_Pos )
            && ( Pin->m_Orient == CurrentPin->m_Orient )
            && ( !( CurrentPin->m_Flags & IS_NEW ) )
            && ( g_EditPinByPinIsOn == false ) )
            Pin->m_Flags |= IS_LINKED | IN_EDIT;
        else
            Pin->m_Flags = 0;
    }

    CurrentPin->DisplayInfo( this );
}


/**
 * Clean up after aborting a move pin command.
 */
static void AbortPinMove( WinEDA_DrawPanel* Panel, wxDC* DC )
{
    WinEDA_LibeditFrame* parent = (WinEDA_LibeditFrame*) Panel->GetParent();

    if( parent == NULL )
        return;

    LibDrawPin* CurrentPin = (LibDrawPin*) parent->GetDrawItem();

    if( CurrentPin == NULL || CurrentPin->Type() != COMPONENT_PIN_DRAW_TYPE )
        return;

    if( CurrentPin->m_Flags & IS_NEW )
        delete CurrentPin;
    else
        CurrentPin->m_Flags = 0;

    /* clear edit flags */
    Panel->ManageCurseur = NULL;
    Panel->ForceCloseManageCurseur = NULL;
    parent->SetDrawItem( NULL );
    parent->SetLastDrawItem( NULL );
    Panel->Refresh( true );
}


/**
 * Managed cursor callback for placing component pins.
 */
void WinEDA_LibeditFrame::PlacePin( wxDC* DC )
{
    LibDrawPin* Pin;
    LibDrawPin* CurrentPin  = (LibDrawPin*) m_drawItem;
    bool        ask_for_pin = true;
    wxPoint     newpos;
    bool        status;

    if( CurrentPin == NULL )
        return;

    newpos.x = GetScreen()->m_Curseur.x;
    newpos.y = -GetScreen()->m_Curseur.y;

    // Tst for an other pin in same new position:
    for( Pin = m_component->GetNextPin(); Pin != NULL;
         Pin = m_component->GetNextPin( Pin ) )
    {
        if( Pin == CurrentPin || newpos != Pin->m_Pos || Pin->m_Flags )
            continue;

        if( ask_for_pin && !g_EditPinByPinIsOn )
        {
            DrawPanel->m_IgnoreMouseEvents = true;
            status = IsOK( this, _( "This position is already occupied by \
another pin. Continue?" ) );
            DrawPanel->MouseToCursorSchema();
            DrawPanel->m_IgnoreMouseEvents = false;
            if( !status )
                return;
            else
                ask_for_pin = false;
        }
    }

    DrawPanel->ManageCurseur = NULL;
    DrawPanel->ForceCloseManageCurseur = NULL;
    GetScreen()->SetModify();
    CurrentPin->m_Pos = newpos;

    if( CurrentPin->m_Flags & IS_NEW )
    {
        LastPinOrient = CurrentPin->m_Orient;
        LastPinType   = CurrentPin->m_PinType;
        LastPinShape  = CurrentPin->m_PinShape;
        CreateImagePins( CurrentPin, m_unit, m_convert, m_showDeMorgan );
        m_lastDrawItem = CurrentPin;
        m_component->AddDrawItem( m_drawItem );
    }

    /* Put linked pins in new position, and clear flags */
    for( Pin = m_component->GetNextPin(); Pin != NULL;
         Pin = m_component->GetNextPin( Pin ) )
    {
        if( Pin->m_Flags == 0 )
            continue;
        Pin->m_Pos   = CurrentPin->m_Pos;
        Pin->m_Flags = 0;
    }

    DrawPanel->CursorOff( DC );
    bool showPinText = true;
    CurrentPin->Draw( DrawPanel, DC, wxPoint( 0, 0 ), -1, GR_DEFAULT_DRAWMODE,
                      &showPinText, DefaultTransformMatrix );
    DrawPanel->CursorOn( DC );

    m_drawItem = NULL;
};


void WinEDA_PinPropertiesFrame::SetPinOrientation( int neworient )
{
    LibDrawPin* CurrentPin = (LibDrawPin*) m_Parent->GetDrawItem();
    LibDrawPin* Pin, * RefPin = CurrentPin;

    if( CurrentPin == NULL || CurrentPin->GetParent() == NULL || RefPin == NULL )
        return;

    m_Parent->GetScreen()->SetModify();

    /* Rotation */
    RefPin->m_Orient = neworient;

    Pin = CurrentPin->GetParent()->GetNextPin();

    for( ; Pin != NULL; Pin = CurrentPin->GetParent()->GetNextPin( Pin ) )
    {
        if( Pin->m_Flags == 0 )
            continue;
        Pin->m_Orient = RefPin->m_Orient;
        if( CurrentPin == NULL )
            Pin->m_Flags = 0;
    }
}


/**
 * Prepare le deplacement d'une pin :
 * Localise la pin pointee par le curseur, et si elle existe active
 * la fonction de gestion curseur ( DrawMovePin() ).
 */
void WinEDA_LibeditFrame::StartMovePin( wxDC* DC )
{
    LibDrawPin* Pin;
    LibDrawPin* CurrentPin = (LibDrawPin*) m_drawItem;
    wxPoint     startPos;

    /* Marquage des pins a traiter */
    Pin = m_component->GetNextPin();
    for( ; Pin != NULL; Pin = m_component->GetNextPin( Pin ) )
    {
        Pin->m_Flags = 0;
        if( Pin == CurrentPin )
            continue;
        if( ( Pin->m_Pos == CurrentPin->m_Pos )
            && ( Pin->m_Orient == CurrentPin->m_Orient )
            && ( g_EditPinByPinIsOn == false ) )
            Pin->m_Flags |= IS_LINKED | IS_MOVED;
    }

    CurrentPin->m_Flags |= IS_LINKED | IS_MOVED;
    PinPreviousPos = OldPos = CurrentPin->m_Pos;

    startPos.x = OldPos.x;
    startPos.y = -OldPos.y;
    DrawPanel->CursorOff( DC );
    GetScreen()->m_Curseur = startPos;
    DrawPanel->MouseToCursorSchema();

    CurrentPin->DisplayInfo( this );
    DrawPanel->ManageCurseur = DrawMovePin;
    DrawPanel->ForceCloseManageCurseur = AbortPinMove;

    DrawPanel->CursorOn( DC );
}


/******************************************************************************/
/* Routine de deplacement de la Pin courante selon position du curseur souris */
/* Routine normalement appelee par la routine de gestion du curseur          */
/******************************************************************************/
static void DrawMovePin( WinEDA_DrawPanel* panel, wxDC* DC, bool erase )
{
    WinEDA_LibeditFrame* parent = (WinEDA_LibeditFrame*) panel->GetParent();

    if( parent == NULL )
        return;

    LibDrawPin* CurrentPin = (LibDrawPin*) parent->GetDrawItem();

    if( CurrentPin == NULL || CurrentPin->Type() != COMPONENT_PIN_DRAW_TYPE )
        return;

    wxPoint pinpos = CurrentPin->m_Pos;
    bool    showPinText = true;

    /* Erase pin in old position */
    if( erase || ( CurrentPin->m_Flags & IS_NEW ) )
    {
        wxLogDebug( _( "Initial pin position (%d, %d)" ),
                    PinPreviousPos.x, PinPreviousPos.y );
        CurrentPin->m_Pos = PinPreviousPos;
        CurrentPin->Draw( panel, DC, wxPoint( 0, 0 ), -1, g_XorMode,
                          &showPinText, DefaultTransformMatrix );
    }

    /* Redraw pin in new position */
    CurrentPin->m_Pos.x = panel->GetScreen()->m_Curseur.x;
    CurrentPin->m_Pos.y = -panel->GetScreen()->m_Curseur.y;
    CurrentPin->Draw( panel, DC, wxPoint( 0, 0 ), -1, wxCOPY,
                      &showPinText, DefaultTransformMatrix );

    PinPreviousPos = CurrentPin->m_Pos;

    /* Keep the original position for existing pin (for Undo command)
     * and the current position for a new pin */
    if( ( CurrentPin->m_Flags & IS_NEW ) == 0 )
        CurrentPin->m_Pos = pinpos;
}


/**********************************************************/
void WinEDA_PinPropertiesFrame::SetPinShape( int newshape )
/**********************************************************/

/* Changement de la forme de la pin courante.
  * Le changement est egalement fait sur les autres pins correspondantes
  * des autres unites de la seule forme convert courante
 */
{
    LibDrawPin* Pin;
    LibDrawPin* CurrentPin = (LibDrawPin*) m_Parent->GetDrawItem();

    if( CurrentPin )
    {
        CurrentPin->m_PinShape = newshape;
        m_Parent->GetScreen()->SetModify();
        CurrentPin->DisplayInfo( m_Parent );

        Pin = CurrentPin->GetParent()->GetNextPin();
        for( ; Pin != NULL; Pin = CurrentPin->GetParent()->GetNextPin( Pin ) )
        {
            if( Pin->m_Flags == 0 || Pin->m_Convert != CurrentPin->m_Convert )
                continue;
            Pin->m_PinShape = newshape;
        }
    }
}


/******************************************************/
void WinEDA_PinPropertiesFrame::SetPinType( int newtype )
/******************************************************/

/* Changement du type electrique de la pin courante.
  * Le changement est egalement fait sur les autres pins correspondantes
  * des autres unites du boitier
 */
{
    LibDrawPin* Pin;
    LibDrawPin* CurrentPin = (LibDrawPin*) m_Parent->GetDrawItem();

    if( CurrentPin == NULL || CurrentPin->GetParent() == NULL )
        return;

    CurrentPin->m_PinType = newtype;
    m_Parent->GetScreen()->SetModify();

    Pin = CurrentPin->GetParent()->GetNextPin();
    for( ; Pin != NULL; Pin = CurrentPin->GetParent()->GetNextPin( Pin ) )
    {
        if( Pin->m_Flags == 0 )
            continue;
        Pin->m_PinType = newtype;
    }
}


/********************************************************************************/
void WinEDA_PinPropertiesFrame::SetPinName( const wxString& newname, int newsize )
/********************************************************************************/

/* Met a jour le nom et la taille de ce nom de la pin courante
  * si newname == NULL, pas de changement de nom
  * si newsize < 0 : pas de changement de taille
 */
{
    LibDrawPin* Pin;
    LibDrawPin* CurrentPin = (LibDrawPin*) m_Parent->GetDrawItem();
    wxString    buf;

    if( CurrentPin == NULL || CurrentPin->GetParent() == NULL )
        return;

    buf = newname;
    buf.Replace( wxT( " " ), wxT( "_" ) );

    if( newsize >= 0 )
        CurrentPin->m_PinNameSize = newsize;

    CurrentPin->m_PinName = buf;

    m_Parent->GetScreen()->SetModify();

    /* Traitement des autres pins */
    Pin = CurrentPin->GetParent()->GetNextPin();
    for( ; Pin != NULL; Pin = CurrentPin->GetParent()->GetNextPin( Pin ) )
    {
        if( (Pin->m_Flags & IS_LINKED) == 0 )
            continue;
        if( newsize >= 0 )
            Pin->m_PinNameSize = newsize;
        Pin->m_PinName = buf;
    }
}


/******************************************************************************/
void WinEDA_PinPropertiesFrame::SetPinNum( const wxString& newnum, int newsize )
/******************************************************************************/

/* Changement du numero de la pin courante.
  * Le changement est egalement fait sur les autres pins correspondantes
  * a la forme convertie
  * Si newnum == NULL: pas de changement de numero
  * Si newsize < 0 ) pase de changement de taille
 */
{
    LibDrawPin* Pin;
    LibDrawPin* CurrentPin = (LibDrawPin*) m_Parent->GetDrawItem();
    wxString    buf;

    buf = newnum;
    buf.Replace( wxT( " " ), wxT( "_" ) );

    if( CurrentPin == NULL || CurrentPin->GetParent() == NULL )
        return;

    CurrentPin->m_PinNum = 0;

    if( newsize >= 0 )
        CurrentPin->m_PinNumSize = newsize;
    CurrentPin->SetPinNumFromString( buf );
    m_Parent->GetScreen()->SetModify();

    Pin = CurrentPin->GetParent()->GetNextPin();
    for( ; Pin != NULL; Pin = CurrentPin->GetParent()->GetNextPin( Pin ) )
    {
        if( ( Pin->m_Flags & IS_LINKED ) == 0
            || Pin->m_Unit != CurrentPin->m_Unit )
            continue;
        if( newsize >= 0 )
            Pin->m_PinNumSize = newsize;
        Pin->m_PinNum = CurrentPin->m_PinNum;
    }
}


/*************************************************/
void WinEDA_LibeditFrame::DeletePin( wxDC*          DC,
                                     LIB_COMPONENT* LibEntry,
                                     LibDrawPin*    Pin )
/*************************************************/

/* Routine d'effacement de la pin pointee par la souris
  * Si g_EditPinByPinIsOn == false :
  *     toutes les pins de meme coordonnee seront effacees.
  * Sinon seule la pin de l'unite en convert courante sera effacee
 */
{
    LibDrawPin* tmp;
    wxPoint     PinPos;

    if( LibEntry == NULL || Pin == NULL )
        return;

    PinPos = Pin->m_Pos;
    LibEntry->RemoveDrawItem( (LIB_DRAW_ITEM*) Pin, DrawPanel, DC );

    /* Effacement des autres pins de meme coordonnees */
    if( g_EditPinByPinIsOn == false )
    {
        tmp = LibEntry->GetNextPin();

        while( tmp != NULL )
        {
            Pin = tmp;
            tmp = LibEntry->GetNextPin( Pin );

            if( Pin->m_Pos != PinPos )
                continue;

            LibEntry->RemoveDrawItem( (LIB_DRAW_ITEM*) Pin );
        }
    }

    GetScreen()->SetModify();
}


/*
 * Create a new pin.
 */
void WinEDA_LibeditFrame::CreatePin( wxDC* DC )
{
    LibDrawPin* CurrentPin;
    bool        showPinText = true;

    if( m_component == NULL )
        return;

    /* Effacement des flags */
    m_component->ClearStatus();

    CurrentPin = new LibDrawPin( m_component );

    m_drawItem = CurrentPin;

    if( CurrentPin == NULL || CurrentPin->Type() != COMPONENT_PIN_DRAW_TYPE )
        return;
    CurrentPin->m_Flags   = IS_NEW;
    CurrentPin->m_Unit    = m_unit;
    CurrentPin->m_Convert = m_convert;

    /* Marquage des pins a traiter */
    if( g_EditPinByPinIsOn == false )
        CurrentPin->m_Flags |= IS_LINKED;

    CurrentPin->m_Pos.x       = GetScreen()->m_Curseur.x;
    CurrentPin->m_Pos.y       = -GetScreen()->m_Curseur.y;
    CurrentPin->m_PinLen      = LastPinSize;
    CurrentPin->m_Orient      = LastPinOrient;
    CurrentPin->m_PinType     = LastPinType;
    CurrentPin->m_PinShape    = LastPinShape;
    CurrentPin->m_PinNameSize = LastPinNameSize;
    CurrentPin->m_PinNumSize  = LastPinNumSize;
    if( LastPinCommonConvert )
        CurrentPin->m_Convert = 0;
    else
        CurrentPin->m_Convert = m_convert;
    if( LastPinCommonUnit )
        CurrentPin->m_Unit = 0;
    else
        CurrentPin->m_Unit = m_unit;
    if( LastPinNoDraw )
        CurrentPin->m_Attributs |= PINNOTDRAW;
    else
        CurrentPin->m_Attributs &= ~PINNOTDRAW;

    if( DC )
        CurrentPin->Draw( DrawPanel, DC, wxPoint( 0, 0 ), -1, wxCOPY,
                          &showPinText, DefaultTransformMatrix );

    PinPreviousPos = CurrentPin->m_Pos;
    wxLogDebug( _( "Initial pin position (%d, %d)" ),
                PinPreviousPos.x, PinPreviousPos.y );
    DrawPanel->m_IgnoreMouseEvents = true;
    InstallPineditFrame( this, DC, wxPoint( -1, -1 ) );
    DrawPanel->MouseToCursorSchema();
    DrawPanel->m_IgnoreMouseEvents = false;
    DrawPanel->ManageCurseur = DrawMovePin;
    DrawPanel->ForceCloseManageCurseur = AbortPinMove;

    CurrentPin->DisplayInfo( this );
    GetScreen()->SetModify();
}


/*  si draw == true
 * - Ajuste le flag visible / invisible (.U.Pin.Flags bit 0 ) de la pin
 * editee
 *
 * si unit == true
 * - Modifie l'attribut Commun / Particulier U.Pin.Unit = 0 ou Num Unite
 * de la pin editee
 *
 * si convert == true
 * - Modifie l'attribut Commun / Particulier U.Pin.Convert = 0 ou Num Unite
 * de la pin editee
 *
 */
void WinEDA_PinPropertiesFrame::SetPinAttributes( bool draw, bool unit,
                                                  bool convert )
{
    LibDrawPin* tmp;
    LibDrawPin* Pin;
    LibDrawPin* CurrentPin = (LibDrawPin*) m_Parent->GetDrawItem();

    if( CurrentPin == NULL )
        return;

    m_Parent->GetScreen()->SetModify();

    if( unit )
    {
        if( LastPinCommonUnit )
            CurrentPin->m_Unit = 0;
        else
            CurrentPin->m_Unit = m_Parent->GetUnit();

        if( CurrentPin->m_Unit == 0 )
        {
            tmp = CurrentPin->GetParent()->GetNextPin();

            while( tmp != NULL )
            {
                Pin = tmp;
                tmp = CurrentPin->GetParent()->GetNextPin( Pin );

                if( Pin->m_Flags == 0 || Pin == CurrentPin )
                    continue;
                if( CurrentPin->m_Convert
                    && ( CurrentPin->m_Convert != Pin->m_Convert ) )
                    continue;
                if( CurrentPin->m_Pos != Pin->m_Pos )
                    continue;
                if( Pin->m_Orient != CurrentPin->m_Orient )
                    continue;

                CurrentPin->GetParent()->RemoveDrawItem( (LIB_DRAW_ITEM*) Pin );
            }
        }
    }   // end if unit

    if( convert )
    {
        if( LastPinCommonConvert )
            CurrentPin->m_Convert = 0;
        else
            CurrentPin->m_Convert = m_Parent->GetConvert();

        if( CurrentPin->m_Convert == 0 )    /* Effacement des pins redondantes */
        {
            tmp = CurrentPin->GetParent()->GetNextPin();

            while( tmp != NULL )
            {
                Pin = tmp;
                tmp = CurrentPin->GetParent()->GetNextPin( Pin );

                if( Pin->m_Flags == 0 )
                    continue;
                if( Pin == CurrentPin )
                    continue;
                if( CurrentPin->m_Unit && ( CurrentPin->m_Unit != Pin->m_Unit ) )
                    continue;
                if( CurrentPin->m_Pos != Pin->m_Pos )
                    continue;
                if( Pin->m_Orient != CurrentPin->m_Orient )
                    continue;

                CurrentPin->GetParent()->RemoveDrawItem( (LIB_DRAW_ITEM*) Pin );
            }
        }
    }       // end if convert

    if( draw )
    {
        if( LastPinNoDraw )
            CurrentPin->m_Attributs |= PINNOTDRAW;
        else
            CurrentPin->m_Attributs &= ~PINNOTDRAW;

        Pin = CurrentPin->GetParent()->GetNextPin();
        for( ; Pin != NULL; Pin = CurrentPin->GetParent()->GetNextPin( Pin ) )
        {
            if( Pin->m_Flags == 0 )
                continue;
            if( LastPinNoDraw )
                Pin->m_Attributs |= PINNOTDRAW;
            else
                Pin->m_Attributs &= ~PINNOTDRAW;
        }
    }
}


/******************************************************/
void WinEDA_PinPropertiesFrame::NewSizePin( int newsize )
/******************************************************/

/* Fonction permettant la mise aux dimensions courantes:
  * - longueur, dimension des textes
  * de la pin courante
 *
 */
{
    LibDrawPin* RefPin, * Pin = (LibDrawPin*) m_Parent->GetDrawItem();

    if( Pin == NULL || Pin->GetParent() == NULL )
        return;

    m_Parent->GetScreen()->SetModify();

    Pin->m_PinLen = newsize;

    Pin->DisplayInfo( m_Parent );

    RefPin = Pin;

    if( g_EditPinByPinIsOn == false )
    {
        Pin = Pin->GetParent()->GetNextPin();
        for( ; Pin != NULL; Pin = Pin->GetParent()->GetNextPin( Pin ) )
        {
            if( Pin->m_Pos != RefPin->m_Pos )
                continue;
            if( Pin->m_Orient != RefPin->m_Orient )
                continue;
            if( Pin->m_Convert == RefPin->m_Convert )
                Pin->m_PinLen = newsize;
        }
    }
}


static void CreateImagePins( LibDrawPin* Pin, int unit, int convert,
                             bool asDeMorgan )
{
    int         ii;
    LibDrawPin* NewPin;
    bool        CreateConv = false;


    if( g_EditPinByPinIsOn )
        return;

    if( asDeMorgan && ( Pin->m_Convert != 0 ) )
        CreateConv = true;

    /* Creation de la pin " convert " pour la part courante */
    if( CreateConv == true )
    {
        NewPin = (LibDrawPin*) Pin->GenCopy();
        if( Pin->m_Convert > 1 )
            NewPin->m_Convert = 1;
        else
            NewPin->m_Convert = 2;
        Pin->GetParent()->AddDrawItem( NewPin );
    }

    for( ii = 1; ii <= Pin->GetParent()->m_UnitCount; ii++ )
    {
        if( ii == unit || Pin->m_Unit == 0 )
            continue;                       /* Pin commune a toutes les unites */

        /* Creation pour la representation "normale" */
        NewPin = (LibDrawPin*) Pin->GenCopy();
        if( convert != 0 )
            NewPin->m_Convert = 1;
        NewPin->m_Unit = ii;
        Pin->GetParent()->AddDrawItem( NewPin );

        /* Creation pour la representation "Convert" */
        if( CreateConv == false )
            continue;

        NewPin = (LibDrawPin*)Pin->GenCopy();
        NewPin->m_Convert = 2;
        if( Pin->m_Unit != 0 )
            NewPin->m_Unit = ii;
        Pin->GetParent()->AddDrawItem( NewPin );
    }
}


/*************************************************/
void WinEDA_LibeditFrame::GlobalSetPins( wxDC* DC,
                                         LibDrawPin* MasterPin, int id )
/*************************************************/

/*  Depending on "id":
  * - Change pin text size (name or num) (range 10 .. 1000 mil)
  * - Change pin lenght.
 *
  * If Pin is selected ( .m_flag == IS_SELECTED ) only the other selected pis are modified
 */
{
    LibDrawPin* Pin;
    bool        selected = ( MasterPin->m_Selected & IS_SELECTED ) != 0;
    bool        showPinText = true;

    if( ( m_component == NULL ) || ( MasterPin == NULL ) )
        return;
    if( MasterPin->Type() != COMPONENT_PIN_DRAW_TYPE )
        return;

    GetScreen()->SetModify();

    Pin = m_component->GetNextPin();
    for( ; Pin != NULL; Pin = m_component->GetNextPin( Pin ) )
    {
        if( ( Pin->m_Convert ) && ( Pin->m_Convert != m_convert ) )
            continue;

        // Is it the "selected mode" ?
        if( selected && ( Pin->m_Selected & IS_SELECTED ) == 0 )
            continue;

        Pin->Draw( DrawPanel, DC, wxPoint( 0, 0 ), -1, g_XorMode,
                   &showPinText, DefaultTransformMatrix );

        switch( id )
        {
        case ID_POPUP_LIBEDIT_PIN_GLOBAL_CHANGE_PINNUMSIZE_ITEM:
            Pin->m_PinNumSize = MasterPin->m_PinNumSize;
            break;

        case ID_POPUP_LIBEDIT_PIN_GLOBAL_CHANGE_PINNAMESIZE_ITEM:
            Pin->m_PinNameSize = MasterPin->m_PinNameSize;
            break;

        case ID_POPUP_LIBEDIT_PIN_GLOBAL_CHANGE_PINSIZE_ITEM:
            Pin->m_PinLen = MasterPin->m_PinLen;
            break;
        }

        Pin->Draw( DrawPanel, DC, wxPoint( 0, 0 ), -1, GR_DEFAULT_DRAWMODE,
                   &showPinText, DefaultTransformMatrix );
    }
}


/************************************************************************/
void WinEDA_LibeditFrame::RepeatPinItem( wxDC* DC, LibDrawPin* SourcePin )
/************************************************************************/
/* Creation d'une nouvelle pin par copie de la pr�c�dente ( fct REPEAT) */
{
    LibDrawPin* Pin;
    wxString    msg;
    int         ox = 0, oy = 0;

    if( m_component == NULL || SourcePin == NULL
        || SourcePin->Type() != COMPONENT_PIN_DRAW_TYPE )
        return;

    Pin = (LibDrawPin*)SourcePin->GenCopy();
    m_component->AddDrawItem( Pin );

    Pin->m_Flags = IS_NEW;
    Pin->m_Pos.x += g_RepeatStep.x;
    ox = Pin->m_Pos.x;
    Pin->m_Pos.y += -g_RepeatStep.y;
    oy = Pin->m_Pos.y; // ici axe Y comme en math
    /*** Increment du numero de label ***/
    IncrementLabelMember( Pin->m_PinName );

    Pin->ReturnPinStringNum( msg );
    IncrementLabelMember( msg );
    Pin->SetPinNumFromString( msg );

    m_drawItem = Pin;

    /* Marquage des pins a traiter */
    if( g_EditPinByPinIsOn == false )
        Pin->m_Flags |= IS_LINKED;

    wxPoint savepos = GetScreen()->m_Curseur;
    DrawPanel->CursorOff( DC );
    GetScreen()->m_Curseur.x = Pin->m_Pos.x;
    GetScreen()->m_Curseur.y = -Pin->m_Pos.y;
    PlacePin( DC );
    GetScreen()->m_Curseur = savepos;

//  DrawPanel->MouseToCursorSchema();
    DrawPanel->CursorOn( DC );

    Pin->DisplayInfo( this );
    GetScreen()->SetModify();
}


int sort_by_pin_number( const void* ref, const void* tst )
{
    const LibDrawPin* Ref = *(LibDrawPin**) ref;
    const LibDrawPin* Tst = *(LibDrawPin**) tst;

    return Ref->m_PinNum - Tst->m_PinNum;
}


void WinEDA_LibeditFrame::OnCheckComponent( wxCommandEvent& event )
{
    int             nb_pins, ii, error;
    LibDrawPin*     Pin;
    LibDrawPin**    PinList;
    wxString        msg;

    if( m_component == NULL )
        return;

    // Construction de la liste des pins:
    Pin = m_component->GetNextPin();
    for( nb_pins = 0; Pin != NULL; Pin = m_component->GetNextPin( Pin ) )
    {
        nb_pins++;
    }

    PinList = (LibDrawPin**) MyZMalloc( (nb_pins + 1) * sizeof(LibDrawPin*) );
    Pin = m_component->GetNextPin();
    for( ii = 0; Pin != NULL; Pin = m_component->GetNextPin( Pin ) )
    {
        if( Pin->Type() == COMPONENT_PIN_DRAW_TYPE )
            PinList[ii++] = Pin;
    }

    // Classement des pins par numero de pin
    qsort( PinList, nb_pins, sizeof(LibDrawPin*), sort_by_pin_number );

    // Controle des duplicates:
    error = 0;
    for( ii = 1; ii < nb_pins; ii++ )
    {
        wxString    aux_msg, StringPinNum;
        LibDrawPin* curr_pin = PinList[ii];
        Pin = PinList[ii - 1];

        if( Pin->m_PinNum != curr_pin->m_PinNum
            || Pin->m_Convert != curr_pin->m_Convert
            || Pin->m_Unit != curr_pin->m_Unit )
            continue;

        error++;
        curr_pin->ReturnPinStringNum( StringPinNum );
        msg.Printf( _( "Duplicate pin %s at location (%d, %d) conflicts \
with pin %s at location (%d, %d)" ),
                    (const wxChar*) StringPinNum,
                    (const wxChar*) curr_pin->m_PinName,
                    curr_pin->m_Pos.x, -curr_pin->m_Pos.y,
                    (const wxChar*) Pin->m_PinName,
                    Pin->m_Pos.x, -Pin->m_Pos.y );

        if( m_component->m_UnitCount > 1 )
        {
            aux_msg.Printf( _( " in part %c" ), 'A' + curr_pin->m_Unit );
            msg += aux_msg;
        }

        if( m_showDeMorgan )
        {
            if( curr_pin->m_Convert )
                msg += _( "  of converted" );
            else
                msg += _( "  of normal" );
        }

        msg += wxT( "." );

        DisplayError( this, msg );
    }

    free( PinList );

    if( error == 0 )
        DisplayInfoMessage( this, _( "No duplicate pins were found." ) );
}
