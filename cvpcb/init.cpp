	/*******************************************************************/
	/*  CVPCB: Routines de base :										  */
	/* lecture Netliste et creation des fenetres composants et modules */
	/*******************************************************************/

#include "fctsys.h"
#include "common.h"

#include "cvpcb.h"
#include "gr_basic.h"

#include "protos.h"
#include "id.h"


/* routines locales : */


/**********************************************************/
void WinEDA_CvpcbFrame::SetNewPkg(const wxString & package)
/*********************************************************/
/*
	- Affecte un module au composant selectionne
	- Selectionne le composant suivant
*/
{
STORECMP * Composant;
int ii, NumCmp, IsNew = 1;
wxString Line;

	if ( g_BaseListeCmp == NULL ) return;

	NumCmp = m_ListCmp->GetSelection();
	if( NumCmp < 0 )
	{
		NumCmp = 0;
		m_ListCmp->SetSelection(NumCmp, TRUE);
	}

	Composant = g_BaseListeCmp;
	for ( ii = 0; Composant != NULL; Composant = Composant->Pnext, ii++ )
	{
		if ( NumCmp == ii ) break;
	}

	if ( Composant == NULL ) return;
	if ( ! Composant->m_Module.IsEmpty() ) IsNew = 0;

	Composant->m_Module = package;

	Line.Printf( CMP_FORMAT ,ii+1,
			Composant->m_Reference.GetData(), Composant->m_Valeur.GetData(),
			Composant->m_Module.GetData());
	modified = 1;
	if ( IsNew ) composants_non_affectes -= 1;

	m_ListCmp->SetString(NumCmp, Line);
	m_ListCmp->SetSelection(NumCmp, FALSE);

	// We activate next component:
	if ( NumCmp < (m_ListCmp->GetCount() - 1) ) NumCmp++;
	m_ListCmp->SetSelection(NumCmp, TRUE);

	Line.Printf( _("Components: %d (free: %d)"),
					nbcomp, composants_non_affectes);
	SetStatusText(Line,1);
}

/********************************************/
void WinEDA_CvpcbFrame::ReadNetListe()
/*******************************************/
/* Lecture de la netliste selon format, ainsi que du fichier des composants
*/
{
STORECMP * Composant;
wxString msg;
int ii;
int error_level;

	error_level = ReadSchematicNetlist();

	if ( error_level < 0 ) return;

	/* lecture des correspondances */
	loadcmp();

	if (m_ListCmp == NULL ) return;

	if ( ! NetInNameBuffer.IsEmpty() )
		wxSetWorkingDirectory( wxPathOnly(NetInNameBuffer) );

	Read_Config(NetInNameBuffer);	// relecture de la config (elle peut etre modifiée)

	listlib();
	BuildFootprintListBox();

	m_ListCmp->Clear();
	Composant = g_BaseListeCmp;

	composants_non_affectes = 0;
	for ( ii = 1;Composant != NULL; Composant = Composant->Pnext, ii++ )
		{
		msg.Printf(CMP_FORMAT ,ii,
				Composant->m_Reference.GetData(), Composant->m_Valeur.GetData(),
				Composant->m_Module.GetData());
		m_ListCmp->AppendLine(msg);
		if( Composant->m_Module.IsEmpty() ) composants_non_affectes += 1;
		}
	if ( g_BaseListeCmp )
		m_ListCmp->SetSelection(0, TRUE);

	msg.Printf(_("Componants: %d (free: %d)"), nbcomp, composants_non_affectes);
	SetStatusText(msg,1);

	/* Mise a jour du titre de la fenetre principale */
	wxString Title = g_Main_Title + wxT(" ") + GetBuildVersion();
	msg.Printf( wxT("%s [%s]"), Title.GetData(), FFileName.GetData());
	SetTitle(msg);

}


/*****************************************************************/
int WinEDA_CvpcbFrame::SaveNetList(const wxString & FullFilename)
/*****************************************************************/
/* Sauvegarde des fichiers netliste et cmp
	Le nom complet du fichier Netliste doit etre dans FFileName.
	Le nom du fichier cmp en est deduit
*/
{
wxString NetlistFullFileName = FullFilename;

	if ( NetlistFullFileName.IsEmpty() )
	{
	wxString Mask = wxT("*") + NetExtBuffer;
		if ( ! NetNameBuffer.IsEmpty() )
		{
			NetlistFullFileName = NetNameBuffer;
			ChangeFileNameExt(NetlistFullFileName, NetExtBuffer);
		}

		NetlistFullFileName = EDA_FileSelector( _("Save NetList and Components List files"),
						NetDirBuffer,		/* Chemin par defaut */
						NetlistFullFileName,			/* nom fichier par defaut */
						NetExtBuffer,		/* extension par defaut */
						Mask,				/* Masque d'affichage */
						this,
						wxFD_SAVE,
						TRUE
						);
	}
	if ( NetlistFullFileName.IsEmpty() ) return -1;

	FFileName = NetlistFullFileName;
	NetNameBuffer = NetlistFullFileName;
	if( SaveComponentList(NetlistFullFileName) == 0 )
	{
		DisplayError(this, _("Unable to create component file (.cmp)") );
		return 0;
	}

	dest = wxFopen(NetlistFullFileName, wxT("wt") );
	if( dest == 0 )
	{
		DisplayError(this, _("Unable to create netlist file") );
		return 0;
	}

	GenNetlistPcbnew() ;

	return 1;
}


/**********************************************************************/
bool WinEDA_CvpcbFrame::ReadInputNetList(const wxString & FullFileName)
/**********************************************************************/
/* Routine de selection du nom de la netliste d'entree, et de lecure de
celle-ci
*/
{
wxString Mask, Line;

	if ( FullFileName.IsEmpty()  )
		{
		if( ! NetInExtBuffer.IsEmpty() ) Mask = wxT("*") + NetInExtBuffer;
		else Mask = wxT("*.net");
		Line = EDA_FileSelector(_("Load Net List"),
					NetDirBuffer,		/* Chemin par defaut */
					NetInNameBuffer,	/* nom fichier par defaut */
					NetInExtBuffer,		/* extension par defaut */
					Mask,				/* Masque d'affichage */
					this,
					0,
					FALSE
					);
		if ( Line.IsEmpty()) return(FALSE);
		}
	else Line = FullFileName;

	NetInNameBuffer = Line;
	NetNameBuffer = Line;
	FFileName = NetInNameBuffer;

	/* Mise a jour du titre de la fenetre principale */
	Line = g_Main_Title + wxT(" ") + GetBuildVersion();
	Line += wxT(" ") + NetInNameBuffer;
	SetTitle(Line);

	ReadNetListe();

	return(TRUE);
}

