/* read_from_file_schematic_items_descriptions.cpp */

/*functions to read schematic items descriptions from file
 */
#include "fctsys.h"
#include "common.h"
#include "confirm.h"
#include "kicad_string.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"

#include "protos.h"


SCH_ITEM* ReadTextDescr( FILE*     aFile,
                         wxString& aMsgDiag,
                         char*     aLine,
                         int       aBufsize,
                         int*      aLineNum,
                         int       aSchematicFileVersion )
{
/**
 * Function ReadTextDescr
 * Reads the data structures for a Text (Comment, label, Hlabel and Hlabel
 * from a FILE in "*.brd" format.
 * @param aFile The FILE to read.
 * @param aLine The buffer used to read the first line of description.
 * @param aBufsize The size of aLine.
 * @param aLineNum a pointer to the line count.
 * @return a poiner to the new created obect if success reading else NULL.
 */
    SCH_ITEM* Struct = NULL;
    char      Name1[256];
    char      Name2[256];
    char      Name3[256];
    int       thickness = 0, size = 0, orient = 0;
    wxPoint   pos;

    char*     SLine = aLine;

    while( (*SLine != ' ' ) && *SLine )
        SLine++;

    // SLine points the start of parameters

    Name1[0] = 0; Name2[0] = 0; Name3[0] = 0;
    int ii = sscanf( SLine, "%s %d %d %d %d %s %s %d", Name1, &pos.x, &pos.y,
                     &orient, &size, Name2, Name3, &thickness );

    if( ii < 4 )
    {
        aMsgDiag.Printf(
            wxT( "EESchema file text struct error line %d, aborted" ),
            *aLineNum );
        return NULL;
    }

    if( feof( aFile ) || GetLine( aFile, aLine, aLineNum, aBufsize ) == NULL )
    {
        aMsgDiag.Printf(
            wxT( "EESchema file text struct error line %d (No text), aborted" ),
            *aLineNum );
        return NULL;
    }
    if( size == 0 )
        size = DEFAULT_SIZE_TEXT;
    char* text = strtok( aLine, "\n\r" );
    if( text == NULL )
        return NULL;

    if( Name1[0] == 'L' )
    {
        SCH_LABEL* TextStruct =
            new SCH_LABEL( pos, CONV_FROM_UTF8( text ) );

        TextStruct->m_Size.x = TextStruct->m_Size.y = size;
        TextStruct->m_Orient = orient;
        if( isdigit( Name3[0] ) )
        {
            thickness = atol( Name3 );
            TextStruct->m_Width = thickness;
        }
        Struct = TextStruct;
        if( stricmp( Name2, "Italic" ) == 0 )
            TextStruct->m_Italic = 1;
    }
    else if( Name1[0] == 'G' && aSchematicFileVersion > '1' )
    {
        SCH_GLOBALLABEL* TextStruct =
            new SCH_GLOBALLABEL( pos, CONV_FROM_UTF8( text ) );

        Struct = TextStruct;
        TextStruct->m_Size.x = TextStruct->m_Size.y = size;
        TextStruct->m_Orient = orient;
        TextStruct->m_Shape  = NET_INPUT;
        TextStruct->m_Width  = thickness;

        if( stricmp( Name2, SheetLabelType[NET_OUTPUT] ) == 0 )
            TextStruct->m_Shape = NET_OUTPUT;
        if( stricmp( Name2, SheetLabelType[NET_BIDI] ) == 0 )
            TextStruct->m_Shape = NET_BIDI;
        if( stricmp( Name2, SheetLabelType[NET_TRISTATE] ) == 0 )
            TextStruct->m_Shape = NET_TRISTATE;
        if( stricmp( Name2, SheetLabelType[NET_UNSPECIFIED] ) == 0 )
            TextStruct->m_Shape = NET_UNSPECIFIED;
        if( stricmp( Name3, "Italic" ) == 0 )
            TextStruct->m_Italic = 1;
    }
    else if( (Name1[0] == 'H')
            || (Name1[0] == 'G' && aSchematicFileVersion == '1') ) //in schematic file version 1, glabels were actually hierarchal labels.
    {
        SCH_HIERLABEL* TextStruct =
            new SCH_HIERLABEL( pos, CONV_FROM_UTF8( text ) );

        Struct = TextStruct;
        TextStruct->m_Size.x = TextStruct->m_Size.y = size;
        TextStruct->m_Orient = orient;
        TextStruct->m_Shape  = NET_INPUT;
        TextStruct->m_Width  = thickness;

        if( stricmp( Name2, SheetLabelType[NET_OUTPUT] ) == 0 )
            TextStruct->m_Shape = NET_OUTPUT;
        if( stricmp( Name2, SheetLabelType[NET_BIDI] ) == 0 )
            TextStruct->m_Shape = NET_BIDI;
        if( stricmp( Name2, SheetLabelType[NET_TRISTATE] ) == 0 )
            TextStruct->m_Shape = NET_TRISTATE;
        if( stricmp( Name2, SheetLabelType[NET_UNSPECIFIED] ) == 0 )
            TextStruct->m_Shape = NET_UNSPECIFIED;
        if( stricmp( Name3, "Italic" ) == 0 )
            TextStruct->m_Italic = 1;
    }
    else
    {
        SCH_TEXT* TextStruct =
            new SCH_TEXT( pos, CONV_FROM_UTF8( text ) );

        TextStruct->m_Size.x = TextStruct->m_Size.y = size;
        TextStruct->m_Orient = orient;
        if( isdigit( Name3[0] ) )
        {
            thickness = atol( Name3 );
            TextStruct->m_Width = thickness;
        }

        if( strnicmp( Name2, "Italic", 6 ) == 0 )
            TextStruct->m_Italic = 1;
        Struct = TextStruct;
    }

    return Struct;
}


/* Fonction utilisee par LoadEEFile().
 *  Lit les lignes relatives a la description d'une feuille de hierarchie
 */
int ReadSheetDescr( wxWindow* frame, char* Line, FILE* f, wxString& aMsgDiag,
                    int* aLineNum, BASE_SCREEN* Window )
{
    int ii, fieldNdx, size;
    char             Name1[256], Char1[256], Char2[256];
    DrawSheetStruct* SheetStruct;
    Hierarchical_PIN_Sheet_Struct* SheetLabelStruct, * OldSheetLabel = NULL;
    int              Failed = FALSE;
    char*            ptcar;

    SheetStruct = new DrawSheetStruct();

    SheetStruct->m_TimeStamp = GetTimeStamp();

    //sheets are added to the EEDrawList like other schematic components.
    //however, in order to preserve the hierarchy (through m_Parent pointers),
    //a duplicate of the sheet is added to m_SubSheet array.
    //must be a duplicate, references just work for a two-layer structure.
    //this is accomplished through the Sync() function.

    if( Line[0] == '$' )   /* Ligne doit etre "$Sheet" */
    {
        *aLineNum++;
        if( fgets( Line, 256 - 1, f ) == 0 )
        {
            aMsgDiag.Printf( wxT( "Read File Errror" ) );
            return TRUE;
        }
    }

    /* Next line: must be "S xx yy nn mm" with xx, yy = sheet position
     *  ( upper left corner  ) et nn,mm = sheet size */
    if( (sscanf( &Line[1], "%d %d %d %d",
                 &SheetStruct->m_Pos.x, &SheetStruct->m_Pos.y,
                 &SheetStruct->m_Size.x, &SheetStruct->m_Size.y ) != 4)
       || (Line[0] != 'S' ) )
    {
        aMsgDiag.Printf(
            wxT( " ** EESchema file sheet struct error at line %d, aborted\n" ),
            *aLineNum );
        aMsgDiag << CONV_FROM_UTF8( Line );
        Failed = TRUE;
        return Failed;
    }

    /* Read fields */
    for( ; ; ) /* Analyse des lignes "Fn "texte" .." */
    {
        *aLineNum++;
        if( fgets( Line, 256 - 1, f ) == NULL )
            return TRUE;
        if( Line[0] == 'U' )
        {
            sscanf( Line + 1, "%lX", &(SheetStruct->m_TimeStamp) );
            if( SheetStruct->m_TimeStamp == 0 )  //zero is not unique!
                SheetStruct->m_TimeStamp = GetTimeStamp();
            continue;
        }
        if( Line[0] != 'F' )
            break;
        sscanf( Line + 1, "%d", &fieldNdx );

        /* Lecture du champ :
         *  si fieldNdx >= 2 :  Fn "texte" t s posx posy
         *  sinon F0 "texte" pour sheetname
         *  et F1 "texte" pour filename */

        ptcar = Line; while( *ptcar && (*ptcar != '"') )
            ptcar++;

        if( *ptcar != '"' )
        {
            aMsgDiag.Printf(
                wxT( "EESchema file sheet label F%d at line %d, aborted\n" ),
                fieldNdx, *aLineNum );
            aMsgDiag << CONV_FROM_UTF8( Line );
            return TRUE;
        }

        for( ptcar++, ii = 0; ; ii++, ptcar++ )
        {
            Name1[ii] = *ptcar;
            if( *ptcar == 0 )
            {
                aMsgDiag.Printf(
                    wxT( "EESchema file sheet field F at line %d, aborted\n" ),
                    *aLineNum );
                aMsgDiag << CONV_FROM_UTF8( Line );
                return TRUE;
            }
            if( *ptcar == '"' )
            {
                Name1[ii] = 0; ptcar++;
                break;
            }
        }

        if( ( fieldNdx == 0 ) || ( fieldNdx == 1 ) )
        {
            if( sscanf( ptcar, "%d", &size ) != 1 )
            {
                aMsgDiag.Printf( wxT( "EESchema file sheet Label Caract " \
                                      "error line %d, aborted\n" ), *aLineNum );
                aMsgDiag << CONV_FROM_UTF8( Line );
                DisplayError( frame, aMsgDiag );
            }
            if( size == 0 )
                size = DEFAULT_SIZE_TEXT;
            if( fieldNdx == 0 )
            {
                SheetStruct->m_SheetName     = CONV_FROM_UTF8( Name1 );
                SheetStruct->m_SheetNameSize = size;
            }
            else
            {
                SheetStruct->SetFileName( CONV_FROM_UTF8( Name1 ) );

                //printf( "in ReadSheetDescr : SheetStruct->m_FileName = %s \n",
                //        Name1 );
                SheetStruct->m_FileNameSize = size;
            }
        }

        if( fieldNdx > 1 )
        {
            SheetLabelStruct =
                new Hierarchical_PIN_Sheet_Struct( SheetStruct, wxPoint( 0, 0 ),
                                                   CONV_FROM_UTF8( Name1 ) );

            if( SheetStruct->m_Label == NULL )
                OldSheetLabel = SheetStruct->m_Label = SheetLabelStruct;
            else
                OldSheetLabel->SetNext( (EDA_BaseStruct*) SheetLabelStruct );
            OldSheetLabel = SheetLabelStruct;

            /* Lecture des coordonnees */
            if( sscanf( ptcar, "%s %s %d %d %d", Char1, Char2,
                        &SheetLabelStruct->m_Pos.x, &SheetLabelStruct->m_Pos.y,
                        &size ) != 5 )
            {
                aMsgDiag.Printf( wxT( "EESchema file Sheet Label Caract " \
                                      "error line %d, aborted\n" ), *aLineNum );
                aMsgDiag << CONV_FROM_UTF8( Line );
                DisplayError( frame, aMsgDiag );
                continue;
            }

            if( size == 0 )
                size = DEFAULT_SIZE_TEXT;
            SheetLabelStruct->m_Size.x = SheetLabelStruct->m_Size.y = size;

            /* Mise a jour des cadrage et type */
            switch( Char1[0] )
            {
            case 'I':
                SheetLabelStruct->m_Shape = NET_INPUT; break;

            case 'O':
                SheetLabelStruct->m_Shape = NET_OUTPUT; break;

            case 'B':
                SheetLabelStruct->m_Shape = NET_BIDI; break;

            case 'T':
                SheetLabelStruct->m_Shape = NET_TRISTATE; break;

            case 'U':
                SheetLabelStruct->m_Shape = NET_UNSPECIFIED; break;
            }

            if( Char2[0] == 'R' )
                SheetLabelStruct->m_Edge = 1;
        }
    }

    if( strnicmp( "$End", Line, 4 ) != 0 )
    {
        aMsgDiag.Printf( wxT( "**EESchema file end_sheet struct error at " \
                              "line %d, aborted\n" ), *aLineNum );
        aMsgDiag << CONV_FROM_UTF8( Line );
        Failed = TRUE;
    }
    if( !Failed )
    {
        SheetStruct->SetNext( Window->EEDrawList );
        Window->EEDrawList = SheetStruct;
        SheetStruct->SetParent( Window );
    }
    return Failed;   /* Fin lecture 1 composant */
}


/******************************************************************/
/* Analyse de l'entete du schema ( dims feuille, cartouche..)
 */
/******************************************************************/
bool ReadSchemaDescr( wxWindow* frame, char* Line, FILE* f, wxString& aMsgDiag,
                      int* aLineNum, BASE_SCREEN* Window )
{
    char Text[256], buf[1024];
    int  ii;
    Ki_PageDescr*        wsheet = &g_Sheet_A4;
    static Ki_PageDescr* SheetFormatList[] =
    {
        &g_Sheet_A4,   &g_Sheet_A3,   &g_Sheet_A2,   &g_Sheet_A1, &g_Sheet_A0,
        &g_Sheet_A,    &g_Sheet_B,    &g_Sheet_C,    &g_Sheet_D,  &g_Sheet_E,
        &g_Sheet_user, NULL
    };
    wxSize               PageSize;

    sscanf( Line, "%s %s %d %d", Text, Text, &PageSize.x, &PageSize.y );
    /* Recherche de la descr correspondante: */
    wxString pagename = CONV_FROM_UTF8( Text );
    for( ii = 0; SheetFormatList[ii] != NULL; ii++ )
    {
        wsheet = SheetFormatList[ii];
        if( wsheet->m_Name.CmpNoCase( pagename ) == 0 ) /* Descr found ! */
        {
            // Get the user page size and make it the default
            if( wsheet == &g_Sheet_user )
            {
                g_Sheet_user.m_Size = PageSize;
            }
            break;
        }
    }

    if( SheetFormatList[ii] == NULL )
    {
        /* Erreur ici: descr non trouvee */
        aMsgDiag.Printf( wxT( "EESchema file Dims Caract error line %d, " \
                              "aborted\n" ), *aLineNum );
        aMsgDiag << CONV_FROM_UTF8( Line );
        DisplayError( frame, aMsgDiag );
    }

    /* Ajuste ecran */
    Window->m_CurrentSheetDesc = wsheet;

    /* Recheche suite et fin de descr */
    for( ; ; )
    {
        if( GetLine( f, Line, aLineNum, 1024 ) == NULL )
            return TRUE;
        if( strnicmp( Line, "$End", 4 ) == 0 )
            break;

        if( strnicmp( Line, "Sheet", 2 ) == 0 )
            sscanf( Line + 5, " %d %d",
                    &Window->m_ScreenNumber, &Window->m_NumberOfScreen );

        if( strnicmp( Line, "Title", 2 ) == 0 )
        {
            ReadDelimitedText( buf, Line, 256 );
            Window->m_Title = CONV_FROM_UTF8( buf );
            continue;
        }

        if( strnicmp( Line, "Date", 2 ) == 0 )
        {
            ReadDelimitedText( buf, Line, 256 );
            Window->m_Date = CONV_FROM_UTF8( buf );
            continue;
        }

        if( strnicmp( Line, "Rev", 2 ) == 0 )
        {
            ReadDelimitedText( buf, Line, 256 );
            Window->m_Revision = CONV_FROM_UTF8( buf );
            continue;
        }

        if( strnicmp( Line, "Comp", 4 ) == 0 )
        {
            ReadDelimitedText( buf, Line, 256 );
            Window->m_Company = CONV_FROM_UTF8( buf );
            continue;
        }

        if( strnicmp( Line, "Comment1", 8 ) == 0 )
        {
            ReadDelimitedText( buf, Line, 256 );
            Window->m_Commentaire1 = CONV_FROM_UTF8( buf );
            continue;
        }

        if( strnicmp( Line, "Comment2", 8 ) == 0 )
        {
            ReadDelimitedText( buf, Line, 256 );
            Window->m_Commentaire2 = CONV_FROM_UTF8( buf );
            continue;
        }

        if( strnicmp( Line, "Comment3", 8 ) == 0 )
        {
            ReadDelimitedText( buf, Line, 256 );
            Window->m_Commentaire3 = CONV_FROM_UTF8( buf );
            continue;
        }

        if( strnicmp( Line, "Comment4", 8 ) == 0 )
        {
            ReadDelimitedText( buf, Line, 256 );
            Window->m_Commentaire4 = CONV_FROM_UTF8( buf );
            continue;
        }
    }

    return false;
}


/* Fonction utilisee par LoadEEFile().
 *  Lit les lignes relatives a la description d'un composant en schema
 */
int ReadPartDescr( wxWindow* frame, char* Line, FILE* f, wxString& aMsgDiag,
                   int* aLineNum, BASE_SCREEN* Window )
{
    int            ii;
    char           Name1[256], Name2[256],
                   Char1[256], Char2[256], Char3[256];
    SCH_COMPONENT* component;
    int            Failed = 0, newfmt = 0;
    char*          ptcar;
    wxString       fieldName;


    component = new SCH_COMPONENT();

    component->m_Convert = 1;

    if( Line[0] == '$' )
    {
        newfmt = 1;
        *aLineNum++;
        if( fgets( Line, 256 - 1, f ) == 0 )
            return TRUE;
    }

    /* Traitement de la 1ere ligne de description */
    if( sscanf( &Line[1], "%s %s", Name1, Name2 ) != 2 )
    {
        aMsgDiag.Printf(
            wxT( "EESchema Component descr error at line %d, aborted" ),
            *aLineNum );
        aMsgDiag << wxT( "\n" ) << CONV_FROM_UTF8( Line );
        Failed = TRUE;
        return Failed;
    }

    if( strcmp( Name1, NULL_STRING ) != 0 )
    {
        for( ii = 0; ii < (int) strlen( Name1 ); ii++ )
            if( Name1[ii] == '~' )
                Name1[ii] = ' ';

        component->m_ChipName = CONV_FROM_UTF8( Name1 );
        if( !newfmt )
            component->GetField( VALUE )->m_Text = CONV_FROM_UTF8( Name1 );
    }
    else
    {
        component->m_ChipName.Empty();
        component->GetField( VALUE )->m_Text.Empty();
        component->GetField( VALUE )->m_Orient    = TEXT_ORIENT_HORIZ;
        component->GetField( VALUE )->m_Attributs = TEXT_NO_VISIBLE;
    }

    if( strcmp( Name2, NULL_STRING ) != 0 )
    {
        bool isDigit = false;
        for( ii = 0; ii < (int) strlen( Name2 ); ii++ )
        {
            if( Name2[ii] == '~' )
                Name2[ii] = ' ';

            // get RefBase from this, too. store in Name1.
            if( Name2[ii] >= '0' && Name2[ii] <= '9' )
            {
                isDigit   = true;
                Name1[ii] = 0;  //null-terminate.
            }
            if( !isDigit )
            {
                Name1[ii] = Name2[ii];
            }
        }

        Name1[ii] = 0; //just in case
        int  jj;
        for( jj = 0; jj<ii && Name1[jj] == ' '; jj++ )
            ;

        if( jj == ii )
        {
            // blank string.
            component->m_PrefixString = wxT( "U" );
        }
        else
        {
            component->m_PrefixString = CONV_FROM_UTF8( &Name1[jj] );

            //printf("prefix: %s\n", CONV_TO_UTF8(component->m_PrefixString));
        }
        if( !newfmt )
            component->GetField( REFERENCE )->m_Text = CONV_FROM_UTF8( Name2 );
    }
    else
    {
        component->GetField( REFERENCE )->m_Attributs = TEXT_NO_VISIBLE;
    }

    /* Traitement des autres lignes de description */

    /* Ces lignes commencent par:
     *  "P " = position
     *  "U " = Num Unit, et Conversion
     *  "Fn" = Champs ( n = 0.. = numero de champ )
     *	"Ar" = AlternateReference, in the case of multiple sheets
     *			referring to one schematic file.
     */

    /* Lecture des champs */
    for( ; ; )
    {
        *aLineNum++;
        if( fgets( Line, 256 - 1, f ) == NULL )
            return TRUE;

        if( Line[0] == 'U' ) /* Lecture num multi, conversion et time stamp */
        {
            sscanf( Line + 1, "%d %d %lX",
                    &component->m_Multi, &component->m_Convert,
                    &component->m_TimeStamp );
        }
        else if( Line[0] == 'P' )
        {
            sscanf( Line + 1, "%d %d",
                    &component->m_Pos.x, &component->m_Pos.y );

            // Set fields position to a default position (that is the component position
            // For existing fields, the real position will be set later
            for( int i = 0; i<component->GetFieldCount();  ++i )
            {
                if( component->GetField( i )->m_Text.IsEmpty() )
                    component->GetField( i )->m_Pos = component->m_Pos;
            }
        }
        else if( Line[0] == 'A' && Line[1] == 'R' )
        {
            /* format:
             * AR Path="/9086AF6E/67452AA0" Ref="C99" Part="1"
             * where 9086AF6E is the unique timestamp of the containing sheet
             * and 67452AA0 is the timestamp of this component.
             * C99 is the reference given this path.
             */
            int ii;
            ptcar = Line + 2;

            //copy the path.
            ii     = ReadDelimitedText( Name1, ptcar, 255 );
            ptcar += ii + 1;
            wxString path = CONV_FROM_UTF8( Name1 );

            // copy the reference
            ii     = ReadDelimitedText( Name1, ptcar, 255 );
            ptcar += ii + 1;
            wxString ref = CONV_FROM_UTF8( Name1 );

            // copy the multi, if exists
            ii = ReadDelimitedText( Name1, ptcar, 255 );
            if( Name1[0] == 0 )  // Nothing read, put a default value
                sprintf( Name1, "%d", component->m_Multi );
            int multi = atoi( Name1 );
            if( multi < 0 || multi > 25 )
                multi = 1;
            component->AddHierarchicalReference( path, ref, multi );
            component->GetField( REFERENCE )->m_Text = ref;
        }
        else if( Line[0] == 'F' )
        {
            int  fieldNdx;

            char FieldUserName[1024];
            GRTextHorizJustifyType hjustify = GR_TEXT_HJUSTIFY_CENTER;
            GRTextVertJustifyType  vjustify = GR_TEXT_VJUSTIFY_CENTER;

            FieldUserName[0] = 0;

            /* Lecture du champ */
            ptcar = Line;

            while( *ptcar && (*ptcar != '"') )
                ptcar++;

            if( *ptcar != '"' )
            {
                aMsgDiag.Printf(
                    wxT( "EESchema file lib field F at line %d, aborted" ),
                    *aLineNum );
                return TRUE;
            }

            for( ptcar++, ii = 0; ; ii++, ptcar++ )
            {
                Name1[ii] = *ptcar;
                if( *ptcar == 0 )
                {
                    aMsgDiag.Printf(
                        wxT( "Component field F at line %d, aborted" ),
                        *aLineNum );
                    return TRUE;
                }

                if( *ptcar == '"' )
                {
                    Name1[ii] = 0;
                    ptcar++;
                    break;
                }
            }

            fieldNdx = atoi( Line + 2 );

            ReadDelimitedText( FieldUserName, ptcar, sizeof(FieldUserName) );

            if( !FieldUserName[0] )
                fieldName = ReturnDefaultFieldName( fieldNdx );
            else
                fieldName = CONV_FROM_UTF8( FieldUserName );

            if( fieldNdx >= component->GetFieldCount() )
            {
                // add as many fields as needed so the m_FieldId's are
                // contiguous, no gaps.
                while( fieldNdx >= component->GetFieldCount() )
                {
                    int           newNdx = component->GetFieldCount();

                    SCH_CMP_FIELD field( wxPoint( 0, 0 ), newNdx, component,
                                         fieldName );
                    component->AddField( field );
                }
            }
            else
            {
                component->GetField( fieldNdx )->m_Name = fieldName;
            }

            component->GetField( fieldNdx )->m_Text = CONV_FROM_UTF8( Name1 );
            memset( Char3, 0, sizeof(Char3) );
            if( ( ii = sscanf( ptcar, "%s %d %d %d %X %s %s", Char1,
                               &component->GetField( fieldNdx )->m_Pos.x,
                               &component->GetField( fieldNdx )->m_Pos.y,
                               &component->GetField( fieldNdx )->m_Size.x,
                               &component->GetField( fieldNdx )->m_Attributs,
                               Char2, Char3 ) ) < 4 )
            {
                aMsgDiag.Printf(
                    wxT( "Component Field error line %d, aborted" ),
                    *aLineNum );
                DisplayError( frame, aMsgDiag );
                continue;
            }

            if( (component->GetField( fieldNdx )->m_Size.x == 0 ) || (ii == 4) )
                component->GetField( fieldNdx )->m_Size.x = DEFAULT_SIZE_TEXT;

            component->GetField( fieldNdx )->m_Orient = TEXT_ORIENT_HORIZ;
            component->GetField( fieldNdx )->m_Size.y =
                component->GetField( fieldNdx )->m_Size.x;

            if( Char1[0] == 'V' )
                component->GetField( fieldNdx )->m_Orient = TEXT_ORIENT_VERT;

            if( ii >= 7 )
            {
                if( *Char2 == 'L' )
                    hjustify = GR_TEXT_HJUSTIFY_LEFT;
                else if( *Char2 == 'R' )
                    hjustify = GR_TEXT_HJUSTIFY_RIGHT;
                if( Char3[0] == 'B' )
                    vjustify = GR_TEXT_VJUSTIFY_BOTTOM;
                else if( Char3[0] == 'T' )
                    vjustify = GR_TEXT_VJUSTIFY_TOP;
                if( Char3[1] == 'I' )
                    component->GetField( fieldNdx )->m_Italic = true;
                else
                    component->GetField( fieldNdx )->m_Italic = false;
                if( Char3[2] == 'B' )
                    component->GetField( fieldNdx )->m_Width =
                        component->GetField( fieldNdx )->m_Size.x / 4;
                else
                    component->GetField( fieldNdx )->m_Width = 0;

                component->GetField( fieldNdx )->m_HJustify = hjustify;
                component->GetField( fieldNdx )->m_VJustify = vjustify;
            }

            if( fieldNdx == REFERENCE )
                if( component->GetField( fieldNdx )->m_Text[0] == '#' )
                    component->GetField( fieldNdx )->m_Attributs |=
                        TEXT_NO_VISIBLE;
        }
        else
            break;
    }

    /* Lecture multi et position du composant */
    if( sscanf( Line, "%d %d %d", &component->m_Multi,
                &component->m_Pos.x, &component->m_Pos.y ) != 3 )
    {
        aMsgDiag.Printf(
            wxT( "Component unit & pos error at line %d, aborted" ),
            *aLineNum );
        Failed = TRUE;
        return Failed;
    }

    /* Lecture de la matrice de miroir / rotation */
    *aLineNum++;
    if( (fgets( Line, 256 - 1, f ) == NULL)
       || (sscanf( Line, "%d %d %d %d",
                   &component->m_Transform[0][0],
                   &component->m_Transform[0][1],
                   &component->m_Transform[1][0],
                   &component->m_Transform[1][1] ) != 4) )
    {
        aMsgDiag.Printf(
            wxT( "Component orient error at line %d, aborted" ),
            *aLineNum );
        Failed = TRUE;
        return Failed;
    }

    if( newfmt )
    {
        *aLineNum++;
        if( fgets( Line, 256 - 1, f ) == NULL )
            return TRUE;
        if( strnicmp( "$End", Line, 4 ) != 0 )
        {
            aMsgDiag.Printf(
                wxT( "Component End expected at line %d, aborted" ),
                *aLineNum );
            Failed = TRUE;
        }
    }

    if( !Failed )
    {
        component->SetNext( Window->EEDrawList );
        Window->EEDrawList = component;
        component->SetParent( Window );
    }

    return Failed;   /* Fin lecture 1 composant */
}
