#include "qepch.h"

#include "ASTGenerator.h"

namespace QuasarEngine
{
    ASTGenerator::ASTGenerator(const NodeGraph* graph) : graph_(graph) {}

    ASTNode* ASTGenerator::Generate()
    {
        ASTNode* root = nullptr;
        for (const auto& node : graph_->GetNodes())
        {
            if (node.second->IsOutputNode())
            {
                //root = node.second->BuildAST(graph_);
                break;
            }
        }
        return root;
    }
}