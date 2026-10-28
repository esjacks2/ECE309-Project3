#include "tools/tool_registry.h"
#include <cstdint>
#include <string>
#include <utility>

ToolRegistry::ToolRegistry() : buckets_(16), count_(0) {}
ToolRegistry::~ToolRegistry() = default;

std::size_t ToolRegistry::hash(std::string_view key) noexcept {
    std::uint64_t h = 1469598103934665603ULL;
    for (unsigned char c : key) {
        h ^= c;
        h *= 1099511628211ULL;
    }
    return static_cast<std::size_t>(h);
}

bool ToolRegistry::add(std::unique_ptr<Tool> tool) {
    if (!tool) return false;
    std::string name = tool->name();
    if (find(name)) return false;
    if ((count_ + 1) * 4 > buckets_.size() * 3) {
        std::vector<std::vector<std::unique_ptr<Tool>>> next(buckets_.size() * 2);
        for (auto& bucket : buckets_) {
            for (auto& t : bucket) {
                std::size_t i = hash(t->name()) % next.size();
                next[i].push_back(std::move(t));
            }
        }
        buckets_ = std::move(next);
    }
    buckets_[hash(name) % buckets_.size()].push_back(std::move(tool));
    ++count_;
    return true;
}

Tool* ToolRegistry::find(std::string_view name) const {
    const auto& bucket = buckets_[hash(name) % buckets_.size()];
    for (const auto& t : bucket) {
        if (t->name() == name) return t.get();
    }
    return nullptr;
}

std::size_t ToolRegistry::size() const noexcept { return count_; }

void ToolRegistry::for_each(const std::function<void(const Tool&)>& fn) const {
    for (const auto& bucket : buckets_)
        for (const auto& t : bucket) fn(*t);
}

std::size_t ToolRegistry::bucket_count() const noexcept { return buckets_.size(); }

double ToolRegistry::load_factor() const noexcept {
    return static_cast<double>(count_) / static_cast<double>(buckets_.size());
}

std::size_t ToolRegistry::max_probe_length() const {
    std::size_t n = 0;
    for (const auto& bucket : buckets_) if (bucket.size() > n) n = bucket.size();
    return n;
}
