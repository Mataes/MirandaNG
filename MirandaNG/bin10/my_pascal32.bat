rem @echo off

pushd ..\plugins

pushd Actman
call make.bat fpc
popd

pushd ImportTXT
call make.bat fpc
popd

pushd Dbx_mmap_SA\Cryptors\Athena
call make.bat fpc
popd

pushd mRadio
call make.bat fpc
popd