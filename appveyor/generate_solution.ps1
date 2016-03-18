$premakeFile = ".\premake\premake5.lua"


Write-Host -ForegroundColor White "Generating Visual Studio Solution"
(&premake5 --file=$premakeFile vs2015) | Write-Host -ForegroundColor White

if($LASTEXITCODE -ne 0)
{
    throw [System.String]::Format("Premake failed to generate a solution without returning an error, ExitCode: {0}.", $LASTEXITCODE) 
}
