#
# manual test session for "Tools -> Scripting Console"
# verify that "board-archive.pretty" folder is present
# result should be identical to "File -> Archive -> ..."
#
import os
import pcbnew
board = pcbnew.GetBoard()
board_path = board.GetFileName()
path_tuple = os.path.splitext(board_path)
board_prefix = path_tuple[0]
aStoreInNewLib = True
aLibName = "footprint-export"
aLibPath = board_prefix + "-archive.pretty"
pcbnew.ArchiveModulesOnBoard(aStoreInNewLib,aLibName,aLibPath)
