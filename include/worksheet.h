/***************/
/* worksheet.h */
/***************/

// For page and paper size, values are in 1/1000 inch

#ifndef WORKSHEET_H_
#define WORKSHEET_H_

#include <colors.h>     // EDA_COLOR_T definition
#include <class_page_info.h>

// Forward declarations:
class EDA_DRAW_PANEL;
class EDA_RECT;
class TITLE_BLOCK;

/**
 * Function DrawPageLayout is a core function to draw the page layout with
 * the frame and the basic inscriptions.
 * @param aDC The device context.
 * @param aClipBox = the clipping rect, or NULL if no clipping.
 * @param aPageInfo for margins and page size (in mils).
 * @param aFullSheetName The sheetpath (full sheet name), for basic inscriptions.
 * @param aFileName The file name, for basic inscriptions.
 * @param aTitleBlock The sheet title block, for basic inscriptions.
 * @param aSheetCount The number of sheets (for basic inscriptions).
 * @param aSheetNumber The sheet number (for basic inscriptions).
 * @param aPenWidth the pen size The line width for drawing.
 * @param aScalar the scale factor to convert from mils to internal units.
 * @param aColor The color for drawing.
 * @param aAltColor The color for items which need to be "hightlighted".
 *
 * Parameters used in aPageInfo
 * - the size of the page layout.
 * - the LTmargin The left top margin of the page layout.
 * - the RBmargin The right bottom margin of the page layout.
 */
void DrawPageLayout( wxDC* aDC, EDA_RECT* aClipBox,
                     const PAGE_INFO& aPageInfo,
                     const wxString &aFullSheetName,
                     const wxString& aFileName,
                     TITLE_BLOCK& aTitleBlock,
                     int aSheetCount, int aSheetNumber,
                     int aPenWidth, double aScalar,
                     EDA_COLOR_T aColor, EDA_COLOR_T aAltColor );


#endif // WORKSHEET_H_
