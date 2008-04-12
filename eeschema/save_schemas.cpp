    /*********************************************/
    /*	eesave.cpp  Module to Save EESchema files */
    /*********************************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"
#include "macros.h"

#include "protos.h"

#include "schframe.h"

/* Format des fichiers: Voir EELOAD.CC */

/* Fonctions externes */

/* Fonctions Locales */
static void SaveLayers(FILE *f);

/* Variables locales */

/*****************************************************************************
* Routine to save an EESchema file.											 *
* FileSave controls how the file is to be saved - under what name.			 *
* Returns TRUE if the file has been saved.									 *
*****************************************************************************/
bool WinEDA_SchematicFrame::SaveEEFile(SCH_SCREEN *screen, int FileSave)
{
    wxString msg;
    wxString Name, BakName;
    const wxChar **LibNames;
    const char * layer, *width;
    int ii, shape;
    bool Failed = FALSE;
    EDA_BaseStruct *Phead;
    Ki_PageDescr * PlotSheet;
    FILE *f;
    wxString dirbuf;

    if ( screen == NULL ) screen = (SCH_SCREEN*)GetScreen();

    /* If no name exists in the window yet - save as new. */
    if( screen->m_FileName.IsEmpty() ) FileSave = FILE_SAVE_NEW;

    switch (FileSave)
    {
        case FILE_SAVE_AS:
            dirbuf = wxGetCwd() + STRING_DIR_SEP;
            Name = MakeFileName(dirbuf, screen->m_FileName, g_SchExtBuffer);
            /* Rename the old file to a '.bak' one: */
            BakName = Name;
            if ( wxFileExists(Name) )
            {
                ChangeFileNameExt(BakName, wxT(".bak"));
                wxRemoveFile(BakName);	/* delete Old .bak file */
                if( ! wxRenameFile(Name, BakName) ){
                    DisplayError(this, wxT("Warning: unable to rename old file"), 10);
                }
            }
            break;

        case FILE_SAVE_NEW:
        {
            wxString mask = wxT("*") + g_SchExtBuffer;
            Name = EDA_FileSelector(_("Schematic files:"),
                    wxEmptyString,			/* Chemin par defaut */
                    screen->m_FileName,		/* nom fichier par defaut, et resultat */
                    g_SchExtBuffer,		/* extension par defaut */
                    mask,				/* Masque d'affichage */
                    this,
                    wxFD_SAVE,
                    FALSE
                    );
            if ( Name.IsEmpty() ) return FALSE;

            screen->m_FileName = Name;
            dirbuf = wxGetCwd() + STRING_DIR_SEP;
            Name = MakeFileName(dirbuf, Name, g_SchExtBuffer);

            break;
        }

        default: break;
    }

    if ((f = wxFopen(Name, wxT("wt"))) == NULL)
    {
        msg = _("Failed to create file ") + Name;
        DisplayError(this, msg);
        return FALSE;
    }

    msg = _("Save file ") + Name;
    Affiche_Message(msg);

    LibNames = GetLibNames();
    BakName.Empty();	// temporary buffer!
    for (ii = 0; LibNames[ii] != NULL; ii++)
    {
        if (ii > 0) BakName += wxT(",");
        BakName += LibNames[ii];
    }
    MyFree( LibNames);

    if (fprintf(f, "%s %s %d\n", EESCHEMA_FILE_STAMP,
        SCHEMATIC_HEAD_STRING, EESCHEMA_VERSION) == EOF ||
        fprintf(f, "LIBS:%s\n", CONV_TO_UTF8(BakName)) == EOF)
    {
        DisplayError(this, _("File write operation failed."));
        fclose(f);
        return FALSE;
    }

    screen->ClrModify();

    SaveLayers(f);
    /* Sauvegarde des dimensions du schema, des textes du cartouche.. */

    PlotSheet = screen->m_CurrentSheetDesc;
    fprintf(f,"$Descr %s %d %d\n",CONV_TO_UTF8(PlotSheet->m_Name),
            PlotSheet->m_Size.x, PlotSheet->m_Size.y);

    fprintf(f,"Sheet %d %d\n",screen->m_ScreenNumber, screen->m_NumberOfScreen);
    fprintf(f,"Title \"%s\"\n",CONV_TO_UTF8(screen->m_Title));
    fprintf(f,"Date \"%s\"\n",CONV_TO_UTF8(screen->m_Date));
    fprintf(f,"Rev \"%s\"\n",CONV_TO_UTF8(screen->m_Revision));
    fprintf(f,"Comp \"%s\"\n",CONV_TO_UTF8(screen->m_Company));
    fprintf(f,"Comment1 \"%s\"\n", CONV_TO_UTF8(screen->m_Commentaire1));
    fprintf(f,"Comment2 \"%s\"\n", CONV_TO_UTF8(screen->m_Commentaire2));
    fprintf(f,"Comment3 \"%s\"\n", CONV_TO_UTF8(screen->m_Commentaire3));
    fprintf(f,"Comment4 \"%s\"\n", CONV_TO_UTF8(screen->m_Commentaire4));

    fprintf(f,"$EndDescr\n");

    /* Sauvegarde des elements du dessin */
    Phead = screen->EEDrawList;
    while (Phead)
        {
        switch(Phead->Type())
            {
            case TYPE_SCH_COMPONENT:		  /* Its a library item. */
                ((SCH_COMPONENT *) Phead)->Save( f );
                break;

            case DRAW_SHEET_STRUCT_TYPE:	   /* Its a Sheet item. */
                ((DrawSheetStruct *) Phead)->Save( f );
                break;

            case DRAW_SEGMENT_STRUCT_TYPE:		 /* Its a Segment item. */
                #undef STRUCT
                #define STRUCT ((EDA_DrawLineStruct *) Phead)
                layer = "Notes"; width = "Line";
                if (STRUCT->m_Layer == LAYER_WIRE) layer = "Wire";
                if (STRUCT->m_Layer == LAYER_BUS) layer = "Bus";
                if( STRUCT->m_Width != GR_NORM_WIDTH) layer = "Bus";
                if (fprintf(f, "Wire %s %s\n", layer, width ) == EOF)
                    {
                    Failed = TRUE; break;
                    }
                if (fprintf(f, "\t%-4d %-4d %-4d %-4d\n",
                        STRUCT->m_Start.x,STRUCT->m_Start.y,
                        STRUCT->m_End.x,STRUCT->m_End.y) == EOF)
                    {
                    Failed = TRUE; break;
                    }
                break;

            case DRAW_BUSENTRY_STRUCT_TYPE:		 /* Its a Raccord item. */
                #undef STRUCT
                #define STRUCT ((DrawBusEntryStruct *) Phead)
                layer = "Wire"; width = "Line";
                if (STRUCT->m_Layer == LAYER_BUS)
                    {
                    layer = "Bus"; width = "Bus";
                    }

                if (fprintf(f, "Entry %s %s\n", layer, width) == EOF)
                    {
                    Failed = TRUE; break;
                    }
                if( fprintf(f, "\t%-4d %-4d %-4d %-4d\n",
                        STRUCT->m_Pos.x,STRUCT->m_Pos.y,
                        STRUCT->m_End().x,STRUCT->m_End().y) == EOF)
                        {
                        Failed = TRUE; break;
                        }
                break;

            case DRAW_POLYLINE_STRUCT_TYPE:		  /* Its a polyline item. */
                #undef STRUCT
                #define STRUCT ((DrawPolylineStruct *) Phead)
                layer = "Notes"; width = "Line";
                if (STRUCT->m_Layer == LAYER_WIRE) layer = "Wire";
                if (STRUCT->m_Layer == LAYER_BUS) layer = "Bus";
                if( STRUCT->m_Width != GR_NORM_WIDTH) width = "Bus";
                if (fprintf(f, "Poly %s %s %d\n",
                            width, layer, STRUCT->m_NumOfPoints) == EOF)
                    {
                    Failed = TRUE; break;
                    }
                for (ii = 0; ii < STRUCT->m_NumOfPoints; ii++)
                    {
                    if (fprintf(f, "\t%-4d %-4d\n",
                        STRUCT->m_Points[ii*2],
                        STRUCT->m_Points[ii*2+1]) == EOF)
                        {
                        Failed = TRUE;
                        break;
                        }
                    }
                break;

            case DRAW_JUNCTION_STRUCT_TYPE:	/* Its a connection item. */
                #undef STRUCT
                #define STRUCT ((DrawJunctionStruct *) Phead)
                if (fprintf(f, "Connection ~ %-4d %-4d\n",
                    STRUCT->m_Pos.x, STRUCT->m_Pos.y) == EOF)
                    {
                    Failed = TRUE;
                    }
                break;

            case DRAW_NOCONNECT_STRUCT_TYPE:	/* Its a NoConnection item. */
                #undef STRUCT
                #define STRUCT ((DrawNoConnectStruct *) Phead)
                if (fprintf(f, "NoConn ~ %-4d %-4d\n",
                    STRUCT->m_Pos.x, STRUCT->m_Pos.y) == EOF)
                    {
                    Failed = TRUE;
                    }
                break;

            case TYPE_SCH_TEXT:			/* Its a text item. */
                #undef STRUCT
                #define STRUCT ((SCH_TEXT *) Phead)
                if (fprintf(f, "Text Notes %-4d %-4d %-4d %-4d ~\n%s\n",
                        STRUCT->m_Pos.x, STRUCT->m_Pos.y,
                        STRUCT->m_Orient, STRUCT->m_Size.x,
                        CONV_TO_UTF8(STRUCT->m_Text)) == EOF)
                    Failed = TRUE;
                break;


            case TYPE_SCH_LABEL:		/* Its a label item. */
                #undef STRUCT
                #define STRUCT ((SCH_LABEL *) Phead)
                shape = '~';
                if (fprintf(f, "Text Label %-4d %-4d %-4d %-4d %c\n%s\n",
                        STRUCT->m_Pos.x, STRUCT->m_Pos.y,
                        STRUCT->m_Orient, STRUCT->m_Size.x, shape,
                        CONV_TO_UTF8(STRUCT->m_Text)) == EOF)
                    Failed = TRUE;
                break;

            case TYPE_SCH_GLOBALLABEL: /* Its a Global label item. */
                #undef STRUCT
                #define STRUCT ((SCH_GLOBALLABEL *) Phead)
                shape = STRUCT->m_Shape;
                if (fprintf(f, "Text GLabel %-4d %-4d %-4d %-4d %s\n%s\n",
                        STRUCT->m_Pos.x, STRUCT->m_Pos.y,
                        STRUCT->m_Orient,	STRUCT->m_Size.x,
                        SheetLabelType[shape],
                        CONV_TO_UTF8(STRUCT->m_Text)) == EOF)
                    Failed = TRUE;
                break;

            case TYPE_SCH_HIERLABEL: /* Its a Hierarchical label item. */
                #undef STRUCT
                #define STRUCT ((SCH_HIERLABEL *) Phead)
                shape = STRUCT->m_Shape;
                if (fprintf(f, "Text HLabel %-4d %-4d %-4d %-4d %s\n%s\n",
                        STRUCT->m_Pos.x, STRUCT->m_Pos.y,
                        STRUCT->m_Orient,	STRUCT->m_Size.x,
                        SheetLabelType[shape],
                        CONV_TO_UTF8(STRUCT->m_Text)) == EOF)
                    Failed = TRUE;
                break;

            case DRAW_MARKER_STRUCT_TYPE:	/* Its a marker item. */
                #undef STRUCT
                #define STRUCT ((DrawMarkerStruct *) Phead)
                if( STRUCT->GetComment() ) msg = STRUCT->GetComment();
                else msg.Empty();
                if (fprintf(f, "Kmarq %c %-4d %-4d \"%s\" F=%X\n",
                                (int) STRUCT->m_Type + 'A',
                                STRUCT->m_Pos.x, STRUCT->m_Pos.y,
                                CONV_TO_UTF8(msg), STRUCT->m_MarkFlags) == EOF)
                    {
                    Failed = TRUE;
                    }
                break;

            case DRAW_SHEETLABEL_STRUCT_TYPE :
            case DRAW_PICK_ITEM_STRUCT_TYPE :
                break;

            default:
                break;
            }

        if (Failed)
            {
            DisplayError(this, _("File write operation failed."));
            break;
            }

        Phead = Phead->Pnext;
        }
    if (fprintf(f, "$EndSCHEMATC\n") == EOF) Failed = TRUE;

    fclose(f);

    if (FileSave == FILE_SAVE_NEW) screen->m_FileName = Name;

    return !Failed;
}


/****************************/
static void SaveLayers(FILE *f)
/****************************/
/* Save a Layer Structure to a file
theses infos are not used in eeschema
*/
{
    fprintf(f,"EELAYER %2d %2d\n", g_LayerDescr.NumberOfLayers,g_LayerDescr.CurrentLayer);
    fprintf(f,"EELAYER END\n");
}


