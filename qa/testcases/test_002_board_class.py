import code
import unittest
import pcbnew
import pdb

from pcbnew import ToMM

class TestBoardClass(unittest.TestCase):

    def setUp(self):
        self.pcb = pcbnew.LoadBoard("data/complex_hierarchy.kicad_pcb")
    
    def test_pcb_find_module(self):
        module = self.pcb.FindModule('P1')
        self.assertEqual(module.GetReference(),'P1')

    def test_pcb_bounding_box(self):
        bounding_box = self.pcb.ComputeBoundingBox()
        
        height = ToMM( bounding_box.GetHeight() )
        width = ToMM( bounding_box.GetWidth() )

        # probably it's a cleaner test to generate a board with 
        # a couple of things, that we can know the exact size,
        # and then compute the bounding box, 

        self.assertAlmostEqual(height, 89.52, 2)
        self.assertAlmostEqual(width, 108.44, 2)

    #def test_interactive(self):
    # 	code.interact(local=locals())

if __name__ == '__main__':
    unittest.main()
   