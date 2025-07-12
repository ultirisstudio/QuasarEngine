#pragma once

#include <fstream>
#include <iostream>
#include <string>

namespace QuasarEngine
{
    class File
    {
    public:
        File(const std::string& filename) : m_filename(filename) {}

        bool OpenForReading()
        {
            m_ifstream.open(m_filename);
            return m_ifstream.is_open();
        }

        bool OpenForWriting()
        {
            m_ofstream.open(m_filename);
            return m_ofstream.is_open();
        }

        void Write(const std::string& data)
        {
            if (m_ofstream.is_open())
            {
                m_ofstream << data;
            }
        }

        std::string Read()
        {
            std::string data;
            if (m_ifstream.is_open())
            {
                std::getline(m_ifstream, data);
            }
            return data;
        }

        ~File()
        {
            if (m_ifstream.is_open())
            {
                m_ifstream.close();
            }
            if (m_ofstream.is_open())
            {
                m_ofstream.close();
            }
        }

    private:
        std::string m_filename;
        std::ifstream m_ifstream;
        std::ofstream m_ofstream;
    };
}