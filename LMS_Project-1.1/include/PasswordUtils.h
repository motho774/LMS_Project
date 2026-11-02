#ifndef PASSWORD_UTILS_H
#define PASSWORD_UTILS_H

#include <string>

class PasswordUtils {
public:
    static bool hashPassword(
        const std::string& password,
        std::string& hash,
        std::string& salt
    );

    static bool verifyPassword(
        const std::string& password,
        const std::string& hash,
        const std::string& salt
    );
};

#endif