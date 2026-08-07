[CmdletBinding()]
param(
    [string]$Executable =
        (Join-Path $PSScriptRoot "..\..\TSREvcTST\TSRE5.exe"),

    [string]$Root =
        (Join-Path $PSScriptRoot "..\..\Gate5Sandbox\Train Simulator"),

    [string]$Route = "SCO_LHR",

    [ValidateSet("Track", "Road")]
    [string]$DatabaseType = "Track",

    [string]$ObjectShape = "A1t0_5mstrt.s",

    [string]$EvidenceDirectory =
        (Join-Path $PSScriptRoot "..\..\Gate5Evidence\ui-stress\prototype"),

    [bool]$DisablePlaceGuard = $true,

    [int]$StartupTimeoutSeconds = 90
)

$ErrorActionPreference = "Stop"

Add-Type -AssemblyName UIAutomationClient
Add-Type -AssemblyName UIAutomationTypes
Add-Type -AssemblyName System.Drawing
Add-Type -AssemblyName System.Windows.Forms
Add-Type @"
using System;
using System.Runtime.InteropServices;
public static class TSREUiNative {
    [DllImport("user32.dll")]
    public static extern bool SetForegroundWindow(IntPtr hWnd);
    [DllImport("user32.dll")]
    public static extern bool SetCursorPos(int X, int Y);
    [DllImport("user32.dll")]
    public static extern void mouse_event(
        uint flags, uint dx, uint dy, uint data, UIntPtr extraInfo);
    [DllImport("user32.dll")]
    public static extern void keybd_event(
        byte virtualKey, byte scanCode, uint flags, UIntPtr extraInfo);
}
"@

function Start-TSRESandbox {
    param([string]$Exe, [string]$RouteRoot, [string]$RouteName)
    return Start-Process -FilePath $Exe `
        -WorkingDirectory (Split-Path -Parent $Exe) `
        -ArgumentList @(
            "--routeedit", "--root", "`"$RouteRoot`"",
            "--route", $RouteName
        ) -PassThru
}

function Wait-TSREWindow {
    param([Diagnostics.Process]$Process, [int]$TimeoutSeconds)
    $deadline = [datetime]::UtcNow.AddSeconds($TimeoutSeconds)
    do {
        Start-Sleep -Milliseconds 250
        $Process.Refresh()
        if ($Process.HasExited) {
            throw "TSRE exited before exposing its main window."
        }
    } while ($Process.MainWindowHandle -eq [IntPtr]::Zero `
        -and [datetime]::UtcNow -lt $deadline)
    if ($Process.MainWindowHandle -eq [IntPtr]::Zero) {
        throw "TSRE did not expose its main window within the timeout."
    }
    return [System.Windows.Automation.AutomationElement]::FromHandle(
        $Process.MainWindowHandle)
}

function Find-ElementByName {
    param(
        [System.Windows.Automation.AutomationElement]$Window,
        [string]$Name,
        [int]$TimeoutSeconds
    )
    $condition = New-Object System.Windows.Automation.PropertyCondition(
        [System.Windows.Automation.AutomationElement]::NameProperty, $Name)
    $deadline = [datetime]::UtcNow.AddSeconds($TimeoutSeconds)
    do {
        $element = $Window.FindFirst(
            [System.Windows.Automation.TreeScope]::Descendants, $condition)
        if ($null -ne $element) {
            return $element
        }
        Start-Sleep -Milliseconds 250
    } while ([datetime]::UtcNow -lt $deadline)
    throw "UI element was not found: $Name"
}

function Find-ObjToolsButton {
    param(
        [System.Windows.Automation.AutomationElement]$Window,
        [string]$Name
    )
    $condition = New-Object System.Windows.Automation.PropertyCondition(
        [System.Windows.Automation.AutomationElement]::NameProperty, $Name)
    $elements = $Window.FindAll(
        [System.Windows.Automation.TreeScope]::Descendants, $condition)
    for ($index = 0; $index -lt $elements.Count; ++$index) {
        $element = $elements.Item($index)
        if ($element.Current.AutomationId -match '\.ObjTools\.') {
            return $element
        }
    }
    throw "Object Selection $Name control was not found."
}

function Find-ObjToolsSearch {
    param([System.Windows.Automation.AutomationElement]$Window)
    $condition = New-Object System.Windows.Automation.PropertyCondition(
        [System.Windows.Automation.AutomationElement]::ControlTypeProperty,
        [System.Windows.Automation.ControlType]::Edit)
    $elements = $Window.FindAll(
        [System.Windows.Automation.TreeScope]::Descendants, $condition)
    for ($index = 0; $index -lt $elements.Count; ++$index) {
        $element = $elements.Item($index)
        if ($element.Current.AutomationId -match '\.ObjTools\.') {
            return $element
        }
    }
    throw "Object Selection search control was not found."
}

function Find-Viewport {
    param([System.Windows.Automation.AutomationElement]$Window)
    $condition = New-Object System.Windows.Automation.PropertyCondition(
        [System.Windows.Automation.AutomationElement]::ClassNameProperty,
        "RouteEditorGLWidget")
    $element = $Window.FindFirst(
        [System.Windows.Automation.TreeScope]::Descendants, $condition)
    if ($null -eq $element) {
        throw "Route Editor OpenGL viewport was not found."
    }
    return $element
}

function Save-WindowImage {
    param(
        [System.Windows.Automation.AutomationElement]$Window,
        [string]$Path
    )
    $bounds = $Window.Current.BoundingRectangle
    $bitmap = New-Object Drawing.Bitmap(
        [int]$bounds.Width, [int]$bounds.Height)
    $graphics = [Drawing.Graphics]::FromImage($bitmap)
    try {
        $graphics.CopyFromScreen(
            [int]$bounds.Left, [int]$bounds.Top, 0, 0, $bitmap.Size)
        $bitmap.Save($Path, [Drawing.Imaging.ImageFormat]::Png)
    } finally {
        $graphics.Dispose()
        $bitmap.Dispose()
    }
}

function Find-RedTrackPoint {
    param(
        [string]$ScreenshotPath,
        [System.Windows.Rect]$WindowBounds,
        [System.Windows.Rect]$ViewportBounds
    )
    $bitmap = [Drawing.Bitmap]::FromFile($ScreenshotPath)
    try {
        $left = [Math]::Max(0, [int](
            $ViewportBounds.Left - $WindowBounds.Left))
        $top = [Math]::Max(0, [int](
            $ViewportBounds.Top - $WindowBounds.Top))
        $right = [Math]::Min($bitmap.Width - 1, [int](
            $ViewportBounds.Right - $WindowBounds.Left))
        $bottom = [Math]::Min($bitmap.Height - 1, [int](
            $ViewportBounds.Bottom - $WindowBounds.Top))

        # The route database overlay is rendered as a strong red line. Keep
        # the search away from the compass and side panels, then choose the
        # qualifying pixel nearest the preferred central-right test area.
        $scanLeft = [int]($left + ($right - $left) * 0.35)
        $scanRight = [int]($left + ($right - $left) * 0.75)
        $scanTop = [int]($top + ($bottom - $top) * 0.35)
        $scanBottom = [int]($top + ($bottom - $top) * 0.70)
        $preferredX = [int]($left + ($right - $left) * 0.62)
        $preferredY = [int]($top + ($bottom - $top) * 0.53)
        $best = $null
        $bestDistance = [double]::MaxValue

        for ($y = $scanTop; $y -le $scanBottom; ++$y) {
            for ($x = $scanLeft; $x -le $scanRight; ++$x) {
                $pixel = $bitmap.GetPixel($x, $y)
                if ($pixel.R -lt 175 -or $pixel.G -gt 105 -or
                        $pixel.B -gt 105 -or
                        ($pixel.R - $pixel.G) -lt 90) {
                    continue
                }
                $distance = [Math]::Abs($x - $preferredX) +
                    [Math]::Abs($y - $preferredY) * 0.25
                if ($distance -lt $bestDistance) {
                    $bestDistance = $distance
                    $best = [Drawing.Point]::new(
                        [int]($WindowBounds.Left + $x),
                        [int]($WindowBounds.Top + $y))
                }
            }
        }
        if ($null -eq $best) {
            throw "No red route-database overlay pixel was found in the safe test area."
        }
        return $best
    } finally {
        $bitmap.Dispose()
    }
}

function Test-GuardError {
    param([System.Windows.Automation.AutomationElement]$Window)
    $condition = New-Object System.Windows.Automation.PropertyCondition(
        [System.Windows.Automation.AutomationElement]::NameProperty, "ERROR")
    return $null -ne $Window.FindFirst(
        [System.Windows.Automation.TreeScope]::Descendants, $condition)
}

function Select-UiItem {
    param([System.Windows.Automation.AutomationElement]$Element)
    $pattern = $Element.GetCurrentPattern(
        [System.Windows.Automation.SelectionItemPattern]::Pattern)
    $pattern.Select()
    Start-Sleep -Milliseconds 150
    $bounds = $Element.Current.BoundingRectangle
    if ($bounds.Width -le 0 -or $bounds.Height -le 0) {
        throw "Selected UI item is not visible and cannot be clicked."
    }
    Click-ScreenPoint `
        -X ([int]($bounds.Left + $bounds.Width / 2)) `
        -Y ([int]($bounds.Top + $bounds.Height / 2))
}

function Enable-Toggle {
    param([System.Windows.Automation.AutomationElement]$Element)
    $pattern = $Element.GetCurrentPattern(
        [System.Windows.Automation.TogglePattern]::Pattern)
    if ($pattern.Current.ToggleState -eq
            [System.Windows.Automation.ToggleState]::Off) {
        $pattern.Toggle()
    }
}

function Invoke-UiButton {
    param([System.Windows.Automation.AutomationElement]$Element)
    $pattern = $Element.GetCurrentPattern(
        [System.Windows.Automation.InvokePattern]::Pattern)
    $pattern.Invoke()
}

function Click-ScreenPoint {
    param([int]$X, [int]$Y)
    [TSREUiNative]::SetCursorPos($X, $Y) | Out-Null
    Start-Sleep -Milliseconds 100
    [TSREUiNative]::mouse_event(0x0002, 0, 0, 0, [UIntPtr]::Zero)
    [TSREUiNative]::mouse_event(0x0004, 0, 0, 0, [UIntPtr]::Zero)
}

function Send-SaveShortcut {
    # Ctrl+Shift+S
    [TSREUiNative]::keybd_event(0x11, 0, 0, [UIntPtr]::Zero)
    [TSREUiNative]::keybd_event(0x10, 0, 0, [UIntPtr]::Zero)
    [TSREUiNative]::keybd_event(0x53, 0, 0, [UIntPtr]::Zero)
    [TSREUiNative]::keybd_event(0x53, 0, 0x0002, [UIntPtr]::Zero)
    [TSREUiNative]::keybd_event(0x10, 0, 0x0002, [UIntPtr]::Zero)
    [TSREUiNative]::keybd_event(0x11, 0, 0x0002, [UIntPtr]::Zero)
}

function Stop-OwnedProcess {
    param([Diagnostics.Process]$Process)
    if ($null -eq $Process -or $Process.HasExited) {
        return
    }
    $Process.CloseMainWindow() | Out-Null
    if (-not $Process.WaitForExit(10000)) {
        Stop-Process -Id $Process.Id -Force
        $Process.WaitForExit()
    }
}

$executablePath = (Resolve-Path -LiteralPath $Executable).Path
$rootPath = (Resolve-Path -LiteralPath $Root).Path
$workspace = (Resolve-Path -LiteralPath (
    Join-Path $PSScriptRoot "..\..")).Path
$requiredRoot = Join-Path $workspace "Gate5Sandbox\Train Simulator"
if ($rootPath -ne $requiredRoot) {
    throw "UI mutation is restricted to the disposable sandbox root: $requiredRoot"
}
$routePath = Join-Path $rootPath "routes\$Route"
if (-not (Test-Path -LiteralPath $routePath -PathType Container)) {
    throw "Sandbox route was not found: $routePath"
}
New-Item -ItemType Directory -Path $EvidenceDirectory -Force | Out-Null

$process = $null
$reloadProcess = $null
try {
    $process = Start-TSRESandbox -Exe $executablePath `
        -RouteRoot $rootPath -RouteName $Route
    $window = Wait-TSREWindow -Process $process `
        -TimeoutSeconds $StartupTimeoutSeconds
    $searchBox = Find-ObjToolsSearch -Window $window
    [TSREUiNative]::SetForegroundWindow($process.MainWindowHandle) | Out-Null
    $searchBounds = $searchBox.Current.BoundingRectangle
    Click-ScreenPoint `
        -X ([int]($searchBounds.Left + $searchBounds.Width / 2)) `
        -Y ([int]($searchBounds.Top + $searchBounds.Height / 2))
    [Windows.Forms.SendKeys]::SendWait("^a")
    [Windows.Forms.SendKeys]::SendWait($ObjectShape)
    Start-Sleep -Seconds 1
    $objectItem = Find-ElementByName -Window $window `
        -Name $ObjectShape -TimeoutSeconds 10
    $placeButton = Find-ObjToolsButton -Window $window -Name "Place New"
    $selectButton = Find-ObjToolsButton -Window $window -Name "Select"
    $viewport = Find-Viewport -Window $window

    if ($DisablePlaceGuard) {
        $guardButton = Find-ElementByName -Window $window `
            -Name "Place Guard: ON" -TimeoutSeconds $StartupTimeoutSeconds
        Invoke-UiButton -Element $guardButton
        Find-ElementByName -Window $window `
            -Name "Place Guard: OFF" -TimeoutSeconds 5 | Out-Null
    }

    $beforeImage = Join-Path $EvidenceDirectory "before-placement.png"
    Save-WindowImage -Window $window `
        -Path $beforeImage
    [TSREUiNative]::SetForegroundWindow($process.MainWindowHandle) | Out-Null
    Select-UiItem -Element $objectItem
    Enable-Toggle -Element $placeButton
    Start-Sleep -Milliseconds 500

    if ($DatabaseType -eq "Track") {
        $placementPoint = Find-RedTrackPoint -ScreenshotPath $beforeImage `
            -WindowBounds $window.Current.BoundingRectangle `
            -ViewportBounds $viewport.Current.BoundingRectangle
        $placementX = $placementPoint.X
        $placementY = $placementPoint.Y
    } else {
        $bounds = $viewport.Current.BoundingRectangle
        $placementX = [int]($bounds.Left + $bounds.Width * 0.62)
        $placementY = [int]($bounds.Top + $bounds.Height * 0.72)
    }
    [TSREUiNative]::SetForegroundWindow($process.MainWindowHandle) | Out-Null
    Click-ScreenPoint -X $placementX -Y $placementY
    Start-Sleep -Seconds 2
    Save-WindowImage -Window $window `
        -Path (Join-Path $EvidenceDirectory "after-placement.png")
    if (Test-GuardError -Window $window) {
        throw "TSRE rejected the automated $DatabaseType placement."
    }

    # TSRE commits a newly placed track or road object to its database when
    # it is deselected. Reproduce the normal editor workflow before saving.
    Enable-Toggle -Element $selectButton
    Start-Sleep -Milliseconds 300
    $viewportBounds = $viewport.Current.BoundingRectangle
    Click-ScreenPoint `
        -X ([int]($viewportBounds.Left + $viewportBounds.Width * 0.62)) `
        -Y ([int]($viewportBounds.Top + $viewportBounds.Height * 0.72))
    Start-Sleep -Seconds 1

    [TSREUiNative]::SetForegroundWindow($process.MainWindowHandle) | Out-Null
    Send-SaveShortcut
    Start-Sleep -Seconds 5
    Stop-OwnedProcess -Process $process

    $reloadProcess = Start-TSRESandbox -Exe $executablePath `
        -RouteRoot $rootPath -RouteName $Route
    $reloadWindow = Wait-TSREWindow -Process $reloadProcess `
        -TimeoutSeconds $StartupTimeoutSeconds
    Start-Sleep -Seconds 2
    Save-WindowImage -Window $reloadWindow `
        -Path (Join-Path $EvidenceDirectory "after-reload.png")

    [pscustomobject]@{
        DatabaseType = $DatabaseType
        ObjectShape = $ObjectShape
        PlacementX = $placementX
        PlacementY = $placementY
        FirstProcessId = $process.Id
        ReloadProcessId = $reloadProcess.Id
        CompletedUtc = [datetime]::UtcNow.ToString("o")
    } | ConvertTo-Json | Set-Content `
        -LiteralPath (Join-Path $EvidenceDirectory "ui-result.json") `
        -Encoding UTF8

    Write-Host "$DatabaseType placement UI cycle completed."
    Write-Host "Shape:      $ObjectShape"
    Write-Host "Screen:     $placementX, $placementY"
    Write-Host "Evidence:   $EvidenceDirectory"
} finally {
    Stop-OwnedProcess -Process $reloadProcess
    Stop-OwnedProcess -Process $process
}
