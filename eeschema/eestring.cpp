	/**************************************/
	/*	Module to handle screen printing. */
	/**************************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"
#include "plot_common.h"

#include "protos.h"

extern void Move_Plume( wxPoint pos, int plume ); // see plot.cpp


/*****************************************************************************/
void PutTextInfo(WinEDA_DrawPanel * panel, wxDC * DC,
					int Orient, const wxPoint& Pos, const wxSize& Size,
					const wxString & Str, int DrawMode, int color)
/*****************************************************************************/
/* Put out a string, always centered to the given position, with given
	orientation, taking into account current zoom factor.
*/
{

	GRSetDrawMode(DC, DrawMode);
	DrawGraphicText(panel, DC, Pos, color, Str, Orient, Size,
						GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER);

}

/*****************************************************************************
Put out pin number and pin text info, given the pin line coordinates.
The line must be vertical or horizontal.
If PinText == NULL nothing is printed. If PinNum = 0 no number is printed.
Current Zoom factor is taken into account.
If TextInside then the text is been put inside,otherwise all is drawn outside.
Pin Name:	substring beteween '~' is negated
*****************************************************************************/
void LibDrawPin::DrawPinTexts(WinEDA_DrawPanel * panel, wxDC * DC,
				wxPoint & pin_pos, int orient,
				int TextInside, bool DrawPinNum, bool DrawPinName,
				int Color, int DrawMode)
/* DrawMode = GR_OR, XOR ... */
{
int ii, x, y, x1, y1, dx, dy, len;
wxString StringPinNum;
wxString PinText;
int PinTextBarPos[256];
int PinTextBarCount;
int NameColor, NumColor;
int PinTxtLen;
wxSize PinNameSize(m_SizeName,m_SizeName);
wxSize PinNumSize(m_SizeNum,m_SizeNum);

	GRSetDrawMode(DC, DrawMode);

	/* Get the num and name colors */
	NameColor =  Color == - 1 ? ReturnLayerColor(LAYER_PINNAM) : Color;
	NumColor = Color == - 1 ? ReturnLayerColor(LAYER_PINNUM) : Color;

	/* Create the pin num string */
	ReturnPinStringNum(StringPinNum);

	x1 = pin_pos.x; y1 = pin_pos.y;
	switch(orient)
	{
		case PIN_UP: y1 -= m_PinLen; break;
		case PIN_DOWN: y1 += m_PinLen; break;
		case PIN_LEFT: x1 -= m_PinLen; break;
		case PIN_RIGHT: x1 += m_PinLen; break;
	}

	const wxChar * textsrc = m_PinName.GetData();
	float fPinTextPitch = PinNameSize.x * 1.1;
	/* Do we need to invert the string? Is this string has only "~"? */
	PinTextBarCount = 0; PinTxtLen = 0;
	ii = 0;
	while ( * textsrc )
	{
		if ( * textsrc == '~' )
		{
			PinTextBarPos[PinTextBarCount++] = (int)(PinTxtLen * fPinTextPitch);
		}
		else
		{
			PinText.Append(* textsrc);
			PinTxtLen ++;
		}
		
		textsrc++;
	}
	
	PinTxtLen = (int) (fPinTextPitch * PinTxtLen);
	PinTextBarPos[PinTextBarCount] = PinTxtLen;	// Needed if no end '~'
	
	if (PinText[0] == 0) DrawPinName = FALSE;

	if( TextInside )  /* Draw the text inside, but the pin numbers outside. */
	{
		if( (orient == PIN_LEFT) || (orient == PIN_RIGHT) )
		// It is an horizontal line
		{
			if (PinText && DrawPinName)
			{
				if( orient == PIN_RIGHT)
				{
					x = x1 + TextInside;
					DrawGraphicText(panel, DC, wxPoint(x, y1), NameColor, PinText,
							TEXT_ORIENT_HORIZ,
							PinNameSize,
							GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER);

					for ( ii = 0; ii < PinTextBarCount; )
					{
						GRMoveTo(x, y1 - TXTMARGE);
						dy = -PinNameSize.y / 2;
						GRMoveRel(0, dy);
						dx = PinTextBarPos[ii++]; // Get the line pos
						GRMoveRel(dx, 0);
						len = PinTextBarPos[ii++] - dx; // Get the line length
						GRLineRel(&panel->m_ClipBox, DC, len, 0, NameColor);
					}
				}
				else	// Orient == PIN_LEFT
				{
					x = x1 - TextInside;
					DrawGraphicText(panel, DC, wxPoint(x, y1) , NameColor, PinText,
							TEXT_ORIENT_HORIZ,
							PinNameSize,
							GR_TEXT_HJUSTIFY_RIGHT, GR_TEXT_VJUSTIFY_CENTER);

					for ( ii = 0; ii < PinTextBarCount; )
					{
						GRMoveTo(x, y1 - TXTMARGE);
						dy = -PinNameSize.y / 2;
						GRMoveRel(0, dy);
						dx = PinTextBarPos[ii++]; // Get the line pos
						GRMoveRel(dx - PinTxtLen, 0);
						len = PinTextBarPos[ii++] - dx; // Get the line length
						GRLineRel(&panel->m_ClipBox, DC, len, 0, NameColor);
					}
				}
			}

			if ( DrawPinNum)
			{
				DrawGraphicText(panel, DC,
						wxPoint((x1 + pin_pos.x) / 2, y1 - TXTMARGE), NumColor, StringPinNum,
						TEXT_ORIENT_HORIZ, PinNumSize,
						GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_BOTTOM);
			}
		}

		else		 /* Its a vertical line. */
		{	// Text is drawn from bottom to top (i.e. to negative value for Y axis)
			if (PinText && DrawPinName)
			{
				if ( orient == PIN_DOWN )
				{
					y = y1 + TextInside;

					DrawGraphicText(panel, DC, wxPoint(x1, y), NameColor, PinText,
							TEXT_ORIENT_VERT, PinNameSize,
							GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_TOP);

					for ( ii = 0; ii < PinTextBarCount; )
					{
						GRMoveTo(x1 - TXTMARGE, y);
						dy = -PinNameSize.y / 2;
						GRMoveRel(dy , 0);
						dx = PinTextBarPos[ii++]; // Get the line pos
						GRMoveRel(0, PinTxtLen - dx);
						len = PinTextBarPos[ii++] - dx; // Get the line length
						GRLineRel(&panel->m_ClipBox, DC, 0, -len, NameColor);
					}
				}

				else	/* PIN_UP */
				{
					y = y1 - TextInside;

					DrawGraphicText(panel, DC, wxPoint(x1, y), NameColor, PinText,
							TEXT_ORIENT_VERT, PinNameSize,
							GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_BOTTOM);

					for ( ii = 0; ii < PinTextBarCount; )
					{
						GRMoveTo(x1 - TXTMARGE, y);
						dy = -PinNameSize.y / 2;
						GRMoveRel(dy, 0);
						dx = PinTextBarPos[ii++]; // Get the line pos
						GRMoveRel(0, - dx);
						len = PinTextBarPos[ii++] - dx; // Get the line length
						GRLineRel(&panel->m_ClipBox, DC, 0, -len, NameColor);
					}
				}
			}

			if(DrawPinNum)
			{
				DrawGraphicText(panel, DC,
						wxPoint(x1  - TXTMARGE, (y1 + pin_pos.y) / 2) , NumColor, StringPinNum,
						TEXT_ORIENT_VERT, PinNumSize,
						GR_TEXT_HJUSTIFY_RIGHT, GR_TEXT_VJUSTIFY_CENTER);
			}
		}
	}
	
	else	 /**** Draw num & text pin outside  ****/
	{
		if( (orient == PIN_LEFT) || (orient == PIN_RIGHT) )
		/* Its an horizontal line. */
		{
			if (PinText && DrawPinName)
			{
               x = (x1 + pin_pos.x) / 2;
				DrawGraphicText(panel, DC, wxPoint(x , y1 - TXTMARGE),
							NameColor, PinText,
							TEXT_ORIENT_HORIZ, PinNameSize,
							GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_BOTTOM);

				for ( ii = 0; ii < PinTextBarCount; )
				{
					GRMoveTo(x, y1 - TXTMARGE*2);
					GRMoveRel(-PinTxtLen / 2, -PinNameSize.y );
					dx = PinTextBarPos[ii++]; // Get the line pos
					GRMoveRel(dx, 0);
					len = PinTextBarPos[ii++] - dx; // Get the line length
					GRLineRel(&panel->m_ClipBox, DC, len, 0, NameColor);
				}
			}
			if(DrawPinNum)
			{
				x = (x1 + pin_pos.x) / 2;
				DrawGraphicText(panel, DC, wxPoint(x, y1 + TXTMARGE),
							NumColor, StringPinNum,
							TEXT_ORIENT_HORIZ, PinNumSize,
							GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_TOP);
			}
		}
		else	 /* Its a vertical line. */
		{
			if (PinText && DrawPinName)
			{
               y = (y1 + pin_pos.y) / 2;
				DrawGraphicText(panel, DC, wxPoint(x1 - TXTMARGE , y ),
							NameColor, PinText,
							TEXT_ORIENT_VERT, PinNameSize,
							GR_TEXT_HJUSTIFY_RIGHT, GR_TEXT_VJUSTIFY_CENTER);

				for ( ii = 0; ii < PinTextBarCount; )
				{
					GRMoveTo(x1 - (TXTMARGE * 2), y);
					GRMoveRel(-PinNameSize.y, -PinTxtLen / 2);
					dx = PinTextBarPos[ii++]; // Get the line pos
					GRMoveRel(0, PinTxtLen - dx);
					len = PinTextBarPos[ii++] - dx; // Get the line length
					GRLineRel(&panel->m_ClipBox, DC, 0, - len, NameColor);
				}
			}

			if(DrawPinNum)
			{
				DrawGraphicText(panel, DC, wxPoint(x1 + TXTMARGE , (y1 + pin_pos.y) / 2),
							NumColor, StringPinNum,
							TEXT_ORIENT_VERT, PinNumSize,
							GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER);
			}
		}
	}
}

/*****************************************************************************
* Plot pin number and pin text info, given the pin line coordinates.	  *
* Same as DrawPinTexts((), but output is the plotter
* The line must be vertical or horizontal.						  *
* If PinNext == NULL nothing is printed.									*
* Current Zoom factor is taken into account.					 *
* If TextInside then the text is been put inside (moving from x1, y1 in		 *
* the opposite direction to x2,y2), otherwise all is drawn outside.		 *
*****************************************************************************/
void LibDrawPin::PlotPinTexts(wxPoint & pin_pos, int orient,
				int TextInside, bool DrawPinNum, bool DrawPinName)
{
int dx, len, start;
int ii , x, y, x1, y1, cte;
wxString StringPinNum;
wxString PinText;
int PinTextBarPos[256];
int PinTextBarCount;
int NameColor, NumColor;
int PinTxtLen = 0;
wxSize PinNameSize = wxSize(m_SizeName,m_SizeName);
wxSize PinNumSize = wxSize(m_SizeNum,m_SizeNum);
bool plot_color = (g_PlotFormat == PLOT_FORMAT_POST) && g_PlotPSColorOpt;
	
	/* Get the num and name colors */
	NameColor = plot_color ? ReturnLayerColor(LAYER_PINNAM) : -1;
	NumColor = plot_color ? ReturnLayerColor(LAYER_PINNUM) : -1;

	/* Create the pin num string */
	ReturnPinStringNum(StringPinNum);
	x1 = pin_pos.x; y1 = pin_pos.y;
	switch(orient)
	{
		case PIN_UP: y1 -= m_PinLen; break;
		case PIN_DOWN: y1 += m_PinLen; break;
		case PIN_LEFT: x1 -= m_PinLen; break;
		case PIN_RIGHT: x1 += m_PinLen; break;
	}

	const wxChar * textsrc = m_PinName.GetData();
	float fPinTextPitch = PinNameSize.x * 1.1;
	/* Do we need to invert the string? Is this string has only "~"? */
	PinTextBarCount = 0; PinTxtLen = 0;
	ii = 0;
	while ( * textsrc )
	{
		if ( * textsrc == '~' )
		{
			PinTextBarPos[PinTextBarCount++] = (int) (fPinTextPitch * PinTxtLen);
		}
		else
		{
			PinText.Append(* textsrc);
			PinTxtLen ++;
		}
		
		textsrc++;
	}
	
	PinTxtLen = (int) (fPinTextPitch * PinTxtLen);
	PinTextBarPos[PinTextBarCount] = PinTxtLen;	// Needed if no end '~'
	
	if (PinText[0] == 0) DrawPinName = FALSE;

	if (TextInside)  /* Draw the text inside, but the pin numbers outside. */
	{
		if( (orient == PIN_LEFT) || (orient == PIN_RIGHT) )
		{	  /* Its an horizontal line. */
			if (PinText && DrawPinName)
			{
				if( orient == PIN_RIGHT)
				{
					x = x1 + TextInside;
					PlotGraphicText(g_PlotFormat, wxPoint(x, y1), NameColor, PinText,
							TEXT_ORIENT_HORIZ,
							PinNameSize,
							GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER);

					for ( ii = 0; ii < PinTextBarCount; )
					{
						cte = y1 - PinNameSize.y/2 - TXTMARGE;
						dx = PinTextBarPos[ii++]; // Get the line pos
						Move_Plume( wxPoint(x+dx, cte), 'U');
						len = PinTextBarPos[ii++]; // Get the line end
						Move_Plume( wxPoint(x + len, cte), 'D');
					}
				}
				else	// orient == PIN_LEFT
				{
					x = x1 - TextInside;
					PlotGraphicText(g_PlotFormat, wxPoint(x, y1),
							NameColor, PinText, TEXT_ORIENT_HORIZ,
							PinNameSize,
							GR_TEXT_HJUSTIFY_RIGHT, GR_TEXT_VJUSTIFY_CENTER);

					for ( ii = 0; ii < PinTextBarCount; )
					{
						cte = y1 - PinNameSize.y/2 - TXTMARGE;
						dx = PinTextBarPos[ii++]; // Get the line pos
						Move_Plume( wxPoint(x+dx - PinTxtLen, cte), 'U');
						len = PinTextBarPos[ii++]; // Get the line end
						Move_Plume(wxPoint(x + len - PinTxtLen, cte), 'D');
					}
				}
			}

			if (DrawPinNum)
			{
				PlotGraphicText( g_PlotFormat, wxPoint((x1 + pin_pos.x) / 2, y1 - TXTMARGE),
						NumColor, StringPinNum,
						TEXT_ORIENT_HORIZ, PinNumSize,
						GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_BOTTOM);
			}
		}

		else		 /* Its a vertical line. */
		{
			if (PinText && DrawPinName)
			{
				if ( orient == PIN_DOWN )
				{
					y = y1 + TextInside;

					PlotGraphicText( g_PlotFormat, wxPoint(x1, y), NameColor, PinText,
							TEXT_ORIENT_VERT, PinNameSize,
							GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_TOP);

					for ( ii = 0; ii < PinTextBarCount; )
					{
						cte = x1 - PinNameSize.y/2 - TXTMARGE;
						dx = PinTextBarPos[ii++]; // Get the line pos
						Move_Plume( wxPoint(cte, y + PinTxtLen - dx), 'U');
						len = PinTextBarPos[ii++]; // Get the line end
						Move_Plume( wxPoint(cte , y + PinTxtLen - len), 'D');
					}
				}

				else	/* PIN_UP */
				{
					y = y1 - TextInside;

					PlotGraphicText( g_PlotFormat, wxPoint(x1, y), NameColor, PinText,
							TEXT_ORIENT_VERT, PinNameSize,
							GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_BOTTOM);

					for ( ii = 0; ii < PinTextBarCount; )
					{
						cte = x1 - PinNameSize.y/2 - TXTMARGE;
						dx = PinTextBarPos[ii++]; // Get the line pos
						Move_Plume( wxPoint(cte, y -dx), 'U');
						len = PinTextBarPos[ii++]; // Get the line end
						Move_Plume(wxPoint(cte, y - len), 'D');
					}
				}
			}

			if (DrawPinNum)
			{
				PlotGraphicText( g_PlotFormat, wxPoint(x1 - TXTMARGE, (y1 + pin_pos.y) / 2),
							NumColor, StringPinNum,
							TEXT_ORIENT_VERT, PinNumSize,
							GR_TEXT_HJUSTIFY_RIGHT, GR_TEXT_VJUSTIFY_CENTER);
			}
		}
	}
	
	else	 /* Draw num & text pin outside */
	{
		if( (orient == PIN_LEFT) || (orient == PIN_RIGHT) )
		/* Its an horizontal line. */
		{
			if (PinText && DrawPinName)
			{
				x = (x1 + pin_pos.x) / 2;
				PlotGraphicText( g_PlotFormat, wxPoint(x, y1 - TXTMARGE),
							NameColor, PinText,
							TEXT_ORIENT_HORIZ, PinNameSize,
							GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_BOTTOM);

				for ( ii = 0; ii < PinTextBarCount; )
				{
					cte = y1 - PinNameSize.y - TXTMARGE*2;
					start = x - (PinTxtLen / 2);
					dx = PinTextBarPos[ii++]; // Get the line pos
					Move_Plume( wxPoint(start + dx, cte), 'U');
					len = PinTextBarPos[ii++]; // Get the line end
					Move_Plume(wxPoint(start + len, cte), 'D');
				}
			}
			if (DrawPinNum)
			{
				x = (x1 + pin_pos.x) / 2;
				PlotGraphicText(g_PlotFormat, wxPoint(x, y1 + TXTMARGE),
							NumColor, StringPinNum,
							TEXT_ORIENT_HORIZ, PinNumSize,
							GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_TOP);
			}
		}
		else	 /* Its a vertical line. */
		{
			if (PinText && DrawPinName)
			{
				y = (y1 + pin_pos.y)  / 2;
				PlotGraphicText( g_PlotFormat, wxPoint(x1 - TXTMARGE,y),
							NameColor, PinText,
							TEXT_ORIENT_VERT, PinNameSize,
							GR_TEXT_HJUSTIFY_RIGHT, GR_TEXT_VJUSTIFY_CENTER);

				for ( ii = 0; ii < PinTextBarCount; )
				{
					cte = x1 - PinNameSize.y - TXTMARGE*2;
					start = y + (PinTxtLen / 2);
					dx = PinTextBarPos[ii++]; // Get the line pos
					Move_Plume( wxPoint(cte, start - dx), 'U');
					len = PinTextBarPos[ii++]; // Get the line end
					Move_Plume( wxPoint(cte, start - len), 'D');
				}
			}

			 if (DrawPinNum)
			{
				PlotGraphicText( g_PlotFormat, wxPoint(x1 + TXTMARGE, (y1 + pin_pos.y) / 2),
							NumColor, StringPinNum,
							TEXT_ORIENT_VERT, PinNumSize,
							GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER);
			}
		}
	}
}

