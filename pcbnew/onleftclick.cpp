/**************************************************************/
/* onleftclick.cpp:                                           */
/* function called when the left button is clicked (released) */
/* function called when the left button is double clicked     */
/**************************************************************/

#include "fctsys.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "pcbnew.h"
#include "wxPcbStruct.h"

#include "id.h"



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
        DrawPanel->m_AutoPAN_Request = false;
        if( DrawStruct && DrawStruct->m_Flags ) // "POPUP" in progress
        {
            DrawPanel->m_IgnoreMouseEvents = TRUE;
            DrawPanel->CursorOff( DC );

            switch( DrawStruct->Type() )
            {
            case TYPE_ZONE_CONTAINER:
                if ( (DrawStruct->m_Flags & IS_NEW) )
                {
                    DrawPanel->m_AutoPAN_Request = true;
                    Begin_Zone( DC );
                }
                else
                    End_Move_Zone_Corner_Or_Outlines( DC, (ZONE_CONTAINER *) DrawStruct );
                exit = true;
                break;

            case TYPE_TRACK:
            case TYPE_VIA:
                if( DrawStruct->m_Flags & IS_DRAGGED )
                {
                    PlaceDraggedTrackSegment( (TRACK*) DrawStruct, DC );
                    exit = true;
                }
                break;

            case TYPE_TEXTE:
                Place_Texte_Pcb( (TEXTE_PCB*) DrawStruct, DC );
                exit = true;
                break;

            case TYPE_TEXTE_MODULE:
                PlaceTexteModule( (TEXTE_MODULE*) DrawStruct, DC );
                exit = true;
                break;

            case TYPE_PAD:
                PlacePad( (D_PAD*) DrawStruct, DC );
                exit = true;
                break;

            case TYPE_MODULE:
                Place_Module( (MODULE*) DrawStruct, DC );
                exit = true;
                break;

            case TYPE_MIRE:
                Place_Mire( (MIREPCB*) DrawStruct, DC );
                exit = true;
                break;

            case TYPE_DRAWSEGMENT:
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

            DrawPanel->m_IgnoreMouseEvents = false;
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
                GetBoard()->DisplayInfo( this );
            else
            {
                NETINFO_ITEM * net  = GetBoard()->FindNet(netcode);
                if ( net )
                    net->DisplayInfo( this );
            }
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
        else if( DrawStruct->Type() == TYPE_MIRE )
        {
            Place_Mire( (MIREPCB*) DrawStruct, DC );
        }
        else
            DisplayError( this, wxT( "Internal err: Struct not TYPE_MIRE" ) );
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

        if( ((PCB_SCREEN*)GetScreen())->m_Active_Layer <= LAST_COPPER_LAYER )
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
                && (DrawStruct->Type() == TYPE_DRAWSEGMENT)
                && (DrawStruct->m_Flags & IS_NEW) )
        {
            DrawStruct = Begin_DrawSegment( (DRAWSEGMENT*) DrawStruct, shape, DC );
            SetCurItem( DrawStruct );
            DrawPanel->m_AutoPAN_Request = TRUE;
        }
        break;
    }

    case ID_TRACK_BUTT:
        if( ((PCB_SCREEN*)GetScreen())->m_Active_Layer > LAST_COPPER_LAYER )
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
        else if( DrawStruct && (DrawStruct->m_Flags & IS_NEW) )
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
            if ( Begin_Zone( DC ) )
            {
                DrawPanel->m_AutoPAN_Request = true;
                DrawStruct = GetBoard()->m_CurrentZoneContour;
                GetScreen()->SetCurItem( DrawStruct );
            }
        }
        else if( DrawStruct
                && (DrawStruct->Type() == TYPE_ZONE_CONTAINER)
                && (DrawStruct->m_Flags & IS_NEW) )
        {
            DrawPanel->m_AutoPAN_Request = true;
            Begin_Zone( DC );
            DrawStruct = GetBoard()->m_CurrentZoneContour;
            GetScreen()->SetCurItem( DrawStruct );
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
        else if( DrawStruct->Type() == TYPE_TEXTE )
        {
            Place_Texte_Pcb( (TEXTE_PCB*) DrawStruct, DC );
            DrawPanel->m_AutoPAN_Request = false;
        }
        else
            DisplayError( this, wxT( "Internal err: Struct not TYPE_TEXTE" ) );
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
        else if( DrawStruct->Type() == TYPE_MODULE )
        {
            Place_Module( (MODULE*) DrawStruct, DC );
            DrawPanel->m_AutoPAN_Request = false;
        }
        else
            DisplayError( this, wxT( "Internal err: Struct not TYPE_MODULE" ) );
        break;

    case ID_PCB_COTATION_BUTT:
        if( ((PCB_SCREEN*)GetScreen())->m_Active_Layer <= LAST_COPPER_LAYER )
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
                && (DrawStruct->Type() == TYPE_COTATION)
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
        DrawPanel->DrawAuxiliaryAxis( DC, GR_XOR );
        m_Auxiliary_Axis_Position = GetScreen()->m_Curseur;
        DrawPanel->DrawAuxiliaryAxis( DC, GR_COPY );
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
        case TYPE_TRACK:
        case TYPE_VIA:
            if( DrawStruct->m_Flags & IS_NEW )
            {
                End_Route( (TRACK*) DrawStruct, DC );
                DrawPanel->m_AutoPAN_Request = false;
            }
            else if( DrawStruct->m_Flags == 0 )
            {
                Edit_TrackSegm_Width( DC, (TRACK*) DrawStruct );
            }
            break;

        case TYPE_TEXTE:
            InstallTextPCBOptionsFrame( (TEXTE_PCB*) DrawStruct, DC );
            DrawPanel->MouseToCursorSchema();
            break;

        case TYPE_PAD:
            InstallPadOptionsFrame( (D_PAD*) DrawStruct, &dc, pos );
            DrawPanel->MouseToCursorSchema();
            break;

        case TYPE_MODULE:
            InstallModuleOptionsFrame( (MODULE*) DrawStruct, &dc );
            DrawPanel->MouseToCursorSchema();
            break;

        case TYPE_MIRE:
            InstallMireOptionsFrame( (MIREPCB*) DrawStruct, &dc, pos );
            DrawPanel->MouseToCursorSchema();
            break;

        case TYPE_COTATION:
            Install_Edit_Cotation( (COTATION*) DrawStruct, &dc, pos );
            DrawPanel->MouseToCursorSchema();
            break;

        case TYPE_TEXTE_MODULE:
            InstallTextModOptionsFrame( (TEXTE_MODULE*) DrawStruct, &dc, pos );
            DrawPanel->MouseToCursorSchema();
            break;

        case TYPE_DRAWSEGMENT:
            InstallGraphicItemPropertiesDialog((DRAWSEGMENT*)DrawStruct, &dc);
            break;

        case TYPE_ZONE_CONTAINER:
            if( DrawStruct->m_Flags )
                break;
            Edit_Zone_Params( &dc, (ZONE_CONTAINER*) DrawStruct );
            break;

        default:
            break;
        }

        break;      // end case 0

    case ID_TRACK_BUTT:
        if( DrawStruct && (DrawStruct->m_Flags & IS_NEW) )
        {
            End_Route( (TRACK*) DrawStruct, DC );
            DrawPanel->m_AutoPAN_Request = false;
        }
        break;

    case ID_PCB_ZONES_BUTT:
        if ( End_Zone( DC ) )
        {
            DrawPanel->m_AutoPAN_Request = false;
            SetCurItem( NULL );
        }
        break;

    case ID_LINE_COMMENT_BUTT:
    case ID_PCB_ARC_BUTT:
    case ID_PCB_CIRCLE_BUTT:
        if( DrawStruct == NULL )
            break;
        if( DrawStruct->Type() != TYPE_DRAWSEGMENT )
        {
            DisplayError( this, wxT( "DrawStruct Type error" ) );
            DrawPanel->m_AutoPAN_Request = false;
            break;
        }
        if( (DrawStruct->m_Flags & IS_NEW) )
        {
            End_Edge( (DRAWSEGMENT*) DrawStruct, &dc );
            DrawPanel->m_AutoPAN_Request = false;
            SetCurItem( NULL );
        }
        break;
    }
}
