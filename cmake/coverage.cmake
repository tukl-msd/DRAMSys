set(
    COVERAGE_BASE_COMMAND
    lcov -c -q -i
    -o "${PROJECT_BINARY_DIR}/coverage_base.info"
    -d "${PROJECT_BINARY_DIR}"
    --include "${PROJECT_SOURCE_DIR}/src/*"
    CACHE STRING
    "Command to generate initial zero coverage data"
)

set(
    COVERAGE_TRACE_COMMAND
    lcov -c -q
    -o "${PROJECT_BINARY_DIR}/coverage_tests.info"
    -d "${PROJECT_BINARY_DIR}"
    --include "${PROJECT_SOURCE_DIR}/src/*"
    CACHE STRING
    "Command to generate coverage data"
)

set(
    COVERAGE_COMBINE_COMMAND
    lcov -q
    -a "${PROJECT_BINARY_DIR}/coverage_base.info"
    -a "${PROJECT_BINARY_DIR}/coverage_tests.info"
    -o "${PROJECT_BINARY_DIR}/coverage.info"
    CACHE STRING
    "Command to combine coverage files to resulting coverage data"
)

set(
    COVERAGE_HTML_COMMAND
    genhtml --legend -f -q
    "${PROJECT_BINARY_DIR}/coverage.info"
    -p "${PROJECT_SOURCE_DIR}"
    -o "${PROJECT_BINARY_DIR}/coverage_html"
    CACHE STRING
    "Command to generate HTML report for the coverage"
)

set(
    COVERAGE_LIST_COMMAND
    lcov
    --list "${PROJECT_BINARY_DIR}/coverage.info"
)

add_custom_target(
    coverage
    COMMAND ${COVERAGE_BASE_COMMAND}
    COMMAND ${COVERAGE_TRACE_COMMAND}
    COMMAND ${COVERAGE_COMBINE_COMMAND}
    COMMAND ${COVERAGE_HTML_COMMAND}
    COMMAND ${COVERAGE_LIST_COMMAND}
    COMMENT "Generate coverage report"
    VERBATIM
)
