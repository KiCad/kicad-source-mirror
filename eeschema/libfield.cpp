/*********************************************************************/
/*	EESchema - edition des librairies: Edition des champs ( Fields ) */
/*********************************************************************/

    /*	Fichier libfield.cpp	*/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"

#include "protos.h"

#include "wx/spinctrl.h"

/* Routines locales */
static void ShowMoveField(WinEDA_DrawPanel * panel, wxDC *DC, bool erase);

/* Variables locales */

extern int CurrentUnit;
static wxPoint StartCursor, LastTextPosition;


/***********************************************************/
static void ExitMoveField(WinEDA_DrawPanel * Panel, wxDC * DC)
/***********************************************************/
{

    Panel->ManageCurseur = NULL;
    Panel->ForceCloseManageCurseur = NULL;
    if(CurrentDrawItem == NULL) return;

    wxPoint curpos;
    curpos = Panel->GetScreen()->m_Curseur;
    Panel->GetScreen()->m_Curseur = StartCursor;
    ShowMoveField(Panel, DC, TRUE);
    Panel->GetScreen()->m_Curseur = curpos;
    CurrentDrawItem->m_Flags = 0;

    CurrentDrawItem = NULL;
}



/****************************************************************************/
void WinEDA_LibeditFrame::StartMoveField(wxDC * DC, LibDrawField *field)
/****************************************************************************/
/* Initialise le deplacement d'un champ ( ref ou Name) */
{
wxPoint startPos;

    if( (CurrentLibEntry == NULL) || ( field == NULL ) ) return;
    CurrentDrawItem = field;
    LastTextPosition = field->m_Pos;
    CurrentDrawItem->m_Flags |= IS_MOVED;

    startPos.x = LastTextPosition.x;
    startPos.y = -LastTextPosition.y;
    DrawPanel->CursorOff(DC);
    GetScreen()->m_Curseur = startPos;
    DrawPanel->MouseToCursorSchema();

    DrawPanel->ManageCurseur = ShowMoveField;
    DrawPanel->ForceCloseManageCurseur = ExitMoveField;
    DrawPanel->ManageCurseur(DrawPanel, DC, TRUE);
    StartCursor = GetScreen()->m_Curseur;

    DrawPanel->CursorOn(DC);
}

/*****************************************************************/
/* Routine d'affichage du texte 'Field' en cours de deplacement. */
/*	Routine normalement attachee au curseur						*/
/*****************************************************************/
static void ShowMoveField(WinEDA_DrawPanel * panel, wxDC *DC, bool erase)
{
int color;
LibDrawField *Field = (LibDrawField *)CurrentDrawItem;

    if( (CurrentLibEntry == NULL) || (Field == NULL) ) return;

    GRSetDrawMode(DC, g_XorMode);

    switch (Field->m_FieldId)
        {
        case VALUE:
            color = ReturnLayerColor(LAYER_VALUEPART);
            break;

        case REFERENCE:
            color = ReturnLayerColor(LAYER_REFERENCEPART);
            break;

        default:
            color = ReturnLayerColor(LAYER_FIELDS);
            break;
        }

int LineWidth = MAX(Field->m_Width, g_DrawMinimunLineWidth);
    if( Field->m_Attributs & TEXT_NO_VISIBLE ) color = DARKGRAY;
    if( erase )
        DrawGraphicText(panel, DC,
                wxPoint(LastTextPosition.x, - LastTextPosition.y),
                color, Field->m_Text,
                Field->m_Orient ? TEXT_ORIENT_VERT : TEXT_ORIENT_HORIZ,
                Field->m_Size,
                Field->m_HJustify, Field->m_VJustify, LineWidth);


    LastTextPosition.x = panel->GetScreen()->m_Curseur.x;
    LastTextPosition.y = - panel->GetScreen()->m_Curseur.y;

    Field->m_Pos = LastTextPosition;

    DrawGraphicText(panel, DC,
            wxPoint(LastTextPosition.x, - LastTextPosition.y),
            color, Field->m_Text,
            Field->m_Orient ? TEXT_ORIENT_VERT : TEXT_ORIENT_HORIZ,
            Field->m_Size,
            Field->m_HJustify, Field->m_VJustify, LineWidth);
}

/*******************************************************************/
void WinEDA_LibeditFrame::PlaceField(wxDC * DC, LibDrawField *Field)
/*******************************************************************/
{
int color;

    if(Field == NULL ) return;

    switch (Field->m_FieldId)
    {
        case REFERENCE:
            color = ReturnLayerColor(LAYER_REFERENCEPART);
            break;

        case VALUE:
            color = ReturnLayerColor(LAYER_VALUEPART);
            break;

        default:
            color = ReturnLayerColor(LAYER_FIELDS);
            break;
    }

    Field->m_Flags = 0;


    if( (Field->m_Attributs & TEXT_NO_VISIBLE) != 0 ) color = DARKGRAY;
    Field->m_Pos.x = GetScreen()->m_Curseur.x;
    Field->m_Pos.y = - GetScreen()->m_Curseur.y;
int LineWidth = MAX(Field->m_Width, g_DrawMinimunLineWidth);
    DrawPanel->CursorOff(DC);

    GRSetDrawMode(DC, GR_DEFAULT_DRAWMODE);
    DrawGraphicText(DrawPanel, DC, wxPoint(Field->m_Pos.x, - Field->m_Pos.y),
                    color, Field->m_Text,
                    Field->m_Orient ? TEXT_ORIENT_VERT : TEXT_ORIENT_HORIZ,
                    Field->m_Size,
                    Field->m_HJustify, Field->m_VJustify, LineWidth);

    DrawPanel->CursorOn(DC);

    GetScreen()->SetModify();
    DrawPanel->ManageCurseur = NULL;
    DrawPanel->ForceCloseManageCurseur = NULL;
    CurrentDrawItem = NULL;
}


/******************************************************************/
void WinEDA_LibeditFrame::EditField(wxDC * DC, LibDrawField *Field)
/******************************************************************/
{
wxString Text;
int color;
wxString title = wxT("Text:");

    if( Field == NULL) return;

    switch (Field->m_FieldId)
    {
        case REFERENCE:
            title = wxT("Reference:");
            color = ReturnLayerColor(LAYER_REFERENCEPART);
            break;

        case VALUE:
            title = wxT("Value:");
            color = ReturnLayerColor(LAYER_VALUEPART);
            break;

        default:
            color = ReturnLayerColor(LAYER_FIELDS);
            break;
    }

    if( Field->m_Attributs & TEXT_NO_VISIBLE ) color = DARKGRAY;

    Text = Field->m_Text;
    Get_Message(title,Text, this);
    Text.Replace( wxT(" ") , wxT("_") );

    GRSetDrawMode(DC, g_XorMode);
int LineWidth = MAX(Field->m_Width, g_DrawMinimunLineWidth);
    DrawGraphicText(DrawPanel, DC, wxPoint(Field->m_Pos.x, - Field->m_Pos.y),
                    color, Field->m_Text,
                    Field->m_Orient ? TEXT_ORIENT_VERT : TEXT_ORIENT_HORIZ,
                    Field->m_Size,
                    Field->m_HJustify, Field->m_VJustify, LineWidth);

    if( ! Text.IsEmpty() )
    {
        SaveCopyInUndoList(CurrentLibEntry);
        Field->m_Text = Text;
    }
    else DisplayError(this, _("No new text: no change") );

    if( Field->m_Flags == 0 ) GRSetDrawMode(DC, GR_DEFAULT_DRAWMODE);

    DrawGraphicText(DrawPanel, DC, wxPoint(Field->m_Pos.x, - Field->m_Pos.y),
                    color, Field->m_Text,
                    Field->m_Orient ? TEXT_ORIENT_VERT : TEXT_ORIENT_HORIZ,
                    Field->m_Size,
                    Field->m_HJustify, Field->m_VJustify, LineWidth);

    GetScreen()->SetModify();

    if ( Field->m_FieldId == VALUE ) ReCreateHToolbar();
}

/********************************************************************/
void WinEDA_LibeditFrame::RotateField(wxDC * DC, LibDrawField *Field)
/********************************************************************/
/* Routine de modification de l'orientation ( Horiz ou Vert. ) du champ.
    si un champ est en cours d'edition, modif de celui ci.
    sinon Modif du champ pointe par la souris
*/
{
int color;

    if( Field == NULL) return;

    GetScreen()->SetModify();
    switch (Field->m_FieldId)
        {
        case REFERENCE:
            color = ReturnLayerColor(LAYER_REFERENCEPART);
            break;

        case VALUE:
            color = ReturnLayerColor(LAYER_VALUEPART);
            break;

        default:
            color = ReturnLayerColor(LAYER_FIELDS);
            break;
        }

    if( (Field->m_Attributs & TEXT_NO_VISIBLE) != 0  ) color = DARKGRAY;

    DrawPanel->CursorOff(DC);

    GRSetDrawMode(DC, g_XorMode);
int LineWidth = MAX(Field->m_Width, g_DrawMinimunLineWidth);
    DrawGraphicText(DrawPanel, DC, wxPoint(Field->m_Pos.x, - Field->m_Pos.y),
                    color, Field->m_Text,
                    Field->m_Orient ? TEXT_ORIENT_VERT : TEXT_ORIENT_HORIZ,
                    Field->m_Size,
                    Field->m_HJustify, Field->m_VJustify, LineWidth);

    if( Field->m_Orient) Field->m_Orient = 0;
    else Field->m_Orient = 1;

    if( Field->m_Flags == 0 ) GRSetDrawMode(DC, GR_DEFAULT_DRAWMODE);

    DrawGraphicText(DrawPanel, DC, wxPoint(Field->m_Pos.x, - Field->m_Pos.y),
                    color, Field->m_Text,
                    Field->m_Orient ? TEXT_ORIENT_VERT : TEXT_ORIENT_HORIZ,
                    Field->m_Size,
                    Field->m_HJustify, Field->m_VJustify, LineWidth);
    DrawPanel->CursorOn(DC);
}

/****************************************************************************/
LibDrawField * WinEDA_LibeditFrame::LocateField(EDA_LibComponentStruct *LibEntry)
/****************************************************************************/
/* Localise le champ (ref ou name) pointe par la souris
     retourne:
        pointeur sur le champ (NULL= Pas de champ)
*/
{
int x0, y0, x1, y1;	/* Rectangle d'encadrement des textes a localiser */
int dx, dy;			/* Dimensions du texte */
LibDrawField *Field;
int hjustify, vjustify;

    /* Localisation du Nom */
    x0 = LibEntry->m_Name.m_Pos.x;
    y0 = - LibEntry->m_Name.m_Pos.y;
    dx = LibEntry->m_Name.m_Size.x * LibEntry->m_Name.m_Text.Len(),
    dy = LibEntry->m_Name.m_Size.y;
    hjustify = LibEntry->m_Name.m_HJustify; vjustify = LibEntry->m_Name.m_VJustify;
    if (LibEntry->m_Name.m_Orient) EXCHG(dx, dy);
    if ( hjustify == GR_TEXT_HJUSTIFY_CENTER ) x0 -= dx/2;
    else if ( hjustify == GR_TEXT_HJUSTIFY_RIGHT ) x0 -= dx;
    if ( vjustify == GR_TEXT_VJUSTIFY_CENTER ) y0 -= dy/2;
    else if ( vjustify == GR_TEXT_VJUSTIFY_BOTTOM ) y0 += dy;
    x1 = x0 + dx; y1 = y0 + dy;

    if( (GetScreen()->m_Curseur.x >= x0) && ( GetScreen()->m_Curseur.x <= x1) &&
            (GetScreen()->m_Curseur.y >= y0) && ( GetScreen()->m_Curseur.y <= y1) )
        return &LibEntry->m_Name;

    /* Localisation du Prefix */
    x0 = LibEntry->m_Prefix.m_Pos.x;
    y0 = - LibEntry->m_Prefix.m_Pos.y;
    dx = LibEntry->m_Prefix.m_Size.x *LibEntry->m_Prefix.m_Text.Len(),
    dy = LibEntry->m_Prefix.m_Size.y;
    hjustify = LibEntry->m_Prefix.m_HJustify; vjustify = LibEntry->m_Prefix.m_VJustify;
    if (LibEntry->m_Prefix.m_Orient) EXCHG(dx, dy);
    if ( hjustify == GR_TEXT_HJUSTIFY_CENTER ) x0 -= dx/2;
    else if ( hjustify == GR_TEXT_HJUSTIFY_RIGHT ) x0 -= dx;
    if ( vjustify == GR_TEXT_VJUSTIFY_CENTER ) y0 -= dy/2;
    else if ( vjustify == GR_TEXT_VJUSTIFY_BOTTOM ) y0 -= dy;
    x1 = x0 + dx; y1 = y0 + dy;

    if( (GetScreen()->m_Curseur.x >= x0) && ( GetScreen()->m_Curseur.x <= x1) &&
        (GetScreen()->m_Curseur.y >= y0) && ( GetScreen()->m_Curseur.y <= y1) )
        return &LibEntry->m_Prefix;

    /* Localisation des autres fields */
    for (Field = LibEntry->Fields; Field != NULL;
                        Field = (LibDrawField*)Field->Pnext)
        {
        if ( Field->m_Text.IsEmpty() ) continue;
        x0 = Field->m_Pos.x; y0 = - Field->m_Pos.y;
        dx = Field->m_Size.x * Field->m_Text.Len(),
        dy = Field->m_Size.y;
        hjustify = Field->m_HJustify; vjustify = Field->m_VJustify;
        if (Field->m_Orient) EXCHG(dx, dy);
        if (LibEntry->m_Prefix.m_Orient) EXCHG(dx, dy);
        if ( hjustify == GR_TEXT_HJUSTIFY_CENTER ) x0 -= dx/2;
        else if ( hjustify == GR_TEXT_HJUSTIFY_RIGHT ) x0 -= dx;
        if ( vjustify == GR_TEXT_VJUSTIFY_CENTER ) y0 -= dy/2;
        else if ( vjustify == GR_TEXT_VJUSTIFY_BOTTOM ) y0 -= dy;
        x1 = x0 + dx; y1 = y0 + dy;
        if( (GetScreen()->m_Curseur.x >= x0) && ( GetScreen()->m_Curseur.x <= x1) &&
            (GetScreen()->m_Curseur.y >= y0) && ( GetScreen()->m_Curseur.y <= y1) )
            return(Field);
        }

    return NULL;
}

/********************************************************************************/
LibEDA_BaseStruct* WinEDA_LibeditFrame::LocateItemUsingCursor()
/********************************************************************************/
{
    LibEDA_BaseStruct* DrawEntry = CurrentDrawItem;

    if ( CurrentLibEntry == NULL ) return NULL;

    if ( (DrawEntry == NULL) || (DrawEntry->m_Flags == 0) )
    { 	// Simple localisation des elements
        DrawEntry = LocatePin(GetScreen()->m_Curseur, CurrentLibEntry, CurrentUnit, CurrentConvert);
        if ( DrawEntry == NULL )
        {
            DrawEntry = CurrentDrawItem = LocateDrawItem((SCH_SCREEN*)GetScreen(),
                    GetScreen()->m_MousePosition,CurrentLibEntry,CurrentUnit,
                    CurrentConvert,LOCATE_ALL_DRAW_ITEM);
        }
        if ( DrawEntry == NULL )
        {
            DrawEntry = CurrentDrawItem = LocateDrawItem((SCH_SCREEN*)GetScreen(), GetScreen()->m_Curseur, CurrentLibEntry,CurrentUnit,
                    CurrentConvert,LOCATE_ALL_DRAW_ITEM);
        }
        if ( DrawEntry == NULL )
        {
            DrawEntry = CurrentDrawItem = (LibEDA_BaseStruct*)
                LocateField(CurrentLibEntry);
        }
    }
    return DrawEntry;
}
