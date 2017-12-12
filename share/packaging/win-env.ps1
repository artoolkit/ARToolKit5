# $env:HASH = git log --max-count=1 --pretty=format:"%H"
# $env:MESSAGE = git log --max-count=1 --pretty=format:"%s"
$env:BUILDTIME = [int][double]::Parse((Get-Date -UFormat %s))
Write-Host "##vso[task.setvariable variable=BUILDTIME]$Env:BUILDTIME"