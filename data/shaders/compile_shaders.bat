@echo off
cls

for %%f in (.\*) do (
    for %%i in (%%f) do (
        if NOT "%%~xi" == ".bat" if NOT "%%~xi" == ".spv" (
            glslangValidator -V %%f -o "%%f.spv"
        )
    )
)