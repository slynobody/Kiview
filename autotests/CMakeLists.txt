# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: 2026 Méven Car <meven@kde.org>

include(ECMAddTests)

ecm_add_test(smoketest.cpp
    TEST_NAME kiviewsmoketest
    LINK_LIBRARIES Qt6::Test Qt6::Gui KF6::Archive
)
target_compile_definitions(kiviewsmoketest PRIVATE KIVIEW_BINARY="$<TARGET_FILE:kiview>")
add_dependencies(kiviewsmoketest kiview)
set_tests_properties(kiviewsmoketest PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
