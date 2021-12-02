#
# KiCad python module for some helper functions
#

import os

def open_file_write(path, mode):
    ''' Open "path" for writing, creating any parent directories as needed.
    '''
    dir_path = os.path.dirname(path)

    if not os.path.isdir(dir_path):
        os.makedirs(dir_path)

    return open(path, mode)