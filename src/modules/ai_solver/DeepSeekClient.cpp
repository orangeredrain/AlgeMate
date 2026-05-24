#include "DeepSeekClient.h"

namespace AlgeMate::AiSolver {

QString DeepSeekClient::createRequestBody(const QString& messages, const QString& model) {
    // This is a simple utility, actual request building is done in AiSolverPage
    return messages;
}

}
