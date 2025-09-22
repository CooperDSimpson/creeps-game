@echo off
setlocal enabledelayedexpansion

:: Count cpp files
set count=0
for %%f in (*.cpp) do set /a count+=1

if %count% equ 0 (
    echo No .cpp files found!
    pause
    goto :eof
)

if %count% equ 1 (
    :: Only one file, build it
    for %%f in (*.cpp) do (
        echo Building %%f...
        g++ "%%f" -o "%%~nf.exe" -luser32 -lgdi32 -lopengl32 -lglu32
        if !errorlevel! equ 0 (
            "%%~nf.exe"
        ) else (
            echo Build failed!
        )
    )
) else (
    :: Multiple files, show menu
    echo Multiple .cpp files found:
    set /a i=1
    for %%f in (*.cpp) do (
        echo !i!. %%f
        set file!i!=%%f
        set /a i+=1
    )
    
    set /p choice="Enter number (1-%count%): "
    if defined file%choice% (
        set selectedfile=!file%choice%!
        echo Building !selectedfile!...
        g++ "!selectedfile!" -o "!selectedfile:~0,-4!.exe" -luser32 -lgdi32 -lopengl32 -lglu32
        if !errorlevel! equ 0 (
            "!selectedfile:~0,-4!.exe"
        ) else (
            echo Build failed!
        )
    ) else (
        echo Invalid selection!
    )
)
