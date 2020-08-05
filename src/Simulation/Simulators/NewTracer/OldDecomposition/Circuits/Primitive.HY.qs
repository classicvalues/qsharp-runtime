// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Quantum.Simulation.Simulators.NewTracer.OldDecomposition.Circuits
{
    operation HY (target : Qubit) : Unit
    is Adj + Ctl {

        H(target);
        S(target);
    }    
}


