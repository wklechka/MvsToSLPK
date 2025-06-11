# MvsToSLPK

 This program takes an XML project of aerial imagry with exterior infomation and a fixed camera.  Also a projection file is needed to setup coordinate systems; the system should be a projected system (uses PROJ part of GDAL).
 The goal is to generate SLPK/3DTiles which contain georeferenced meshes for viewing in programs like ArcPro.

 ## The Process:
 - Converts a xml project to a COLMAP project that is georeferenced.
 - Uses COLMAP to make the sparse data set.
 - The COLMAP data is converted to OpenMVS.
 - Use OpenMVS executables to generate a textured mesh. An offset file contains the georeferenced offset for the mesh.
 - This newly generated mesh is then split up and levels of detail are made.
 - The split up mesh is then generated into SLPK and 3DTiles

## Program Overview:
This program consists of two execuatbles made for Windows 10 or newer.
 - MvsToSLPK.exe: calls COLMAP and OpenMVS and finally generates SLPK.
 - mvsToSlpkUI.exe: a simple GUI on top of MvsToSLPK.exe.

 3rd Party executables can be obtained here:
 - COLMAP exes https://colmap.github.io/install.html
 - OpenMVS exes https://github.com/cdcseacave/openMVS
 
 3rd Party libraries:
 - GDAL 3.9 https://github.com/OSGeo/gdal
 - PROJ-9.4 https://github.com/OSGeo/PROJ/releases
 - CURL 8.3 https://github.com/curl/curl
 - VTK https://github.com/Kitware/VTK
 - vcg https://github.com/cdcseacave/VCG
 - Eigen 3.4 https://github.com/PX4/eigen
 - spdlog-1.x https://github.com/gabime/spdlog
 - tclap-1.4.0 https://github.com/mirror/tclap
 - happly https://github.com/nmwsharp/happly
 - i3s https://github.com/Esri/i3s-spec
 - OpenMesh https://github.com/Lawrencemm/openmesh
 - RectangleBinPack https://github.com/juj/RectangleBinPack
 - libjxl https://github.com/libjxl/libjxl

## Licenses:
Each directory has a license file.  Only the ones that touch other GPL code will be GPL; others will be MIT
 - GPL : MvsToSLPK, slpk3DTile
 - MIT : ImageRW, PTFile, SmtPrj, StdUtil

## ToDo
Add sample project

