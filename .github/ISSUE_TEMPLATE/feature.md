ISSUE: "Feature Suggestion" <br>
Title: "[Feature] Title Text" <br>
---
#### Labels: enhancement, optimization, extension, etc

## Description
Briefly describe the feature you’d like to see.

##  Behavioural Specification
```gherkin
# Example Of A Gherkin Code  (Optional)
# Recommended for Complex Features
# If not required then remove 
# Behavioural Specification

Feature: JSON String Processing

  Scenario: Parse simple Valid JSON String 
    Given a JSON string "{ \"status\": \"ok\" }"
    When the function process_json is called
    Then it should return an object
    And the object["status"] should equal "ok"  
```

## Additional Context
Screenshots, references, or links that help explain.
