#ifdef __cplusplus
extern "C" {
#endif

#ifndef DARWIN
#define _POSIX_C_SOURCE 200809L
#endif
#include "emf2svg_private.h"
#include "emf2svg_print.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iconv.h>
#include <errno.h>
#include <ft2build.h>
#include <fontconfig/fontconfig.h>
#include FT_FREETYPE_H

void U_EMRNOTIMPLEMENTED_draw(const char *name, const char *contents, FILE *out,
                              drawingStates *states) {
    UNUSED(name);
    // if (states->verbose){U_EMRNOTIMPLEMENTED_print(contents, states);}
    UNUSED(contents);
}
void U_swap4(void *ul, unsigned int count);
//! \endcond

/**
  \brief Print rect and rectl objects from Upper Left and Lower Right corner
  points.
  \param rect U_RECTL object
  */
#ifndef _MSC_VER
double _dsign(double v) {
    if (v >= 0)
        return 1;
    else
        return -1;
}
#endif

void arc_circle_draw(const char *contents, FILE *out, drawingStates *states) {
    PU_EMRANGLEARC pEmr = (PU_EMRANGLEARC)(contents);
    startPathDraw(states, out);
    U_POINTL radii;
    int sweep_flag = 0;
    int large_arc_flag = 0;
    // FIXME calculate the real orientation
    if (states->currentDeviceContext.arcdir > 0) {
        sweep_flag = 1;
        large_arc_flag = 1;
    } else {
        sweep_flag = 0;
        large_arc_flag = 0;
    }
    radii.x = pEmr->nRadius;
    radii.y = pEmr->nRadius;

    fprintf(out, "M ");
    POINT_D start;
    double angle = pEmr->eStartAngle * U_PI / 180;

    addNewSegPath(states, SEG_LINE);
    start.x = pEmr->nRadius * cos(angle) + pEmr->ptlCenter.x;
    start.y = pEmr->nRadius * sin(angle) + pEmr->ptlCenter.y;
    point_draw_d(states, start, out);
    pointCurrPathAddD(states, start, 0);

    addNewSegPath(states, SEG_ARC);
    fprintf(out, "A ");
    point_draw(states, radii, out);
    pointCurrPathAdd(states, radii, 0);

    fprintf(out, "0 ");
    fprintf(out, "%d %d ", large_arc_flag, sweep_flag);

    angle = (pEmr->eStartAngle + pEmr->eSweepAngle) * U_PI / 180;
    POINT_D end;
    end.x = pEmr->nRadius * cos(angle) + pEmr->ptlCenter.x;
    end.y = pEmr->nRadius * sin(angle) + pEmr->ptlCenter.y;
    point_draw_d(states, end, out);
    pointCurrPathAddD(states, end, 1);

    endPathDraw(states, out);
}
void arc_draw(const char *contents, FILE *out, drawingStates *states,
              int type) {
    PU_EMRARC pEmr = (PU_EMRARC)(contents);
    startPathDraw(states, out);
    U_POINTL radii;
    int sweep_flag = 0;
    int large_arc_flag = 0;
    // FIXME calculate the real orientation
    if (states->currentDeviceContext.arcdir > 0) {
        sweep_flag = 1;
        large_arc_flag = 1;
    } else {
        sweep_flag = 0;
        large_arc_flag = 0;
    }
    radii.x = (pEmr->rclBox.right - pEmr->rclBox.left) / 2;
    radii.y = (pEmr->rclBox.bottom - pEmr->rclBox.top) / 2;

    addNewSegPath(states, SEG_LINE);
    fprintf(out, "M ");
    POINT_D start = int_el_rad(pEmr->ptlStart, pEmr->rclBox);
    point_draw_d(states, start, out);
    pointCurrPathAddD(states, start, 0);

    addNewSegPath(states, SEG_ARC);

    fprintf(out, "A ");
    point_draw(states, radii, out);
    pointCurrPathAdd(states, radii, 0);

    fprintf(out, "0 ");
    fprintf(out, "%d %d ", large_arc_flag, sweep_flag);

    POINT_D end = int_el_rad(pEmr->ptlEnd, pEmr->rclBox);
    point_draw_d(states, end, out);
    pointCurrPathAddD(states, end, 1);

    switch (type) {
    case ARC_PIE:
        fprintf(out, "L ");
        U_POINTL center;
        center.x = (pEmr->rclBox.right + pEmr->rclBox.left) / 2;
        center.y = (pEmr->rclBox.bottom + pEmr->rclBox.top) / 2;
        point_draw(states, center, out);
        addNewSegPath(states, SEG_LINE);
        pointCurrPathAdd(states, center, 0);
        fprintf(out, "Z ");
        addNewSegPath(states, SEG_END);
        endFormDraw(states, out);
        break;
    case ARC_CHORD:
        fprintf(out, "Z ");
        addNewSegPath(states, SEG_END);
        endFormDraw(states, out);
        break;
    default:
        endPathDraw(states, out);
        break;
    }
}
void basic_stroke(drawingStates *states, FILE *out) {
    color_stroke(states, out);
    width_stroke(states, out, states->currentDeviceContext.stroke_width);
}
bool checkOutOfEMF(drawingStates *states, uintptr_t address) {
    if (address > states->endAddress) {
        states->Error = true;
        return true;
    } else {
        return false;
    }
}
bool checkOutOfOTIndex(drawingStates *states, int64_t index) {
    if (index > states->objectTableSize) {
        states->Error = true;
        return true;
    } else {
        return false;
    }
}
void color_stroke(drawingStates *states, FILE *out) {
    fprintf(out, "stroke=\"#%02X%02X%02X\" ",
            states->currentDeviceContext.stroke_red,
            states->currentDeviceContext.stroke_green,
            states->currentDeviceContext.stroke_blue);
}
void copyDeviceContext(EMF_DEVICE_CONTEXT *dest, EMF_DEVICE_CONTEXT *src) {
    // copy simple data (int, double...)
    *dest = *src;

    // copy more complex data (pointers...)
    if (src->font_name != NULL) {
        dest->font_name =
            (char *)calloc(strlen(src->font_name) + 1, sizeof(char));
        strcpy(dest->font_name, src->font_name);
    }
    if (src->font_family != NULL) {
        dest->font_family =
            (char *)calloc(strlen(src->font_family) + 1, sizeof(char));
        strcpy(dest->font_family, src->font_family);
    }
    copy_path(src->clipRGN, &(dest->clipRGN));
}
void cubic_bezier16_draw(const char *name, const char *contents, FILE *out,
                         drawingStates *states, int startingPoint) {
    UNUSED(name);
    unsigned int i;
    PU_EMRPOLYBEZIER16 pEmr = (PU_EMRPOLYBEZIER16)(contents);
    startPathDraw(states, out);
    PU_POINT16 papts = (PU_POINT16)(&(pEmr->apts));
    returnOutOfEmf((intptr_t)papts +
                   (intptr_t)(pEmr->cpts) * sizeof(U_POINT16));
    if (startingPoint == 1) {
        fprintf(out, "M ");
        point16_draw(states, papts[0], out);
        addNewSegPath(states, SEG_MOVE);
        pointCurrPathAdd16(states, papts[0], 0);
    }
    const int ctrl1 = (0 + startingPoint) % 3;
    const int ctrl2 = (1 + startingPoint) % 3;
    const int to = (2 + startingPoint) % 3;
    int index = 0;
    for (i = startingPoint; i < pEmr->cpts; i++) {
        if ((i % 3) == ctrl1) {
            index = 0;
            addNewSegPath(states, SEG_BEZIER);
            pointCurrPathAdd16(states, papts[i], index);
            index++;
            fprintf(out, "C ");
            point16_draw(states, papts[i], out);
        } else if ((i % 3) == ctrl2) {
            point16_draw(states, papts[i], out);
            pointCurrPathAdd16(states, papts[i], index);
            index++;
        } else if ((i % 3) == to) {
            point16_draw(states, papts[i], out);
            pointCurrPathAdd16(states, papts[i], index);
            index++;
        }
    }
    endPathDraw(states, out);
}
void cubic_bezier_draw(const char *name, const char *contents, FILE *out,
                       drawingStates *states, int startingPoint) {
    UNUSED(name);
    unsigned int i;
    PU_EMRPOLYBEZIER pEmr = (PU_EMRPOLYBEZIER)(contents);
    startPathDraw(states, out);
    PU_POINT papts = (PU_POINT)(&(pEmr->aptl));
    returnOutOfEmf((intptr_t)papts + (intptr_t)pEmr->cptl * sizeof(U_POINT));
    if (startingPoint == 1) {
        fprintf(out, "M ");
        point_draw(states, papts[0], out);
        addNewSegPath(states, SEG_BEZIER);
        pointCurrPathAdd(states, papts[0], 0);
    }
    const int ctrl1 = (0 + startingPoint) % 3;
    const int ctrl2 = (1 + startingPoint) % 3;
    const int to = (2 + startingPoint) % 3;
    int index = 0;
    for (i = startingPoint; i < pEmr->cptl; i++) {
        if ((i % 3) == ctrl1) {
            index = 0;
            addNewSegPath(states, SEG_BEZIER);
            pointCurrPathAdd(states, papts[i], index);
            index++;
            fprintf(out, "C ");
            point_draw(states, papts[i], out);
        } else if ((i % 3) == ctrl2) {
            point_draw(states, papts[i], out);
            pointCurrPathAdd(states, papts[i], index);
            index++;
        } else if ((i % 3) == to) {
            point_draw(states, papts[i], out);
            pointCurrPathAdd(states, papts[i], index);
            index++;
        }
    }
    endPathDraw(states, out);
}
void endFormDraw(drawingStates *states, FILE *out) {
    if (!(states->inPath)) {
        fprintf(out, "\" ");
        bool filled = false;
        bool stroked = false;
        stroke_draw(states, out, &filled, &stroked);
        fill_draw(states, out, &filled, &stroked);
        clipset_draw(states, out);
        if (!filled)
            fprintf(out, "fill=\"none\" ");
        if (!stroked)
            fprintf(out, "stroke=\"none\" ");
        fprintf(out, " />\n");
    }
}
void endPathDraw(drawingStates *states, FILE *out) {
    if (!(states->inPath)) {
        fprintf(out, "\" ");
        bool filled;
        bool stroked;
        stroke_draw(states, out, &filled, &stroked);
        fprintf(out, " fill=\"none\" />\n");
    }
}
void fill_draw(drawingStates *states, FILE *out, bool *filled, bool *stroked) {
    if (states->verbose) {
        fill_print(states);
    }
    char *fill_rule = calloc(40, sizeof(char));
    switch (states->currentDeviceContext.fill_mode) {
    case (U_ALTERNATE):
        sprintf(fill_rule, "fill-rule:\"evenodd\" ");
        break;
    case (U_WINDING):
        sprintf(fill_rule, "fill-rule:\"nonzero\" ");
        break;
    default:
        sprintf(fill_rule, " ");
        break;
    }
    switch (states->currentDeviceContext.fill_mode) {
    case U_BS_SOLID:
        *filled = true;
        fprintf(out, "%s", fill_rule);
        fprintf(out, "fill=\"#%02X%02X%02X\" ",
                states->currentDeviceContext.fill_red,
                states->currentDeviceContext.fill_green,
                states->currentDeviceContext.fill_blue);
        break;
    case U_BS_NULL:
        fprintf(out, "fill=\"none\" ");
        *filled = true;
        break;
    case U_BS_MONOPATTERN:
        fprintf(out, "fill=\"#img-%d-ref\" ",
                states->currentDeviceContext.fill_idx);
        *filled = true;
        break;
    case U_BS_HATCHED:
    case U_BS_PATTERN:
    case U_BS_INDEXED:
    case U_BS_DIBPATTERN:
    case U_BS_DIBPATTERNPT:
    case U_BS_PATTERN8X8:
    case U_BS_DIBPATTERN8X8:
    default:
        // partial
        fprintf(out, "fill=\"#%02X%02X%02X\" ",
                states->currentDeviceContext.fill_red,
                states->currentDeviceContext.fill_green,
                states->currentDeviceContext.fill_blue);
        *filled = true;
        break;
    }
    free(fill_rule);
    return;
}
void freeDeviceContext(EMF_DEVICE_CONTEXT *dc) {
    if (dc != NULL) {
        if (dc->font_name != NULL)
            free(dc->font_name);
        if (dc->font_family != NULL)
            free(dc->font_family);
        free_path(&(dc->clipRGN));
    }
}
void freeDeviceContextStack(drawingStates *states) {
    EMF_DEVICE_CONTEXT_STACK *stack_entry = states->DeviceContextStack;
    while (stack_entry != NULL) {
        EMF_DEVICE_CONTEXT_STACK *next_entry = stack_entry->previous;
        freeDeviceContext(&(stack_entry->DeviceContext));
        free(stack_entry);
        stack_entry = next_entry;
    }
}
void freeObject(drawingStates *states, uint16_t index) {
    if (states->objectTable[index].font_name != NULL)
        free(states->objectTable[index].font_name);
    if (states->objectTable[index].font_family != NULL)
        free(states->objectTable[index].font_family);
    states->objectTable[index] = (const emfGraphObject){0};
}
void freeObjectTable(drawingStates *states) {
    for (int32_t i = 0; i < (states->objectTableSize + 1); i++) {
        freeObject(states, i);
    }
}
void freePathStack(pathStack *stack) {
    while (stack != NULL) {
        // free(stack->pathStruct);
        pathStack *tmp = stack;
        stack = stack->next;
        free(tmp);
    }
    return;
}
int get_id(drawingStates *states) {
    states->uniqId = rand();
    return states->uniqId;
}
POINT_D int_el_rad(U_POINTL pt, U_RECTL rect) {
    POINT_D center, intersect, radii, pt_no;
    center.x = (rect.right + rect.left) / 2;
    center.y = (rect.bottom + rect.top) / 2;

    radii.x = (rect.right - rect.left) / 2;
    radii.y = (rect.bottom - rect.top) / 2;

    if ((radii.x == 0) || (radii.y == 0)) {
        return center;
    }

    // change orgin (new origin is ellipse center)
    pt_no.x = pt.x - center.x;
    pt_no.y = pt.y - center.y;

    if (pt_no.x == 0) {
        intersect.x = center.x;
        intersect.y = _dsign(pt_no.y) * radii.y + center.y;
        return intersect;
    }

    if (pt_no.y == 0) {
        intersect.x = _dsign(pt_no.x) * radii.x + center.x;
        intersect.y = center.y;
        return intersect;
    }

    // slope of the radial
    double slope = pt_no.y / pt_no.x;

    // Calculate the intersection.
    // With center as the origin:
    // * ellipse equation is: (x / radii.x)^2 + (y / radii.y) + 1
    // * radial equation is:  y = x * slope
    // Three part of the calculus:
    // * '_dsign(...) *' -> get correct quadrant
    // * 'sqrt(...)'     -> solve the equation
    // * '+ center.x/y'  -> back the EMF origin
    intersect.x =
        _dsign(pt_no.x) *
            sqrt(1 / ((pow(1 / radii.x, 2)) + pow((slope / radii.y), 2))) +
        center.x;
    intersect.y = _dsign(pt_no.y) * sqrt(1 / ((pow(1 / (slope * radii.x), 2)) +
                                              pow((1 / radii.y), 2))) +
                  center.y;

    return intersect;
}
void lineto_draw(const char *name, const char *field1, const char *field2,
                 const char *contents, FILE *out, drawingStates *states) {
    UNUSED(name);
    PU_EMRGENERICPAIR pEmr = (PU_EMRGENERICPAIR)(contents);
    startPathDraw(states, out);
    fprintf(out, "L ");
    point_draw(states, pEmr->pair, out);
    addNewSegPath(states, SEG_LINE);
    pointCurrPathAdd(states, pEmr->pair, 0);
    endPathDraw(states, out);
}
void moveto_draw(const char *name, const char *field1, const char *field2,
                 const char *contents, FILE *out, drawingStates *states) {
    UNUSED(name);
    PU_EMRGENERICPAIR pEmr = (PU_EMRGENERICPAIR)(contents);
    point_draw(states, pEmr->pair, out);
    addNewSegPath(states, SEG_MOVE);
    pointCurrPathAdd(states, pEmr->pair, 0);
}
void newPathStruct(drawingStates *states) {
    pathStack *new_entry = calloc(1, sizeof(pathStack));
    if (states->emfStructure.pathStack == NULL) {
        states->emfStructure.pathStack = new_entry;
        states->emfStructure.pathStackLast = new_entry;
    } else {
        states->emfStructure.pathStackLast->next = new_entry;
        states->emfStructure.pathStackLast = new_entry;
    }
}
void no_stroke(drawingStates *states, FILE *out) {
    if (states->currentDeviceContext.fill_mode != U_BS_NULL) {
        fprintf(out, "stroke-width=\"1px\" ");
        fprintf(out, "stroke=\"#%02X%02X%02X\" ",
                states->currentDeviceContext.fill_red,
                states->currentDeviceContext.fill_green,
                states->currentDeviceContext.fill_blue);
    } else {
        fprintf(out, "stroke=\"none\" ");
        fprintf(out, "stroke-width=\"0.0\" ");
    }
}
void point16_draw(drawingStates *states, U_POINT16 pt, FILE *out) {
    POINT_D ptd = point_cal(states, (double)pt.x, (double)pt.y);
    states->cur_x = pt.x;
    states->cur_y = pt.y;
    fprintf(out, "%.4f,%.4f ", ptd.x, ptd.y);
}

double scaleX(drawingStates *states, double x) {
    double ret;
    double scalingX;

    switch (states->MapMode) {
    case U_MM_TEXT:
        scalingX = 1.0;
        break;
    case U_MM_LOMETRIC:
        // convert to 0.1 mm to pixel and invert Y
        scalingX = states->pxPerMm * 0.1 * 1;
        break;
    case U_MM_HIMETRIC:
        // convert to 0.01 mm to pixel and invert Y
        scalingX = states->pxPerMm * 0.01 * 1;
        break;
    case U_MM_LOENGLISH:
        // convert to 0.01 inch to pixel and invert Y
        scalingX = states->pxPerMm * 0.01 * mmPerInch * 1;
        break;
    case U_MM_HIENGLISH:
        // convert to 0.001 inch to pixel and invert Y
        scalingX = states->pxPerMm * 0.001 * mmPerInch * 1;
        break;
    case U_MM_TWIPS:
        // convert to 1 twips to pixel and invert Y
        scalingX = states->pxPerMm / 1440 * mmPerInch * 1;
        break;
    case U_MM_ISOTROPIC:
        if (states->windowExSet && states->viewPortExSet) {
            scalingX = states->viewPortExX / states->windowExX;
        } else {
            scalingX = 1.0;
        }
        break;
    case U_MM_ANISOTROPIC:
        if (states->windowExSet && states->viewPortExSet) {
            scalingX = states->viewPortExX / states->windowExX;
        } else {
            scalingX = 1.0;
        }
        break;
    default:
        scalingX = 1.0;
    }
    ret = x * scalingX * states->scaling;
    return ret;
}

double scaleY(drawingStates *states, double y) {
    double ret;
    double scalingY;

    switch (states->MapMode) {
    case U_MM_TEXT:
        scalingY = 1.0;
        break;
    case U_MM_LOMETRIC:
        // convert to 0.1 mm to pixel and invert Y
        scalingY = states->pxPerMm * 0.1 * 1;
        break;
    case U_MM_HIMETRIC:
        // convert to 0.01 mm to pixel and invert Y
        scalingY = states->pxPerMm * 0.01 * 1;
        break;
    case U_MM_LOENGLISH:
        // convert to 0.01 inch to pixel and invert Y
        scalingY = states->pxPerMm * 0.01 * mmPerInch * 1;
        break;
    case U_MM_HIENGLISH:
        // convert to 0.001 inch to pixel and invert Y
        scalingY = states->pxPerMm * 0.001 * mmPerInch * 1;
        break;
    case U_MM_TWIPS:
        // convert to 1 twips to pixel and invert Y
        scalingY = states->pxPerMm / 1440 * mmPerInch * 1;
        break;
    case U_MM_ISOTROPIC:
        if (states->windowExSet && states->viewPortExSet) {
            scalingY = states->viewPortExX / states->windowExX;
        } else {
            scalingY = 1.0;
        }
        break;
    case U_MM_ANISOTROPIC:
        if (states->windowExSet && states->viewPortExSet) {
            scalingY = states->viewPortExY / states->windowExY;
        } else {
            scalingY = 1.0;
        }
        break;
    default:
        scalingY = 1.0;
    }
    ret = y * scalingY * states->scaling;
    return ret;
}

POINT_D point_cal(drawingStates *states, double x, double y) {
    POINT_D ret;
    double scalingX;
    double scalingY;
    double windowOrgX = 0.0;
    double windowOrgY = 0.0;
    double viewPortOrgX = 0.0;
    double viewPortOrgY = 0.0;

    switch (states->MapMode) {
    case U_MM_TEXT:
        scalingX = 1.0;
        scalingY = 1.0;
        break;
    case U_MM_LOMETRIC:
        // convert to 0.1 mm to pixel and invert Y
        scalingX = states->pxPerMm * 0.1 * 1;
        scalingY = states->pxPerMm * 0.1 * -1;
        break;
    case U_MM_HIMETRIC:
        // convert to 0.01 mm to pixel and invert Y
        scalingX = states->pxPerMm * 0.01 * 1;
        scalingY = states->pxPerMm * 0.01 * -1;
        break;
    case U_MM_LOENGLISH:
        // convert to 0.01 inch to pixel and invert Y
        scalingX = states->pxPerMm * 0.01 * mmPerInch * 1;
        scalingY = states->pxPerMm * 0.01 * mmPerInch * -1;
        break;
    case U_MM_HIENGLISH:
        // convert to 0.001 inch to pixel and invert Y
        scalingX = states->pxPerMm * 0.001 * mmPerInch * 1;
        scalingY = states->pxPerMm * 0.001 * mmPerInch * -1;
        break;
    case U_MM_TWIPS:
        // convert to 1 twips to pixel and invert Y
        scalingX = states->pxPerMm / 1440 * mmPerInch * 1;
        scalingY = states->pxPerMm / 1440 * mmPerInch * -1;
        break;
    case U_MM_ISOTROPIC:
        if (states->windowExSet && states->viewPortExSet) {
            scalingX = states->viewPortExX / states->windowExX;
        } else {
            scalingX = 1.0;
        }
        scalingY = scalingX;
        windowOrgX = states->windowOrgX;
        windowOrgY = states->windowOrgY;
        viewPortOrgX = states->viewPortOrgX;
        viewPortOrgY = states->viewPortOrgY;
        break;
    case U_MM_ANISOTROPIC:
        if (states->windowExSet && states->viewPortExSet) {
            scalingX = states->viewPortExX / states->windowExX;
            scalingY = states->viewPortExY / states->windowExY;
        } else {
            scalingX = 1.0;
            // If fixBrokenYTransform is true, we have to flip the Y axis.
            scalingY = (states->fixBrokenYTransform ? -1.0 : 1.0);
        }
        windowOrgX = states->windowOrgX;
        windowOrgY = states->windowOrgY;
        viewPortOrgX = states->viewPortOrgX;
        viewPortOrgY = states->viewPortOrgY;
        break;
    default:
        scalingX = 1.0;
        scalingY = 1.0;
    }
    ret.x = ((x - windowOrgX) * scalingX + viewPortOrgX) * states->scaling;
    ret.y = ((y - windowOrgY) * scalingY + viewPortOrgY) * states->scaling;
    return ret;
}

POINT_D point_s(drawingStates *states, U_POINT pt) {
    return point_cal(states, (double)pt.x, (double)pt.y);
}

POINT_D point_s16(drawingStates *states, U_POINT16 pt) {
    return point_cal(states, (double)pt.x, (double)pt.y);
}

void point_draw(drawingStates *states, U_POINT pt, FILE *out) {
    POINT_D ptd = point_cal(states, (double)pt.x, (double)pt.y);
    states->cur_x = pt.x;
    states->cur_y = pt.y;
    fprintf(out, "%.4f,%.4f ", ptd.x, ptd.y);
}
void point_draw_d(drawingStates *states, POINT_D pt, FILE *out) {
    POINT_D ptd = point_cal(states, pt.x, pt.y);
    states->cur_x = pt.x;
    states->cur_y = pt.y;
    fprintf(out, "%.4f,%.4f ", ptd.x, ptd.y);
}

void point_draw_raw_d(POINT_D pt, FILE *out) {
    fprintf(out, "%.4f,%.4f ", pt.x, pt.y);
}
void polyline16_draw(const char *name, const char *contents, FILE *out,
                     drawingStates *states, bool polygon) {
    UNUSED(name);
    unsigned int i;
    PU_EMRPOLYBEZIER16 pEmr = (PU_EMRPOLYBEZIER16)(contents);
    PU_POINT16 papts = (PU_POINT16)(&(pEmr->apts));
    returnOutOfEmf((intptr_t)papts +
                   (intptr_t)(pEmr->cpts) * sizeof(U_POINT16));
    startPathDraw(states, out);
    for (i = 0; i < pEmr->cpts; i++) {
        if (polygon && i == 0) {
            fprintf(out, "M ");
            addNewSegPath(states, SEG_MOVE);
        } else {
            fprintf(out, "L ");
            addNewSegPath(states, SEG_LINE);
        }
        pointCurrPathAdd16(states, papts[i], 0);
        point16_draw(states, papts[i], out);
    }
    endPathDraw(states, out);
}
void polyline_draw(const char *name, const char *contents, FILE *out,
                   drawingStates *states, bool polygon) {
    UNUSED(name);
    unsigned int i;
    PU_EMRPOLYLINETO pEmr = (PU_EMRPOLYLINETO)(contents);
    startPathDraw(states, out);
    PU_POINT papts = (PU_POINT)(&(pEmr->aptl));
    returnOutOfEmf((intptr_t)papts + (intptr_t)(pEmr->cptl) * sizeof(U_POINT));
    for (i = 0; i < pEmr->cptl; i++) {
        if (polygon && i == 0) {
            fprintf(out, "M ");
            addNewSegPath(states, SEG_MOVE);
        } else {
            fprintf(out, "L ");
            addNewSegPath(states, SEG_LINE);
        }
        point_draw(states, pEmr->aptl[i], out);
        pointCurrPathAdd(states, pEmr->aptl[i], 0);
    }
    endPathDraw(states, out);
}
void polypolygon16_draw(const char *name, const char *contents, FILE *out,
                        drawingStates *states, bool polygon) {
    UNUSED(name);
    unsigned int i;
    PU_EMRPOLYPOLYLINE16 pEmr = (PU_EMRPOLYPOLYLINE16)(contents);
    PU_POINT16 papts = (PU_POINT16)((char *)pEmr->aPolyCounts +
                                    sizeof(uint32_t) * pEmr->nPolys);
    returnOutOfEmf((intptr_t)papts +
                   (intptr_t)(pEmr->cpts) * sizeof(U_POINT16));

    int counter = 0;
    int polygon_index = 0;
    for (i = 0; i < pEmr->cpts; i++) {
        if (counter == 0) {
            fprintf(out, "M ");
            point16_draw(states, papts[i], out);
            addNewSegPath(states, SEG_MOVE);
            pointCurrPathAdd16(states, papts[i], 0);
        } else {
            fprintf(out, "L ");
            point16_draw(states, papts[i], out);
            addNewSegPath(states, SEG_LINE);
            pointCurrPathAdd16(states, papts[i], 0);
        }
        counter++;
        if (pEmr->aPolyCounts[polygon_index] == counter) {
            if (polygon) {
                fprintf(out, "Z ");
                addNewSegPath(states, SEG_END);
            }
            counter = 0;
            polygon_index++;
        }
    }
}
void polypolygon_draw(const char *name, const char *contents, FILE *out,
                      drawingStates *states, bool polygon) {
    UNUSED(name);
    unsigned int i;
    PU_EMRPOLYPOLYLINE16 pEmr = (PU_EMRPOLYPOLYLINE16)(contents);
    PU_POINT papts =
        (PU_POINT)((char *)pEmr->aPolyCounts + sizeof(uint32_t) * pEmr->nPolys);

    int counter = 0;
    int polygon_index = 0;
    returnOutOfEmf((intptr_t)papts + (intptr_t)(pEmr->cpts) * sizeof(U_POINT));
    for (i = 0; i < pEmr->cpts; i++) {
        if (counter == 0) {
            fprintf(out, "M ");
            point_draw(states, papts[i], out);
            addNewSegPath(states, SEG_MOVE);
            pointCurrPathAdd(states, papts[i], 0);
        } else {
            fprintf(out, "L ");
            point_draw(states, papts[i], out);
            addNewSegPath(states, SEG_LINE);
            pointCurrPathAdd(states, papts[i], 0);
        }
        counter++;
        if (pEmr->aPolyCounts[polygon_index] == counter) {
            if (polygon) {
                fprintf(out, "Z ");
                addNewSegPath(states, SEG_END);
            }
            counter = 0;
            polygon_index++;
        }
    }
}
void rectl_draw(drawingStates *states, FILE *out, U_RECTL rect) {
    U_POINT pt;
    fprintf(out, "M ");
    pt.x = rect.left;
    pt.y = rect.top;
    addNewSegPath(states, SEG_MOVE);
    pointCurrPathAdd(states, pt, 0);
    point_draw(states, pt, out);
    fprintf(out, "L ");
    pt.x = rect.right;
    pt.y = rect.top;
    addNewSegPath(states, SEG_LINE);
    pointCurrPathAdd(states, pt, 0);
    point_draw(states, pt, out);
    fprintf(out, "L ");
    pt.x = rect.right;
    pt.y = rect.bottom;
    addNewSegPath(states, SEG_LINE);
    pointCurrPathAdd(states, pt, 0);
    point_draw(states, pt, out);
    fprintf(out, "L ");
    pt.x = rect.left;
    pt.y = rect.bottom;
    addNewSegPath(states, SEG_LINE);
    pointCurrPathAdd(states, pt, 0);
    point_draw(states, pt, out);
    fprintf(out, "L ");
    pt.x = rect.left;
    pt.y = rect.top;
    addNewSegPath(states, SEG_LINE);
    pointCurrPathAdd(states, pt, 0);
    point_draw(states, pt, out);
    fprintf(out, "Z ");
    addNewSegPath(states, SEG_END);
}
void restoreDeviceContext(drawingStates *states, int32_t index) {
    EMF_DEVICE_CONTEXT_STACK *stack_entry = states->DeviceContextStack;
    // we recover the 'abs(index)' element of the stack
    // we stop if the index was outside the DeviceContextStack
    int i = -1;
    for (; i > index && stack_entry != NULL; i--) {
        if (stack_entry->previous != NULL) {
            stack_entry = stack_entry->previous;
        } else {
            break;
        }
    }
    if (stack_entry == NULL || i != index) {
        states->Error = true;
        return;
    }
    // we copy it as the current device context
    freeDeviceContext(&(states->currentDeviceContext));
    states->currentDeviceContext = (EMF_DEVICE_CONTEXT){0};
    copyDeviceContext(&(states->currentDeviceContext),
                      &(stack_entry->DeviceContext));
}
void saveDeviceContext(drawingStates *states) {
    // create the new device context in the stack
    EMF_DEVICE_CONTEXT_STACK *new_entry =
        (EMF_DEVICE_CONTEXT_STACK *)calloc(1, sizeof(EMF_DEVICE_CONTEXT_STACK));
    copyDeviceContext(&(new_entry->DeviceContext),
                      &(states->currentDeviceContext));
    // put the new entry on the stack
    new_entry->previous = states->DeviceContextStack;
    states->DeviceContextStack = new_entry;
}
void setTransformIdentity(drawingStates *states) {
    states->currentDeviceContext.worldTransform.eM11 = 1.0;
    states->currentDeviceContext.worldTransform.eM12 = 0.0;
    states->currentDeviceContext.worldTransform.eM21 = 0.0;
    states->currentDeviceContext.worldTransform.eM22 = 1.0;
    states->currentDeviceContext.worldTransform.eDx = 0.0;
    states->currentDeviceContext.worldTransform.eDy = 0.0;
}
void startPathDraw(drawingStates *states, FILE *out) {
    if (!(states->inPath)) {
        fprintf(out, "<%spath ", states->nameSpaceString);
        clipset_draw(states, out);
        fprintf(out, "d=\"M ");
        U_POINT pt;
        pt.x = states->cur_x;
        pt.y = states->cur_y;
        point_draw(states, pt, out);
        addNewSegPath(states, SEG_MOVE);
        pointCurrPathAdd(states, pt, 0);
    }
}
void stroke_draw(drawingStates *states, FILE *out, bool *filled,
                 bool *stroked) {
    float unit_stroke =
        states->currentDeviceContext.stroke_width * states->scaling;
    float dash_len = unit_stroke * 5;
    float dot_len = unit_stroke;
    if (states->verbose) {
        stroke_print(states);
    }

    if ((states->currentDeviceContext.stroke_mode & 0x000000FF) == U_PS_NULL) {
        // no stroke with the fill color with a with of 1px
        no_stroke(states, out);
        *stroked = true;
        return;
    }
    // pen type
    switch (states->currentDeviceContext.stroke_mode & 0x000F0000) {
    case U_PS_COSMETIC:
        color_stroke(states, out);
        // width_stroke(states, out, 1 / states->scaling);
        width_stroke(states, out, 1);
        *stroked = true;
        break;
    case U_PS_GEOMETRIC:
        basic_stroke(states, out);
        *stroked = true;
        break;
    }
    // line style.
    switch (states->currentDeviceContext.stroke_mode & 0x000000FF) {
    case U_PS_SOLID:
        break;
    case U_PS_DASH:
        fprintf(out, "stroke-dasharray=\"%.4f,%.4f\" ", dash_len, dash_len);
        break;
    case U_PS_DOT:
        fprintf(out, "stroke-dasharray=\"%.4f,%.4f\" ", dot_len, dot_len);
        break;
    case U_PS_DASHDOT:
        fprintf(out, "stroke-dasharray=\"%.4f,%.4f,%.4f,%.4f\" ", dash_len,
                dash_len, dot_len, dash_len);
        break;
    case U_PS_DASHDOTDOT:
        fprintf(out, "stroke-dasharray=\"%.4f,%.4f,%.4f,%.4f,%.4f,%.4f\" ",
                dash_len, dash_len, dot_len, dot_len, dot_len, dash_len);
        break;
    case U_PS_INSIDEFRAME:
    case U_PS_USERSTYLE:
    case U_PS_ALTERNATE:
    default:
        // partial
        break;
    }
    // line cap.
    switch (states->currentDeviceContext.stroke_mode & 0x00000F00) {
    case U_PS_ENDCAP_ROUND:
        fprintf(out, " stroke-linecap=\"round\" ");
        break;
    case U_PS_ENDCAP_SQUARE:
        fprintf(out, " stroke-linecap=\"square\" ");
        break;
    case U_PS_ENDCAP_FLAT:
        fprintf(out, " stroke-linecap=\"butt\" ");
        break;
    default:
        break;
    }
    // line join.
    switch (states->currentDeviceContext.stroke_mode & 0x0000F000) {
    case U_PS_JOIN_ROUND:
        fprintf(out, " stroke-linejoin=\"round\" ");
        break;
    case U_PS_JOIN_BEVEL:
        fprintf(out, " stroke-linejoin=\"bevel\" ");
        break;
    case U_PS_JOIN_MITER:
        fprintf(out, " stroke-linejoin=\"miter\" ");
        if (states->currentDeviceContext.miterLimit)
            fprintf(out, " stroke-miterlimit=\"%.4f\" ",
                    states->scaling *
                        (double)states->currentDeviceContext.miterLimit);
        break;
    default:
        break;
    }
}

static void text_style(FILE *out, drawingStates *states, POINT_D Org,
                       const U_EMRTEXT *text, const char *record, uint8_t type, uint32_t dxOffset) {
    double font_height =
        fabs(scaleX(states, states->currentDeviceContext.font_height));
    if (states->currentDeviceContext.font_family != NULL)
        fprintf(out, "font-family=\"%s\" ",
                states->currentDeviceContext.font_family);
    fprintf(out, "fill=\"#%02X%02X%02X\" ",
            states->currentDeviceContext.text_red,
            states->currentDeviceContext.text_green,
            states->currentDeviceContext.text_blue);
    int orientation = 1;
    if (scaleY(states, 1.0) > 0) {
        orientation = -1;
    } else {
        orientation = 1;
    }

    if (states->currentDeviceContext.font_escapement != 0) {
        fprintf(out, "transform=\"rotate(%.4f, %.4f, %.4f)\" ",
                orientation * (states->currentDeviceContext.font_escapement / 10.0),
                Org.x, Org.y);
    }

    if (states->text_layout == U_LAYOUT_RTL) {
        fprintf(out, "writing-mode=\"rl-tb\" ");
    }

    if (states->currentDeviceContext.font_italic) {
        fprintf(out, "font-style=\"italic\" ");
    }

    fprintf(out, "style =\"white-space:pre;\" ");

    if (states->currentDeviceContext.font_underline &&
        states->currentDeviceContext.font_strikeout) {
        fprintf(out, "text-decoration=\"line-through,underline\" ");
    } else if (states->currentDeviceContext.font_underline) {
        fprintf(out, "text-decoration=\"underline\" ");
    } else if (states->currentDeviceContext.font_strikeout) {
        fprintf(out, "text-decoration=\"line-through\" ");
    }

    if (states->currentDeviceContext.font_weight != 0)
        fprintf(out, "font-weight=\"%d\" ",
                states->currentDeviceContext.font_weight);

    uint16_t align = states->currentDeviceContext.text_align;
    double baseline = (align & (U_TA_BOTTOM | U_TA_BASELINE)) ? 0 : font_height * 0.9;
    const char *anchor = "start";
    double factor = 0;
    if ((align & U_TA_CENTER) == U_TA_CENTER || (align & U_TA_CENTER2) == U_TA_CENTER2) {
        anchor = "middle";
        factor = 0.5;
    } else if ((align & U_TA_RIGHT) == U_TA_RIGHT) {
        anchor = "end";
        factor = 1;
    }

    if (text) {
        size_t stride = (text->fOptions & U_ETO_PDY) ? 2 : 1;
        double total[2] = {0, 0};
        for (uint32_t i = 0; i < text->nChars; ++i) {
            int32_t delta[2] = {0, 0};
            memcpy(delta, record + dxOffset + i * stride * 4, stride * 4);
            total[0] += delta[0];
            total[1] += delta[1];
        }
        fprintf(out, "text-anchor=\"start\" ");
        for (unsigned axis = 0; axis < 2; ++axis) {
            double cursor = -factor * total[axis];
            fprintf(out, "%c=\"", axis ? 'y' : 'x');
            for (uint32_t i = 0; i < text->nChars; ++i) {
                double coordinate = axis ? Org.y + baseline + scaleY(states, cursor)
                                         : Org.x + scaleX(states, cursor);
                fprintf(out, "%.4f ", coordinate);
                int32_t delta[2] = {0, 0};
                memcpy(delta, record + dxOffset + i * stride * 4, stride * 4);
                cursor += delta[axis];
                uint16_t codepoint = 0;
                if (type == UTF_16)
                    memcpy(&codepoint, record + text->offString + 2 * i, 2);
                if (codepoint >= 0xd800 && codepoint <= 0xdbff && i + 1 < text->nChars) {
                    ++i;
                    memcpy(delta, record + dxOffset + i * stride * 4, stride * 4);
                    cursor += delta[axis];
                }
            }
            fprintf(out, "\" ");
        }
    } else {
        fprintf(out, "text-anchor=\"%s\" x=\"%.4f\" y=\"%.4f\" ",
                anchor, Org.x, Org.y + baseline);
    }
    fprintf(out, "font-size=\"%.4f\" ", font_height);
}

void text_style_draw(FILE *out, drawingStates *states, POINT_D Org) {
    text_style(out, states, Org, NULL, NULL, 0, 0);
}

// get the closest ttf file matching font_family, weight, italic
static int get_fontpath(char *font_family, int weight, int italic,
                        FcConfig *config, char **path) {
    *path = NULL;
    FcPattern *pat = FcPatternCreate();
    if (!pat)
        return 1;

    FcPatternAddString(pat, FC_FAMILY, (const FcChar8 *)font_family);
    FcPatternAddInteger(pat, FC_WEIGHT, FcWeightFromOpenType(weight ? weight : 400));
    FcPatternAddInteger(pat, FC_SLANT, italic ? FC_SLANT_ITALIC : FC_SLANT_ROMAN);
    FcConfigSubstitute(config, pat, FcMatchPattern);
    FcDefaultSubstitute(pat);
    FcResult result;
    FcPattern *match = FcFontMatch(config, pat, &result);
    FcPatternDestroy(pat);
    if (!match)
        return 1;

    FcChar8 *file = NULL;
    if (FcPatternGetString(match, FC_FILE, 0, &file) == FcResultMatch)
        *path = strdup((const char *)file);

    FcPatternDestroy(match);
    return *path == NULL;
}

// generate the reverse cmap from the ttf file
static int cmap_rev(const char *fpath, cmap_collection *rcmap) {
    FT_Library library;
    if (FT_Init_FreeType(&library))
        return 1;

    FT_Face face;
    if (FT_New_Face(library, fpath, 0, &face)) {
        FT_Done_FreeType(library);
        return 1;
    }
    if (FT_Select_Charmap(face, FT_ENCODING_UNICODE)) {
        FT_Done_Face(face);
        FT_Done_FreeType(library);
        return 1;
    }

    rcmap->size = face->num_glyphs;
    rcmap->uni = calloc(rcmap->size, sizeof(uint32_t));
    if (rcmap->uni) {
        FT_UInt index;
        FT_ULong codepoint = FT_Get_First_Char(face, &index);
        while (index) {
            if (index < rcmap->size && !rcmap->uni[index])
                rcmap->uni[index] = codepoint;
            codepoint = FT_Get_Next_Char(face, codepoint, &index);
        }
    }
    FT_Done_Face(face);
    FT_Done_FreeType(library);
    return rcmap->uni == NULL;
}

// generate the reverse cmap of a given font_family
static int gen_reverse_map(char *font_family, int weight, bool italic,
                           FcConfig *config, cmap_collection *rcmap) {
    char *path = NULL;
    int ret = 0;
    ret = get_fontpath(font_family, weight, italic, config, &path);
    if (ret) {
        // printf("error while search font");
        free(path);
        return 1;
    }
    ret = cmap_rev(path, rcmap);
    if (ret) {
        // printf("error while generating reverse mapping");
        free(path);
        return 1;
    }
    free(path);
    return 0;
}

/*
 * EMF files can contain weird "encoding".
 * This file handles one of those:
 * if ETO_GLYPH_INDEX is set in *TEXTOUT options,
 * the encoding of a char is basically the index
 * of its corresponding glyph inside the font ttf file.
 *
 * That's great... Thanks a lot Microsoft for this crappy scheme.
 *
 * To handle this case, we define some reverse mapping tables
 * (index of glyph -> unicode).
 *
 * Recovering the ttf file is done using fontconfig (function: get_fontpath).
 * The reverse mapping is done with freetype (function: cmap_rev).
 *
 * The .ttf font must be present on your system and properly indexed by
 * fontconfig
 *
 */

static int fontindex_to_utf8(uint16_t *in, size_t size_in, char **out,
                             size_t *out_len, char *font_name, int weight,
                             bool italic, FcConfig *config) {
    int ret;
    *out_len = 0;
    cmap_collection rcmap;
    rcmap.uni = NULL;
    if (font_name == NULL) {
        *out = NULL;
        return 1;
    }
    ret = gen_reverse_map(font_name, weight, italic, config, &rcmap);

    if (rcmap.uni == NULL) {
        *out = NULL;
        return 1;
    }

    if (rcmap.uni && ret) {
        free(rcmap.uni);
        *out = NULL;
        return 1;
    }

    size_t buf_size_left = U_MAX(size_in, 5);
    char *buf = calloc(buf_size_left, sizeof(char));
    if (!buf) {
        free(rcmap.uni);
        *out = NULL;
        return -1;
    }

    for (int i = 0; i < size_in; i++) {
        uint16_t index;
        memcpy(&index, (const char *)in + 2 * i, sizeof(index));
        // Calibri uses glyph 3 for NBSP; metric substitutes have different glyph order.
        bool calibriSpace = index == 3 && strcmp(font_name, "Calibri") == 0;
        if (!calibriSpace && (index >= rcmap.size || rcmap.uni[index] == 0)) {
            free(buf);
            free(rcmap.uni);
            *out = NULL;
            return 1;
        }
        uint32_t codepoint = calibriSpace ? 0x00a0 : rcmap.uni[index];
        {
            if (codepoint <= 0x7f) {
                buf[*out_len] = (codepoint & 0x7f);
                (*out_len)++;
                buf_size_left--;
            } else if (codepoint <= 0x7ff) {
                buf[*out_len] = (0xc0 | (codepoint >> 6));
                (*out_len)++;
                buf_size_left--;
                buf[*out_len] = (0x80 | (codepoint & 0x3f));
                (*out_len)++;
                buf_size_left--;
            } else if (codepoint <= 0xffff) {
                buf[*out_len] = (0xe0 | (codepoint >> 12));
                (*out_len)++;
                buf_size_left--;
                buf[*out_len] = (0x80 | ((codepoint >> 6) & 0x3f));
                (*out_len)++;
                buf_size_left--;
                buf[*out_len] = (0x80 | (codepoint & 0x3f));
                (*out_len)++;
                buf_size_left--;
            } else if (codepoint <= 0x1fffff) {
                buf[*out_len] = (0xf0 | (codepoint >> 18));
                (*out_len)++;
                buf_size_left--;
                buf[*out_len] = (0x80 | ((codepoint >> 12) & 0x3f));
                (*out_len)++;
                buf_size_left--;
                buf[*out_len] = (0x80 | ((codepoint >> 6) & 0x3f));
                (*out_len)++;
                buf_size_left--;
                buf[*out_len] = (0x80 | (codepoint & 0x3f));
                (*out_len)++;
                buf_size_left--;
            }
        }
        if (buf_size_left <= 5) {
            char *ptr;
            size_t increase = 50;
            ptr = realloc(buf, *out_len + increase + buf_size_left);
            if (!ptr) {
                free(buf);
                free(rcmap.uni);
                *out = NULL;
                return -1;
            }
            buf_size_left += increase;
            buf = ptr;
        }
    }
    free(rcmap.uni);
    buf[*out_len] = '\0';
    *out = buf;
    return 0;
}

static int enc_to_utf8(char *in, size_t size_in, char **out, size_t *out_len,
                       char *from_enc) {
    iconv_t cd;
    char *inbuf, *outbuf;
    size_t inbytesleft, outbytesleft, nchars, out_buf_len;

    cd = iconv_open("UTF-8", from_enc);
    if (cd == (iconv_t)-1) {
        *out = NULL;
        return 1;
    }

    inbytesleft = size_in;
    if (inbytesleft == 0) {
        iconv_close(cd);
        *out = NULL;
        return 1;
    }
    inbuf = in;
    out_buf_len = inbytesleft;
    *out = calloc(out_buf_len, 1);
    if (!*out) {
        iconv_close(cd);
        *out = NULL;
        return 1;
    }
    outbytesleft = out_buf_len;
    outbuf = *out;

    nchars = iconv(cd, &inbuf, &inbytesleft, &outbuf, &outbytesleft);
    while (nchars == (size_t)-1 && errno == E2BIG) {
        char *ptr;
        size_t increase = 50;
        size_t len;
        out_buf_len += increase;
        outbytesleft += increase;
        ptr = realloc(*out, out_buf_len);
        if (!ptr) {
            free(*out);
            iconv_close(cd);
            *out = NULL;
            return 1;
        }
        len = outbuf - *out;
        *out = ptr;
        outbuf = *out + len;
        nchars = iconv(cd, &inbuf, &inbytesleft, &outbuf, &outbytesleft);
    }
    if (outbytesleft == 0) {
        char *ptr;
        size_t increase = 50;
        out_buf_len += increase;
        outbytesleft += increase;
        ptr = realloc(*out, out_buf_len);
        if (!ptr) {
            free(*out);
            iconv_close(cd);
            *out = NULL;
            return 1;
        }
        *out = ptr;
    }
    if (nchars == (size_t)-1) {
        free(*out);
        iconv_close(cd);
        *out = NULL;
        return 1;
    }

    iconv_close(cd);
    *out_len = out_buf_len - outbytesleft;
    (*out)[(*out_len)] = '\0';
    return 0;
}

void reverse_utf8(char *in, size_t size_in) {
    /* this assumes that str is valid UTF-8 */
    char *scanl, *scanr, *scanr2, c;

    /* first reverse the string */
    for ((scanl = in, scanr = in + size_in); scanl < scanr;) {
        c = *scanl;
        *scanl++ = *--scanr;
        *scanr = c;
    }

    /* then scan all bytes and reverse each multibyte character */
    scanl = scanr = in;
    for (; (c = *scanr++);) {
        if ((c & 0x80) == 0) // ASCII char
            scanl = scanr;
        else if ((c & 0xc0) == 0xc0) { // start of multibyte
            scanr2 = scanr;
            switch (scanr - scanl) {
            case 4:
                c = *scanl, *scanl++ = *--scanr, *scanr = c; // fallthrough
            case 3:                                          // fallthrough
            case 2:
                c = *scanl, *scanl++ = *--scanr, *scanr = c;
            }
            scanr = scanl = scanr2;
        }
    }
}

void text_convert(char *in, size_t size_in, char **out, size_t *size_out,
                  uint8_t type, drawingStates *states) {
    uint8_t *string;
    int ret = 0;

    switch (type) {
    case UTF_16:
        returnOutOfEmf((intptr_t)in + 2 * (intptr_t)size_in);
        ret = enc_to_utf8(in, 2 * size_in, (char **)&string, size_out,
                          "UTF-16LE");

        break;
    case FONTINDEX:
        returnOutOfEmf((intptr_t)in + 2 * (intptr_t)size_in);
        ret = fontindex_to_utf8((uint16_t *)in, size_in, (char **)&string,
                                size_out,
                                states->currentDeviceContext.font_family,
                                states->currentDeviceContext.font_weight,
                                states->currentDeviceContext.font_italic,
                                states->fontConfig);
        if (ret==0 && string!=NULL) {
            switch (states->currentDeviceContext.font_charset) {
            case U_HEBREW_CHARSET:
            case U_ARABIC_CHARSET:
                /* with Utf-8 strings, the strings must always be
                 * stored in logical order, not visual order.
                 * Unicode Bidirectional (bidi) Algorithm does the work
                 * of rendering the text properly.
                 * So we reverse the text to be in logical order.
                 * However, this seems imcomplete,
                 * Right to Left ordering can also be set in
                 * ExtTextOutOptions (EMR_*TEXTOUT* records) and EMR_SETLAYOUT
                 * records, and it's completely ignored here.
                 * FIXME this is probably to simplistic.
                 */
                reverse_utf8((char*)string, *size_out);
                break;
            case U_ANSI_CHARSET:
            case U_DEFAULT_CHARSET:
            case U_SYMBOL_CHARSET:
            case U_SHIFTJIS_CHARSET:
            case U_HANGUL_CHARSET:
            case U_GB2312_CHARSET:
            case U_CHINESEBIG5_CHARSET:
            case U_GREEK_CHARSET:
            case U_TURKISH_CHARSET:
            case U_BALTIC_CHARSET:
            case U_RUSSIAN_CHARSET:
            case U_EASTEUROPE_CHARSET:
            case U_THAI_CHARSET:
            case U_JOHAB_CHARSET:
            case U_MAC_CHARSET:
            case U_OEM_CHARSET:
            case U_VISCII_CHARSET:
            case U_TCVN_CHARSET:
            case U_KOI8_CHARSET:
            case U_ISO3_CHARSET:
            case U_ISO4_CHARSET:
            case U_ISO10_CHARSET:
            case U_CELTIC_CHARSET:
            default:
                break;
            }
        }
        break;
    default:
        if (checkOutOfEMF(states,
                          (uintptr_t)((uintptr_t)in + (uintptr_t)size_in))) {
            string = NULL;
        }
        else {
            string = (uint8_t *)calloc((size_in + 1), 1);
            strncpy((char *)string, in, size_in);
            *size_out = size_in;
        }
        break;
    }

    if (ret != 0) {
        states->Error = true;
        string = NULL;
    }

    if (string == NULL) {
            return;
    }

    int i = 0;
    while (i < (*size_out) && string[i] != 0x0) {
        // Clean-up not printable ascii char like bells \r etc...
        if (string[i] < 0x20 && string[i] != 0x09 && string[i] != 0x0A &&
            string[i] != 0x0B && string[i] != 0x09) {
            string[i] = 0x20;
        }
        // If it's specified as ascii, it must be ascii,
        // so, replace any char > 127 with 0x20 (space)
        if (type == ASCII && string[i] > 0x7F) {
            string[i] = 0x20;
        }
        i++;
    }
    *out = (char *)string;
}

void text_draw(const char *contents, FILE *out, drawingStates *states,
               uint8_t type) {
    const U_EMR *record = (const U_EMR *)contents;
    if (record->nSize < sizeof(U_EMREXTTEXTOUTW)) {
        states->Error = true;
        return;
    }
    const U_EMRTEXT *pemt =
        (const U_EMRTEXT *)(contents + sizeof(U_EMREXTTEXTOUTA) - sizeof(U_EMRTEXT));
    size_t dxField = sizeof(U_EMREXTTEXTOUTW) +
                     ((pemt->fOptions & U_ETO_NO_RECT) ? 0 : sizeof(U_RECTL));
    if (dxField > record->nSize || sizeof(uint32_t) > record->nSize - dxField) {
        states->Error = true;
        return;
    }
    uint32_t dxOffset;
    memcpy(&dxOffset, contents + dxField, sizeof(dxOffset));
    if (pemt->fOptions & U_ETO_GLYPH_INDEX)
        type = FONTINDEX;

    size_t charSize = type == ASCII ? 1 : 2;
    size_t stride = (pemt->fOptions & U_ETO_PDY) ? 8 : 4;
    if (pemt->offString > record->nSize ||
        pemt->nChars > (record->nSize - pemt->offString) / charSize ||
        (dxOffset && (dxOffset > record->nSize ||
                        pemt->nChars > (record->nSize - dxOffset) / stride))) {
        states->Error = true;
        return;
    }

    if (pemt->nChars == 0)
        return;

    char *string = NULL;
    size_t string_size = 0;
    text_convert((char *)(contents + pemt->offString), pemt->nChars, &string,
                 &string_size, type, states);
    if (states->Error) {
        free(string);
        return;
    }

    fprintf(out, "<%stext ", states->nameSpaceString);
    clipset_draw(states, out);
    POINT_D Org = point_cal(states, (double)pemt->ptlReference.x,
                            (double)pemt->ptlReference.y);
    text_style(out, states, Org, dxOffset ? pemt : NULL, contents, type, dxOffset);
    fprintf(out, ">");
    if (string) {
        fprintf(out, "<![CDATA[%s]]>", string);
        free(string);
    }
    fprintf(out, "</%stext>\n", states->nameSpaceString);
}
void transform_draw(drawingStates *states, FILE *out) {
    // transformation could be set inside path.
    // If we are in a path, we do nothing here.
    // However the transformation is set in BEGINPATH or ENDPATH.
    // The "pre" parsing is used to determine if such cases can occure
    // and records transformations that doesn't occure where the record is
    // declared.
    // (function U_emf_onerec_analyse)
    if (states->inPath)
        return;
    if (states->transform_open) {
        fprintf(out, "</%sg>\n", states->nameSpaceString);
    }
    fprintf(
        out, "<%sg transform=\"matrix(%.4f %.4f %.4f %.4f %.4f %.4f)\">\n",
        states->nameSpaceString,
        (double)states->currentDeviceContext.worldTransform.eM11,
        (double)states->currentDeviceContext.worldTransform.eM12,
        (double)states->currentDeviceContext.worldTransform.eM21,
        (double)states->currentDeviceContext.worldTransform.eM22,
        (double)scaleX(states, states->currentDeviceContext.worldTransform.eDx),
        (double)scaleY(states,
                       states->currentDeviceContext.worldTransform.eDy));
    states->transform_open = true;
}
bool transform_set(drawingStates *states, U_XFORM xform, uint32_t iMode) {
    switch (iMode) {
    case U_MWT_IDENTITY: {
        setTransformIdentity(states);
        return true;
    }
    case U_MWT_LEFTMULTIPLY: {
        float a11 = xform.eM11;
        float a12 = xform.eM12;
        float a13 = 0.0;
        float a21 = xform.eM21;
        float a22 = xform.eM22;
        float a23 = 0.0;
        float a31 = xform.eDx;
        float a32 = xform.eDy;
        float a33 = 1.0;

        float b11 = states->currentDeviceContext.worldTransform.eM11;
        float b12 = states->currentDeviceContext.worldTransform.eM12;
        // float b13 = 0.0;
        float b21 = states->currentDeviceContext.worldTransform.eM21;
        float b22 = states->currentDeviceContext.worldTransform.eM22;
        // float b23 = 0.0;
        float b31 = states->currentDeviceContext.worldTransform.eDx;
        float b32 = states->currentDeviceContext.worldTransform.eDy;
        // float b33 = 1.0;

        float c11 = a11 * b11 + a12 * b21 + a13 * b31;
        float c12 = a11 * b12 + a12 * b22 + a13 * b32;
        // float c13 = a11*b13 + a12*b23 + a13*b33;;
        float c21 = a21 * b11 + a22 * b21 + a23 * b31;
        float c22 = a21 * b12 + a22 * b22 + a23 * b32;
        // float c23 = a21*b13 + a22*b23 + a23*b33;;
        float c31 = a31 * b11 + a32 * b21 + a33 * b31;
        float c32 = a31 * b12 + a32 * b22 + a33 * b32;
        // float c33 = a31*b13 + a32*b23 + a33*b33;;

        states->currentDeviceContext.worldTransform.eM11 = c11;
        states->currentDeviceContext.worldTransform.eM12 = c12;
        states->currentDeviceContext.worldTransform.eM21 = c21;
        states->currentDeviceContext.worldTransform.eM22 = c22;
        states->currentDeviceContext.worldTransform.eDx = c31;
        states->currentDeviceContext.worldTransform.eDy = c32;

        return true;
    }
    case U_MWT_RIGHTMULTIPLY: {
        float a11 = states->currentDeviceContext.worldTransform.eM11;
        float a12 = states->currentDeviceContext.worldTransform.eM12;
        float a13 = 0.0;
        float a21 = states->currentDeviceContext.worldTransform.eM21;
        float a22 = states->currentDeviceContext.worldTransform.eM22;
        float a23 = 0.0;
        float a31 = states->currentDeviceContext.worldTransform.eDx;
        float a32 = states->currentDeviceContext.worldTransform.eDy;
        float a33 = 1.0;

        float b11 = xform.eM11;
        float b12 = xform.eM12;
        // float b13 = 0.0;
        float b21 = xform.eM21;
        float b22 = xform.eM22;
        // float b23 = 0.0;
        float b31 = xform.eDx;
        float b32 = xform.eDy;
        // float b33 = 1.0;

        float c11 = a11 * b11 + a12 * b21 + a13 * b31;
        float c12 = a11 * b12 + a12 * b22 + a13 * b32;
        // float c13 = a11*b13 + a12*b23 + a13*b33;;
        float c21 = a21 * b11 + a22 * b21 + a23 * b31;
        float c22 = a21 * b12 + a22 * b22 + a23 * b32;
        // float c23 = a21*b13 + a22*b23 + a23*b33;;
        float c31 = a31 * b11 + a32 * b21 + a33 * b31;
        float c32 = a31 * b12 + a32 * b22 + a33 * b32;
        // float c33 = a31*b13 + a32*b23 + a33*b33;;

        states->currentDeviceContext.worldTransform.eM11 = c11;
        states->currentDeviceContext.worldTransform.eM12 = c12;
        states->currentDeviceContext.worldTransform.eM21 = c21;
        states->currentDeviceContext.worldTransform.eM22 = c22;
        states->currentDeviceContext.worldTransform.eDx = c31;
        states->currentDeviceContext.worldTransform.eDy = c32;

        return true;
    }
    case U_MWT_SET: {
        states->currentDeviceContext.worldTransform = xform;

        return true;
    }
    default:
        return false;
    }
}
void width_stroke(drawingStates *states, FILE *out, double width) {
    double tmp_w = scaleX(states, width);
    // minimum size of a line seems to be 1px, even if smaller after resize
    // keeping this behavior
    if ((tmp_w / states->scaling) < 1.0) {
        fprintf(out, "stroke-width=\"1px\" ");
    } else {
        fprintf(out, "stroke-width=\"%.4f\" ", tmp_w);
    }
}

static char encoding_table[] = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
    'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
    'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'};

static int mod_table[] = {0, 2, 1};

char *base64_encode(const unsigned char *data, size_t input_length,
                    size_t *output_length) {
    *output_length = 4 * ((input_length + 2) / 3) + 3;

    char *encoded_data = calloc(*output_length, 1);
    if (encoded_data == NULL)
        return NULL;

    for (int i = 0, j = 0; i < input_length;) {
        uint32_t octet_a = i < input_length ? (unsigned char)data[i++] : 0;
        uint32_t octet_b = i < input_length ? (unsigned char)data[i++] : 0;
        uint32_t octet_c = i < input_length ? (unsigned char)data[i++] : 0;

        uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

        encoded_data[j++] = encoding_table[(triple >> 3 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 2 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 1 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 0 * 6) & 0x3F];
    }

    for (int i = 0; i < mod_table[input_length % 3]; i++)
        encoded_data[*output_length - 1 - i] = '=';

    return encoded_data;
}

void pointCurrPathAdd16(drawingStates *states, U_POINT16 pt, int index) {
    if (states->inPath) {
        states->currentPath->last->section.points[index] =
            point_s16(states, pt);
    }
}

void pointCurrPathAdd(drawingStates *states, U_POINT pt, int index) {
    if (states->inPath) {
        states->currentPath->last->section.points[index] = point_s(states, pt);
    }
}

void pointCurrPathAddD(drawingStates *states, POINT_D pt, int index) {
    if (states->inPath) {
        states->currentPath->last->section.points[index] = pt;
    }
}

void addNewSegPath(drawingStates *states, uint8_t type) {
    if (states->inPath) {
        PATH **path = &(states->currentPath);
        add_new_seg(path, type);
    }
}

void free_path(PATH **path) {
    if ((*path) == NULL) {
        return;
    }
    PATH *tmp1 = (*path);
    PATH *tmp2 = (*path);
    while (tmp1 != NULL) {
        tmp1 = tmp1->next;
        free(tmp2->section.points);
        free(tmp2);
        tmp2 = tmp1;
    }
    (*path) = NULL;
}

void draw_path(PATH *in, FILE *out) {
    PATH *tmp = in;
    while (tmp != NULL) {
        uint8_t type = tmp->section.type;
        POINT_D *pt = tmp->section.points;
        switch (type) {
        case SEG_END:
            fprintf(out, "Z ");
            break;
        case SEG_MOVE:
            fprintf(out, "M ");
            point_draw_raw_d(pt[0], out);
            break;
        case SEG_LINE:
            fprintf(out, "L ");
            point_draw_raw_d(pt[0], out);
            break;
        case SEG_ARC:
            fprintf(out, "A ");
            point_draw_raw_d(pt[0], out);
            point_draw_raw_d(pt[1], out);
            break;
        case SEG_BEZIER:
            fprintf(out, "C ");
            point_draw_raw_d(pt[0], out);
            point_draw_raw_d(pt[1], out);
            point_draw_raw_d(pt[2], out);
            break;
        }
        tmp = tmp->next;
    }
}

void copy_path(PATH *in, PATH **out) {
    PATH *tmp = in;
    PATH *out_current = NULL;
    while (tmp != NULL) {
        uint8_t type = tmp->section.type;
        POINT_D *pt = tmp->section.points;
        add_new_seg(&out_current, type);
        switch (type) {
        case SEG_END:
            break;
        case SEG_MOVE:
            out_current->last->section.points[0] = pt[0];
            break;
        case SEG_LINE:
            out_current->last->section.points[0] = pt[0];
            break;
        case SEG_ARC:
            out_current->last->section.points[0] = pt[0];
            out_current->last->section.points[1] = pt[1];
            break;
        case SEG_BEZIER:
            out_current->last->section.points[0] = pt[0];
            out_current->last->section.points[1] = pt[1];
            out_current->last->section.points[2] = pt[2];
            break;
        }
        tmp = tmp->next;
    }
    (*out) = out_current;
}

void offset_path(PATH *in, POINT_D pt) {
    PATH *tmp = in;
    while (tmp != NULL) {
        uint8_t type = tmp->section.type;
        switch (type) {
        case SEG_END:
            break;
        case SEG_MOVE:
            tmp->section.points[0].x += pt.x;
            tmp->section.points[0].y += pt.y;
            break;
        case SEG_LINE:
            tmp->section.points[0].x += pt.x;
            tmp->section.points[0].y += pt.y;
            break;
        case SEG_ARC:
            tmp->section.points[1].x += pt.x;
            tmp->section.points[1].y += pt.y;
            break;
        case SEG_BEZIER:
            tmp->section.points[2].x += pt.x;
            tmp->section.points[2].y += pt.y;
            break;
        }
        tmp = tmp->next;
    }
}

void add_new_seg(PATH **path, uint8_t type) {
    PATH *new_path = calloc(1, sizeof(PATH));
    POINT_D *new_seg;
    switch (type) {
    case SEG_END:
        new_seg = NULL;
        break;
    case SEG_MOVE:
        new_seg = calloc(1, sizeof(POINT_D));
        break;
    case SEG_LINE:
        new_seg = calloc(1, sizeof(POINT_D));
        break;
    case SEG_ARC:
        new_seg = calloc(2, sizeof(POINT_D));
        break;
    case SEG_BEZIER:
        new_seg = calloc(3, sizeof(POINT_D));
        break;
    default:
        new_seg = NULL;
        break;
    }
    new_path->section.points = new_seg;
    new_path->section.type = type;
    if (*path == NULL || (*path)->last == NULL) {
        *path = new_path;
        new_path->last = new_path;
    } else {
        (*path)->last->next = new_path;
        (*path)->last = new_path;
    }
}

void clipset_draw(drawingStates *states, FILE *out) {
    int clipID = states->currentDeviceContext.clipID;
    if (clipID)
        fprintf(out, " clip-path=\"url(#clip-%d)\" ", clipID);
}

#ifdef __cplusplus
}
#endif
/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
