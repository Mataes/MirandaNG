@echo off

pushd ..\plugins

pushd Actman
call make.bat fpc 11
if errorlevel 1 goto :Error
popd

pushd ImportTXT
call make.bat fpc 11
if errorlevel 1 goto :Error
popd

pushd mRadio
call make.bat fpc 11
if errorlevel 1 goto :Error
popd

pushd Dbx_mmap_SA\Cryptors\Athena
call make.bat fpc 11
if errorlevel 1 goto :Error
popd

popd
goto :eof

:Error
echo ============================= FAIL! =============================
exit
