#include <chrono>

namespace minimal_systems {
namespace base {

class boot_clock {
  public:
    using time_point = std::chrono::steady_clock::time_point;

    static time_point now() { return std::chrono::steady_clock::now(); }
};

}  // namespace base
}  // namespace minimal_systems
