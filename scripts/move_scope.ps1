$dst = 'docs\SCOPE'
$stamp = (Get-Date -Format 'yyyyMMdd_HHmmss')
$backup = Join-Path $dst "backup_pre_move_$stamp"
New-Item -Path $backup -ItemType Directory -Force | Out-Null
if (Test-Path $dst) {
    Get-ChildItem $dst -Force | Where-Object { $_.Name -ne (Split-Path $backup -Leaf) } | ForEach-Object {
        try { Copy-Item -Path $_.FullName -Destination $backup -Recurse -Force -ErrorAction SilentlyContinue } catch { }
    }
}
$items = @('audit','business','market-research','stakeholders','suggestions','COMPETITOR_ANALYSIS.json','DOCUMENTATION_INDEX.md','HEKTOR_ANALYSIS.json','README.md')
foreach ($i in $items) {
    $src = Join-Path 'scope' $i
    if (Test-Path $src) {
        Write-Output "Moving $src to $dst"
        Move-Item -Path $src -Destination $dst -Force -ErrorAction Stop
    } else {
        Write-Output "Not found: $src"
    }
}
try { git add docs/SCOPE scope } catch { Write-Output 'git add failed' }
try { git commit -m "docs(scope): move scope content into docs/SCOPE (backup existing)" } catch { Write-Output 'git commit skipped (no changes?)' }
try { git push } catch { Write-Output 'git push skipped' }