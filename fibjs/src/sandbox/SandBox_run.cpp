/*
 * SandBox_run.cpp
 *
 *  Created on: Oct 22, 2012
 *      Author: lion
 */

#include "object.h"
#include "SandBox.h"
#include "Buffer.h"
#include "path.h"
#include "options.h"
#include "ifs/global.h"

namespace fibjs {

result_t SandBox::run_main(exlib::string fname, v8::Local<v8::Array> argv)
{
    result_t hr;
    obj_ptr<Buffer_base> bin;

    if (fname[0] == '-' && fname[1] == '-') {
        int32_t i;
        exlib::string tmp("opt_tools/");
        tmp += fname.c_str() + 2;

        for (i = 0; opt_tools[i].name && qstrcmp(opt_tools[i].name, tmp.c_str()); i++)
            ;
        opt_tools[i].getDate(bin);
    } else {
        bool isAbs;
        exlib::string rname;

        path_base::isAbsolute(fname, isAbs);
        if (!isAbs) {
            rname = fname;
            os_resolve(fname);
        } else
            path_base::normalize(fname, fname);

        hr = resolveFile(fname, bin, NULL);
        if (hr < 0) {
            if (isAbs)
                return hr;

            fname = "node_modules/.bin/" + rname;
            os_resolve(fname);

            hr = resolveFile(fname, bin, NULL);
            if (hr < 0)
                return hr;
        }
    }

    obj_ptr<ExtLoader> l;
    hr = get_loader(fname, l);
    if (hr < 0)
        return hr;

    Context context(this, fname);

    std::vector<ExtLoader::arg> extarg(1);
    extarg[0] = ExtLoader::arg("__argv", argv);

    return l->run_script(&context, bin, fname, extarg, true);
}

result_t SandBox::run_worker(exlib::string fname, Worker_base* master)
{
    result_t hr;
    bool isAbs;

    path_base::isAbsolute(fname, isAbs);
    if (!isAbs)
        return CHECK_ERROR(Runtime::setError("SandBox: Invalid file name."));
    path_base::normalize(fname, fname);

    obj_ptr<Buffer_base> bin;
    hr = resolveFile(fname, bin, NULL);
    if (hr < 0)
        return hr;

    obj_ptr<ExtLoader> l;
    hr = get_loader(fname, l);
    if (hr < 0)
        return hr;

    Context context(this, fname);

    std::vector<ExtLoader::arg> extarg(1);
    extarg[0] = ExtLoader::arg("Master", master->wrap());

    return l->run_script(&context, bin, fname, extarg, false);
}

result_t SandBox::run(exlib::string fname, v8::Local<v8::Array> argv)
{
    Scope _scope(this);

    result_t hr;
    bool isAbs;

    path_base::isAbsolute(fname, isAbs);
    if (!isAbs)
        return CHECK_ERROR(Runtime::setError("SandBox: Invalid file name."));
    path_base::normalize(fname, fname);

    obj_ptr<Buffer_base> bin;
    hr = resolveFile(fname, bin, NULL);
    if (hr < 0)
        return hr;

    obj_ptr<ExtLoader> l;
    hr = get_loader(fname, l);
    if (hr < 0)
        return hr;

    Context context(this, fname);
    std::vector<ExtLoader::arg> extarg(1);
    extarg[0] = ExtLoader::arg("__argv", argv);

    return l->run_script(&context, bin, fname, extarg, false);
}
}
