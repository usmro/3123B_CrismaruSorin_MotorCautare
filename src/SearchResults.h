#ifndef SEARCH_RESULTS_H
#define SEARCH_RESULTS_H

#include <map>
#include <string>
#include <unordered_map>
#include <vector>

struct LinieRezultat {
    std::vector<std::string> cuvinte;
};

struct DocumentRezultat {
    std::string cale;
    int scor = 0;
    std::map<int, LinieRezultat> linii;
};

std::vector<std::string> extrageCuvintePentruHighlight(const std::string& query);
std::vector<DocumentRezultat> construiesteRezultateSortate(
    const std::unordered_map<std::string, std::unordered_map<std::string, std::vector<int>>>& rezultate
);
void afiseazaRezultatePentruDocument(
    const DocumentRezultat& docInfo,
    const std::vector<std::string>& cuvinteHighlight
);

#endif // SEARCH_RESULTS_H