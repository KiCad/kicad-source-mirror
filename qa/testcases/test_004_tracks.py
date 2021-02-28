import unittest
import pcbnew

class TestTracks(unittest.TestCase):

    def setUp(self):
        self.pcb = pcbnew.LoadBoard("data/tracks_arcs_vias.kicad_pcb")

    def test_tracks(self):
        tracks = [t for t in self.pcb.Tracks() if t.GetClass() == 'TRACK']
        self.assertEqual(16, len(tracks))
        track = sorted(tracks, key=lambda t: [t.GetStart()[0], t.GetStart()[1]])[0]
        self.assertEqual([27000000, 27585787], [track.GetStart()[0], track.GetStart()[1]])
        self.assertEqual([27000000, 26500000], [track.GetEnd()[0], track.GetEnd()[1]])
        self.assertEqual(250000, track.GetWidth())
        self.assertEqual('McNetty', track.GetNetname())

    def test_arcs(self):
        arcs = [t for t in self.pcb.Tracks() if t.GetClass() == 'ARC']
        self.assertEqual(13, len(arcs))
        arc = sorted(arcs, key=lambda t: [t.GetStart()[0], t.GetStart()[1]])[0]
        self.assertEqual([29414211, 26499999], [arc.GetCenter()[0], arc.GetCenter()[1]])
        self.assertEqual([1800, 2250], [round(arc.GetArcAngleStart()), round(arc.GetArcAngleEnd())])
        self.assertEqual(2414211, round(arc.GetRadius()))

    def test_vias(self):
        vias = [t for t in self.pcb.Tracks() if t.GetClass() == 'VIA']
        self.assertEqual(2, len(vias))
        via = sorted(vias, key=lambda t: [t.GetStart()[0], t.GetStart()[1]])[0]
        self.assertEqual([29000000, 41000000], [via.GetStart()[0], via.GetStart()[1]])
        self.assertEqual(400000, via.GetDrillValue())
