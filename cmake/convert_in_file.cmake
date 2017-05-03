# Convert .in files with @@VARIABLE@@ expressions into @VARIABLE@

message(STATUS "Processing ${INPUT_FILE}")
file(READ ${INPUT_FILE} CONTENT_IN)
string(REGEX REPLACE "@@" "@" CONTENT_OUT "${CONTENT_IN}")
file(WRITE ${OUTPUT_FILE} "${CONTENT_OUT}")
