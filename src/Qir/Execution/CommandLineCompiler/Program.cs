﻿// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

using System;
using System.CommandLine;
using System.CommandLine.Builder;
using System.CommandLine.Help;
using System.CommandLine.Invocation;
using System.CommandLine.Parsing;
using System.IO;
using System.Text;
using System.Threading.Tasks;
using Microsoft.Quantum.Qir.Tools;

namespace CommandLineCompiler
{
    class Program
    {
        private static async Task<int> Main(string[] args)
        {
            var buildCommand = CreateBuildCommand();

            var root = new RootCommand() { buildCommand };
            SetSubCommandAsDefault(root, buildCommand);
            root.Description = "Command-line tool for processing QIR DLL files.";
            root.TreatUnmatchedTokensAsErrors = true;

            Console.OutputEncoding = Encoding.UTF8;
            return await new CommandLineBuilder(root)
                .UseDefaults()
                .UseHelpBuilder(context => new QsHelpBuilder(context.Console))
                .Build()
                .InvokeAsync(args);
        }

        /// <summary>
        /// Creates the Build command for the command line compiler, which is used to
        /// compile the executables from a given QIR DLL.
        /// </summary>
        /// <returns>The Build command.</returns>
        private static Command CreateBuildCommand()
        {
            var buildCommand = new Command("build", "(default) Build the executables from a QIR DLL.")
            {
                Handler = CommandHandler.Create((BuildOptions settings) =>
                {
                    if (settings.LibraryDirectories.Length < 1)
                    {
                        throw new ArgumentException("The '--libraryDirectories' option requires at least one argument.");
                    }
                    return QirTools.BuildFromQSharpDll(settings.QsharpDll, settings.LibraryDirectories[0], settings.IncludeDirectory, settings.ExecutablesDirectory);
                })
            };
            buildCommand.TreatUnmatchedTokensAsErrors = true;

            Option<FileInfo> qsharpDllOption = new Option<FileInfo>(
                aliases: new string[] { "--qsharpDll", "--dll" },
                description: "The path to the .NET DLL file generated by the Q# compiler.")
            {
                Required = true
            };
            buildCommand.AddOption(qsharpDllOption);

            Option<DirectoryInfo[]> libraryDirectories = new Option<DirectoryInfo[]>(
                aliases: new string[] { "--libraryDirectories", "--lib" },
                description: "One or more paths to the directories containing the libraries to be linked.")
            {
                Required = true
            };
            buildCommand.AddOption(libraryDirectories);

            Option<DirectoryInfo> includeDirectory = new Option<DirectoryInfo>(
                aliases: new string[] { "--includeDirectory", "--include" },
                description: "The path to the directory containing the headers required for compilation.")
            {
                Required = true
            };
            buildCommand.AddOption(includeDirectory);

            Option<DirectoryInfo> executablesDirectory = new Option<DirectoryInfo>(
                aliases: new string[] { "--executablesDirectory", "--exe" },
                description: "The path to the output directory where the created executables will be placed.")
            {
                Required = true
            };
            buildCommand.AddOption(executablesDirectory);

            return buildCommand;
        }

        /// <summary>
        /// Copies the handle and options from the given sub command to the given command.
        /// </summary>
        /// <param name="root">The command whose handle and options will be set.</param>
        /// <param name="subCommand">The sub command that will be copied from.</param>
        private static void SetSubCommandAsDefault(Command root, Command subCommand)
        {
            root.Handler = subCommand.Handler;
            foreach (var option in subCommand.Options)
            {
                root.AddOption(option);
            }
        }

        /// <summary>
        /// A modification of the command-line <see cref="HelpBuilder"/> class.
        /// </summary>
        private sealed class QsHelpBuilder : HelpBuilder
        {
            /// <summary>
            /// Creates a new help builder using the given console.
            /// </summary>
            /// <param name="console">The console to use.</param>
            internal QsHelpBuilder(IConsole console) : base(console)
            {
            }

            protected override string ArgumentDescriptor(IArgument argument)
            {
                // Hide long argument descriptors.
                var descriptor = base.ArgumentDescriptor(argument);
                return descriptor.Length > 30 ? argument.Name : descriptor;
            }
        }

        /// <summary>
        /// A class for encapsulating the different options for the build command.
        /// </summary>
        public sealed class BuildOptions
        {
            /// <summary>
            /// The path to the .NET DLL file generated by the Q# compiler.
            /// </summary>
            public FileInfo QsharpDll { get; set; }

            /// <summary>
            /// One or more paths to the directories containing the libraries to be linked.
            /// </summary>
            public DirectoryInfo[] LibraryDirectories { get; set; }

            /// <summary>
            /// The path to the directory containing the headers required for compilation.
            /// </summary>
            public DirectoryInfo IncludeDirectory { get; set; }

            /// <summary>
            /// The path to the output directory where the created executables will be placed.
            /// </summary>
            public DirectoryInfo ExecutablesDirectory { get; set; }
        }
    }
}
