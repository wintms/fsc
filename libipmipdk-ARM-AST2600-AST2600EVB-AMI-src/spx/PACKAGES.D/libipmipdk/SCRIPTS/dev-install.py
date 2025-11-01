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
    
	retval = Py_MkdirClean(SPXLIB+"/ipmipdk")
	if retval != 0:
		return retval
    
	retval = Py_CopyDir("./", IMAGETREE+"/usr/local/lib/")
	if retval != 0:
		return retval

	retval = Py_AddLiblinks(IMAGETREE+"/usr/local/lib/","libipmipdk.so."+PKG_MAJOR+"."+PKG_MINOR+"."+PKG_AUX)
	if retval != 0:
		return retval

	retval = Py_CopyFile("./libipmipdk.so."+PKG_MAJOR+"."+PKG_MINOR+"."+PKG_AUX,SPXLIB+"/ipmipdk/")
	if retval != 0:
		return retval

	retval = Py_AddLiblinks(SPXLIB+"/ipmipdk","libipmipdk.so."+PKG_MAJOR+"."+PKG_MINOR+"."+PKG_AUX)
	if retval != 0:
		return retval

	return 0
#-------------------------------------------------------------------------------------------------------
#				Rules for Debug Install

#-------------------------------------------------------------------------------------------------------
def debug_install():
	return 0
