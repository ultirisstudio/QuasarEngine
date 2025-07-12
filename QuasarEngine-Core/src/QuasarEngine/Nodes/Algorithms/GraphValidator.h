#pragma once

#include <QuasarEngine/Nodes/NodeGraph.h>

namespace QuasarEngine
{
    class GraphValidator
    {
    public:
        explicit GraphValidator(const NodeGraph* graph);
        bool HasCycle() const;
        bool ValidateTypes() const;
        std::vector<std::string> GetErrors() const;
    private:
        const NodeGraph* graph_;
    };
}