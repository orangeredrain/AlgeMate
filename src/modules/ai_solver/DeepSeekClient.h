#ifndef ALGEMATE_DEEPSEEK_CLIENT_H
#define ALGEMATE_DEEPSEEK_CLIENT_H

#include <QString>

namespace AlgeMate::AiSolver {

/**
 * @brief Helper class for DeepSeek API configuration and utilities
 */
class DeepSeekClient {
public:
    static constexpr const char* API_URL = "https://api.deepseek.com/chat/completions";
    static constexpr const char* API_MODEL = "deepseek-chat";

    /**
     * @brief Create request body for API call
     */
    static QString createRequestBody(const QString& messages, const QString& model = API_MODEL);
};

}

#endif
