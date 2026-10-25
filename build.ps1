param([string]$Target = 'all')

$ErrorActionPreference = 'Stop'

if (-not (Get-Command docker -ErrorAction SilentlyContinue)) {
    throw 'Docker was not found. Install Docker Desktop before building.'
}

$image = 'highway-rollers-builder'

$makeArguments = @($Target)
if ($Target -in @('all', 'release', 'debug')) {
    $isDebug = $Target -eq 'debug'
    $outputName = if ($isDebug) { 'HighwayRollersRandomizer_DEBUG' } else { 'HighwayRollersRandomizer' }
    # recompile when switching modes
    $makeArguments = @('-B', "DEBUG=$([int]$isDebug)", "TARGET=$outputName")
}

docker build --tag $image $PSScriptRoot
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

docker run --rm --volume "${PSScriptRoot}:/project" $image make @makeArguments
exit $LASTEXITCODE
