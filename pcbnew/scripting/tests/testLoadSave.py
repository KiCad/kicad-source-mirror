from pcbnew import *
import unittest

class TestLoadSave(unittest.TestCase):

    def setUp(self):
				self.TITLE="Test Board"
				self.COMMENT1="For load/save test"
				self.FILENAME="/tmp/test.brd"
				
    def test_00_save(self):
				pcb = BOARD()
				pcb.GetTitleBlock().SetTitle(self.TITLE)
				pcb.GetTitleBlock().SetComment1(self.COMMENT1)
				result = SaveBoard(self.FILENAME,pcb)
				self.assertTrue(result)
        
    def test_01_load(self):
        pcb2 = LoadBoard(self.FILENAME)
        self.assertIsNotNone(pcb2)

    def test_02_titleblock_ok(self):
				pcb2 = LoadBoard(self.FILENAME)
				tb = pcb2.GetTitleBlock()
				self.assertEqual(tb.GetTitle(),self.TITLE)
				self.assertEqual(tb.GetComment1(),self.COMMENT1)

if __name__ == '__main__':
    unittest.main()