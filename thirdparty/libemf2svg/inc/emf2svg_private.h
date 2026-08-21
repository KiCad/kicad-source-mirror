#ifdef __cplusplus
extern "C" {
#endif

#include "uemf.h"
#include <stddef.h> /* for offsetof() macro */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define KNRM "\x1B[0m"
#define KRED "\x1B[31m"
#define KGRN "\x1B[32m"
#define KYEL "\x1B[33m"
#define KBLU "\x1B[34m"
#define KMAG "\x1B[35m"
#define KCYN "\x1B[36m"
#define KWHT "\x1B[37m"

#define verbose_printf(...)                                                    \
    if (states->verbose)                                                       \
        printf(__VA_ARGS__);
#define FLAG_SUPPORTED                                                         \
    verbose_printf("   Status:         %sSUPPORTED%s\n", KGRN, KNRM);
#define FLAG_IGNORED                                                           \
    verbose_printf("   Status:         %sIGNORED%s\n", KRED, KNRM);
#define FLAG_PARTIAL                                                           \
    verbose_printf("   Status:         %sPARTIAL SUPPORT%s\n", KYEL, KNRM);
#define FLAG_UNUSED                                                            \
    verbose_printf("   Status:         %sUNUSED%s\n", KMAG, KNRM);
#define FLAG_RESET verbose_printf("%s", KNRM);

#define returnOutOfEmf(a)                                                      \
    if (checkOutOfEMF(states, (uintptr_t)(a))) {                                \
        return;                                                                \
    }
#define returnOutOfOTIndex(a)                                                  \
    if (checkOutOfOTIndex(states, (int64_t)(a))) {                             \
        return;                                                                \
    }

#define UNUSED(x) (void)(x)

#define FONTINDEX 2
#define UTF_16 1
#define ASCII 0

// apparently 0x04 is text-align middle to
#define U_TA_CENTER2 0x04

#define mmPerInch 25.4

#define ARC_SIMPLE 0
#define ARC_PIE 1
#define ARC_CHORD 2

#define SEG_END 0
#define SEG_MOVE 1
#define SEG_LINE 2
#define SEG_ARC 3
#define SEG_BEZIER 4

typedef struct {
    double x;
    double y;
} POINT_D;

typedef struct _PATH_SECTION {
    uint8_t type;
    POINT_D *points;
} PATH_SECTION;

typedef struct _PATH {
    PATH_SECTION section;
    struct _PATH *last;
    struct _PATH *next;
} PATH;

typedef struct emf_graph_object {
    bool font_set;
    char *font_name;
    char *font_family;
    uint32_t font_height;
    uint32_t font_width;
    bool font_italic;
    bool font_underline;
    bool font_strikeout;
    uint32_t font_weight;
    uint32_t font_escapement;
    uint32_t font_orientation;
    uint8_t font_charset;

    bool stroke_set;
    int stroke_mode;   // enumeration from drawmode, not used if fill_set is not
                       // True
    int stroke_idx;    // used with DRAW_PATTERN and DRAW_IMAGE to return the
                       // appropriate fill
    int stroke_recidx; // record used to regenerate hatch when it needs to be
                       // redone due to bkmode, textmode, etc. change
    // stroke color
    uint8_t stroke_red;
    uint8_t stroke_blue;
    uint8_t stroke_green;
    double stroke_width;

    bool fill_set;
    int fill_mode;   // enumeration from drawmode, not used if fill_set is not
                     // True
    int fill_idx;    // used with DRAW_PATTERN and DRAW_IMAGE to return the
                     // appropriate fill
    int fill_recidx; // record used to regenerate hatch when it needs to be
                     // redone due to bkmode, textmode, etc. change
    // filling colors
    uint8_t fill_red;
    uint8_t fill_blue;
    uint8_t fill_green;
    uint16_t fill_hatch_style;

    // text properties
    uint8_t text_red;
    uint8_t text_blue;
    uint8_t text_green;
    uint16_t text_align;

    // bk propertier
    uint8_t bk_red;
    uint8_t bk_blue;
    uint8_t bk_green;
    uint16_t bk_mode;

} emfGraphObject;

// EMF Device Context structure
typedef struct emf_device_context {
    bool font_set;
    char *font_name;
    char *font_family;
    uint32_t font_height;
    uint32_t font_width;
    bool font_italic;
    bool font_underline;
    bool font_strikeout;
    uint32_t font_weight;
    uint32_t font_escapement;
    uint32_t font_orientation;
    uint8_t font_charset;

    uint32_t stretchMode;
    uint32_t miterLimit;

    int16_t arcdir;

    bool stroke_set;
    int stroke_mode;   // enumeration from drawmode, not used if fill_set is not
                       // True
    int stroke_idx;    // used with DRAW_PATTERN and DRAW_IMAGE to return the
                       // appropriate fill
    int stroke_recidx; // record used to regenerate hatch when it needs to be
                       // redone due to bkmode, textmode, etc. change
    // stroke color
    uint8_t stroke_red;
    uint8_t stroke_blue;
    uint8_t stroke_green;
    double stroke_width;

    bool fill_set;
    int fill_mode;   // enumeration from drawmode, not used if fill_set is not
                     // True
    int fill_idx;    // used with DRAW_PATTERN and DRAW_IMAGE to return the
                     // appropriate fill
    int fill_recidx; // record used to regenerate hatch when it needs to be
                     // redone due to bkmode, textmode, etc. change
    int fill_polymode;
    // filling colors
    uint8_t fill_red;
    uint8_t fill_blue;
    uint8_t fill_green;
    uint16_t fill_hatch_style;
    int dirty; // holds the dirty bits for text, stroke, fill

    // text properties
    uint8_t text_red;
    uint8_t text_blue;
    uint8_t text_green;
    uint16_t text_align;

    // bk propertier
    uint8_t bk_red;
    uint8_t bk_blue;
    uint8_t bk_green;
    uint16_t bk_mode;

    U_XFORM worldTransform;

    // clipping structures
    PATH *clipRGN;
    // if set to zero, clip is not set
    int clipID;
} EMF_DEVICE_CONTEXT, *PEMF_DEVICE_CONTEXT;

// Stack of EMF Device Contexts
typedef struct dc_stack {
    EMF_DEVICE_CONTEXT DeviceContext;
    struct dc_stack *previous;
} EMF_DEVICE_CONTEXT_STACK;

typedef struct {
    uint32_t fillOffset;
    uint32_t strokeFillOffset;
    uint32_t strokeOffset;
    uint32_t flattenOffset;
    uint32_t widdenOffset;
    uint32_t clipOffset;
    uint32_t abortOffset;
    bool wtBeforeSet;
    bool wtAfterSet;
    uint32_t wtBeforeiMode;
    uint32_t wtAfteriMode;
    U_XFORM wtBeforexForm;
    U_XFORM wtAfterxForm;
} pathStruct;

typedef struct pathstack {
    pathStruct pathStruct;
    struct pathstack *next;
} pathStack;

typedef struct {
    pathStack *pathStack;
    struct pathstack *pathStackLast;
} emfStruct;

// Image library for images used as fill patterns
typedef struct imageLibrary {
    int id;
    PU_BITMAPINFOHEADER content;
    struct imageLibrary *next;
} emfImageLibrary;

// structure recording drawing states
typedef struct {
    // unique ID (simple increment)
    int uniqId;
    // SVG namespace (the '<something>' before each fields)
    char *nameSpace;
    // Same as previously, but with ':'
    char *nameSpaceString;
    // Verbose mode, output fields and fields values if True
    bool verbose;
    // Handle emf+ records or not
    bool emfplus;
    // draw svg document delimiter or not
    bool svgDelimiter;
    // error flag
    bool Error;
    // end address of the emf content
    // used for checks against overflow
    // 64 bits to prevent pointer pointer calculation overflow
    uint64_t endAddress;
    struct _FcConfig *fontConfig;
    /* The current EMF Device Context
     * (Device Context == pen, brush, palette... see [MS-EMF].pdf) */
    EMF_DEVICE_CONTEXT currentDeviceContext;
    /* Device Contexts can be saved (EMR_SAVEDC),
     * Previous Device Contexts can be restored (EMR_RESTOREDC)
     * So we store then in this stack
     */
    EMF_DEVICE_CONTEXT_STACK *DeviceContextStack;
    // flag to know if we are in an SVG path or not
    bool inPath;
    // flag to know if we have start to draw the path
    bool pathDrawn;
    // object table
    emfGraphObject *objectTable;
    // size of the object table (warning, could be negative)
    // initialized to -1
    // the "real" object table is objectTableSize + 1
    // reason: (indexes in emf files start a 1 and not 0)
    int64_t objectTableSize;
    // scaling ratio
    double scaling;
    double RefX;
    double RefY;
    double viewPortOrgX;
    double viewPortOrgY;
    double viewPortExX;
    double viewPortExY;
    // true if viewport extent has been set, false otherwise
    bool viewPortExSet;
    double windowOrgX;
    double windowOrgY;
    double windowExX;
    double windowExY;
    // true if windows extent has been set, false otherwise
    bool windowExSet;
    // true if we are fixing layout problems from Wine-generated EMF
    bool fixBrokenYTransform;
    double pxPerMm;
    uint16_t MapMode;
    // Text orientation
    uint32_t text_layout;
    // Image dimensions
    double imgHeight;
    double imgWidth;
    // flag if there is an opened transformation
    bool transform_open;
    // current cursor position
    double cur_x;
    double cur_y;
    // general emf structure
    // used to associate records together
    // for example, associate path and pathfill/pathstroke/clipping
    emfStruct emfStructure;
    PATH *currentPath;
    // image library for pattern support
    int count_images;
    emfImageLibrary *library;
} drawingStates;

typedef struct cmap_collection {
    size_t size;
    uint32_t *uni;
} cmap_collection;

#define U_MWT_SET 4 //!< Transform is basic SET

#define BUFFERSIZE 1024
//! \cond

/* manipulate device context */

// add a device context on the stack included in states
void saveDeviceContext(drawingStates *states);
// copy device context from src in dest
void copyDeviceContext(EMF_DEVICE_CONTEXT *dest, EMF_DEVICE_CONTEXT *src);
// restore device context at <index> in the stack as current device context
void restoreDeviceContext(drawingStates *states, int32_t index);
// free the device context stack
void freeDeviceContextStack(drawingStates *states);
// stroke shape
void stroke_draw(drawingStates *states, FILE *out, bool *filled, bool *stroked);
void point16_draw(drawingStates *states, U_POINT16 pt, FILE *out);
void point_draw(drawingStates *states, U_POINT pt, FILE *out);
void freePathStack(pathStack *stack);
// checks if address is outside the memory containing the emf file
bool checkOutOfEMF(drawingStates *states, uintptr_t address);
// checks if index is greater than the object table size
bool checkOutOfOTIndex(drawingStates *states, int64_t index);
void fill_draw(drawingStates *states, FILE *out, bool *filled, bool *stroked);
double scaleY(drawingStates *states, double y);
double scaleX(drawingStates *states, double x);

/* prototypes for objects used in EMR records */
void hexbytes_draw(drawingStates *states, uint8_t *buf, unsigned int num);
void colorref_draw(drawingStates *states, U_COLORREF color);
void rgbquad_draw(drawingStates *states, U_RGBQUAD color);
void rectl_draw(drawingStates *states, FILE *out, U_RECTL rect);
void sizel_draw(drawingStates *states, U_SIZEL sz);
void pointl_draw(drawingStates *states, U_POINTL pt);
void lcs_gamma_draw(drawingStates *states, U_LCS_GAMMA lg);
void lcs_gammargb_draw(drawingStates *states, U_LCS_GAMMARGB lgr);
void trivertex_draw(drawingStates *states, U_TRIVERTEX tv);
void gradient3_draw(drawingStates *states, U_GRADIENT3 g3);
void gradient4_draw(drawingStates *states, U_GRADIENT4 g4);
void logbrush_draw(drawingStates *states, U_LOGBRUSH lb);
void xform_draw(drawingStates *states, U_XFORM xform);
void ciexyz_draw(drawingStates *states, U_CIEXYZ ciexyz);
void ciexyztriple_draw(drawingStates *states, U_CIEXYZTRIPLE cie3);
void logcolorspacea_draw(drawingStates *states, U_LOGCOLORSPACEA lcsa);
void logcolorspacew_draw(drawingStates *states, U_LOGCOLORSPACEW lcsa);
void panose_draw(drawingStates *states, U_PANOSE panose);
void logfont_draw(drawingStates *states, U_LOGFONT lf);
void logfont_panose_draw(drawingStates *states, U_LOGFONT_PANOSE lfp);
int bitmapinfoheader_draw(drawingStates *states, const char *Bmih);
void bitmapinfo_draw(drawingStates *states, const char *Bmi);
void blend_draw(drawingStates *states, U_BLEND blend);
void extlogpen_draw(drawingStates *states, const PU_EXTLOGPEN elp);
void logpen_draw(drawingStates *states, U_LOGPEN lp);
void logpltntry_draw(drawingStates *states, U_LOGPLTNTRY lpny);
void logpalette_draw(drawingStates *states, const PU_LOGPALETTE lp);
void rgndataheader_draw(drawingStates *states, U_RGNDATAHEADER rdh);
void rgndata_draw(drawingStates *states, const PU_RGNDATA rd);
void coloradjustment_draw(drawingStates *states, U_COLORADJUSTMENT ca);
void pixelformatdescriptor_draw(drawingStates *states,
                                U_PIXELFORMATDESCRIPTOR pfd);
void emrtext_draw(drawingStates *states, const char *emt, const char *record,
                  int type);
void arc_circle_draw(const char *contents, FILE *out, drawingStates *states);
void addFormToStack(drawingStates *states);
bool transform_set(drawingStates *states, U_XFORM xform, uint32_t iMode);
void transform_draw(drawingStates *states, FILE *out);
void arc_draw(const char *contents, FILE *out, drawingStates *states, int type);
void newPathStruct(drawingStates *states);
void setTransformIdentity(drawingStates *states);
void freeObjectTable(drawingStates *states);
void freePathStack(pathStack *stack);
void freeDeviceContext(EMF_DEVICE_CONTEXT *dc);
POINT_D point_cal(drawingStates *states, double x, double y);
void text_draw(const char *contents, FILE *out, drawingStates *states,
               uint8_t type);
void lineto_draw(const char *name, const char *field1, const char *field2,
                 const char *contents, FILE *out, drawingStates *states);
void cubic_bezier_draw(const char *name, const char *contents, FILE *out,
                       drawingStates *states, int startingPoint);
void startPathDraw(drawingStates *states, FILE *out);
void point_draw_d(drawingStates *states, POINT_D pt, FILE *out);
void point_draw(drawingStates *states, U_POINT pt, FILE *out);
void endPathDraw(drawingStates *states, FILE *out);
POINT_D int_el_rad(U_POINTL pt, U_RECTL rect);
void endFormDraw(drawingStates *states, FILE *out);
void color_stroke(drawingStates *states, FILE *out);
void width_stroke(drawingStates *states, FILE *out, double width);
void freeObject(drawingStates *states, uint16_t index);
void cubic_bezier16_draw(const char *name, const char *contents, FILE *out,
                         drawingStates *states, int startingPoint);
void polyline_draw(const char *name, const char *contents, FILE *out,
                   drawingStates *states, bool polygon);
void polypolygon16_draw(const char *name, const char *contents, FILE *out,
                        drawingStates *states, bool polygon);
void U_swap4(void *ul, unsigned int count);
void moveto_draw(const char *name, const char *field1, const char *field2,
                 const char *contents, FILE *out, drawingStates *states);
void polypolygon_draw(const char *name, const char *contents, FILE *out,
                      drawingStates *states, bool polygon);
void polyline16_draw(const char *name, const char *contents, FILE *out,
                     drawingStates *states, bool polygon);
char *base64_encode(const unsigned char *data, size_t input_length,
                    size_t *output_length);

/* prototypes for EMR records */
void U_EMRNOTIMPLEMENTED_draw(const char *name, const char *contents, FILE *out,
                              drawingStates *states);
void U_EMRHEADER_draw(const char *contents, FILE *out, drawingStates *states);
void U_EMRPOLYBEZIER_draw(const char *contents, FILE *out,
                          drawingStates *states);
void U_EMRPOLYGON_draw(const char *contents, FILE *out, drawingStates *states);
void U_EMRPOLYLINE_draw(const char *contents, FILE *out, drawingStates *states);
void U_EMRPOLYBEZIERTO_draw(const char *contents, FILE *out,
                            drawingStates *states);
void U_EMRPOLYLINETO_draw(const char *contents, FILE *out,
                          drawingStates *states);
void U_EMRPOLYPOLYLINE_draw(const char *contents, FILE *out,
                            drawingStates *states);
void U_EMRPOLYPOLYGON_draw(const char *contents, FILE *out,
                           drawingStates *states);
void U_EMRSETWINDOWEXTEX_draw(const char *contents, FILE *out,
                              drawingStates *states);
void U_EMRSETWINDOWORGEX_draw(const char *contents, FILE *out,
                              drawingStates *states);
void U_EMRSETVIEWPORTEXTEX_draw(const char *contents, FILE *out,
                                drawingStates *states);
void U_EMRSETVIEWPORTORGEX_draw(const char *contents, FILE *out,
                                drawingStates *states);
void U_EMRSETBRUSHORGEX_draw(const char *contents, FILE *out,
                             drawingStates *states);
void U_EMREOF_draw(const char *contents, FILE *out, drawingStates *states);
void U_EMRSETPIXELV_draw(const char *contents, FILE *out,
                         drawingStates *states);
void U_EMRSETMAPPERFLAGS_draw(const char *contents, FILE *out,
                              drawingStates *states);
void U_EMRSETMAPMODE_draw(const char *contents, FILE *out,
                          drawingStates *states);
void U_EMRSETBKMODE_draw(const char *contents, FILE *out,
                         drawingStates *states);
void U_EMRSETPOLYFILLMODE_draw(const char *contents, FILE *out,
                               drawingStates *states);
void U_EMRSETROP2_draw(const char *contents, FILE *out, drawingStates *states);
void U_EMRSETSTRETCHBLTMODE_draw(const char *contents, FILE *out,
                                 drawingStates *states);
void U_EMRSETTEXTALIGN_draw(const char *contents, FILE *out,
                            drawingStates *states);
void U_EMRSETCOLORADJUSTMENT_draw(const char *contents, FILE *out,
                                  drawingStates *states);
void U_EMRSETTEXTCOLOR_draw(const char *contents, FILE *out,
                            drawingStates *states);
void U_EMRSETBKCOLOR_draw(const char *contents, FILE *out,
                          drawingStates *states);
void U_EMROFFSETCLIPRGN_draw(const char *contents, FILE *out,
                             drawingStates *states);
void U_EMRMOVETOEX_draw(const char *contents, FILE *out, drawingStates *states);
void U_EMRSETMETARGN_draw(const char *contents, FILE *out,
                          drawingStates *states);
void U_EMREXCLUDECLIPRECT_draw(const char *contents, FILE *out,
                               drawingStates *states);
void U_EMRINTERSECTCLIPRECT_draw(const char *contents, FILE *out,
                                 drawingStates *states);
void U_EMRSCALEVIEWPORTEXTEX_draw(const char *contents, FILE *out,
                                  drawingStates *states);
void U_EMRSCALEWINDOWEXTEX_draw(const char *contents, FILE *out,
                                drawingStates *states);
void U_EMRSAVEDC_draw(const char *contents, FILE *out, drawingStates *states);
void U_EMRRESTOREDC_draw(const char *contents, FILE *out,
                         drawingStates *states);
void U_EMRSETWORLDTRANSFORM_draw(const char *contents, FILE *out,
                                 drawingStates *states);
void U_EMRMODIFYWORLDTRANSFORM_draw(const char *contents, FILE *out,
                                    drawingStates *states);
void U_EMRSELECTOBJECT_draw(const char *contents, FILE *out,
                            drawingStates *states);
void U_EMRCREATEPEN_draw(const char *contents, FILE *out,
                         drawingStates *states);
void U_EMRCREATEBRUSHINDIRECT_draw(const char *contents, FILE *out,
                                   drawingStates *states);
void U_EMRDELETEOBJECT_draw(const char *contents, FILE *out,
                            drawingStates *states);
void U_EMRANGLEARC_draw(const char *contents, FILE *out, drawingStates *states);
void U_EMRELLIPSE_draw(const char *contents, FILE *out, drawingStates *states);
void U_EMRRECTANGLE_draw(const char *contents, FILE *out,
                         drawingStates *states);
void U_EMRROUNDRECT_draw(const char *contents, FILE *out,
                         drawingStates *states);
void U_EMRARC_draw(const char *contents, FILE *out, drawingStates *states);
void U_EMRCHORD_draw(const char *contents, FILE *out, drawingStates *states);
void U_EMRPIE_draw(const char *contents, FILE *out, drawingStates *states);
void U_EMRSELECTPALETTE_draw(const char *contents, FILE *out,
                             drawingStates *states);
void U_EMRCREATEPALETTE_draw(const char *contents, FILE *out,
                             drawingStates *states);
void U_EMRSETPALETTEENTRIES_draw(const char *contents, FILE *out,
                                 drawingStates *states);
void U_EMRRESIZEPALETTE_draw(const char *contents, FILE *out,
                             drawingStates *states);
void U_EMRREALIZEPALETTE_draw(const char *contents, FILE *out,
                              drawingStates *states);
void U_EMREXTFLOODFILL_draw(const char *contents, FILE *out,
                            drawingStates *states);
void U_EMRLINETO_draw(const char *contents, FILE *out, drawingStates *states);
void U_EMRARCTO_draw(const char *contents, FILE *out, drawingStates *states);
void U_EMRPOLYDRAW_draw(const char *contents, FILE *out, drawingStates *states);
void U_EMRSETARCDIRECTION_draw(const char *contents, FILE *out,
                               drawingStates *states);
void U_EMRSETMITERLIMIT_draw(const char *contents, FILE *out,
                             drawingStates *states);
void U_EMRBEGINPATH_draw(const char *contents, FILE *out,
                         drawingStates *states);
void U_EMRENDPATH_draw(const char *contents, FILE *out, drawingStates *states);
void U_EMRCLOSEFIGURE_draw(const char *contents, FILE *out,
                           drawingStates *states);
void U_EMRFILLPATH_draw(const char *contents, FILE *out, drawingStates *states);
void U_EMRSTROKEANDFILLPATH_draw(const char *contents, FILE *out,
                                 drawingStates *states);
void U_EMRSTROKEPATH_draw(const char *contents, FILE *out,
                          drawingStates *states);
void U_EMRFLATTENPATH_draw(const char *contents, FILE *out,
                           drawingStates *states);
void U_EMRWIDENPATH_draw(const char *contents, FILE *out,
                         drawingStates *states);
void U_EMRSELECTCLIPPATH_draw(const char *contents, FILE *out,
                              drawingStates *states);
void U_EMRABORTPATH_draw(const char *contents, FILE *out,
                         drawingStates *states);
bool U_EMRCOMMENT_is_emfplus(const char *contents, const char *blimit);
void U_EMRCOMMENT_draw(const char *contents, FILE *out, drawingStates *states,
                       const char *blimit, size_t off);
void U_EMRFILLRGN_draw(const char *contents, FILE *out, drawingStates *states);
void U_EMRFRAMERGN_draw(const char *contents, FILE *out, drawingStates *states);
void U_EMRINVERTRGN_draw(const char *contents, FILE *out,
                         drawingStates *states);
void U_EMRPAINTRGN_draw(const char *contents, FILE *out, drawingStates *states);
void U_EMREXTSELECTCLIPRGN_draw(const char *contents, FILE *out,
                                drawingStates *states);
void U_EMRBITBLT_draw(const char *contents, FILE *out, drawingStates *states);
void U_EMRSTRETCHBLT_draw(const char *contents, FILE *out,
                          drawingStates *states);
void U_EMRMASKBLT_draw(const char *contents, FILE *out, drawingStates *states);
void U_EMRPLGBLT_draw(const char *contents, FILE *out, drawingStates *states);
void U_EMRSETDIBITSTODEVICE_draw(const char *contents, FILE *out,
                                 drawingStates *states);
void U_EMRSTRETCHDIBITS_draw(const char *contents, FILE *out,
                             drawingStates *states);
void U_EMREXTCREATEFONTINDIRECTW_draw(const char *contents, FILE *out,
                                      drawingStates *states);
void U_EMREXTTEXTOUTA_draw(const char *contents, FILE *out,
                           drawingStates *states);
void U_EMREXTTEXTOUTW_draw(const char *contents, FILE *out,
                           drawingStates *states);
void U_EMRPOLYBEZIER16_draw(const char *contents, FILE *out,
                            drawingStates *states);
void U_EMRPOLYGON16_draw(const char *contents, FILE *out,
                         drawingStates *states);
void U_EMRPOLYLINE16_draw(const char *contents, FILE *out,
                          drawingStates *states);
void U_EMRPOLYBEZIERTO16_draw(const char *contents, FILE *out,
                              drawingStates *states);
void U_EMRPOLYLINETO16_draw(const char *contents, FILE *out,
                            drawingStates *states);
void U_EMRPOLYPOLYLINE16_draw(const char *contents, FILE *out,
                              drawingStates *states);
void U_EMRPOLYPOLYGON16_draw(const char *contents, FILE *out,
                             drawingStates *states);
void U_EMRPOLYDRAW16_draw(const char *contents, FILE *out,
                          drawingStates *states);
void U_EMRCREATEMONOBRUSH_draw(const char *contents, FILE *out,
                               drawingStates *states);
void U_EMRCREATEDIBPATTERNBRUSHPT_draw(const char *contents, FILE *out,
                                       drawingStates *states);
void U_EMREXTCREATEPEN_draw(const char *contents, FILE *out,
                            drawingStates *states);
void U_EMRSETICMMODE_draw(const char *contents, FILE *out,
                          drawingStates *states);
void U_EMRCREATECOLORSPACE_draw(const char *contents, FILE *out,
                                drawingStates *states);
void U_EMRSETCOLORSPACE_draw(const char *contents, FILE *out,
                             drawingStates *states);
void U_EMRDELETECOLORSPACE_draw(const char *contents, FILE *out,
                                drawingStates *states);
void U_EMRPIXELFORMAT_draw(const char *contents, FILE *out,
                           drawingStates *states);
void U_EMRSMALLTEXTOUT_draw(const char *contents, FILE *out,
                            drawingStates *states);
void U_EMRALPHABLEND_draw(const char *contents, FILE *out,
                          drawingStates *states);
void U_EMRSETLAYOUT_draw(const char *contents, FILE *out,
                         drawingStates *states);
void U_EMRTRANSPARENTBLT_draw(const char *contents, FILE *out,
                              drawingStates *states);
void U_EMRGRADIENTFILL_draw(const char *contents, FILE *out,
                            drawingStates *states);
void U_EMRCREATECOLORSPACEW_draw(const char *contents, FILE *out,
                                 drawingStates *states);
int U_emf_onerec_draw(const char *contents, const char *blimit, int recnum,
                      size_t off, FILE *out, drawingStates *states);
void dib_img_writer(const char *contents, FILE *out, drawingStates *states,
                    PU_BITMAPINFOHEADER BmiSrc, const unsigned char *BmpSrc,
                    size_t size, bool assign_mono_colors_from_dc);
emfImageLibrary *image_library_writer(const char *contents, FILE *out,
                                      drawingStates *states,
                                      PU_BITMAPINFOHEADER BmiSrc, size_t size,
                                      const unsigned char *BmpSrc);
emfImageLibrary *image_library_create(int id, PU_BITMAPINFOHEADER BmiSrc,
                                      size_t size);
emfImageLibrary *image_library_add(drawingStates *states,
                                   PU_BITMAPINFOHEADER BmiSrc, size_t size);
emfImageLibrary *image_library_find(emfImageLibrary *lib,
                                    PU_BITMAPINFOHEADER BmiSrc, size_t size);
void freeEmfImageLibrary(drawingStates *states);
void text_style_draw(FILE *out, drawingStates *states, POINT_D Org);
void char_to_utf16(char *in, size_t size_in, char **out);
void text_convert(char *in, size_t size_in, char **out, size_t *size_out,
                  uint8_t type, drawingStates *states);
void text_draw(const char *contents, FILE *out, drawingStates *states,
               uint8_t type);
void clipset_draw(drawingStates *states, FILE *out);
void free_path(PATH **path);
void add_new_seg(PATH **path, uint8_t type);
POINT_D point_s(drawingStates *states, U_POINT pt);
POINT_D point_s16(drawingStates *states, U_POINT16 pt);
void addNewSegPath(drawingStates *states, uint8_t type);
void pointCurrPathAddD(drawingStates *states, POINT_D pt, int index);
void pointCurrPathAdd(drawingStates *states, U_POINT pt, int index);
void pointCurrPathAdd16(drawingStates *states, U_POINT16 pt, int index);
void clip_rgn_mix(drawingStates *states, PATH *path, uint32_t mode);
void clip_rgn_draw(drawingStates *states, FILE *out);
void copy_path(PATH *in, PATH **out);
void offset_path(PATH *in, POINT_D pt);
void draw_path(PATH *in, FILE *out);
void point_draw_raw_d(POINT_D pt, FILE *out);
int get_id(drawingStates *states);
//! \endcond

#ifdef __cplusplus
}
#endif

/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
