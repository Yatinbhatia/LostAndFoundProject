@echo off

REM Compile sqlite3.c with gcc (C compiler)
gcc -c sqlite3.c -o sqlite3.o

REM Compile other C++ files and link with sqlite3.o
g++ -I. main.cpp db_utils.cpp match_logic.cpp sqlite3.o -o lostfound.exe

IF %ERRORLEVEL% EQU 0 (
    echo ✅ Build successful. Run with lostfound.exe
) ELSE (
    echo ❌ Build failed.
)
