#include "Canvas/Algorithms/Count.hpp"


namespace canvas {

size_t KernelPsCount(const GraphSP& graph,
                     const Variable::StaticSpecs& specs,
                     const Variable::DynamicFills& fills) {
    size_t sum = 0;
    for (const auto& p: graph->primitives)
        sum += p->PsCount(specs, fills);
    return sum;
}

size_t KernelFLOPsCount(const GraphSP& graph,
                        const Variable::StaticSpecs& specs,
                        const Variable::DynamicFills& fills) {
    size_t sum = 0;
    for (const auto& p: graph->primitives)
        sum += p->FLOPsCount(specs, fills);
    return sum;
}

bool KernelCheckTensorSizeOverflow(const GraphSP& graph,
                                   const Variable::StaticSpecs& specs,
                                   const Variable::DynamicFills& fills) {
    return std::all_of(graph->tensors.begin(), graph->tensors.end(), [=](const TensorSP& t) -> bool {
        size_t size = t->shape.Pi().FillToInteger(specs, fills);
        // A reasonable pre-defined PyTorch resource limit (with batch size 512)
        return size * kPredefinedMaxBatchSize <= std::numeric_limits<int>::max();
    });
}

size_t NetPsCount(const NetSpecsSP& specs,
                  const GraphSP& graph,
                  const HeuristicPreferences& preferences,
                  const NetFillsSP& fills) {
    auto n_kernels = specs->kernel_specs.size();
    size_t sum = 0;
    if (fills == nullptr) {
        assert(graph->DynamicVarCount() == 0);
        for (int i = 0; i < n_kernels; ++ i)
            sum += KernelPsCount(graph, MergeIntoStaticSpecs(preferences, specs->kernel_specs[i]),
                                 Variable::DynamicFills());
    } else {
        assert(fills->Size() == n_kernels);
        for (int i = 0; i < n_kernels; ++ i)
            sum += KernelPsCount(graph, MergeIntoStaticSpecs(preferences, specs->kernel_specs[i]), fills->At(i));
    }
    return sum;
}

size_t NetFLOPsCount(const NetSpecsSP& specs,
                     const GraphSP& graph,
                     const HeuristicPreferences& preferences,
                     const NetFillsSP& fills) {
    auto n_kernels = specs->kernel_specs.size();
    size_t sum = 0;
    if (fills == nullptr) {
        assert(graph->DynamicVarCount() == 0);
        for (int i = 0; i < n_kernels; ++ i)
            sum += KernelFLOPsCount(graph, MergeIntoStaticSpecs(preferences, specs->kernel_specs[i]),
                                    Variable::DynamicFills());
    } else {
        assert(fills->Size() == n_kernels);
        for (int i = 0; i < n_kernels; ++ i)
            sum += KernelFLOPsCount(graph, MergeIntoStaticSpecs(preferences, specs->kernel_specs[i]), fills->At(i));
    }
    return sum;
}

bool NetCheckTensorSizeOverflow(const NetSpecsSP& specs,
                                const GraphSP& graph,
                                const HeuristicPreferences& preferences,
                                const NetFillsSP& fills) {
    auto n_kernels = specs->kernel_specs.size();
    for (int i = 0; i < n_kernels; ++ i) {
        auto kernel_fills = (fills == nullptr ? Variable::DynamicFills() : fills->At(i));
        if (not KernelCheckTensorSizeOverflow(graph, MergeIntoStaticSpecs(preferences, specs->kernel_specs[i]),
                                              kernel_fills))
            return false;
    }
    return true;
}

} // End namespace canvas