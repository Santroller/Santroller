function(compile_proto)
	find_package(Python3 REQUIRED COMPONENTS Interpreter)

	set(VENV ${CMAKE_CURRENT_BINARY_DIR}/venv)
	set(VENV_FILE ${VENV}/environment.txt)

	if(CMAKE_HOST_WIN32)
		set(VENV_BIN_DIR ${VENV}/Scripts)
	else()
		set(VENV_BIN_DIR ${VENV}/bin)
	endif()

	add_custom_command(
		DEPENDS ${CMAKE_SOURCE_DIR}/lib/nanopb/extra/requirements.txt
		COMMAND ${Python3_EXECUTABLE} -m venv ${VENV}
		COMMAND ${VENV_BIN_DIR}/pip --disable-pip-version-check install -r ${CMAKE_SOURCE_DIR}/lib/nanopb/extra/requirements.txt
		COMMAND ${VENV_BIN_DIR}/pip freeze > ${VENV_FILE}
		OUTPUT ${VENV_FILE}
		COMMENT "Setting up Python Virtual Environment"
	)

	set(NANOPB_GENERATOR ${CMAKE_SOURCE_DIR}/lib/nanopb/generator/nanopb_generator.py)
	set(PROTO_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/proto)
	set(PROTO_OUTPUT_DIR ${PROTO_OUTPUT_DIR} PARENT_SCOPE)

	add_custom_command(
		DEPENDS ${VENV_FILE} ${NANOPB_GENERATOR} ${CMAKE_SOURCE_DIR}/proto/enums.proto ${CMAKE_SOURCE_DIR}/proto/input_enums.proto ${CMAKE_SOURCE_DIR}/proto/config.proto ${CMAKE_SOURCE_DIR}/proto/device.proto ${CMAKE_SOURCE_DIR}/proto/input.proto ${CMAKE_SOURCE_DIR}/lib/nanopb/generator/proto/nanopb.proto ${CMAKE_SOURCE_DIR}/proto/events.proto ${CMAKE_SOURCE_DIR}/proto/commands.proto
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		COMMAND ${CMAKE_COMMAND} -E make_directory ${PROTO_OUTPUT_DIR}
		COMMAND ${VENV_BIN_DIR}/python ${NANOPB_GENERATOR}
		-q
		-D ${PROTO_OUTPUT_DIR}
		-I ${CMAKE_SOURCE_DIR}/proto
		-I ${CMAKE_SOURCE_DIR}/lib/nanopb/generator/proto
		${CMAKE_SOURCE_DIR}/proto/enums.proto
		COMMAND ${VENV_BIN_DIR}/python ${NANOPB_GENERATOR}
		-q
		-D ${PROTO_OUTPUT_DIR}
		-I ${CMAKE_SOURCE_DIR}/proto
		-I ${CMAKE_SOURCE_DIR}/lib/nanopb/generator/proto
		${CMAKE_SOURCE_DIR}/proto/input_enums.proto
		COMMAND ${VENV_BIN_DIR}/python ${NANOPB_GENERATOR}
		-q
		-D ${PROTO_OUTPUT_DIR}
		-I ${CMAKE_SOURCE_DIR}/proto
		-I ${CMAKE_SOURCE_DIR}/lib/nanopb/generator/proto
		${CMAKE_SOURCE_DIR}/proto/config.proto
		COMMAND ${VENV_BIN_DIR}/python ${NANOPB_GENERATOR}
		-q
		-D ${PROTO_OUTPUT_DIR}
		-I ${CMAKE_SOURCE_DIR}/proto
		-I ${CMAKE_SOURCE_DIR}/lib/nanopb/generator/proto
		${CMAKE_SOURCE_DIR}/proto/device.proto
		COMMAND ${VENV_BIN_DIR}/python ${NANOPB_GENERATOR}
		-q
		-D ${PROTO_OUTPUT_DIR}
		-I ${CMAKE_SOURCE_DIR}/proto
		-I ${CMAKE_SOURCE_DIR}/lib/nanopb/generator/proto
		${CMAKE_SOURCE_DIR}/proto/input.proto
		COMMAND ${VENV_BIN_DIR}/python ${NANOPB_GENERATOR}
		-q
		-D ${PROTO_OUTPUT_DIR}
		-I ${CMAKE_SOURCE_DIR}/proto
		-I ${CMAKE_SOURCE_DIR}/lib/nanopb/generator/proto
		${CMAKE_SOURCE_DIR}/proto/events.proto
		COMMAND ${VENV_BIN_DIR}/python ${NANOPB_GENERATOR}
		-q
		-D ${PROTO_OUTPUT_DIR}
		-I ${CMAKE_SOURCE_DIR}/proto
		-I ${CMAKE_SOURCE_DIR}/lib/nanopb/generator/proto
		${CMAKE_SOURCE_DIR}/proto/commands.proto
		OUTPUT ${PROTO_OUTPUT_DIR}/input.pb.c ${PROTO_OUTPUT_DIR}/input.pb.h ${PROTO_OUTPUT_DIR}/device.pb.c ${PROTO_OUTPUT_DIR}/device.pb.h ${PROTO_OUTPUT_DIR}/config.pb.c ${PROTO_OUTPUT_DIR}/config.pb.h ${PROTO_OUTPUT_DIR}/input_enums.pb.c ${PROTO_OUTPUT_DIR}/input_enums.pb.h ${PROTO_OUTPUT_DIR}/enums.pb.c ${PROTO_OUTPUT_DIR}/enums.pb.h ${PROTO_OUTPUT_DIR}/events.pb.c ${PROTO_OUTPUT_DIR}/events.pb.h  ${PROTO_OUTPUT_DIR}/commands.pb.c ${PROTO_OUTPUT_DIR}/commands.pb.h
		COMMENT "Compiling enums.proto and config.proto"
	)
endfunction()