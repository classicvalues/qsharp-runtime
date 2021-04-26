﻿using System;
using System.Collections.Generic;
using System.IO;
using System.Threading.Tasks;
using Microsoft.Quantum.Runtime;

namespace Microsoft.Quantum.EntryPointDriver.Mocks
{
    /// <summary>
    /// A QIR submitter that does nothing.
    /// </summary>
    internal class NoOpQirSubmitter : IQirSubmitter
    {
        /// <summary>
        /// The target ID for the no-op QIR submitter.
        /// </summary>
        internal const string TargetId = "test.qir.noop";

        public string ProviderId => nameof(NoOpQirSubmitter);

        public string Target => TargetId;

        public Task<IQuantumMachineJob> SubmitAsync(Stream qir, string entryPoint, IReadOnlyList<Argument> arguments) =>
            Task.FromResult<IQuantumMachineJob>(new ExampleJob());
    }
}
