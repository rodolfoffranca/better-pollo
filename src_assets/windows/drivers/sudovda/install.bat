@echo off
setlocal
pushd %~dp0

rem Validate the package before removing an existing working driver.
for %%F in (SudoVDA.dll nefconc.exe SudoVDA.inf sudovda.cat sudovda.cer) do (
  if not exist "%%F" (
    echo ERROR: Incomplete SudoVDA package: missing %%F.
    popd
    exit /b 1
  )
)

set "CERTUTIL=certutil"
where certutil >nul 2>&1 || set "CERTUTIL=%SystemRoot%\System32\certutil.exe"

echo ================
echo Installing cert for the SudoVDA driver...

"%CERTUTIL%" -addstore -f root "sudovda.cer"
if errorlevel 1 goto failed
"%CERTUTIL%" -addstore -f TrustedPublisher "sudovda.cer"
if errorlevel 1 goto failed

echo ================
echo Removing the old driver... It's OK to show an error if you're installing the driver for the first time.

nefconc.exe --remove-device-node --hardware-id root\sudomaker\sudovda --class-guid "4D36E968-E325-11CE-BFC1-08002BE10318"

echo ================
echo Installing the new driver...

nefconc.exe --create-device-node --class-name Display --class-guid "4D36E968-E325-11CE-BFC1-08002BE10318" --hardware-id root\sudomaker\sudovda
if errorlevel 1 goto failed
nefconc.exe --install-driver --inf-path "SudoVDA.inf"
if errorlevel 1 goto failed

echo ================
echo Done!

popd
exit /b 0

:failed
echo ERROR: SudoVDA installation failed. Run this script as administrator and check the errors above.
popd
exit /b 1
