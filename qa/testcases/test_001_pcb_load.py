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
        modules = list(self.pcb.GetModules())
        self.assertEqual(len(modules), 72)

    def test_pcb_module_references(self):
        board_refs = list(module.GetReference() for 
                          module in self.pcb.GetModules())

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

    #def test_interactive(self):
    #    code.interact(local=locals())

if __name__ == '__main__':
    unittest.main()
