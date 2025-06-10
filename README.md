# MvsToSLPK
 MVS to SLPK

 This program takes an XML project or aerial imagry with exterior infomation and a fixed camera.  Also a projection file is needed to setup coordinate systems; the system should be a projected system (uses PROJ part of GDAL).
 The goal is to generate SLPK/3DTiles mesh for viewing in ArcPro.

 The process:
 -Converts the xml project to COLMAP project that is Georeferenced.
 -Uses COLMAP to make the sparse data set.
 -The COLMAP project is converted to OpenMVS.
 -OpenMVS will generate a textured mesh. An offset file contains the georeferenced offset for the mesh.
 -This newly generated mesh is then split up and levels of detail are made.
 -The split up mesh is then generated into SLPK and 3DTiles

 3rd Party EXEs:
 COLMAP exes
 OpenMVS exes
 
 3rd Party libraries:
 GDAL 3.9
 CURL 8.3
 VTK
 PROJ-9.4
 Eigen 3.4
 spdlog-1.x
 tclap-1.4.0
 happly
 i3s
 OpenMesh
 RectangleBinPack
 vcg
 libjxl
