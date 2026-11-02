#ifndef MARKING_SERVICE_H
#define MARKING_SERVICE_H

#include "Database.h"
#include "LatePenaltyStrategy.h"

#include <string>

class MarkingService {
public:
    explicit MarkingService(Database& database);

    bool markSubmission(
        int lecturerId,
        int submissionId,
        double rawMark,
        const std::string& feedback,
        const std::string& reason,
        double& finalMark,
        std::string& error
    );

    double calculateLatePenalty(
        const SubmissionMarkContext& context,
        double rawMark
    ) const;

private:
    Database& database;
    StandardLatePenalty latePenaltyStrategy;
};

#endif
