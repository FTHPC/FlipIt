import os
import sys

"""Converts run output files to the formate expected by the flipit analysis
    scripts

    Parameters
    argv[1] : str
        file prefix of the run files to look for e.g. foo for run outfiles
        such as foo.oe139493
    argv[2] : str
        path to where the run files are located

    Notes
    -----
    Keeps the original run ouput files.
    Places the newly generated run output files in the same directory
    as the orginals.
"""
# usage: python migrate.py <file_prefix> <file_path>
# e.g. python migrate.py faultInjectionTrial /path/to/run/files/
if __name__ == '__main__':
    prefix = sys.argv[1]
    path = sys.argv[2]
    files = []
    
    # cache all the file names
    for p, s, f in os.walk(path):
        for name in f:
            if prefix in name:
                print "Adding ", name, len(files)
                files.append(name)

    #order the file names and then append the trial number        
    files.sort() 
    for i in xrange(0, len(files):
        os.system("cp " + path + "/" + f + " " + path +"/" + prefix +  "_" + str(i))
