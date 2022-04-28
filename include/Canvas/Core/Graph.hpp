#pragma once

#include <boost/container/small_vector.hpp>
#include <boost/range/join.hpp>
#include <fstream>
#include <iostream>
#include <map>
#include <optional>
#include <sstream>

#include "Canvas/Core/NetSpecs.hpp"
#include "Canvas/Core/Primitive.hpp"
#include "Canvas/Core/Preferences.hpp"
#include "Canvas/Core/Tensor.hpp"
#include "Canvas/Primitives/Input.hpp"


namespace canvas {

namespace bc = boost::container;
namespace br = boost::range;

struct Graph;
typedef std::shared_ptr<Graph> GraphSP;

/// Kernel graph
struct Graph {
    static constexpr int kPreservedNumStructures = 8;

#ifdef CANVAS_ENABLE_BOOST_STYLE_SMALL_VECTOR
    typedef bc::small_vector<TensorSP, kPreservedNumStructures> TensorArray;
    typedef bc::small_vector<PrimitiveSP, kPreservedNumStructures> PrimitiveArray;
    typedef bc::small_vector<size_t, kPreservedNumStructures> SizeTArray;
#else
    typedef std::vector<TensorSP> TensorArray;
    typedef std::vector<PrimitiveSP> PrimitiveArray;
    typedef std::vector<size_t> SizeTArray;
#endif

    /// Topology
    TensorSP in;
    TensorArray tensors;
    PrimitiveArray primitives; // This array must be in topological order

    /// Hash cache
    bool hash_cached = false;
    size_t hash_value = 0;

    Graph() {
        auto p = std::make_shared<InputPrimitive>();
        Apply(p);
        in = p->outs[0];
    }

    Graph(const Graph& rhs);

    ~Graph();

    void LegalityCheck() const;

    [[nodiscard]] bool AlgebraCheck(const Variable::StaticSpecs& specs,
                                    const Variable::DynamicFills& fills=Variable::DynamicFills()) const;

    [[nodiscard]] bool IsTopologicalFinished() const;

    template <typename PrimitiveType>
    [[nodiscard]] int PrimitiveCount() const {
        int n = 0;
        for (const auto& p: primitives)
            n += (DynamicCast<PrimitiveType>(p) != nullptr);
        return n;
    }

    [[nodiscard]] int DynamicVarCount() const;

    [[nodiscard]] std::optional<int> NextUnusedDynamicVarIndex() const;

    [[nodiscard]] GraphSP Copy() const { return std::make_shared<Graph>(*this); }

    [[nodiscard]] TensorSP In() const { return in; }

    /// Return the only tensor with no consumers (null if not only)
    [[nodiscard]] TensorSP Out() const;

    /// Return tensors with no consumers
    [[nodiscard]] TensorArray Outs() const;

    /// Return the number of tensors of which the out degree is zero
    [[nodiscard]] int Width() const;

    size_t Hash() { return hash_cached ? hash_value : CalculateHash(); }

    size_t CalculateSubgraphHash(const TensorSP& t, SizeTArray& cache);

    size_t CalculateHash();

    Variable::DynamicFills GetMinimumFills(const Variable::StaticSpecs& specs);

    void SolveDynamicVar(const VarSolution& s);

    /// Apply a primitive in the current graph (later can not be applied in another graph)
    void Apply(const PrimitiveSP& p);

    void Apply(const PrimitiveApply& pa);

    void ApplyOutput();

    [[nodiscard]] PrimitiveSP RemapPrimitive(const PrimitiveSP& p) const;

    [[nodiscard]] std::pair<GraphSP, PrimitiveSP> CopyAndApply(const PrimitiveSP& p) const;

    [[nodiscard]] std::pair<GraphSP, PrimitiveApply> CopyAndApply(const PrimitiveApply& pa) const;
};

} // End namespace canvas

CanvasHashTemplate(canvas::GraphSP, ->Hash());
