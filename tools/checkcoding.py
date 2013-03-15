#!/usr/bin/env python
import subprocess, os, difflib

EXTENSIONS=["cpp","cxx","h","hpp"]

#
# Function to call uncrustify, it returns the re-formated code and
# any errors
#

def uncrustify_file(filename):
	args = ("uncrustify", "-c", "uncrustify.cfg", "-f", filename)
	popen = subprocess.Popen(args, stdout=subprocess.PIPE,stderr=subprocess.PIPE)
	popen.wait()
	return [popen.stdout.readlines(),popen.stderr.read()]



#
# This function talks to bzr, and gets the list of modified files
#

def bzr_modified():
	modifieds = []
	args = ("bzr","status")
	popen = subprocess.Popen(args, stdout=subprocess.PIPE)
	popen.wait()
	output = popen.stdout.readlines()

	in_modifieds = False
	for line in output:
		line = line.rstrip("\r\n")
		if line.endswith(":"): in_modifieds = False
		if line.startswith("modified:"):
			in_modifieds = True
			continue
		if in_modifieds:
			modifieds.append( line.lstrip("\t ").rstrip("\t ") )

	return modifieds

def extension(filename):
	return os.path.splitext(filename)[1][1:].strip().lower()

def read_file(filename):
	f = open(filename,'r')
	data = f.readlines()
	f.close()
	return data

def ask_user(filename):
	msg = 'Shall I clean %s ?'%filename
	return raw_input("%s (y/N/E) " % msg).lower()


modified_files = bzr_modified()


for file in modified_files:

	if extension(file) in EXTENSIONS:

		[uncrustified,errors] = uncrustify_file(file)
		original = read_file(file)

		if len(errors.split("\n"))>2:
			print "There was a problem processing "+file+":"+errors
			continue

		if uncrustified==original:
			print file + " looks perfect!, well done!"
		else:
			print "Suggestions for: "+file

			diff = difflib.unified_diff(original,uncrustified,file,file+".uncrustified")

			for line in diff:
				print line.rstrip("\r\n")

			reply = ask_user(file)

			if reply in ["y","yes"]:
				f = open(file,'w')
				for line in uncrustified:
					f.write(line)
				f.close()
				print file + " UPDATED"

			if reply in ["e","ed","edit"]:
				os.system("$EDITOR "+file)

