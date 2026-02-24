# Copyright (c) 2023, RPTU Kaiserslautern-Landau
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its
#    contributors may be used to endorse or promote products derived from
#    this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
# TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER
# OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# Authors:
#    Derek Christ

find_program(SqlDiff sqldiff)

if(NOT SqlDiff)
    message(WARNING "Regression tests require sqldiff to be installed")
    return()
endif()

set(TABLES_TO_COMPARE
    Phases
    Transactions
    Power
)

set(REGRESSION_PACKAGE "regression-test-dbs")
set(REGRESSION_VERSION "5.6.0")

if(DEFINED ENV{CI_JOB_TOKEN})
    set(AUTH_HEADER "JOB-TOKEN:$ENV{CI_JOB_TOKEN}")
    set(ENDPOINT "https://$ENV{CI_SERVER_HOST}/api/v4/projects/$ENV{CI_PROJECT_ID}/packages/generic/")
else()
    set(AUTH_HEADER "PRIVATE-TOKEN:$ENV{GITLAB_TOKEN}")
    set(ENDPOINT "https://erbenschell.iese.fraunhofer.de/api/v4/projects/2132/packages/generic/")
endif()

function(fetch_reference test_name reference)
    set(REF_DATA_FILE "${CMAKE_CURRENT_BINARY_DIR}/${test_name}/expected/${reference}")
    if (NOT EXISTS ${REF_DATA_FILE})
        file(DOWNLOAD
            "${ENDPOINT}/${REGRESSION_PACKAGE}/${REGRESSION_VERSION}/${reference}"
            ${REF_DATA_FILE}
            HTTPHEADER ${AUTH_HEADER}
            STATUS transfer_status
        )
        list(GET transfer_status 0 status_code)
        if(status_code)
            message(WARNING "Failed to fetch regression databases")
        endif()
    endif()
endfunction()

function(test_standard test_name base_config reference output_filename)
    fetch_reference(${test_name} ${reference})

    # Put all the generated files into a subdirectory
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${test_name})

    # Test to create database
    add_test(
        NAME Regression${test_name}.CreateDatabase
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${test_name}
        COMMAND $<TARGET_FILE:dramsys_bin> ${base_config}
    )
    set_tests_properties(Regression${test_name}.CreateDatabase PROPERTIES FIXTURES_SETUP Regression${test_name}.CreateDatabase)

    # Test to diff the whole database. This test should not fail.
    # The purpose of this test is solely to output the differences of the two databases
    # so that they can be inspected easily.
    add_test(
        NAME Regression${test_name}.SqlDiff
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${test_name}
        COMMAND sqldiff expected/${reference} ${output_filename}
    )
    set_tests_properties(Regression${test_name}.SqlDiff PROPERTIES FIXTURES_REQUIRED Regression${test_name}.CreateDatabase)

    # Tests to diff individual tables
    foreach(table IN LISTS TABLES_TO_COMPARE)
        add_test(
            NAME Regression${test_name}.SqlDiff.${table}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${test_name}
            COMMAND sqldiff --table ${table} expected/${reference} ${output_filename}
        )
        set_tests_properties(Regression${test_name}.SqlDiff.${table} PROPERTIES FIXTURES_REQUIRED Regression${test_name}.CreateDatabase)

        # Only pass test if output is empty
        set_tests_properties(Regression${test_name}.SqlDiff.${table} PROPERTIES PASS_REGULAR_EXPRESSION "^$")
    endforeach()
endfunction()
