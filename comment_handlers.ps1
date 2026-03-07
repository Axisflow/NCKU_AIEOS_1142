<#
.SYNOPSIS
    Comments out SVC_Handler, PendSV_Handler, and SysTick_Handler
    in stm32f4xx_it.c and stm32f4xx_it.h for FreeRTOS compatibility.
.DESCRIPTION
    FreeRTOS provides its own SVC_Handler, PendSV_Handler, and SysTick_Handler.
    This script comments out the default STM32 HAL implementations to avoid
    linker "multiple definition" errors.
.USAGE
    Run from the project root:
        powershell -ExecutionPolicy Bypass -File comment_handlers.ps1
#>

$handlers = @("SVC_Handler", "PendSV_Handler", "SysTick_Handler")

# --- Process stm32f4xx_it.c ---
$cFile = Join-Path $PSScriptRoot "Core\Src\stm32f4xx_it.c"

if (Test-Path $cFile) {
    $lines = Get-Content $cFile

    foreach ($handler in $handlers) {
        # Find the line: void <handler>(void)
        $funcLineIdx = -1
        for ($i = 0; $i -lt $lines.Count; $i++) {
            if ($lines[$i] -match "^\s*void\s+${handler}\s*\(void\)") {
                $funcLineIdx = $i
                break
            }
        }

        if ($funcLineIdx -eq -1) {
            Write-Host "[SKIP] ${handler} not found in stm32f4xx_it.c" -ForegroundColor Yellow
            continue
        }

        # Already commented out?
        if ($funcLineIdx -ge 1 -and $lines[$funcLineIdx - 1] -match "#if\s+0") {
            Write-Host "[SKIP] ${handler} already commented out in stm32f4xx_it.c" -ForegroundColor Yellow
            continue
        }

        # Walk backwards to find the start of the Doxygen comment block (/**)
        $commentStart = $funcLineIdx
        for ($j = $funcLineIdx - 1; $j -ge 0; $j--) {
            if ($lines[$j] -match "^\s*/\*\*") {
                $commentStart = $j
                break
            }
            # Stop searching if we hit a closing brace or another function — not part of our comment
            if ($lines[$j] -match "^\}" -or $lines[$j] -match "^\s*void\s+\w+\s*\(") {
                break
            }
        }

        # Walk forwards from '{' to find the matching closing '}'
        $braceDepth = 0
        $funcEnd = $funcLineIdx
        $braceFound = $false
        for ($k = $funcLineIdx; $k -lt $lines.Count; $k++) {
            foreach ($ch in $lines[$k].ToCharArray()) {
                if ($ch -eq '{') { $braceDepth++; $braceFound = $true }
                if ($ch -eq '}') { $braceDepth-- }
            }
            if ($braceFound -and $braceDepth -eq 0) {
                $funcEnd = $k
                break
            }
        }

        # Insert #if 0 / #endif
        $lines[$commentStart] = "#if 0 /* Commented out for FreeRTOS */" + [Environment]::NewLine + $lines[$commentStart]
        $lines[$funcEnd] = $lines[$funcEnd] + [Environment]::NewLine + "#endif"

        Write-Host "[OK] Commented out ${handler} in stm32f4xx_it.c (lines $($commentStart+1)..$($funcEnd+1))" -ForegroundColor Green
    }

    $lines | Set-Content -Path $cFile
} else {
    Write-Host "[ERROR] File not found: $cFile" -ForegroundColor Red
}

# --- Process stm32f4xx_it.h ---
$hFile = Join-Path $PSScriptRoot "Core\Inc\stm32f4xx_it.h"

if (Test-Path $hFile) {
    $content = Get-Content $hFile -Raw

    foreach ($handler in $handlers) {
        $declaration = "void ${handler}(void);"
        if ($content.Contains($declaration)) {
            $replacement = "// void ${handler}(void); /* Commented out for FreeRTOS */"
            $content = $content.Replace($declaration, $replacement)
            Write-Host "[OK] Commented out ${handler} in stm32f4xx_it.h" -ForegroundColor Green
        } else {
            Write-Host "[SKIP] ${handler} not found (or already commented) in stm32f4xx_it.h" -ForegroundColor Yellow
        }
    }

    Set-Content -Path $hFile -Value $content -NoNewline
} else {
    Write-Host "[ERROR] File not found: $hFile" -ForegroundColor Red
}

Write-Host "`nDone." -ForegroundColor Cyan
