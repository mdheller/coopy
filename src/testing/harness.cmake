if (in)
  execute_process(COMMAND ${process} INPUT_FILE ${in} OUTPUT_FILE ${out})
else()
  execute_process(COMMAND ${process} OUTPUT_FILE ${out})
endif()