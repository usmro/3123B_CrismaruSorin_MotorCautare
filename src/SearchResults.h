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

struct MatchSpan {
    int start = 0;
    int length = 0;
    std::string term;
};

std::vector<std::string> extrageCuvintePentruHighlight(const std::string& query);
std::vector<DocumentRezultat> construiesteRezultateSortate(
    const std::unordered_map<std::string, std::unordered_map<std::string, std::vector<int>>>& rezultate
);
void afiseazaRezultatePentruDocument(
    const DocumentRezultat& docInfo,
    const std::vector<std::string>& cuvinteHighlight
);

// Returnează o listă de span-uri ne-suprapuse (offset-uri în bytes) pentru termenii furnizați în `text`.
std::vector<MatchSpan> gasesteSpanuriPentruCuvinte(const std::string& text, const std::vector<std::string>& terms);

#endif // SEARCH_RESULTS_H