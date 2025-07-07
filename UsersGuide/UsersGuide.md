# Users Guide

Here is the user interface:

![UI](theUI.jpg)

There are two input files: a Summit project (smtxml) and a PRJ. The PRJ is a file containing the PROJ WKT coordinate system. The smtxml is an XML file used by Datem's Summit.

The output will go to the selected 'Results' folder.

## Options Explained:

![UIOptions](options.jpg)

The most import option is Image Detail. This controls how much memory you will use and how detailed the ending mesh will be.  Using the default of 1000 is quite small, but you will be able the run most projects at this level without any division or splitting. Run the project with a small image detail first.  It will run relatively fast this way and use minimal memory.  Get results first before trying to increase the detail. 

Option Minimum Views (2,3, or 4). The minimum number of views needed for a point to be considered. The default is 3. Use 2 for low overlap datasets. Use 3 for typical datasets. Use 4 for higher-overlapped datasets.

How to deal with large projects:

The preferred method is "Single Project" without OpenMVS split. This results in a single mesh and a single SLPK.  The "Use OpenMVS split" is really only here for completeness. OpenMVS has its own method of splitting up a project; I don't recommend this way, but it can be attempted for those that want to try it or are familiar with its use.

'Divide Project', this splits up the project into multiple projects. It does this by using a Quad Tree and continuously dividing up the imagery into quarters until each rectangle is less than the Maximum Images. The results will be a mesh/SLPK for each generated project.  Each project is treated individually.

## Expected Results:

Sample folders layout:

![UIfolders](folders.jpg)

Sample SLPK in ArcMap Pro:

![UISLPK](slpkSamp.jpg)

Sample Mesh PLY in MeshLab:

![UIMESH](meshSamp.jpg)
