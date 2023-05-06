import pcbnew
import pytest


class TestPCBLoad:
    pcb : pcbnew.BOARD = None

    def setup_method(self):
        self.pcb = pcbnew.LoadBoard("../data/pcbnew/complex_hierarchy.kicad_pcb")

    def test_pcb_load(self):
        assert self.pcb is not None

    def test_pcb_track_count(self):
        tracks = list(self.pcb.GetTracks())
        assert len(tracks) == 361

    def test_pcb_modules(self):
        modules = list(self.pcb.GetFootprints())
        assert len(modules) == 72

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
            assert ref in board_refs

    def test_pcb_netcount(self):
        assert self.pcb.GetNetCount() == 51

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
        assert sorted(coordinates) == sorted(expected_coordinates)

    def test_pcb_text(self):
        drawings = list(self.pcb.GetDrawings())
        text = [d for d in drawings if d.GetClass() == "PCB_TEXT"]
        self.verify_text(text[0], 173355000, 68453000, pcbnew.F_Cu,
                         u'Actionneur\nPiezo New Amp\nV02')
        self.verify_text(text[1], 176149000, 64643000, pcbnew.B_Cu,
                         u'Actionneur\nPiezo New Amp\nV02')

    def verify_text(self, text, x, y, layer, s):
        assert list(text.GetPosition()) == [x, y]
        assert text.GetLayer() == layer
        assert text.GetText() == s
