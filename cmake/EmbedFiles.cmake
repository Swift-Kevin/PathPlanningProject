# Packs files into the program as byte arrays, so it runs without any files next to it

# embed_files(<output .cpp> <name> <file> [<name> <file> ...])
# Writes a .cpp file that fills in EMBEDDED_FILES from Interface/Style/EmbeddedFiles.h
# File paths are relative to this project folder, and editing one of them re-runs CMake on the next build
function(embed_files OUTPUT_FILE)
	set(pairs ${ARGN})
	list(LENGTH pairs pairCount)
	math(EXPR lastIndex "${pairCount} - 1")
	math(EXPR fileCount "${pairCount} / 2")

	# 16 bytes per line in the generated arrays
	string(REPEAT "0x[0-9a-f][0-9a-f]," 16 linePattern)

	set(arrays "")
	set(entries "")

	foreach(nameIndex RANGE 0 ${lastIndex} 2)
		math(EXPR fileIndex "${nameIndex} + 1")
		list(GET pairs ${nameIndex} name)
		list(GET pairs ${fileIndex} file)
		set(path "${CMAKE_CURRENT_SOURCE_DIR}/${file}")

		file(READ "${path}" hex HEX)
		string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," bytes "${hex}")
		string(REGEX REPLACE "(${linePattern})" "\\1\n\t\t" bytes "${bytes}")

		string(APPEND arrays "\t// ${file}\n\tconst unsigned char ${name}_DATA[] =\n\t{\n\t\t${bytes}\n\t};\n\n")
		string(APPEND entries "\t{ \"${name}\", ${name}_DATA, sizeof(${name}_DATA) },\n")
		set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${path}")
	endforeach()

	set(content "// Made by cmake/EmbedFiles.cmake from the list in CMakeLists.txt, don't edit\n")
	string(APPEND content "#include \"Interface/Style/EmbeddedFiles.h\"\n\n")
	string(APPEND content "namespace\n{\n${arrays}}\n\n")
	string(APPEND content "const EmbeddedFile EMBEDDED_FILES[] =\n{\n${entries}};\n\n")
	string(APPEND content "const int EMBEDDED_FILE_COUNT = ${fileCount};\n")

	# only replace the output when it changes, so a re-run doesn't rebuild anything
	file(WRITE "${OUTPUT_FILE}.tmp" "${content}")
	configure_file("${OUTPUT_FILE}.tmp" "${OUTPUT_FILE}" COPYONLY)
endfunction()
