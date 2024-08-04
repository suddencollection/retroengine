#include "program.hpp"

#include <cstdlib>
#include <iostream>

int main()
{
  try {
    Program p;
    p.run();
  }
  catch(std::exception& e) {
    std::cerr << e.what() << '.' << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
