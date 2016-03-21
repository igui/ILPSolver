$scenes = 'cornell-move-cone', 'sponza-hole', 'conference-simple';
$ilpsolver = '..\x64\Debug\ILPSolver.exe';
$triesPerScene = 10;

foreach ($scene in $scenes) {
	for($i=1; $i -le $triesPerScene; $i++){
		$outLogFile = ".\examples\$scene-$i-log.txt";

		if(Test-Path $outLogFile){
			Write-Host $scene $i "Omitted";
			continue;
		}

		Write-Host $scene $i;
		Invoke-Expression "& $ilpsolver .\examples\$scene.xml";
		Move-Item ".\examples\log.txt" $outLogFile;
		Write-Host "Cooling down";
		Start-Sleep -s 10;
	}
}