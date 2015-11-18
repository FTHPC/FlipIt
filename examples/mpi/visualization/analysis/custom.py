import numpy as np
from matplotlib import pyplot as plot
from analysis_config import *
from visualize import *

def customInit(c):
    """User defined function to initialize the fault injection campaign
    database.

    Parameters
    ----------
    c : object
        sqlite3 database handle that is open to a valid filled database
    Notes
    ----------
    Users will  want to use this funcction to extend current tables or
    add there own tables to the data.
    
    Example
    -------
    c.execute("SELECT name FROM sqlite_master WHERE type='table' AND name='custom'")
    if len(c.fetchall()) == 0:
        c.execute("CREATE TABLE custom (trial int, site, iter int, rank int, level int, direction int)")
        c.execute("ALTER TABLE trials ADD COLUMN iter int")
        c.execute("ALTER TABLE trials ADD COLUMN converged int")
    """
     
def customParser(c, line, trial):
    """Parses the output of a fault injection trial for items interested to the user
    Parameters
    ----------
    c : object
        sqlite3 database handle that is open to a valid filled database

    line : str
        Single line from the output file of a fault injection trial
    trial: int
        number of fault injection trial. obtained from filename
    Notes
    ----------
    Users will want to fill out this function to parse both the custom
    injection log and any other lines such as detection messages or
    algorithm progress that may be visualized later.
    
    Example
    -------
    if numIterationMessage in l:
        iter = int(l.split(" ")[-1])
        c.execute("UPDATE  trials SET  iter = ? WHERE trials.trial = ?", (iter, trial))
    if notConvergedMsg in l:
        c.execute("UPDATE trials SET converged = ? WHERE trials.trial = ?", (0,trial))
    """

