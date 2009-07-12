/**************/
/* listlib.cpp */
/**************/

/*
 *  cherche toutes les ref <chemin lib>*.??? si nom fichier pr‚sent,
 *  ou examine <chemin lib>[MODULE.LIB]
 */

#include "fctsys.h"
#include "wxstruct.h"
#include "common.h"
#include "confirm.h"
#include "kicad_string.h"
#include "gestfich.h"
#include "macros.h"
#include "appl_wxstruct.h"

#include "cvpcb.h"
#include "protos.h"

/* MDC file strings */ 
wxString mdc_files_not_found;
wxString mdc_files_invalid;

/* routines locales : */
static void ReadDocLib( const wxString& ModLibName, FOOTPRINT_LIST& list );


/**
 * Routine lisant la liste des librairies, et generant la liste chainee
 *  des modules disponibles
 *
 *  Module descr format:
 *  $MODULE c64acmd
 *  Li c64acmd
 *  Cd Connecteur DIN Europe 96 Contacts AC male droit
 *  Kw PAD_CONN DIN
 *  $EndMODULE
 *
 */
bool LoadFootprintFiles( const wxArrayString& libNames,
                         FOOTPRINT_LIST& list )
{
	FILE*       file;   /* pour lecture librairie */
	char        buffer[1024];
	wxFileName  filename;
	int         end;
	FOOTPRINT*  ItemLib;
	unsigned    i;
	wxString    tmp, msg;

	/* Check if footprint list is not empty */
	if( !list.empty() )
		list.clear();

	/* Check if there are footprint libraries in project file */ 
    if( libNames.GetCount() == 0 )
    {
        wxMessageBox( _( "No PCB foot print libraries are listed in the current project file." ),
                      _( "Project File Error" ), wxOK | wxICON_ERROR );
        return false;
    }

    /* Parse Libraries Listed */
    for( i = 0; i < libNames.GetCount(); i++ )
    {
        /* Calcul du nom complet de la librairie */
        filename = libNames[i];
        filename.SetExt( ModuleFileExtension );

        tmp = wxGetApp().FindLibraryPath( filename );


        if( !tmp )
        {
            msg.Printf( _( "PCB foot print library file <%s> could not be found in the default search paths." ),
                        filename.GetFullName().c_str() );
            wxMessageBox( msg, titleLibLoadError, wxOK | wxICON_ERROR );
            continue;
        }

        /* Open library file */
        file = wxFopen( tmp, wxT( "rt" ) );

        if( file == NULL )
        {
            msg.Printf( _( "Could not open PCB foot print library file <%s>." ),
                        tmp.c_str() );
            wxMessageBox( msg, titleLibLoadError, wxOK | wxICON_ERROR );
            continue;
        }

        /* Check if library type is valid */
        fgets( buffer, 32, file );
        if( strncmp( buffer, ENTETE_LIBRAIRIE, L_ENTETE_LIB ) != 0 )
        {
            msg.Printf( _( "<%s> is not a valid Kicad PCB foot print library" ),
                        tmp.c_str() );
            wxMessageBox( msg, titleLibLoadError, wxOK | wxICON_ERROR );
            fclose( file );
            continue;
        }

        /* TODO Lecture du nombre de composants */
        fseek( file, 0, 0 );

        /* TODO lecture nom des composants : */
        end = 0;
        while( !end && fgets( buffer, 255, file ) != NULL )
        {
            if( strnicmp( buffer, "$INDEX", 6 ) == 0 )
            {
                while( fgets( buffer, 255, file ) != NULL )
                {
                    if( strnicmp( buffer, "$EndINDEX", 6 ) == 0 )
                    {
                        end = 1;
                        break;
                    }

                    ItemLib = new FOOTPRINT();
                    ItemLib->m_Module = CONV_FROM_UTF8( StrPurge( buffer ) );
                    ItemLib->m_LibName = tmp;
                    list.push_back( ItemLib );
                }

                if( !end )
                {
                    msg.Printf( _( "Unexpected end of file occurred while parsing PCB foot print library <%s>." ),
                                tmp.c_str() );
                    wxMessageBox( msg, titleLibLoadError, wxOK | wxICON_ERROR );
                }
            }
        }

        fclose( file );
        ReadDocLib( tmp, list );
    }




	/* Display if there are mdc files not found */
	if( !mdc_files_not_found.IsEmpty() )
	{
		wxString message = _("Some MDC files could not be found!");
		DIALOG_LOAD_ERROR *dialog = new DIALOG_LOAD_ERROR(NULL);
		dialog->Show();
		dialog->MessageSet(&message);
		dialog->ListSet(&mdc_files_not_found);
		mdc_files_not_found = wxT("");
	}

	/* Display if there are mdc files invalid */
	if( !mdc_files_invalid.IsEmpty() )
	{
		wxString message = _("Some MDC files are invalid!");
		DIALOG_LOAD_ERROR *dialog = new DIALOG_LOAD_ERROR(NULL);
		dialog->Show();
		dialog->MessageSet(&message);
		dialog->ListSet(&mdc_files_invalid);
		mdc_files_invalid = wxT("");
	}

	list.sort();

	return true;
}


/** 
 * Routine de lecture du fichier Doc associe a la librairie ModLibName.
 *   Cree en memoire la chaine liste des docs pointee par MList
 *   ModLibName = full file Name de la librairie Modules
 */
static void
ReadDocLib( const wxString& ModLibName,
            FOOTPRINT_LIST& list )
{
	FOOTPRINT* NewMod;
	char       Line[1024];
	wxString   ModuleName;
	FILE*      mdc_file;
	wxFileName mdc_filename = ModLibName;

	/* Set mdc filename extension */
	mdc_filename.SetExt( wxT( "mdc" ) );

	/* Check if mdc file exists and can be opened */
	if( ( mdc_file = wxFopen( mdc_filename.GetFullPath(), wxT( "rt" ) ) ) == NULL )
	{
		mdc_files_not_found += mdc_filename.GetFullPath() + wxT("\n");
		return;
	}

	/* Check if mdc file is valid */
	GetLine( mdc_file, Line, NULL, sizeof(Line) - 1 );
	if( strnicmp( Line, ENTETE_LIBDOC, L_ENTETE_LIB ) != 0 )
	{
		mdc_files_invalid += mdc_filename.GetFullPath() + wxT("\n");
		return;
	}

	/* Read the mdc file */
	while( GetLine( mdc_file, Line, NULL, sizeof(Line) - 1 ) )
	{
		NewMod = NULL;
		if( Line[0] != '$' )
			continue;
		if( Line[1] == 'E' )
			break;
		if( Line[1] == 'M' )    /* 1 module description */
		{
			/* Parse file line by line */
			while( GetLine( mdc_file, Line, NULL, sizeof(Line) - 1 ) )
			{
				/* $EndMODULE */
				if( Line[0] == '$' )
					break;

				switch( Line[0] )
				{
					/* LibName */
					case 'L':       /* LibName */
						ModuleName = CONV_FROM_UTF8( StrPurge( Line + 3 ) );
						BOOST_FOREACH( FOOTPRINT& footprint, list )
						{
							if( ModuleName == footprint.m_Module )
							{
								NewMod = &footprint;
								break;
							}
						}
					break;

					/* KeyWords */
					case 'K':
						if( NewMod && (!NewMod->m_KeyWord) )
						NewMod->m_KeyWord = CONV_FROM_UTF8( StrPurge( Line + 3 ) );
					break;

					/* Doc */
					case 'C':
						if( NewMod && ( !NewMod->m_Doc ) )
						NewMod->m_Doc = CONV_FROM_UTF8( StrPurge( Line + 3 ) );
					break;
				}
			}
		} /* Parsed one module documentation */
	} /* Parsed complete library documentation */

	fclose( mdc_file );
}
