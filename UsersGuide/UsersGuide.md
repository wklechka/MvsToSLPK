# Users Guide

Here is the user interface:

![UI](theUI.jpg)

There are two input files: a Summit project (smtxml) and a PRJ. The PRJ is a file containing the PROJ WKT coordinate system. The smtxml is an XML file used by Datem's Summit.

The output will go to the selected 'Results' folder.

## Options Explained:

![UIfolders](options.jpg)

The most import option is Image Detail. This controls how much memory you will use and how detailed the ending mesh will be.  Using the default of 1000 is quite small, but you will be able the run most projects at this level without any division or splitting. Run the project with a small image detail first.  It will run relatively fast this way and use minimal memory.  Get results first before trying to increase the detail. 

Option Minimum Views (2,3, or 4). The minimum number of views needed for a point to be considered. The default is 3. Use 2 for low overlap datasets. Use 3 for typical datasets. Use 4 for higher-overlapped datasets.

## Expected Results:

Sample folders layout:

![UIfolders](folders.jpg)
