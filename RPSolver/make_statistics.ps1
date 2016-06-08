$scenes = 'cornell-move-cone', 'sponza-2-holes', 'conference-simple';
$rpsolver = '..\x64\Debug\RPSolver.exe';
$triesPerScene = 10;

foreach ($scene in $scenes) {
	for($i=1; $i -le $triesPerScene; $i++){
		$outLogFile = ".\examples\$scene-log-$i.txt";
		$outSolutionsFile = ".\examples\$scene-solutions-$i.csv";

		if(Test-Path $outLogFile){
			Write-Host $scene $i "Omitted";
			continue;
		}

		Write-Host $scene $i;
		Invoke-Expression "& $rpsolver .\examples\$scene.xml";
		Move-Item ".\examples\log.txt" $outLogFile;
		Move-Item ".\examples\output\solutions.csv" $outSolutionsFile;
		Write-Host "Cooling down";
		Start-Sleep -s 10;
	}
}