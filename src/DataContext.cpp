// 由AI(DeepSeek V4 Flash)生成，可能不完整或有错误，请自行检查和修改
#include "DataContext.h"

shared_ptr<DataContext> DataContext::s_instance = nullptr;

shared_ptr<DataContext> DataContext::instance() {
    if (!s_instance) {
        struct make_shared_enabler : public DataContext {};
        s_instance = make_shared<make_shared_enabler>();
    }
    return s_instance;
}

void DataContext::set(const string& key, const DataValue& value) {
    m_data[key] = value;

    auto it = m_watchers.find(key);
    if (it != m_watchers.end()) {
        for (auto& w : it->second) {
            w(value);
        }
    }
}

DataValue DataContext::get(const string& key) const {
    auto it = m_data.find(key);
    if (it != m_data.end()) return it->second;
    return DataValue();
}

bool DataContext::has(const string& key) const {
    return m_data.find(key) != m_data.end();
}

void DataContext::watch(const string& key, Watcher watcher) {
    m_watchers[key].push_back(move(watcher));
}

void DataContext::unwatchAll(const string& key) {
    m_watchers.erase(key);
}

void DataContext::unwatchAll() {
    m_watchers.clear();
}

void DataContext::clear() {
    m_data.clear();
    m_watchers.clear();
}
