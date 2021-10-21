"""Unit-test parts of the CONNECTIVITY_DATA Python API."""

import unittest

import pcbnew


class TestConnectivity(unittest.TestCase):
    """Test that calls on BOARD.Connectivity() are functional."""

    def setUp(self):
        """Setup shared attributes."""
        self.pcb = pcbnew.LoadBoard("data/complex_hierarchy.kicad_pcb")
        self.connectivity = self.pcb.GetConnectivity()
        self.nets = self.pcb.GetNetsByName()

    def test_get_connectivity_returns_connectivity_data_object(self):
        """Verify: GetConnectivity() returns a CONNECTIVITY_DATA object."""
        connectivity = self.pcb.GetConnectivity()
        self.assertEqual(type(connectivity).__name__, "CONNECTIVITY_DATA")

    def test_get_connected_pads_on_track_returns_iterable_of_pads(self):
        """Verify: GetConnectedPads(track) returns an iterable of pads."""
        net = list(self.nets.values())[1]
        tracks = list(self.pcb.TracksInNet(net.GetNetCode()))
        track_with_pad = tracks[1]
        pads = self.connectivity.GetConnectedPads(track_with_pad)
        self.assertGreater(len(pads), 0)
        self.assertTrue(all(pad.GetClass() == "PAD" for pad in pads))

    def test_get_connected_tracks_returns_iterable_of_tracks(self):
        """Verify: GetConnectedTracks(track) returns an iterable of tracks."""
        net = list(self.nets.values())[1]
        net_tracks = self.pcb.TracksInNet(net.GetNetCode())
        net_track = list(net_tracks)[0]
        connected_tracks = self.connectivity.GetConnectedTracks(net_track)
        self.assertGreater(len(connected_tracks), 1)
        self.assertTrue(
            all(track.GetClass() == "PCB_TRACK" for track in connected_tracks)
        )
