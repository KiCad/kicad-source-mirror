/**
 * @file pcbnew/cross-probing.cpp
 * @brief Cross probing functions to handle communication to andfrom Eeschema.
 */

/**
 * Handle messages between Pcbnew and Eeschema via a socket, the port numbers are
 * KICAD_PCB_PORT_SERVICE_NUMBER (currently 4242) (Eeschema to Pcbnew)
 * KICAD_SCH_PORT_SERVICE_NUMBER (currently 4243) (Pcbnew to Eeschema)
 * Note: these ports must be enabled for firewall protection
 */

#include <fctsys.h>
#include <pgm_base.h>
#include <kiface_i.h>
#include <kiway_express.h>
#include <wxPcbStruct.h>
#include <eda_dde.h>
#include <macros.h>

#include <pcbnew_id.h>
#include <class_board.h>
#include <class_module.h>

#include <collectors.h>
#include <pcbnew.h>


/* Execute a remote command send by Eeschema via a socket,
 * port KICAD_PCB_PORT_SERVICE_NUMBER
 * cmdline = received command from Eeschema
 * Commands are
 * $PART: "reference"   put cursor on component
 * $PIN: "pin name"  $PART: "reference" put cursor on the footprint pin
 */
void PCB_EDIT_FRAME::ExecuteRemoteCommand( const char* cmdline )
{
    char            line[1024];
    wxString        msg;
    wxString        modName;
    char*           idcmd;
    char*           text;
    MODULE*         module = 0;
    BOARD* pcb = GetBoard();
    wxPoint         pos;

    strncpy( line, cmdline, sizeof(line) - 1 );

    idcmd = strtok( line, " \n\r" );
    text  = strtok( NULL, " \n\r" );

    if( !idcmd || !text )
        return;

    if( strcmp( idcmd, "$PART:" ) == 0 )
    {
        modName = FROM_UTF8( text );

        module = pcb->FindModuleByReference( modName );

        if( module )
            msg.Printf( _( "%s found" ), GetChars( modName ) );
        else
            msg.Printf( _( "%s not found" ), GetChars( modName ) );

        SetStatusText( msg );

        if( module )
            pos = module->GetPosition();
    }
    else if( strcmp( idcmd, "$PIN:" ) == 0 )
    {
        wxString pinName;
        D_PAD*   pad     = NULL;
        int      netcode = -1;

        pinName = FROM_UTF8( text );

        text = strtok( NULL, " \n\r" );
        if( text && strcmp( text, "$PART:" ) == 0 )
            text = strtok( NULL, "\n\r" );

        modName = FROM_UTF8( text );

        module = pcb->FindModuleByReference( modName );

        if( module )
            pad = module->FindPadByName( pinName );

        if( pad )
        {
            netcode = pad->GetNetCode();

            // put cursor on the pad:
            pos = pad->GetPosition();
        }

        if( netcode > 0 )               // highlight the pad net
        {
            pcb->HighLightON();
            pcb->SetHighLightNet( netcode );
        }
        else
        {
            pcb->HighLightOFF();
            pcb->SetHighLightNet( -1 );
        }

        if( module == NULL )
        {
            msg.Printf( _( "%s not found" ), GetChars( modName ) );
        }
        else if( pad == NULL )
        {
            msg.Printf( _( "%s pin %s not found" ), GetChars( modName ), GetChars( pinName ) );
            SetCurItem( module );
        }
        else
        {
            msg.Printf( _( "%s pin %s found" ), GetChars( modName ), GetChars( pinName ) );
            SetCurItem( pad );
        }

        SetStatusText( msg );
    }

    if( module )  // if found, center the module on screen, and redraw the screen.
    {
        SetCrossHairPosition( pos );
        RedrawScreen( pos, false );
    }
}


std::string FormatProbeItem( BOARD_ITEM* aItem )
{
    MODULE*     module;

    switch( aItem->Type() )
    {
    case PCB_MODULE_T:
        module = (MODULE*) aItem;
        return StrPrintf( "$PART: \"%s\"", TO_UTF8( module->GetReference() ) );

    case PCB_PAD_T:
        {
            module = (MODULE*) aItem->GetParent();
            wxString pad = ((D_PAD*)aItem)->GetPadName();

            return StrPrintf( "$PART: \"%s\" $PAD: \"%s\"",
                     TO_UTF8( module->GetReference() ),
                     TO_UTF8( pad ) );
        }

    case PCB_MODULE_TEXT_T:
        {
            module = (MODULE*) aItem->GetParent();

            TEXTE_MODULE*   text_mod = (TEXTE_MODULE*) aItem;

            const char*     text_key;

            if( text_mod->GetType() == TEXTE_MODULE::TEXT_is_REFERENCE )
                text_key = "$REF:";
            else if( text_mod->GetType() == TEXTE_MODULE::TEXT_is_VALUE )
                text_key = "$VAL:";
            else
                break;

            return StrPrintf( "$PART: \"%s\" %s \"%s\"",
                     TO_UTF8( module->GetReference() ),
                     text_key,
                     TO_UTF8( text_mod->GetText() ) );
        }

    default:
        break;
    }

    return "";
}


/**
 * Send a remote command to Eeschema via a socket,
 * @param objectToSync = item to be located on schematic (module, pin or text)
 * Commands are
 * $PART: "reference"   put cursor on component anchor
 * $PART: "reference" $PAD: "pad number" put cursor on the component pin
 * $PART: "reference" $REF: "reference" put cursor on the component ref
 * $PART: "reference" $VAL: "value" put cursor on the component value
 */
void PCB_EDIT_FRAME::SendMessageToEESCHEMA( BOARD_ITEM* aSyncItem )
{
#if 1
    wxASSERT( aSyncItem );      // can't we fix the caller?
#else
    if( !aSyncItem )
        return;
#endif

    std::string packet = FormatProbeItem( aSyncItem );

    if( packet.size() )
    {
        if( Kiface().IsSingle() )
            SendCommand( MSG_TO_SCH, packet.c_str() );
        else
        {
            // Typically ExpressMail is going to be s-expression packets, but since
            // we have existing interpreter of the cross probe packet on the other
            // side in place, we use that here.
            Kiway().ExpressMail( FRAME_SCH, MAIL_CROSS_PROBE, packet, this );
        }
    }
}


void PCB_EDIT_FRAME::KiwayMailIn( KIWAY_EXPRESS& mail )
{
    const std::string& payload = mail.GetPayload();

    switch( mail.Command() )
    {
    case MAIL_CROSS_PROBE:
        ExecuteRemoteCommand( payload.c_str() );
        break;

    // many many others.
    default:
        ;
    }
}

