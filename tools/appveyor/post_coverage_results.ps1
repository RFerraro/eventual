Add-Type -AssemblyName System.Net.Http

$coverallsApi = "https://coveralls.io/api/v1/jobs"

$commitID = $ENV:APPVEYOR_REPO_COMMIT
$committerName = $ENV:APPVEYOR_REPO_COMMIT_AUTHOR
$committerEmail = $ENV:APPVEYOR_REPO_COMMIT_AUTHOR_EMAIL
$commitMessage = $ENV:APPVEYOR_REPO_COMMIT_MESSAGE
$pullRequestNumber = $ENV:APPVEYOR_PULL_REQUEST_NUMBER
$buildVersion = $ENV:APPVEYOR_BUILD_VERSION
$buildNumber = [System.Int32]::Parse($ENV:APPVEYOR_BUILD_NUMBER)
$buildJobID = $ENV:APPVEYOR_JOB_ID  
$branchName = $ENV:APPVEYOR_REPO_BRANCH
$remoteUrl = ("https://github.com/" + $ENV:APPVEYOR_REPO_NAME + ".git")
$buildPath = ([string]($Env:APPVEYOR_BUILD_FOLDER)).TrimEnd('\') + '\'
$commitTime = [System.DateTime]::Parse($ENV:APPVEYOR_REPO_COMMIT_TIMESTAMP)

$buildUrl = ("https://ci.appveyor.com/project/RFerraro/eventual/build/" + $buildVersion)

function Read-Packages
{
    [CmdletBinding()]
    param([Parameter(Mandatory=$true,ValueFromPipeline=$true)] [System.Xml.XmlElement]$packagesNode)

    PROCESS
    {
        Foreach($package in $packagesNode.ChildNodes)
        {
            $package.classes | Read-Classes | Write-Output
        }
    }
}

function Read-Classes
{
    [CmdletBinding()]
    param([Parameter(Mandatory=$true,ValueFromPipeline=$true)] [System.Xml.XmlElement]$classesNode)

    PROCESS
    {
    
        $baseUri = (New-Object System.Uri $buildPath)
        $rootDrive = [System.IO.Path]::GetPathRoot($baseUri.AbsolutePath)

        Foreach($class in $classesNode.ChildNodes)
        {
            $fullUri = (New-Object System.Uri ($rootDrive + $class.filename))

            $classCoverage = [PSCustomObject]@{
                name = $baseUri.MakeRelativeUri($fullUri)
                source_digest = Compute-Digest($fullUri)
                coverage = Read-Lines($class)
            }

            Write-Output $classCoverage
        }
    }
}

function Read-Lines([System.Xml.XmlElement]$classNode)
{
    
    $lines = $classNode.SelectNodes("lines/line")

    $hitlist = New-Object System.Collections.ArrayList
    Foreach($line in $lines)
    {
        while(($hitlist.Count - 1) -lt $line.number)
        {
            $hitlist.Add($null) | Out-Null
        }

        $hitlist[$line.number] = [System.Nullable[int]]$line.hits
    }

    return ($hitlist.ToArray())
}

function Compute-Digest([System.Uri]$filePath)
{
    return (Get-FileHash -Path $filePath.AbsolutePath -Algorithm MD5).Hash
}

function Create-Git-Info
{
    PROCESS
    {
        $head = Create-Git-Head
        $remotes = Create-Git-Remotes

        $info = [PSCustomObject]@{
            head = $head
            branch = $branchName
            remotes = @($remotes)
        }

        Write-Verbose ("Creating Git Info: ")

        Write-Verbose $info

        Write-Output $info
    }
}

function Create-Git-Head
{
    PROCESS
    {
        #author_name = $committerName
        #author_email = $committerEmail

        Write-Verbose ("Creating Git Head: ")

        $head = [PSCustomObject]@{
            id = $commitID
            committer_name = $committerName
            committer_email = $committerEmail
            message = $commitMessage
        }

        Write-Verbose $head

        Write-Output $head
    }
}

function Create-Git-Remotes
{
    PROCESS
    {
        Write-Verbose "Creating Git Remotes: "
        $remote = [PSCustomObject]@{
            name = "origin"
            url = $remoteUrl
        }
    
        Write-Verbose $remote

        Write-Output $remote
    }
}

function Post-To_Coveralls
{
    [CmdletBinding()]
    param([Parameter(Mandatory=$true,ValueFromPipeline=$true)] [PSCustomObject]$msgObject)

    PROCESS
    {
        $testConfiguration = ($env:CONFIGURATION + "-" + $env:PLATFORM)

        Write-Verbose "JSON Message: "
        $msgObject | ConvertTo-Json -Depth 2 | Write-Verbose
        $msgObject | ConvertTo-Json -Depth 3 | Out-File ".\coverage.json"
        Push-AppveyorArtifact ".\coverage.json" -DeploymentName $testConfiguration

        $msgObject.repo_token = $ENV:coveralls_token
        $json = $msgObject | ConvertTo-Json -Compress -Depth 3

        $client = New-Object System.Net.Http.Httpclient
        $content = New-Object System.Net.Http.MultipartFormDataContent
        $data = New-Object System.Net.Http.StringContent -ArgumentList @($json, ([System.Text.Encoding]::UTF8), "application/json")

        try
        {
            $content.Add($data, "json_file", "coverage.json")
        
            $response = $client.PostAsync($coverallsApi, $content).Result
            if(-Not $response.IsSuccessStatusCode){
                Write-Error "Error posting to coveralls"
                Write-Error "Result code: " $response.StatusCode
                Write-Error $response.Content.ReadAsStreamAsync().Result
            }
        }
        finally
        {
            $data.Dispose()
            $content.Dispose()
            $client.Dispose()
        }
    }
}

$coberturaFile = [System.IO.Path]::Combine($buildPath, "coverage_results.xml")

if(Test-Path $coberturaFile)
{
    Write-Host ("Loading Coverage File: " + $coberturaFile)

    [xml]$coverage = Get-Content $coberturaFile

    #timestamp from github
    $gitTimestamp = (& git log -1 --format="%ci") | Get-Date
    $runTime = $gitTimestamp.ToUniversalTime()
    $midnight = $runTime.Date
    $gitInfo = Create-Git-Info
    $sourceFiles = @($coverage.coverage.packages | Read-Packages)

    # encode a coveralls build number as: MMDDmmm
    # XX  : number of months after 12/31/2015
    # DD  : Day of current month (utc)
    # mmm : minutes after midnight utc / 2

    $epoch = New-Object System.DateTime -ArgumentList @(2015, 12, 31)
    $months = (($commitTime.Year - $epoch.Year) * 12) + ($commitTime.Month - $epoch.Month)
    $minutes = [Math]::Truncate((($runTime - $midnight).TotalMinutes) / 2)
    $serviceNumber = [System.String]::Format("{0:##}{1:00}{2:000}", $months, $commitTime.Day, $minutes)

    $msg = [PSCustomObject]@{
        repo_token = "[Secure]"
        service_name = "appveyor-custom"
        service_number = $serviceNumber
        service_branch = $branchName
        service_build_url = $buildUrl
        service_job_id = $buildJobID
        service_pull_request = $pullRequestNumber
        commit_sha = $commitID
        run_at = $runTime.ToString("yyyy-MM-dd HH:mm:ss zzz")
        git = $gitInfo
        source_files = $sourceFiles
    }

    if($msg.service_pull_request -eq $null){
        $msg = $msg | Select-Object -Property * -ExcludeProperty service_pull_request
    }

    $msg | Post-To_Coveralls

    Write-Host "Finished Posting to Coveralls"
}
else
{
    Write-Host "No code coverage to report."
}
