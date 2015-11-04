$scenes = 'cornell-move-cone', 'sponza-hole', 'conference';
$ilpsolver = '..\x64\Debug\ILPSolver.exe';
$triesPerScene = 10;

foreach ($scene in $scenes) {
	for($i=1; $i -le $triesPerScene; $i++){
		Write-Host $scene $i;
		Invoke-Expression "& $ilpsolver .\examples\$scene.xml";
		Copy-Item ".\examples\log.txt" ".\examples\$scene-$i-log.txt";
	}
}