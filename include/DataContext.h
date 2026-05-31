// 由AI(DeepSeek V4 Flash)生成，可能不完整或有错误，请自行检查和修改
#ifndef DataContextH
#define DataContextH

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <functional>
#include <variant>

using namespace std;

class DataValue {
public:
    using Variant = variant<string, double, bool>;

    DataValue() : m_val("") {}
    DataValue(const string& v) : m_val(v) {}
    DataValue(double v) : m_val(v) {}
    DataValue(bool v) : m_val(v) {}
    DataValue(int v) : m_val((double)v) {}

    bool isString() const { return holds_alternative<string>(m_val); }
    bool isNumber() const { return holds_alternative<double>(m_val); }
    bool isBool() const   { return holds_alternative<bool>(m_val); }

    string  asString() const { return isString() ? get<string>(m_val) : ""; }
    double  asDouble() const { return isNumber() ? get<double>(m_val) : 0.0; }
    bool    asBool() const   { return isBool()   ? get<bool>(m_val)   : false; }

private:
    Variant m_val;
};

class DataContext {
public:
    using Watcher = function<void(const DataValue&)>;

    static shared_ptr<DataContext> instance();

    void set(const string& key, const DataValue& value);
    DataValue get(const string& key) const;
    bool has(const string& key) const;

    void watch(const string& key, Watcher watcher);
    void unwatchAll(const string& key);
    void unwatchAll();
    void clear();

private:
    DataContext() = default;
    friend shared_ptr<DataContext>; // allow make_shared

    static shared_ptr<DataContext> s_instance;

    unordered_map<string, DataValue> m_data;
    unordered_map<string, vector<Watcher>> m_watchers;
};

#endif
