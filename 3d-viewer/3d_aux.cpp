/////////////////////////////////////////////////////////////////////////////
// Name:        3d_aux.cpp
/////////////////////////////////////////////////////////////////////////////


#ifdef __GNUG__
#pragma implementation
#pragma interface
#endif

#include "fctsys.h"

#if !wxUSE_GLCANVAS
#error Please set wxUSE_GLCANVAS to 1 in setup.h.
#endif

#include "common.h"
#include "trigo.h"

#include "bitmaps.h"

#include "3d_viewer.h"
#include "3d_struct.h"
#include "trackball.h"

/**************************************************************************/
void Struct3D_Master::Set_Object_Coords(S3D_Vertex * coord, int nbcoord )
/**************************************************************************/
{
int ii;


	/* adjust object scale, rotation and offset position */
	for ( ii = 0; ii < nbcoord; ii++ )
	{
	coord[ii].x *= m_MatScale.x;
	coord[ii].y *= m_MatScale.y;
	coord[ii].z *= m_MatScale.z;
	/* adjust rotation */
	if ( m_MatRotation.x )
		RotatePoint(&coord[ii].y, &coord[ii].z,
			(int)(m_MatRotation.x * 10));
	if ( m_MatRotation.y )
		RotatePoint(&coord[ii].z, &coord[ii].x,
			(int)(m_MatRotation.y * 10));
	if ( m_MatRotation.z )
		RotatePoint(&coord[ii].x, &coord[ii].y,
			(int)(m_MatRotation.z * 10));
	/* adjust offset position (offset is given in UNIT 3D (0.1 inch) */
#define SCALE_3D_CONV (PCB_INTERNAL_UNIT/UNITS3D_TO_UNITSPCB)
	coord[ii].x += m_MatPosition.x * SCALE_3D_CONV;
	coord[ii].y += m_MatPosition.y * SCALE_3D_CONV;
	coord[ii].z += m_MatPosition.z * SCALE_3D_CONV;
	}
}

/************************************************************/
void Set_Object_Data(const S3D_Vertex * coord, int nbcoord )
/************************************************************/
{
int ii;
GLfloat ax,ay,az,bx,by,bz,nx,ny,nz,r;

    /* ignore faces with less than 3 points */
    if (nbcoord < 3) return;

    /* calculate normal direction */
    ax = coord[1].x - coord[0].x;
    ay = coord[1].y - coord[0].y;
    az = coord[1].z - coord[0].z;

    bx = coord[nbcoord-1].x - coord[0].x;
    by = coord[nbcoord-1].y - coord[0].y;
    bz = coord[nbcoord-1].z - coord[0].z;

    nx = ay * bz - az * by;
    ny = az * bx - ax * bz;
    nz = ax * by - ay * bx;

    r = sqrt(nx*nx + ny*ny + nz*nz);
    if (r >= 0.000001) /* avoid division by zero */
	{
		nx /= r; ny /= r; nz /= r;
		glNormal3f(nx,ny,nz);
	}

    /* glBegin/glEnd */
    switch (nbcoord)
	{
		case 3:
			glBegin(GL_TRIANGLES);
			break;

		case 4:
			glBegin(GL_QUADS);
			break;

		default:
			glBegin(GL_POLYGON);
			break;
    }

    /* draw polygon/triangle/quad */
    for (ii = 0; ii < nbcoord; ii++)
	{
		glVertex3f(coord[ii].x * DataScale3D,
					coord[ii].y * DataScale3D,
					coord[ii].z * DataScale3D);
	}

	glEnd();
}


/**********************************************/
GLuint Pcb3D_GLCanvas::DisplayCubeforTest(void)
/**********************************************/
{
GLuint gllist = glGenLists( 1 );

    glNewList( gllist, GL_COMPILE_AND_EXECUTE );
    /* draw six faces of a cube */
    glBegin(GL_QUADS);
    glNormal3f( 0.0F, 0.0F, 1.0F);
    glVertex3f( 0.5F, 0.5F, 0.5F); glVertex3f(-0.5F, 0.5F, 0.5F);
    glVertex3f(-0.5F,-0.5F, 0.5F); glVertex3f( 0.5F,-0.5F, 0.5F);

    glNormal3f( 0.0F, 0.0F,-1.0F);
    glVertex3f(-0.5F,-0.5F,-0.5F); glVertex3f(-0.5F, 0.5F,-0.5F);
    glVertex3f( 0.5F, 0.5F,-0.5F); glVertex3f( 0.5F,-0.5F,-0.5F);

    glNormal3f( 0.0F, 1.0F, 0.0F);
    glVertex3f( 0.5F, 0.5F, 0.5F); glVertex3f( 0.5F, 0.5F,-0.5F);
    glVertex3f(-0.5F, 0.5F,-0.5F); glVertex3f(-0.5F, 0.5F, 0.5F);

    glNormal3f( 0.0F,-1.0F, 0.0F);
    glVertex3f(-0.5F,-0.5F,-0.5F); glVertex3f( 0.5F,-0.5F,-0.5F);
    glVertex3f( 0.5F,-0.5F, 0.5F); glVertex3f(-0.5F,-0.5F, 0.5F);

    glNormal3f( 1.0F, 0.0F, 0.0F);
    glVertex3f( 0.5F, 0.5F, 0.5F); glVertex3f( 0.5F,-0.5F, 0.5F);
    glVertex3f( 0.5F,-0.5F,-0.5F); glVertex3f( 0.5F, 0.5F,-0.5F);

    glNormal3f(-1.0F, 0.0F, 0.0F);
    glVertex3f(-0.5F,-0.5F,-0.5F); glVertex3f(-0.5F,-0.5F, 0.5F);
    glVertex3f(-0.5F, 0.5F, 0.5F); glVertex3f(-0.5F, 0.5F,-0.5F);
    glEnd();

    glEndList();

	return gllist;
}



/**********************/
/* class Info_3D_Visu */
/**********************/

/* Constructor */
Info_3D_Visu::Info_3D_Visu(void)
{
int ii;
    m_Beginx = m_Beginy = 0.0;	/* position of mouse */
    m_Zoom = 1.0;				/* field of view in degrees */
	trackball( m_Quat, 0.0, 0.0, 0.0, 0.0 );
	for ( ii = 0; ii < 4; ii++ ) m_Rot[ii] = 0.0;
	m_Layers = 1;
	m_BoardSettings = NULL;
}


Info_3D_Visu::~Info_3D_Visu(void)
{
}


/*****************************************************************/
/* Classe pour afficher et editer un Vertex (triplet de valeurs),*/
/* en INCHES ou MM ou sans unites								 */
/*****************************************************************/

WinEDA_VertexCtrl::WinEDA_VertexCtrl(wxWindow *parent, const wxString & title,
						wxBoxSizer * BoxSizer,
						int units, int internal_unit)
{
wxString text;
wxStaticText * msgtitle;
	
	m_Units = units;
	m_Internal_Unit = internal_unit;

	if ( title.IsEmpty()  )	text = _("Vertex ");
	else text = title;
	text += ReturnUnitSymbol(units);

	msgtitle = new wxStaticText(parent, -1, text, wxDefaultPosition, wxSize(-1,-1), 0 );
	BoxSizer->Add(msgtitle, wxGROW|wxLEFT|wxRIGHT|wxTOP|wxBOTTOM|wxADJUST_MINSIZE);

    wxFlexGridSizer * GridSizer = new wxFlexGridSizer(3, 2, 0, 0);
    BoxSizer->Add(GridSizer, 0, wxGROW|wxALL, 5);

	msgtitle = new wxStaticText(parent, -1, wxT("X:"));
    GridSizer->Add(msgtitle, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxADJUST_MINSIZE, 5);
	m_XValueCtrl = new wxTextCtrl(parent, -1, wxEmptyString, wxDefaultPosition, wxSize(-1,-1), 0 );
	GridSizer->Add(m_XValueCtrl, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5);

	msgtitle = new wxStaticText(parent, -1, wxT("Y:"), wxDefaultPosition, wxSize(-1,-1), 0 );
    GridSizer->Add(msgtitle, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxADJUST_MINSIZE, 5);
	m_YValueCtrl = new wxTextCtrl(parent, -1, wxEmptyString, wxDefaultPosition, wxSize(-1,-1), 0 );
	GridSizer->Add(m_YValueCtrl, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5);

	msgtitle = new wxStaticText(parent, -1, wxT("Z:"), wxDefaultPosition, wxSize(-1,-1), 0 );
	GridSizer->Add(msgtitle, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxADJUST_MINSIZE, 5);
	m_ZValueCtrl = new wxTextCtrl(parent, -1, wxEmptyString, wxDefaultPosition, wxSize(-1,-1), 0 );
	GridSizer->Add(m_ZValueCtrl, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5);
}

WinEDA_VertexCtrl::~WinEDA_VertexCtrl(void)
{
}


/*******************************************/
S3D_Vertex WinEDA_VertexCtrl::GetValue(void)
/*******************************************/
/* Retourne (en unites internes) les coordonnes entrees (en unites utilisateur)
*/
{
S3D_Vertex value;
double dtmp;

	m_XValueCtrl->GetValue().ToDouble(&dtmp);
	value.x = dtmp;
	m_YValueCtrl->GetValue().ToDouble(&dtmp);
	value.y = dtmp;
	m_ZValueCtrl->GetValue().ToDouble(&dtmp);
	value.z = dtmp;
	return value;
}

/**************************************************/
void WinEDA_VertexCtrl::SetValue(S3D_Vertex vertex)
/**************************************************/
{
wxString text;

	text.Printf( wxT("%f"), vertex.x);
	m_XValueCtrl->Clear();
	m_XValueCtrl->AppendText(text);

	text.Printf(wxT("%f"), vertex.y);
	m_YValueCtrl->Clear();
	m_YValueCtrl->AppendText(text);

	text.Printf(wxT("%f"), vertex.z);
	m_ZValueCtrl->Clear();
	m_ZValueCtrl->AppendText(text);
}

/*****************************************/
void WinEDA_VertexCtrl::Enable(bool onoff)
/*****************************************/
{
	m_XValueCtrl->Enable(onoff);
	m_YValueCtrl->Enable(onoff);
	m_ZValueCtrl->Enable(onoff);
}

