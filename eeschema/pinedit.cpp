/***************************/
/*  EESchema - PinEdit.cpp */
/***************************/

#if defined (__GNUG__) && !defined (NO_GCC_PRAGMA)
#pragma implementation "pinedit-dialog.h"
#endif

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
static void CreateImagePins( LibDrawPin* Pin );
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

    if( CurrentDrawItem )                           // Set Pin Name & Num
    {
        if( !(CurrentDrawItem->m_Flags & IS_NEW) )  // if IS_NEW, copy for undo is done before place
            m_Parent->SaveCopyInUndoList( CurrentLibEntry );

        SetPinName( m_PinNameCtrl->GetValue(), LastPinNameSize );
        msg = m_PinNumCtrl->GetValue();
        if( msg.IsEmpty() )
            msg = wxT( "~" );
        SetPinNum( msg, LastPinNumSize );
        NewSizePin( LastPinSize );
        SetPinShape( LastPinShape );
        SetPinType( LastPinType );
        SetPinOrient( LastPinOrient );
        SetAttributsPin( true, true, true );    // Set all attributes (visibility, common to units and common to convert options)
        CurrentDrawItem->DisplayInfo( m_Parent );
    }

    m_Parent->DrawPanel->Refresh();
}


/*********************************************/
void WinEDA_LibeditFrame::InitEditOnePin()
/*********************************************/

/* Called when installing the edit pin dialog frame
 * Set pins flags (.m_Flags pins member) to ensure a correctins edition:
 * If 2 or more pins are on the same location (and the same orientation) they are all moved or resized.
 * This is usefull for components which have more than one part per package
 * In this case all parts can be edited at once.
 * Note: if the option "Edit Pin per Pin" (tool of the main toolbar) is activated, only the current part is edited
 */
{
    LibDrawPin* Pin;
    LibDrawPin* CurrentPin = (LibDrawPin*) CurrentDrawItem;

    if( CurrentLibEntry == NULL )
        return;
    if( CurrentPin == NULL )
        return;

    /* Marquage des pins a traiter,Si edition d'une pin non deja selectionnee */
    Pin = (LibDrawPin*) CurrentLibEntry->m_Drawings;
    for( ; Pin != NULL; Pin = Pin->Next() )
    {
        if( Pin->Type() != COMPONENT_PIN_DRAW_TYPE )
            continue;
        if( Pin == CurrentPin )
            continue;
        if( ( Pin->m_Pos == CurrentPin->m_Pos )
            && ( Pin->m_Orient == CurrentPin->m_Orient )
            && ( !( CurrentPin->m_Flags & IS_NEW ) )
            && ( g_EditPinByPinIsOn == false )     // This is set by the tool of the main toolbar
            )
            Pin->m_Flags |= IS_LINKED | IN_EDIT;
        else
            Pin->m_Flags = 0;
    }

    CurrentPin->DisplayInfo( this );
}


/*************************************************************/
static void AbortPinMove( WinEDA_DrawPanel* Panel, wxDC* DC )
/*************************************************************/

/* Used to abort the move pin command.
 */
{
    LibDrawPin* CurrentPin = (LibDrawPin*) CurrentDrawItem;

    if( CurrentPin  && ( CurrentPin->m_Flags & IS_NEW ) )
        CurrentLibEntry->RemoveDrawItem( CurrentPin, Panel, DC );

    /* clear edit flags */
    LibEDA_BaseStruct* item = CurrentLibEntry->m_Drawings;
    for( ; item != NULL; item = item->Next() )
        item->m_Flags = 0;

    Panel->ManageCurseur = NULL;
    Panel->ForceCloseManageCurseur = NULL;
    CurrentDrawItem = NULL;
    LibItemToRepeat = NULL;
    Panel->Refresh( true );
}


/********************************************/
void WinEDA_LibeditFrame::PlacePin( wxDC* DC )
/********************************************/
/* Routine de fin de deplacement de la pin selectionnee */
{
    LibDrawPin* Pin;
    LibDrawPin* CurrentPin  = (LibDrawPin*) CurrentDrawItem;
    bool        ask_for_pin = true;
    wxPoint     newpos;
    bool        status;

    if( CurrentPin == NULL )
        return;

    newpos.x = GetScreen()->m_Curseur.x;
    newpos.y = -GetScreen()->m_Curseur.y;

    Pin = (LibDrawPin*) CurrentLibEntry->m_Drawings;

    // Tst for an other pin in same new position:
    for( ; Pin != NULL; Pin = Pin->Next() )
    {
        if( Pin->Type() != COMPONENT_PIN_DRAW_TYPE )
            continue;
        if( Pin == CurrentPin )
            continue;
        if( newpos != Pin->m_Pos )
            continue;
        if( Pin->m_Flags )
            continue;
        if( ask_for_pin && !g_EditPinByPinIsOn )
        {
            DrawPanel->m_IgnoreMouseEvents = true;
            status = IsOK( this, _( "Occupied by other pin. Continue?" ) );
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
        CreateImagePins( CurrentPin );
        LibItemToRepeat = CurrentPin;
    }

    /* Put linked pins in new position, and clear flags */
    Pin = (LibDrawPin*) CurrentLibEntry->m_Drawings;
    for( ; Pin != NULL; Pin = Pin->Next() )
    {
        if( Pin->Type() != COMPONENT_PIN_DRAW_TYPE )
            continue;
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

    CurrentDrawItem = NULL;
};

/***********************************************************/
void WinEDA_PinPropertiesFrame::SetPinOrient( int neworient )
/***********************************************************/
/* Routine de Rotation de la pin courante*/
{
    LibDrawPin* CurrentPin = (LibDrawPin*) CurrentDrawItem;
    LibDrawPin* Pin, * RefPin = CurrentPin;

    if( CurrentLibEntry == NULL )
        return;
    if( RefPin == NULL )
        return;

    m_Parent->GetScreen()->SetModify();

    /* Rotation */
    RefPin->m_Orient = neworient;

    Pin = (LibDrawPin*) CurrentLibEntry->m_Drawings;
    for( ; Pin != NULL; Pin = Pin->Next() )
    {
        if( Pin->m_Flags == 0 )
            continue;
        Pin->m_Orient = RefPin->m_Orient;
        if( CurrentPin == NULL )
            Pin->m_Flags = 0;
    }
}


/*************************************************/
void WinEDA_LibeditFrame::StartMovePin( wxDC* DC )
/*************************************************/

/* Prepare le deplacement d'une pin :
  * Localise la pin pointee par le curseur, et si elle existe active
  * la fonction de gestion curseur ( DrawMovePin() ).
 */
{
    LibDrawPin* Pin;
    LibDrawPin* CurrentPin = (LibDrawPin*) CurrentDrawItem;
    wxPoint     startPos;

    /* Marquage des pins a traiter */
    Pin = (LibDrawPin*) CurrentLibEntry->m_Drawings;
    for( ; Pin != NULL; Pin = Pin->Next() )
    {
        Pin->m_Flags = 0;
        if( Pin->Type() != COMPONENT_PIN_DRAW_TYPE )
            continue;
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
    LibDrawPin* CurrentPin = (LibDrawPin*) CurrentDrawItem;
    wxPoint     pinpos = CurrentPin->m_Pos;
    bool        showPinText = true;

    /* Erase pin in old position */
    if( erase || ( CurrentPin->m_Flags & IS_NEW ) )
    {
        CurrentPin->m_Pos = PinPreviousPos;
        CurrentPin->Draw( panel, DC, wxPoint( 0, 0 ), -1, g_XorMode,
                          &showPinText, DefaultTransformMatrix );
    }

    /* Redraw pin in new position */
    CurrentPin->m_Pos.x = panel->GetScreen()->m_Curseur.x;
    CurrentPin->m_Pos.y = -panel->GetScreen()->m_Curseur.y;
    CurrentPin->Draw( panel, DC, wxPoint( 0, 0 ), -1, g_XorMode,
                      &showPinText, DefaultTransformMatrix );

    PinPreviousPos = CurrentPin->m_Pos;

    /* Keep the original position for existing pin (for Undo command)
      * and the current position for a new pin */
    if( (CurrentPin->m_Flags & IS_NEW) == 0 )
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
    LibDrawPin* CurrentPin = (LibDrawPin*) CurrentDrawItem;

    if( CurrentPin )
    {
        CurrentPin->m_PinShape = newshape;
        m_Parent->GetScreen()->SetModify();
        CurrentPin->DisplayInfo( m_Parent );

        Pin = (LibDrawPin*) CurrentLibEntry->m_Drawings;
        for( ; Pin != NULL; Pin = Pin->Next() )
        {
            if( Pin->Type() != COMPONENT_PIN_DRAW_TYPE )
                continue;
            if( Pin->m_Flags == 0 )
                continue;
            if( Pin->m_Convert != CurrentPin->m_Convert )
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
    LibDrawPin* CurrentPin = (LibDrawPin*) CurrentDrawItem;

    if( CurrentPin == NULL )
        return;

    CurrentPin->m_PinType = newtype;
    m_Parent->GetScreen()->SetModify();

    Pin = (LibDrawPin*) CurrentLibEntry->m_Drawings;
    for( ; Pin != NULL; Pin = Pin->Next() )
    {
        if( Pin->Type() != COMPONENT_PIN_DRAW_TYPE )
            continue;
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
    LibDrawPin* CurrentPin = (LibDrawPin*) CurrentDrawItem;
    wxString    buf;

    buf = newname;
    buf.Replace( wxT( " " ), wxT( "_" ) );

    if( newsize >= 0 )
        CurrentPin->m_PinNameSize = newsize;

    CurrentPin->m_PinName = buf;

    m_Parent->GetScreen()->SetModify();

    /* Traitement des autres pins */
    Pin = (LibDrawPin*) CurrentLibEntry->m_Drawings;
    for( ; Pin != NULL; Pin = Pin->Next() )
    {
        if( Pin->Type() != COMPONENT_PIN_DRAW_TYPE )
            continue;
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
    LibDrawPin* CurrentPin = (LibDrawPin*) CurrentDrawItem;
    wxString    buf;

    buf = newnum;
    buf.Replace( wxT( " " ), wxT( "_" ) );

    if( CurrentPin == NULL )
        return;

    CurrentPin->m_PinNum = 0;

    if( newsize >= 0 )
        CurrentPin->m_PinNumSize = newsize;
    CurrentPin->SetPinNumFromString( buf );
    m_Parent->GetScreen()->SetModify();

    Pin = (LibDrawPin*) CurrentLibEntry->m_Drawings;
    for( ; Pin != NULL; Pin = Pin->Next() )
    {
        if( Pin->Type() != COMPONENT_PIN_DRAW_TYPE )
            continue;
        if( ( Pin->m_Flags & IS_LINKED ) == 0 )
            continue;
        if( Pin->m_Unit != CurrentPin->m_Unit )
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
    LibEDA_BaseStruct* DrawItem;
    wxPoint            PinPos;

    if( LibEntry == NULL || Pin == NULL )
        return;

    PinPos = Pin->m_Pos;
    LibEntry->RemoveDrawItem( (LibEDA_BaseStruct*) Pin, DrawPanel, DC );

    /* Effacement des autres pins de meme coordonnees */
    if( g_EditPinByPinIsOn == false )
    {
        DrawItem = LibEntry->m_Drawings;
        for( ; DrawItem != NULL; )
        {
            if( DrawItem->Type() != COMPONENT_PIN_DRAW_TYPE )
            {
                DrawItem = DrawItem->Next();
                continue;
            }
            Pin      = (LibDrawPin*) DrawItem;
            DrawItem = DrawItem->Next();
            if( Pin->m_Pos != PinPos )
                continue;
            LibEntry->RemoveDrawItem( (LibEDA_BaseStruct*) Pin );
        }
    }
    GetScreen()->SetModify();
}


/*********************************************/
void WinEDA_LibeditFrame::CreatePin( wxDC* DC )
/*********************************************/
/* Creation d'une nouvelle pin */
{
    LibEDA_BaseStruct* DrawItem;
    LibDrawPin*        CurrentPin;
    bool               showPinText = true;

    if( CurrentLibEntry == NULL )
        return;

    /* Effacement des flags */
    DrawItem = CurrentLibEntry->m_Drawings;
    for( ; DrawItem != NULL; DrawItem = DrawItem->Next() )
        DrawItem->m_Flags = 0;

    CurrentPin = new LibDrawPin(CurrentLibEntry);

    CurrentDrawItem = CurrentPin;
    if( CurrentPin == NULL )
        return;
    CurrentPin->m_Flags   = IS_NEW;
    CurrentPin->m_Unit    = CurrentUnit;
    CurrentPin->m_Convert = CurrentConvert;

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
        CurrentPin->m_Convert = CurrentConvert;
    if( LastPinCommonUnit )
        CurrentPin->m_Unit = 0;
    else
        CurrentPin->m_Unit = CurrentUnit;
    if( LastPinNoDraw )
        CurrentPin->m_Attributs |= PINNOTDRAW;
    else
        CurrentPin->m_Attributs &= ~PINNOTDRAW;

    CurrentPin->SetNext( CurrentLibEntry->m_Drawings );
    CurrentLibEntry->m_Drawings = CurrentPin;
    CurrentLibEntry->SortDrawItems();

    if( DC )
        CurrentPin->Draw( DrawPanel, DC, wxPoint( 0, 0 ), -1, g_XorMode,
                          &showPinText, DefaultTransformMatrix );

    DrawPanel->m_IgnoreMouseEvents = true;
    InstallPineditFrame( this, DC, wxPoint( -1, -1 ) );
    DrawPanel->MouseToCursorSchema();
    DrawPanel->m_IgnoreMouseEvents = false;

    PinPreviousPos = CurrentPin->m_Pos;

    DrawPanel->ManageCurseur = DrawMovePin;
    DrawPanel->ForceCloseManageCurseur = AbortPinMove;

    CurrentPin->DisplayInfo( this );
    GetScreen()->SetModify();
}


/*********************************************************/
void WinEDA_PinPropertiesFrame::SetAttributsPin( bool draw,
                                                 bool unit, bool convert )
/*********************************************************/

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
{
    LibEDA_BaseStruct* DrawItem;
    LibDrawPin*        Pin, * CurrentPin = (LibDrawPin*) CurrentDrawItem;

    if( CurrentPin == NULL )
        return;

    m_Parent->GetScreen()->SetModify();

    if( unit  )
    {
        if( LastPinCommonUnit )
            CurrentPin->m_Unit = 0;
        else
            CurrentPin->m_Unit = CurrentUnit;

        Pin = (LibDrawPin*) CurrentLibEntry->m_Drawings;

        if( CurrentPin->m_Unit == 0 )
        {
            DrawItem = CurrentLibEntry->m_Drawings;
            for( ; DrawItem != NULL; )
            {
                Pin      = (LibDrawPin*) DrawItem;
                DrawItem = DrawItem->Next();
                if( Pin->Type() != COMPONENT_PIN_DRAW_TYPE )
                    continue;
                if( Pin->m_Flags == 0 )
                    continue;
                if( Pin == CurrentPin )
                    continue;
                if( CurrentPin->m_Convert
                    && (CurrentPin->m_Convert != Pin->m_Convert) )
                    continue;
                if( CurrentPin->m_Pos != Pin->m_Pos )
                    continue;
                if( Pin->m_Orient != CurrentPin->m_Orient )
                    continue;

                CurrentLibEntry->RemoveDrawItem( (LibEDA_BaseStruct*) Pin );
            }
        }
    }   // end if unit

    if( convert )
    {
        if( LastPinCommonConvert )
            CurrentPin->m_Convert = 0;
        else
            CurrentPin->m_Convert = CurrentConvert;

        if( CurrentPin->m_Convert == 0 )    /* Effacement des pins redondantes */
        {
            DrawItem = CurrentLibEntry->m_Drawings;
            for( ; DrawItem != NULL; )
            {
                Pin = (LibDrawPin*) DrawItem;
                DrawItem = DrawItem->Next();
                if( Pin->Type() != COMPONENT_PIN_DRAW_TYPE )
                    continue;
                if( Pin->m_Flags == 0 )
                    continue;
                if( Pin == CurrentPin )
                    continue;
                if( CurrentPin->m_Unit && (CurrentPin->m_Unit != Pin->m_Unit) )
                    continue;
                if( CurrentPin->m_Pos != Pin->m_Pos )
                    continue;
                if( Pin->m_Orient != CurrentPin->m_Orient )
                    continue;

                CurrentLibEntry->RemoveDrawItem( (LibEDA_BaseStruct*) Pin );
            }
        }
    }       // end if convert

    if( draw )
    {
        if( LastPinNoDraw )
            CurrentPin->m_Attributs |= PINNOTDRAW;
        else
            CurrentPin->m_Attributs &= ~PINNOTDRAW;

        Pin = (LibDrawPin*) CurrentLibEntry->m_Drawings;
        for( ; Pin != NULL; Pin = Pin->Next() )
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
    LibDrawPin* RefPin, * Pin = (LibDrawPin*) CurrentDrawItem;

    if( CurrentLibEntry == NULL )
        return;
    if( Pin == NULL )
        return;

    m_Parent->GetScreen()->SetModify();

    Pin->m_PinLen = newsize;

    Pin->DisplayInfo( m_Parent );

    RefPin = Pin;

    if( g_EditPinByPinIsOn == false )
    {
        Pin = (LibDrawPin*) CurrentLibEntry->m_Drawings;
        for( ; Pin != NULL; Pin = Pin->Next() )
        {
            if( Pin->Type() != COMPONENT_PIN_DRAW_TYPE )
                continue;
            if( Pin->m_Pos != RefPin->m_Pos )
                continue;
            if( Pin->m_Orient != RefPin->m_Orient )
                continue;
            if( Pin->m_Convert == RefPin->m_Convert )
                Pin->m_PinLen = newsize;
        }
    }
}


/********************************************/
static void CreateImagePins( LibDrawPin* Pin )
/********************************************/

/* Creation des autres pins pour les autres unites et pour convert, apres
  * creation d'une pin
 */
{
    int         ii;
    LibDrawPin* NewPin;
    bool        CreateConv = false;

    if( g_EditPinByPinIsOn )
        return;

    if( g_AsDeMorgan && (Pin->m_Convert != 0 ) )
        CreateConv = true;

    /* Creation de la pin " convert " pour la part courante */
    if( CreateConv == true )
    {
        NewPin = (LibDrawPin*) Pin->GenCopy();
        if( Pin->m_Convert > 1 )
            NewPin->m_Convert = 1;
        else
            NewPin->m_Convert = 2;
        NewPin->SetNext( CurrentLibEntry->m_Drawings );
        CurrentLibEntry->m_Drawings = NewPin;
    }

    for( ii = 1; ii <= CurrentLibEntry->m_UnitCount; ii++ )
    {
        if( ii == CurrentUnit )
            continue;                       /* Deja fait */
        if( Pin->m_Unit == 0 )
            continue;                       /* Pin commune a toutes les unites */

        /* Creation pour la representation "normale" */
        NewPin = (LibDrawPin*) Pin->GenCopy();
        if( CurrentConvert != 0 )
            NewPin->m_Convert = 1;
        NewPin->m_Unit = ii;
        NewPin->SetNext( CurrentLibEntry->m_Drawings );
        CurrentLibEntry->m_Drawings = NewPin;

        /* Creation pour la representation "Convert" */
        if( CreateConv == false )
            continue;

        NewPin = (LibDrawPin*)Pin->GenCopy();
        NewPin->m_Convert = 2;
        if( Pin->m_Unit != 0 )
            NewPin->m_Unit = ii;
        NewPin->SetNext( CurrentLibEntry->m_Drawings );
        CurrentLibEntry->m_Drawings = NewPin;
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

    if( ( CurrentLibEntry == NULL ) || ( MasterPin == NULL ) )
        return;
    if( MasterPin->Type() != COMPONENT_PIN_DRAW_TYPE )
        return;

    GetScreen()->SetModify();

    Pin = (LibDrawPin*) CurrentLibEntry->m_Drawings;
    for( ; Pin != NULL; Pin = Pin->Next() )
    {
        if( Pin->Type() != COMPONENT_PIN_DRAW_TYPE )
            continue;
        if( ( Pin->m_Convert ) && ( Pin->m_Convert != CurrentConvert ) )
            continue;

        // Is it the "selected mode" ?
        if( selected && (Pin->m_Selected & IS_SELECTED) == 0 )
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

    if( CurrentLibEntry == NULL )
        return;
    if( SourcePin == NULL )
        return;
    if( SourcePin->Type() != COMPONENT_PIN_DRAW_TYPE )
        return;

    Pin = (LibDrawPin*)SourcePin->GenCopy();
    Pin->SetNext( CurrentLibEntry->m_Drawings );
    CurrentLibEntry->m_Drawings = Pin;
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

    CurrentDrawItem = Pin;

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


/***************************************************************/
bool WinEDA_LibeditFrame::TestPins( LIB_COMPONENT* LibEntry )
/***************************************************************/

// Test des pins ( duplicates...)
{
    int             nb_pins, ii, error;
    LibDrawPin*     Pin;
    LibDrawPin**    PinList;
    wxString        msg;

    if( CurrentLibEntry == NULL )
        return false;

    // Construction de la liste des pins:
    Pin = (LibDrawPin*) CurrentLibEntry->m_Drawings;
    for( nb_pins = 0; Pin != NULL; Pin = Pin->Next() )
    {
        if( Pin->Type() == COMPONENT_PIN_DRAW_TYPE )
            nb_pins++;
    }

    PinList = (LibDrawPin**) MyZMalloc( (nb_pins + 1) * sizeof(LibDrawPin*) );
    Pin = (LibDrawPin*) CurrentLibEntry->m_Drawings;
    for( ii = 0; Pin != NULL; Pin = Pin->Next() )
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

        if( Pin->m_PinNum != curr_pin->m_PinNum )
            continue;

        if( Pin->m_Convert != curr_pin->m_Convert )
            continue;

        if( Pin->m_Unit != curr_pin->m_Unit )
            continue;

        error++;
        curr_pin->ReturnPinStringNum( StringPinNum );
        msg.Printf( _( "Duplicate Pin %4.4s (Pin %s loc %d, %d, and Pin %s loc %d, %d)" ),
                    StringPinNum.GetData(), curr_pin->m_PinName.GetData(),
                    curr_pin->m_Pos.x, -curr_pin->m_Pos.y,
                    Pin->m_PinName.GetData(), Pin->m_Pos.x, -Pin->m_Pos.y );

        if( CurrentLibEntry->m_UnitCount > 1 )
        {
            aux_msg.Printf( _( "  Part %d" ), curr_pin->m_Unit );
            msg += aux_msg;
        }

        if( g_AsDeMorgan )
        {
            if( curr_pin->m_Convert )
                msg += _( "  Convert" );
            else
                msg += _( "  Normal" );
        }

        DisplayError( this, msg );
    }

    free( PinList );
    return error ? true : false;
}
