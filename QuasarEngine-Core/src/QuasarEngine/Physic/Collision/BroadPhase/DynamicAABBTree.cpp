#include "qepch.h"
#include "DynamicAABBTree.h"
#include "QuasarEngine/Physic/Collision/Collider.h"
#include <limits>
#include <cassert>

namespace QuasarEngine {

    DynamicAABBTree::DynamicAABBTree() {}
    DynamicAABBTree::~DynamicAABBTree() {}

    int DynamicAABBTree::AllocateNode() {
        int id;
        if (!freeList.empty()) {
            id = freeList.back();
            freeList.pop_back();
        }
        else {
            id = nextNodeID++;
        }

        nodes[id] = Node();
        nodes[id].id = id;
        return id;
    }

    void DynamicAABBTree::DeallocateNode(int id) {
        nodes.erase(id);
        freeList.push_back(id);
    }

    int DynamicAABBTree::Insert(Collider* collider, const AABB& aabb) {
        int id = AllocateNode();

        Node& node = nodes[id];
        node.box = aabb;
        node.collider = collider;
        node.height = 0;
        node.left = node.right = -1;

        InsertLeaf(id);
        return id;
    }

    void DynamicAABBTree::Remove(int id) {
        if (nodes.find(id) != nodes.end()) {
            RemoveLeaf(id);
            DeallocateNode(id);
        }
    }

    void DynamicAABBTree::Update(int id, const AABB& newAABB) {
        if (nodes.find(id) == nodes.end()) return;

        Node& node = nodes[id];
        if (node.box.Overlaps(newAABB)) return;

        RemoveLeaf(id);
        node.box = newAABB;
        InsertLeaf(id);
    }

    void DynamicAABBTree::InsertLeaf(int leaf) {
        if (root == -1) {
            root = leaf;
            nodes[root].parent = -1;
            return;
        }

        AABB leafBox = nodes[leaf].box;
        int index = root;

        while (!nodes[index].isLeaf()) {
            int left = nodes[index].left;
            int right = nodes[index].right;

            AABB combined = Combine(nodes[index].box, leafBox);
            float cost = 2.0f * GetSurfaceArea(combined);

            float inheritance = 2.0f * (GetSurfaceArea(combined) - GetSurfaceArea(nodes[index].box));

            float costLeft;
            if (nodes[left].isLeaf()) {
                AABB aabb = Combine(leafBox, nodes[left].box);
                costLeft = GetSurfaceArea(aabb) + inheritance;
            }
            else {
                AABB aabb = Combine(leafBox, nodes[left].box);
                costLeft = GetSurfaceArea(aabb) - GetSurfaceArea(nodes[left].box) + inheritance;
            }

            float costRight;
            if (nodes[right].isLeaf()) {
                AABB aabb = Combine(leafBox, nodes[right].box);
                costRight = GetSurfaceArea(aabb) + inheritance;
            }
            else {
                AABB aabb = Combine(leafBox, nodes[right].box);
                costRight = GetSurfaceArea(aabb) - GetSurfaceArea(nodes[right].box) + inheritance;
            }

            index = (costLeft < costRight) ? left : right;
        }

        int sibling = index;
        int oldParent = nodes[sibling].parent;
        int newParent = AllocateNode();

        nodes[newParent].parent = oldParent;
        nodes[newParent].box = Combine(leafBox, nodes[sibling].box);
        nodes[newParent].height = nodes[sibling].height + 1;
        nodes[newParent].left = sibling;
        nodes[newParent].right = leaf;
        nodes[newParent].collider = nullptr;

        nodes[sibling].parent = newParent;
        nodes[leaf].parent = newParent;

        if (oldParent == -1) {
            root = newParent;
        }
        else {
            if (nodes[oldParent].left == sibling)
                nodes[oldParent].left = newParent;
            else
                nodes[oldParent].right = newParent;
        }

        Refit(nodes[leaf].parent);
    }

    void DynamicAABBTree::RemoveLeaf(int leaf) {
        if (leaf == root) {
            root = -1;
            return;
        }

        int parent = nodes[leaf].parent;
        int grandParent = nodes[parent].parent;
        int sibling = (nodes[parent].left == leaf) ? nodes[parent].right : nodes[parent].left;

        if (grandParent == -1) {
            root = sibling;
            nodes[sibling].parent = -1;
        }
        else {
            if (nodes[grandParent].left == parent)
                nodes[grandParent].left = sibling;
            else
                nodes[grandParent].right = sibling;

            nodes[sibling].parent = grandParent;
        }

        DeallocateNode(parent);
        Refit(grandParent);
    }

    void DynamicAABBTree::Refit(int nodeID) {
        while (nodeID != -1) {
            Node& node = nodes[nodeID];
            int left = node.left;
            int right = node.right;

            node.box = Combine(nodes[left].box, nodes[right].box);
            node.height = 1 + std::max(nodes[left].height, nodes[right].height);

            nodeID = node.parent;
        }
    }

    void DynamicAABBTree::ComputePairs(std::vector<std::pair<Collider*, Collider*>>& result) const {
        if (root == -1) return;

        std::vector<int> stack;
        stack.push_back(root);

        while (!stack.empty()) {
            int nodeID = stack.back();
            stack.pop_back();

            const Node& node = nodes.at(nodeID);
            if (!node.isLeaf()) {
                stack.push_back(node.left);
                stack.push_back(node.right);
            }
            else {
                for (const auto& [otherID, other] : nodes) {
                    if (nodeID == otherID || !other.isLeaf()) continue;

                    if (node.box.Overlaps(other.box)) {
                        result.emplace_back(node.collider, other.collider);
                    }
                }
            }
        }
    }

    void DynamicAABBTree::Query(const AABB& aabb, std::vector<Collider*>& result) const {
        if (root == -1) return;

        std::vector<int> stack;
        stack.push_back(root);

        while (!stack.empty()) {
            int nodeID = stack.back();
            stack.pop_back();

            const Node& node = nodes.at(nodeID);
            if (!node.box.Overlaps(aabb)) continue;

            if (node.isLeaf()) {
                result.push_back(node.collider);
            }
            else {
                stack.push_back(node.left);
                stack.push_back(node.right);
            }
        }
    }

    AABB DynamicAABBTree::Combine(const AABB& a, const AABB& b) const {
        return AABB(glm::min(a.min, b.min), glm::max(a.max, b.max));
    }

    float DynamicAABBTree::GetSurfaceArea(const AABB& aabb) const {
        glm::vec3 d = aabb.max - aabb.min;
        return 2.0f * (d.x * d.y + d.x * d.z + d.y * d.z);
    }

}
