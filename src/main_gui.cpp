#include "Index.h"
#include "Logger.h"
#include "SearchResults.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <algorithm>

#include <limits.h>
#include <unistd.h>
#include <cstring>

namespace {

struct GuiTextSegment {
    std::string text;
    ImVec4 color;
};

static void renderWrappedColoredText(const std::vector<GuiTextSegment>& segments) {
    const float startX = ImGui::GetCursorPosX();
    const float wrapX = startX + ImGui::GetContentRegionAvail().x;

    auto renderPiece = [&](const std::string& piece, const ImVec4& color) {
        if (piece.empty()) return;

        const ImVec2 size = ImGui::CalcTextSize(piece.c_str());
        const float curX = ImGui::GetCursorPosX();
        if (curX + size.x > wrapX && curX > startX) {
            ImGui::NewLine();
            ImGui::SetCursorPosX(startX);
        }

        ImGui::TextColored(color, "%s", piece.c_str());
        ImGui::SameLine(0.0f, 0.0f);
    };

    for (const auto& seg : segments) {
        std::string token;
        for (char ch : seg.text) {
            if (std::isspace(static_cast<unsigned char>(ch))) {
                renderPiece(token, seg.color);
                token.clear();
                renderPiece(std::string(1, ch), seg.color);
            } else {
                token.push_back(ch);
            }
        }
        renderPiece(token, seg.color);
    }

    ImGui::NewLine();
}

static void incarcaFontCuDiacritice(const std::filesystem::path& directorProiect) {
    ImGuiIO& io = ImGui::GetIO();

    ImFontConfig config;
    config.OversampleH = 3;
    config.OversampleV = 1;
    config.PixelSnapH = true;

    static const ImWchar rangeLatinExtins[] = { 0x0020, 0x024F, 0 };
    const std::vector<std::filesystem::path> caiFont = {
        directorProiect / "vendor" / "imgui" / "misc" / "fonts" / "DroidSans.ttf",
        directorProiect / "vendor" / "imgui" / "misc" / "fonts" / "Roboto-Medium.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"
    };

    for (const auto& caleFont : caiFont) {
        if (ImFont* font = io.Fonts->AddFontFromFileTTF(caleFont.string().c_str(), 18.0f, &config, rangeLatinExtins)) {
            io.FontDefault = font;
            return;
        }
    }
}

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

static void glfw_error_callback(int error, const char* description) {
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

int main(int, char**) {
    // Error handling openGL
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "Motor de Cautare", NULL, NULL);
    if (window == NULL)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Vsync activat

    // Seteaza context ImGUI
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    // Stil ImGUI
    ImGui::StyleColorsDark();

    // Seteaza backend Platform/Renderer
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Initializare căi implicite
    const std::string directorImplicit = obtineDirectorImplicitCautare();
    const std::filesystem::path caleStopwords = std::filesystem::path(directorImplicit) / "stopwords.txt";
    const std::filesystem::path caleLog = std::filesystem::path(directorImplicit) / "log.txt";

    // Font Unicode pentru diacritice românești
    incarcaFontCuDiacritice(std::filesystem::path(directorImplicit));

    // Starea aplicatiei
    Index index(caleStopwords.string());
    Logger logger(caleLog.string());
    index.adaugaObserver(&logger);
    
    char folderPath[512] = "";
    std::strncpy(folderPath, directorImplicit.c_str(), sizeof(folderPath) - 1);

    char searchQuery[128] = "";
    std::unordered_map<std::string, std::unordered_map<std::string, std::vector<int>>> searchResults;
    bool searchPerformed = false;

    index.incarcaDocumenteDinDirector(directorImplicit);
    index.construiesteIndex();

    // Loop principal
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // GUI
        ImGui::Begin("Panou de Control");

        ImGui::InputText("Cale Director", folderPath, IM_ARRAYSIZE(folderPath));
        if (ImGui::Button("Incarca si Indexeaza")) {
            index.incarcaDocumenteDinDirector(folderPath);
            index.construiesteIndex();
        }

        ImGui::Separator();

        const auto& documente = index.obtineDocumente();
        if (ImGui::TreeNode(("Documente indexate (" + std::to_string(documente.size()) + ")").c_str())) {
            for (const auto& doc : documente) {
                ImGui::BulletText("%s", doc.obtineCaleFisier().c_str());
            }
            ImGui::TreePop();
        }

        ImGui::Separator();

        ImGui::InputText("Interogare Cautare", searchQuery, IM_ARRAYSIZE(searchQuery));
        if (ImGui::Button("Cauta") || ImGui::IsKeyPressed(ImGuiKey_Enter)) {
            searchResults = index.cauta(searchQuery);
            searchPerformed = true;
        }

        ImGui::End();

        // Fereastra rezultate
        ImGui::Begin("Rezultate Cautare");
            if (searchPerformed) {
                if (searchResults.empty()) {
                    ImGui::TextWrapped("Niciun rezultat gasit.");
                } else {
                    // Construiește rezultatele sortate pentru GUI (reprezentare locală)
                    struct GuiDocInfo { std::string cale; int scor = 0; std::map<int, std::vector<std::string>> linii; };

                    std::map<std::string, GuiDocInfo> mapDocs;
                    for (const auto& [cuvant, docs] : searchResults) {
                        for (const auto& [doc, linii] : docs) {
                            mapDocs[doc].cale = doc;
                            mapDocs[doc].scor += static_cast<int>(linii.size());
                            for (int linie : linii) {
                                mapDocs[doc].linii[linie].push_back(cuvant);
                            }
                        }
                    }

                    std::vector<GuiDocInfo> docsSortate;
                    docsSortate.reserve(mapDocs.size());
                    for (const auto& [cale, info] : mapDocs) docsSortate.push_back(info);
                    std::sort(docsSortate.begin(), docsSortate.end(), [](const GuiDocInfo& a, const GuiDocInfo& b) {
                        return a.scor > b.scor;
                    });

                    // extrage termenii din interogarea curentă pentru highlighting
                    std::vector<std::string> queryTerms = extrageCuvintePentruHighlight(std::string(searchQuery));

                    for (const auto& docInfo : docsSortate) {
                        ImGui::TextColored(ImVec4(0.0f, 0.6f, 1.0f, 1.0f), "%s (%d potriviri)", docInfo.cale.c_str(), docInfo.scor);

                        // deschide fișierul și afișează fiecare linie găsită cu termenii potriviți dedesubt
                        std::ifstream fisier(docInfo.cale);
                        if (!fisier.is_open()) {
                            ImGui::TextWrapped("  (Eroare la deschiderea fisierului pentru a extrage textul)");
                            continue;
                        }

                        std::string linieFisier;
                        int indexLinieCurenta = 1;
                        auto itLinii = docInfo.linii.begin();

                        while (std::getline(fisier, linieFisier) && itLinii != docInfo.linii.end()) {
                            if (indexLinieCurenta == itLinii->first) {
                                // evidențiere inline a termenilor potriviți din linie
                                std::vector<std::string> cuvinteDeEvidentiat = itLinii->second;
                                for (const auto& q : queryTerms) {
                                    if (std::find(cuvinteDeEvidentiat.begin(), cuvinteDeEvidentiat.end(), q) == cuvinteDeEvidentiat.end()) {
                                        cuvinteDeEvidentiat.push_back(q);
                                    }
                                }

                                const std::vector<MatchSpan> spans = gasesteSpanuriPentruCuvinte(linieFisier, cuvinteDeEvidentiat);

                                // afișează numărul liniei
                                ImGui::Text("  L%d:", itLinii->first);
                                ImGui::SameLine();

                                // culori
                                ImVec4 colNormal = ImGui::GetStyleColorVec4(ImGuiCol_Text);
                                ImVec4 colHighlight = ImVec4(1.0f, 0.8f, 0.2f, 1.0f);

                                std::vector<GuiTextSegment> segmente;
                                int pos = 0;
                                for (size_t si = 0; si < spans.size(); ++si) {
                                    const auto& s = spans[si];
                                    if (s.start > pos) {
                                        segmente.push_back({ linieFisier.substr(pos, s.start - pos), colNormal });
                                    }

                                    segmente.push_back({ linieFisier.substr(s.start, s.length), colHighlight });
                                    pos = s.start + s.length;
                                }

                                if (pos < static_cast<int>(linieFisier.size())) {
                                    segmente.push_back({ linieFisier.substr(pos), colNormal });
                                }

                                renderWrappedColoredText(segmente);

                                ++itLinii;
                            }
                            ++indexLinieCurenta;
                        }
                        ImGui::Spacing();
                    }
                }
        }
        ImGui::End();

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // Curatare
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
