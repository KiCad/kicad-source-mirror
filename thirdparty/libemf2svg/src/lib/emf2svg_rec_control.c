#ifdef __cplusplus
extern "C" {
#endif

#ifndef DARWIN
#define _POSIX_C_SOURCE 200809L
#endif
#include "emf2svg_private.h"
#include "emf2svg_print.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void U_EMREOF_draw(const char *contents, FILE *out, drawingStates *states) {
    FLAG_PARTIAL;
    if (states->verbose) {
        U_EMREOF_print(contents, states);
    }
    if (states->transform_open) {
        fprintf(out, "</%sg>\n", states->nameSpaceString);
    }
    fprintf(out, "</%sg>\n", states->nameSpaceString);
    if (states->svgDelimiter)
        fprintf(out, "</%ssvg>\n", states->nameSpaceString);
}
void U_EMRHEADER_draw(const char *contents, FILE *out, drawingStates *states) {
    FLAG_PARTIAL;
    if (states->verbose) {
        U_EMRHEADER_print(contents, states);
    }
    char *string;
    int p1len;

    PU_EMRHEADER pEmr = (PU_EMRHEADER)(contents);
    if (pEmr->offDescription) {
        returnOutOfEmf((uint16_t *)((char *)(intptr_t)pEmr +
                                    (intptr_t)pEmr->offDescription) +
                       2 * (intptr_t)pEmr->nDescription);
        string =
            U_Utf16leToUtf8((uint16_t *)((char *)pEmr + pEmr->offDescription),
                            pEmr->nDescription, NULL);
        free(string);
        p1len =
            2 +
            2 * wchar16len((uint16_t *)((char *)pEmr + pEmr->offDescription));
        returnOutOfEmf((uint16_t *)((char *)(intptr_t)pEmr +
                                    (intptr_t)pEmr->offDescription +
                                    (intptr_t)p1len) +
                       2 * (intptr_t)pEmr->nDescription);
        string = U_Utf16leToUtf8(
            (uint16_t *)((char *)pEmr + pEmr->offDescription + p1len),
            pEmr->nDescription, NULL);
        free(string);
    }
    // object table allocation
    // allocate one more to directly use object indexes (starts at 1 and not 0)
    states->objectTable = calloc(pEmr->nHandles + 1, sizeof(emfGraphObject));
    states->objectTableSize = pEmr->nHandles;

    double ratioXY = (double)(pEmr->rclBounds.right - pEmr->rclBounds.left) /
                     (double)(pEmr->rclBounds.bottom - pEmr->rclBounds.top);

    /**
    In EMF coordinates are specified using an origin (`[0,0]` point) located at
    the upper-left corner: x-coordinates increase to the right; y-coordinates
    increase from top to bottom.

    The SVG coordinate system, on the other hand, uses the same origin (`[0,0]`
    point) at the bottom-left corner: x-coordinates increase to the right; but
    y-coordinates increase from top to bottom.

    Typically, a simple shift of the y-axis through a single SVG/CSS
    transformation is used to transform from EMF coordinates to SVG coordinates.

    However, under certain circumstances some tools (for instance, SparxSystem
    Enterprise Architect in Wine) will generate EMF files with malformed
    coordinates. These images have an origin at the top-left corner with
    y-coordinates increasing from top to bottom, yet these y-coordinates are
    inverted (multiplied by `-1`) to simulate a normal EMF look.

    Furthermore, this inversion phenomenon cannot be solved with plain mirroring
    as it occurs to all (complex) objects of the hierarchy. For example, text
    boxes have only their y-coordinate anchor point mirrored, but the text
    direction is set properly.

    This specific layout issue cannot be fixed by a single SVG/CSS
    transformation, and therefore the processing code is required to detect and
    invert only the affected y-coordinates, while keeping other attributes
    intact.

    Condition: top and bottom points are at different sides of the X axis. We
    assume this condition indicates that this image was generated with  broken
    transformation (possibly on Wine) and a fix is required as described above.

    While this may be a weak assumption, nothing better came to mind.
    **/

    if ((pEmr->rclBounds.top < 0 && pEmr->rclBounds.bottom > 0) ||
        (pEmr->rclBounds.top > 0 && pEmr->rclBounds.bottom < 0)) {
        states->fixBrokenYTransform = true;
    }

    if ((states->imgHeight != 0) && (states->imgWidth != 0)) {
        double tmpWidth = states->imgHeight * ratioXY;
        double tmpHeight = states->imgWidth / ratioXY;
        if (tmpWidth > states->imgWidth) {
            states->imgHeight = tmpHeight;
        } else {
            states->imgWidth = tmpWidth;
        }
    } else if (states->imgHeight != 0) {
        states->imgWidth = states->imgHeight * ratioXY;
    } else if (states->imgWidth != 0) {
        states->imgHeight = states->imgWidth / ratioXY;
    } else {
        states->imgWidth =
            (double)abs(pEmr->rclBounds.right - pEmr->rclBounds.left);
        states->imgHeight =
            (double)abs(pEmr->rclBounds.bottom - pEmr->rclBounds.top);
    }

    // set scaling for original resolution
    // states->scaling = 1;
    states->scaling = states->imgWidth /
                      (double)abs(pEmr->rclBounds.right - pEmr->rclBounds.left);


    // remember reference point of the output DC
    states->RefX = (double)pEmr->rclBounds.left;
    states->RefY = (double)pEmr->rclBounds.top;

    states->pxPerMm =
        (double)pEmr->szlDevice.cx / (double)pEmr->szlMillimeters.cx;

    if (states->svgDelimiter) {
        fprintf(
            out,
            "<?xml version=\"1.0\"  encoding=\"UTF-8\" standalone=\"no\"?>\n");
        fprintf(out, "<%ssvg version=\"1.1\" ", states->nameSpaceString);
        fprintf(out, "xmlns=\"http://www.w3.org/2000/svg\" ");
        fprintf(out, "xmlns:xlink=\"http://www.w3.org/1999/xlink\"");
        if ((states->nameSpace != NULL) && (strlen(states->nameSpace) != 0)) {
            fprintf(out, "xmlns:%s=\"http://www.w3.org/2000/svg\"",
                    states->nameSpace);
        }
    // https://www.w3.org/TR/SVG2/coords.html
        if (states->fixBrokenYTransform) {
            fprintf(out, " width=\"%.4f\" height=\"%.4f\">\n",
                states->imgWidth + 1,
                states->imgHeight + 1);
            fprintf(out, "<%sg transform=\"translate(0.0000, 0.00 00)\">\n",
                    states->nameSpaceString);
        } else {
            fprintf(out, " width=\"%.4f\" height=\"%.4f\">\n", states->imgWidth,
                states->imgHeight);
            fprintf(out, "<%sg transform=\"translate(%.4f, %.4f)\">\n",
                states->nameSpaceString, -1.0 * states->RefX * states->scaling,
                -1.0 * states->RefY * states->scaling);
        }
    }
}

#ifdef __cplusplus
}
#endif
/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
