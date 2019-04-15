#
# manual test session for "Tools -> Scripting Console"
# verify that "board.dsn" file is created after this session
# result should be identical to "File -> Export -> Specctra DSN"
#
import os
import pcbnew
board = pcbnew.GetBoard()
board_path = board.GetFileName()
path_tuple = os.path.splitext(board_path)
board_prefix = path_tuple[0]
export_path = board_prefix + ".dsn"
pcbnew.ExportSpecctraDSN(export_path)
