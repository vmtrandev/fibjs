#!/usr/local/bin/fibjs

var test = require("test");
test.setup();

global.full_test = process.argv.indexOf('--full') >= 0;

const CI_SUBPROCESS_CHECK = !!process.env.CI_SUBPROCESS_CHECK;

run("./assert_test.js");
run("./test_test.js");
run("./class_test.js");
run("./console_test.js");
run("./icu_test.js");
run("./punycode_test.js");
run("./timer_test.js");
run("./buffer_test.js");
run("./path_test.js");
run("./util_test.js");
run("./promise_test.js");
run("./xml_test.js");

if (process.env.CI)
    run("./xml_suite.js");

run("./coroutine_test.js");
run("./fibmod_test.js");
run("./trigger_test.js");
run("./lock_test.js");
run("./fs_test.js");
run("./fswatch_test.js");
run("./ms_test.js");
run("./io_test.js");
run("./os_test.js");
run("./process_test.js");
run("./child_process_test.js");
run("./encoding_test.js");
run("./json_test.js");
run("./module_test.js");
run("./dns_test.js");
run("./net_test.js");
run("./dgram_test.js");
run("./buffered_test.js");
run("./hash_test.js");
run("./crypto_test.js");
run("./ssl_test.js");
run("./string_decoder_test.js");
run("./url_test.js");
run("./querystring_test.js");
run("./http_test.js");
run("./mq_test.js");
run("./gui_test.js");
run("./registry_test.js");
run("./uuid_test.js");
run("./gd_test.js");
run("./zlib_test.js");
run("./unzip_test.js");
run("./ws_test.js");
run("./vm_test.js");
run("./db_test.js");
run("./vec_test.js");
run("./wasm_test.js");

if (global.full_test) {
    run("./mongo_test.js");
    run("./redis_test.js");
    run("./tty_test.js");
}

run("./selfzip_test.js");

run("./profiler_test.js");

run("./v8_test.js");

run("./getter_throw.js")

run("./internal_test/helpers.js")
run("./opt_tools_test/index.js")

test.run();