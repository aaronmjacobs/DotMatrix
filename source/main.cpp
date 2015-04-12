#include "Constants.h"
#include "LogHelper.h"

#include <cstdlib>

int main(int argc, char *argv[]) {
   LOG_INFO(PROJECT_NAME << " " << VERSION_TYPE << " " << VERSION_MAJOR << "." << VERSION_MINOR << "."
      << VERSION_MICRO << "." << VERSION_BUILD);

   return EXIT_SUCCESS;
}
