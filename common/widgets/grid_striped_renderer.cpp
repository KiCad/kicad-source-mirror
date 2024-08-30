
#include <widgets/grid_striped_renderer.h>

void STRIPED_CELL_RENDERER::Draw(wxGrid& grid, wxGridCellAttr& attr, wxDC& dc,
                                  const wxRect& rect, int row, int col, bool isSelected)
{
    // First draw the striped background for empty cells
    wxString cellValue = grid.GetCellValue(row, col);
    if (cellValue.IsEmpty())
    {
        DrawStripedBackground(dc, rect, isSelected);
    }

    // Then draw the text content using the parent class
    wxGridCellStringRenderer::Draw(grid, attr, dc, rect, row, col, isSelected);
}

void STRIPED_CELL_RENDERER::drawStripedBackground(wxDC& dc, const wxRect& rect, bool isSelected) const
{
    if (isSelected)
    {
        // For selected cells, use the selection color
        dc.SetBrush(wxBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT)));
        dc.SetPen(*wxTRANSPARENT_PEN);
        dc.DrawRectangle(rect);
        return;
    }

    // Draw horizontal stripes
    const int stripeHeight = 3;
    wxColour color1 = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);
    wxColour color2(240, 240, 240); // Light gray

    dc.SetPen(*wxTRANSPARENT_PEN);

    bool useColor1 = true;
    for (int y = rect.GetTop(); y < rect.GetBottom(); y += stripeHeight)
    {
        wxColour currentColor = useColor1 ? color1 : color2;
        dc.SetBrush(wxBrush(currentColor));

        int stripeBottom = wxMin(y + stripeHeight, rect.GetBottom());
        dc.DrawRectangle(rect.GetLeft(), y, rect.GetWidth(), stripeBottom - y);

        useColor1 = !useColor1;
    }
}