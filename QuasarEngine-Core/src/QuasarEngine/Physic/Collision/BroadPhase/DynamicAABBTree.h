#pragma once

#include <unordered_map>
#include <vector>
#include <glm/glm.hpp>
#include "QuasarEngine/Physic/Collision/AABB.h"

namespace QuasarEngine {

    class Collider;

    class DynamicAABBTree {
    public:
        DynamicAABBTree();
        ~DynamicAABBTree();

        int Insert(Collider* collider, const AABB& aabb);
        void Remove(int id);
        void Update(int id, const AABB& newAABB);
        void ComputePairs(std::vector<std::pair<Collider*, Collider*>>& result) const;
        void Query(const AABB& aabb, std::vector<Collider*>& result) const;

    private:
        struct Node {
            int id = -1;
            AABB box;
            int parent = -1;
            int left = -1;
            int right = -1;
            int height = 0;
            Collider* collider = nullptr;
            bool isLeaf() const { return left == -1; }
        };

        int root = -1;
        int nextNodeID = 0;

        std::unordered_map<int, Node> nodes;
        std::vector<int> freeList;

        int AllocateNode();
        void DeallocateNode(int id);

        void InsertLeaf(int leaf);
        void RemoveLeaf(int leaf);
        void Refit(int nodeID);
        int Balance(int nodeID);
        AABB Combine(const AABB& a, const AABB& b) const;
        float GetSurfaceArea(const AABB& aabb) const;
    };

}