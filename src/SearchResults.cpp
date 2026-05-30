#include "SearchResults.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>

namespace {

constexpr const char* CULOARE_RESET = "\033[0m";
constexpr const char* CULOARE_CYAN_BOLD = "\033[1;36m";
constexpr const char* CULOARE_GALBEN = "\033[33m";

std::string escapeazaPentruRegex(const std::string& text) {
    std::string rezultat;
    rezultat.reserve(text.size() * 2);

    for (unsigned char caracter : text) {
        switch (caracter) {
            case '.': case '^': case '$': case '|': case '(': case ')':
            case '[': case ']': case '{': case '}': case '*': case '+':
            case '?': case '\\':
                rezultat.push_back('\\');
                rezultat.push_back(static_cast<char>(caracter));
                break;
            default:
                rezultat.push_back(static_cast<char>(caracter));
                break;
        }
    }

    return rezultat;
}

bool esteCaracterDeCuvant(unsigned char caracter) {
    return std::isalnum(caracter) || caracter >= 128;
}

bool areGranitaLaPozitia(const std::string& text, size_t pozitie) {
    if (pozitie == 0 || pozitie >= text.size()) {
        return true;
    }

    const unsigned char stanga = static_cast<unsigned char>(text[pozitie - 1]);
    const unsigned char dreapta = static_cast<unsigned char>(text[pozitie]);
    return !esteCaracterDeCuvant(stanga) || !esteCaracterDeCuvant(dreapta);
}

std::string evidentiazaTextCuCuvant(const std::string& text, const std::string& cuvant) {
    if (text.empty() || cuvant.empty()) {
        return text;
    }

    std::string rezultat;
    rezultat.reserve(text.size() + 32);

    const std::string cuvantEscapat = escapeazaPentruRegex(cuvant);
    const std::regex pattern(cuvantEscapat, std::regex_constants::icase);

    size_t pozitieCurenta = 0;
    while (pozitieCurenta < text.size()) {
        std::smatch potrivire;
        const std::string rest = text.substr(pozitieCurenta);
        if (std::regex_search(rest, potrivire, pattern)) {
            const size_t inceput = pozitieCurenta + static_cast<size_t>(potrivire.position());
            const size_t sfarsit = inceput + static_cast<size_t>(potrivire.length());

            if (areGranitaLaPozitia(text, inceput) && areGranitaLaPozitia(text, sfarsit)) {
                rezultat.append(text, pozitieCurenta, inceput - pozitieCurenta);
                rezultat += CULOARE_GALBEN;
                rezultat.append(text, inceput, sfarsit - inceput);
                rezultat += CULOARE_RESET;
                pozitieCurenta = sfarsit;
                continue;
            }

            rezultat.append(text, pozitieCurenta, inceput - pozitieCurenta + 1);
            pozitieCurenta = inceput + 1;
        } else {
            rezultat.append(text.substr(pozitieCurenta));
            break;
        }
    }

    return rezultat;
}

} // namespace

std::vector<std::string> extrageCuvintePentruHighlight(const std::string& query) {
    std::vector<std::string> cuvinte;
    std::string token;
    bool inGhilimele = false;

    for (size_t i = 0; i < query.size(); ++i) {
        const char caracter = query[i];

        if (caracter == '"') {
            if (inGhilimele) {
                if (!token.empty()) {
                    cuvinte.push_back(token);
                    token.clear();
                }
                inGhilimele = false;
            } else {
                if (!token.empty() && token != "AND" && token != "OR") {
                    cuvinte.push_back(token);
                }
                token.clear();
                inGhilimele = true;
            }
            continue;
        }

        if (!inGhilimele && std::isspace(static_cast<unsigned char>(caracter))) {
            if (!token.empty()) {
                if (token != "AND" && token != "OR") {
                    cuvinte.push_back(token);
                }
                token.clear();
            }
        } else {
            token.push_back(caracter);
        }
    }

    if (!token.empty() && token != "AND" && token != "OR") {
        cuvinte.push_back(token);
    }

    return cuvinte;
}

std::vector<DocumentRezultat> construiesteRezultateSortate(
    const std::unordered_map<std::string, std::unordered_map<std::string, std::vector<int>>>& rezultate
) {
    std::map<std::string, DocumentRezultat> mapDocs;

    for (const auto& [cuvant, docs] : rezultate) {
        for (const auto& [doc, linii] : docs) {
            mapDocs[doc].cale = doc;
            mapDocs[doc].scor += static_cast<int>(linii.size());
            for (int linie : linii) {
                mapDocs[doc].linii[linie].cuvinte.push_back(cuvant);
            }
        }
    }

    std::vector<DocumentRezultat> docsSortate;
    docsSortate.reserve(mapDocs.size());
    for (const auto& [cale, info] : mapDocs) {
        docsSortate.push_back(info);
    }

    std::sort(docsSortate.begin(), docsSortate.end(), [](const DocumentRezultat& a, const DocumentRezultat& b) {
        return a.scor > b.scor;
    });

    return docsSortate;
}

void afiseazaRezultatePentruDocument(
    const DocumentRezultat& docInfo,
    const std::vector<std::string>& cuvinteHighlight
) {
    std::cout << "- " << CULOARE_CYAN_BOLD << docInfo.cale << CULOARE_RESET
              << " (" << docInfo.scor << " potriviri):" << std::endl;

    std::ifstream fisier(docInfo.cale);
    if (!fisier.is_open()) {
        std::cout << "  (Eroare la deschiderea fisierului pentru a extrage textul)\n";
        return;
    }

    std::string linieFisier;
    int indexLinieCurenta = 1;
    auto itLinii = docInfo.linii.begin();

    while (std::getline(fisier, linieFisier) && itLinii != docInfo.linii.end()) {
        if (indexLinieCurenta == itLinii->first) {
            std::string textFinal = linieFisier;
            std::vector<std::string> cuvinteDeEvidentiat = itLinii->second.cuvinte;

            for (const auto& cw : cuvinteHighlight) {
                if (std::find(cuvinteDeEvidentiat.begin(), cuvinteDeEvidentiat.end(), cw) == cuvinteDeEvidentiat.end()) {
                    cuvinteDeEvidentiat.push_back(cw);
                }
            }

            for (const auto& cw : cuvinteDeEvidentiat) {
                textFinal = evidentiazaTextCuCuvant(textFinal, cw);
            }

            std::cout << "  L" << itLinii->first << ": " << textFinal << std::endl;
            ++itLinii;
        }

        ++indexLinieCurenta;
    }
}

std::vector<MatchSpan> gasesteSpanuriPentruCuvinte(const std::string& text, const std::vector<std::string>& terms) {
    std::vector<MatchSpan> spans;
    if (text.empty() || terms.empty()) return spans;

    for (const auto& term : terms) {
        if (term.empty()) continue;
        const std::string termEsc = escapeazaPentruRegex(term);
        const std::regex pattern(termEsc, std::regex_constants::icase);

        auto begin = std::sregex_iterator(text.begin(), text.end(), pattern);
        auto end = std::sregex_iterator();

        for (auto it = begin; it != end; ++it) {
            const std::smatch m = *it;
            const size_t inceput = static_cast<size_t>(m.position());
            const size_t sfarsit = inceput + static_cast<size_t>(m.length());

            if (!areGranitaLaPozitia(text, inceput) || !areGranitaLaPozitia(text, sfarsit)) {
                continue;
            }

            MatchSpan ms;
            ms.start = static_cast<int>(inceput);
            ms.length = static_cast<int>(m.length());
            ms.term = term;
            spans.push_back(ms);
        }
    }

    // Sortează span-urile după poziția de început și elimină suprapunerile (preferă span-urile anterioare)
    std::sort(spans.begin(), spans.end(), [](const MatchSpan& a, const MatchSpan& b) {
        if (a.start != b.start) return a.start < b.start;
        return a.length > b.length;
    });

    std::vector<MatchSpan> nonOverlapping;
    int curEnd = -1;
    for (const auto& s : spans) {
        if (s.start >= curEnd) {
            nonOverlapping.push_back(s);
            curEnd = s.start + s.length;
        }
    }

    return nonOverlapping;
}