#ifndef DOCUMENT_H
#define DOCUMENT_H

#include <string>
#include <vector>
#include <functional>
//cum functioneaza pt. documente extreme
#include "Observable.h"

class Document : public Observable {
public:
    void notificaCautare(const std::string& cuvant) {
        notifica("S-a cautat cuvantul: " + cuvant);
    }
        Document(const std::string& caleFisier);
        //referinta, read-only
        const std::string& obtineCaleFisier() const;
        void proceseazaCuvinte(const std::function<void(const std::string&, int)>& callback) const;

    private:
        std::string m_caleFisier;
};

#endif // DOCUMENT_H
