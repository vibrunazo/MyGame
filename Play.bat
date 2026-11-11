@echo off
rem Runs the packaged game (not the editor). Build it first with RunUAT BuildCookRun into Saved\Packaged (see MIGRATION.md, Phase 5).
rem Arguments pass through. To replay what the dev overlay (top left) shows: Play.bat -Seed=6032 -Floor=1 -Room=3
set GAME=%~dp0Saved\Packaged\Windows\MyGame.exe
if not exist "%GAME%" (
    echo The packaged game isn't built yet: %GAME% is missing.
    pause
    exit /b 1
)
start "" "%GAME%" %*
