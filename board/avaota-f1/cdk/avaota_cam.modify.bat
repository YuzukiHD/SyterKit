@echo off
SET PATH=%Systemroot%\System32;%PATH%
forfiles.exe -P "%1" -M %2 -C "cmd /c echo %1/%2 is modified at: @fdate @ftime" | findstr modified
