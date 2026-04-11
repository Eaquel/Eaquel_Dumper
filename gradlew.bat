@rem Eaquel_Dumper Gradle Wrapper
@if "%DEBUG%"=="" @echo off
setlocal

set DIRNAME=%~dp0
if "%DIRNAME%"=="" set DIRNAME=.

set WRAPPER_JAR=%DIRNAME%Gradle\gradle-wrapper.jar

if not exist "%WRAPPER_JAR%" (
    echo ERROR: gradle-wrapper.jar not found at %WRAPPER_JAR%
    goto fail
)

if defined JAVA_HOME (
    set JAVA_EXE=%JAVA_HOME%\bin\java.exe
    if not exist "%JAVA_EXE%" (
        echo ERROR: JAVA_HOME is invalid: %JAVA_HOME%
        goto fail
    )
) else (
    set JAVA_EXE=java.exe
    java.exe -version >NUL 2>&1 || (
        echo ERROR: No java found in PATH. Install JDK 21.
        goto fail
    )
)

"%JAVA_EXE%" -Xmx64m -Xms64m ^
    -classpath "%WRAPPER_JAR%" ^
    org.gradle.wrapper.GradleWrapperMain ^
    %*

:end
if %ERRORLEVEL% neq 0 goto fail
endlocal
exit /B 0

:fail
endlocal
exit /B %ERRORLEVEL%
