#include "filename_prefix.hpp"

std::string generate_prefix() {
  char hostname[256];
  gethostname(hostname, 256);
  int pid = (int)getpid();
  return std::string(hostname) + "-" + std::to_string(pid);
}