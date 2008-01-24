/**************************************************************/
/* onleftclick.cpp:                                           */
/* function called when the left button is clicked (released) */
/* function called when the left button is double clicked     */
/**************************************************************/

#include "fctsys.h"

#include "gr_basic.h"

#include "common.h"
#include "pcbnew.h"
#include "autorout.h"

#include "id.h"
#include "protos.h"
#include "eda_dde.h"



/********************************************************************/
void WinEDA_PcbFrame::OnLeftClick( wxDC* DC, const wxPoint& MousePos )
/********************************************************************/

/* Handle the left buttom mouse click, when a tool is active
 */
{
    BOARD_ITEM* DrawStruct = GetCurItem();
	bool exit = false;

    if( (m_ID_current_state == 0) || ( DrawStruct && DrawStruct->m_Flags ) )
    {
        DrawPanel->m_AutoPAN_Request = FALSE;
        if( DrawStruct && DrawStruct->m_Flags ) // "POPUP" in progress
        {
			DrawPanel->m_IgnoreMouseEvents = TRUE;
			DrawPanel->CursorOff( DC );

            switch( DrawStruct->Type() )
            {
            case TYPEZONE_CONTAINER:
                End_Move_Zone_Corner_Or_Outlines( DC, (ZONE_CONTAINER *) DrawStruct );
                exit = true;
                break;

            case TYPETRACK:
            case TYPEVIA:
                if( DrawStruct->m_Flags & IS_DRAGGED )
                {
                    PlaceDraggedTrackSegment( (TRACK*) DrawStruct, DC );
                    exit = true;
                }
                break;

            case TYPETEXTE:
                Place_Texte_Pcb( (TEXTE_PCB*) DrawStruct, DC );
                exit = true;
                break;

            case TYPETEXTEMODULE:
                PlaceTexteModule( (TEXTE_MODULE*) DrawStruct, DC );
                exit = true;
                break;

            case TYPEPAD:
                PlacePad( (D_PAD*) DrawStruct, DC );
                exit = true;
                break;

            case TYPEMODULE:
                Place_Module( (MODULE*) DrawStruct, DC );
                exit = true;
                break;

            case TYPEMIRE:
                Place_Mire( (MIREPCB*) DrawStruct, DC );
                exit = true;
                break;

            case TYPEDRAWSEGMENT:
                if( m_ID_current_state == 0 )
                {
                    Place_DrawItem( (DRAWSEGMENT*) DrawStruct, DC );
                    exit = true;
                }
                break;

            default:
                if( m_ID_current_state == 0 )
                {
                    DisplayError( this,
                         wxT( "WinEDA_PcbFrame::OnLeftClick() err: DrawType %d m_Flags != 0" ), DrawStruct->Type() );
                    exit = true;
                }
				break;
            }

			DrawPanel->m_IgnoreMouseEvents = FALSE;
			DrawPanel->CursorOn( DC );
			if ( exit ) return;
        }
        
        else if( !wxGetKeyState(WXK_SHIFT) && !wxGetKeyState(WXK_ALT) &&
                !wxGetKeyState(WXK_CONTROL) )
        {
            DrawStruct = PcbGeneralLocateAndDisplay();
            if( DrawStruct )
                SendMessageToEESCHEMA( DrawStruct );
        }            
    }
       
    switch( m_ID_current_state )
    {
    case ID_MAIN_MENUBAR:
    case 0:
        break;

    case ID_NO_SELECT_BUTT:
        break;

    case ID_PCB_MUWAVE_TOOL_SELF_CMD:
    case ID_PCB_MUWAVE_TOOL_GAP_CMD:
    case ID_PCB_MUWAVE_TOOL_STUB_CMD:
    case ID_PCB_MUWAVE_TOOL_STUB_ARC_CMD:
    case ID_PCB_MUWAVE_TOOL_FUNCTION_SHAPE_CMD:
        MuWaveCommand( DC, MousePos );
        break;

    case ID_PCB_HIGHLIGHT_BUTT:
    {
        int netcode = Select_High_Light( DC );
        if( netcode < 0 )
            m_Pcb->Display_Infos( this );
        else
            Affiche_Infos_Equipot( netcode, this );
    }
        break;

    case ID_PCB_SHOW_1_RATSNEST_BUTT:
        DrawStruct = PcbGeneralLocateAndDisplay();
        Show_1_Ratsnest( DrawStruct, DC );
        
        if( DrawStruct )
            SendMessageToEESCHEMA( DrawStruct );
        break;

    case ID_PCB_MIRE_BUTT:
        if( (DrawStruct == NULL) || (DrawStruct->m_Flags == 0) )
        {
            SetCurItem( Create_Mire( DC ) );
            DrawPanel->MouseToCursorSchema();
        }
        else if( DrawStruct->Type() == TYPEMIRE )
        {
            Place_Mire( (MIREPCB*) DrawStruct, DC );
        }
        else
            DisplayError( this, wxT( "Internal err: Struct not TYPEMIRE" ) );
        break;

    case ID_PCB_CIRCLE_BUTT:
    case ID_PCB_ARC_BUTT:
    case ID_LINE_COMMENT_BUTT:
    {
        int shape = S_SEGMENT;
        if( m_ID_current_state == ID_PCB_CIRCLE_BUTT )
            shape = S_CIRCLE;
        if( m_ID_current_state == ID_PCB_ARC_BUTT )
            shape = S_ARC;

        if( GetScreen()->m_Active_Layer <= LAST_COPPER_LAYER )
        {
            DisplayError( this, _( "Graphic not authorized on Copper layers" ) );
            break;
        }
        if( (DrawStruct == NULL) || (DrawStruct->m_Flags == 0) )
        {
            DrawStruct = Begin_DrawSegment( NULL, shape, DC ); 
            SetCurItem( DrawStruct );
            DrawPanel->m_AutoPAN_Request = TRUE;
        }
        else if( DrawStruct
                && (DrawStruct->Type() == TYPEDRAWSEGMENT)
                && (DrawStruct->m_Flags & IS_NEW) )
        {
            DrawStruct = Begin_DrawSegment( (DRAWSEGMENT*) DrawStruct, shape, DC );
            SetCurItem( DrawStruct );
            DrawPanel->m_AutoPAN_Request = TRUE;
        }
        break;
    }

    case ID_TRACK_BUTT:
        if( GetScreen()->m_Active_Layer > LAST_COPPER_LAYER )
        {
            DisplayError( this, _( "Tracks on Copper layers only " ) );
            break;
        }

        if( (DrawStruct == NULL) || (DrawStruct->m_Flags == 0) )
        {
            DrawStruct = Begin_Route( NULL, DC );
            SetCurItem( DrawStruct );
            if( DrawStruct )
                DrawPanel->m_AutoPAN_Request = TRUE;
        }
        else if( DrawStruct &&

//					(DrawStruct->Type() == TYPETRACK) &&
                (DrawStruct->m_Flags & IS_NEW) )
        {
            TRACK* track = Begin_Route( (TRACK*) DrawStruct, DC );
            if( track )  // c'est a dire si OK
                SetCurItem( DrawStruct = track );
            DrawPanel->m_AutoPAN_Request = TRUE;
        }
        break;


    case ID_PCB_ZONES_BUTT:
        if( (DrawStruct == NULL) || (DrawStruct->m_Flags == 0) )
        {
            GetScreen()->SetCurItem( DrawStruct = Begin_Zone( DC ) );
        }
        else if( DrawStruct
                && (DrawStruct->Type() == TYPEEDGEZONE)
                && (DrawStruct->m_Flags & IS_NEW) )
        {
            GetScreen()->SetCurItem( DrawStruct = Begin_Zone( DC ) );
        }
        else
            DisplayError( this, wxT( "Edit: zone internal error" ) );
        break;

    case ID_TEXT_COMMENT_BUTT:
        if( (DrawStruct == NULL) || (DrawStruct->m_Flags == 0) )
        {
            SetCurItem( Create_Texte_Pcb( DC ) );
            DrawPanel->MouseToCursorSchema();
            DrawPanel->m_AutoPAN_Request = TRUE;
        }
        else if( DrawStruct->Type() == TYPETEXTE )
        {
            Place_Texte_Pcb( (TEXTE_PCB*) DrawStruct, DC );
            DrawPanel->m_AutoPAN_Request = FALSE;
        }
        else
            DisplayError( this, wxT( "Internal err: Struct not TYPETEXTE" ) );
        break;

    case ID_COMPONENT_BUTT:
        if( (DrawStruct == NULL) || (DrawStruct->m_Flags == 0) )
        {
            DrawPanel->MouseToCursorSchema();
            DrawStruct = Load_Module_From_Library( wxEmptyString, DC );             
            SetCurItem( DrawStruct );
            if( DrawStruct )
                StartMove_Module( (MODULE*) DrawStruct, DC );
        }
        else if( DrawStruct->Type() == TYPEMODULE )
        {
            Place_Module( (MODULE*) DrawStruct, DC );
            DrawPanel->m_AutoPAN_Request = FALSE;
        }
        else
            DisplayError( this, wxT( "Internal err: Struct not TYPEMODULE" ) );
        break;

    case ID_PCB_COTATION_BUTT:
        if( GetScreen()->m_Active_Layer <= LAST_COPPER_LAYER )
        {
            DisplayError( this, _( "Cotation not authorized on Copper layers" ) );
            break;
        }
        if( (DrawStruct == NULL) || (DrawStruct->m_Flags == 0) )
        {
            DrawStruct = Begin_Cotation( NULL, DC );
            SetCurItem( DrawStruct );
            DrawPanel->m_AutoPAN_Request = TRUE;
        }
        else if( DrawStruct
                && (DrawStruct->Type() == TYPECOTATION)
                && (DrawStruct->m_Flags & IS_NEW) )
        {
            DrawStruct = Begin_Cotation( (COTATION*) DrawStruct, DC ); 
            SetCurItem( DrawStruct );
            DrawPanel->m_AutoPAN_Request = TRUE;
        }
        else
            DisplayError( this, wxT( "Internal err: Struct not COTATION" ) );
        break;

    case ID_PCB_DELETE_ITEM_BUTT:
        if( !DrawStruct || (DrawStruct->m_Flags == 0) )
        {
            DrawStruct = PcbGeneralLocateAndDisplay();
            if( DrawStruct && (DrawStruct->m_Flags == 0) )
            {
                RemoveStruct( DrawStruct, DC );
                SetCurItem( DrawStruct = NULL );
            }
        }
        break;

    case ID_PCB_PLACE_OFFSET_COORD_BUTT:
        DrawPanel->m_Draw_Auxiliary_Axis( DC, GR_XOR );
        m_Auxiliary_Axis_Position = GetScreen()->m_Curseur;
        DrawPanel->m_Draw_Auxiliary_Axis( DC, GR_COPY );
        GetScreen()->SetModify();
        break;

    default:
        DrawPanel->SetCursor( wxCURSOR_ARROW );
        DisplayError( this, wxT( "WinEDA_PcbFrame::OnLeftClick() id error" ) );
        SetToolID( 0, wxCURSOR_ARROW, wxEmptyString );
        break;
    }
}


/********************************************************************************/
void WinEDA_PcbFrame::OnLeftDClick( wxDC* DC, const wxPoint& MousePos )
/********************************************************************************/

/* handle the double click on the mouse left button 
 */
{
    BOARD_ITEM*     DrawStruct = GetCurItem();
    wxPoint         pos = GetPosition();
    wxClientDC      dc( DrawPanel );

    DrawPanel->PrepareGraphicContext( &dc );

    switch( m_ID_current_state )
    {
    case 0:
        if( (DrawStruct == NULL) || (DrawStruct->m_Flags == 0) )
        {
            DrawStruct = PcbGeneralLocateAndDisplay();
        }

        if( (DrawStruct == NULL) || (DrawStruct->m_Flags != 0) )
            break;

        SendMessageToEESCHEMA( DrawStruct );

        // An item is found
        SetCurItem( DrawStruct );

        switch( DrawStruct->Type() )
        {
        case TYPETRACK:
        case TYPEVIA:
            if( DrawStruct->m_Flags & IS_NEW )
            {
                End_Route( (TRACK*) DrawStruct, DC );
                DrawPanel->m_AutoPAN_Request = FALSE;
            }
            else if( DrawStruct->m_Flags == 0 )
            {
                Edit_TrackSegm_Width( DC, (TRACK*) DrawStruct );
            }
            break;

        case TYPETEXTE:
            InstallTextPCBOptionsFrame( (TEXTE_PCB*) DrawStruct,
                                       DC, ( (TEXTE_PCB*) DrawStruct )->m_Pos );
            DrawPanel->MouseToCursorSchema();
            break;

        case TYPEPAD:
            InstallPadOptionsFrame( (D_PAD*) DrawStruct, &dc, pos );
            DrawPanel->MouseToCursorSchema();
            break;

        case TYPEMODULE:
            InstallModuleOptionsFrame( (MODULE*) DrawStruct, &dc, pos );
            DrawPanel->MouseToCursorSchema();
            break;

        case TYPEMIRE:
            InstallMireOptionsFrame( (MIREPCB*) DrawStruct, &dc, pos );
            DrawPanel->MouseToCursorSchema();
            break;

        case TYPETEXTEMODULE:
            InstallTextModOptionsFrame( (TEXTE_MODULE*) DrawStruct, &dc, pos );
            DrawPanel->MouseToCursorSchema();
            break;

        case TYPEDRAWSEGMENT:
            break;

        default:
            break;
        }

        break;      // end case 0

    case ID_TRACK_BUTT:
        if( DrawStruct && (DrawStruct->m_Flags & IS_NEW) )
        {
            End_Route( (TRACK*) DrawStruct, DC );
            DrawPanel->m_AutoPAN_Request = FALSE;
        }
        break;

    case ID_PCB_ZONES_BUTT:
        if ( End_Zone( DC ) )
		{
			DrawPanel->m_AutoPAN_Request = FALSE;
			SetCurItem( NULL );
		}
        break;

    case ID_LINE_COMMENT_BUTT:
    case ID_PCB_ARC_BUTT:
    case ID_PCB_CIRCLE_BUTT:
        if( DrawStruct == NULL )
            break;
        if( DrawStruct->Type() != TYPEDRAWSEGMENT )
        {
            DisplayError( this, wxT( "DrawStruct Type error" ) );
            DrawPanel->m_AutoPAN_Request = FALSE;
            break;
        }
        if( (DrawStruct->m_Flags & IS_NEW) )
        {
            End_Edge( (DRAWSEGMENT*) DrawStruct, &dc );
            DrawPanel->m_AutoPAN_Request = FALSE;
            SetCurItem( NULL );
        }
        break;
    }
}
