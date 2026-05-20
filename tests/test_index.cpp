#include "../src/Index.h"
#include "../src/Document.h"

#include <iostream>
#include <cassert>
#include <fstream>
#include <sstream>
#include <filesystem>

// Simple test framework
class TestRunner {
private:
    int passed = 0;
    int failed = 0;

public:
    void assertTrue(bool condition, const std::string& testName) {
        if (condition) {
            std::cout << "✓ " << testName << " - PASSED" << std::endl;
            passed++;
        } else {
            std::cout << "✗ " << testName << " - FAILED" << std::endl;
            failed++;
        }
    }

    void assertEqual(int actual, int expected, const std::string& testName) {
        if (actual == expected) {
            std::cout << "✓ " << testName << " - PASSED" << std::endl;
            passed++;
        } else {
            std::cout << "✗ " << testName << " - FAILED (expected: " << expected
                      << ", got: " << actual << ")" << std::endl;
            failed++;
        }
    }

    void assertEqual(const std::string& actual, const std::string& expected, const std::string& testName) {
        if (actual == expected) {
            std::cout << "✓ " << testName << " - PASSED" << std::endl;
            passed++;
        } else {
            std::cout << "✗ " << testName << " - FAILED (expected: " << expected
                      << ", got: " << actual << ")" << std::endl;
            failed++;
        }
    }

    void printSummary() {
        int total = passed + failed;
        std::cout << "\n=== TEST SUMMARY ===" << std::endl;
        std::cout << "Passed: " << passed << "/" << total << std::endl;
        std::cout << "Failed: " << failed << "/" << total << std::endl;
    }
};

std::string getBaseTestDir() {
    return std::filesystem::temp_directory_path().string() + "/motor_cautare_tests";
}

void setupTestDir(const std::string& dir) {
    if (std::filesystem::exists(dir)) {
        std::filesystem::remove_all(dir);
    }
    std::filesystem::create_directories(dir);
}

// Test Document class
void testDocument(TestRunner& runner) {
    std::cout << "\n--- Testing Document Class ---" << std::endl;

    std::string testDir = getBaseTestDir() + "/doc_test";
    setupTestDir(testDir);
    std::string testFile = testDir + "/test_document.txt";
    
    std::ofstream outFile(testFile);
    outFile << "Acesta este un document de test.\n";
    outFile << "Continutul documentului.\n";
    outFile.close();

    Document doc(testFile);

    runner.assertEqual(doc.obtineCaleFisier(), testFile, "Document path retrieval");
    std::string continut;
    doc.proceseazaCuvinte([&continut](const std::string& cuvant, int) {
        continut += cuvant + " ";
    });
    runner.assertTrue(!continut.empty(), "Document content loading");
    runner.assertTrue(continut.find("test") != std::string::npos,
                     "Document content contains expected text");
}

// Test Index normalization
void testIndexNormalization(TestRunner& runner) {
    std::cout << "\n--- Testing Index Normalization ---" << std::endl;

    std::string testDir = getBaseTestDir() + "/normalization";
    setupTestDir(testDir);
    std::string stopwordsFile = testDir + "/stopwords.txt";
    std::string testFile = testDir + "/normalization_test.txt";

    std::ofstream swFile(stopwordsFile);
    swFile.close();

    std::ofstream docFile(testFile);
    docFile << "MOTOR!!!\n";
    docFile << "Alt cuvant.\n";
    docFile.close();

    Index index(stopwordsFile);
    index.incarcaDocumenteDinDirector(testDir);
    index.construiesteIndex();

    auto rezultate = index.cautaUnSingurCuvant("motor");
    runner.assertTrue(!rezultate.empty(), "Normalization finds uppercase word with punctuation");
    runner.assertTrue(rezultate.count(testFile) == 1, "Normalization returns the expected document path");
}

// Test Index with stopwords
void testIndexStopwords(TestRunner& runner) {
    std::cout << "\n--- Testing Index Stopwords ---" << std::endl;

    std::string testDir = getBaseTestDir() + "/stopwords";
    setupTestDir(testDir);
    std::string stopwordsFile = testDir + "/stopwords.txt";
    std::string testFile = testDir + "/stopwords_test.txt";

    std::ofstream swFile(stopwordsFile);
    swFile << "si\n";
    swFile << "in\n";
    swFile << "de\n";
    swFile.close();

    std::ofstream docFile(testFile);
    docFile << "Acesta este un document.\n";
    docFile << "Si continutul lui este important.\n";
    docFile.close();

    Index index(stopwordsFile);
    index.incarcaDocumenteDinDirector(testDir);
    index.construiesteIndex();

    auto rezultate = index.cauta("si document");
    runner.assertTrue(!rezultate.empty(), "Stopwords are removed, valid words indexed");
    runner.assertTrue(rezultate.count("document") == 1, "Mixed query keeps the meaningful word");
    runner.assertTrue(rezultate.count("si") == 0, "Mixed query drops the stopword");
}

// Test Index search functionality
void testIndexSearch(TestRunner& runner) {
    std::cout << "\n--- Testing Index Search ---" << std::endl;

    std::string testDir = getBaseTestDir() + "/search";
    setupTestDir(testDir);
    
    std::string stopwordsFile = testDir + "/stopwords.txt";
    std::ofstream swFile(stopwordsFile);
    swFile << "si\n";
    swFile << "in\n";
    swFile.close();

    std::string testFile = testDir + "/test_search.txt";
    std::ofstream docFile(testFile);
    docFile << "Motor de cautare simplu\n";
    docFile << "Cautarea este importanta\n";
    docFile << "Cuvinte cheie pentru test\n";
    docFile.close();

    Index index(stopwordsFile);
    index.incarcaDocumenteDinDirector(testDir);
    index.construiesteIndex();

    auto rezultate = index.cautaUnSingurCuvant("cautare");
    runner.assertTrue(!rezultate.empty(), "Search finds existing word");

    auto rezultateEmpty = index.cautaUnSingurCuvant("inexistent");
    runner.assertTrue(rezultateEmpty.empty(), "Search returns empty for non-existing word");
}

// Test AND/OR operations
void testAndOrSearch(TestRunner& runner) {
    std::cout << "\n--- Testing AND/OR Search ---" << std::endl;

    std::string testDir = getBaseTestDir() + "/andor";
    setupTestDir(testDir);
    std::string stopwordsFile = testDir + "/stopwords.txt";
    std::string docBoth = testDir + "/both.txt";
    std::string docOnlyMotor = testDir + "/motor.txt";

    std::ofstream swFile(stopwordsFile);
    swFile.close();

    std::ofstream bothFile(docBoth);
    bothFile << "motor document\n";
    bothFile.close();

    std::ofstream motorFile(docOnlyMotor);
    motorFile << "motor\n";
    motorFile.close();

    Index index(stopwordsFile);
    index.incarcaDocumenteDinDirector(testDir);
    index.construiesteIndex();

    auto rezultateOr = index.cauta("motor OR document");
    runner.assertTrue(rezultateOr.count("motor") == 1, "OR search includes motor results");
    runner.assertTrue(rezultateOr.count("document") == 1, "OR search includes document results");
    runner.assertTrue(rezultateOr["motor"].count(docBoth) == 1, "OR search returns the shared document for motor");
    runner.assertTrue(rezultateOr["motor"].count(docOnlyMotor) == 1, "OR search returns the motor-only document");

    auto rezultateAnd = index.cauta("motor AND document");
    runner.assertTrue(rezultateAnd.count("motor") == 1, "AND search includes motor results");
    runner.assertTrue(rezultateAnd.count("document") == 1, "AND search includes document results");
    runner.assertTrue(rezultateAnd["motor"].count(docBoth) == 1, "AND search keeps only the document that contains both words for motor");
    runner.assertTrue(rezultateAnd["document"].count(docBoth) == 1, "AND search keeps only the document that contains both words for document");
    runner.assertTrue(rezultateAnd["motor"].count(docOnlyMotor) == 0, "AND search excludes the motor-only document");
}

int main() {
    TestRunner runner;

    std::cout << "========== UNIT TESTS ==========" << std::endl;

    testDocument(runner);
    testIndexNormalization(runner);
    testIndexStopwords(runner);
    testIndexSearch(runner);
    testAndOrSearch(runner);

    runner.printSummary();

    return 0;
}
