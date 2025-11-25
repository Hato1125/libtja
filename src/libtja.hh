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
#include <unordered_set>

#ifdef __LIBTJA_DEBUGGING
  #include <locale>
  #include <codecvt>
#endif

#include <ttfs.hh>

namespace tja {
  constexpr std::uint8_t major = 1;
  constexpr std::uint8_t minor = 0;
  constexpr std::uint8_t patch = 0;

  enum class errkind {
    unopened_block,
    unclosed_block,
    empty_header_value,
    missing_header_format,
    empty_command_value,
    missing_command_format,
    int_convert_failed,
    float_convert_failed,
    no_implemented,
  };

  struct error {
    errkind kind;
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

      std::expected<int, errkind> convert_i32(
        std::u32string_view str
      ) noexcept {
        try {
          return std::stoi(reinterpret_cast<const char*>(str.data()));
        } catch (std::exception& _) {
          return std::unexpected(errkind::int_convert_failed);
        }
      }

      std::expected<float, errkind> convert_f32(
        std::u32string_view str
      ) noexcept {
        try {
          return std::stof(reinterpret_cast<const char*>(str.data()));
        } catch (std::exception& _) {
          return std::unexpected(errkind::float_convert_failed);
        }
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

    struct line {
      std::u32string_view str;
      std::size_t line;
    };

    inline std::vector<line> split_lines(
      std::u32string_view text
    ) noexcept {
      auto lines_view = text
        | std::ranges::views::split(U'\n')
        | std::ranges::views::transform([](auto&& rng) {
            const auto first = &*rng.begin();
            const auto last = &*rng.end();
            return std::u32string_view(first, std::distance(first, last));
          });

      const std::vector<std::u32string_view> lines {
        lines_view.begin(),
        lines_view.end()
      };
      const std::size_t lines_size = lines.size();

      std::vector<line> result;

      for (std::size_t i = 0; i < lines_size; ++i) {
        result.push_back({
          .str = lines[i],
          .line = i,
        });
      }

      return result;
    }

    inline std::vector<std::span<line>> split_courses(
      std::vector<line>& lines
    ) noexcept {
      std::size_t begin = 0;
      std::size_t size = lines.size();
      std::vector<std::span<line>> courses;

      for (auto i = 0; i < size; ++i) {
        if (utils::first_eq(lines[i].str, U"COURSE")) {
          if (i > begin) {
            courses.emplace_back(lines.begin() + begin, i - begin);
          }
          begin = i;
        }
      }

      if (begin < size) {
        courses.emplace_back(lines.begin() + begin, size - begin);
      }

      return courses;
    }

    struct name_value {
      std::u32string name;
      std::u32string value;
    };

    using header = name_value;
    std::expected<header, error> parse_header(const line& line) noexcept {
      if (!line.str.contains(':')) {
        return std::unexpected(error{
          .kind = errkind::missing_header_format,
          .line = line.line,
          .column = 0,
        });
      }

      std::vector<std::u32string_view> words;
      for (const auto word : line.str | std::views::split(U':')) {
        const auto first = &*word.begin();
        const auto last = &*word.end();
        words.emplace_back(first, std::distance(first, last));
      }

      if (words.size() > 1 && words[1].empty()) {
        return std::unexpected(error{
          .kind = errkind::empty_header_value,
          .line = line.line,
          .column = 0,
        });
      }

      return header{
        std::u32string(words[0]),
        std::u32string(words[1]),
      };
    }

    using command = name_value;
    std::expected<command, error> parse_command(const line& line) noexcept {
      if (line.str.begin() != line.str.end() && line.str[0] != U'#') {
        return std::unexpected(error{
          .kind = errkind::missing_command_format,
          .line = line.line,
          .column = 0,
        });
      }

      const auto no_chape_command = std::span(line.str)
        .subspan(1, line.str.size() - 1);

      std::vector<std::u32string_view> words;
      for(const auto word : no_chape_command | std::views::split(U' ')) {
        const auto first = &*word.begin();
        const auto last = &*word.end();
        words.emplace_back(first, std::distance(first, last));
      }

      if(!words.empty()) {
        if(words.size() > 1) {
          return command{
            std::u32string(words[0]),
            std::u32string(words[1]),
          };
        } else {
          const std::unordered_set<std::u32string_view> no_value_commands = {
            U"START",
            U"END",
            U"GOGOSTART",
            U"GOGOEND",
            U"BARLINEON",
            U"BARLINEOFF",
          };

          if(no_value_commands.contains(words[0])) {
            return command{
              std::u32string(words[0]),
            };
          }

          return std::unexpected(error{
            .kind = errkind::empty_command_value,
            .line = line.line,
            .column = 0,
          });
        }
      }

      return std::unexpected(error{
        .kind = errkind::missing_command_format,
        .line = line.line,
        .column = 0,
      });
    }

    ttfs::genre_type parse_genre_type(std::u32string_view value) noexcept {
      if (value == U"pop"
        || value == U"j-pop"
        || value == U"ポップス"
        || value == U"Jポップ") {
        return ttfs::genre_type::pop;
      }

      if (value == U"kid"
        || value == U"kids"
        || value == U"キッズ"
        || value == U"どうよう") {
        return ttfs::genre_type::kids;
      }

      if (value == U"namco"
        || value == U"ナムコ"
        || value == U"ナムコオリジナル") {
        return ttfs::genre_type::namco;
      }

      if (value == U"classic" || value == U"クラシック") {
        return ttfs::genre_type::classic;
      }

      if (value == U"variety" || value == U"バラエティー") {
        return ttfs::genre_type::variety;
      }

      if (value == U"game"
        || value == U"ゲーム"
        || value == U"ゲームミュージック") {
        return ttfs::genre_type::game;
      }

      if (value == U"vocaloid" || value == U"ボーカロイド") {
        return ttfs::genre_type::vocaloid;
      }

      if (value == U"anime" || value == U"アニメ") {
        return ttfs::genre_type::anime;
      }

      return ttfs::genre_type::unknown;
    }

    std::expected<ttfs::chart_headers, error> parse_common_headers(
      std::span<line>& lines
    ) noexcept {
      ttfs::chart_headers headers;

      for (const auto& line : lines) {
        if (auto rs = parse_header(line); rs) {
          if (rs->name == U"GENRE") {
            headers.genre = parse_genre_type(rs->value);
          } else if (rs->name == U"SUBGENRE") {
            headers.subgenre = parse_genre_type(rs->value);
          } else if (rs->name == U"WAVE") {
            headers.wave = std::move(rs->value);
          } else if (rs->name == U"TITLE") {
            headers.title = std::move(rs->value);
          } else if (rs->name == U"SUBTITLE") {
            headers.subtitle = std::move(rs->value);
          } else if (rs->name == U"BPM") {
            if (auto rs1 = utils::convert_f32(rs->value); rs1) {
              headers.bpm = *rs1;
            } else {
              return std::unexpected(error{
                .kind = rs1.error(),
                .line = line.line,
                .column = 0,
              });
            }
          } else if (rs->name == U"OFFSET") {
            if (auto rs1 = utils::convert_f32(rs->value); rs1) {
              headers.offset = *rs1;
            } else {
              return std::unexpected(error{
                .kind = rs1.error(),
                .line = line.line,
                .column = 0,
              });
            }
          }  else if (rs->name == U"DEMOSTART") {
            if (auto rs1 = utils::convert_f32(rs->value); rs1) {
              headers.demostart = *rs1;
            } else {
              return std::unexpected(error{
                .kind = rs1.error(),
                .line = line.line,
                .column = 0,
              });
            }
          } else {
            headers.extra.insert({ rs->name, rs->value });
          }
        } else {
          return std::unexpected(rs.error());
        }
      }

      return headers;
    }
  }

  std::expected<ttfs::chart, error> parse(std::u32string_view text) noexcept {
    auto lines = detail::split_lines(text);
    auto courses = detail::split_courses(lines);

    return std::unexpected(error{});
  }
}

#endif

