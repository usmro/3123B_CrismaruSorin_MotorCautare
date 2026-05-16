#include "Index.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <fstream>

Index::Index(const std::string& stopwordsPath) {
    incarcaStopwords(stopwordsPath);
}

void Index::incarcaStopwords(const std::string& caleFisier) {
    if (caleFisier.empty()) {
        return;
    }

    std::ifstream fisier(caleFisier);
    if (!fisier.is_open()) {
        std::filesystem::path caleAbsoluta = std::filesystem::absolute(caleFisier);
        std::cerr << "Nu s-a putut deschide fisierul de stopwords: " << caleAbsoluta << std::endl;
        return;
    }

    std::string cuvant;
    while (fisier >> cuvant) {
        m_stopwords.insert(normalizeazaCuvant(cuvant));
    }
}

void Index::incarcaDocumenteDinDirector(const std::string& caleDirector) {
    m_documente.clear();

    try {
        for (const auto& entry : std::filesystem::directory_iterator(caleDirector)) {
            if (!entry.is_regular_file()) {
                continue;
            }

            const std::string numeFisier = entry.path().filename().string();
            if (!numeFisier.empty() && numeFisier[0] == '.') {
                continue;
            }

            const std::string caleFisier = entry.path().string();
            if (esteFisierText(caleFisier)) {
                m_documente.emplace_back(caleFisier);
            }
        }
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Eroare la citirea directorului: " << e.what() << std::endl;
    }
}

void Index::construiesteIndex() {
    m_index.clear();

    for (size_t docId = 0; docId < m_documente.size(); ++docId) {
        const auto& document = m_documente[docId];
        document.proceseazaCuvinte([this, docId](const std::string& cuvant, int numarLinie) {
            if (!cuvant.empty() && !m_stopwords.count(cuvant)) {
                auto& doc_list = m_index[cuvant];
                if (doc_list.empty() || doc_list.back().first != docId) {
                    doc_list.push_back({docId, {numarLinie}});
                } else {
                    auto& vector_linii = doc_list.back().second;
                    if (vector_linii.back() != numarLinie) {
                        vector_linii.push_back(numarLinie);
                    }
                }
            }
        });
    }
}

std::unordered_map<std::string, std::unordered_map<std::string, std::vector<int>>> Index::cauta(const std::string& query) {
    std::istringstream iss(query);
    std::string token;
    std::vector<std::string> cuvinte;
    std::string op = "AND"; // Default la AND

    // Folosim OR?
    if (query.find(" OR ") != std::string::npos) {
        op = "OR";
    }

    while (iss >> token) {
        if (token != "AND" && token != "OR") {
            cuvinte.push_back(token);
        }
    }

    std::unordered_map<std::string, std::unordered_map<std::string, std::vector<int>>> rezultatFinal;

    if (cuvinte.empty()) {
        return {};
    }

    if (cuvinte.size() == 1) {
        auto rezultatCuvant = cautaUnSingurCuvant(cuvinte[0]);
        if (!rezultatCuvant.empty()) {
            rezultatFinal[cuvinte[0]] = rezultatCuvant;
        }
        return rezultatFinal;
    }

    if (op == "OR") {
        for (const auto& cuvant : cuvinte) {
            auto rezultatCuvant = cautaUnSingurCuvant(cuvant);
            if (!rezultatCuvant.empty()) {
                 rezultatFinal[cuvant] = rezultatCuvant;
            }
        }
    } else { // AND
        // Gasim documente ce contin toate cuvintele
        // Afisam fiecare linie pe care se afla fiecare cuvant
        std::unordered_map<std::string, std::vector<int>> documenteComune;
        bool primulCuvant = true;

        for (const auto& cuvant : cuvinte) {
            auto rezultatCuvant = cautaUnSingurCuvant(cuvant);
            if (primulCuvant) {
                documenteComune = rezultatCuvant;
                primulCuvant = false;
            } else {
                std::unordered_map<std::string, std::vector<int>> tempDocs;
                for (auto const& [doc, linii] : documenteComune) {
                    if (rezultatCuvant.count(doc)) {
                        tempDocs[doc] = {}; // Populam liniile mai tarziu
                    }
                }
                documenteComune = tempDocs;
            }
        }

        if (!documenteComune.empty()) {
            for (const auto& cuvant : cuvinte) {
                auto rezultatCuvant = cautaUnSingurCuvant(cuvant);
                for (auto const& [doc, linii] : documenteComune) {
                     if (rezultatCuvant.count(doc)) {
                        rezultatFinal[cuvant][doc] = rezultatCuvant.at(doc);
                    }
                }
            }
        }
    }

    return rezultatFinal;
}

std::unordered_map<std::string, std::vector<int>> Index::cautaUnSingurCuvant(const std::string& cuvant) {
    const std::string cuvantNormalizat = normalizeazaCuvant(cuvant);
    if (m_stopwords.count(cuvantNormalizat)) {
        return {};
    }
    const auto it = m_index.find(cuvantNormalizat);

    std::stringstream logMessage;
    logMessage << "Cautare pentru '" << cuvant << "': ";

    std::unordered_map<std::string, std::vector<int>> rezultateFormatate;

    if (it == m_index.end() || it->second.empty()) {
        logMessage << "Niciun document gasit.";
        notifica(logMessage.str());
        return rezultateFormatate;
    }

    for (const auto& pair : it->second) {
        std::string numeDoc = m_documente[pair.first].obtineCaleFisier();
        rezultateFormatate[numeDoc] = pair.second;
        
        logMessage << "Gasit in " << numeDoc << " (linii: ";
        for (size_t i = 0; i < pair.second.size(); ++i) {
            logMessage << pair.second[i] << (i < pair.second.size() - 1 ? ", " : "");
        }
        logMessage << "). ";
    }
    
    notifica(logMessage.str());
    return rezultateFormatate;
}

std::string Index::normalizeazaCuvant(const std::string& cuvant) {
    std::string rezultat;
    rezultat.reserve(cuvant.size());

    for (const unsigned char caracter : cuvant) {
        if (std::isalnum(caracter)) {
            rezultat.push_back(static_cast<char>(std::tolower(caracter)));
        }
    }

    return rezultat;
}

bool Index::esteFisierText(const std::string& caleFisier) {
    const std::filesystem::path path(caleFisier);
    return path.extension() == ".txt";
}

std::vector<Document>& Index::obtineDocumente() {
    return m_documente;
}
