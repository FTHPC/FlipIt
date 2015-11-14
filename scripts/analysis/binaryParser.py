import struct

NEW_FILE_MASK = 0x8000
currSize = 0
def unpack(binary, fmt, size = None):
    """Reads a value from a binary file 'binary'.
    Parameters
    ----------
    binary : open binary file
        open binary file to read from
    fmt : single character
        format code for the data type to read from the binary file
    size : int
        used when reading strings from the binary file as the size of the
        string that should be read
    """

    global currSize
    value = None

    if fmt == 'B':
        value = struct.unpack(fmt, binary[currSize])[0]
        currSize += 1
    elif fmt == 'H':
        value = struct.unpack(fmt, binary[currSize:currSize+2])[0]
        currSize += 2
    elif fmt == 'L':
        value = struct.unpack(fmt, binary[currSize:currSize+8])[0]
        currSize += 8
    elif fmt == 's':
        value = binary[currSize:currSize + size]
        currSize += size

    return value


class INJ_TYPE:
    ARITHMETIC_FP = 0
    POINTER = 1
    ARITHMETIC_FIX = 2
    CONTROL_LOOP = 3
    CONTROL_BRANCH = 4
    UNKNOWN_INJ = 5


class INJ_INFO_TYPE:
    RESULT = 0
    VALUE = RESULT
    ADDRESS = 1
    UNKNOWN_INJ_TYPE = 30

class INST_TYPE:
    Unknown = 0

    # Terminator Instructions
    Ret = 1
    Br = 2
    Switch = 3
    IndirectBr = 4
    Invoke = 5
    Resume = 6
    Unreachable = 7

    # Standard binary operators...
    Add = 8
    FAdd = 9
    Sub = 10
    FSub = 11
    Mul = 12
    FMul = 13
    UDiv = 14
    SDiv = 15
    FDiv = 16
    URem = 17
    SRem = 18
    FRem = 19

    # Logical operators...
    Shl = 20
    LShr = 21
    Ashr = 22
    And = 23
    Or = 24
    Xor = 25

    #Memory operators...
    Alloca = 26
    Load = 27
    Store = 28
    GetElementPtr = 29
    Fence = 30
    AtomicCmpXchg = 31
    AtomicRMW = 32

    # Cast operators ...
    Truc = 33
    ZExt = 34
    SExt = 35
    FPToUI = 36
    FPToSI = 37
    UIToFP = 38
    SIToFP = 39
    FPTrunc = 40
    FPExt = 41
    PtrToInt = 42
    IntToPtr = 43
    BitCast = 44
    AddrSpaceCast = 45

    # Other operators...
    ICmp = 46
    FCmp = 47
    PHI = 48
    Call = 49
    Select = 50
    UserOp1 = 51
    UserOp2 = 52
    VAArg = 53
    ExtractElement = 54
    InsertElement = 55
    ShuffleVector = 56
    ExtractValue = 57
    InsertValue = 58
    LandingPad = 59

INST_STR = ["Unknown", "Ret", "Br", "Switch", "IndirectBr", "Invoke", "Resume",\
    "Unreachable", "Add", "FAdd", "Sub", "FSub", "Mul", "FMul", "UDiv", "SDiv",\
    "FDiv", "URem", "SRem", "FRem", "Shl", "LShr", "Ashr", "And", "Or", "Xor", \
    "Alloca", "Load", "Store", "GetElementPtr", "Fence", "AtomicCmpXchg", \
    "AtomicRMW", "Truc", "ZExt", "SExt", "FPToUI", "FPToSI", "UIToFP", "SIToFP",\
    "FPTrunc", "FPExt", "PtrToInt", "IntToPtr", "BitCast", "AddrSpaceCast", \
    "ICmp", "FCmp", "PHI", "Call", "Select", "UserOp1", "UserOp2", "VAArg", \
    "ExtractElement", "InsertElement", "ShuffleVector", "ExtractValue", \
    "InsertValue", "LandingPad"]


def info2Str(info, opstr):
    """Converts injection site info encoded as an integer into an ASCII string.
    Parameters
    ----------
    info : int
        injection type incoded as an integer to express as a string
    opstr : string
        opcode string from the same fault site that 'info' comes from, and is
        used to better specify the injection
    """

    if info == INJ_INFO_TYPE.RESULT:
        if opstr == "Store":
            return "Value"
        else:
            return "Result"
    elif info == INJ_INFO_TYPE.UNKNOWN_INJ_TYPE:
        return "Unknown"
    else:
        return "Arg " + str(info-1)


def type2Str(ty):
    """Converts an injection type encoded as an integer into an ASCII string.
    Parameters
    ----------
    ty : int
        injection type incoded as an integer to express as a string
    """

    if ty == INJ_TYPE.ARITHMETIC_FP:
        return "Arith-FP"
    elif ty == INJ_TYPE.POINTER:
        return "Pointer"
    elif ty == INJ_TYPE.ARITHMETIC_FIX:
        return "Arith-Fix"
    elif ty == INJ_TYPE.CONTROL_LOOP:
        return "Contorl-Loop"
    elif ty == INJ_TYPE.CONTROL_BRANCH:
        return "Control-Branch"
    else:
        return "Unknown"

def opcode2Str(opcode):
    """Converts an opcode into an ASCII string.
    Parameters
    ----------
    opcode : int
        opcode to express as a string
    """

    if opcode < len(INST_STR):
        return INST_STR[opcode]
    else: 
        return INST_STR[0]


def parseBinaryLogFile(c, filename, outfile = None):
    """Reads FLipIt LLVM log file and adds fault injection site
    information into the database.
    Parameters
    ----------
    c : object
        sqlite3 database handle that is open to a valid filled database
    filename : str
        absolute name of the LLVM log file
    outfile : any
        if value is not 'None' then this function will write an ASCII
        version of the log file to disk with the name 'filename' but
        use the extension .txt
    """

    if outfile != None:
        name = outfile
        if name == "":
            name = filename
            if name[-3:] == "bin":
                name = name[0:-3] + "txt"
            
        outfile = open(name, "w")
    with open(filename, "rb") as f:
        logfile = f.read()
        fileVersion =  unpack(logfile, 'B')
        nameSize = unpack(logfile, 'H')
        srcFile = unpack(logfile, 's', nameSize)
        siteIdx = 0
        if outfile != None:
            outfile.write("File Version #: "+ str(fileVersion))
            outfile.write("\nFile Name: " + str(srcFile))

        # loop over all functions and fault locations
        while currSize < len(logfile): # for rest of file
            opcode = unpack(logfile, 'B') #read function header
            if opcode != 255: 
                # opcode(1 byte), Types/Info(1 byte [3,5 bits]), Location (2+ bytes)
                info_type = unpack(logfile, 'B')
                ty = info_type >> 5
                info = info_type & 0x1F
                lineNum = unpack(logfile, 'H')
                #print "opcode= ", opcode, " info= ", info, " ty= ", ty, " lineNum= ", lineNum
                msg = "\n#" + str(siteIdx) + "\t" + opcode2Str(opcode) + "\t" + info2Str(info, opcode2Str(opcode))\
                    + "\t" + type2Str(ty)
                
                # if the MSB bit in lineNum is set then lineNum is the size of
                # a new filename string and we must read a new lineNum
                #print "AND=", lineNum & NEW_FILE_MASK
                if lineNum & NEW_FILE_MASK != 0:
                    size = lineNum & 0x7FFF
                    lineNum = unpack(logfile, 'H', 2)
                    srcFile = unpack(logfile, 's', size)
                    #print "size=", size, " new file: ", srcFile, ":", lineNum #unpack(logfile, 's', size & 0x7F)
                #else:
                #    print "old file: ", srcFile, ":", lineNum
                msg += "\t" + srcFile + ":" + str(lineNum)
                if c != None:                
                    c.execute("INSERT INTO sites VALUES (?,?,?,?,?,?,?)", (siteIdx, type, comment, file, funcName, lineNum, opcode))
                if outfile != None:
                    outfile.write(msg)
                siteIdx += 1
            else: # start of function
                size = unpack(logfile, 'B')
                if outfile != None:
                    outfile.write("\n\nFunction Name: " + unpack(logfile, 's', size))
                    outfile.write("\n------------------------------------------------------------------------------")
                siteIdx = unpack(logfile, 'L')
                #print "Fault Site Idx: ", siteIdx

    if outfile != None:
        outfile.write("\n")
        outfile.close()

#parseBinaryLogFile("work.c.LLVM.bin", "OUT.c.LLVM.txt")
#parseBinaryLogFile("/home/aperson40/research/compilerSDC/HPCCG-1.0/ddot.cpp.LLVM.bin", "DD.c.LLVM.txt")

