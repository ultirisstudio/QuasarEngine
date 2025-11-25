#pragma once

#include <vector>
#include <cstddef>
#include <utility>

namespace QuasarEngine
{
    template<typename State>
    class UndoStack
    {
    public:
        explicit UndoStack(std::size_t maxSize = 64)
            : m_MaxSize(maxSize)
        {
        }

        void Clear()
        {
            m_Undo.clear();
            m_Redo.clear();
        }

        bool CanUndo() const { return m_Undo.size() > 1; }
        bool CanRedo() const { return !m_Redo.empty(); }

        std::size_t UndoCount() const { return m_Undo.size(); }
        std::size_t RedoCount() const { return m_Redo.size(); }

        void Push(const State& s)
        {
            m_Redo.clear();
            m_Undo.push_back(s);
            Trim();
        }

        void Push(State&& s)
        {
            m_Redo.clear();
            m_Undo.push_back(std::move(s));
            Trim();
        }

        bool Undo(State& out)
        {
            if (m_Undo.size() <= 1)
                return false;

            m_Redo.push_back(std::move(m_Undo.back()));
            m_Undo.pop_back();

            out = m_Undo.back();
            return true;
        }

        bool Redo(State& out)
        {
            if (m_Redo.empty())
                return false;

            m_Undo.push_back(std::move(m_Redo.back()));
            m_Redo.pop_back();

            out = m_Undo.back();
            return true;
        }

    private:
        void Trim()
        {
            if (m_Undo.size() > m_MaxSize)
                m_Undo.erase(m_Undo.begin());
        }

        std::vector<State> m_Undo;
        std::vector<State> m_Redo;
        std::size_t        m_MaxSize;
    };
}