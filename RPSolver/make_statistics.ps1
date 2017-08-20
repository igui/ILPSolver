$scenes = 'sponza-2-holes';
$rpsolver = '..\x64\Debug\RPSolver.exe';
$triesPerScene = 5;

foreach ($scene in $scenes) {
	for($i=1; $i -le $triesPerScene; $i++){
		$outLogFile = ".\examples\$scene-log-$i.txt";
		$outSolutionsFile = ".\examples\$scene-solutions-$i.csv";
		$outImageZip = ".\examples\$scene-solutions-images-$i.zip";

		if(Test-Path $outLogFile){
			Write-Host $scene $i "Omitted";
			continue;
		}

		Write-Host $scene $i;
		Invoke-Expression "& $rpsolver .\examples\$scene.xml";
		Move-Item ".\examples\log.txt" $outLogFile;
		Move-Item ".\examples\output\solutions.csv" $outSolutionsFile;
		Compress-Archive -Path ".\examples\output\evaluation-solution-*.png" -DestinationPath $outImageZip -CompressionLevel NoCompression;

		Write-Host "Cooling down";
		Start-Sleep -s 10;
	}
} 