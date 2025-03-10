#ifndef INIT_PARSER_H
#define INIT_PARSER_H

#include <string>
#include <vector>

namespace minimal_systems {
namespace init {

/**
 * @brief Parses initialization files in the specified directory.
 *
 * This function iterates over the files in the given directory path and
 * processes each regular file found.
 *
 * @param dir_path The path of the directory to parse.
 * @return true if parsing succeeds, false if the directory does not exist.
 */
bool parse_init_files(const std::string& dir_path);

/**
 * @brief Checks if a file exists.
 *
 * This function checks for the existence of a file at the given path.
 *
 * @param filename The path to the file.
 * @return true if the file exists, false otherwise.
 */
bool file_exists(const std::string& filename);

/**
 * @brief Performs recovery-specific initialization.
 *
 * This function handles initialization in recovery mode, including parsing
 * the `init.rc` file.
 *
 * @return true if recovery initialization succeeds, false otherwise.
 */
bool recovery_init();

/**
 * @brief Main initialization entry point.
 *
 * This function checks the boot mode and performs the appropriate
 * initialization. If in recovery mode, it executes `recovery_init`. Otherwise,
 * it parses all directories specified in `init_dirs`.
 *
 * @return true if initialization succeeds, false otherwise.
 */
bool parse_init();

}  // namespace init
}  // namespace minimal_systems

#endif  // INIT_PARSER_H
