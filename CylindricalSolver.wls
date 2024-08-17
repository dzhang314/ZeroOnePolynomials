#!/usr/bin/env wolframscript
(* ::Package:: *)

ImportSystems[path_String] := ToExpression[
	Import[path, "Text"] ~StringSplit~ "\n\n" ~StringSplit~ "\n"];

Do[Print["Reading file: ", filename, "\n"];
	With[{systems = ImportSystems[filename]},
		Do[With[{s = systems[[i]]}, Block[{time, result, mem},
			mem = MaxMemoryUsed[{time, result} = AbsoluteTiming[
				TimeConstrained[CylindricalDecomposition[
					(# == 1)& /@ s, Variables[s]], 0.25]]];
			Which[result === $Aborted,
				Print["Failed to solve system ", i, " of ",
					Length[systems], " from file: ", filename];
				Print["Took ", time, " seconds and ",
					mem, " bytes of memory."];
			result =!= False,
				Print["FOUND COUNTEREXAMPLE!"];
				Print["SYSTEM ", i, " IN FILE: ", filename];
				Print /@ s;
				Quit[],
			True,
				Print["Solved system ", i, " of ",
					Length[systems], " from file: ", filename];
				Print["Took ", time, " seconds and ",
					mem, " bytes of memory."]]]],
			{i, Length[systems]}],
	{filename, FileNames["data/WeaklyCanonizedEquations-*.txt"]}]];
