import code
import unittest
import pcbnew
import pdb

class TestPCBLoad(unittest.TestCase):

    def setUp(self):
        self.pcb = pcbnew.LoadBoard("data/complex_hierarchy.kicad_pcb")

    def test_pcb_load(self):
        self.assertNotEqual(self.pcb,None)

    def test_pcb_track_count(self):
        tracks = list(self.pcb.GetTracks())
        self.assertEqual(len(tracks),361)

    def test_pcb_modules(self):
        modules = list(self.pcb.GetFootprints())
        self.assertEqual(len(modules), 72)

    def test_pcb_module_references(self):
        board_refs = list(module.GetReference() for
                          module in self.pcb.GetFootprints())

        known_refs = [u'P1', u'P3', u'C2', u'C1', u'D1', u'Q3', u'Q5', u'Q7',
                      u'Q6', u'Q1', u'Q2', u'Q4', u'Q8', u'P2', u'U1', u'U4',
                      u'P4', u'P5', u'P6', u'U3', u'R9', u'R15', u'RV1', u'RV2',
                      u'C3', u'C4', u'C5', u'C6', u'C7', u'C8', u'C9', u'D2',
                      u'D3', u'D4', u'D5', u'D6', u'D7', u'R3', u'R4', u'R5',
                      u'R6', u'R7', u'R8', u'R10', u'R11', u'R12', u'R13',
                      u'R14', u'R16', u'R17', u'R18', u'R19', u'R20', u'R21',
                      u'R22', u'MIRE', u'C10', u'C11',
                      u'U2', u'C14', u'C12', u'R23', u'R24', u'D9', u'D8', u'R25',
                      u'R26', u'R27', u'R28']

        for ref in known_refs:
            self.assertTrue(ref in board_refs)

    def test_pcb_netcount(self):
        self.assertEqual(self.pcb.GetNetCount(),51)

    def test_pcb_shapes(self):
        drawings = list(self.pcb.GetDrawings())
        edge_cuts = [d for d in drawings if d.GetLayer() == pcbnew.Edge_Cuts]
        coordinates = [[list(edge.GetStart()), list(edge.GetEnd())] for edge in edge_cuts]
        expected_coordinates = [
            [[88265000, 51816000], [188595000, 51816000]],
            [[88265000, 131826000], [88265000, 51816000]],
            [[188595000, 51816000], [188595000, 131826000]],
            [[188595000, 131826000], [88265000, 131826000]]
        ]
        self.assertEqual(sorted(coordinates), sorted(expected_coordinates))

    def test_pcb_text(self):
        drawings = list(self.pcb.GetDrawings())
        text = [d for d in drawings if d.GetClass() == "PTEXT"]
        self.verify_text(text[0], 173355000, 68453000, pcbnew.F_Cu,
                         u'Actionneur\nPiezo New Amp\nV02')
        self.verify_text(text[1], 176149000, 64643000, pcbnew.B_Cu,
                         u'Actionneur\nPiezo New Amp\nV02')

    def test_text_as_segments(self):
        footprint = self.pcb.FindFootprintByReference("U1")
        reference = footprint.Reference()
        segments = [[p.x, p.y] for p in reference.TransformToSegmentList()]
        expected_segments = [
            [141901333, 69196857], [143340666, 69196857], [143340666, 69196857],
            [143510000, 69142428], [143510000, 69142428], [143594666, 69088000],
            [143594666, 69088000], [143679333, 68979142], [143679333, 68979142],
            [143679333, 68761428], [143679333, 68761428], [143594666, 68652571],
            [143594666, 68652571], [143510000, 68598142], [143510000, 68598142],
            [143340666, 68543714], [143340666, 68543714], [141901333, 68543714],
            [143679333, 67400714], [143679333, 68053857], [143679333, 67727285],
            [141901333, 67727285], [141901333, 67727285], [142155333, 67836142],
            [142155333, 67836142], [142324666, 67945000], [142324666, 67945000],
            [142409333, 68053857]
        ]
        self.assertEqual(segments, expected_segments)

    def verify_text(self, text, x, y, layer, s):
        self.assertEquals(list(text.GetPosition()), [x, y])
        self.assertEquals(text.GetLayer(), layer)
        self.assertEquals(text.GetText(), s)

    #def test_interactive(self):
    #    code.interact(local=locals())

if __name__ == '__main__':
    unittest.main()
