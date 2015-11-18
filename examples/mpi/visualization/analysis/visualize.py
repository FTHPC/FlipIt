import os, sys
from matplotlib import pyplot as plot
import numpy as np
import matplotlib
from analysis_config import *


numFigs = 0
numTrials = 0
numTrialsInj = 0
colors = ['g','b', 'y', 'm', 'c', 'r']
TYPES = ["Arith-FP", "Pointer", "Arith-Fix", "Ctrl-Loop", "Ctrl-Branch"]
TYPES_LONG = ["Floating-Point", "Pointer", "Fixed-Point", "Control-Loop", "Control-Branch"]
typeIdx= {"Arith-FP":0, "Arithmetic":0, "Control": 6, "Pointer":1, "Arith-Fix":2, "Control-Loop":3, "Control-Branch":4}
nClassifications = len(TYPES)

def initVis(c):
    """Perform database operations to cache some values needed in later
    analysis.
    Parameters
    ----------
    c : object
        sqlite3 database handle that is open to a valid filled database
    """

    global numTrialsInj, numTrials
    c.execute("SELECT * FROM trials WHERE trials.numInj > 0")
    numTrialsInj = 1. * len(c.fetchall())
    c.execute("SELECT * FROM trials")
    numTrials = 1. * len(c.fetchall())
    if numTrialsInj == 0:
        print "No injections found to visualize..."
        sys.exit(1)
    print "Visualizing ", numTrials, " fault injection trials ("\
        , numTrialsInj,") with faults"

# wrapper around creating a pie chart
def barchart(bins, values, xlabel, ylabel, title, name=None, ticks=None):
    """Wapper around the matplotlib bar function. Useful when we need bars
    from multiple trends.

    Parameters
    ----------
    bins : np.array
        x values of where the bars should reside 1-D array
    values : np.array
        heights of the bars 2-D array [# trends][ len(bins)]
    xlabel : str
        label to display on the x axis
    ylabel : str
        label to display on the y axis
    title : str
        title of the figure
    name : list str
        trend names of length len(values) # trends
    ticks : list str
        x axis ticks to display of length len(bins)

    See Also
    ----------
    histo()
    """
    global numFigs
    plot.figure(numFigs)
    width = 1./len(values)
    if name == None:
        name = ["" for i in xrange(len(values))]
    for set in range(0, len(values)):
        plot.bar(bins+width*set, values[set][:], width, label=name[set], color=colors[set])
    plot.xlabel(xlabel)
    plot.ylabel(ylabel)
    plot.legend(fancybox = True, shadow=True)
    if ticks != None:
        plot.xticks(bins+.5, ticks, rotation=60)
    plot.title(title)
    plot.tight_layout()
    numFigs += 1

def piechart(percents, labels, title):
    """Wrapper around the matplotlib function pie

    Parameters
    ----------
    percents : array like
        values to plot as percentages
    labels : list of str
        trend names corresponding to the percentages in percents
    title : str
        title of the pie chart
    """
    
    global numFigs
    plot.figure(numFigs)
    patches, texts, autotexts = plot.pie(percents, labels=labels, autopct='%1.1f%%', colors=colors)
    for i in range(0, len(autotexts)):
        autotexts[i].set_color('w')
        autotexts[i].set_weight('bold')
        #autotexts[i].set_fontsize(16)
    plot.title(title)
    ax = plot.axis()
    v = (ax[0] - (ax[1] - ax[0])/4., ax[1] + (ax[1] - ax[0])/4., ax[2], ax[3])
    plot.axis(v)
    plot.tight_layout()
    numFigs += 1

def histo(values, bins, xlabel, ylabel, title, ticks=None, label=None, c=None):
    """Wrapper around the matplotlib function bar. Useful for a single trend

    Parameters
    ----------
    values : array like
        heights of the bars 1-D
    bins : array like
        x values of where the bars should reside 1-D array
    xlabel : str
        label to display on the x axis
    ylabel : str
        label to display on the y axis
    title : str
        title of the figure
    ticks : list str
        x axis ticks to display of length len(bins)
    label : list str
        trend name of the data being graphed
    c : char
        color of the trend
    
    See Also
    ---------
    barchart
    """
    global numFigs
    fig = plot.figure(numFigs)
    width = .8
    if c != None and label != None:
        plot.bar(bins, values, width, align ='center', label=label, color=c)
    else:
        plot.bar(bins, values, width, align = 'center')
    plot.xlabel(xlabel)
    plot.ylabel(ylabel)
    if ticks != None:
        plot.xticks(bins, ticks, rotation=60)
        #plot.xticks(bins+.5, ticks, rotation=60)
    plot.legend(loc='upper left',fancybox = True, shadow=True)
    plot.title(title)
    plot.tight_layout()
    numFigs += 1

def visClassifications(c, moreDetail=None):
    """Graphs of what types of faults were injected. Classification based
    on FlipIt classification. 
    
    Parameters
    ----------
    c : object
        sqlite3 database handle that is open to a valid filled database
    moreDetail : list of str
        function names to generate extra analysis of injections inside them
    Notes
    ----------
    More detail currently not implimented. 
    """
    typeBuckets = np.zeros(nClassifications + 1)
    bits =  np.zeros((nClassifications, 64))
    c.execute("SELECT site, bit  FROM injections")
    injs = c.fetchall()
    if len(injs) == 0:
        print "Error in visClassifications: No Injections\n"
        return
    maximum = max(injs)[0] +1
    locs = np.zeros((nClassifications, maximum))
        
    c.execute("SELECT type FROM sites INNER JOIN injections ON sites.site = injections.site")
    types = c.fetchall()
    
    for i in range(len(injs)):
        type = types[i][0]
        site = injs[i][0]
        bit = injs[i][1]
        #print type
        #print typeIdx
        if type in typeIdx:
            idx = typeIdx[type]
            if idx == 6:
                print "Warning: mapping type (", type,\
                    ") to type ( Control-Branch )"
                idx = 4
            typeBuckets[idx] += 1
            locs[idx][site] += 1
            bits[idx][bit] += 1
        else:
            print "VIZ: not classified = ", i
            typeBuckets[nClassifications] += 1
    fracs = typeBuckets/np.sum(typeBuckets)
    piechart(fracs[0:-1], TYPES_LONG, "Classification of Injections Based on Type")
    barchart(np.linspace(0,64,num=64), bits, "Bit Location", "Frequency",  "Injected bit", TYPES) 
    plot.xlim((0,64))




def visFunctions(c, moreDetail=None):
    """Graphs percentages of what functions faults were injection into
    
    Parameters
    ----------
    c : object
        sqlite3 database handle that is open to a valid filled database
    moreDetail : list of str
        function names to generate extra analysis of injections inside them
    """
    global numFigs
    c.execute("SELECT DISTINCT function FROM sites INNER JOIN injections ON injections.site = sites.site")
    funcs = c.fetchall()
    values = []
    for i in funcs:
        c.execute("SELECT COUNT(trial) FROM injections INNER JOIN sites ON sites.site = injections.site AND sites.function = ?", i)
        values.append(1. * c.fetchone()[0])
    piechart(np.array(values)/sum(values), [i[0] for i in funcs], "Injected Functions")
    
    ind = 0
    width = .5
    fig = plot.figure(numFigs)
    ax = plot.subplot(111)
    for i in funcs:
        i = i[0]
        c.execute("SELECT type FROM sites INNER JOIN injections ON sites.site = injections.site AND sites.function = ?", (i,))
        types = c.fetchall()
        
        tot = float(len(types))
        per = np.zeros(nClassifications)
        per = [ 0 for i in xrange(nClassifications)]
        for t in types:
            #per[typeIdx[t[0]]] += 1.
            idx = typeIdx[t[0]]
            if idx == 6:
                print "Warning: mapping type ( Control ) to type "\
                "( Control-Branch )"
                idx = 4
            per[idx] += 1
        per  = np.array(per)/tot * 100
        btm = 0
        legend = []
        for t in xrange(0,nClassifications):
            p1 = ax.bar(ind, per[t], width, align='center', color=colors[t], bottom=btm)
            btm += per[t]
            legend.append(p1)
        ind += 1
    ax.set_xticks(np.arange(len(funcs)))
    ax.set_xticklabels([f[0] for f in funcs], rotation=60, ha='center')
    numFigs += 1
    
    ax.set_ylim((0,100))
    ax.set_ylabel("Percent")
    
    # shrink graph to add legend and title at top
    plot.tight_layout()
    box = ax.get_position()
    ax.set_position([box.x0, box.y0,
                 box.width, box.height * 0.8])
    ax.legend(legend, TYPES, loc='upper center', bbox_to_anchor=(0.5, 1.15),
          fancybox=True, shadow=True, ncol=5)
    plot.setp(plot.gca().get_legend().get_texts(), fontsize='x-small')
    plot.text(0.5, 1.15, "Breakdown of Injection Type per Function",
    horizontalalignment='center', fontsize=14, transform = ax.transAxes) 

    # more detail for a function creates an html file with the source
    # code colored based on injection percentatge
    if moreDetail != None:
        visInjectionsInCode(c, moreDetail)

def visInjectionsInCode(c, functions):
    """Creates an html file with the source code colored based on injection
    percentages

    Parameters
    ----------
    c : object
        sqlite3 database handle that is open to a valid filled database
    functions : list of str
        function names to generate extra analysis of injections inside them
    """
    outfile = open("more.html", 'w')
    outfile.write("<!DOCTYPE html>\n<html>\n<body>\n")
    for func in functions:
        # grab all injections in this function
        c.execute("SELECT file, line FROM sites INNER JOIN injections ON sites.site = injections.site AND sites.function = ?", (func,))
        result = c.fetchall()
        if len(result) == 0:
            print "Warning (visInjectionsInCode): no injections in target function -- ", func
            continue

        # locate the min and max source line num to shrink output file size
        # we only want to show the section of code that we inject in
        lines = [i[1] for i in result] 
        file = result[0][0]
        if ".LLVM.txt" in file:
            file = result[-1][0]
        minimum = np.min(lines)-1
        minimum = minimum if minimum >= 0 else 0
        maximum = np.max(lines)+1
        bins = np.arange(minimum, maximum+1)
        values, bins = np.histogram(lines, bins, density=False) # <------------ check here
        bins = np.arange(minimum, maximum)
        values = 1.*values/np.sum(values)*100 # percents
        histo(values, bins, "Source Line Number", "Percent",\
        "Injections mapped to source line numbers for function: " + func)

        outfile.write("<h1>" + func + "()</h1>\n<table>\n")
        if minimum == 0:
            outfile.write("Unable to assign " + str(values[0]) + "\% of injections to source code.\n")
            minimum = np.min(np.trim_zeros(lines)) - 1
            values = values[minimum:]
        outfile.write("<table>\n")
        if os.path.isfile(file):
            srcPath = ""
        if not os.path.isfile(srcPath+file):
            print "Warning (visInjectionsInCode): source file not found -- ", srcPath + file
            continue
        print "\nRelating injections to source code in file: ", srcPath+file
        FILE = open(srcPath+file, "r")
        function = FILE.readlines()[minimum:maximum]
        FILE.close()
        

        for i in range(1,len(function)):
            color = "bgcolor=\"" + getColor(values[i]) +"\""
            outfile.write("<tr " + color +">\n<td>"+ str(minimum+i) +\
                "</td>\n<td><code>" + str2html(function[i-1]) + "</code></td>\n<td>"\
                + str(values[i]) + "</td>\n</tr>\n")
        outfile.write("</table>\n")
        
    outfile.write("</body>\n</html>\n")
    outfile.close()

def str2html(s):
    """Replaces '<', '>', and '&' with html equlivants

    Parameters
    ----------
    s : str
        string to convert to a vaild html string to display properly
    """
    return s.replace("&", "&amp;").replace(">", "&gt;").replace("<", "&lt;")

def getColor(x):
    """Selects an html color based on 0 <= x <= 100 
    
    Parameters
    ----------
    x : float
        percent of injection to color visually. Higher the percent the darker
        the color
    Returns
    ----------
    html color name useful for classifying
    """
    if x >= 75:
        return "red"
    elif x >= 50:
        return "orange"
    elif x >= 25:
        return "yellow"
    elif x >= 5:
        return "lime"
    else:
        return "white"


# graphs percentage of trials that crashed
def visCrashes(c):
    """Graph percentage of trials that crashed, and bit location and type
    of the corresponding injection
    
    Parameters
    ----------
    c : object
        sqlite3 database handle that is open to a valid filled database
    
    Notes
    ----------
    Only considers trials with injections when calculating percentages
    """

    bits =  np.zeros((nClassifications, 64))
    c.execute("SELECT type FROM sites INNER JOIN injections ON sites.site = injections.site INNER JOIN trials ON trials.trial = injections.trial AND trials.crashed = 1")
    crash = c.fetchall()
    crashed = float(len(crash))
    c.execute("SELECT site, bit  FROM injections INNER JOIN trials ON injections.trial = trials.trial AND trials.crashed = 1")
    sitesBits = c.fetchall()
    for i in range(len(sitesBits)):
        type = crash[i][0]
        bit = sitesBits[i][1]
        #bits[typeIdx[type]][bit] += 1
        idx = typeIdx[type]
        if idx == 6:
            print "Warning: mapping type ( Control ) to type "\
            "( Control-Branch )"
            idx = 4
        bits[idx][bit] += 1
    
    piechart([(numTrialsInj - crashed)/numTrialsInj, crashed/numTrialsInj],\
         ["Didn't Crash", "Crashed"], "Unexpected Termination") 
    barchart(np.linspace(0,64,num=64), bits, "Bit Location", "Frequency", "Unexpected Termination: Injected bit", TYPES) 
    plot.legend(loc='upper left', fancybox = True, shadow=True)
    plot.xlim((0,64))
   
    for i in range(nClassifications):
        if np.sum(bits[i][:]) > 0:
            histo(bits[i][:], np.linspace(0, 64, num=64), "Bit Location", "Frequency", "Unexpected Termination", None, TYPES[i], colors[i])
            plot.xlim((0,64))
            #plot.legend('upper left')

# graphs percent of trials that threw at least 1 assert 
def visAsserts(c):
    """Graphs percent of trials that threw at least 1 assert
    
    Parameters
    ----------
    c : object
        sqlite3 database handle that is open to a valid filled database
    Notes
    ----------
    Only considers trials with injections when calculating percentages
    """
    c.execute("SELECT DISTINCT trial from signal WHERE num == 6") 
    asserts = len(c.fetchall())
    piechart([asserts/numTrialsInj, (numTrials - asserts)/numTrialsInj],\
         ["Failed Assert(s)", "Didn't Assert"], "Trials with Injections Asserting")

# graphs percent of trials that generated at a certian signal type that is reported in the output file
def visSignals(c):
    """Graphs the percent of trials that generated a certian signal type
    reported in the output file
    
    Parameters
    ----------
    c : object
        sqlite3 database handle that is open to a valid filled database
    
    Notes
    ----------
    If a trial generates multiple signals, e.g. each rank asserts, we regard
    this as a single signal for the trial.
    """
    numSigs = 0.   
    sigs = {}

    c.execute("SELECT DISTINCT trial, num FROM signals")
    #build histogram for what signals were raised
    signals =  c.fetchall()
    for pair in signals:
        s = pair[1]
        numSigs += 1
        if s in sigs:#  and s != 11:
            sigs[s] += 1
        else:
            sigs[s] = 1.
    
    fracs = [(numTrials - numSigs)/numTrialsInj]
    labels = ["No Signal"]
    for s in sigs:
        fracs.append(sigs[s]/numTrialsInj)
        labels.append("Signal " + str(s))
    c.execute("SELECT DISTINCT trial from signals") 
    unique = len(c.fetchall())
    piechart(fracs, labels, str(unique) + " Trials Signaling")

def visDetections(c, moreDetail=None):
    """Graphs the percentage of trials that generate detection 
    
    Parameters
    ----------
    c : object
        sqlite3 database handle that is open to a valid filled database
    moreDetail : list of str
        function names to generate extra analysis of detections inside them
    Notes
    ----------
    Assumes one injection per trial.
    TODO: add graphs for more detail option
    """
    #if moreDetail != None:
    #    print "TODO: implement more detail option for 'visDetections'"
    c.execute("SELECT COUNT(trial) FROM trials WHERE detection = 1")
    detected = float(c.fetchone()[0])
    c.execute("SELECT SUM(numInj) FROM trials")
    numInj = float(c.fetchone()[0])
    
    piechart([detected/numInj, (numInj - detected)/numInj],\
    ["Detected", "Didn't Detect"], "Number of Trials with Detection ("+str(detected)+")")


def visDetectedInjections(c, moreDetail=None):
    """Graphs bit locations and type of what injections were detected
    
    Parameters
    ----------
    c : object
        sqlite3 database handle that is open to a valid filled database

    moreDetail : list of str
        function names to generate extra analysis of detections inside them
    Notes
    ----------
    TODO: add graphs for more detail option
    TODO: visualize injection sites detected
    TODO: visualize injection types detected
    Allows for user custom visualzations after first 'plot.show()'
    """
    #if moreDetail != None:
    #    print "TODO: implement more detail option for 'visDetectedInjections'"
    bits =  np.zeros((nClassifications,64))
    c.execute("SELECT site, bit  FROM injections INNER JOIN trials ON injections.trial = trials.trial AND trials.detection = 1")
    injs = c.fetchall()
    c.execute("SELECT type FROM sites INNER JOIN injections ON sites.site = injections.site INNER JOIN trials ON injections.trial = trials.trial AND trials.detection = 1")
    types = c.fetchall()
    
    for i in range(len(injs)):
        type = types[i][0]
        site = injs[i][0]
        bit = injs[i][1]
        #bits[typeIdx[type]][bit] += 1
        idx = typeIdx[type]
        if idx == 6:
            print "Warning: mapping type ( Control ) to type "\
            "( Control-Branch )"
            idx = 4
        bits[idx][bit] += 1

    barchart(np.linspace(0,64,num=64), bits, "Injected bit", "Frequency", "Detected Injection Bit Location",  TYPES) 
    plot.xlim((0,64))

    

def visDetectionLatency(c):
    """Visualizes the detection latency of an injection in the form of
    a bar chart with the x-axis as number of instruction executed after
    injection.
    
    Parameters
    ----------
    c : object
        sqlite3 database handle that is open to a valid filled database
    
    Notes
    ----------
    Assumes the user modifed the latency value in the detections table. It can
    be calucated by the
    'LLVM_dynamic_inst_of_detection - LLVM_dynamic_inst_of_injection'.
    The later can be obtained from the injection table for the trial, and the
    former can be obtained at detection time though the FlipIt API call
    'FLIPIT_GetDynInstCount()'.
    """
    #TODO: Extend to look at latency for each detector
    c.execute("SELECT latency FROM detections")

    buckets = [-1, 0, 1, 2, 3, 4, 5, 10, 1e2, 1e3, 1e9, 1e13]
    data = [ i[0] for i in c.fetchall()]
    values, bins = np.histogram(data, buckets, normed=False)
    xlabel = "# of instrumented LLVM instructions till detection"
    ylabel = "Frequency"
    title = "Detection Latency"
    ticks = ["-1", "0", "1", "2", "3", "4", "5->", "10->", "1e2->", "1e3->", "1e9->"]
    bins = np.arange(0,11)
    histo(values, bins, xlabel, ylabel, title, ticks)

