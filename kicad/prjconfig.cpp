/*************************************************************/
/** prjconfig.cpp : load and save configuration (file *.pro) */
/*************************************************************/


#ifdef KICAD_PYTHON
#include <pyhandler.h>
#endif

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "common.h"
#include "confirm.h"
#include "gestfich.h"
#include "kicad.h"
#include "protos.h"
#include "prjconfig.h"


/* Variables locales */
PARAM_CFG_WXSTRING SchematicRootFileNameCfg
(
	wxT("RootSch"),			  /* identification */
	&g_SchematicRootFileName /* Adresse du parametre */
);

PARAM_CFG_WXSTRING BoardFileNameCfg
(
	wxT("BoardNm"),		/* identification */
	&g_BoardFileName	/* Adresse du parametre */
);


PARAM_CFG_BASE * CfgParamList[] =
{
	& SchematicRootFileNameCfg,
	& BoardFileNameCfg,
	NULL
};



/*******************************************/
void WinEDA_MainFrame::Load_Prj_Config()
/*******************************************/
{
    if( !wxFileExists( m_PrjFileName ) )
    {
        wxString msg = _( "Kicad project file <" ) + m_PrjFileName +
            _( "> not found" );
        DisplayError( this, msg );
        return;
    }

    wxSetWorkingDirectory( wxPathOnly( m_PrjFileName ) );
    SetTitle( g_Main_Title + wxT( " " ) + GetBuildVersion() + wxT( " " ) +
              m_PrjFileName );
    SetLastProject( m_PrjFileName );
    m_LeftWin->ReCreateTreePrj();

    wxString msg = _( "\nWorking dir: " ) + wxGetCwd();
    msg << _( "\nProject: " ) << m_PrjFileName << wxT( "\n" );
    PrintMsg( msg );

#ifdef KICAD_PYTHON
    PyHandler::GetInstance()->TriggerEvent( wxT( "kicad::LoadProject" ),
                                            PyHandler::Convert( m_PrjFileName ) );
#endif
}


/*********************************************/
void WinEDA_MainFrame::Save_Prj_Config()
/*********************************************/
{
    wxString FullFileName;

    wxString mask( wxT( "*" ) );

    g_Prj_Config_Filename_ext = wxT( ".pro" );
    mask += g_Prj_Config_Filename_ext;
    FullFileName = m_PrjFileName;
    ChangeFileNameExt( FullFileName, g_Prj_Config_Filename_ext );

    FullFileName = EDA_FileSelector( _( "Save Project File:" ),
                                     wxGetCwd(),                /* Chemin par defaut */
                                     FullFileName,              /* nom fichier par defaut */
                                     g_Prj_Config_Filename_ext, /* extension par defaut */
                                     mask,                      /* Masque d'affichage */
                                     this,
                                     wxFD_SAVE,
                                     TRUE
                                     );

    if( FullFileName.IsEmpty() )
        return;

    /* ecriture de la configuration */
    wxGetApp().WriteProjectConfig( FullFileName, wxT( "/general" ),
                                   CfgParamList );
}
