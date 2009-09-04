/*********************************************************************/
/*  EESchema - edition des librairies: Edition des champs ( Fields ) */
/*********************************************************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"

#include "program.h"
#include "libcmp.h"
#include "general.h"

#include "protos.h"


/* Routines locales */
static void ShowMoveField( WinEDA_DrawPanel* panel, wxDC* DC, bool erase );

/*
 * if the field is the reference, return reference like schematic,
 * i.e U -> U? or U?A or the field text for others
 */
static wxString ReturnFieldFullText( LibDrawField* aField );


/* Variables locales */
extern int     CurrentUnit;
static wxPoint StartCursor, LastTextPosition;


static void ExitMoveField( WinEDA_DrawPanel* Panel, wxDC* DC )
{
    Panel->ManageCurseur = NULL;
    Panel->ForceCloseManageCurseur = NULL;
    if( CurrentDrawItem == NULL )
        return;

    wxPoint curpos = Panel->GetScreen()->m_Curseur;

    Panel->GetScreen()->m_Curseur = StartCursor;
    ShowMoveField( Panel, DC, TRUE );
    Panel->GetScreen()->m_Curseur = curpos;
    CurrentDrawItem->m_Flags = 0;

    CurrentDrawItem = NULL;
}


/*
 * Initialise le deplacement d'un champ ( ref ou Name )
 */
void WinEDA_LibeditFrame::StartMoveField( wxDC* DC, LibDrawField* field )
{
    wxPoint startPos;

    if( ( CurrentLibEntry == NULL ) || ( field == NULL ) )
        return;

    CurrentDrawItem  = field;
    LastTextPosition = field->m_Pos;
    CurrentDrawItem->m_Flags |= IS_MOVED;

    startPos.x = LastTextPosition.x;
    startPos.y = -LastTextPosition.y;
    DrawPanel->CursorOff( DC );
    GetScreen()->m_Curseur = startPos;
    DrawPanel->MouseToCursorSchema();

    DrawPanel->ManageCurseur = ShowMoveField;
    DrawPanel->ForceCloseManageCurseur = ExitMoveField;
    DrawPanel->ManageCurseur( DrawPanel, DC, TRUE );
    StartCursor = GetScreen()->m_Curseur;

    DrawPanel->CursorOn( DC );
}


/*
 * If the field is the reference, return reference like schematic,
 * i.e U -> U? or U?A or the field text for others
 *
 * @fixme This should be handled by the field object.
 */
static wxString ReturnFieldFullText( LibDrawField* aField )
{
    if( aField->m_FieldId != REFERENCE )
        return aField->m_Text;

    wxString text = aField->m_Text;

    if( CurrentLibEntry->m_UnitCount > 1 )
    {
#if defined(KICAD_GOST)
        text.Printf( wxT( "%s?.%c" ),
                     aField->m_Text.GetData(), CurrentUnit + '1' - 1 );
#else

        text.Printf( wxT( "%s?%c" ),
                     aField->m_Text.GetData(), CurrentUnit + 'A' - 1 );
#endif
    }
    else
        text << wxT( "?" );

    return text;
}


/*****************************************************************/
/* Routine d'affichage du texte 'Field' en cours de deplacement. */
/*  Routine normalement attachee au curseur                     */
/*****************************************************************/
static void ShowMoveField( WinEDA_DrawPanel* panel, wxDC* DC, bool erase )
{
    LibDrawField* Field = (LibDrawField*) CurrentDrawItem;

    if( ( CurrentLibEntry == NULL ) || ( Field == NULL ) )
        return;

    wxString text = ReturnFieldFullText( Field );

    if( erase )
        Field->Draw( panel, DC, wxPoint( 0, 0 ), -1, g_XorMode, &text,
                     DefaultTransformMatrix );

    LastTextPosition.x = panel->GetScreen()->m_Curseur.x;
    LastTextPosition.y = -panel->GetScreen()->m_Curseur.y;

    Field->m_Pos = LastTextPosition;
    Field->Draw( panel, DC, wxPoint( 0, 0 ), -1, g_XorMode, &text,
                 DefaultTransformMatrix );
}


void WinEDA_LibeditFrame::PlaceField( wxDC* DC, LibDrawField* Field )
{
    if( Field == NULL )
        return;

    Field->m_Flags = 0;
    Field->m_Pos.x = GetScreen()->m_Curseur.x;
    Field->m_Pos.y = -GetScreen()->m_Curseur.y;
    DrawPanel->CursorOff( DC );

    wxString fieldText = ReturnFieldFullText( Field );

    Field->Draw( DrawPanel, DC, wxPoint( 0, 0 ), -1, GR_DEFAULT_DRAWMODE,
                 &fieldText, DefaultTransformMatrix );

    DrawPanel->CursorOn( DC );
    GetScreen()->SetModify();
    DrawPanel->ManageCurseur = NULL;
    DrawPanel->ForceCloseManageCurseur = NULL;
    CurrentDrawItem = NULL;
}


void WinEDA_LibeditFrame::EditField( wxDC* DC, LibDrawField* Field )
{
    wxString   Text;
    wxString   title;

    if( Field == NULL )
        return;

    switch( Field->m_FieldId )
    {
    case REFERENCE:
        title = wxT( "Reference:" );
        break;

    case VALUE:
        title = wxT( "Component Name / Value:" );
        break;

    default:
        break;
    }

    Text = Field->m_Text;

    /* FIXME: Use wxTextEntry dialog here and check for cancel button. */
    Get_Message( title, _( "Edit field" ), Text, this );
    Text.Replace( wxT( " " ), wxT( "_" ) );

    /* If the value is changed, this is equivalent to create a new component
     * from the old one.
     * So we must check for an existing alias of this "new" component
     * and change the value only if there is no existing alias with the same
     * name for this component
     */
    if( Field->m_FieldId == VALUE )
    {
        /* test for an existing name in alias list: */
        for( unsigned ii = 0; ii < CurrentLibEntry->m_AliasList.GetCount();
             ii += ALIAS_NEXT )
        {
            wxString aliasname = CurrentLibEntry->m_AliasList[ii + ALIAS_NAME];

            if( Text.CmpNoCase( aliasname ) == 0 )
            {
                DisplayError( this, _( "This name is an existing alias of \
the component\nAborting" ) );
                return;
            }
        }
    }

    wxString fieldText = ReturnFieldFullText( Field );

    Field->Draw( DrawPanel, DC, wxPoint( 0, 0 ), -1, g_XorMode, &fieldText,
                 DefaultTransformMatrix );

    if( !Text.IsEmpty() )
    {
        SaveCopyInUndoList( CurrentLibEntry );
        Field->m_Text = Text;
    }
    else
    {
        DisplayError( this, _( "No new text: no change" ) );
    }

    fieldText = ReturnFieldFullText( Field );
    int drawMode = g_XorMode;

    if( Field->m_Flags == 0 )
        drawMode = GR_DEFAULT_DRAWMODE;

    Field->Draw( DrawPanel, DC, wxPoint( 0, 0 ), -1, drawMode, &fieldText,
                 DefaultTransformMatrix );

    GetScreen()->SetModify();

    if( Field->m_FieldId == VALUE )
        ReCreateHToolbar();
}


/*
 * Routine de modification de l'orientation ( Horiz ou Vert. ) du champ.
 * si un champ est en cours d'edition, modif de celui ci.
 * sinon Modif du champ pointe par la souris
 */
void WinEDA_LibeditFrame::RotateField( wxDC* DC, LibDrawField* Field )
{

    if( Field == NULL )
        return;

    GetScreen()->SetModify();
    DrawPanel->CursorOff( DC );
    GRSetDrawMode( DC, g_XorMode );

    wxString fieldText = ReturnFieldFullText( Field );

    Field->Draw( DrawPanel, DC, wxPoint( 0, 0 ), -1, g_XorMode, &fieldText,
                 DefaultTransformMatrix );

    if( Field->m_Orient )
        Field->m_Orient = 0;
    else
        Field->m_Orient = 1;

    int drawMode = g_XorMode;

    if( Field->m_Flags == 0 )
        drawMode = GR_DEFAULT_DRAWMODE;

    Field->Draw( DrawPanel, DC, wxPoint( 0, 0 ), -1, drawMode, &fieldText,
                 DefaultTransformMatrix );

    DrawPanel->CursorOn( DC );
}


/*
 * Locate the component fiels (ref, name or auxiliary fields) under the
 * mouse cursor
 * return:
 *   pointer on the field (or NULL )
 */
LibDrawField* WinEDA_LibeditFrame::LocateField( EDA_LibComponentStruct* LibEntry )
{
    wxPoint refpos = GetScreen()->m_Curseur;
    /* Test reference */
    if( LibEntry->m_Name.HitTest( refpos ) )
        return &LibEntry->m_Name;

    /* Test Prefix */
    if( LibEntry->m_Prefix.HitTest( refpos ) )
        return &LibEntry->m_Prefix;

    /* Test others fields */
    for( LibDrawField* field = LibEntry->m_Fields; field != NULL;
         field = field->Next() )
    {
        if( field->m_Text.IsEmpty() )
            continue;

        if( field->HitTest( refpos ) )
            return field;
    }

    return NULL;
}


LibEDA_BaseStruct* WinEDA_LibeditFrame::LocateItemUsingCursor()
{
    LibEDA_BaseStruct* DrawEntry = CurrentDrawItem;

    if( CurrentLibEntry == NULL )
        return NULL;

    if( ( DrawEntry == NULL ) || ( DrawEntry->m_Flags == 0 ) )
    {
        DrawEntry = LocatePin( GetScreen()->m_Curseur, CurrentLibEntry,
                               CurrentUnit, CurrentConvert );
        if( DrawEntry == NULL )
        {
            DrawEntry = CurrentDrawItem =
                LocateDrawItem( (SCH_SCREEN*) GetScreen(),
                                GetScreen()->m_MousePosition,
                                CurrentLibEntry, CurrentUnit,
                                CurrentConvert, LOCATE_ALL_DRAW_ITEM );
        }
        if( DrawEntry == NULL )
        {
            DrawEntry = CurrentDrawItem =
                LocateDrawItem( (SCH_SCREEN*) GetScreen(),
                                GetScreen()->m_Curseur, CurrentLibEntry,
                                CurrentUnit, CurrentConvert,
                                LOCATE_ALL_DRAW_ITEM );
        }
        if( DrawEntry == NULL )
        {
            DrawEntry = CurrentDrawItem =
                (LibEDA_BaseStruct*) LocateField( CurrentLibEntry );
        }
    }

    return DrawEntry;
}
