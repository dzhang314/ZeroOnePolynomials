#!/usr/bin/env wolframscript
(* ::Package:: *)

ImportSystems[path_String] :=
	ToExpression@StringSplit[StringSplit[Import[path], "\n\n"], "\n"];

Do[
	Print["Reading file ", filename, "..."];
	With[{systems = ImportSystems[filename]},
		Print["Successfully loaded ", Length[systems], " systems."];
		Do[With[{s = systems[[i]]},
			With[{result = AbsoluteTiming[CylindricalDecomposition[(# == 1)& /@ s, Variables[s]]]},
				If[result[[2]] === False,
					Print["Solved system ", i, " of ", Length[systems], " in ", result[[1]], " seconds."],
					Print["FOUND COUNTEREXAMPLE AT SYSTEM ", i, " IN FILE ", filename, "!"];
					Print /@ s;
					Quit[]
				]
			]
		], {i, Length[systems]}]
	],
	{filename, $ScriptCommandLine[[2;;]]}
];
