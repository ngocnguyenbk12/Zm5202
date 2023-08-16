import os
import sys
import glob
import fnmatch
import shutil


def CreateMyProduct():
    if not os.path.exists('starter.c'):
        print 'starter.c is NOT existed in current DIR'
        if os.path.exists('../util_func/starter.c'):
            print ' ... copying'
            shutil.copy('../util_func/starter.c', 'starter.c')
        else:
            print 'ERROR: ../util_func/starter.c not found'
    else:    
        print 'starter.c is already existed in current DIR'
        
    if not os.path.exists('starter.h'):
        print 'starter.h is NOT existed in current DIR'
        if os.path.exists('../util_func/starter.h'):
            print ' ... copying'
            shutil.copy('../util_func/starter.h', 'starter.h')
        else:
            print 'ERROR: ../util_func/starter.h not found'
    else:    
        print 'starter.h is already existed in current DIR'
        
        


def CreateCPatch():
    patched_names = glob.glob('*_patch.c')
    print 'patched_names >>>>>>>>>>>'
    for name in patched_names: 
        print name
    print '------------------------' 

    all_names = glob.glob('*.c')
    print 'all_names >>>>>>>>>>>'
    for name in all_names: 
        print name
    print '------------------------' 


    for filename in glob.glob('*.c'):
        if not (filename in patched_names):
            if ((os.path.splitext(filename)[0]+'_patch.c') in all_names):
                print filename + ' HAS _patch ver  <<<<<<<<<<<<<<<<'
            else:
                print filename + ' HAS NOT _patch ver  >>>>>>>>>>>>>'
                shutil.copy(filename, os.path.splitext(filename)[0]+'_patch.c')
        else:
            print filename +' is patched already <<<<<<<<<<<<<'  



def main():
    if len(sys.argv) < 2:
        print 'Param requred: P|M -Patch or MyProduct mode'
        sys.exit(1)
    args = sys.argv[1:]
    for arg in args:  
        if arg == 'P':
            CreateCPatch()
        elif arg == 'M':
            CreateMyProduct()
        else:
            print 'Param error =>' + arg  
        
    sys.exit(0)

main()  
