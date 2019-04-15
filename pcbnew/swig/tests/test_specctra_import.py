#
# manual test session for "Tools -> Scripting Console"
# verify that "board.ses" file is applied after this session
# result should be identical to "File -> Import -> Specctra Session"
#
import os
import pcbnew
board = pcbnew.GetBoard()
board_path = board.GetFileName()
path_tuple = os.path.splitext(board_path)
board_prefix = path_tuple[0]
import_path = board_prefix + ".ses"
pcbnew.ImportSpecctraSES(import_path)
