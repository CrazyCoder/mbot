@echo off

echo Setting up mBot environment...

set MIRANDA_HOME=d:\projects\miranda-svn\miranda
set PHP_HOME=c:\projects\amipdev\php-new\php-5.2.13

call "%VS90COMNTOOLS%\vsvars32.bat"
devenv /useenv msp.sln
