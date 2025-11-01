#!/usr/bin/env python
#-----------------------------------------Import Rules Files -------------------------------------------
import PyRules
from   PyRules import PrjVars
from   PyRules import *
#--------------------------------------- Extra Python modules -------------------------------------------
#-------------------------------------------------------------------------------------------------------


#-------------------------------------------------------------------------------------------------------
#		  	      Rules for Installing to ImageTree
#-------------------------------------------------------------------------------------------------------
def build_install():
    IMAGETREE=PrjVars["IMAGE_TREE"]
    PKG_MINOR=PrjVars["PKG_MINOR"]
    PKG_MAJOR=PrjVars["PKG_MAJOR"]
    PKG_AUX=PrjVars["PKG_AUX"]
    SPXLIB=PrjVars["SPXLIB"]

    retval = Py_MkdirClean(SPXLIB+"/fscpdk")
    if retval != 0:
        return retval

    retval = Py_CopyDir("./", IMAGETREE+"/usr/local/lib/")
    if retval != 0:
        return retval

    retval = Py_AddLiblinks(IMAGETREE+"/usr/local/lib/","libfscpdk.so."+PKG_MAJOR+"."+PKG_MINOR+"."+PKG_AUX)
    if retval != 0:
        return retval

    retval = Py_CopyFile("./libfscpdk.so."+PKG_MAJOR+"."+PKG_MINOR+"."+PKG_AUX,SPXLIB+"/fscpdk/")
    if retval != 0:
        return retval

    retval = Py_AddLiblinks(SPXLIB+"/fscpdk","libfscpdk.so."+PKG_MAJOR+"."+PKG_MINOR+"."+PKG_AUX)
    if retval != 0:
        return retval

    # Create FSC configuration directory
    retval = Py_MkdirClean(IMAGETREE+"/conf/fsc")
    if retval != 0:
        return retval

    # Install all FSC configuration files to /conf/fsc/
    config_files = [
        "fsc_z9964f_b2f.json",
        "fsc_z9964f_f2b.json", 
        "fsc_z9964fl.json",
        "fsc_z9964f_b2f_ambient.json",
        "fsc_polynomial_example.json",
        "fsc_b2f_polynomial_calibration.json"
    ]
    
    for config_file in config_files:
        retval = Py_CopyFile(PrjVars["BUILD"]+"/"+PrjVars["PACKAGE"]+"/data/configs/"+config_file, IMAGETREE+"/conf/fsc/")
        if retval != 0:
            return retval

    # Install default configuration files to /etc/defconfig/
    retval = Py_CopyFile(PrjVars["BUILD"]+"/"+PrjVars["PACKAGE"]+"/data/configs/"+"fsc_z9964f_b2f.json", IMAGETREE+"/etc/defconfig/")
    retval = Py_CopyFile(PrjVars["BUILD"]+"/"+PrjVars["PACKAGE"]+"/data/configs/"+"fsc_z9964f_f2b.json", IMAGETREE+"/etc/defconfig/")
    retval = Py_CopyFile(PrjVars["BUILD"]+"/"+PrjVars["PACKAGE"]+"/data/configs/"+"fsc_z9964fl.json", IMAGETREE+"/etc/defconfig/")
    return 0
#-------------------------------------------------------------------------------------------------------
#				Rules for Debug Install

#-------------------------------------------------------------------------------------------------------
def debug_install():
    return 0
