#pragma once

struct LogSettings {
    bool hideAll = false;
    bool logRaw = false;
    bool logTokens = true;
    bool logParsed = true;
    bool logOutput = true;

    void setDebug() {
        logRaw = true;
        logTokens = true;
        logParsed = true;
        logOutput = true;
    }
};