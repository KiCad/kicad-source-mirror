#!/usr/bin/env python
# Created for KiCad project by Miguel
# Some modifications by Edwin
# GPL2

import subprocess
import os
import difflib


# class for checking and uncrustifying files
# defaults to cpp,cxx,h,hpp and c files
class coding_checker(object):
    file_filter = ["cpp", "cxx", "h", "hpp", "c"]

    # Function to call uncrustify, it returns the re-formatted code and
    # any errors
    #

    def uncrustify_file(self, filename=None):
        try:
            args = ("uncrustify", "-c", "uncrustify.cfg", "-f", filename)
            popen = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            popen.wait()
            return [popen.stdout.readlines(), popen.stderr.read()]
        except OSError as e:
            print "System returned : {e}\nCould not run uncrustify. Is it installed?".format(e=e.strerror)
            return [None, None]

    # This function runs bzr, and gets the list of modified files
    def bzr_modified(self):
        modified_files = []
        args = ("bzr", "status")
        try:
            popen = subprocess.Popen(args, stdout=subprocess.PIPE)
            popen.wait()
            output = popen.stdout.readlines()
        except OSError as e:
            print "System returned : {e}\nCould not run bzr. Is it installed?".format(e=e.strerror)
            return None

        in_modifieds = False
        for line in output:
            line = line.rstrip("\r\n")
            if line.endswith(":"):
                in_modifieds = False
            if line.startswith("modified:"):
                in_modifieds = True
                continue
            if line.startswith("added:"):
                in_modifieds = True
                continue

            if in_modifieds:
                modified_files.append(line.lstrip("\t ").rstrip("\t "))

        return modified_files


    def extension(self, filename):
        return os.path.splitext(filename)[1][1:].strip().lower()


    def read_file(self, filename):
        f = open(filename, 'r')
        data = f.readlines()
        f.close()
        return data


    def ask_user(self, filename):
        msg = 'Shall I clean %s ?' % filename
        return raw_input("%s (y/N/E) " % msg).lower()


    def main(self):
        # make list of modified file names
        modified_files = self.bzr_modified()

        if not modified_files:
            print "No modified files\n"
        else:

            for filename in modified_files:
                if self.extension(filename) in self.file_filter:
                    self.compare_and_suggest(filename)

    def compare_and_suggest(self,filename):
        # if it is a 'c' file try to uncrustify
        [uncrustified, errors] = self.uncrustify_file(filename)

        if not (uncrustified and errors):
            print "Program end"
            # problem in uncrustify
            return

        original = self.read_file(filename)

        if len(errors.split("\n")) > 2:
            print "There was a problem processing " + filename + ":" + errors
            return

        if uncrustified == original:
            print filename + " looks perfect!, well done!"
        else:
            print "Suggestions for: " + filename

            diff = difflib.unified_diff(original, uncrustified, filename, filename + ".uncrustified")

            for line in diff:
                print line.rstrip("\r\n")
            print ""
            reply = self.ask_user(filename)

            if reply in ["y", "yes"]:
                f = open(filename, 'w')
                for line in uncrustified:
                    f.write(line)
                f.close()
                print filename + " UPDATED"

            if reply in ["e", "ed", "edit"]:
                os.system("$EDITOR " + filename)

            print ""


if __name__ == '__main__':
    print "This program tries to do 2 things\n" \
          "1) call bzr to find changed files (related to the KiCad project)\n" \
          "2) call uncrustify on the changed files (to make the files comply with coding standards)\n"

    cc = coding_checker()
    cc.main()
