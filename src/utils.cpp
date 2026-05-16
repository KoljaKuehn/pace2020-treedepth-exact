#include "utils.hpp"

namespace sms {
int Log::log_level_ = 0;
void Log::SetLogLevel(int lvl) {
  log_level_ = lvl;
}
} // namespace sms
