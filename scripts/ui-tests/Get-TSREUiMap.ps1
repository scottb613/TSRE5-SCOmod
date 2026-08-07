[CmdletBinding()]
param(
    [string]$Executable =
        (Join-Path $PSScriptRoot "..\..\TSREvcTST\TSRE5.exe"),

    [string]$Root =
        (Join-Path $PSScriptRoot "..\..\Gate5Sandbox\Train Simulator"),

    [string]$Route = "SCO_LHR",

    [string]$OutputPath =
        (Join-Path $PSScriptRoot "..\..\Gate5Evidence\ui-stress\ui-map.csv"),

    [string]$ScreenshotPath =
        (Join-Path $PSScriptRoot "..\..\Gate5Evidence\ui-stress\ui-map.png"),

    [int]$StartupTimeoutSeconds = 90
)

$ErrorActionPreference = "Stop"

Add-Type -AssemblyName UIAutomationClient
Add-Type -AssemblyName UIAutomationTypes
Add-Type -AssemblyName System.Drawing

$executablePath = (Resolve-Path -LiteralPath $Executable).Path
$rootPath = (Resolve-Path -LiteralPath $Root).Path
$routePath = Join-Path $rootPath "routes\$Route"
if (-not (Test-Path -LiteralPath $routePath -PathType Container)) {
    throw "Sandbox route was not found: $routePath"
}

$outputDirectory = Split-Path -Parent $OutputPath
New-Item -ItemType Directory -Path $outputDirectory -Force | Out-Null

$process = $null
try {
    $process = Start-Process -FilePath $executablePath `
        -WorkingDirectory (Split-Path -Parent $executablePath) `
        -ArgumentList @(
            "--routeedit", "--root", "`"$rootPath`"", "--route", $Route
        ) -PassThru

    $deadline = [datetime]::UtcNow.AddSeconds($StartupTimeoutSeconds)
    do {
        Start-Sleep -Milliseconds 250
        $process.Refresh()
        if ($process.HasExited) {
            throw "TSRE exited before exposing its main window."
        }
    } while ($process.MainWindowHandle -eq [IntPtr]::Zero `
        -and [datetime]::UtcNow -lt $deadline)

    if ($process.MainWindowHandle -eq [IntPtr]::Zero) {
        throw "TSRE did not expose a main window within the timeout."
    }

    $window = [System.Windows.Automation.AutomationElement]::FromHandle(
        $process.MainWindowHandle)
    $condition = [System.Windows.Automation.Condition]::TrueCondition
    $elements = $window.FindAll(
        [System.Windows.Automation.TreeScope]::Descendants, $condition)

    $rows = for ($index = 0; $index -lt $elements.Count; ++$index) {
        $element = $elements.Item($index)
        $current = $element.Current
        [pscustomobject]@{
            Index = $index
            Name = $current.Name
            AutomationId = $current.AutomationId
            ClassName = $current.ClassName
            ControlType = $current.ControlType.ProgrammaticName
            IsEnabled = $current.IsEnabled
            IsOffscreen = $current.IsOffscreen
            Left = $current.BoundingRectangle.Left
            Top = $current.BoundingRectangle.Top
            Width = $current.BoundingRectangle.Width
            Height = $current.BoundingRectangle.Height
        }
    }
    $rows | Export-Csv -LiteralPath $OutputPath `
        -NoTypeInformation -Encoding UTF8

    $bounds = $window.Current.BoundingRectangle
    $bitmap = New-Object Drawing.Bitmap(
        [int]$bounds.Width, [int]$bounds.Height)
    $graphics = [Drawing.Graphics]::FromImage($bitmap)
    try {
        $graphics.CopyFromScreen(
            [int]$bounds.Left, [int]$bounds.Top, 0, 0, $bitmap.Size)
        $bitmap.Save($ScreenshotPath, [Drawing.Imaging.ImageFormat]::Png)
    } finally {
        $graphics.Dispose()
        $bitmap.Dispose()
    }

    Write-Host "Process ID: $($process.Id)"
    Write-Host "Window:     $($window.Current.Name)"
    Write-Host "Controls:   $($rows.Count)"
    Write-Host "UI map:     $OutputPath"
    Write-Host "Screenshot: $ScreenshotPath"
} finally {
    if ($null -ne $process -and -not $process.HasExited) {
        $process.CloseMainWindow() | Out-Null
        if (-not $process.WaitForExit(10000)) {
            Stop-Process -Id $process.Id -Force
            $process.WaitForExit()
        }
    }
}
