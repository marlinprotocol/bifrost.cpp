find_package(cxxopts QUIET)
if(NOT cxxopts_FOUND)
	message("-- cxxopts not found. Using internal cxxopts.")

	include(FetchContent)

	FetchContent_Declare(cxxopts
		GIT_REPOSITORY https://github.com/jarro2783/cxxopts.git
		GIT_TAG v2.2.0
	)
	FetchContent_MakeAvailable(cxxopts)
else()
	message("-- cxxopts found. Using system cxxopts.")
endif()
