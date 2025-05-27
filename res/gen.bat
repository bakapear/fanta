@echo off
setlocal enabledelayedexpansion

set appPath=app/
set resFile=%~dp0fanta.rc

> "%resFile%" ( 
    for /r "%~dp0%appPath%" %%f in (*) do (
        set "fullpath=%%f"
        set "relpath=!fullpath:%~dp0%=!"
        set "escapedPath=!relpath:\=/!"
        echo "!escapedPath:%appPath%=!" HTML "!escapedPath!"
    )
)

rc "%resFile%"
