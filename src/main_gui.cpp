#include "Index.h"
#include "Logger.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
#include <map>

#include <limits.h>
#include <unistd.h>
#include <cstring>

namespace {

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
                for (const auto& [cuvant, docs] : searchResults) {
                    ImGui::TextWrapped("Cuvant: %s", cuvant.c_str());
                    for (const auto& [docPath, linii] : docs) {
                        ImGui::TextWrapped("  Document: %s", docPath.c_str());
                        std::string liniiString = "Linii: ";
                        for (size_t i = 0; i < linii.size(); ++i) {
                            liniiString += std::to_string(linii[i]);
                            if(i < linii.size() - 1){
                                liniiString += ", ";
                            }
                        }
                        ImGui::TextWrapped("%s", liniiString.c_str());
                        ImGui::Spacing();
                    }
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
