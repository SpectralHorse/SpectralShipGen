#pragma once

#include <string>
#include <vector>
#include <utility>

namespace PixelShipGenerator
{
    enum class ValidationSeverity
    {
        ERROR = 0,
        WARNING
    };

    struct ValidationIssue
    {
        std::string Field;
        std::string Message;
        ValidationSeverity Severity = ValidationSeverity::ERROR;
    };

    struct ValidationResult
    {
        std::vector<ValidationIssue> Errors;
        std::vector<ValidationIssue> Warnings;

        bool isValid() const noexcept { return Errors.empty(); }
    };

    inline void appendValidationIssues(ValidationResult& destination, const ValidationResult& source, const std::string& prefix = {})
    {
        for (const ValidationIssue& issue : source.Errors)
        {
            ValidationIssue copy = issue;
            if (!prefix.empty()) { copy.Field = prefix + copy.Field; }
            destination.Errors.push_back(std::move(copy));
        }
        for (const ValidationIssue& issue : source.Warnings)
        {
            ValidationIssue copy = issue;
            if (!prefix.empty()) { copy.Field = prefix + copy.Field; }
            destination.Warnings.push_back(std::move(copy));
        }
    }
} // namespace PixelShipGenerator
