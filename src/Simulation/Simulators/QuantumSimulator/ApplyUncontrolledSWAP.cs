// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

using System;
using Microsoft.Quantum.Simulation.Core;
using Microsoft.Quantum.Intrinsic.Interfaces;

namespace Microsoft.Quantum.Simulation.Simulators
{
    public partial class QuantumSimulator
    {
        void IIntrinsicApplyUncontrolledSWAP.Body(Qubit qubit1, Qubit qubit2)
        {
            // Issue #44 (https://github.com/microsoft/qsharp-runtime/issues/44)
            // If/when the simulator provides access to the accelerated SWAP functionality,
            // this can be replaced with a call to that instead of MCX.

            this.CheckQubits(new QArray<Qubit>(new Qubit[]{ qubit1, qubit2 }));

            MCX(this.Id, 1, new IntPtr[]{(IntPtr)qubit1.Id}, (IntPtr)qubit2.Id);
            MCX(this.Id, 1, new IntPtr[]{(IntPtr)qubit2.Id}, (IntPtr)qubit1.Id);
            MCX(this.Id, 1, new IntPtr[]{(IntPtr)qubit1.Id}, (IntPtr)qubit2.Id);
        }
    }
}
