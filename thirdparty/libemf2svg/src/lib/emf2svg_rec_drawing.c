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

void U_EMRANGLEARC_draw(const char *contents, FILE *out,
                        drawingStates *states) {
    FLAG_SUPPORTED;
    if (states->verbose) {
        U_EMRANGLEARC_print(contents, states);
    }
    arc_circle_draw(contents, out, states);
}
void U_EMRARC_draw(const char *contents, FILE *out, drawingStates *states) {
    FLAG_PARTIAL;
    if (states->verbose) {
        U_EMRARC_print(contents, states);
    }
    arc_draw(contents, out, states, ARC_SIMPLE);
}
void U_EMRARCTO_draw(const char *contents, FILE *out, drawingStates *states) {
    FLAG_PARTIAL;
    if (states->verbose) {
        U_EMRARCTO_print(contents, states);
    }
    arc_draw(contents, out, states, ARC_SIMPLE);
}
void U_EMRCHORD_draw(const char *contents, FILE *out, drawingStates *states) {
    FLAG_PARTIAL;
    if (states->verbose) {
        U_EMRCHORD_print(contents, states);
    }
    arc_draw(contents, out, states, ARC_CHORD);
}
void U_EMRCLOSEFIGURE_draw(const char *contents, FILE *out,
                           drawingStates *states) {
    FLAG_SUPPORTED;
    if (states->verbose) {
        U_EMRCLOSEFIGURE_print(contents, states);
    }
    fprintf(out, "Z ");
    UNUSED(contents);
}
void U_EMRELLIPSE_draw(const char *contents, FILE *out, drawingStates *states) {
    FLAG_SUPPORTED;
    if (states->verbose) {
        U_EMRELLIPSE_print(contents, states);
    }
    PU_EMRELLIPSE pEmr = (PU_EMRELLIPSE)(contents);
    POINT_D LT =
        point_cal(states, (double)pEmr->rclBox.left, (double)pEmr->rclBox.top);
    POINT_D RB = point_cal(states, (double)pEmr->rclBox.right,
                           (double)pEmr->rclBox.bottom);
    POINT_D center;
    POINT_D radius;
    center.x = (LT.x + RB.x) / 2;
    center.y = (LT.y + RB.y) / 2;
    radius.x = (RB.x - LT.x) / 2;
    radius.y = (RB.y - LT.y) / 2;
    fprintf(out, "<%sellipse cx=\"%.4f\" cy=\"%.4f\" rx=\"%.4f\" ry=\"%.4f\" ",
            states->nameSpaceString, center.x, center.y, radius.x, radius.y);
    bool filled = false;
    bool stroked = false;
    fill_draw(states, out, &filled, &stroked);
    stroke_draw(states, out, &filled, &stroked);
    clipset_draw(states, out);
    if (!filled)
        fprintf(out, "fill=\"none\" ");
    if (!stroked)
        fprintf(out, "stroke=\"none\" ");
    fprintf(out, "/>\n");
}
void U_EMREXTFLOODFILL_draw(const char *contents, FILE *out,
                            drawingStates *states) {
    FLAG_IGNORED;
    if (states->verbose) {
        U_EMREXTFLOODFILL_print(contents, states);
    }
    // PU_EMREXTFLOODFILL pEmr = (PU_EMREXTFLOODFILL)(contents);
}
void U_EMREXTTEXTOUTA_draw(const char *contents, FILE *out,
                           drawingStates *states) {
    FLAG_PARTIAL;
    if (states->verbose) {
        U_EMREXTTEXTOUTA_print(contents, states);
    }
    text_draw(contents, out, states, ASCII);
}
void U_EMREXTTEXTOUTW_draw(const char *contents, FILE *out,
                           drawingStates *states) {
    FLAG_PARTIAL;
    if (states->verbose) {
        U_EMREXTTEXTOUTW_print(contents, states);
    }
    text_draw(contents, out, states, UTF_16);
}
void U_EMRFILLPATH_draw(const char *contents, FILE *out,
                        drawingStates *states) {
    FLAG_PARTIAL;
    if (states->verbose) {
        U_EMRFILLPATH_print(contents, states);
    }
    // real work done in U_EMRENDPATH
}
void U_EMRFILLRGN_draw(const char *contents, FILE *out, drawingStates *states) {
    FLAG_IGNORED;
    if (states->verbose) {
        U_EMRFILLRGN_print(contents, states);
    }
    // PU_EMRFILLRGN pEmr = (PU_EMRFILLRGN)(contents);
}
void U_EMRFRAMERGN_draw(const char *contents, FILE *out,
                        drawingStates *states) {
    FLAG_IGNORED;
    if (states->verbose) {
        U_EMRFRAMERGN_print(contents, states);
    }
    // PU_EMRFRAMERGN pEmr = (PU_EMRFRAMERGN)(contents);
}
void U_EMRGRADIENTFILL_draw(const char *contents, FILE *out,
                            drawingStates *states) {
    FLAG_IGNORED;
    if (states->verbose) {
        U_EMRGRADIENTFILL_print(contents, states);
    }
    // PU_EMRGRADIENTFILL pEmr = (PU_EMRGRADIENTFILL)(contents);
}
void U_EMRLINETO_draw(const char *contents, FILE *out, drawingStates *states) {
    FLAG_SUPPORTED;
    if (states->verbose) {
        U_EMRLINETO_print(contents, states);
    }
    lineto_draw("U_EMRLINETO", "ptl:", "", contents, out, states);
}
void U_EMRPAINTRGN_draw(const char *contents, FILE *out,
                        drawingStates *states) {
    FLAG_IGNORED;
    if (states->verbose) {
        U_EMRPAINTRGN_print(contents, states);
    }
}
void U_EMRPIE_draw(const char *contents, FILE *out, drawingStates *states) {
    FLAG_PARTIAL;
    if (states->verbose) {
        U_EMRPIE_print(contents, states);
    }
    arc_draw(contents, out, states, ARC_PIE);
}
void U_EMRPOLYBEZIER_draw(const char *contents, FILE *out,
                          drawingStates *states) {
    FLAG_SUPPORTED;
    if (states->verbose) {
        U_EMRPOLYBEZIER_print(contents, states);
    }
    cubic_bezier_draw("U_EMRPOLYBEZIER", contents, out, states, 1);
}
void U_EMRPOLYBEZIER16_draw(const char *contents, FILE *out,
                            drawingStates *states) {
    FLAG_SUPPORTED;
    if (states->verbose) {
        U_EMRPOLYBEZIER16_print(contents, states);
    }
    cubic_bezier16_draw("U_EMRPOLYBEZIER16", contents, out, states, 1);
}
void U_EMRPOLYBEZIERTO_draw(const char *contents, FILE *out,
                            drawingStates *states) {
    FLAG_SUPPORTED;
    if (states->verbose) {
        U_EMRPOLYBEZIERTO_print(contents, states);
    }
    cubic_bezier_draw("U_EMRPOLYBEZIER", contents, out, states, 0);
}
void U_EMRPOLYBEZIERTO16_draw(const char *contents, FILE *out,
                              drawingStates *states) {
    FLAG_SUPPORTED;
    if (states->verbose) {
        U_EMRPOLYBEZIERTO16_print(contents, states);
    }
    cubic_bezier16_draw("U_EMRPOLYBEZIERTO16", contents, out, states, 0);
}
void U_EMRPOLYDRAW_draw(const char *contents, FILE *out,
                        drawingStates *states) {
    FLAG_IGNORED;
    if (states->verbose) {
        U_EMRPOLYDRAW_print(contents, states);
    }
    // PU_EMRPOLYDRAW pEmr = (PU_EMRPOLYDRAW)(contents);
}
void U_EMRPOLYDRAW16_draw(const char *contents, FILE *out,
                          drawingStates *states) {
    FLAG_IGNORED;
    if (states->verbose) {
        U_EMRPOLYDRAW16_print(contents, states);
    }
    // PU_EMRPOLYDRAW16 pEmr = (PU_EMRPOLYDRAW16)(contents);
}
void U_EMRPOLYGON_draw(const char *contents, FILE *out, drawingStates *states) {
    FLAG_SUPPORTED;
    if (states->verbose) {
        U_EMRPOLYGON_print(contents, states);
    }
    bool localPath = false;
    if (!states->inPath) {
        localPath = true;
        states->inPath = true;
        fprintf(out, "<%spath ", states->nameSpaceString);
        clipset_draw(states, out);
        fprintf(out, "d=\"");
    }
    bool ispolygon = true;
    polyline_draw("U_EMRPOLYGON16", contents, out, states, ispolygon);

    if (localPath) {
        states->inPath = false;
        fprintf(out, "Z\" ");
        bool filled = false;
        bool stroked = false;
        stroke_draw(states, out, &filled, &stroked);
        fill_draw(states, out, &filled, &stroked);
        if (!filled)
            fprintf(out, "fill=\"none\" ");
        if (!stroked)
            fprintf(out, "stroke=\"none\" ");

        fprintf(out, "/>\n");
    }
}
void U_EMRPOLYGON16_draw(const char *contents, FILE *out,
                         drawingStates *states) {
    FLAG_SUPPORTED;
    if (states->verbose) {
        U_EMRPOLYGON16_print(contents, states);
    }
    bool localPath = false;
    if (!states->inPath) {
        localPath = true;
        states->inPath = true;
        fprintf(out, "<%spath ", states->nameSpaceString);
        clipset_draw(states, out);
        fprintf(out, "d=\"");
    }
    bool ispolygon = true;
    polyline16_draw("U_EMRPOLYGON16", contents, out, states, ispolygon);

    if (localPath) {
        states->inPath = false;
        fprintf(out, "Z\" ");
        bool filled = false;
        bool stroked = false;
        stroke_draw(states, out, &filled, &stroked);
        fill_draw(states, out, &filled, &stroked);
        if (!filled)
            fprintf(out, "fill=\"none\" ");
        if (!stroked)
            fprintf(out, "stroke=\"none\" ");

        fprintf(out, "/>\n");
    }
}
void U_EMRPOLYLINE_draw(const char *contents, FILE *out,
                        drawingStates *states) {
    FLAG_SUPPORTED;
    if (states->verbose) {
        U_EMRPOLYLINE_print(contents, states);
    }
    bool localPath = false;
    if (!states->inPath) {
        localPath = true;
        states->inPath = true;
        fprintf(out, "<%spath ", states->nameSpaceString);
        clipset_draw(states, out);
        fprintf(out, "d=\"");
    }
    bool ispolygon = true;
    polyline_draw("U_EMRPOLYLINE", contents, out, states, ispolygon);
    if (localPath) {
        states->inPath = false;
        // fprintf(out, "Z\" ");
        fprintf(out, "\" ");
        bool filled = false;
        bool stroked = false;
        stroke_draw(states, out, &filled, &stroked);
        if (!filled)
            fprintf(out, "fill=\"none\" ");
        if (!stroked)
            fprintf(out, "stroke=\"none\" ");
        fprintf(out, "/>\n");
    }
}
void U_EMRPOLYLINE16_draw(const char *contents, FILE *out,
                          drawingStates *states) {
    FLAG_SUPPORTED;
    bool localPath = false;
    if (!states->inPath) {
        localPath = true;
        states->inPath = true;
        fprintf(out, "<%spath ", states->nameSpaceString);
        clipset_draw(states, out);
        fprintf(out, "d=\"");
    }
    bool ispolygon = true;
    polyline16_draw("U_EMRPOLYGON16", contents, out, states, ispolygon);

    if (localPath) {
        states->inPath = false;
        // fprintf(out, "Z\" ");
        fprintf(out, "\" ");
        bool filled = false;
        bool stroked = false;
        stroke_draw(states, out, &filled, &stroked);
        if (!filled)
            fprintf(out, "fill=\"none\" ");
        if (!stroked)
            fprintf(out, "stroke=\"none\" ");
        fprintf(out, "/>\n");
    }

    if (states->verbose) {
        U_EMRPOLYLINE16_print(contents, states);
    }
}
void U_EMRPOLYLINETO_draw(const char *contents, FILE *out,
                          drawingStates *states) {
    FLAG_SUPPORTED;
    if (states->verbose) {
        U_EMRPOLYLINETO_print(contents, states);
    }
    polyline_draw("U_EMRPOLYLINETO", contents, out, states, false);
}
void U_EMRPOLYLINETO16_draw(const char *contents, FILE *out,
                            drawingStates *states) {
    FLAG_SUPPORTED;
    if (states->verbose) {
        U_EMRPOLYLINETO16_print(contents, states);
    }
    polyline16_draw("U_EMRPOLYLINETO16", contents, out, states, false);
}
void U_EMRPOLYPOLYGON_draw(const char *contents, FILE *out,
                           drawingStates *states) {
    FLAG_SUPPORTED;
    if (states->verbose) {
        U_EMRPOLYPOLYLINE_print(contents, states);
    }
    bool localPath = false;
    if (!states->inPath) {
        localPath = true;
        states->inPath = true;
        fprintf(out, "<%spath ", states->nameSpaceString);
        clipset_draw(states, out);
        fprintf(out, "d=\"");
    }
    bool ispolygon = true;
    polypolygon_draw("U_EMRPOLYPOLYGON", contents, out, states, ispolygon);

    if (localPath) {
        states->inPath = false;
        fprintf(out, "\" ");
        bool filled = false;
        bool stroked = false;
        fill_draw(states, out, &filled, &stroked);
        stroke_draw(states, out, &filled, &stroked);
        if (!filled)
            fprintf(out, "fill=\"none\" ");
        if (!stroked)
            fprintf(out, "stroke=\"none\" ");

        fprintf(out, "/>\n");
    }
}
void U_EMRPOLYPOLYGON16_draw(const char *contents, FILE *out,
                             drawingStates *states) {
    FLAG_SUPPORTED;
    if (states->verbose) {
        U_EMRPOLYPOLYGON16_print(contents, states);
    }
    bool localPath = false;
    if (!states->inPath) {
        localPath = true;
        states->inPath = true;
        fprintf(out, "<%spath ", states->nameSpaceString);
        clipset_draw(states, out);
        fprintf(out, "d=\"");
    }
    bool ispolygon = true;
    polypolygon16_draw("U_EMRPOLYPOLYGON16", contents, out, states, ispolygon);

    if (localPath) {
        states->inPath = false;
        fprintf(out, "\" ");
        bool filled = false;
        bool stroked = false;
        fill_draw(states, out, &filled, &stroked);
        stroke_draw(states, out, &filled, &stroked);
        if (!filled)
            fprintf(out, "fill=\"none\" ");
        if (!stroked)
            fprintf(out, "stroke=\"none\" ");

        fprintf(out, "/>\n");
    }
}
void U_EMRPOLYPOLYLINE_draw(const char *contents, FILE *out,
                            drawingStates *states) {
    FLAG_SUPPORTED;
    if (states->verbose) {
        U_EMRPOLYPOLYLINE_print(contents, states);
    }
    bool localPath = false;
    if (!states->inPath) {
        localPath = true;
        states->inPath = true;
        fprintf(out, "<%spath ", states->nameSpaceString);
        clipset_draw(states, out);
        fprintf(out, "d=\"");
    }
    polypolygon_draw("U_EMRPOLYPOLYGON16", contents, out, states, false);

    if (localPath) {
        states->inPath = false;
        fprintf(out, "\" ");
        bool filled = false;
        bool stroked = false;
        stroke_draw(states, out, &filled, &stroked);
        if (!filled)
            fprintf(out, "fill=\"none\" ");
        if (!stroked)
            fprintf(out, "stroke=\"none\" ");

        fprintf(out, "/>\n");
    }
}
void U_EMRPOLYPOLYLINE16_draw(const char *contents, FILE *out,
                              drawingStates *states) {
    FLAG_SUPPORTED;
    if (states->verbose) {
        U_EMRPOLYPOLYLINE16_print(contents, states);
    }
    bool localPath = false;
    if (!states->inPath) {
        localPath = true;
        states->inPath = true;
        fprintf(out, "<%spath ", states->nameSpaceString);
        clipset_draw(states, out);
        fprintf(out, "d=\"");
    }
    polypolygon16_draw("U_EMRPOLYPOLYGON16", contents, out, states, false);

    if (localPath) {
        states->inPath = false;
        fprintf(out, "\" ");
        bool filled = false;
        bool stroked = false;
        stroke_draw(states, out, &filled, &stroked);
        if (!filled)
            fprintf(out, "fill=\"none\" ");
        if (!stroked)
            fprintf(out, "stroke=\"none\" ");

        fprintf(out, "/>\n");
    }
}
void U_EMRRECTANGLE_draw(const char *contents, FILE *out,
                         drawingStates *states) {
    FLAG_SUPPORTED;
    if (states->verbose) {
        U_EMRRECTANGLE_print(contents, states);
    }
    PU_EMRRECTANGLE pEmr = (PU_EMRRECTANGLE)(contents);
    POINT_D LT =
        point_cal(states, (double)pEmr->rclBox.left, (double)pEmr->rclBox.top);
    POINT_D RB = point_cal(states, (double)pEmr->rclBox.right,
                           (double)pEmr->rclBox.bottom);
    POINT_D dim;
    dim.x = RB.x - LT.x;
    dim.y = RB.y - LT.y;

    // If fixBrokenYTransform is true, we have to flip points on the Y axis.
    if (states->fixBrokenYTransform && dim.y < 0) {
        dim.y = -dim.y;
        LT.y -= dim.y;
    }

    fprintf(out,
            "<%srect x=\"%.4f\" y=\"%.4f\" width=\"%.4f\" height=\"%.4f\" ",
            states->nameSpaceString, LT.x, LT.y, dim.x, dim.y);
    bool filled = false;
    bool stroked = false;
    fill_draw(states, out, &filled, &stroked);
    stroke_draw(states, out, &filled, &stroked);
    clipset_draw(states, out);
    if (!filled)
        fprintf(out, "fill=\"none\" ");
    if (!stroked)
        fprintf(out, "stroke=\"none\" ");
    fprintf(out, "/>\n");
}
void U_EMRROUNDRECT_draw(const char *contents, FILE *out,
                         drawingStates *states) {
    FLAG_SUPPORTED;
    if (states->verbose) {
        U_EMRROUNDRECT_print(contents, states);
    }
    PU_EMRROUNDRECT pEmr = (PU_EMRROUNDRECT)(contents);
    POINT_D LT =
        point_cal(states, (double)pEmr->rclBox.left, (double)pEmr->rclBox.top);
    POINT_D RB = point_cal(states, (double)pEmr->rclBox.right,
                           (double)pEmr->rclBox.bottom);
    POINT_D dim;
    dim.x = RB.x - LT.x;
    dim.y = RB.y - LT.y;

    // If fixBrokenYTransform is true, we have to flip points on the Y axis.
    if (states->fixBrokenYTransform && dim.y < 0) {
        dim.y = -dim.y;
        LT.y -= dim.y;
    }

    fprintf(out,
            "<%srect x=\"%.4f\" y=\"%.4f\" width=\"%.4f\" height=\"%.4f\" ",
            states->nameSpaceString, LT.x, LT.y, dim.x, dim.y);
    fprintf(out, "rx=\"%.4f\" ry=\"%.4f\" ", scaleX(states, (double)pEmr->szlCorner.cx), scaleX(states, (double)pEmr->szlCorner.cy));
    bool filled = false;
    bool stroked = false;
    fill_draw(states, out, &filled, &stroked);
    stroke_draw(states, out, &filled, &stroked);
    clipset_draw(states, out);
    if (!filled)
        fprintf(out, "fill=\"none\" ");
    if (!stroked)
        fprintf(out, "stroke=\"none\" ");
    fprintf(out, "/>\n");
}
void U_EMRSETPIXELV_draw(const char *contents, FILE *out,
                         drawingStates *states) {
    FLAG_IGNORED;
    if (states->verbose) {
        U_EMRSETPIXELV_print(contents, states);
    }
    // PU_EMRSETPIXELV pEmr = (PU_EMRSETPIXELV)(contents);
}
void U_EMRSMALLTEXTOUT_draw(const char *contents, FILE *out,
                            drawingStates *states) {
    FLAG_PARTIAL;
    if (states->verbose) {
        U_EMRSMALLTEXTOUT_print(contents, states);
    }

    PU_EMRSMALLTEXTOUT pEmr = (PU_EMRSMALLTEXTOUT)(contents);
    fprintf(out, "<%stext ", states->nameSpaceString);
    clipset_draw(states, out);
    POINT_D Org = point_cal(states, (double)pEmr->Dest.x, (double)pEmr->Dest.y);

    size_t roff = sizeof(U_EMRSMALLTEXTOUT);
    if (!(pEmr->fuOptions & U_ETO_NO_RECT)) {
        roff += sizeof(U_RECTL);
    }

    returnOutOfEmf(pEmr + roff + pEmr->cChars);
    // FIXME, I gave up, it's directly taken from libUEMF/emf-inout.cpp, without
    // understanding it...
    uint32_t *dup_wt = NULL;
    if (pEmr->fuOptions & U_ETO_SMALL_CHARS) {
        dup_wt = U_Utf8ToUtf32le((char *)pEmr + roff, pEmr->cChars, NULL);
    } else {
        dup_wt = U_Utf16leToUtf32le((uint16_t *)((char *)pEmr + roff),
                                    pEmr->cChars, NULL);
    }
    char *ansi_text;
    ansi_text = (char *)U_Utf32leToUtf8((uint32_t *)dup_wt, 0, NULL);

    free(dup_wt);
    text_style_draw(out, states, Org);
    fprintf(out, ">");
    fprintf(out, "<![CDATA[%s]]>", ansi_text);
    fprintf(out, "</%stext>\n", states->nameSpaceString);
    free(ansi_text);
}
void U_EMRSTROKEANDFILLPATH_draw(const char *contents, FILE *out,
                                 drawingStates *states) {
    FLAG_PARTIAL;
    if (states->verbose) {
        U_EMRSTROKEANDFILLPATH_print(contents, states);
    }
    // real work done in U_EMRENDPATH
}
void U_EMRSTROKEPATH_draw(const char *contents, FILE *out,
                          drawingStates *states) {
    FLAG_PARTIAL;
    if (states->verbose) {
        U_EMRSTROKEPATH_print(contents, states);
    }
    // real work done in U_EMRENDPATH
}

#ifdef __cplusplus
}
#endif
/* vim:set shiftwidth=2 softtabstop=2 expandtab: */
