NanoSVG: https://github.com/memononen/nanosvg
Revision: 239e102ec2c691f2902e20ace2ed36ee4a35cfe6
License: zlib (LICENSE.zlib).

KiCad keeps declarations and implementations in separate files. Local changes
retain the FILE* API (which closes its input), stylesheet precedence, inferred
image dimensions, centered aspect ratio and zero default stroke width.

Text and tspan elements produce cubic outline paths using Fontconfig, FreeType
and HarfBuzz. Supported text properties include UTF-8 and XML entities, CDATA,
x/y/dx/dy lists, font family/size/weight/style, whitespace preservation, text
anchor and transforms. An optional per-parse Fontconfig configuration selects
application fonts without changing process settings. The caller owns it.

Text on paths, vertical writing, textLength and SVG font definitions are not
supported. Fontconfig supplies the closest installed font when a family is
unavailable. The rasterizer consumes the same parsed outlines as SVG import.
