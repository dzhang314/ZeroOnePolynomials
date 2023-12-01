#!/usr/bin/env wolframscript
(* ::Package:: *)

ImportSystems[path_String] :=
	ToExpression@StringSplit[StringSplit[Import[path], "\n\n"], "\n"];


Print["Loading data..."];
systems = ImportSystems[$ScriptCommandLine[[2]]];
Print["Successfully loaded ", Length[systems], " systems."];


Do[
	With[
		{s = systems[[i]]},
		If[CylindricalDecomposition[(# == 1)& /@ s, Variables[s]] === False,
			Print["Solved system ", i, " of ", Length[systems], "."],
			Print["FOUND COUNTEREXAMPLE AT SYSTEM ", i, "!"];
			Break[]
		]
	],
	{i, Length[systems]}
];
