	/***************/
	/* genstf()  */
	/***************/

/* genere le fichier STF type 'ref' 'nom_empreinte' pour DRAFT */

#include "fctsys.h"

#include "wxstruct.h"
#include "common.h"
#include "cvpcb.h"

#include "protos.h"

void WinEDA_CvpcbFrame::WriteStuffList(wxCommandEvent & event)
{
FILE * FileEquiv;
STORECMP * Cmp;
wxString Line, FullFileName, Mask;

	if( nbcomp <= 0 ) return;

	/* calcul du nom du fichier */
	Mask = wxT("*") + ExtRetroBuffer;
	FullFileName = FFileName;
	ChangeFileNameExt(FullFileName, ExtRetroBuffer);

	FullFileName = EDA_FileSelector( wxT("Create Stuff File"),
					wxGetCwd(),					/* Chemin par defaut */
					FullFileName,		/* nom fichier par defaut */
					ExtRetroBuffer,		/* extension par defaut */
					Mask,				/* Masque d'affichage */
					this,
					wxFD_SAVE,
					TRUE
					);
	if ( FullFileName.IsEmpty() ) return;

	FileEquiv = wxFopen(FullFileName, wxT("wt") );
	if (FileEquiv == 0 )
	{
		Line = _("Unable to create ") + FullFileName;
		DisplayError(this, Line,30);
		return;
	}

	/* Generation de la liste */
	for (Cmp = g_BaseListeCmp ; Cmp != NULL ; Cmp = Cmp->Pnext )
	{
		/* génération du composant si son empreinte est définie */
		if ( Cmp->m_Module.IsEmpty() ) continue;
		fprintf(FileEquiv, "comp = \"%s\" module = \"%s\"\n",
				CONV_TO_UTF8(Cmp->m_Reference),
				CONV_TO_UTF8(Cmp->m_Module));
	}

	fclose(FileEquiv);
}

