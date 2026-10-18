#pragma once
#include "circuit.hpp"
namespace thinkercad {
// One immutable connectivity snapshot shared by solving, diagnostics and UI.
// Net IDs are snapshot-local; persist Endpoints instead.
class NetTopology {
  public:
    explicit NetTopology(const CircuitDocument &document);
    int node(const Endpoint &endpoint) const {
        return parent_.at(index_.at(endpoint));
    }
    bool contains(const Endpoint &endpoint) const {
        return index_.contains(endpoint);
    }
    bool connected(const Endpoint &a, const Endpoint &b) const {
        return node(a) == node(b);
    }
    const std::map<Endpoint, int> &endpoints() const {
        return index_;
    }
    int representative(int index) const {
        return parent_.at(index);
    }
    const std::vector<Endpoint> &members(const Endpoint &endpoint) const {
        return members_.at(node(endpoint));
    }
    const std::vector<std::string> &wires(const Endpoint &endpoint) const;
    std::vector<Endpoint> preview(const Endpoint &a, const Endpoint &b) const;
    static std::string signature(const CircuitDocument &document);

  private:
    std::map<Endpoint, int> index_;
    std::vector<int> parent_;
    std::map<int, std::vector<Endpoint>> members_;
    std::map<int, std::vector<std::string>> wires_;
    int find(int a) {
        while (parent_[a] != a) {
            parent_[a] = parent_[parent_[a]];
            a = parent_[a];
        }
        return a;
    }
    void join(const Endpoint &a, const Endpoint &b) {
        int x = find(index_.at(a)), y = find(index_.at(b));
        if (x != y)
            parent_[y] = x;
    }
};
} // namespace thinkercad
