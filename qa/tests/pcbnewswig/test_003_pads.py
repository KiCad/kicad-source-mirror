import pytest
import pcbnew

class TestPads:
    pcb : pcbnew.BOARD = None

    def setup_method(self):
        self.pcb = pcbnew.LoadBoard("../data/pcbnew/custom_pads.kicad_pcb")

    def test_custom_pads_outline(self):
        custom_pad1 = self.pcb.FindFootprintByReference("SB1").Pads()[0]
        expected_polygons = [[
            [1000000, 0],
            [500000, 750000],
            [-500000, 750000],
            [-500000, -750000],
            [500000, -750000]
        ]]
        # test accessor without layer
        polygon_set = custom_pad1.GetCustomShapeAsPolygon()
        assert expected_polygons == self.parse_polygon_set(polygon_set)

        # test accessor with layer
        polygon_set = custom_pad1.GetCustomShapeAsPolygon(pcbnew.F_Cu)
        assert expected_polygons == self.parse_polygon_set(polygon_set)

    def parse_polygon_set(self, polygon_set):
        result = []
        for polygon_index in range(polygon_set.OutlineCount()):
            outline = polygon_set.Outline(polygon_index)
            parsed_outline = []
            for point_index in range(outline.PointCount()):
                point = outline.CPoint(point_index)
                parsed_outline.append([point.x, point.y])
            result.append(parsed_outline)
        return result
