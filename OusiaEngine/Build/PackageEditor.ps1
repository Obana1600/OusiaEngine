# エディタ（Develop）と Player（Release）をビルドして、チームのリポジトリの Editor/ に置く
#
# 使い方（OusiaEngine のリポジトリで）:
#   powershell -ExecutionPolicy Bypass -File OusiaEngine\Build\PackageEditor.ps1 -TeamRepo C:\path\to\TeamRepo
#   -SkipBuild  ビルドせず、今ある Generated/Outputs のものを置く
#   -Project    exe のプロジェクト（既定は Game）
#
# 置くもの:
#   Editor/OusiaEditor.exe, Runtime/（dxcompiler.dll, dxil.dll）, EngineAssets/, Player/Player.exe, version.txt とドキュメント
#   リポジトリの直下の .gitignore・.gitattributes・README.md は、ないときだけ置く
#   Assets/・ProjectSettings/ がなければ、空のプロジェクトとして作る

param(
	[Parameter(Mandatory = $true)][string]$TeamRepo,
	[string]$Project = "Game",
	[switch]$SkipBuild
)

$ErrorActionPreference = "Stop"

$sourceRoot = Split-Path $PSScriptRoot -Parent         # OusiaEngine/
$repoRoot = Split-Path $sourceRoot -Parent             # リポジトリの直下
$outputs = Join-Path $repoRoot "Generated\Outputs"
$templates = Join-Path $PSScriptRoot "TeamRepo"

# ---- ビルド ----
if (-not $SkipBuild) {
	$vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
	$msbuild = & $vswhere -latest -requires Microsoft.Component.MSBuild -find "MSBuild\**\Bin\amd64\MSBuild.exe" | Select-Object -First 1
	if (-not $msbuild) { throw "MSBuild が見つかりません" }

	foreach ($configuration in "Develop", "Release") {
		Write-Host "ビルド: $configuration"
		& $msbuild (Join-Path $sourceRoot "OusiaEngine.slnx") "-p:Configuration=$configuration" "-p:Platform=x64" -m -v:m -nologo
		if ($LASTEXITCODE -ne 0) { throw "$configuration のビルドに失敗しました" }
	}
}

$develop = Join-Path $outputs "Develop\$Project"
$release = Join-Path $outputs "Release\$Project"
foreach ($file in "$develop\$Project.exe", "$release\$Project.exe") {
	if (-not (Test-Path $file)) { throw "見つかりません: $file（-SkipBuild を外してビルドしてください）" }
}

# ---- Editor/ ----
New-Item -ItemType Directory $TeamRepo -Force | Out-Null
$TeamRepo = (Resolve-Path $TeamRepo).Path
$editor = Join-Path $TeamRepo "Editor"

# エディタが開いていると exe を上書きできない
$running = Get-Process OusiaEditor -ErrorAction SilentlyContinue | Where-Object { $_.Path -and $_.Path.StartsWith($editor, [StringComparison]::OrdinalIgnoreCase) }
if ($running) { throw "チームのリポジトリのエディタが開いています。閉じてから実行してください" }

New-Item -ItemType Directory $editor -Force | Out-Null
# dxcompiler.dll・dxil.dll は、DLL を exe の隣に置いていたころのもの
foreach ($old in "EngineAssets", "Player", "Runtime", "dxcompiler.dll", "dxil.dll") {
	$path = Join-Path $editor $old
	if (Test-Path $path) { Remove-Item $path -Recurse -Force }
}

Copy-Item "$develop\$Project.exe" (Join-Path $editor "OusiaEditor.exe") -Force
$runtime = Join-Path $editor "Runtime"
New-Item -ItemType Directory $runtime | Out-Null
Copy-Item "$develop\Runtime\dxcompiler.dll", "$develop\Runtime\dxil.dll" $runtime -Force
Copy-Item (Join-Path $sourceRoot "Engine\EngineAssets") (Join-Path $editor "EngineAssets") -Recurse

# 書き出し（File > Build Settings > Build...）に使う Player
$player = Join-Path $editor "Player"
New-Item -ItemType Directory $player | Out-Null
Copy-Item "$release\$Project.exe" (Join-Path $player "Player.exe")
# DLL は置かない。書き出すときはエディタの Runtime/ のものを使う（同じものを 2 つ入れないため）

# どの版かがわかるように、エンジンのコミットを書いておく
$commit = (git -C $repoRoot rev-parse --short HEAD 2>$null)
if ((git -C $repoRoot status --porcelain 2>$null) -ne $null) { $commit = "$commit（コミットしていない変更あり）" }
$version = "OusiaEngine $commit  $(Get-Date -Format 'yyyy-MM-dd HH:mm')"
[IO.File]::WriteAllText((Join-Path $editor "version.txt"), "$version`r`n", (New-Object Text.UTF8Encoding $false))
Copy-Item (Join-Path $templates "Editor-README.md") (Join-Path $editor "README.md") -Force
Copy-Item (Join-Path $templates "Scripting.md"), (Join-Path $templates "Script-API.md") $editor -Force

# ---- リポジトリの直下（ないときだけ置く。チームで書き換えたものは上書きしない） ----
foreach ($name in ".gitignore", ".gitattributes", "README.md") {
	$destination = Join-Path $TeamRepo $name
	if (-not (Test-Path $destination)) { Copy-Item (Join-Path $templates $name) $destination }
}

# ---- 初めてなら、空のプロジェクトを作る ----
$assets = Join-Path $TeamRepo "Assets"
if (-not (Test-Path $assets)) { New-Item -ItemType Directory (Join-Path $assets "Scenes") | Out-Null }

$projectSettings = Join-Path $TeamRepo "ProjectSettings"
if (-not (Test-Path $projectSettings)) {
	New-Item -ItemType Directory $projectSettings | Out-Null
	# レイヤーや物理の設定はエンジンの開発用のプロジェクトのものから始める（シーンの一覧と製品の名前は持ってこない）
	foreach ($file in "TagManager.json", "DynamicsManager.json") {
		$source = Join-Path $sourceRoot "$Project\ProjectSettings\$file"
		if (Test-Path $source) { Copy-Item $source $projectSettings }
	}
}

Write-Host ""
Write-Host "置きました: $editor"
Write-Host $version
Write-Host "チームのリポジトリで変更を確かめてから、コミットしてください"
