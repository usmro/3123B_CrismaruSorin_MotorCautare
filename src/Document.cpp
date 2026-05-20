#include "Document.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>

Document::Document(const std::string& caleFisier) : m_caleFisier(caleFisier) {
}

const std::string& Document::obtineCaleFisier() const {
    return m_caleFisier;
}

void Document::proceseazaCuvinte(const std::function<void(const std::string&, int)>& callback) const {
    std::ifstream fisier(m_caleFisier, std::ios::binary);
    if (!fisier.is_open()) {
        std::filesystem::path caleAbsoluta = std::filesystem::absolute(m_caleFisier);
        std::cerr << "Nu s-a putut deschide fisierul: " << caleAbsoluta << std::endl;
        return;
    }

    const size_t bufferSize = 65536; // 64 KB chunk
    std::vector<char> buffer(bufferSize);
    std::string cuvant; // Un "cuvant" este limitat de memoria heap
    int numarLinie = 1;

    while (fisier.read(buffer.data(), bufferSize) || fisier.gcount() > 0) {
        std::streamsize bytes = fisier.gcount();
        for (std::streamsize i = 0; i < bytes; ++i) {
            unsigned char c = buffer[i];
            
            if (c == '\n') {
                if (!cuvant.empty()) {
                    callback(cuvant, numarLinie);
                    cuvant.clear();
                }
                numarLinie++;
            } else if (std::isspace(c)) {
                if (!cuvant.empty()) {
                    callback(cuvant, numarLinie);
                    cuvant.clear();
                }
            } else if (std::isalnum(c)) {
                cuvant.push_back(static_cast<char>(std::tolower(c)));
            }
        }
    }
    
    if (!cuvant.empty()) {
        callback(cuvant, numarLinie);
    }
}
