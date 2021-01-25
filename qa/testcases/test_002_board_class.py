import code
import unittest
import os
import pcbnew
import pdb
import tempfile


from pcbnew import *


BACK_COPPER = 'Back_Copper'
B_CU = 'B.Cu'
NEW_NAME = 'My_Fancy_Layer_Name'


class TestBoardClass(unittest.TestCase):

    def setUp(self):
        self.pcb = LoadBoard("data/complex_hierarchy.kicad_pcb")
        self.TITLE="Test Board"
        self.COMMENT1="For load/save test"
        self.FILENAME=tempfile.mktemp()+".kicad_pcb"

    def test_pcb_find_module(self):
        module = self.pcb.FindFootprintByReference('P1')
        self.assertEqual(module.GetReference(),'P1')

    def test_pcb_get_track_count(self):
        pcb = BOARD()

        self.assertEqual(pcb.Tracks().size(),0)

        track0 = TRACK(pcb)
        pcb.Add(track0)
        self.assertEqual(pcb.Tracks().size(),1)

        track1 = TRACK(pcb)
        pcb.Add(track1)
        self.assertEqual(pcb.Tracks().size(),2)

    def test_pcb_bounding_box(self):
        pcb = BOARD()
        track = TRACK(pcb)
        pcb.Add(track)

        track.SetStart(wxPointMM(10.0, 10.0))
        track.SetEnd(wxPointMM(20.0, 30.0))

        track.SetWidth(FromMM(0.5))

        #!!! THIS FAILS? == 0.0 x 0.0 ??
        #height, width = ToMM(pcb.ComputeBoundingBox().GetSize())
        bounding_box = pcb.ComputeBoundingBox()
        height, width = ToMM(bounding_box.GetSize())

        margin = 0 # margin around bounding boxes (currently 0)
        self.assertAlmostEqual(width, (30-10) + 0.5 + margin, 2)
        self.assertAlmostEqual(height,  (20-10) + 0.5 + margin, 2)

    def test_pcb_get_pad(self):
        pcb = BOARD()
        module = FOOTPRINT(pcb)
        pcb.Add(module)
        pad = PAD(module)
        module.Add(pad)

        pad.SetShape(PAD_SHAPE_OVAL)
        pad.SetSize(wxSizeMM(2.0, 3.0))
        pad.SetPosition(wxPointMM(0,0))

        #Update the footprint bounding box, because
        #the new pad must be inside the bounding box to be located
        module.CalculateBoundingBox()

        # easy case
        p1 = pcb.GetPad(wxPointMM(0,0))

        # top side
        p2 = pcb.GetPad(wxPointMM(0.9,0.0))

        # bottom side
        p3 = pcb.GetPad(wxPointMM(0,1.4))

        # TODO: get pad == p1 evaluated as true instead
        #       of relying in the internal C++ object pointer
        self.assertEqual(pad.this, p1.this)
        self.assertEqual(pad.this, p2.this)
        self.assertEqual(pad.this, p3.this)

    def test_pcb_save_and_load(self):
        pcb = BOARD()
        pcb.GetTitleBlock().SetTitle(self.TITLE)
        pcb.GetTitleBlock().SetComment(0,self.COMMENT1)
        result = SaveBoard(self.FILENAME,pcb)
        self.assertTrue(result)

        pcb2 = LoadBoard(self.FILENAME)
        self.assertNotEqual(pcb2,None)

        tb = pcb2.GetTitleBlock()
        self.assertEqual(tb.GetTitle(),self.TITLE)
        self.assertEqual(tb.GetComment(0),self.COMMENT1)

        os.remove(self.FILENAME)

    def test_pcb_layer_name_set_get(self):
        pcb = BOARD()
        pcb.SetLayerName(31, BACK_COPPER)
        self.assertEqual(pcb.GetLayerName(31), BACK_COPPER)

    def test_pcb_layer_name_set_get(self):
        pcb = BOARD()
        pcb.SetLayerName(31, BACK_COPPER)
        self.assertEqual(pcb.GetLayerName(31), BACK_COPPER)

    def test_pcb_layer_id_get(self):
        pcb = BOARD()
        b_cu_id = pcb.GetLayerID(B_CU)
        pcb.SetLayerName(b_cu_id, NEW_NAME)

        # ensure we can get the ID for the new name
        self.assertEqual(pcb.GetLayerID(NEW_NAME), b_cu_id)

        # ensure we can get to the ID via the STD name too
        self.assertEqual(pcb.GetLayerID(B_CU), b_cu_id)

    def test_footprint_properties(self):
        pcb = LoadBoard("data/custom_fields.kicad_pcb")
        footprint = pcb.FindFootprintByReference('J1')
        expected_properties = {
            'Sheet file': 'custom_fields.kicad_sch',
            'Sheet name': '',
            'myfield': 'myvalue'
        }
        self.assertEquals(footprint.GetProperties(), expected_properties)
        self.assertEquals(footprint.GetProperty('myfield'), 'myvalue')
        self.assertEquals(footprint.HasProperty('myfield'), True)
        self.assertEquals(footprint.HasProperty('abcd'), False)
        footprint.SetProperty('abcd', 'efgh')
        self.assertEquals(footprint.HasProperty('abcd'), True)
        self.assertEquals(footprint.GetProperty('abcd'), 'efgh')

    #def test_interactive(self):
    # 	code.interact(local=locals())

if __name__ == '__main__':
    unittest.main()

