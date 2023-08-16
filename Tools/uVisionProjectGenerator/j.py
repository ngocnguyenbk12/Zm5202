import os
import sys
import glob
import fnmatch
import shutil
import string
from string import upper
from string import split

def ThPa():
    map_fname = '.\\build_prj\\' + sys.argv[1] + '\\Rels\\' + sys.argv[1] + '_patch.map'
    lin_fname = '.\\build_prj\\' + sys.argv[1] + '\\Rels\\' + sys.argv[1] + '_patch.lnp'
    f_lin = open(lin_fname)
    lin_str = f_lin.readlines()
    addr = 0
    if os.path.exists(map_fname):
        f_map = open(map_fname)
        f_map_lines = f_map.readlines() #
        for line in f_map_lines:              #
            if string.find(line, 'XDATA_PATCH    ?XD?ZW_XDATA_TAIL')>-1:
                addr = string.atoi(split(line,'H')[0],16) + 0xB000
                addr_str = upper(hex(addr))
                code = ' CODE(C:' + addr_str + '-C:0XFFFF),\n'
                code_patch = ' CODE_PATCH(C:' + addr_str + '-C:0XFFFF),\n'
                const = ' CONST(C:' + addr_str + '-C:0XFFFF),\n'
                const_patch = ' CONST_PATCH(C:' + addr_str + '-C:0XFFFF)\n'
                break
    else:
        print map_fname + ' not found'
    if addr < 4096 + 0xB000:
        print 'XDATA <<<<<<<<<<<<<<<<<< 4 KB Barier'  
        sys.exit(0)
    else:
        print 'XDATA >>>>>>>>>>>>>>>>>> 4 KB Barier'
    
    f_lin2_name =  '.\\build_prj\\' + sys.argv[1] + '\\Rels\\' + sys.argv[1] + '_patch2.lin'   
    f_lin2 = open(f_lin2_name,'w')
    for line in lin_str:
        if string.find(line, 'CODE (C:') > -1:
            f_lin2.write(code)
        else:    
            if string.find(line, 'CODE_PATCH (C:') > -1:       
                f_lin2.write(code_patch)
            else:
                if string.find(line, 'CONST (C:') > -1:       
                    f_lin2.write(const)
                else:
                    if string.find(line, 'CONST_PATCH (C:') > -1:       
                        f_lin2.write(const_patch)
                    else:
                        f_lin2.write(line)
    f_lin2.close()
    lx51 = os.environ['KEILPATH'] + '\BIN\LX51.EXE'
    print lx51 + ' @' + f_lin2_name
    os.system(lx51 + ' @' + f_lin2_name)
    och51 = os.environ['TOOLSDIR'] + '\uVisionProjectGenerator\och51.bat'
    param = '  ' + sys.argv[1]
    os.system(och51 + param)
    
def main():
    ThPa()        
    sys.exit(0)

main()  