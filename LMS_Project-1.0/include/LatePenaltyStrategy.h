#ifndef LATE_PENALTY_STRATEGY_H
#define LATE_PENALTY_STRATEGY_H

#include <string>

class ILatePenaltyStrategy {
public:
    virtual ~ILatePenaltyStrategy() = default;

    virtual double calculate(
        double rawMark,
        double maxMark,
        const std::string& submittedAt,
        const std::string& deadline
    ) const = 0;
};

class StandardLatePenalty : public ILatePenaltyStrategy {
public:
    double calculate(
        double rawMark,
        double maxMark,
        const std::string& submittedAt,
        const std::string& deadline
    ) const override;
};

#endif
