
#ifndef AUTHSERVICE_H
#define AUTHSERVICE_H

#include "Database.h"
#include <string>

struct AuthenticatedUser {
    int userId;
    std::string username;
    std::string role;
};

class AuthService {
private:
    Database& database;

public:
    explicit AuthService(Database& database);

    bool login(
        const std::string& username,
        const std::string& password,
        AuthenticatedUser& user
    );
};

#endif
