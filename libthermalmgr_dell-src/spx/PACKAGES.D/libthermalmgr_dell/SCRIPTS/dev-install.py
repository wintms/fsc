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

    retval = Py_MkdirClean(SPXLIB+"/thermalmgr_dell")
    if retval != 0:
        return retval

    retval = Py_CopyFile("./libthermalmgr_dell.so."+PKG_MAJOR+"."+PKG_MINOR+"."+PKG_AUX, SPXLIB+"/thermalmgr_dell/")
    if retval != 0:
        return retval

    retval = Py_AddLiblinks(SPXLIB+"/thermalmgr_dell","libthermalmgr_dell.so."+PKG_MAJOR+"."+PKG_MINOR+"."+PKG_AUX)
    if retval != 0:
        return retval

    retval = Py_CopyDir("./", IMAGETREE+"/usr/local/lib/")
    if retval != 0:
        return retval

    retval = Py_AddLiblinks(IMAGETREE+"/usr/local/lib/","libthermalmgr_dell.so."+PKG_MAJOR+"."+PKG_MINOR+"."+PKG_AUX)
    if retval != 0:
        return retval

    # Create FSC configuration directory
    retval = Py_MkdirClean(IMAGETREE+"/conf/fsc")
    if retval != 0:
        return retval

    retval = Py_MkdirClean(IMAGETREE+"/etc/defconfig/fsc")
    if retval != 0:
        return retval

    # Install all FSC configuration files to /conf/fsc/
    config_files = [
        "fsc_z9964f_b2f.json",
        "fsc_z9964f_f2b.json"
    ]
    
    for config_file in config_files:
        retval = Py_CopyFile(config_file, IMAGETREE+"/conf/fsc/")
        if retval != 0:
            return retval

    # Install default configuration files to /etc/defconfig/
    for config_file in config_files:
        retval = Py_CopyFile(config_file, IMAGETREE+"/etc/defconfig/fsc/")
        if retval != 0:
            return retval

    if os.path.exists(IMAGETREE+"/etc/defconfig/fsc"):
        os.chmod(IMAGETREE+"/etc/defconfig/fsc",0o777)

    if os.path.exists(IMAGETREE+"/etc/defconfig/BMC1/ast2600evb_ami"):
        for config_file in config_files:
            os.chmod(IMAGETREE+"/etc/defconfig/fsc/"+config_file, 0o644)

    if os.path.exists(IMAGETREE+"/conf/fsc"):
        os.chmod(IMAGETREE+"/conf/fsc", 0o777)

    for config_file in config_files:
        if os.path.exists(IMAGETREE+"/conf/fsc/"+config_file):
            os.chmod(IMAGETREE+"/conf/fsc/"+config_file, 0o644)

    return 0
#-------------------------------------------------------------------------------------------------------
#				Rules for Debug Install

#-------------------------------------------------------------------------------------------------------
def debug_install():
    return 0
