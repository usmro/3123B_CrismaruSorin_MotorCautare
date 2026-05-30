#include "Index.h"
#include "Logger.h"
#include "SearchResults.h"

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include <limits.h>
#include <unistd.h>

namespace {

constexpr const char* CULOARE_RESET = "\033[0m";
constexpr const char* CULOARE_CYAN = "\033[36m";
constexpr const char* CULOARE_CYAN_BOLD = "\033[1;36m";
constexpr const char* CULOARE_GALBEN = "\033[33m";

std::filesystem::path obtineDirectorExecutabil() {
    char buffer[PATH_MAX + 1] = {0};
    const ssize_t lungime = ::readlink("/proc/self/exe", buffer, PATH_MAX);
    if (lungime <= 0) {
        return std::filesystem::current_path();
    }

    return std::filesystem::path(std::string(buffer, static_cast<size_t>(lungime))).parent_path();
}

std::string obtineDirectorImplicitCautare() {
    const std::filesystem::path directorExecutabil = obtineDirectorExecutabil();
    const std::filesystem::path directorProiect = directorExecutabil.parent_path().empty()
        ? directorExecutabil
        : directorExecutabil.parent_path();

    return directorProiect.string();
}

} // namespace

void afiseazaMeniu() {
    std::cout << "\n" << CULOARE_CYAN_BOLD << "=== Motor de Cautare CLI ===" << CULOARE_RESET << std::endl;
    std::cout << CULOARE_CYAN << "1. Schimba directorul de cautare" << CULOARE_RESET << std::endl;
    std::cout << CULOARE_CYAN << "2. Vezi documentele indexate" << CULOARE_RESET << std::endl;
    std::cout << CULOARE_CYAN << "3. Cauta cuvinte" << CULOARE_RESET << std::endl;
    std::cout << CULOARE_CYAN << "4. Iesire" << CULOARE_RESET << std::endl;
    std::cout << CULOARE_CYAN_BOLD << "Alegeti o optiune: " << CULOARE_RESET;
}

int main(int argc, char* argv[]) {
    const std::string directorImplicit = obtineDirectorImplicitCautare();
    const std::filesystem::path caleStopwords = std::filesystem::path(directorImplicit) / "stopwords.txt";

    Logger logger((std::filesystem::path(directorImplicit) / "log.txt").string());
    Index index(caleStopwords.string());
    index.adaugaObserver(&logger);

    std::string directorCautare = directorImplicit;
    if (argc > 1 && argv[1] != nullptr && std::string(argv[1]).size() > 0) {
        directorCautare = argv[1];
    } else {
        std::cout << "Director de cautare [" << directorImplicit << "]: ";
        std::string input;
        std::getline(std::cin, input);
        if (!input.empty()) {
            directorCautare = input;
        }
    }

    std::cout << "Se încarcă documentele din '" << directorCautare << "' și se construiește indexul..." << std::endl;
    index.incarcaDocumenteDinDirector(directorCautare);
    index.construiesteIndex();
    std::cout << index.obtineDocumente().size() << " " << "documente indexate." << std::endl;

    bool ruleaza = true;
    while (ruleaza) {
        std::cout << "\n" << CULOARE_GALBEN << "Director curent: " << CULOARE_RESET << directorCautare << std::endl;
        afiseazaMeniu();
        
        std::string optiune;
        std::getline(std::cin, optiune);

        std::cout << "\n" << CULOARE_CYAN_BOLD << "=== Motor de Cautare CLI ===" << CULOARE_RESET << std::endl;

        if (optiune == "1") {
            std::cout << "Introduceti noua cale (sau lasati gol pentru a anula): ";
            std::string nouaCale;
            std::getline(std::cin, nouaCale);
            if (!nouaCale.empty()) {
                directorCautare = nouaCale;
                std::cout << "Se reincarca documentele din '" << directorCautare << "'..." << std::endl;
                index.incarcaDocumenteDinDirector(directorCautare);
                index.construiesteIndex();
                std::cout << index.obtineDocumente().size() << " " << "documente indexate." << std::endl;
            }
        } else if (optiune == "2") {
            const auto& documente = index.obtineDocumente();
            std::cout << "\nDocumente indexate (" << documente.size() << "):" << std::endl;
            for (const auto& doc : documente) {
                std::cout << "- " << doc.obtineCaleFisier() << std::endl;
            }
        } else if (optiune == "3") {
            std::cout << "\nIntroduceți interogarea de căutare (sau lasati gol pentru a anula): ";
            std::string query;
            std::getline(std::cin, query);

            if (query.empty()) {
                continue;
            }

            const auto cuvinteHighlight = extrageCuvintePentruHighlight(query);
            const auto rezultate = index.cauta(query);

            std::cout << "Rezultate pentru query-ul '" << query << "':" << std::endl;
            if (rezultate.empty()) {
                std::cout << "Niciun document gasit." << std::endl;
            } else {
                const auto docsSortate = construiesteRezultateSortate(rezultate);
                for (const auto& docInfo : docsSortate) {
                    afiseazaRezultatePentruDocument(docInfo, cuvinteHighlight);
                }
            }
        } else if (optiune == "4" || optiune == "exit" || optiune == "quit") {
            ruleaza = false;
        } else {
            std::cout << "Optiune invalida!" << std::endl;
        }
    }

    return 0;
}
