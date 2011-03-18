/****************/
/* savecmp.cpp  */
/****************/

#include "fctsys.h"
#include "wxstruct.h"
#include "common.h"
#include "confirm.h"
#include "kicad_string.h"
#include "gestfich.h"
#include "macros.h"
#include "appl_wxstruct.h"

#include "cvpcb.h"
#include "cvpcb_mainframe.h"

#include "build_version.h"


/* File header. */
char EnteteCmpMod[] = { "Cmp-Mod V01" };

#define titleComponentLibErr _( "Component Library Error" )


/*
 * Backup modules file.
 *
 * @param NetlistFullFileName - Name of net list file to save.
 * @returns - 1 if OK, 0 if error.
 */
int CVPCB_MAINFRAME::SaveComponentList( const wxString& NetlistFullFileName )
{
    FILE*       dest;
    wxFileName  fn( NetlistFullFileName );
    char        Line[1024];
    wxString    Title = wxGetApp().GetTitle() + wxT( " " ) + GetBuildVersion();

    fn.SetExt( ComponentFileExtension );

    dest = wxFopen( fn.GetFullPath(), wxT( "wt" ) );
    if( dest == NULL )
        return 0;

    fprintf( dest, "%s", EnteteCmpMod );
    fprintf( dest, " Created by %s", TO_UTF8( Title ) );
    fprintf( dest, " date = %s\n", DateAndTime( Line ) );

    BOOST_FOREACH( COMPONENT& component, m_components )
    {
        fprintf( dest, "\nBeginCmp\n" );
        fprintf( dest, "TimeStamp = %s;\n",
                 TO_UTF8( component.m_TimeStamp ) );
        fprintf( dest, "Reference = %s;\n",
                 TO_UTF8( component.m_Reference ) );
        fprintf( dest, "ValeurCmp = %s;\n",
                 TO_UTF8( component.m_Value ) );
        fprintf( dest, "IdModule  = %s;\n",
                 TO_UTF8( component.m_Module ) );
        fprintf( dest, "EndCmp\n" );
    }

    fprintf( dest, "\nEndListe\n" );
    fclose( dest );
    return 1;
}


/*
 * Load list of associated components and footprints.
 */
bool CVPCB_MAINFRAME::LoadComponentFile( const wxString& fileName )
{
    wxString    timestamp, valeur, ilib, namecmp, msg;
    bool        read_cmp_data = FALSE, eof = FALSE;
    char        Line[1024], * ident, * data;
    FILE*       source;
    wxFileName  fn = fileName;

    fn.SetExt( ComponentFileExtension );

    source = wxFopen( fn.GetFullPath(), wxT( "rt" ) );
    if( source == NULL )
    {
        msg.Printf( _( "Cannot open CvPcb component file <%s>." ),
                    GetChars( fn.GetFullPath() ) );
        msg << wxT("\n") << _("This is normal if you are opening a new netlist file");
        wxMessageBox( msg, titleComponentLibErr, wxOK | wxICON_ERROR );
        return false;
    }

    /* Identification of the type of file CmpMod */
    if( fgets( Line, 79, source ) == 0 )
    {
        msg.Printf( _( " <%s> does not appear to be a valid Kicad component library." ),
                    GetChars( fn.GetFullPath() ) );
        wxMessageBox( msg, titleComponentLibErr, wxOK | wxICON_ERROR );
        fclose( source );
        return false;
    }

    if( strnicmp( Line, EnteteCmpMod, 11 ) != 0 ) /* old file version*/
    {
        msg.Printf( _( "<%s> is an old version component file." ) );
        wxMessageBox( msg, titleComponentLibErr, wxOK | wxICON_ERROR );
        fclose( source );
        return false;
    }

    while( !eof && fgets( Line, 79, source ) != 0 )
    {
        if( strnicmp( Line, "EndListe", 8 ) == 0 )
            break;

        /* Search the beginning of the component description. */
        if( strnicmp( Line, "BeginCmp", 8 ) != 0 )
            continue;
        timestamp.Empty();
        valeur.Empty();
        ilib.Empty();
        namecmp.Empty();
        read_cmp_data = TRUE;

        while( !eof && read_cmp_data )
        {
            if( fgets( Line, 1024, source ) == 0 )
            {
                eof = TRUE;
                break;
            }

            if( strnicmp( Line, "EndCmp", 6 ) == 0 )
            {
                read_cmp_data = TRUE;
                break;
            }

            ident = strtok( Line, "=;\n\r" );
            data  = strtok( NULL, ";\n\r" );

            if( strnicmp( ident, "TimeStamp", 9 ) == 0 )
            {
                timestamp = FROM_UTF8( data );
                timestamp.Trim( TRUE );
                timestamp.Trim( FALSE );
                continue;
            }

            if( strnicmp( ident, "Reference", 9 ) == 0 )
            {
                namecmp = FROM_UTF8( data );
                namecmp.Trim( TRUE );
                namecmp.Trim( FALSE );
                continue;
            }

            if( strnicmp( ident, "ValeurCmp", 9 ) == 0 )
            {
                valeur = FROM_UTF8( data );
                valeur.Trim( TRUE );
                valeur.Trim( FALSE );
                continue;
            }

            if( strnicmp( ident, "IdModule", 8 ) == 0 )
            {
                ilib = FROM_UTF8( data );
                ilib.Trim( TRUE );
                ilib.Trim( FALSE );
                continue;
            }
        } /* End reading component description. */

        /* Search corresponding component and NetList
         * Update its parameters. */
        BOOST_FOREACH( COMPONENT& component, m_components )
        {
            if( namecmp != component.m_Reference )
                continue;

            /* Copy the name of the corresponding module. */
            component.m_Module = ilib;
        }
    }

    fclose( source );
    return true;
}
