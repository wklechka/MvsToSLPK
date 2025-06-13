SET COLMAP_LOCATION_VAR=J:\COLMAP
echo %COLMAP_LOCATION_VAR%

SET LOCATION_VAR=.\FreeTheMeshEXE
mkdir %LOCATION_VAR%
mkdir %LOCATION_VAR%\COLMAP
mkdir %LOCATION_VAR%\openMVS

rem copy colmap
xcopy "%COLMAP_LOCATION_VAR%\*.*" %LOCATION_VAR%\COLMAP /s /e
rem 
rem copy openMVS
copy ".\openMVS\Buildv17\bin\vc17\x64\Release\*.exe" %LOCATION_VAR%\openMVS
copy ".\openMVS\Buildv17\bin\vc17\x64\Release\*.dll" %LOCATION_VAR%\openMVS

copy ".\GitHub\MvsToSLPK\mvsToSlpkUI\x64\Release\*.exe" %LOCATION_VAR%
copy ".\GitHub\MvsToSLPK\mvsToSlpkUI\x64\Release\*.dll" %LOCATION_VAR%

rem  copy 3rd party stuff

SET THIRD_PARTY_LOCATION_VAR=.\GitHub\MvsToSLPK\third_party

copy "%THIRD_PARTY_LOCATION_VAR%\Mesh\VTK\Buildv17\bin\Release\*.*" %LOCATION_VAR%


copy "%THIRD_PARTY_LOCATION_VAR%\gdal.3.9\third_party\curl\Buildv17_64\lib\Release\libcurl.dll" %LOCATION_VAR%
copy "%THIRD_PARTY_LOCATION_VAR%\gdal.3.9\Buildv17\Release\gdal.3.9.dll" %LOCATION_VAR%
copy "%THIRD_PARTY_LOCATION_VAR%\gdal.3.9\third_party\PROJ-9.4\Buildv17\bin\Release\proj_9_4.dll" %LOCATION_VAR%
copy "%THIRD_PARTY_LOCATION_VAR%\gdal.3.9\third_party\sqlite\sqlite3.dll" %LOCATION_VAR%
copy "%THIRD_PARTY_LOCATION_VAR%\gdal.3.9\third_party\libjxl\Buildv17\Release\jxl.dll" %LOCATION_VAR%
copy "%THIRD_PARTY_LOCATION_VAR%\gdal.3.9\third_party\libjxl\Buildv17\Release\jxl_dec.dll" %LOCATION_VAR%
copy "%THIRD_PARTY_LOCATION_VAR%\gdal.3.9\third_party\libjxl\Buildv17\Release\jxl_threads.dll" %LOCATION_VAR%

copy "%THIRD_PARTY_LOCATION_VAR%\gdal.3.9\third_party\libjxl\Buildv17\third_party\brotli\Release\brotlicommon.dll" %LOCATION_VAR%
copy "%THIRD_PARTY_LOCATION_VAR%\gdal.3.9\third_party\libjxl\Buildv17\third_party\brotli\Release\brotlidec.dll" %LOCATION_VAR%
copy "%THIRD_PARTY_LOCATION_VAR%\gdal.3.9\third_party\libjxl\Buildv17\third_party\brotli\Release\brotlienc.dll" %LOCATION_VAR%


rem CURL
copy "%THIRD_PARTY_LOCATION_VAR%\curl\curl-curl-8_3_0\Buildv17\lib\Release\libcurl8.3.dll" %LOCATION_VAR%
copy "%THIRD_PARTY_LOCATION_VAR%\curl\libssh2-1.11.0\Buildv17\src\Release\libssh2.dll" %LOCATION_VAR%
copy "%THIRD_PARTY_LOCATION_VAR%\curl\nghttp2-master\Buildv17\lib\Release\nghttp2.dll" %LOCATION_VAR%
copy "%THIRD_PARTY_LOCATION_VAR%\curl\openssl-OpenSSL_1_1_1-stable\Release\bin\libcrypto-1_1-x64.dll" %LOCATION_VAR%
copy "%THIRD_PARTY_LOCATION_VAR%\curl\openssl-OpenSSL_1_1_1-stable\Release\bin\libssl-1_1-x64.dll" %LOCATION_VAR%

rem I3S
copy "%THIRD_PARTY_LOCATION_VAR%\Mesh\i3s\Buildv17\Release\*.dll" %LOCATION_VAR%

copy ".\GitHub\mvsToSlpk\LICENSE.rtf" %LOCATION_VAR%