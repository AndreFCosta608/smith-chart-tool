#ifndef S1P_PARSER_H
#define S1P_PARSER_H

#include "types.h"
#include <vector>
#include <QString>

class S1PParser {
public:
    static bool loadFile(const QString& filePath, std::vector<S1PPoint>& outPoints, double z0 = 50.0);
};

#endif // S1P_PARSER_H
