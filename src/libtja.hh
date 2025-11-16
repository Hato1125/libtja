#ifndef _LIBTJA_LIBTJA_HH
#define _LIBTJA_LIBTJA_HH

#if __cplusplus <= 202302L && defined(DEBUG_BUILD)
  #define __LIBTJA_DEBUGGING
#endif

#include <ranges>
#include <vector>
#include <cstdint>
#include <expected>
#include <string_view>

#ifdef __LIBTJA_DEBUGGING
  #include <locale>
  #include <codecvt>
#endif

#include <ttfs.hh>

namespace tja {
  constexpr std::uint8_t major = 1;
  constexpr std::uint8_t minor = 0;
  constexpr std::uint8_t patch = 0;

  struct error {
    enum class kind {
      unopened_block,
      unclosed_block,
    };

    kind kind;
    std::size_t line;
    std::size_t column;
  };

  namespace detail {
    namespace utils {
      inline bool first_eq(
        std::u32string_view str,
        std::u32string_view target
      ) noexcept {
        const auto ss = str.size();
        const auto ts = target.size();

        if (ss >= ts) {
          for (std::size_t i = 0; i < ts; ++i) {
            if (str[i] != target[i]) {
              return false;
            }
          }
          return true;
        }

        return false;
      }
    }

#ifdef __LIBTJA_DEBUGGING
    namespace debug {
      inline std::string u32_to_u8(std::u32string_view str32) {
        std::u32string temp(str32);
        std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> conv;
        return conv.to_bytes(temp);
      }
    }
#endif

    inline std::vector<std::u32string_view> split_lines(
      std::u32string_view text
    ) noexcept {
      auto lines_view = text
        | std::ranges::views::split(U'\n')
        | std::ranges::views::transform([](auto&& rng) {
            const auto first = &*rng.begin();
            const auto last  = &*rng.end();
            return std::u32string_view(first, std::distance(first, last));
          });

      return {lines_view.begin(), lines_view.end()};
    }

    inline std::expected<std::vector<std::size_t>, error> locate_sections(
      std::vector<std::u32string_view>& lines
    ) noexcept {
      std::vector<std::size_t> position = {};
      std::size_t size = lines.size();

      bool opened = false;

      for (std::size_t i = 0; i < size; ++i) {
        if (utils::first_eq(lines[i], U"#START")) {
          if (opened) [[unlikely]] {
            return std::unexpected(error{
              .kind = error::kind::unclosed_block,
              .line = i,
              .column = 0,
            });
          }

          opened = true;
          position.push_back(i);
        }

        if (utils::first_eq(lines[i], U"#END")) {
          if (!opened) [[unlikely]] {
            return std::unexpected(error{
              .kind = error::kind::unopened_block,
              .line = i,
              .column = 0,
            });
          }

          opened = false;
          position.push_back(i);
        }
      }

      if (opened) [[unlikely]] {
        return std::unexpected(error{
          .kind = error::kind::unclosed_block,
          .line = size - 1,
          .column = 0,
        });
      }

      return position;
    }
  }

  std::expected<ttfs::chart, error> parse(std::u32string_view text) noexcept {
    return {};
  }
}

#endif
