// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include <bitset>
#include <cassert>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_set>
#include <cstdint>

#include "CoreTypes.hpp"
#include "QirContext.hpp"
#include "QirTypes.hpp"
#include "QirRuntimeApi_I.hpp"
#include "SimFactory.hpp"
#include "SimulatorStub.hpp"

#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this in one cpp file
#include "catch.hpp"

// Identifiers exposed externally:
extern "C" void __quantum__qis__k__body(Qubit q);                    // NOLINT
extern "C" void __quantum__qis__k__ctl(QirArray* controls, Qubit q); // NOLINT

// Used by a couple test simulators. Catch's REQUIRE macro doesn't deal well with static class members so making it
// into a global constant.
constexpr int RELEASED = -1;

using namespace Microsoft::Quantum;
using namespace std;

/*
Forward declared `extern "C"` methods below are implemented in the *.ll files. Most of the files are generated by Q#
compiler, in which case the corresponding *.qs file was used as a source. Some files might have been authored manually
or manually edited.

To update the *.ll files to a newer version:
- enlist and build qsharp-compiler
- find <location1>\qsc.exe and <location2>\QirCore.qs, <location2>\QirTarget.qs built files
- [if different] copy QirCore.qs and QirTarget.qs into the "compiler" folder
- run: qsc.exe build --qir s --build-exe --input name.qs compiler\qircore.qs compiler\qirtarget.qs --proj name
- the generated file name.ll will be placed into `s` folder
*/

struct Array
{
    int64_t itemSize;
    void* buffer;
};

// The function replaces array[index] with value, then creates a new array that consists of every other element up to
// index (starting from index backwards) and every element from index to the end. It returns the sum of elements in this
// new array
extern "C" int64_t Microsoft__Quantum__Testing__QIR__Test_Arrays__Interop( // NOLINT
    Array* array, int64_t index, int64_t val);
TEST_CASE("QIR: Using 1D arrays", "[qir][qir.arr1d]")
{
    QirExecutionContext::Scoped qirctx(nullptr, true /*trackAllocatedObjects*/);

    constexpr int64_t n = 5;
    int64_t values[n]   = {0, 1, 2, 3, 4};
    auto array          = Array{5, values};

    int64_t res = Microsoft__Quantum__Testing__QIR__Test_Arrays__Interop(&array, 2, 42);
    REQUIRE(res == (0 + 42) + (42 + 3 + 4));
}

extern "C" void Microsoft__Quantum__Testing__QIR__TestQubitResultManagement__Interop(); // NOLINT
struct QubitsResultsTestSimulator : public Microsoft::Quantum::SimulatorStub
{
    // no intelligent reuse, we just want to check that QIR releases all qubits
    vector<int> qubits;           // released, or |0>, or |1> states (no entanglement allowed)
    vector<int> results = {0, 1}; // released, or Zero(0) or One(1)

    uint64_t GetQubitId(qubitid_t qubit) const
    {
        const uint64_t id = (uint64_t)qubit;
        REQUIRE(id < this->qubits.size());

        return id;
    }

    uint8_t GetResultId(Result result) const
    {
        const uint8_t id = (uint8_t)(uintptr_t)result;
        REQUIRE(id < this->results.size());

        return id;
    }

    qubitid_t AllocateQubit() override
    {
        qubits.push_back(0);
        return reinterpret_cast<qubitid_t>(this->qubits.size() - 1);
    }

    void ReleaseQubit(qubitid_t qubit) override
    {
        const uint64_t id = GetQubitId(qubit);
        REQUIRE(this->qubits[id] != RELEASED); // no double-release
        this->qubits[id] = RELEASED;
    }

    void X(qubitid_t qubit) override
    {
        const uint64_t id = GetQubitId(qubit);
        REQUIRE(this->qubits[id] != RELEASED); // the qubit must be alive
        this->qubits[id] = 1 - this->qubits[id];
    }

    Result Measure([[maybe_unused]] long numBases, PauliId* /* bases */, long /* numTargets */,
                   qubitid_t targets[]) override
    {
        assert(numBases == 1 && "QubitsResultsTestSimulator doesn't support joint measurements");

        const uint64_t id = GetQubitId(targets[0]);
        REQUIRE(this->qubits[id] != RELEASED); // the qubit must be alive
        this->results.push_back(this->qubits[id]);
        return reinterpret_cast<Result>(this->results.size() - 1);
    }

    bool AreEqualResults(Result r1, Result r2) override
    {
        uint8_t i1 = GetResultId(r1);
        uint8_t i2 = GetResultId(r2);
        REQUIRE(this->results[i1] != RELEASED);
        REQUIRE(this->results[i2] != RELEASED);

        return (results[i1] == results[i2]);
    }

    void ReleaseResult(Result result) override
    {
        uint8_t i = GetResultId(result);
        REQUIRE(this->results[i] != RELEASED); // no double release
        this->results[i] = RELEASED;
    }

    Result UseZero() override
    {
        return reinterpret_cast<Result>(0);
    }

    Result UseOne() override
    {
        return reinterpret_cast<Result>(1);
    }
};
TEST_CASE("QIR: allocating and releasing qubits and results", "[qir][qir.qubit][qir.result]")
{
    unique_ptr<QubitsResultsTestSimulator> sim = make_unique<QubitsResultsTestSimulator>();
    QirExecutionContext::Scoped qirctx(sim.get(), true /*trackAllocatedObjects*/);

    REQUIRE_NOTHROW(Microsoft__Quantum__Testing__QIR__TestQubitResultManagement__Interop());

    // check that all qubits have been released
    for (size_t id = 0; id < sim->qubits.size(); id++)
    {
        INFO(std::string("unreleased qubit: ") + std::to_string(id));
        CHECK(sim->qubits[id] == RELEASED);
    }

    // check that all results, allocated by measurements have been released
    // TODO: enable after https://github.com/microsoft/qsharp-compiler/issues/780 is fixed
    // for (size_t id = 2; id < sim->results.size(); id++)
    // {
    //     INFO(std::string("unreleased results: ") + std::to_string(id));
    //     CHECK(sim->results[id] == RELEASED);
    // }
}

#ifdef _WIN32
// A non-sensical function that creates a 3D array with given dimensions, then projects on the index = 1 of the
// second dimension and returns a function of the sizes of the dimensions of the projection and a the provided value,
// that is written to the original array at [1,1,1] and then retrieved from [1,1].
// Thus, all three dimensions must be at least 2.
extern "C" int64_t TestMultidimArrays(char value, int64_t dim0, int64_t dim1, int64_t dim2);
TEST_CASE("QIR: multidimensional arrays", "[qir][qir.arrMultid]")
{
    QirExecutionContext::Scoped qirctx(nullptr, true /*trackAllocatedObjects*/);

    REQUIRE(42 + (2 + 8) / 2 == TestMultidimArrays(42, 2, 4, 8));
    REQUIRE(17 + (3 + 7) / 2 == TestMultidimArrays(17, 3, 5, 7));
}
#else // not _WIN32
// TODO: The bridge for variadic functions is broken on Linux!
#endif

// Manually authored QIR to test dumping range [0..2..6] into string and then raising a failure with it
extern "C" void TestFailWithRangeString(int64_t start, int64_t step, int64_t end);
TEST_CASE("QIR: Report range in a failure message", "[qir][qir.range]")
{
    QirExecutionContext::Scoped qirctx(nullptr, true /*trackAllocatedObjects*/);

    bool failed = false;
    try
    {
        TestFailWithRangeString(0, 5, 42); // Returns with exception. Leaks the instances created from the moment of
                                           // call to the moment of exception throw.
                                           // TODO: Extract into a separate file compiled with leaks check off.
    }
    catch (const std::exception& e)
    {
        failed = true;
        REQUIRE(std::string(e.what()) == "0..5..42");
    }
    REQUIRE(failed);
}

// TestPartials subtracts the second argument from the first and returns the result.
extern "C" int64_t Microsoft__Quantum__Testing__QIR__TestPartials__Interop(int64_t, int64_t); // NOLINT
TEST_CASE("QIR: Partial application of a callable", "[qir][qir.partCallable]")
{
    QirExecutionContext::Scoped qirctx(nullptr, true /*trackAllocatedObjects*/);

    const int64_t res = Microsoft__Quantum__Testing__QIR__TestPartials__Interop(42, 17);
    REQUIRE(res == 42 - 17);
}

// The Microsoft__Quantum__Testing__QIR__TestFunctors__Interop tests needs proper semantics of X and M, and nothing
// else. The validation is done inside the test and it would throw in case of failure.
struct FunctorsTestSimulator : public Microsoft::Quantum::SimulatorStub
{
    std::vector<int> qubits;

    uint64_t GetQubitId(qubitid_t qubit) const
    {
        const uint64_t id = (uint64_t)qubit;
        REQUIRE(id < this->qubits.size());
        return id;
    }

    qubitid_t AllocateQubit() override
    {
        this->qubits.push_back(0);
        return reinterpret_cast<qubitid_t>(this->qubits.size() - 1);
    }

    void ReleaseQubit(qubitid_t qubit) override
    {
        const uint64_t id = GetQubitId(qubit);
        REQUIRE(this->qubits[id] != RELEASED);
        this->qubits[id] = RELEASED;
    }

    void X(qubitid_t qubit) override
    {
        const uint64_t id = GetQubitId(qubit);
        REQUIRE(this->qubits[id] != RELEASED); // the qubit must be alive
        this->qubits[id] = 1 - this->qubits[id];
    }

    void ControlledX(long numControls, qubitid_t controls[], qubitid_t qubit) override
    {
        for (long i = 0; i < numControls; i++)
        {
            const uint64_t id = GetQubitId(controls[i]);
            REQUIRE(this->qubits[id] != RELEASED);
            if (this->qubits[id] == 0)
            {
                return;
            }
        }
        X(qubit);
    }

    Result Measure([[maybe_unused]] long numBases, PauliId* /* bases */, long /* numTargets */,
                   qubitid_t targets[]) override
    {
        assert(numBases == 1 && "FunctorsTestSimulator doesn't support joint measurements");

        const uint64_t id = GetQubitId(targets[0]);
        REQUIRE(this->qubits[id] != RELEASED);
        return reinterpret_cast<Result>(this->qubits[id]);
    }

    bool AreEqualResults(Result r1, Result r2) override
    {
        // those are bogus pointers but it's ok to compare them _as pointers_
        return (r1 == r2);
    }

    void ReleaseResult(Result /*result*/) override
    {
    } // the results aren't allocated by this test simulator

    Result UseZero() override
    {
        return reinterpret_cast<Result>(0);
    }

    Result UseOne() override
    {
        return reinterpret_cast<Result>(1);
    }
};
static FunctorsTestSimulator* g_ctrqapi = nullptr;
static int g_cKCalls                    = 0;
static int g_cKCallsControlled          = 0;
extern "C" void Microsoft__Quantum__Testing__QIR__TestFunctors__Interop();       // NOLINT
extern "C" void Microsoft__Quantum__Testing__QIR__TestFunctorsNoArgs__Interop(); // NOLINT
extern "C" void __quantum__qis__k__body(Qubit q)                                 // NOLINT
{
    g_cKCalls++;
    g_ctrqapi->X(QubitToQubitId(q));
}
extern "C" void __quantum__qis__k__ctl(QirArray* controls, Qubit q) // NOLINT
{
    g_cKCallsControlled++;
    g_ctrqapi->ControlledX((long)(controls->count), BufferAsArrayOfQubitIds(controls->buffer), QubitToQubitId(q));
}
TEST_CASE("QIR: application of nested controlled functor", "[qir][qir.functor]")
{
    unique_ptr<FunctorsTestSimulator> qapi = make_unique<FunctorsTestSimulator>();
    QirExecutionContext::Scoped qirctx(qapi.get(), true /*trackAllocatedObjects*/);
    g_ctrqapi = qapi.get();

    CHECK_NOTHROW(Microsoft__Quantum__Testing__QIR__TestFunctors__Interop());

    const int cKCalls           = g_cKCalls;
    const int cKCallsControlled = g_cKCallsControlled;
    CHECK_NOTHROW(Microsoft__Quantum__Testing__QIR__TestFunctorsNoArgs__Interop());
    CHECK(g_cKCalls - cKCalls == 3);
    CHECK(g_cKCallsControlled - cKCallsControlled == 5);

    g_ctrqapi = nullptr;
}
