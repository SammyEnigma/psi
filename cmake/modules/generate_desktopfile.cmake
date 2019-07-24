cmake_minimum_required( VERSION 3.1.0 )

set(DESKTOP_FILE "${PROJECT_SOURCE_DIR}/psi.desktop")
set(DESKTOP_FILE_SEC_PART "${PROJECT_SOURCE_DIR}/psi-extra-action1.desktop")
file(READ ${DESKTOP_FILE} DESK_FILE_CONTENTS)
file(READ ${DESKTOP_FILE_SEC_PART} PART2_CONTENTS)
set(OUT_DESK_FILE "${CMAKE_BINARY_DIR}/${VERBOSED_NAME}.desktop")
file(WRITE ${OUT_DESK_FILE}
    ${DESK_FILE_CONTENTS}
    ${PART2_CONTENTS}
    )
unset(DESK_FILE_CONTENTS)
unset(PART2_CONTENTS)
if(VERBOSE_PROGRAM_NAME)
    if(IS_PSIPLUS)
        set(NAME_PREFIX "Psi+")
        set(EXEC_REGEXP "Exec=psi-plus ")
        set(NAME_REGEXP "Name=Psi\\+")
        set(ICON_REGEXP "Icon=psi-plus")
    else()
        set(NAME_PREFIX "Psi")
        set(EXEC_REGEXP "Exec=psi ")
        set(NAME_REGEXP "Name=Psi")
        set(ICON_REGEXP "Icon=psi")
    endif()
    set(TMP_DESK_FILE "${CMAKE_CURRENT_BINARY_DIR}/${VERBOSED_NAME}.desktop.in")
    file(WRITE ${TMP_DESK_FILE} "")
    file(READ ${OUT_DESK_FILE} DESK_FILE_CONTENTS)
    #hack for desktop file generaion
    string(REGEX REPLACE "${EXEC_REGEXP}" "Exec=${VERBOSED_NAME} " FIX1 "${DESK_FILE_CONTENTS}")
    string(REGEX REPLACE "${ICON_REGEXP}" "Icon=${VERBOSED_NAME}" FIX2 "${FIX1}")
    if(IS_WEBENGINE)
        string(REGEX REPLACE "${NAME_REGEXP}" "Name=${NAME_PREFIX} Webengine" FIX3 "${FIX2}")
    elseif(IS_WEBKIT)
        string(REGEX REPLACE "${NAME_REGEXP}" "Name=${NAME_PREFIX} Webkit" FIX3 "${FIX2}")
    endif()
    if(FIX3)
        file(APPEND ${TMP_DESK_FILE} "${FIX3}")
    elseif(FIX2)
        file(APPEND ${TMP_DESK_FILE} "${FIX2}")
    else()
        file(APPEND ${TMP_DESK_FILE} "${FIX1}")
    endif()
    configure_file(${TMP_DESK_FILE} ${OUT_DESK_FILE} COPYONLY)
    unset(DESK_FILE_CONTENTS)
endif()
message(STATUS "${OUT_DESK_FILE} file generated")
