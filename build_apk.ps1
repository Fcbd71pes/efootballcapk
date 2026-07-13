$ErrorActionPreference = "Stop"
$ProjectPath = "d:\Desktop\Personal_and_Others\efootball"
$SdkPath = "$ProjectPath\sdk"
$Env:ANDROID_HOME = $SdkPath

Write-Host "Creating SDK directories..."
if (!(Test-Path -Path "$SdkPath\cmdline-tools")) {
    New-Item -Path "$SdkPath\cmdline-tools" -ItemType Directory -Force | Out-Null
}

if (!(Test-Path -Path "$SdkPath\cmdline-tools\latest\bin\sdkmanager.bat")) {
    Write-Host "Downloading Android Command Line Tools..."
    Invoke-WebRequest -Uri "https://dl.google.com/android/repository/commandlinetools-win-10406996_latest.zip" -OutFile "$SdkPath\cmdline-tools.zip"
    
    Write-Host "Extracting Command Line Tools..."
    Expand-Archive -Path "$SdkPath\cmdline-tools.zip" -DestinationPath "$SdkPath\cmdline-tools" -Force
    
    # Move from cmdline-tools to latest
    Rename-Item -Path "$SdkPath\cmdline-tools\cmdline-tools" -NewName "latest"
    Remove-Item -Path "$SdkPath\cmdline-tools.zip" -Force
}

Write-Host "Accepting licenses..."
Start-Process -FilePath "cmd.exe" -ArgumentList "/c echo y | ""$SdkPath\cmdline-tools\latest\bin\sdkmanager.bat"" --licenses" -Wait -NoNewWindow

Write-Host "Installing NDK, CMake, and Platform Tools..."
& "$SdkPath\cmdline-tools\latest\bin\sdkmanager.bat" "platforms;android-33" "build-tools;33.0.1" "ndk;25.2.9519653" "cmake;3.22.1" "platform-tools"

if (!(Test-Path -Path "$SdkPath\gradle-8.0")) {
    Write-Host "Downloading Gradle 8.0..."
    Invoke-WebRequest -Uri "https://services.gradle.org/distributions/gradle-8.0-bin.zip" -OutFile "$SdkPath\gradle.zip"
    Write-Host "Extracting Gradle..."
    Expand-Archive -Path "$SdkPath\gradle.zip" -DestinationPath "$SdkPath" -Force
    Remove-Item -Path "$SdkPath\gradle.zip" -Force
}

Write-Host "Building APK..."
cd "$ProjectPath\android_app"
& "$SdkPath\gradle-8.0\bin\gradle.bat" assembleDebug

if ($LASTEXITCODE -eq 0) {
    Write-Host "APK Built Successfully! Saved in: $ProjectPath\android_app\app\build\outputs\apk\debug\app-debug.apk"
} else {
    Write-Host "Build failed."
}
