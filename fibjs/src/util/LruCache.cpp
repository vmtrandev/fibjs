/*
 * LruCache.cpp
 *
 *  Created on: Dec 7, 2013
 *      Author: lion
 */

#include "object.h"
#include <LruCache.h>
#include <Event.h>

namespace fibjs {

result_t LruCache_base::_new(int32_t size, int32_t timeout,
    obj_ptr<LruCache_base>& retVal, v8::Local<v8::Object> This)
{
    retVal = new LruCache(size, timeout);
    return 0;
}

void LruCache::cleanup()
{
    std::map<exlib::string, _linkedNode>::iterator it;

    if (m_timeout > 0) {
        date_t t;
        t.now();

        while ((it = m_end) != m_datas.end() && t.diff(it->second.insert) > m_timeout)
            expire(it);
    }

    if (m_size > 0) {
        while ((int32_t)m_datas.size() > m_size)
            expire(m_end_lru);
    }
}

result_t LruCache::get_size(int32_t& retVal)
{
    cleanup();

    retVal = (int32_t)m_datas.size();
    return 0;
}

result_t LruCache::get_timeout(int32_t& retVal)
{
    retVal = m_timeout;
    return 0;
}

result_t LruCache::set_timeout(int32_t newVal)
{
    m_timeout = newVal;
    return 0;
}

result_t LruCache::clear()
{
    m_datas.clear();

    m_begin_lru = m_datas.end();
    m_end_lru = m_datas.end();

    m_begin = m_datas.end();
    m_end = m_datas.end();

    return 0;
}

result_t LruCache::has(exlib::string name, bool& retVal)
{
    cleanup();

    retVal = m_datas.find(name) != m_datas.end();
    return 0;
}

result_t LruCache::get(exlib::string name, v8::Local<v8::Value>& retVal)
{
    return get(name, v8::Local<v8::Function>(), retVal);
}

result_t LruCache::get(exlib::string name, v8::Local<v8::Function> updater,
    v8::Local<v8::Value>& retVal)
{
    static _linkedNode newNode;
    v8::Local<v8::Object> o = wrap();
    Isolate* isolate = holder();
    exlib::string sname(name);
    v8::Local<v8::Value> a = isolate->NewString(name);

    std::map<exlib::string, _linkedNode>::iterator find;

    cleanup();

    while (true) {
        obj_ptr<Event_base> e;

        find = m_datas.find(sname);
        if (find != m_datas.end())
            break;

        std::map<exlib::string, obj_ptr<Event_base>>::iterator padding;
        padding = m_paddings.find(sname);
        if (padding == m_paddings.end()) {
            if (updater.IsEmpty())
                return 0;

            e = new Event();
            padding = m_paddings.insert(std::pair<exlib::string, obj_ptr<Event_base>>(sname, e)).first;
            v8::Local<v8::Value> v = updater->Call(updater->GetCreationContextChecked(), o, 1, &a).FromMaybe(v8::Local<v8::Value>());
            m_paddings.erase(padding);
            e->set();

            if (v.IsEmpty())
                return CALL_E_JAVASCRIPT;

            if (!IsEmpty(v)) {
                std::pair<std::map<exlib::string, _linkedNode>::iterator, bool> ret;
                ret = m_datas.insert(std::pair<exlib::string, _linkedNode>(sname, newNode));
                if (ret.second) {
                    find = ret.first;
                    insert(find);

                    find->second.insert.now();
                    SetPrivate(name, v);
                }
            }

            retVal = v;
            return 0;
        }

        e = padding->second;

        {
            METHOD_NAME("LruCache.get");
            e->wait();
        }
    }

    update(find);
    retVal = GetPrivate(name);

    return 0;
}

result_t LruCache::set(exlib::string name, v8::Local<v8::Value> value)
{
    static _linkedNode newNode;
    std::map<exlib::string, _linkedNode>::iterator find = m_datas.find(name);

    if (find == m_datas.end()) {
        find = m_datas.insert(std::pair<exlib::string, _linkedNode>(name, newNode)).first;
        insert(find);
    } else {
        update(find);
        update_time(find);
    }

    find->second.insert.now();
    SetPrivate(name, value);

    cleanup();

    return 0;
}

inline result_t _map(LruCache* o, v8::Local<v8::Object> m,
    result_t (LruCache::*fn)(exlib::string name, v8::Local<v8::Value> value))
{
    v8::Local<v8::Context> context = m->GetCreationContextChecked();
    JSArray ks = m->GetPropertyNames(context);
    int32_t len = ks->Length();
    int32_t i;
    Isolate* isolate = o->holder();

    for (i = 0; i < len; i++) {
        JSValue k = ks->Get(context, i);
        (o->*fn)(isolate->toString(k), JSValue(m->Get(context, k)));
    }

    return 0;
}

result_t LruCache::set(v8::Local<v8::Object> map)
{
    return _map(this, map, &LruCache::set);
}

result_t LruCache::remove(exlib::string name)
{
    std::map<exlib::string, _linkedNode>::iterator find = m_datas.find(name);

    if (find == m_datas.end())
        return 0;

    remove(find);

    return 0;
}

result_t LruCache::isEmpty(bool& retVal)
{
    cleanup();

    retVal = m_datas.empty();
    return 0;
}

result_t LruCache::toJSON(exlib::string key, v8::Local<v8::Value>& retVal)
{
    cleanup();

    Isolate* isolate = holder();
    v8::Local<v8::Context> context = isolate->context();
    std::map<exlib::string, _linkedNode>::iterator it = m_begin_lru;
    v8::Local<v8::Object> obj = v8::Object::New(isolate->m_isolate);

    while (it != m_datas.end()) {
        v8::Local<v8::String> name = isolate->NewString(it->first);
        obj->Set(context, name, GetPrivate(it->first)).IsJust();;
        it = _instantiate(it->second.m_next);
    }

    retVal = obj;
    return 0;
}

} /* namespace fibjs */
