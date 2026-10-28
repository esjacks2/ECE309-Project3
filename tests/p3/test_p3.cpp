// tests/p3/test_p3.cpp — YOUR P3 TEST SUITE.
//
// The five tests below are worked examples. Keep them, and add your own
// until you have AT LEAST 15 (spec §6 lists what must be covered).
// Name every test <area>_<what>: reg_ for the registry, ws_ for the
// workspace, calc_ for the calculator.
//
// Build & run:   cmake --build build && ./build/test_p3
// One area:      ./build/test_p3 reg_        (names starting with "reg_")
// One test:      ./build/test_p3 =reg_add_and_find

#include "../mini_test.h"
#include "../test_helpers.h"     // TempDir, write_text, args1, ...

#include "tools/calculator.h"
#include "tools/tool_registry.h"
#include "tools/workspace.h"

#include <cstdio>
#include <memory>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

// A tool whose name you choose — use it to fill the registry in tests.
class NamedTool : public Tool {
public:
    explicit NamedTool(std::string n) : n_(std::move(n)), schema_(JsonValue::object()) {}
    std::string name() const override { return n_; }
    std::string description() const override { return "test tool"; }
    const JsonValue& schema() const override { return schema_; }
    ToolResult invoke(const JsonValue&, ToolContext&) override { return ToolResult::success(n_); }
private:
    std::string n_;
    JsonValue   schema_;   // an empty JSON object: {}
};

// Tokenize with the provided tokenizer, then run YOUR calc_evaluate.
bool eval(const std::string& expr, double* value, std::string* error) {
    std::vector<CalcToken> tokens;
    if (!calc_tokenize(expr, &tokens, error)) return false;
    return calc_evaluate(tokens, value, error);
}

}  // namespace

// ---------------------------------------------------------------- examples

TEST(reg_add_and_find) {
    ToolRegistry r;
    auto t = std::make_unique<NamedTool>("alpha");
    Tool* raw = t.get();                        // remember the address before moving
    REQUIRE(r.add(std::move(t)));
    CHECK(r.size() == 1);
    CHECK(r.find("alpha") == raw);              // same object, not a copy
    CHECK(r.find("beta") == nullptr);
}

TEST(reg_duplicate_keeps_original) {
    ToolRegistry r;
    auto first = std::make_unique<NamedTool>("alpha");
    Tool* original = first.get();
    REQUIRE(r.add(std::move(first)));
    CHECK(!r.add(std::make_unique<NamedTool>("alpha")));
    CHECK(r.find("alpha") == original);
    CHECK(r.size() == 1);
}

TEST(ws_rejects_parent_escape) {
    TempDir d;
    Workspace ws(d.path());
    fs::path out;
    std::string reason;
    REQUIRE(ws.resolve("notes/todo.txt", &out, &reason));   // sanity: accepts *something*
    CHECK(out == ws.root() / "notes" / "todo.txt");
    CHECK(!ws.resolve("../etc/passwd", &out, &reason));
    CHECK(!reason.empty());
}

TEST(calc_precedence) {
    double v = 0;
    std::string err;
    REQUIRE(eval("2 + 3 * 4", &v, &err));
    CHECK(v == 14);
}

TEST(calc_division_by_zero) {
    double v = 0;
    std::string err;
    CHECK(!eval("1 / 0", &v, &err));
    CHECK(err == "division by zero");
}

// ------------------------------------------------------------- your tests
//
// Still to write (spec §6 has the full list). Suggested names:
//   reg_null_rejected, reg_for_each_visits_each_once
//   reg_10000_tools               REQUIRED: print bucket_count / load_factor /
//                                 max_probe_length, check the bound
//   reg_alternative_hash          REQUIRED: longest chain of a weak hash
//   ws_<case>                     REQUIRED: every row of the table in spec §4.4
//   calc_<case>                   left-to-right, parentheses, negatives

TEST(reg_null_rejected) {
    ToolRegistry r;
    CHECK(!r.add(nullptr));
    CHECK(r.size() == 0);
}

TEST(reg_for_each_visits_all) {
    ToolRegistry r;
    REQUIRE(r.add(std::make_unique<NamedTool>("a")));
    REQUIRE(r.add(std::make_unique<NamedTool>("b")));
    REQUIRE(r.add(std::make_unique<NamedTool>("c")));
    int count = 0;
    r.for_each([&](const Tool&) { ++count; });
    CHECK(count == 3);
}

TEST(reg_scale_10000) {
    ToolRegistry r;
    Tool* first = nullptr;
    for (int i = 0; i < 10000; ++i) {
        REQUIRE(r.add(std::make_unique<NamedTool>("tool_" + std::to_string(i))));
        if (i == 0) first = r.find("tool_0");
        CHECK(r.load_factor() <= 1.0);
    }
    for (int i = 0; i < 10000; ++i)
        CHECK(r.find("tool_" + std::to_string(i)) != nullptr);
    CHECK(r.find("tool_0") == first);
    CHECK(r.max_probe_length() <= 64);
    std::printf("buckets=%zu load=%.4f longest=%zu\n", r.bucket_count(), r.load_factor(), r.max_probe_length());
}

TEST(reg_weak_hash_comparison) {
    ToolRegistry r;
    for (int i = 0; i < 10000; ++i)
        REQUIRE(r.add(std::make_unique<NamedTool>("tool_" + std::to_string(i))));
    std::vector<int> counts(r.bucket_count(), 0);
    for (int i = 0; i < 10000; ++i) {
        std::string key = "tool_" + std::to_string(i);
        std::size_t weak = static_cast<unsigned char>(key[0]) + key.size();
        ++counts[weak % counts.size()];
    }
    int longest = 0;
    for (int count : counts) if (count > longest) longest = count;
    std::printf("weak_hash_longest=%d real_hash_longest=%zu\n", longest, r.max_probe_length());
    CHECK(longest > static_cast<int>(r.max_probe_length()));
}

TEST(ws_accepts_normalized) {
    TempDir d;
    Workspace ws(d.path());
    fs::path out;
    std::string reason;
    for (const auto& entry : std::vector<std::pair<std::string, std::string>>{
        {"notes/todo.txt", "notes/todo.txt"},
        {"./notes//todo.txt", "notes/todo.txt"},
        {".", "."},
        {"a/../b", "b"},
        {"a/..", "."},
        {"new/dir/file.txt", "new/dir/file.txt"},
        {"my..notes.txt", "my..notes.txt"}
    }) {
        REQUIRE(ws.resolve(entry.first, &out, &reason));
        if (entry.second == ".") CHECK(out == ws.root());
        else CHECK(out.lexically_normal() == (ws.root() / entry.second).lexically_normal());
    }
}

TEST(ws_rejects_invalid) {
    TempDir d;
    Workspace ws(d.path());
    fs::path out;
    std::string reason;
    std::vector<std::string> paths = {
        "", "/etc/passwd", "..", "../etc/passwd", "a/../../b",
        "notes/../../ws/notes/todo.txt", std::string("notes") + '\0' + "/x"
    };
    for (const auto& path : paths) {
        CHECK(!ws.resolve(path, &out, &reason));
        CHECK(!reason.empty());
    }
}

TEST(calc_left_associative) {
    double v = 0;
    std::string err;
    REQUIRE(eval("10 - 4 - 3", &v, &err));
    CHECK(v == 3);
    REQUIRE(eval("100 / 10 / 5", &v, &err));
    CHECK(v == 2);
}

TEST(calc_parentheses) {
    double v = 0;
    std::string err;
    REQUIRE(eval("(2 + 3) * 4", &v, &err));
    CHECK(v == 20);
    REQUIRE(eval("2 * (3 + 4) - 5", &v, &err));
    CHECK(v == 9);
}

TEST(calc_negative) {
    double v = 0;
    std::string err;
    REQUIRE(eval("-3 * -3", &v, &err));
    CHECK(v == 9);
}

TEST(calc_decimals) {
    double v = 0;
    std::string err;
    REQUIRE(eval("7 / 2", &v, &err));
    CHECK(v == 3.5);
    REQUIRE(eval("1.5 + 2.5", &v, &err));
    CHECK(v == 4);
}

TEST(calc_nested) {
    double v = 0;
    std::string err;
    REQUIRE(eval("((8 - 3) * (2 + 1)) / 5", &v, &err));
    CHECK(v == 3);
}

TEST(calc_division_by_zero_parentheses) {
    double v = 0;
    std::string err;
    CHECK(!eval("1 / (2 - 2)", &v, &err));
    CHECK(err == "division by zero");
}

int main(int argc, char** argv) { return mt::run_all(argc, argv); }
