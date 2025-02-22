/*
 * MongoCursor.cpp
 *
 *  Created on: Oct 31, 2012
 *      Author: lion
 */

#include "object.h"
#include "MongoCursor.h"
#include "encoding_bson.h"
#include "ifs/util.h"
#include "Fiber.h"

namespace fibjs {

result_t MongoCursor::_initCursor(MongoDB* db, AsyncEvent* ac)
{
    if (ac->isSync())
        return CHECK_ERROR(CALL_E_NOSYNC);

    m_cursor = new cursor;
    m_cursor->m_db = db;

    mongo_cursor_init(m_cursor, (m_cursor->m_db)->m_conn, m_ns.c_str());
    mongo_cursor_set_query(m_cursor, &m_bbq);
    mongo_cursor_set_fields(m_cursor, &m_bbp);
    return 0;
}

result_t MongoCursor::_nextCursor(int32_t& retVal, AsyncEvent* ac)
{
    if (ac->isSync())
        return CHECK_ERROR(CALL_E_NOSYNC);

    retVal = mongo_cursor_next(m_cursor);
    return 0;
}

MongoCursor::MongoCursor(MongoDB* db, const exlib::string& ns,
    const exlib::string& name, v8::Local<v8::Object> query,
    v8::Local<v8::Object> projection)
{
    Isolate* isolate = holder();

    m_state = CUR_NONE;

    m_ns = ns;
    m_name = name;

    v8::Local<v8::Value> _query;
    util_base::clone(query, _query);
    m_query.Reset(isolate->m_isolate, v8::Local<v8::Object>::Cast(_query)->Clone());

    encodeObject(isolate, &m_bbp, projection);

    ac__initCursor(db);

    m_bInit = false;
    m_bSpecial = false;
}

static result_t _close(MongoCursor::cursor* cur)
{
    cur->m_db->m_lock.lock();
    mongo_cursor_destroy(cur);
    cur->m_db->m_lock.unlock();
    delete cur;
    return 0;
}

MongoCursor::~MongoCursor()
{
    m_query.Reset();

    asyncCall(_close, m_cursor);

    if (m_bInit)
        bson_destroy(&m_bbq);
    bson_destroy(&m_bbp);
}

void MongoCursor::ensureSpecial()
{
    if (!m_bSpecial) {
        Isolate* isolate = holder();
        v8::Local<v8::Context> context = isolate->context();
        v8::Local<v8::Object> o = v8::Object::New(isolate->m_isolate);

        o->Set(context, isolate->NewString("query"), m_query.Get(isolate->m_isolate)).IsJust();
        m_query.Reset();

        m_query.Reset(isolate->m_isolate, o);
        m_bSpecial = true;
    }
}

result_t MongoCursor::hint(v8::Local<v8::Object> opts,
    obj_ptr<MongoCursor_base>& retVal)
{
    return _addSpecial("$hint", opts, retVal);
}

result_t MongoCursor::limit(int32_t size, obj_ptr<MongoCursor_base>& retVal,
    AsyncEvent* ac)
{
    if (m_bInit)
        return CHECK_ERROR(CALL_E_INVALID_CALL);

    if (ac->isSync())
        return CHECK_ERROR(CALL_E_NOSYNC);

    mongo_cursor_set_limit(m_cursor, size);

    retVal = this;
    return 0;
}

result_t MongoCursor::count(bool applySkipLimit, int32_t& retVal)
{
    bson bbq;

    bson_init(&bbq);
    bson_append_string(&bbq, "count", m_name.c_str());

    Isolate* isolate = holder();
    v8::Local<v8::Context> context = isolate->context();
    if (m_bSpecial)
        encodeValue(isolate, &bbq, "query",
            JSValue(m_query.Get(isolate->m_isolate)->Get(context, isolate->NewString("query"))));
    else
        encodeValue(isolate, &bbq, "query", m_query.Get(isolate->m_isolate));

    if (applySkipLimit) {
        if (m_cursor->limit)
            bson_append_int(&bbq, "limit", m_cursor->limit);
        if (m_cursor->skip)
            bson_append_int(&bbq, "skip", m_cursor->skip);
    }

    bson_finish(&bbq);

    v8::Local<v8::Object> res;

    result_t hr = m_cursor->m_db->bsonHandler(&bbq, res);
    if (hr < 0)
        return hr;

    retVal = isolate->toInt32Value(JSValue(res->Get(context, isolate->NewString("n"))));

    return 0;
}

result_t MongoCursor::forEach(v8::Local<v8::Function> func)
{
    result_t hr;
    v8::Local<v8::Object> o;
    Isolate* isolate = holder();
    v8::Local<v8::Context> context = isolate->context();

    while ((hr = next(o)) != CALL_RETURN_NULL && hr >= 0) {
        v8::EscapableHandleScope handle_scope(isolate->m_isolate);
        v8::Local<v8::Value> a = o;
        v8::Local<v8::Value> v = func->Call(context, v8::Undefined(isolate->m_isolate), 1, &a).FromMaybe(v8::Local<v8::Value>());
        if (v.IsEmpty())
            return CALL_E_JAVASCRIPT;
    }

    return hr < 0 ? hr : 0;
}

result_t MongoCursor::map(v8::Local<v8::Function> func,
    v8::Local<v8::Array>& retVal)
{
    result_t hr;
    Isolate* isolate = holder();
    v8::Local<v8::Context> context = isolate->context();
    v8::Local<v8::Object> o;
    v8::Local<v8::Array> as = v8::Array::New(isolate->m_isolate);
    int32_t n = 0;

    while ((hr = next(o)) != CALL_RETURN_NULL && hr >= 0) {
        v8::EscapableHandleScope handle_scope(isolate->m_isolate);
        v8::Local<v8::Value> a = o;
        v8::Local<v8::Value> v = func->Call(context, v8::Undefined(isolate->m_isolate), 1, &a).FromMaybe(v8::Local<v8::Value>());
        if (v.IsEmpty())
            return CALL_E_JAVASCRIPT;

        as->Set(context, n, handle_scope.Escape(v)).IsJust();
        n++;
    }

    retVal = as;
    return hr < 0 ? hr : 0;
}

result_t MongoCursor::hasNext(bool& retVal)
{
    if (!m_bInit) {
        result_t hr;
        Isolate* isolate = holder();

        hr = encodeObject(isolate, &m_bbq, m_query.Get(isolate->m_isolate));
        if (hr < 0)
            return hr;

        m_bInit = true;
    }

    if (m_state == CUR_NONE) {
        int32_t res;
        ac__nextCursor(res);
        m_state = (res == MONGO_OK) ? CUR_DATA : CUR_NODATA;
    }

    retVal = m_state == CUR_DATA;

    return CHECK_ERROR(m_cursor->m_db->error());
}

result_t MongoCursor::next(v8::Local<v8::Object>& retVal)
{
    bool has;
    result_t hr = hasNext(has);
    if (hr < 0)
        return hr;

    if (!has)
        return CALL_RETURN_NULL;

    hr = decodeObject(holder(), mongo_cursor_bson(m_cursor), retVal);
    if (hr < 0)
        return hr;

    m_state = CUR_NONE;

    return 0;
}

result_t MongoCursor::size(int32_t& retVal)
{
    return count(true, retVal);
}

result_t MongoCursor::skip(int32_t num, obj_ptr<MongoCursor_base>& retVal,
    AsyncEvent* ac)
{
    if (m_bInit)
        return CHECK_ERROR(CALL_E_INVALID_CALL);

    if (ac->isSync())
        return CHECK_ERROR(CALL_E_NOSYNC);

    mongo_cursor_set_skip(m_cursor, num);

    retVal = this;
    return 0;
}

result_t MongoCursor::sort(v8::Local<v8::Object> opts,
    obj_ptr<MongoCursor_base>& retVal)
{
    return _addSpecial("orderby", opts, retVal);
}

result_t MongoCursor::_addSpecial(const char* name, v8::Local<v8::Value> opts,
    obj_ptr<MongoCursor_base>& retVal)
{
    if (m_bInit)
        return CHECK_ERROR(CALL_E_INVALID_CALL);

    ensureSpecial();
    Isolate* isolate = holder();
    v8::Local<v8::Context> context = isolate->context();
    m_query.Get(isolate->m_isolate)->Set(context, isolate->NewString(name), opts).IsJust();

    retVal = this;
    return 0;
}

result_t MongoCursor::toArray(v8::Local<v8::Array>& retVal)
{
    Isolate* isolate = holder();
    v8::Local<v8::Context> context = isolate->context();
    result_t hr;
    v8::Local<v8::Object> o;
    v8::Local<v8::Array> as = v8::Array::New(isolate->m_isolate);
    int32_t n = 0;

    while ((hr = next(o)) != CALL_RETURN_NULL && hr >= 0) {
        as->Set(context, n, o).IsJust();
        n++;
    }

    if (hr < 0)
        return hr;

    retVal = as;
    return 0;
}

result_t MongoCursor::toJSON(exlib::string key, v8::Local<v8::Value>& retVal)
{
    result_t hr;
    v8::Local<v8::Array> as;

    hr = toArray(as);
    if (hr < 0)
        return hr;

    retVal = as;
    return 0;
}

} /* namespace fibjs */
