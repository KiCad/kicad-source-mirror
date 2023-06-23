import pytest
import os
import pcbnew
import tempfile


from pcbnew import *


BACK_COPPER = 'Back_Copper'
B_CU = 'B.Cu'
NEW_NAME = 'My_Fancy_Layer_Name'


class TestBoardClass:
    pcb : pcbnew.BOARD = None

    def setup_method(self):
        self.pcb = LoadBoard("../data/pcbnew/complex_hierarchy.kicad_pcb")
        self.TITLE="Test Board"
        self.COMMENT1="For load/save test"
        self.FILENAME=tempfile.mktemp()+".kicad_pcb"

    def test_pcb_find_module(self):
        module = self.pcb.FindFootprintByReference('P1')
        assert module.GetReference() =='P1'

    def test_pcb_get_track_count(self):
        pcb = BOARD()

        assert pcb.Tracks().size() == 0

        track0 = PCB_TRACK(pcb)
        pcb.Add(track0)
        assert pcb.Tracks().size() == 1

        track1 = PCB_TRACK(pcb)
        pcb.Add(track1)
        assert pcb.Tracks().size() == 2

    def test_pcb_bounding_box(self):
        pcb = BOARD()
        track = PCB_TRACK(pcb)
        pcb.Add(track)

        track.SetStart(VECTOR2I_MM(10.0, 10.0))
        track.SetEnd(VECTOR2I_MM(20.0, 30.0))

        track.SetWidth(FromMM(0.5))

        #!!! THIS FAILS? == 0.0 x 0.0 ??
        #height, width = ToMM(pcb.ComputeBoundingBox().GetSize())
        bounding_box = pcb.ComputeBoundingBox()
        height, width = ToMM(bounding_box.GetSize())

        margin = 0 # margin around bounding boxes (currently 0)
        assert width == pytest.approx((30-10) + 0.5 + margin, 2)
        assert height == pytest.approx((20-10) + 0.5 + margin, 2)

    def test_pcb_get_pad(self):
        pcb = BOARD()
        module = FOOTPRINT(pcb)
        pcb.Add(module)
        pad = PAD(module)
        module.Add(pad)

        pad.SetShape(PAD_SHAPE_OVAL)
        pad.SetSize(VECTOR2I_MM(2.0, 3.0))
        pad.SetPosition(VECTOR2I_MM(0,0))

        # easy case
        p1 = pcb.GetPad(VECTOR2I_MM(0,0))

        # top side
        p2 = pcb.GetPad(VECTOR2I_MM(0.9,0.0))

        # bottom side
        p3 = pcb.GetPad(VECTOR2I_MM(0,1.4))

        # TODO: get pad == p1 evaluated as true instead
        #       of relying in the internal C++ object pointer
        assert pad.this == p1.this
        assert pad.this == p2.this
        assert pad.this == p3.this

    def test_pcb_save_and_load(self):
        pcb = BOARD()
        pcb.GetTitleBlock().SetTitle(self.TITLE)
        pcb.GetTitleBlock().SetComment(0,self.COMMENT1)
        result = SaveBoard(self.FILENAME,pcb)
        assert result

        pcb2 = LoadBoard(self.FILENAME)
        assert pcb2 is not None

        tb = pcb2.GetTitleBlock()
        assert tb.GetTitle() == self.TITLE
        assert tb.GetComment(0) == self.COMMENT1

        os.remove(self.FILENAME)

    def test_pcb_layer_name_set_get(self):
        pcb = BOARD()
        pcb.SetLayerName(31, BACK_COPPER)
        assert pcb.GetLayerName(31) == BACK_COPPER

    def test_pcb_layer_name_set_get(self):
        pcb = BOARD()
        pcb.SetLayerName(31, BACK_COPPER)
        assert pcb.GetLayerName(31) == BACK_COPPER

    def test_pcb_layer_id_get(self):
        pcb = BOARD()
        b_cu_id = pcb.GetLayerID(B_CU)
        pcb.SetLayerName(b_cu_id, NEW_NAME)

        # ensure we can get the ID for the new name
        assert pcb.GetLayerID(NEW_NAME) == b_cu_id

        # ensure we can get to the ID via the STD name too
        assert pcb.GetLayerID(B_CU) == b_cu_id

    def test_footprint_properties(self):
        pcb = LoadBoard("../data/pcbnew/custom_fields.kicad_pcb")
        footprint = pcb.FindFootprintByReference('J1')
        expected_fields = {
            'myfield': 'myvalue'
        }
        assert footprint.GetSheetfile() == 'custom_fields.kicad_sch'
        assert footprint.GetSheetname() == ''
        assert footprint.GetFieldText('myfield') == 'myvalue'
        assert footprint.HasField('myfield')
        assert not footprint.HasField('abcd')
        footprint.SetField('abcd', 'efgh')
        assert footprint.HasField('abcd')
        assert footprint.GetFieldText('abcd') == 'efgh'
