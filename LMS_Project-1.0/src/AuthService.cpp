
#include "AuthService.h"

AuthService::AuthService(Database& database)
    : database(database)
{
}

bool AuthService::login(
    const std::string& username,
    const std::string& password,
    AuthenticatedUser& user
)
{
    // Reject empty credentials.
    if (username.empty() || password.empty()) {
        return false;
    }

    int userId = 0;
    std::string role;

    // Ask Layer 3 to verify the credentials.
    if (!database.login(username, password, userId, role)) {
        return false;
    }

    // Return the authenticated user's details to Layer 1.
    user.userId = userId;
    user.username = username;
    user.role = role;

    return true;
}
