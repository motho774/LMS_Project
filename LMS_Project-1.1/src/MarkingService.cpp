#include "MarkingService.h"

#include <algorithm>

MarkingService::MarkingService(Database& database)
    : database(database) {
}

double MarkingService::calculateLatePenalty(
    const SubmissionMarkContext& context,
    double rawMark
) const {
    if (!context.isLate) {
        return 0.0;
    }

    return latePenaltyStrategy.calculate(
        rawMark,
        context.maxMark,
        context.submittedAt,
        context.deadline
    );
}

bool MarkingService::markSubmission(
    int lecturerId,
    int submissionId,
    double rawMark,
    const std::string& feedback,
    const std::string& reason,
    double& finalMark,
    std::string& error
) {
    error.clear();
    finalMark = 0.0;

    if (lecturerId <= 0 || submissionId <= 0) {
        error = "Invalid lecturer or submission selection.";
        return false;
    }

    if (feedback.empty()) {
        error = "Feedback is required before saving a mark.";
        return false;
    }

    if (reason.empty()) {
        error = "A reason is required for any mark change.";
        return false;
    }

    SubmissionMarkContext context;
    if (!database.getSubmissionMarkingContext(
            lecturerId, submissionId, context, error)) {
        return false;
    }

    if (!context.isCurrentSubmission) {
        error = "A newer submission is the current version for marking.";
        return false;
    }

    if (rawMark < 0.0 || rawMark > context.maxMark) {
        error = "The mark must be between 0 and the assignment maximum mark.";
        return false;
    }

    const double penalty = calculateLatePenalty(context, rawMark);

    finalMark = std::max(0.0, rawMark - penalty);
    finalMark = std::min(finalMark, context.maxMark);

    if (!database.markSubmission(
            lecturerId,
            submissionId,
            rawMark,
            finalMark,
            feedback,
            reason,
            error)) {
        return false;
    }

    return true;
}
