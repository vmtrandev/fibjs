/*
 * Fiber.h
 *
 *  Created on: Jul 23, 2012
 *      Author: lion
 */

#pragma once

#include "ifs/coroutine.h"
#include "ifs/Fiber.h"
#include "QuickArray.h"

namespace fibjs {

class JSFiber : public Fiber_base,
                public exlib::linkitem {
public:
    class EnterJsScope {
    public:
        EnterJsScope(JSFiber* fb = NULL, bool task = false);
        ~EnterJsScope();

        JSFiber* operator->()
        {
            return m_pFiber;
        }

    public:
        result_t m_hr;

    public:
        obj_ptr<JSFiber> m_pFiber;
        bool m_task;
        v8::Global<v8::Object> m_fiber;
        TryCatch try_catch;
    };

public:
    JSFiber()
    {
        m_id = holder()->m_fid++;
    }

    ~JSFiber()
    {
        clear();
    }

    FIBER_FREE();

public:
    // Fiber_base
    virtual result_t join();
    virtual result_t get_id(int64_t& retVal);
    virtual result_t get_stack(exlib::string& retVal);
    virtual result_t get_caller(obj_ptr<Fiber_base>& retVal);
    virtual result_t get_stack_usage(int32_t& retVal);

public:
    static void FiberProcRunJavascript(void* p);
    void start();

    void set_caller(Fiber_base* caller);

    static JSFiber* current();
    result_t js_invoke();

    template <typename T>
    void New(v8::Local<v8::Function> func, T* args, int32_t nArgCount,
        v8::Local<v8::Object> pThis)
    {
        Isolate* isolate = holder();
        int32_t i;

        m_argv.resize(nArgCount);
        for (i = 0; i < nArgCount; i++)
            m_argv[i].Reset(isolate->m_isolate, args[i]);
        m_func.Reset(isolate->m_isolate, func);
        m_this.Reset(isolate->m_isolate, pThis);

        start();
    }

    template <typename T>
    static result_t New(v8::Local<v8::Object> pThis, v8::Local<v8::Function> func,
        v8::Local<v8::Value>* args, int32_t argCount, obj_ptr<T>& retVal)
    {
        obj_ptr<JSFiber> fb = new JSFiber();
        fb->New(func, args, argCount, pThis);
        retVal = fb;

        return 0;
    }

    template <typename T>
    static result_t New(v8::Local<v8::Function> func, OptArgs args,
        obj_ptr<T>& retVal)
    {
        std::vector<v8::Local<v8::Value>> datas;
        args.GetData(datas);

        obj_ptr<JSFiber> fb = new JSFiber();
        fb->New(func, datas.data(), args.Length(), fb->wrap());
        retVal = fb;

        return 0;
    }

    result_t get_result(v8::Local<v8::Value>& retVal)
    {
        if (m_result.IsEmpty())
            return CALL_RETURN_NULL;

        retVal = m_result.Get(holder()->m_isolate);
        return 0;
    }

    void clear()
    {
        size_t i;

        for (i = 0; i < m_argv.size(); i++)
            m_argv[i].Reset();
        m_argv.resize(0);

        m_func.Reset();
        m_result.Reset();
        m_this.Reset();
    }

public:
    exlib::string m_message;
    exlib::Event m_quit;
    weak_ptr<Fiber_base> m_caller;
    exlib::Thread_base* m_bind_thread = NULL;

    const char* m_native_name = NULL;
    void* m_c_entry_fp_ = NULL;
    void* m_handler_ = NULL;
    bool m_termed = false;

private:
    int64_t m_id;
    v8::Global<v8::Function> m_func;
    QuickArray<v8::Global<v8::Value>> m_argv;
    v8::Global<v8::Value> m_result;
    v8::Global<v8::Object> m_this;
};

} /* namespace fibjs */
