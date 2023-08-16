#!/usr/bin/python


# This tools finds the names of all files that are included
# from a given file (also the ones that are included from files
# that are themselves included).
#
# The output is a file with one line of the format
#
#  <inputfilename>: <list of included files>
#
# suitable for inclusion in makefiles.
#
# No support for conditional compiles and commments!

from sys import argv
from sys import exit
import re
import os

def scanfile(filename):
    incfiles = []
    filename = os.path.normpath(filename)
    if filename != "UI_gen.h" and filename != "ui_gen.c":
        try:
          f = open(filename, "r")
          for line in f.readlines():
            m = re.match(r"[ \t]*#include[ \t]*[<\"](.*)[>\"][ \t]*", line)
            if m <> None:
                incfile = os.path.normpath(m.group(1))
                incfiles.append(incfile)
          f.close()
        except IOError:
            dummy=1
            #print "current dir:", os.getcwd()
            #print "********* Couldn't open:", filename

    return incfiles
    

if __name__ == "__main__":
    if len(argv) != 4:
        print "Usage: " + argv[0] + " <input file> <target file> <output file>"
        exit(1)


    directory = os.path.dirname(argv[1])
    filename = os.path.basename(argv[1])
    
    queue = [ filename ]
    depend = []
    done = []

    if directory != "": os.chdir(directory)
    
    while not len(queue) == 0:
        # move the head of queue into currFile
        currFile = queue[0]
        queue = queue[1:]

        done.append(currFile)

        # find the files directly included by currFile
        currIncFiles = scanfile(currFile)

        for includedfile in currIncFiles:
            # Put the files included from currFile into depend if not already there
            if not includedfile in depend:
                depend.append(includedfile)
            # Put the files included from currFile into queue if they're not done yet
            # and they're not already in the queue
            if not includedfile in done and not includedfile in queue:
                queue.append(includedfile)

    f = open(argv[3],"w")
    f.write("%s:" % os.path.basename(argv[2]))
    for depfile in depend:
        f.write(" %s" % os.path.basename(depfile))
    f.write("\n")
    f.close
    
