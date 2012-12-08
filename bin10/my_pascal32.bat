rem @echo off

pushd ..\plugins

pushd Actman
call make.bat fpc 10
popd

pushd ImportTXT
call make.bat fpc 10
popd

pushd Dbx_mmap_SA\Cryptors\Athena
call make.bat fpc 10
popd

pushd mRadio
call make.bat fpc 10
popd